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

#define L2MEM_BAND0_CPU      ((volatile uint16_t *)(uintptr_t)(L2MEM_BAND0_PHYS + 0x40000000UL))
#define L2MEM_BAND1_CPU      ((volatile uint16_t *)(uintptr_t)(L2MEM_BAND1_PHYS + 0x40000000UL))
#define L2MEM_BAND2_CPU      ((volatile uint16_t *)(uintptr_t)(L2MEM_BAND2_PHYS + 0x40000000UL))

/* Colors (RGB565) */
#define COLOR_BLACK          0x0000U
#define COLOR_WHITE          0xFFFFU
#define COLOR_DARK_BLUE      0x0968U  /* #0B1D33 - Deep Elegant Navy Terminal Background */
#define COLOR_HEADER_BLUE    0x1A6DU  /* #1B446F - Sleek Royal Blue Header Bar */
#define COLOR_CYAN           0x07FFU  /* #00FFFF - High-visibility Cyan */
#define COLOR_GREEN          0x07E0U  /* #00FF00 - Crisp Green */
#define COLOR_YELLOW         0xFFE0U  /* #FFFF00 - Bright Amber/Yellow */
#define COLOR_RED            0xF800U  /* #FF0000 - Red */
#define COLOR_MAGENTA        0xF81FU  /* #FF00FF - Magenta */
#define COLOR_DARK_SLATE     0x0968U  /* Slate Navy */
#define COLOR_GRAY           0x8410U  /* Neutral Gray */

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

/* Physical hardware panel dimensions */
#define TAB5_PHYS_W          720
#define TAB5_PHYS_H          1280

/* Logical Landscape dimensions (Rotation 3: 270 deg) */
#define TAB5_LOG_W           1280
#define TAB5_LOG_H           720

#define CHAR_W               16
#define CHAR_H               32
#define TERM_COLS            (TAB5_LOG_W / CHAR_W) /* 80 cols */
#define TERM_ROWS            (TAB5_LOG_H / CHAR_H) /* 22 rows */
#define PROMPT_ROW_START     4

#define PSRAM_FB_BASE        0x48100000UL
static volatile uint16_t    *s_fb = (volatile uint16_t *)(uintptr_t)PSRAM_FB_BASE;

/* Terminal text buffers */
static char     s_term_lines[TERM_ROWS][TERM_COLS + 1];
static char     s_sram_grid[16][81];
static uint8_t  s_term_cur_row = PROMPT_ROW_START;
static uint8_t  s_term_cur_col = 0U;
static uint8_t  s_display_ready = 0U;
static uint8_t  s_has_psram = 0U;

/* Basic 8x16 font table for printable ASCII 32..126 (95 glyphs) */
#include "font_8x16.h"

static void fb_sync(void) {
    __asm__ volatile("fence rw, rw" ::: "memory");
    if (s_has_psram) {
        int (*rom_wb)(uint32_t) = (int (*)(uint32_t))0x4FC00414UL;
        (void)rom_wb(0x30U); /* WriteBack L1 DCache (0x10) | L2 Cache (0x20) */
    }
}

/* Rotation 3 pixel transformation (Landscape 1280x720):
 * Logical (x, y) with 0 <= x < 1280, 0 <= y < 720
 * Physical panel (px, py) with 0 <= px < 720, 0 <= py < 1280
 *
 * Rotation 3:
 * px = y
 * py = 1279 - x
 */
static inline void put_pixel_rot3(int x, int y, uint16_t color) {
    if (!s_has_psram || x < 0 || x >= TAB5_LOG_W || y < 0 || y >= TAB5_LOG_H) return;
    int px = y;
    int py = 1279 - x;
    s_fb[py * TAB5_PHYS_W + px] = color;
}

static void fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (!s_has_psram) return;
    int max_x = x + w;
    int max_y = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (max_x > TAB5_LOG_W) max_x = TAB5_LOG_W;
    if (max_y > TAB5_LOG_H) max_y = TAB5_LOG_H;

    for (int cy = y; cy < max_y; ++cy) {
        int px = cy;
        for (int cx = x; cx < max_x; ++cx) {
            int py = 1279 - cx;
            s_fb[py * TAB5_PHYS_W + px] = color;
        }
    }
}

static void draw_char_2x(int col, int row, char c, uint16_t fg, uint16_t bg) {
    if (!s_has_psram || col < 0 || col >= (int)TERM_COLS || row < 0 || row >= (int)TERM_ROWS) return;
    int px_x = col * CHAR_W;
    int px_y = row * CHAR_H;
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = &g_font_8x16[((int)c - 32) * 16];

    for (int y = 0; y < 16; ++y) {
        uint8_t line = glyph[y];
        for (int x = 0; x < 8; ++x) {
            int on = (line & (0x80 >> x)) != 0;
            uint16_t color = on ? fg : bg;
            int dst_x = px_x + (x * 2);
            int dst_y = px_y + (y * 2);
            put_pixel_rot3(dst_x,     dst_y,     color);
            put_pixel_rot3(dst_x + 1, dst_y,     color);
            put_pixel_rot3(dst_x,     dst_y + 1, color);
            put_pixel_rot3(dst_x + 1, dst_y + 1, color);
        }
    }
}

static void draw_str_2x(int col, int row, const char *str, uint16_t fg, uint16_t bg) {
    if (!s_has_psram || str == NULL) return;
    size_t len = 0;
    while (str[len] != '\0') len++;
    fill_rect(col * CHAR_W, row * CHAR_H, (int)len * CHAR_W, CHAR_H, bg);
    while (*str && col < (int)TERM_COLS) {
        draw_char_2x(col, row, *str++, fg, bg);
        col++;
    }
}

/* Safe SRAM band rendering functions for fallback mode */
static void draw_sram_char(uint32_t col, uint32_t row, char c, uint16_t fg, uint16_t bg) {
    if (col >= 80U || row >= 16U) return;
    volatile uint16_t *band = (row < 8U) ? L2MEM_BAND0_CPU : L2MEM_BAND1_CPU;
    uint32_t local_row = row % 8U;
    uint32_t x0 = col * 8U;
    uint32_t y0 = local_row * 16U;
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = &g_font_8x16[(c - 32) * 16];

    for (uint32_t py = 0; py < 16U; ++py) {
        uint8_t bits = glyph[py];
        uint32_t line_offset = (y0 + py) * TAB5_DISP_WIDTH + x0;
        for (uint32_t px = 0; px < 8U; ++px) {
            uint16_t color = ((bits >> (7U - px)) & 1U) ? fg : bg;
            band[line_offset + px] = color;
        }
    }
}

static void draw_sram_str(uint32_t col, uint32_t row, const char *str, uint16_t fg, uint16_t bg) {
    if (!str) return;
    while (*str && col < 80U) {
        draw_sram_char(col++, row, *str++, fg, bg);
    }
}

static void term_scroll_sram(void) {
    for (uint32_t r = PROMPT_ROW_START; r < 15U; ++r) {
        for (uint32_t c = 0; c < 80U; ++c) {
            char ch = s_sram_grid[r + 1][c];
            s_sram_grid[r][c] = ch;
            draw_sram_char(c, r, ch, COLOR_WHITE, COLOR_DARK_BLUE);
        }
    }
    for (uint32_t c = 0; c < 80U; ++c) {
        s_sram_grid[15][c] = ' ';
        draw_sram_char(c, 15, ' ', COLOR_WHITE, COLOR_DARK_BLUE);
    }
    s_term_cur_row = 15U;
    s_term_cur_col = 0U;
}

static void term_scroll(void) {
    if (!s_has_psram) {
        term_scroll_sram();
        return;
    }
    for (int r = PROMPT_ROW_START; r < ((int)TERM_ROWS - 1); ++r) {
        for (int c = 0; c < (int)TERM_COLS; ++c) {
            char ch = s_term_lines[r + 1][c];
            s_term_lines[r][c] = ch;
            draw_char_2x(c, r, ch, COLOR_WHITE, COLOR_BLACK);
        }
    }
    int last_r = (int)TERM_ROWS - 1;
    for (int c = 0; c < (int)TERM_COLS; ++c) {
        s_term_lines[last_r][c] = ' ';
    }
    fill_rect(0, last_r * CHAR_H, TAB5_LOG_W, CHAR_H, COLOR_BLACK);
    s_term_cur_row = (uint8_t)last_r;
    s_term_cur_col = 0U;
    fb_sync();
}

static int s_exclusive_mode = 0;

void maia_display_set_exclusive(int exclusive) {
    s_exclusive_mode = exclusive;
}

void maia_display_putc(char c) {
    if (s_exclusive_mode || !s_display_ready) {
        return;
    }
    if (s_has_psram) {
        if (c == '\r') {
            s_term_cur_col = 0U;
            return;
        }
        if (c == '\n') {
            s_term_cur_col = 0U;
            s_term_cur_row++;
            if (s_term_cur_row >= (uint8_t)TERM_ROWS) {
                term_scroll();
            }
            fb_sync();
            return;
        }
        if (c == '\b' || c == 127) {
            if (s_term_cur_col > 0U) {
                s_term_cur_col--;
                s_term_lines[s_term_cur_row][s_term_cur_col] = ' ';
                draw_char_2x((int)s_term_cur_col, (int)s_term_cur_row, ' ', COLOR_WHITE, COLOR_BLACK);
                fb_sync();
            }
            return;
        }
        if (c == '\t') {
            uint8_t next_tab = (uint8_t)((s_term_cur_col + 4U) & ~3U);
            while (s_term_cur_col < next_tab && s_term_cur_col < (uint8_t)TERM_COLS) {
                maia_display_putc(' ');
            }
            return;
        }
        if ((uint8_t)c < 32 || (uint8_t)c > 126) {
            return;
        }

        if (s_term_cur_col >= (uint8_t)TERM_COLS) {
            s_term_cur_col = 0U;
            s_term_cur_row++;
            if (s_term_cur_row >= (uint8_t)TERM_ROWS) {
                term_scroll();
            }
        }

        s_term_lines[s_term_cur_row][s_term_cur_col] = c;
        draw_char_2x((int)s_term_cur_col, (int)s_term_cur_row, c, COLOR_WHITE, COLOR_BLACK);
        s_term_cur_col++;
        fb_sync();
    } else {
        if (c == '\r') {
            s_term_cur_col = 0U;
            return;
        }
        if (c == '\n') {
            s_term_cur_col = 0U;
            s_term_cur_row++;
            if (s_term_cur_row >= 16U) {
                term_scroll_sram();
            }
            return;
        }
        if (c == '\b' || c == 127) {
            if (s_term_cur_col > 0U) {
                s_term_cur_col--;
                s_sram_grid[s_term_cur_row][s_term_cur_col] = ' ';
                draw_sram_char(s_term_cur_col, s_term_cur_row, ' ', COLOR_WHITE, COLOR_DARK_BLUE);
            }
            return;
        }
        if (c == '\t') {
            uint8_t next_tab = (uint8_t)((s_term_cur_col + 4U) & ~3U);
            while (s_term_cur_col < next_tab && s_term_cur_col < 80U) {
                maia_display_putc(' ');
            }
            return;
        }
        if ((uint8_t)c < 32 || (uint8_t)c > 126) {
            return;
        }

        if (s_term_cur_col >= 80U) {
            s_term_cur_col = 0U;
            s_term_cur_row++;
            if (s_term_cur_row >= 16U) {
                term_scroll_sram();
            }
        }

        s_sram_grid[s_term_cur_row][s_term_cur_col] = c;
        draw_sram_char(s_term_cur_col, s_term_cur_row, c, COLOR_WHITE, COLOR_DARK_BLUE);
        s_term_cur_col++;
    }
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

static void __attribute__((unused)) disp_print_hex8(uint8_t v) {
    static const char kHex[] = "0123456789ABCDEF";
    char s[5];
    s[0] = '0';
    s[1] = 'x';
    s[2] = kHex[(v >> 4U) & 0xFU];
    s[3] = kHex[v & 0xFU];
    s[4] = '\0';
    maia_console_puts(s);
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
    if (s_has_psram) {
        fill_rect(0, 0, TAB5_LOG_W, TAB5_LOG_H, COLOR_BLACK);
        for (int r = 0; r < (int)TERM_ROWS; ++r) {
            for (int c = 0; c < (int)TERM_COLS; ++c) {
                s_term_lines[r][c] = ' ';
            }
            s_term_lines[r][TERM_COLS] = '\0';
        }

        draw_str_2x(1, 0, "=== MaiaRTOS Hardware Bring-up (M5Stack Tab5 Landscape 1280x720) ===", COLOR_CYAN, COLOR_BLACK);
        draw_str_2x(1, 1, "Display: ST7121 MIPI-DSI @ Rot 3 | Keyboard: Ext.Port1 (A164 @ 0x6D)", COLOR_YELLOW, COLOR_BLACK);
        draw_str_2x(1, 2, "Status: MaiaShell Ready. Type on physical keyboard or USB-Serial.", COLOR_GREEN, COLOR_BLACK);
        draw_str_2x(0, 3, "--------------------------------------------------------------------------------", COLOR_GRAY, COLOR_BLACK);

        s_term_cur_row = PROMPT_ROW_START;
        s_term_cur_col = 0U;
        fb_sync();
    } else {
        /* Safe internal SRAM band mode (no buffer overflow) */
        for (uint32_t i = 0; i < (TAB5_DISP_WIDTH * TAB5_BAND_HEIGHT); ++i) {
            L2MEM_BAND0_CPU[i] = COLOR_HEADER_BLUE;
            L2MEM_BAND1_CPU[i] = COLOR_DARK_BLUE;
            L2MEM_BAND2_CPU[i] = COLOR_DARK_BLUE;
        }
        for (int r = 0; r < 16; ++r) {
            for (int c = 0; c < 80; ++c) {
                s_sram_grid[r][c] = ' ';
            }
            s_sram_grid[r][80] = '\0';
        }
        draw_sram_str(1, 0, "=== MaiaRTOS Hardware Bring-up (M5Stack Tab5 Bare-Metal) ===", COLOR_YELLOW, COLOR_HEADER_BLUE);
        draw_sram_str(1, 1, "Display: ST7121 MIPI-DSI (L2 SRAM) | Keyboard: Ext.Port1 (A164 @ 0x6D)", COLOR_CYAN, COLOR_HEADER_BLUE);
        draw_sram_str(1, 2, "Status: MaiaShell Ready. Type on physical keyboard or USB-Serial.", COLOR_GREEN, COLOR_HEADER_BLUE);
        draw_sram_str(0, 3, "--------------------------------------------------------------------------------", COLOR_GRAY, COLOR_HEADER_BLUE);

        s_term_cur_row = PROMPT_ROW_START;
        s_term_cur_col = 0U;
    }
}

void maia_display_refresh_dma_if_needed(void) {
    if (!s_display_ready) {
        return;
    }
    /* If DW_GDMA channel 0 stopped unexpectedly, re-arm circular descriptors */
    if ((MAIA_MMIO32(GDMA_BASE + 0x18UL) & 1UL) == 0U) {
        volatile dw_gdma_lli_t *lli = (volatile dw_gdma_lli_t *)((uintptr_t)s_lli_storage + 0x40000000UL);

        for (uint32_t i = 0; i < TAB5_NUM_BLOCKS; ++i) {
            if (s_has_psram) {
                lli[i].sar_lo = PSRAM_FB_BASE + i * (TAB5_BAND_WORDS64 * 8U);
            }
            /* Keep last=0 (bit 30 cleared) for continuous autonomous circular streaming */
            lli[i].ctrl_hi = (1UL << 31U) | (16UL << 16U) | (1UL << 15U) | (16UL << 7U) | (1UL << 6U);
        }

        /* Reload DSI bridge block and frame counters */
        MAIA_MMIO32(DSI_BRG_BASE + 0x0CUL) = TAB5_TOTAL_WORDS64 | (1UL << 31U);
        MAIA_MMIO32(DSI_BRG_BASE + 0x68UL) = TAB5_BAND_WORDS64 | (1UL << 31U);

        /* Clear channel 0 interrupts */
        MAIA_MMIO32(GDMA_BASE + 0x198UL) = ~0UL;

        uint32_t head_phys = (uint32_t)(uintptr_t)s_lli_storage;
        MAIA_MMIO32(GDMA_BASE + 0x128UL) = (head_phys & ~0x3FUL) | 1UL; /* loc0, lms=1 */
        MAIA_MMIO32(GDMA_BASE + 0x12CUL) = 0U;
        MAIA_MMIO32(GDMA_BASE + 0x18UL)  = 0x101UL; /* Enable ch0 */
    }
}

void maia_display_set_rgb_test_mode(void) {
    volatile dw_gdma_lli_t *lli = (volatile dw_gdma_lli_t *)((uintptr_t)s_lli_storage + 0x40000000UL);
    for (uint32_t i = 0; i < TAB5_NUM_BLOCKS; ++i) {
        uint32_t src_phys = L2MEM_BAND2_PHYS; /* Blue: blocks 7..9 (lines 896..1279) */
        if (i < 3U) {
            src_phys = L2MEM_BAND0_PHYS;      /* Red: blocks 0..2 (lines 0..383) */
        } else if (i < 7U) {
            src_phys = L2MEM_BAND1_PHYS;      /* Green: blocks 3..6 (lines 384..895) */
        }
        lli[i].sar_lo = src_phys;
    }
    __asm__ volatile ("fence rw, rw" ::: "memory");
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

    /* Write D-PHY PLL settings for 960 Mbps (refclk = 20 MHz, M = 96, N = 2, range = 0x1A) */
    dsi_phy_write_reg(0x44U, 0x34U); /* hs_freq_sel = 0x1A << 1 = 0x34 */
    dsi_phy_write_reg(0x19U, 0x30U);
    dsi_phy_write_reg(0x17U, 0x01U); /* pll_N - 1 = 1 */
    dsi_phy_write_reg(0x18U, 0x1FU); /* (pll_M - 1) & 0x1F = 95 & 0x1F = 31 = 0x1F */
    dsi_phy_write_reg(0x18U, 0x82U); /* 0x80 | ((pll_M - 1) >> 5) = 0x80 | 2 = 0x82 */

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
            maia_udelay(1000U);
        }
        maia_console_puts("[DSI] ST7121 init commands complete!\n");

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
    MAIA_MMIO32(DSI_BRG_BASE + 0x6CUL) = (1UL << 28U) | (1UL << 29U) | (9UL << 10U) | 9UL; /* dma_frame_interval: multiblk_en=1 */
    MAIA_MMIO32(DSI_BRG_BASE + 0x68UL) = TAB5_BAND_WORDS64 | (1UL << 31U); /* blk_raw_num_cfg */
    MAIA_MMIO32(DSI_BRG_BASE + 0x8CUL) = 768UL;                    /* raw_buf_almost_empty_thrd = 768 */

    /* Check if PSRAM is accessible (initialized by bootloader) */
    volatile uint32_t *p_test = (volatile uint32_t *)PSRAM_FB_BASE;
    uint32_t old_val = *p_test;
    *p_test = 0x55AA1234UL;
    __asm__ volatile("fence rw, rw" ::: "memory");
    if (*p_test == 0x55AA1234UL) {
        *p_test = 0xAA554321UL;
        __asm__ volatile("fence rw, rw" ::: "memory");
        if (*p_test == 0xAA554321UL) {
            *p_test = old_val;
            s_has_psram = 1U;
        }
    }

    if (s_has_psram) {
        maia_console_puts("[DISPLAY] PSRAM verified at 0x48100000! 1280x720 Landscape Framebuffer ACTIVE\n");
        s_fb = (volatile uint16_t *)(uintptr_t)PSRAM_FB_BASE;
    } else {
        maia_console_puts("[DISPLAY] PSRAM not detected, using internal SRAM band fallback\n");
        s_fb = L2MEM_BAND0_CPU;
    }

    /* 6. Setup Terminal Content and Bands before DMA starts */
    term_init_banner();

    /* 7. Configure 10-Item Circular DW_GDMA Linked-List Descriptors in Non-Cacheable Internal SRAM */
    volatile dw_gdma_lli_t *lli = (volatile dw_gdma_lli_t *)((uintptr_t)s_lli_storage + 0x40000000UL);

    for (uint32_t i = 0; i < TAB5_NUM_BLOCKS; ++i) {
        uint32_t src_phys;
        if (s_has_psram) {
            src_phys = PSRAM_FB_BASE + i * (TAB5_BAND_WORDS64 * 8U);
        } else {
            src_phys = L2MEM_BAND2_PHYS;
            if (i == 0U) {
                src_phys = L2MEM_BAND0_PHYS;
            } else if (i == 1U) {
                src_phys = L2MEM_BAND1_PHYS;
            }
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

        /* ctrl_hi: valid=1, last=0 (CIRCULAR RING!), arlen=16, arlen_en=1, awlen=16, awlen_en=1 */
        uint32_t hi = (1UL << 31U) | (16UL << 16U) | (1UL << 15U) | (16UL << 7U) | (1UL << 6U);
        /* DO NOT set shadowreg_or_lli_last (bit 30) - block 9 loops continuously back to block 0 */
        lli[i].ctrl_hi = hi;

        lli[i].sstat       = 0U;
        lli[i].dstat       = 0U;
        lli[i].status_lo   = 0U;
        lli[i].status_hi   = 0U;
        lli[i].reserved_38 = 0U;
        lli[i].reserved_3c = 0U;
    }

    /* Memory barrier to ensure non-cacheable writes are ordered */
    __asm__ volatile ("fence rw, rw" ::: "memory");

    /* 8. Configure & Start DW_GDMA Channel 0 to prefill bridge FIFO */
    MAIA_MMIO32(GDMA_BASE + 0x10UL)  = 3UL; /* dmac_en=1, int_en=1 */
    MAIA_MMIO32(GDMA_BASE + 0x120UL) = (3UL << 2U) | 3UL; /* List transfer */
    /* cfg1: dst_osr_lmt=1, src_osr_lmt=4, ch_prior=3, dst_per=0, tt_fc=1 (M2P_DMAC) */
    MAIA_MMIO32(GDMA_BASE + 0x124UL) = (1UL << 27U) | (4UL << 23U) | (3UL << 17U) | 1UL;

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

    /* Debug dump GDMA and DSI Bridge status */
    maia_console_puts("[DEBUG] GDMA: EN=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x18UL));
    maia_console_puts(" SAR=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x100UL));
    maia_console_puts(" DAR=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x108UL));
    maia_console_puts(" BLKTS=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x110UL));
    maia_console_puts(" LLP=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x128UL));
    maia_console_puts("\n[DEBUG] CFG0=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x120UL));
    maia_console_puts(" CFG1=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x124UL));
    maia_console_puts(" STAT0=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x130UL));
    maia_console_puts(" INT_ST0=");
    disp_print_hex32(MAIA_MMIO32(GDMA_BASE + 0x188UL));
    maia_console_puts("\n[DEBUG] BRG: flow_st=");
    disp_print_hex32(MAIA_MMIO32(DSI_BRG_BASE + 0x54UL));
    maia_console_puts(" raw_buf=");
    disp_print_hex32(MAIA_MMIO32(DSI_BRG_BASE + 0x58UL));
    maia_console_puts(" int_st=");
    disp_print_hex32(MAIA_MMIO32(DSI_BRG_BASE + 0x78UL));
    maia_console_puts("\n");
}

