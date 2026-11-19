#include <maia/arch/irq.h>
#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/services/lcd_output.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_display.h>
#include <maia/soc/esp32p4_psram.h>

#include "dsi_framebuffer_probe.h"
#include "soc_display_native.h"

#define TAB5_DISP_WIDTH  720U
#define TAB5_DISP_HEIGHT 1280U
#define TAB5_PHYS_W      720
#define TAB5_PHYS_H      1280
#define TAB5_LOG_W       1280
#define TAB5_LOG_H       720
#define CHAR_W           16
#define CHAR_H           32
#define TERM_COLS        (TAB5_LOG_W / CHAR_W)
#define TERM_ROWS        (TAB5_LOG_H / CHAR_H)
#define PROMPT_ROW_START 4

#define COLOR_BLACK  0x0000U
#define COLOR_WHITE  0xFFFFU
#define COLOR_CYAN   0x07FFU
#define COLOR_GREEN  0x07E0U
#define COLOR_YELLOW 0xFFE0U
#define COLOR_GRAY   0x8410U

static uint16_t *s_fb;
static char s_term_lines[TERM_ROWS][TERM_COLS + 1];
static uint8_t s_term_cur_row = PROMPT_ROW_START;
static uint8_t s_term_cur_col;
static uint8_t s_display_ready;
static int s_exclusive_mode;
static volatile uint32_t s_dirty_rows;
static uint8_t s_lcd_stack[8192] __attribute__((aligned(16)));
static uint8_t s_term_stack[2048] __attribute__((aligned(16)));

#include "font_8x16.h"

static void disp_print_hex32(uint32_t value) {
    static const char hex[] = "0123456789ABCDEF";
    char text[11] = "0x00000000";
    for (unsigned i = 0; i < 8U; ++i) {
        text[2U + i] = hex[(value >> ((7U - i) * 4U)) & 0xFU];
    }
    maia_console_puts(text);
}

static inline void put_pixel_rot3(int x, int y, uint16_t color) {
    if (x < 0 || x >= TAB5_LOG_W || y < 0 || y >= TAB5_LOG_H) {
        return;
    }
    int px = y;
    int py = 1279 - x;
    s_fb[py * TAB5_PHYS_W + px] = color;
}

static void draw_char_2x(int col, int row, char ch, uint16_t fg, uint16_t bg) {
    if (col < 0 || col >= (int)TERM_COLS || row < 0 || row >= (int)TERM_ROWS) {
        return;
    }
    if (ch < 32 || ch > 126) {
        ch = ' ';
    }
    const uint8_t *glyph = &g_font_8x16[((int)ch - 32) * 16];
    int px_x = col * CHAR_W;
    int px_y = row * CHAR_H;
    for (int y = 0; y < 16; ++y) {
        uint8_t bits = glyph[y];
        for (int x = 0; x < 8; ++x) {
            uint16_t color = (bits & (0x80U >> x)) ? fg : bg;
            int dst_x = px_x + x * 2;
            int dst_y = px_y + y * 2;
            put_pixel_rot3(dst_x, dst_y, color);
            put_pixel_rot3(dst_x + 1, dst_y, color);
            put_pixel_rot3(dst_x, dst_y + 1, color);
            put_pixel_rot3(dst_x + 1, dst_y + 1, color);
        }
    }
}

static uint16_t row_color(int row) {
    switch (row) {
    case 0: return COLOR_CYAN;
    case 1: return COLOR_YELLOW;
    case 2: return COLOR_GREEN;
    case 3: return COLOR_GRAY;
    default: return COLOR_WHITE;
    }
}

static void term_scroll(void) {
    for (int row = PROMPT_ROW_START; row < (int)TERM_ROWS - 1; ++row) {
        for (int col = 0; col < (int)TERM_COLS; ++col) {
            s_term_lines[row][col] = s_term_lines[row + 1][col];
        }
    }
    for (int col = 0; col < (int)TERM_COLS; ++col) {
        s_term_lines[TERM_ROWS - 1][col] = ' ';
    }
    s_term_cur_row = (uint8_t)(TERM_ROWS - 1U);
    s_dirty_rows |= ((1UL << TERM_ROWS) - 1UL) &
                    ~((1UL << PROMPT_ROW_START) - 1UL);
}

static void term_newline(void) {
    s_dirty_rows |= 1UL << s_term_cur_row;
    s_term_cur_col = 0U;
    if (++s_term_cur_row >= (uint8_t)TERM_ROWS) {
        term_scroll();
    }
    s_dirty_rows |= 1UL << s_term_cur_row;
}

static void term_putc(char ch) {
    uintptr_t irq = maia_arch_irq_save();
    if (ch == '\r') {
        s_term_cur_col = 0U;
    } else if (ch == '\n') {
        term_newline();
    } else if (ch == '\b' || ch == 127) {
        if (s_term_cur_col > 0U) {
            s_term_lines[s_term_cur_row][--s_term_cur_col] = ' ';
        }
    } else if (ch == '\t') {
        uint8_t next_tab = (uint8_t)((s_term_cur_col + 4U) & ~3U);
        while (s_term_cur_col < next_tab && s_term_cur_col < (uint8_t)TERM_COLS) {
            s_term_lines[s_term_cur_row][s_term_cur_col++] = ' ';
        }
    } else if ((uint8_t)ch >= 32U && (uint8_t)ch <= 126U) {
        if (s_term_cur_col >= (uint8_t)TERM_COLS) {
            term_newline();
        }
        s_term_lines[s_term_cur_row][s_term_cur_col++] = ch;
    }
    s_dirty_rows |= 1UL << s_term_cur_row;
    maia_arch_irq_restore(irq);
}

static void draw_row(int row) {
    char line[TERM_COLS];
    uintptr_t irq = maia_arch_irq_save();
    for (int col = 0; col < (int)TERM_COLS; ++col) {
        line[col] = s_term_lines[row][col];
    }
    int cursor = row == (int)s_term_cur_row ? (int)s_term_cur_col : -1;
    maia_arch_irq_restore(irq);

    uint16_t fg = row_color(row);
    for (int col = 0; col < (int)TERM_COLS; ++col) {
        if (col == cursor) {
            draw_char_2x(col, row, line[col], COLOR_BLACK, COLOR_WHITE);
        } else {
            draw_char_2x(col, row, line[col], fg, COLOR_BLACK);
        }
    }
}

static void draw_terminal(uint16_t *pixels, uint32_t rows) {
    s_fb = pixels;
    for (int row = 0; row < (int)TERM_ROWS; ++row) {
        if (rows & (1UL << row)) {
            draw_row(row);
        }
    }
}

static void lcd_draw(uint16_t *pixels, uint32_t width, uint32_t height, void *arg) {
    (void)arg;
    if (width != TAB5_PHYS_W || height != TAB5_PHYS_H) {
        return;
    }
    uintptr_t irq = maia_arch_irq_save();
    uint32_t rows = s_dirty_rows;
    s_dirty_rows = 0U;
    maia_arch_irq_restore(irq);
    draw_terminal(pixels, rows);
}

static void lcd_task(void *arg) {
    (void)arg;
    maia_status_t result = maia_lcd_output_run();
    maia_console_puts("[DISPLAY] Native LCD service stopped result=");
    disp_print_hex32((uint32_t)result);
    maia_console_puts("\n");
    for (;;) {
        maia_task_delay_ticks(1000U);
    }
}

static void terminal_task(void *arg) {
    (void)arg;
    for (;;) {
        if (s_dirty_rows != 0U) {
            (void)maia_lcd_output_request_update(lcd_draw, NULL);
        }
        maia_task_delay_ticks(20U);
    }
}

static void report_step(const char *step, uint32_t result, uint32_t stage) {
    maia_console_puts("[DISPLAY] NATIVE_LCD step=");
    maia_console_puts(step);
    maia_console_puts(" result=");
    disp_print_hex32(result);
    maia_console_puts(" stage=");
    disp_print_hex32(stage);
    maia_console_puts("\n");
}

static const char *const banner[PROMPT_ROW_START] = {
    " === MaiaRTOS (M5Stack Tab5 1280x720) - native LCD service, sem FreeRTOS ===",
    " Display: ST7121 MIPI-DSI | PSRAM framebuffer via servico LCD aprovado",
    " Status: MaiaShell Ready. Type on physical keyboard or USB-Serial.",
    "--------------------------------------------------------------------------------",
};

static int native_lcd_start(void) {
    maia_psram_cache_report_t cache;
    maia_psram_snapshot_t snapshot;
    maia_psram_clock_report_t clock;
    maia_psram_mode_switch_report_t mode;
    maia_psram_fast_report_t fast;
    maia_psram_window_report_t window;
    maia_psram_result_t result;

    result = maia_esp32p4_psram_cache_adopt(&cache);
    report_step("cache_adopt", (uint32_t)result, cache.stage);
    if (result != MAIA_PSRAM_OK) return -1;
    result = maia_esp32p4_psram_snapshot(&snapshot);
    report_step("snapshot", (uint32_t)result, 0U);
    if (result != MAIA_PSRAM_OK) return -1;
    result = maia_esp32p4_psram_xtal20_start(&clock);
    report_step("xtal20_start", (uint32_t)result, clock.stage);
    if (result != MAIA_PSRAM_OK) return -1;
    result = maia_esp32p4_psram_x8_mode(0U, 10000U, &mode);
    report_step("x8_mode", (uint32_t)result, mode.stage);
    if (result != MAIA_PSRAM_OK) return -1;
    result = maia_esp32p4_psram_fast_start(200U, 10000U, &fast);
    report_step("fast_start", (uint32_t)result, fast.mhz);
    if (result != MAIA_PSRAM_OK) return -1;
    result = maia_esp32p4_psram_window_start(10000U, &window);
    report_step("window_start", (uint32_t)result, window.stage);
    if (result != MAIA_PSRAM_OK) return -1;

    uint16_t *fb = (uint16_t *)(uintptr_t)DSI_FB_SOURCE;
    for (uint32_t i = 0; i < TAB5_DISP_WIDTH * TAB5_DISP_HEIGHT; ++i) {
        fb[i] = COLOR_BLACK;
    }
    for (int row = 0; row < (int)TERM_ROWS; ++row) {
        for (int col = 0; col < (int)TERM_COLS; ++col) {
            s_term_lines[row][col] = ' ';
        }
        s_term_lines[row][TERM_COLS] = '\0';
    }
    for (int row = 0; row < PROMPT_ROW_START; ++row) {
        for (int col = 0; col < (int)TERM_COLS && banner[row][col] != '\0'; ++col) {
            s_term_lines[row][col] = banner[row][col];
        }
    }
    s_term_cur_row = PROMPT_ROW_START;
    s_term_cur_col = 0U;
    draw_terminal(fb, (1UL << TERM_ROWS) - 1UL);
    result = maia_esp32p4_psram_window_sync(10000U, &window);
    report_step("window_sync", (uint32_t)result, window.stage);
    if (result != MAIA_PSRAM_OK) return -1;

    maia_task_id_t lcd_id = -1;
    maia_task_id_t terminal_id = -1;
    maia_status_t status = maia_task_create("lcd", lcd_task, NULL,
        MAIA_PRIO_HIGH + 1U, s_lcd_stack, sizeof(s_lcd_stack), &lcd_id);
    report_step("lcd_task", (uint32_t)status, (uint32_t)lcd_id);
    if (status != MAIA_OK) return -1;
    status = maia_lcd_output_init(lcd_id);
    report_step("lcd_output_init", (uint32_t)status, 0U);
    if (status != MAIA_OK) return -1;
    status = maia_task_create("lcd_term", terminal_task, NULL,
        MAIA_PRIO_HIGH, s_term_stack, sizeof(s_term_stack), &terminal_id);
    report_step("term_task", (uint32_t)status, (uint32_t)terminal_id);
    if (status != MAIA_OK) return -1;

    maia_tab5_backlight_set_duty(4095U);
    s_display_ready = 1U;
    maia_console_puts("[DISPLAY] Native LCD service ready (shell terminal on PSRAM framebuffer)\n");
    return 0;
}

void maia_display_init(uint32_t panel_type) {
    uint8_t panel_firmware = 0U;
    if (panel_type != MAIA_TAB5_PANEL_ST7121 ||
        maia_tab5_get_detected_touch_firmware(&panel_firmware) != 0 ||
        panel_firmware != 1U) {
        maia_console_puts("[DISPLAY] Native product LCD requires a detected ST7121 firmware 1 panel\n");
        return;
    }
    maia_console_puts("[DISPLAY] Using guarded native ST7121 DSI initialization\n");
    if (maia_esp32p4_display_st7121_prepare(panel_firmware) != 0) {
        return;
    }
    if (native_lcd_start() != 0) {
        maia_console_puts("[DISPLAY] Native LCD startup failed; no fallback video path\n");
    }
}

void maia_display_set_exclusive(int exclusive) {
    s_exclusive_mode = exclusive;
}

void maia_display_putc(char ch) {
    if (!s_exclusive_mode && s_display_ready) {
        term_putc(ch);
    }
}

void maia_display_puts(const char *text) {
    if (text == NULL) {
        return;
    }
    while (*text != '\0') {
        maia_display_putc(*text++);
    }
}

void maia_display_write(const char *buffer, size_t length) {
    if (buffer == NULL) {
        return;
    }
    for (size_t i = 0; i < length; ++i) {
        maia_display_putc(buffer[i]);
    }
}

void maia_display_clear(void) {
    if (!s_display_ready) {
        return;
    }
    uintptr_t irq = maia_arch_irq_save();
    for (int row = 0; row < (int)TERM_ROWS; ++row) {
        for (int col = 0; col < (int)TERM_COLS; ++col) {
            s_term_lines[row][col] = ' ';
        }
    }
    s_term_cur_row = PROMPT_ROW_START;
    s_term_cur_col = 0U;
    s_dirty_rows = (1UL << TERM_ROWS) - 1UL;
    maia_arch_irq_restore(irq);
}

void maia_display_refresh_dma_if_needed(void) {
}
