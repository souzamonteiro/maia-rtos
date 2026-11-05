#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_display.h>
#include <maia/soc/esp32p4_regs.h>

/*
 * M5Stack Tab5 Clean-Room Bare-Metal MIPI-DSI Display Driver
 * Target: ESP32-P4 rev v1.3 + M5Stack Tab5 5.0" (720x1280 MIPI-DSI RGB565)
 * Runtime: ZERO FreeRTOS / ZERO ESP-IDF / ZERO Arduino / ZERO PSRAM
 */

#define TAB5_DISP_WIDTH      720U
#define TAB5_DISP_HEIGHT     1280U
#define TAB5_BAND_HEIGHT     128U
#define TAB5_NUM_BLOCKS      10U /* 10 * 128 = 1280 lines */
#define TAB5_BAND_WORDS64    ((TAB5_DISP_WIDTH * TAB5_BAND_HEIGHT * 2U) / 8U) /* 23040 64-bit words */
#define TAB5_TOTAL_WORDS64   ((TAB5_DISP_WIDTH * TAB5_DISP_HEIGHT * 2U) / 8U) /* 230400 64-bit words */

/* Base addresses */
#define GDMA_BASE            0x50081000UL
#define DSI_HOST_BASE        0x500A0000UL
#define DSI_BRG_BASE         0x500A0800UL
#define DSI_BRG_MEM_BASE     0x50105000UL
#define PMU_BASE             0x50115000UL
#define LP_CLKRST_BASE       0x50111000UL
#define HP_SYS_CLKRST_BASE   0x500E6000UL
/* Internal L2MEM Frameband Buffers (SRAM physical addresses in L2MEM_LOW) */
#define L2MEM_BAND0_PHYS     0x4FF20000UL
#define L2MEM_BAND1_PHYS     0x4FF4D000UL
#define L2MEM_BAND2_PHYS     0x4FF7A000UL

#define L2MEM_BAND0_CPU      ((volatile uint16_t *)(uintptr_t)L2MEM_BAND0_PHYS)
#define L2MEM_BAND1_CPU      ((volatile uint16_t *)(uintptr_t)L2MEM_BAND1_PHYS)
#define L2MEM_BAND2_CPU      ((volatile uint16_t *)(uintptr_t)L2MEM_BAND2_PHYS)

/* Colors (RGB565) */
#define COLOR_BLACK          0x0000U
#define COLOR_WHITE          0xFFFFU
#define COLOR_NAVY           0x0010U
#define COLOR_DARK_BLUE      0x0014U
#define COLOR_CYAN           0x07FFU
#define COLOR_GREEN          0x07E0U
#define COLOR_YELLOW         0xFFE0U
#define COLOR_RED            0xF800U
#define COLOR_MAGENTA        0xF81FU
#define COLOR_DARK_SLATE     0x0862U
#define COLOR_GRAY           0x8410U

/* DW_GDMA 64-byte aligned Link List Item structure */
typedef struct {
    uint32_t sar_lo;
    uint32_t sar_hi;
    uint32_t dar_lo;
    uint32_t dar_hi;
    uint32_t block_ts_lo;
    uint32_t reserved_14;
    uint32_t llp_lo;
    uint32_t llp_hi;
    uint32_t ctrl_lo;
    uint32_t ctrl_hi;
    uint32_t sstat;
    uint32_t dstat;
    uint32_t status_lo;
    uint32_t status_hi;
    uint32_t reserved_38;
    uint32_t reserved_3c;
} __attribute__((aligned(64))) dw_gdma_lli_t;

/* LLI table placed in BSS (.bss internal SRAM) */
static dw_gdma_lli_t s_lli_storage[TAB5_NUM_BLOCKS] __attribute__((aligned(64)));

/* Terminal text state (16 text rows of 90 columns across Band 0 and Band 1) */
#define TERM_COLS            90U
#define TERM_ROWS            16U

static char s_term_grid[TERM_ROWS][TERM_COLS];
static uint16_t s_term_fg[TERM_ROWS];
static uint16_t s_term_bg[TERM_ROWS];
static uint8_t s_term_cur_row = 3U;
static uint8_t s_term_cur_col = 0U;
static uint8_t s_display_ready = 0U;

/* Basic 8x16 font table for printable ASCII 32..126 (95 glyphs) */
#include "font_8x16.h"

static void draw_char_in_band(volatile uint16_t *band_fb, uint32_t col, uint32_t local_row,
                              char c, uint16_t fg, uint16_t bg) {
    if (col >= TERM_COLS || local_row >= 8U) {
        return;
    }
    uint32_t x0 = col * 8U;
    uint32_t y0 = local_row * 16U;
    const uint8_t *glyph = NULL;

    if (c >= 32 && c <= 126) {
        glyph = &g_font_8x16[(c - 32) * 16];
    }

    for (uint32_t py = 0; py < 16U; ++py) {
        uint8_t bits = glyph ? glyph[py] : 0U;
        uint32_t line_offset = (y0 + py) * TAB5_DISP_WIDTH + x0;
        for (uint32_t px = 0; px < 8U; ++px) {
            uint16_t color = ((bits >> (7U - px)) & 1U) ? fg : bg;
            band_fb[line_offset + px] = color;
        }
    }
}

static void term_render_row(uint8_t row) {
    volatile uint16_t *band = (row < 8U) ? L2MEM_BAND0_CPU : L2MEM_BAND1_CPU;
    uint32_t local_row = row % 8U;
    uint16_t fg = s_term_fg[row];
    uint16_t bg = s_term_bg[row];

    for (uint32_t col = 0; col < TERM_COLS; ++col) {
        char c = s_term_grid[row][col];
        if (c == '\0') {
            c = ' ';
        }
        draw_char_in_band(band, col, local_row, c, fg, bg);
    }
}

static void term_scroll(void) {
    /* Scroll output rows 3..14 up by 1 (preserving rows 0..2 header and row 15 prompt) */
    for (uint8_t r = 3U; r < 14U; ++r) {
        for (uint8_t c = 0; c < TERM_COLS; ++c) {
            s_term_grid[r][c] = s_term_grid[r + 1U][c];
        }
        s_term_fg[r] = s_term_fg[r + 1U];
        s_term_bg[r] = s_term_bg[r + 1U];
        term_render_row(r);
    }
    /* Clear line 14 */
    for (uint8_t c = 0; c < TERM_COLS; ++c) {
        s_term_grid[14U][c] = ' ';
    }
    s_term_fg[14U] = COLOR_WHITE;
    s_term_bg[14U] = COLOR_DARK_SLATE;
    term_render_row(14U);
    s_term_cur_row = 14U;
    s_term_cur_col = 0U;
}

void maia_display_putc(char c) {
    if (!s_display_ready) {
        return;
    }
    if (c == '\r') {
        s_term_cur_col = 0U;
        return;
    }
    if (c == '\n') {
        s_term_cur_col = 0U;
        if (s_term_cur_row < 14U) {
            s_term_cur_row++;
        } else {
            term_scroll();
        }
        return;
    }
    if (c == '\b') {
        if (s_term_cur_col > 0U) {
            s_term_cur_col--;
            s_term_grid[s_term_cur_row][s_term_cur_col] = ' ';
            term_render_row(s_term_cur_row);
        }
        return;
    }
    if (c == '\t') {
        uint8_t next_tab = (uint8_t)((s_term_cur_col + 4U) & ~3U);
        while (s_term_cur_col < next_tab && s_term_cur_col < TERM_COLS) {
            maia_display_putc(' ');
        }
        return;
    }
    if ((uint8_t)c < 32 || (uint8_t)c > 126) {
        return;
    }

    if (s_term_cur_col >= TERM_COLS) {
        s_term_cur_col = 0U;
        if (s_term_cur_row < 14U) {
            s_term_cur_row++;
        } else {
            term_scroll();
        }
    }

    s_term_grid[s_term_cur_row][s_term_cur_col] = c;
    term_render_row(s_term_cur_row);
    s_term_cur_col++;
}

void maia_display_puts(const char *str) {
    if (str == NULL) {
        return;
    }
    while (*str != '\0') {
        maia_display_putc(*str++);
    }
}

void maia_display_write(const char *buf, size_t len) {
    if (buf == NULL) {
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        maia_display_putc(buf[i]);
    }
}

static void disp_print_hex32(uint32_t val) {
    static const char kHex[] = "0123456789ABCDEF";
    char s[11];
    s[0] = '0';
    s[1] = 'x';
    for (int i = 7; i >= 0; --i) {
        s[2 + (7 - i)] = kHex[(val >> (i * 4)) & 0xFU];
    }
    s[10] = '\0';
    maia_console_puts(s);
}

static void dsi_phy_write_reg(uint8_t reg, uint8_t val) {
    MAIA_MMIO32(DSI_HOST_BASE + 0xB4UL) = 0U; /* clk=0, clear=0 */
    MAIA_MMIO32(DSI_HOST_BASE + 0xB8UL) = (1UL << 16U) | (uint32_t)reg;
    MAIA_MMIO32(DSI_HOST_BASE + 0xB4UL) = 2U; /* clk=1 */
    MAIA_MMIO32(DSI_HOST_BASE + 0xB4UL) = 0U; /* clk=0 */
    MAIA_MMIO32(DSI_HOST_BASE + 0xB8UL) = (uint32_t)val;
    MAIA_MMIO32(DSI_HOST_BASE + 0xB4UL) = 2U; /* clk=1 */
    MAIA_MMIO32(DSI_HOST_BASE + 0xB4UL) = 0U; /* clk=0 */
}

static void dsi_send_dcs_cmd(uint8_t cmd, const uint8_t *payload, size_t len) {
    if (len == 0U) {
        /* Wait until Command FIFO is not full (bit 1 is GEN_CMD_FULL) */
        for (uint32_t t = 0; t < 20000U; ++t) {
            if ((MAIA_MMIO32(DSI_HOST_BASE + 0x74UL) & (1UL << 1U)) == 0U) {
                break;
            }
            maia_udelay(2U);
        }
        /* DCS Short Write 0 parameters (DT=0x05, VC=0) */
        MAIA_MMIO32(DSI_HOST_BASE + 0x6CUL) = ((uint32_t)cmd << 8U) | 0x05UL;
    } else if (len == 1U) {
        for (uint32_t t = 0; t < 20000U; ++t) {
            if ((MAIA_MMIO32(DSI_HOST_BASE + 0x74UL) & (1UL << 1U)) == 0U) {
                break;
            }
            maia_udelay(2U);
        }
        /* DCS Short Write 1 parameter (DT=0x15, VC=0, LSB=cmd, MSB=payload[0]) */
        MAIA_MMIO32(DSI_HOST_BASE + 0x6CUL) = ((uint32_t)payload[0] << 16U) | ((uint32_t)cmd << 8U) | 0x15UL;
    } else {
        /* DCS Long Write (DT=0x39, VC=0). Word count = 1 (cmd) + len */
        uint32_t total_bytes = 1U + (uint32_t)len;
        uint8_t temp_buf[80];
        if (total_bytes > sizeof(temp_buf)) {
            return;
        }
        temp_buf[0] = cmd;
        for (size_t i = 0; i < len; ++i) {
            temp_buf[1U + i] = payload[i];
        }

        /* Write payload in 32-bit words into GEN_PLD_DATA (0x70) */
        uint32_t remain = total_bytes;
        const uint8_t *p = temp_buf;
        while (remain >= 4U) {
            uint32_t word = (uint32_t)p[0] | ((uint32_t)p[1] << 8U) |
                            ((uint32_t)p[2] << 16U) | ((uint32_t)p[3] << 24U);
            for (uint32_t t = 0; t < 20000U; ++t) {
                if ((MAIA_MMIO32(DSI_HOST_BASE + 0x74UL) & (1UL << 3U)) == 0U) { /* GEN_PLD_W_FULL */
                    break;
                }
                maia_udelay(2U);
            }
            MAIA_MMIO32(DSI_HOST_BASE + 0x70UL) = word;
            p += 4U;
            remain -= 4U;
        }
        if (remain > 0U) {
            uint32_t word = 0U;
            for (uint32_t i = 0; i < remain; ++i) {
                word |= ((uint32_t)p[i] << (i * 8U));
            }
            for (uint32_t t = 0; t < 20000U; ++t) {
                if ((MAIA_MMIO32(DSI_HOST_BASE + 0x74UL) & (1UL << 3U)) == 0U) {
                    break;
                }
                maia_udelay(2U);
            }
            MAIA_MMIO32(DSI_HOST_BASE + 0x70UL) = word;
        }

        /* Write packet header to GEN_HDR (0x6C): DT=0x39, VC=0, LSB=total & 0xFF, MSB=(total>>8)&0xFF */
        for (uint32_t t = 0; t < 20000U; ++t) {
            if ((MAIA_MMIO32(DSI_HOST_BASE + 0x74UL) & (1UL << 1U)) == 0U) {
                break;
            }
            maia_udelay(2U);
        }
        MAIA_MMIO32(DSI_HOST_BASE + 0x6CUL) = (((total_bytes >> 8U) & 0xFFUL) << 16U) |
                                              ((total_bytes & 0xFFUL) << 8U) | 0x39UL;
    }

    /* Wait until command FIFO and payload FIFO are empty */
    uint32_t wait_t = 0U;
    for (; wait_t < 20000U; ++wait_t) {
        uint32_t status = MAIA_MMIO32(DSI_HOST_BASE + 0x74UL);
        if ((status & (1UL << 0U)) != 0U && (status & (1UL << 2U)) != 0U) { /* EMPTY */
            break;
        }
        maia_udelay(2U);
    }
    uint32_t st_after = MAIA_MMIO32(DSI_HOST_BASE + 0xC0UL);
    if ((st_after & (1UL << 8U)) != 0U) {
        maia_console_puts("[DSI ERR] GEN_CMD_WR_ERR cmd=0x");
        disp_print_hex32((uint32_t)cmd);
        maia_console_puts(" len=");
        disp_print_hex32((uint32_t)len);
        maia_console_puts(" fifo=");
        disp_print_hex32(MAIA_MMIO32(DSI_HOST_BASE + 0x74UL));
        maia_console_puts("\n");
    }
    if (wait_t >= 20000U) {
        maia_console_puts("[DSI ERR] timeout cmd=0x");
        disp_print_hex32((uint32_t)cmd);
        maia_console_puts(" st74=");
        disp_print_hex32(MAIA_MMIO32(DSI_HOST_BASE + 0x74UL));
        maia_console_puts(" st1=");
        disp_print_hex32(st_after);
        maia_console_puts("\n");
    }
}

/* Sitronix ST7121 Vendor Initialization Sequence */
typedef struct {
    uint8_t cmd;
    const uint8_t *data;
    size_t len;
    uint32_t delay_ms;
} dsi_panel_cmd_t;

static const uint8_t s_st7121_c60_1[] = {0x71, 0x21, 0xA2};
static const uint8_t s_st7121_c60_2[] = {0x71, 0x21, 0xA3};
static const uint8_t s_st7121_c60_3[] = {0x71, 0x21, 0xA4};
static const uint8_t s_st7121_c78[]   = {0x21};
static const uint8_t s_st7121_c79[]   = {0xEF};
static const uint8_t s_st7121_ca4[]   = {0x31};
static const uint8_t s_st7121_cb7[]   = {0x00, 0x00, 0x5F, 0x5F, 0x44, 0x1A};
static const uint8_t s_st7121_cb0[]   = {0x22, 0x6B, 0x11, 0x89, 0x25, 0x43, 0x43};
static const uint8_t s_st7121_cbf[]   = {0xA7, 0xA7};
static const uint8_t s_st7121_ca5[]   = {0xF0, 0x03};
static const uint8_t s_st7121_cd7[]   = {0x10, 0x2C, 0x14, 0x2A, 0x80, 0x80};
static const uint8_t s_st7121_c90[]   = {0x71, 0x23, 0x5A, 0x20, 0x24, 0x11, 0x21};
static const uint8_t s_st7121_ca3[]   = {
    0x80, 0x01, 0x8C, 0xFF, 0x45, 0x00, 0x00, 0x00, 0x00, 0x00, 0x46, 0x00,
    0x00, 0x1E, 0x5C, 0x1E, 0x80, 0x10, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x46, 0x00, 0x00, 0x1E, 0x5C, 0x1E, 0x80, 0x10, 0xEF, 0x58, 0x00,
    0x00, 0x00, 0xFF
};
static const uint8_t s_st7121_ca6[]   = {
    0x0A, 0x00, 0x24, 0x71, 0x36, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF,
    0x00, 0x24, 0x71, 0x37, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x00,
    0x24, 0x71, 0x00, 0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x00, 0x2C,
    0x71, 0x00, 0x01, 0x00, 0x00, 0x68, 0x68, 0xFF, 0xFF, 0x00, 0x08, 0x80,
    0x08, 0x80, 0x06, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t s_st7121_ca7[]   = {
    0x1A, 0x1A, 0xC0, 0x64, 0x40, 0x04, 0x15, 0x40, 0x00, 0x40, 0x00, 0x68,
    0x68, 0x91, 0xFF, 0x08, 0x80, 0x64, 0x40, 0x26, 0x37, 0x40, 0x00, 0x00,
    0x00, 0x68, 0x68, 0x91, 0xFF, 0x08, 0x80, 0x64, 0x40, 0x8C, 0x9D, 0x40,
    0x00, 0x00, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x08, 0x80, 0x64, 0x40, 0xAE,
    0xBF, 0x00, 0x00, 0x20, 0x00, 0x68, 0x68, 0x91, 0xFF, 0x08, 0x80, 0x79
};
static const uint8_t s_st7121_cac[]   = {
    0x1D, 0x18, 0x19, 0x1D, 0x18, 0x19, 0x04, 0x1C, 0x1D, 0x08, 0x0A, 0x10,
    0x12, 0x0C, 0x0E, 0x14, 0x16, 0x00, 0x1D, 0x1D, 0x1D, 0x1D, 0x1D, 0x18,
    0x19, 0x1D, 0x18, 0x19, 0x06, 0x1C, 0x1D, 0x09, 0x0B, 0x11, 0x13, 0x0D,
    0x0F, 0x15, 0x17, 0x02, 0x1D, 0x1D, 0x1D, 0x1D
};
static const uint8_t s_st7121_cad[]   = {
    0x0C, 0x40, 0x46, 0x00, 0x07, 0x4B, 0x4B, 0xFF, 0xFF, 0xF0, 0x40, 0x0E,
    0x01, 0x07, 0x42, 0x42, 0xFF, 0xFF, 0x01, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF
};
static const uint8_t s_st7121_cae[]   = {0xF0, 0xFF, 0x03, 0xF0, 0xFF, 0x03, 0x00};
static const uint8_t s_st7121_cb2[]   = {
    0x15, 0x19, 0x05, 0x23, 0x49, 0x2D, 0x03, 0x2E, 0x5C, 0xD2, 0xFF, 0x10,
    0x60, 0xFD, 0x20, 0xC0, 0x00
};
static const uint8_t s_st7121_ce8[]   = {
    0x20, 0x60, 0x04, 0x8E, 0x8E, 0x3E, 0x04, 0xDC, 0xDC, 0x3E, 0x06, 0xFA,
    0x26, 0x3E
};
static const uint8_t s_st7121_c75[]   = {0x03, 0x04};
static const uint8_t s_st7121_ce7[]   = {
    0x4B, 0x00, 0x00, 0xBE, 0x4B, 0x8C, 0x20, 0x1A, 0xF0, 0x7D, 0x14, 0x7D,
    0x14, 0x7D, 0x14, 0x7D, 0x14, 0xFF, 0x00, 0x32, 0x30, 0x73, 0x00, 0x00,
    0xC8, 0x6A, 0xFF, 0x5A, 0x64, 0x38, 0x88, 0x15, 0xB1, 0x01, 0x01, 0x64,
    0x01, 0x01, 0x7C, 0xFF, 0x1A, 0x51
};
static const uint8_t s_st7121_ce1[]   = {0x0C, 0x0C};
static const uint8_t s_st7121_cea[]   = {0x15, 0x00, 0x01};
static const uint8_t s_st7121_cc8[]   = {
    0x00, 0x00, 0x04, 0x08, 0x10, 0x00, 0x1F, 0x01, 0x39, 0x3E, 0x00, 0x78,
    0x06, 0xE2, 0x02, 0x11, 0x33, 0x01, 0x7A, 0x0D, 0x21, 0xC4, 0x0B, 0x19,
    0x08, 0x32, 0xA0, 0x08, 0x1A, 0x0A, 0xF3, 0x7F, 0x0E, 0xC5, 0xE8, 0x03,
    0xFF
};
static const uint8_t s_st7121_cc9[]   = {
    0x00, 0x00, 0x04, 0x08, 0x10, 0x00, 0x1F, 0x01, 0x39, 0x3E, 0x00, 0x78,
    0x06, 0xE2, 0x02, 0x11, 0x33, 0x01, 0x7A, 0x0D, 0x21, 0xC4, 0x0B, 0x19,
    0x08, 0x32, 0xA0, 0x08, 0x1A, 0x0A, 0xF3, 0x7F, 0x0E, 0xC5, 0xE8, 0x03,
    0xFF
};
static const uint8_t s_st7121_c60_4[] = {0x71, 0x21, 0x00};
static const uint8_t s_st7121_c35[]   = {0x00};

static const dsi_panel_cmd_t s_st7121_init_cmds[] = {
    {0x60, s_st7121_c60_1, sizeof(s_st7121_c60_1), 0},
    {0x60, s_st7121_c60_2, sizeof(s_st7121_c60_2), 0},
    {0x60, s_st7121_c60_3, sizeof(s_st7121_c60_3), 0},
    {0x78, s_st7121_c78,   sizeof(s_st7121_c78),   0},
    {0x79, s_st7121_c79,   sizeof(s_st7121_c79),   0},
    {0xA4, s_st7121_ca4,   sizeof(s_st7121_ca4),   0},
    {0xB7, s_st7121_cb7,   sizeof(s_st7121_cb7),   0},
    {0xB0, s_st7121_cb0,   sizeof(s_st7121_cb0),   0},
    {0xBF, s_st7121_cbf,   sizeof(s_st7121_cbf),   0},
    {0xA5, s_st7121_ca5,   sizeof(s_st7121_ca5),   0},
    {0xD7, s_st7121_cd7,   sizeof(s_st7121_cd7),   0},
    {0x90, s_st7121_c90,   sizeof(s_st7121_c90),   0},
    {0xA3, s_st7121_ca3,   sizeof(s_st7121_ca3),   0},
    {0xA6, s_st7121_ca6,   sizeof(s_st7121_ca6),   0},
    {0xA7, s_st7121_ca7,   sizeof(s_st7121_ca7),   0},
    {0xAC, s_st7121_cac,   sizeof(s_st7121_cac),   0},
    {0xAD, s_st7121_cad,   sizeof(s_st7121_cad),   0},
    {0xAE, s_st7121_cae,   sizeof(s_st7121_cae),   0},
    {0xB2, s_st7121_cb2,   sizeof(s_st7121_cb2),   0},
    {0xE8, s_st7121_ce8,   sizeof(s_st7121_ce8),   0},
    {0x75, s_st7121_c75,   sizeof(s_st7121_c75),   0},
    {0xE7, s_st7121_ce7,   sizeof(s_st7121_ce7),   0},
    {0xE1, s_st7121_ce1,   sizeof(s_st7121_ce1),   0},
    {0xEA, s_st7121_cea,   sizeof(s_st7121_cea),   0},
    {0xC8, s_st7121_cc8,   sizeof(s_st7121_cc8),   0},
    {0xC9, s_st7121_cc9,   sizeof(s_st7121_cc9),   0},
    {0x60, s_st7121_c60_4, sizeof(s_st7121_c60_4), 0},
    {0x11, NULL,           0,                      80},  /* SLPOUT: 80ms delay */
    {0x29, NULL,           0,                      800}, /* DISPON: 800ms required by ST7121 */
    {0x35, s_st7121_c35,   sizeof(s_st7121_c35),   0},
};

#include "ili9881c_cmds.h"

static void term_init_banner(void) {
    for (uint8_t r = 0; r < TERM_ROWS; ++r) {
        s_term_fg[r] = COLOR_WHITE;
        s_term_bg[r] = COLOR_NAVY;
        for (uint8_t c = 0; c < TERM_COLS; ++c) {
            s_term_grid[r][c] = ' ';
        }
    }

    /* Row 0: Title banner (Bright Yellow on Dark Blue) */
    s_term_fg[0] = COLOR_YELLOW;
    s_term_bg[0] = COLOR_DARK_BLUE;
    const char *title = "  *** MaiaRTOS v0.1.0 — Tab5 Physical LCD & Keyboard Bring-Up ***";
    for (uint8_t i = 0; title[i] != '\0' && i < TERM_COLS; ++i) {
        s_term_grid[0][i] = title[i];
    }

    /* Row 1: Hardware banner (Bright Cyan on Dark Blue) */
    s_term_fg[1] = COLOR_CYAN;
    s_term_bg[1] = COLOR_DARK_BLUE;
    const char *hw_str = "  M5Stack Tab5 (ESP32-P4 v1.3) | ST7121 720x1280 RGB565 | Keyboard A164 @ 0x6D";
    for (uint8_t i = 0; hw_str[i] != '\0' && i < TERM_COLS; ++i) {
        s_term_grid[1][i] = hw_str[i];
    }

    /* Row 2: Status separator (Green on Black) */
    s_term_fg[2] = COLOR_GREEN;
    s_term_bg[2] = COLOR_BLACK;
    for (uint8_t i = 0; i < TERM_COLS; ++i) {
        s_term_grid[2][i] = '=';
    }

    /* Row 3: Instructions */
    s_term_fg[3] = COLOR_YELLOW;
    s_term_bg[3] = COLOR_NAVY;
    const char *inst = "  [LIVE ECHO] Type on Tab5 keyboard or Mac mini USB-Serial to echo below:";
    for (uint8_t i = 0; inst[i] != '\0' && i < TERM_COLS; ++i) {
        s_term_grid[3][i] = inst[i];
    }

    /* Render static bands 0 & 1 */
    for (uint8_t r = 0; r < TERM_ROWS; ++r) {
        term_render_row(r);
    }

    /* Fill Band 2 (Tiled bottom lines 256..1279): 7 vivid high-contrast color test stripes */
    for (uint32_t y = 0; y < TAB5_BAND_HEIGHT; ++y) {
        uint16_t color;
        if (y < 146U) {
            color = COLOR_RED;       /* Bright Red */
        } else if (y < 292U) {
            color = COLOR_GREEN;     /* Bright Green */
        } else if (y < 438U) {
            color = 0x001FU;         /* Bright Blue */
        } else if (y < 584U) {
            color = COLOR_YELLOW;    /* Bright Yellow */
        } else if (y < 730U) {
            color = COLOR_MAGENTA;   /* Bright Magenta */
        } else if (y < 876U) {
            color = COLOR_CYAN;      /* Bright Cyan */
        } else {
            color = COLOR_WHITE;     /* Pure White */
        }
        uint32_t line_offset = y * TAB5_DISP_WIDTH;
        for (uint32_t x = 0; x < TAB5_DISP_WIDTH; ++x) {
            L2MEM_BAND2_CPU[line_offset + x] = color;
        }
    }

    s_term_cur_row = 4U;
    s_term_cur_col = 0U;
}

void maia_display_refresh_dma_if_needed(void) {
    if (!s_display_ready) {
        return;
    }
    /* If DW_GDMA channel 0 is idle (frame finished), re-arm descriptors and restart */
    if ((MAIA_MMIO32(GDMA_BASE + 0x18UL) & 1UL) == 0U) {
        volatile dw_gdma_lli_t *lli = s_lli_storage;

        for (uint32_t i = 0; i < TAB5_NUM_BLOCKS; ++i) {
            lli[i].ctrl_hi |= (1UL << 31U); /* Mark valid */
        }

        uint32_t head_phys = (uint32_t)(uintptr_t)s_lli_storage;
        MAIA_MMIO32(GDMA_BASE + 0x128UL) = (head_phys & ~0x3FUL) | 1UL; /* loc0, lms=1 */
        MAIA_MMIO32(GDMA_BASE + 0x12CUL) = 0U;
        MAIA_MMIO32(GDMA_BASE + 0x18UL)  = 0x101UL; /* Enable ch0 */
    }
}

void maia_display_init(uint32_t panel_type) {
    (void)panel_type;

    /* 1. Power up LDO_VO3 (2.5V for MIPI D-PHY) via PMU: dref=13, mul=3, en_vdet=1 (eFuse calibrated 2500mV) */
    MAIA_MMIO32(PMU_BASE + 0x1C4UL) = 0xD5800000UL;
    MAIA_MMIO32(PMU_BASE + 0x1C0UL) |= (1UL << 7U) | (1UL << 8U);    /* force_tieh_sel=1, xpd=1 */
    maia_udelay(10000U);

    /* 2. Enable FOSC, CPLL, DW_GDMA, and DSI Clocks in PMU, LP_CLKRST, HP_SYS_CLKRST */
    MAIA_MMIO32(PMU_BASE + 0xACUL) |= (1UL << 30U);
    MAIA_MMIO32(PMU_BASE + 0xCCUL) |= (1UL << 17U) | (1UL << 18U) | (1UL << 23U) |
                                      (1UL << 24U) | (1UL << 27U) | (1UL << 28U);
    MAIA_MMIO32(LP_CLKRST_BASE + 0x28UL) |= (1UL << 31U);
    MAIA_MMIO32(LP_CLKRST_BASE + 0x40UL) |= (1UL << 24U);
    MAIA_MMIO32(HP_SYS_CLKRST_BASE + 0x2CUL) |= (1UL << 8U);

    /* Enable GDMA and DSI Clocks */
    MAIA_MMIO32(HP_SYS_CLKRST_BASE + 0x14UL) |= (1UL << 13U); /* reg_gdma_cpu_clk_en */
    MAIA_MMIO32(HP_SYS_CLKRST_BASE + 0x18UL) |= (1UL << 5U) | (1UL << 12U); /* gdma_sys, dsi_sys */

    /* Pulse resets on GDMA and DSI Bridge */
    MAIA_MMIO32(HP_SYS_CLKRST_BASE + 0xC0UL) |= (1UL << 21U) | (1UL << 26U);
    maia_udelay(1000U);
    MAIA_MMIO32(HP_SYS_CLKRST_BASE + 0xC0UL) &= ~((1UL << 21U) | (1UL << 26U));

    /* DPHY refclk source (PLL_F20M 20MHz crystal) and DPI clk (PLL_F240M / 3 = 80MHz) */
    MAIA_MMIO32(HP_SYS_CLKRST_BASE + 0x38UL) &= ~(3UL << 30U); /* DPHY clk src = PLL_F20M */
    MAIA_MMIO32(HP_SYS_CLKRST_BASE + 0x3CUL) = (2UL << 8U) | (1UL << 7U) | (1UL << 5U) | (1UL << 1U) | (1UL << 0U);

    /* 3. Configure DSI Host & D-PHY */
    MAIA_MMIO32(DSI_HOST_BASE + 0xA4UL) = 1UL;                 /* 2 data lanes, stop_wait_time=0 */
    MAIA_MMIO32(DSI_HOST_BASE + 0x04UL) = 1UL;                 /* Power up DSI host controller */
    MAIA_MMIO32(DSI_HOST_BASE + 0xA0UL) = 0x01UL;              /* Reset D-PHY digital section (shutdownz=1, rstz=0) */
    maia_udelay(100U);
    MAIA_MMIO32(DSI_HOST_BASE + 0xA0UL) = 0x0FUL;              /* forcepll=1, enableclk=1, rstz=1, shutdownz=1 */
    maia_udelay(100U);

    /* Write D-PHY PLL settings for 960 Mbps (refclk = 20 MHz, M = 48, N = 1, range = 0x1A) */
    dsi_phy_write_reg(0x44U, 0x34U); /* hs_freq_sel = 0x1A << 1 */
    dsi_phy_write_reg(0x19U, 0x30U);
    dsi_phy_write_reg(0x17U, 0x00U); /* pll_N - 1 = 0 */
    dsi_phy_write_reg(0x18U, 0x0FU); /* (pll_M - 1) & 0x1F = 47 & 0x1F = 15 = 0x0F */
    dsi_phy_write_reg(0x18U, 0x81U); /* 0x80 | ((pll_M - 1) >> 5) = 0x81 */

    /* Wait for D-PHY PLL lock and lanes stop state */
    for (uint32_t t = 0; t < 50000U; ++t) {
        if ((MAIA_MMIO32(DSI_HOST_BASE + 0xB0UL) & 1UL) != 0U) {
            break;
        }
        maia_udelay(2U);
    }
    for (uint32_t t = 0; t < 50000U; ++t) {
        /* mask = bit 2 (clk stop), bit 4 (lane0 stop), bit 7 (lane1 stop) = 0x94 */
        if ((MAIA_MMIO32(DSI_HOST_BASE + 0xB0UL) & 0x94UL) == 0x94UL) {
            break;
        }
        maia_udelay(2U);
    }

    uint32_t phy_status = MAIA_MMIO32(DSI_HOST_BASE + 0xB0UL);
    maia_console_puts("[DSI] D-PHY status: ");
    disp_print_hex32(phy_status);
    maia_console_puts("\n");

    /* Set Command Mode, clock lane LP, transition timers, and LP packet config */
    MAIA_MMIO32(DSI_HOST_BASE + 0x34UL) = 1UL; /* Command Mode */
    MAIA_MMIO32(DSI_HOST_BASE + 0x94UL) = 0UL; /* LP Clock Lane */
    MAIA_MMIO32(DSI_HOST_BASE + 0x98UL) = (46UL << 16U) | 128UL; /* clkhs2lp=46, clklp2hs=128 */
    MAIA_MMIO32(DSI_HOST_BASE + 0x9CUL) = (50UL << 16U) | 104UL; /* hs2lp=50, lp2hs=104 */
    MAIA_MMIO32(DSI_HOST_BASE + 0x2CUL) = (1UL << 0U) | (1UL << 3U) | (1UL << 4U); /* eotp_tx_en=1, ecc_rx_en=1, crc_rx_en=1 */
    MAIA_MMIO32(DSI_HOST_BASE + 0x08UL) = (12UL << 8U) | 6UL;    /* to_clk_div=12, esc_clk_div=6 */
    MAIA_MMIO32(DSI_HOST_BASE + 0x78UL) = 0UL;
    MAIA_MMIO32(DSI_HOST_BASE + 0x7CUL) = 0UL;
    MAIA_MMIO32(DSI_HOST_BASE + 0x80UL) = 0UL;
    MAIA_MMIO32(DSI_HOST_BASE + 0x84UL) = 0UL;
    MAIA_MMIO32(DSI_HOST_BASE + 0x88UL) = 0UL;
    MAIA_MMIO32(DSI_HOST_BASE + 0x8CUL) = 0UL;
    MAIA_MMIO32(DSI_HOST_BASE + 0x90UL) = 0UL;
    MAIA_MMIO32(DSI_HOST_BASE + 0xF4UL) = 6000UL;                /* max_rd_time */
    MAIA_MMIO32(DSI_HOST_BASE + 0xA4UL) = (0x3FUL << 8U) | 1UL; /* stop_wait_time = 0x3F, 2 lanes */
    MAIA_MMIO32(DSI_HOST_BASE + 0x68UL) = 0x010F7F02UL;          /* LP commands enabled + cmd ack */

    /* 4. Send Panel Wakeup & Initialization Sequence over DBI and Configure Video Timings */
    if (panel_type == MAIA_TAB5_PANEL_ST7121 || panel_type == MAIA_TAB5_PANEL_ST7123 || panel_type == MAIA_TAB5_PANEL_UNKNOWN) {
        maia_console_puts("[DISPLAY] Initializing ST7121 panel commands & timings...\n");
        /* Software reset ST7121 and wait 120ms */
        dsi_send_dcs_cmd(0x01U, NULL, 0U);
        maia_udelay(120000U);

        for (size_t i = 0; i < sizeof(s_st7121_init_cmds) / sizeof(s_st7121_init_cmds[0]); ++i) {
            dsi_send_dcs_cmd(s_st7121_init_cmds[i].cmd, s_st7121_init_cmds[i].data, s_st7121_init_cmds[i].len);
            if (s_st7121_init_cmds[i].delay_ms > 0U) {
                maia_udelay(s_st7121_init_cmds[i].delay_ms * 1000U);
            }
            maia_udelay(5000U);
        }

        /* 5. Configure DPI Video Mode on DSI Host (ST7121 timings) */
        MAIA_MMIO32(DSI_HOST_BASE + 0x0CUL) = 0UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x10UL) = 0UL; /* 16-bit RGB565 */
        MAIA_MMIO32(DSI_HOST_BASE + 0x14UL) = 0UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x38UL) = 0xFF02UL; /* Video burst with sync pulses + frame ack */
        MAIA_MMIO32(DSI_HOST_BASE + 0x3CUL) = 720UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x40UL) = 0UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x44UL) = 0UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x48UL) = 3UL;    /* vid_hsa_time (2 * 1.5) */
        MAIA_MMIO32(DSI_HOST_BASE + 0x4CUL) = 60UL;   /* vid_hbp_time (40 * 1.5) */
        MAIA_MMIO32(DSI_HOST_BASE + 0x50UL) = 1203UL; /* vid_hline_time (802 * 1.5) */
        MAIA_MMIO32(DSI_HOST_BASE + 0x54UL) = 20UL;   /* vsa lines */
        MAIA_MMIO32(DSI_HOST_BASE + 0x58UL) = 24UL;   /* vbp lines */
        MAIA_MMIO32(DSI_HOST_BASE + 0x5CUL) = 200UL;  /* vfp lines */
        MAIA_MMIO32(DSI_HOST_BASE + 0x60UL) = 1280UL; /* vactive lines */

        MAIA_MMIO32(DSI_BRG_BASE + 0x00UL) = 1UL; /* clk_en = 1 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x04UL) = 1UL; /* dsi_en = 1 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x80UL) = 1UL; /* host_ctrl: dsi_cfg_ref_clk_en = 1 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x18UL) = 2UL; /* pixel_type: RGB565 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x30UL) = (1280UL << 16U) | 1524UL; /* dpi_v_cfg0: vdisp=1280, vtotal=1524 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x34UL) = (20UL << 16U) | 24UL;      /* dpi_v_cfg1: vsync=20, vbank=24 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x38UL) = (720UL << 16U) | 802UL;   /* dpi_h_cfg0: hdisp=720, htotal=802 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x3CUL) = (2UL << 16U) | 40UL;      /* dpi_h_cfg1: hsync=2, hbank=40 */
    } else {
        /* Default: ILI9881C panel */
        maia_console_puts("[DISPLAY] Initializing ILI9881C panel commands & timings...\n");
        for (size_t i = 0; i < sizeof(s_ili9881c_init_cmds) / sizeof(s_ili9881c_init_cmds[0]); ++i) {
            dsi_send_dcs_cmd(s_ili9881c_init_cmds[i].cmd, s_ili9881c_init_cmds[i].data, s_ili9881c_init_cmds[i].len);
            if (s_ili9881c_init_cmds[i].delay_ms > 0U) {
                maia_udelay(s_ili9881c_init_cmds[i].delay_ms * 1000U);
            }
            maia_udelay(2000U);
        }

        /* 5. Configure DPI Video Mode on DSI Host (ILI9881C timings: 720x1280 @ 60MHz DPI)
         * H: hact=720, hsync=40, hbp=140, hfp=40 -> htotal=940
         * V: vact=1280, vsync=4, vbp=20, vfp=20 -> vtotal=1324
         * Byte clock = 100 MHz (800 Mbps D-PHY), DPI clock = 60 MHz (ratio 5/3 = 1.6667)
         * vid_hsa_time = 40 * 100 / 60 = 66
         * vid_hbp_time = 140 * 100 / 60 = 233
         * vid_hline_time = 940 * 100 / 60 = 1566
         */
        MAIA_MMIO32(DSI_HOST_BASE + 0x0CUL) = 0UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x10UL) = 0UL; /* 16-bit RGB565 */
        MAIA_MMIO32(DSI_HOST_BASE + 0x14UL) = 0UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x38UL) = 0xBF02UL; /* Video burst with sync pulses */
        MAIA_MMIO32(DSI_HOST_BASE + 0x3CUL) = 720UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x40UL) = 0UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x44UL) = 0UL;
        MAIA_MMIO32(DSI_HOST_BASE + 0x48UL) = 66UL;   /* vid_hsa_time */
        MAIA_MMIO32(DSI_HOST_BASE + 0x4CUL) = 233UL;  /* vid_hbp_time */
        MAIA_MMIO32(DSI_HOST_BASE + 0x50UL) = 1566UL; /* vid_hline_time */
        MAIA_MMIO32(DSI_HOST_BASE + 0x54UL) = 4UL;    /* vsa lines */
        MAIA_MMIO32(DSI_HOST_BASE + 0x58UL) = 20UL;   /* vbp lines */
        MAIA_MMIO32(DSI_HOST_BASE + 0x5CUL) = 20UL;   /* vfp lines */
        MAIA_MMIO32(DSI_HOST_BASE + 0x60UL) = 1280UL; /* vactive lines */

        MAIA_MMIO32(DSI_BRG_BASE + 0x00UL) = 1UL; /* clk_en = 1 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x04UL) = 1UL; /* dsi_en = 1 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x80UL) = 1UL; /* host_ctrl: dsi_cfg_ref_clk_en = 1 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x18UL) = 2UL; /* pixel_type: RGB565 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x30UL) = (1280UL << 16U) | 1324UL; /* dpi_v_cfg0: vdisp=1280, vtotal=1324 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x34UL) = (4UL << 16U) | 20UL;       /* dpi_v_cfg1: vsync=4, vbank=20 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x38UL) = (720UL << 16U) | 940UL;    /* dpi_h_cfg0: hdisp=720, htotal=940 */
        MAIA_MMIO32(DSI_BRG_BASE + 0x3CUL) = (40UL << 16U) | 140UL;     /* dpi_h_cfg1: hsync=40, hbank=140 */
    }
    MAIA_MMIO32(DSI_BRG_BASE + 0x08UL) = 256UL;                     /* dma_req_cfg: dma_burst_len = 256 */
    MAIA_MMIO32(DSI_BRG_BASE + 0x0CUL) = TAB5_TOTAL_WORDS64 | (1UL << 31U); /* raw_num_cfg: total 64-bit words */
    MAIA_MMIO32(DSI_BRG_BASE + 0x88UL) = (10UL << 4U);             /* dma_flow_ctrl: multiblk_num=10, flow_ctrl=0(DMA) */
    MAIA_MMIO32(DSI_BRG_BASE + 0x6CUL) = (9UL << 0U) | (9UL << 10U) | (1UL << 28U) | (1UL << 29U); /* dma_frame_interval */
    MAIA_MMIO32(DSI_BRG_BASE + 0x8CUL) = 768UL;                    /* raw_buf_almost_empty_thrd = 768 */
    MAIA_MMIO32(DSI_BRG_BASE + 0x68UL) = TAB5_BAND_WORDS64 | (1UL << 31U); /* blk_raw_num_cfg */

    /* 6. Setup Terminal Content and Bands before DMA starts */
    term_init_banner();

    /* 7. Configure 10-Item Circular DW_GDMA Linked-List Descriptors in Internal SRAM */
    volatile dw_gdma_lli_t *lli = s_lli_storage;

    for (uint32_t i = 0; i < TAB5_NUM_BLOCKS; ++i) {
        uint32_t src_phys = L2MEM_BAND2_PHYS;
        if (i == 0U) {
            src_phys = L2MEM_BAND0_PHYS;
        } else if (i == 1U) {
            src_phys = L2MEM_BAND1_PHYS;
        }

        lli[i].sar_lo      = src_phys;
        lli[i].sar_hi      = 0U;
        lli[i].dar_lo      = DSI_BRG_MEM_BASE;
        lli[i].dar_hi      = 0U;
        lli[i].block_ts_lo = TAB5_BAND_WORDS64 - 1U;
        lli[i].reserved_14 = 0U;

        /* Circular ring: block 9 loops back to block 0 for continuous autonomous frame streaming */
        if (i == (TAB5_NUM_BLOCKS - 1U)) {
            uint32_t head_phys = (uint32_t)(uintptr_t)&s_lli_storage[0];
            lli[i].llp_lo = (head_phys & ~0x3FUL) | 1UL; /* LMS = 1: loop back to head */
        } else {
            uint32_t next_phys = (uint32_t)(uintptr_t)&s_lli_storage[i + 1U];
            lli[i].llp_lo = (next_phys & ~0x3FUL) | 1UL; /* LMS = 1 (memory port 1) */
        }
        lli[i].llp_hi = 0U;

        /* ctrl_lo: dst_msize=7(256), src_msize=8(512), dst_tr_width=3(64b), src_tr_width=3(64b),
         * dinc=1(fixed), sinc=0(inc), dms=0(DSI port 0), sms=1(Mem port 1) */
        lli[i].ctrl_lo = (7UL << 18U) | (8UL << 14U) | (3UL << 11U) | (3UL << 8U) |
                         (1UL << 6U)  | (0UL << 4U)  | (0UL << 2U)  | 1UL;

        /* ctrl_hi: valid=1, last=0 (continuous circular streaming), arlen=16, arlen_en=1, awlen=16, awlen_en=1 */
        lli[i].ctrl_hi = (1UL << 31U) | (16UL << 16U) | (1UL << 15U) | (16UL << 7U) | (1UL << 6U);

        lli[i].sstat       = 0U;
        lli[i].dstat       = 0U;
        lli[i].status_lo   = 0U;
        lli[i].status_hi   = 0U;
        lli[i].reserved_38 = 0U;
        lli[i].reserved_3c = 0U;
    }

    /* 8. Configure & Start DW_GDMA Channel 0 to prefill bridge FIFO */
    MAIA_MMIO32(GDMA_BASE + 0x10UL)  = 3UL; /* dmac_en=1, int_en=1 */
    MAIA_MMIO32(GDMA_BASE + 0x120UL) = (3UL << 2U) | 3UL; /* List transfer */
    /* cfg1: dst_osr_lmt=1, src_osr_lmt=4, ch_prior=1, dst_per=0, tt_fc=1 (M2P_DMAC) */
    MAIA_MMIO32(GDMA_BASE + 0x124UL) = (1UL << 27U) | (4UL << 23U) | (1UL << 17U) | 1UL;

    uint32_t head_phys = (uint32_t)(uintptr_t)s_lli_storage;
    MAIA_MMIO32(GDMA_BASE + 0x128UL) = (head_phys & ~0x3FUL) | 1UL;
    MAIA_MMIO32(GDMA_BASE + 0x12CUL) = 0U;

    /* Start First Continuous Display Frame (prefill FIFO) */
    MAIA_MMIO32(GDMA_BASE + 0x18UL) = 0x101UL;
    maia_udelay(5000U);

    /* 9. Switch DSI Host to Video Mode and enable Bridge DPI output */
    MAIA_MMIO32(DSI_HOST_BASE + 0x34UL) = 0UL;               /* Video Mode */
    MAIA_MMIO32(DSI_HOST_BASE + 0x94UL) = 3UL;               /* Clock Lane AUTO (High-Speed transmission) */
    MAIA_MMIO32(DSI_BRG_BASE + 0x40UL) = (720UL << 4U) | 1UL; /* dpi_en = 1 */
    MAIA_MMIO32(DSI_BRG_BASE + 0x44UL) = 1UL;                /* dpi_config_update = 1 */

    /* 10. Turn on LCD Backlight at 100% duty */
    maia_tab5_backlight_set_duty(4095U);

    uint32_t brg_raw = MAIA_MMIO32(DSI_BRG_BASE + 0x58UL);
    uint32_t host_st0 = MAIA_MMIO32(DSI_HOST_BASE + 0xBCUL);
    uint32_t host_st1 = MAIA_MMIO32(DSI_HOST_BASE + 0xC0UL);
    maia_console_puts("[DSI] brg_raw=");
    disp_print_hex32(brg_raw);
    maia_console_puts(" host_st0=");
    disp_print_hex32(host_st0);
    maia_console_puts(" host_st1=");
    disp_print_hex32(host_st1);
    maia_console_puts("\n");

    s_display_ready = 1U;
    maia_console_puts("[DISPLAY] Initialized MIPI-DSI 720x1280 (10x128 multi-block DMA in L2MEM)\n");
}

