#include <maia/exec/mnm.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

#include "font_8x16.h"

/* Logical Screen Size */
#define SCREEN_W 1280
#define SCREEN_H 720

/* Color Palette RGB565 */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_BG        0x0863  /* Midnight Blue #0b0f19 */
#define COLOR_TASKBAR   0x10c5  /* Dark Acrylic #141824 */
#define COLOR_TASK_ACC  0x07bf  /* Neon Cyan accent #00f0ff */
#define COLOR_TEXT      0xefbe  /* Off-white */
#define COLOR_TEXT_DIM  0x8410  /* Muted Gray */
#define COLOR_CYAN      0x07ff
#define COLOR_GREEN     0x07e0
#define COLOR_YELLOW    0xffe0
#define COLOR_RED       0xf800
#define COLOR_ORANGE    0xfd20
#define COLOR_MAGENTA   0xf81f
#define COLOR_BLUE      0x001f
#define COLOR_DOCK_BG   0x10e4
#define COLOR_DOCK_BD   0x29ec
#define COLOR_WIN_BG    0x18e5
#define COLOR_WIN_HDR   0x2128
#define COLOR_WIN_BD    0x3a0c
#define COLOR_BTN_BG    0x2169
#define COLOR_BTN_PR    0x04bf
#define COLOR_BTN_BD    0x4a4d

/* Window Identifiers */
enum {
    WIN_NONE = 0,
    WIN_CALC,
    WIN_CLOCK,
    WIN_SYSINFO,
    WIN_BANNER
};

/* Module Global State */
static const maia_module_context_t *g_ctx = NULL;
static uint16_t *g_fb = NULL;
static int g_active_win = WIN_NONE;
static int g_prev_win = -1;
static bool g_running = true;

/* Selective Redraw / Dirty State Flags (LVGL-style Invalidation) */
static bool g_dirty_canvas  = true;
static bool g_dirty_taskbar = true;
static bool g_dirty_dock    = true;
static bool g_dirty_window  = true;
static uint32_t g_prev_sec  = 0xFFFFFFFF;

/* Stopwatch State */
static bool g_sw_running = false;
static uint64_t g_sw_start_us = 0;
static uint64_t g_sw_elapsed_us = 0;

/* Calculator State */
static char g_calc_expr[64] = "0";
static char g_calc_res[32] = "";
static bool g_calc_clear_on_next = true;

/* --- Drawing Primitives --- */

static inline void put_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    if (g_fb) {
        /* Direct RGB565 framebuffer: Tab5 ST7121 rotated layout px = y; py = 1279 - x */
        int px = y;
        int py = 1279 - x;
        g_fb[py * 720 + px] = color;
    } else if (g_ctx && g_ctx->host && g_ctx->host->display && g_ctx->host->display->put_pixel) {
        g_ctx->host->display->put_pixel(x, y, color);
    }
}

static void fill_rect(int x, int y, int w, int h, uint16_t color) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    if (w <= 0 || h <= 0) return;

    if (g_fb) {
        for (int row = y; row < y + h; ++row) {
            for (int col = x; col < x + w; ++col) {
                int px = row;
                int py = 1279 - col;
                g_fb[py * 720 + px] = color;
            }
        }
    } else if (g_ctx && g_ctx->host && g_ctx->host->display && g_ctx->host->display->fill_rect) {
        g_ctx->host->display->fill_rect(x, y, w, h, color);
    }
}

static void draw_rect(int x, int y, int w, int h, uint16_t color) {
    fill_rect(x, y, w, 1, color);
    fill_rect(x, y + h - 1, w, 1, color);
    fill_rect(x, y, 1, h, color);
    fill_rect(x + w - 1, y, 1, h, color);
}

static void draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, bool transparent) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = &g_font_8x16[(c - 32) * 16];
    for (int r = 0; r < 16; ++r) {
        uint8_t line = glyph[r];
        for (int bit = 0; bit < 8; ++bit) {
            if (line & (0x80 >> bit)) {
                put_pixel(x + bit, y + r, fg);
            } else if (!transparent) {
                put_pixel(x + bit, y + r, bg);
            }
        }
    }
}

static void draw_str(int x, int y, const char *str, uint16_t fg, uint16_t bg, bool transparent) {
    if (!str) return;
    int cur_x = x;
    while (*str) {
        draw_char(cur_x, y, *str++, fg, bg, transparent);
        cur_x += 8;
    }
}

static void draw_char_2x(int x, int y, char c, uint16_t fg, uint16_t bg, bool transparent) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = &g_font_8x16[(c - 32) * 16];
    for (int r = 0; r < 16; ++r) {
        uint8_t line = glyph[r];
        for (int bit = 0; bit < 8; ++bit) {
            uint16_t col = (line & (0x80 >> bit)) ? fg : bg;
            if ((line & (0x80 >> bit)) || !transparent) {
                put_pixel(x + bit * 2,     y + r * 2,     col);
                put_pixel(x + bit * 2 + 1, y + r * 2,     col);
                put_pixel(x + bit * 2,     y + r * 2 + 1, col);
                put_pixel(x + bit * 2 + 1, y + r * 2 + 1, col);
            }
        }
    }
}

static void draw_str_2x(int x, int y, const char *str, uint16_t fg, uint16_t bg, bool transparent) {
    if (!str) return;
    int cur_x = x;
    while (*str) {
        draw_char_2x(cur_x, y, *str++, fg, bg, transparent);
        cur_x += 16;
    }
}

static void draw_button(int x, int y, int w, int h, const char *label, uint16_t bg, uint16_t fg, uint16_t border, bool pressed) {
    fill_rect(x, y, w, h, pressed ? COLOR_BTN_PR : bg);
    draw_rect(x, y, w, h, border);
    int len = 0;
    const char *p = label;
    while (*p++) len++;
    int text_x = x + (w - len * 8) / 2;
    int text_y = y + (h - 16) / 2;
    draw_str(text_x, text_y, label, fg, pressed ? COLOR_BTN_PR : bg, true);
}

/* --- Math Expression Evaluator for Calculator --- */

static int64_t parse_calc(const char *expr, bool *err) {
    *err = false;
    const char *p = expr;
    while (*p == ' ') p++;
    if (!*p) return 0;

    int64_t left = 0;
    bool neg = false;
    if (*p == '-') { neg = true; p++; }
    while (*p >= '0' && *p <= '9') {
        left = left * 10 + (*p - '0');
        p++;
    }
    if (neg) left = -left;

    while (*p == ' ') p++;
    if (!*p) return left;

    char op = *p++;
    while (*p == ' ') p++;

    int64_t right = 0;
    bool neg_r = false;
    if (*p == '-') { neg_r = true; p++; }
    while (*p >= '0' && *p <= '9') {
        right = right * 10 + (*p - '0');
        p++;
    }
    if (neg_r) right = -right;

    switch (op) {
        case '+': return left + right;
        case '-': return left - right;
        case '*': return left * right;
        case '/':
            if (right == 0) { *err = true; return 0; }
            return left / right;
        default:
            *err = true;
            return left;
    }
}

static void calc_input(char key) {
    size_t len = 0;
    while (g_calc_expr[len]) len++;

    if (key == 'C') {
        g_calc_expr[0] = '0';
        g_calc_expr[1] = '\0';
        g_calc_res[0] = '\0';
        g_calc_clear_on_next = true;
        return;
    }
    if (key == 'D') { // DEL
        if (len > 1) {
            g_calc_expr[len - 1] = '\0';
        } else {
            g_calc_expr[0] = '0';
            g_calc_expr[1] = '\0';
        }
        return;
    }
    if (key == '=') {
        bool err = false;
        int64_t val = parse_calc(g_calc_expr, &err);
        if (err) {
            g_calc_res[0] = 'E'; g_calc_res[1] = 'r'; g_calc_res[2] = 'r'; g_calc_res[3] = '\0';
        } else {
            char buf[32];
            int i = 0;
            bool neg = (val < 0);
            uint64_t uval = neg ? -val : val;
            if (uval == 0) buf[i++] = '0';
            while (uval > 0 && i < 28) {
                buf[i++] = '0' + (uval % 10);
                uval /= 10;
            }
            if (neg) buf[i++] = '-';
            int j = 0;
            g_calc_res[j++] = '=';
            g_calc_res[j++] = ' ';
            for (int k = i - 1; k >= 0; --k) {
                g_calc_res[j++] = buf[k];
            }
            g_calc_res[j] = '\0';
        }
        g_calc_clear_on_next = true;
        return;
    }

    if (g_calc_clear_on_next && (key >= '0' && key <= '9')) {
        g_calc_expr[0] = key;
        g_calc_expr[1] = '\0';
        g_calc_res[0] = '\0';
        g_calc_clear_on_next = false;
        return;
    }

    g_calc_clear_on_next = false;
    if (len == 1 && g_calc_expr[0] == '0' && (key >= '0' && key <= '9')) {
        g_calc_expr[0] = key;
        return;
    }

    if (len + 3 < sizeof(g_calc_expr)) {
        if (key == '+' || key == '-' || key == '*' || key == '/') {
            g_calc_expr[len++] = ' ';
            g_calc_expr[len++] = key;
            g_calc_expr[len++] = ' ';
            g_calc_expr[len] = '\0';
        } else {
            g_calc_expr[len++] = key;
            g_calc_expr[len] = '\0';
        }
    }
}

/* --- UI Rendering Functions --- */

static void render_taskbar_time(uint64_t now_us) {
    uint32_t sec = (uint32_t)(now_us / 1000000ULL);
    uint32_t hrs = (sec / 3600) % 24;
    uint32_t min = (sec / 60) % 60;
    uint32_t s = sec % 60;
    char time_str[10];
    time_str[0] = '0' + (hrs / 10);
    time_str[1] = '0' + (hrs % 10);
    time_str[2] = ':';
    time_str[3] = '0' + (min / 10);
    time_str[4] = '0' + (min % 10);
    time_str[5] = ':';
    time_str[6] = '0' + (s / 10);
    time_str[7] = '0' + (s % 10);
    time_str[8] = '\0';
    fill_rect(576, 2, 128, 32, COLOR_TASKBAR);
    draw_str_2x(576, 2, time_str, COLOR_WHITE, COLOR_TASKBAR, true);
}

static void render_taskbar(uint64_t now_us) {
    fill_rect(0, 0, SCREEN_W, 36, COLOR_TASKBAR);
    fill_rect(0, 35, SCREEN_W, 1, COLOR_TASK_ACC);

    /* Left: OS Branding */
    draw_str_2x(16, 2, "[ MAIA OS ]", COLOR_CYAN, COLOR_TASKBAR, true);
    draw_str(160, 10, "Desktop Environment", COLOR_TEXT_DIM, COLOR_TASKBAR, true);

    /* Center: Clock & Uptime */
    render_taskbar_time(now_us);

    /* Right: Hardware Profile & Exit Button */
    draw_str(850, 10, "PSRAM: 30.3MB", COLOR_GREEN, COLOR_TASKBAR, true);
    draw_str(980, 10, "L2: 311KB", COLOR_YELLOW, COLOR_TASKBAR, true);
    draw_str(1070, 10, "360MHz", COLOR_CYAN, COLOR_TASKBAR, true);

    /* Exit Button */
    draw_button(1160, 4, 104, 28, "X EXIT", COLOR_RED, COLOR_WHITE, COLOR_RED, false);
}

static void render_dock(int active_win) {
    int dock_x = 340;
    int dock_y = 642;
    int dock_w = 600;
    int dock_h = 68;

    /* Floating Dock Panel */
    fill_rect(dock_x, dock_y, dock_w, dock_h, COLOR_DOCK_BG);
    draw_rect(dock_x, dock_y, dock_w, dock_h, COLOR_DOCK_BD);
    draw_rect(dock_x + 1, dock_y + 1, dock_w - 2, dock_h - 2, COLOR_DOCK_BD);

    /* 4 App Launcher Buttons (Width=130, Height=48, Gap=15) */
    int bx = dock_x + 16;
    int by = dock_y + 10;
    int bw = 130;
    int bh = 48;

    draw_button(bx, by, bw, bh, "CALC", (active_win == WIN_CALC) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_CALC) ? COLOR_CYAN : COLOR_WHITE, COLOR_BTN_BD, false);
    bx += bw + 14;
    draw_button(bx, by, bw, bh, "CLOCK", (active_win == WIN_CLOCK) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_CLOCK) ? COLOR_YELLOW : COLOR_WHITE, COLOR_BTN_BD, false);
    bx += bw + 14;
    draw_button(bx, by, bw, bh, "SYSINFO", (active_win == WIN_SYSINFO) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_SYSINFO) ? COLOR_GREEN : COLOR_WHITE, COLOR_BTN_BD, false);
    bx += bw + 14;
    draw_button(bx, by, bw, bh, "BANNER", (active_win == WIN_BANNER) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_BANNER) ? COLOR_MAGENTA : COLOR_WHITE, COLOR_BTN_BD, false);
}

static void render_calc_window(void) {
    int wx = 415, wy = 70, ww = 450, wh = 550;

    /* Window Frame */
    fill_rect(wx, wy, ww, wh, COLOR_WIN_BG);
    draw_rect(wx, wy, ww, wh, COLOR_WIN_BD);

    /* Header Bar */
    fill_rect(wx + 1, wy + 1, ww - 2, 36, COLOR_WIN_HDR);
    draw_rect(wx + 1, wy + 37, ww - 2, 1, COLOR_TASK_ACC);
    draw_str_2x(wx + 14, wy + 3, "Calculator (calc)", COLOR_CYAN, COLOR_WIN_HDR, true);
    draw_button(wx + ww - 44, wy + 5, 36, 26, "X", COLOR_RED, COLOR_WHITE, COLOR_RED, false);

    /* Display Screen */
    int dx = wx + 20, dy = wy + 48, dw = ww - 40, dh = 72;
    fill_rect(dx, dy, dw, dh, COLOR_BLACK);
    draw_rect(dx, dy, dw, dh, COLOR_CYAN);
    draw_str(dx + 12, dy + 10, g_calc_expr, COLOR_TEXT_DIM, COLOR_BLACK, true);
    draw_str_2x(dx + 12, dy + 32, g_calc_res[0] ? g_calc_res : "= 0", COLOR_GREEN, COLOR_BLACK, true);

    /* Keypad Grid (4 cols x 5 rows) */
    static const char *keys[5][4] = {
        {"C", "(", ")", "/"},
        {"7", "8", "9", "*"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"},
        {"0", ".", "DEL", "="}
    };

    int kx_start = wx + 20;
    int ky_start = wy + 132;
    int kw = 92;
    int kh = 70;
    int gap = 14;

    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 4; ++c) {
            int kx = kx_start + c * (kw + gap);
            int ky = ky_start + r * (kh + gap);
            const char *label = keys[r][c];
            uint16_t bg = COLOR_BTN_BG;
            uint16_t fg = COLOR_WHITE;
            if (label[0] == '=') { bg = COLOR_GREEN; fg = COLOR_BLACK; }
            else if (label[0] == 'C' || label[0] == 'D') { bg = COLOR_RED; fg = COLOR_WHITE; }
            else if (label[0] == '+' || label[0] == '-' || label[0] == '*' || label[0] == '/') { bg = COLOR_ORANGE; fg = COLOR_BLACK; }
            draw_button(kx, ky, kw, kh, label, bg, fg, COLOR_BTN_BD, false);
        }
    }
}

static void render_clock_uptime(uint64_t now_us) {
    int wx = 430, wy = 120;
    uint32_t sec = (uint32_t)(now_us / 1000000ULL);
    uint32_t hrs = (sec / 3600) % 24;
    uint32_t min = (sec / 60) % 60;
    uint32_t s = sec % 60;
    char clk_buf[10];
    clk_buf[0] = '0' + (hrs / 10);
    clk_buf[1] = '0' + (hrs % 10);
    clk_buf[2] = ':';
    clk_buf[3] = '0' + (min / 10);
    clk_buf[4] = '0' + (min % 10);
    clk_buf[5] = ':';
    clk_buf[6] = '0' + (s / 10);
    clk_buf[7] = '0' + (s % 10);
    clk_buf[8] = '\0';
    fill_rect(wx + 130, wy + 80, 160, 36, COLOR_WIN_BG);
    draw_str_2x(wx + 140, wy + 82, clk_buf, COLOR_CYAN, COLOR_WIN_BG, true);
}

static void render_clock_stopwatch_digits(uint64_t now_us) {
    int wx = 430, wy = 120, ww = 420;
    int sw_box_x = wx + 50, sw_box_y = wy + 175;
    uint64_t cur_sw = g_sw_elapsed_us;
    if (g_sw_running) {
        cur_sw += (now_us - g_sw_start_us);
    }
    uint32_t sw_s = (uint32_t)(cur_sw / 1000000ULL);
    uint32_t sw_m = sw_s / 60;
    sw_s %= 60;
    uint32_t sw_ms = (uint32_t)((cur_sw % 1000000ULL) / 10000ULL); // Hundredths

    char sw_buf[10];
    sw_buf[0] = '0' + ((sw_m / 10) % 10);
    sw_buf[1] = '0' + (sw_m % 10);
    sw_buf[2] = ':';
    sw_buf[3] = '0' + (sw_s / 10);
    sw_buf[4] = '0' + (sw_s % 10);
    sw_buf[5] = '.';
    sw_buf[6] = '0' + (sw_ms / 10);
    sw_buf[7] = '0' + (sw_ms % 10);
    sw_buf[8] = '\0';

    fill_rect(sw_box_x + 80, sw_box_y + 15, 160, 40, COLOR_BLACK);
    draw_str_2x(sw_box_x + 90, sw_box_y + 18, sw_buf, COLOR_YELLOW, COLOR_BLACK, true);
}

static void render_clock_window(uint64_t now_us) {
    int wx = 430, wy = 120, ww = 420, wh = 450;

    /* Window Frame */
    fill_rect(wx, wy, ww, wh, COLOR_WIN_BG);
    draw_rect(wx, wy, ww, wh, COLOR_WIN_BD);

    /* Header Bar */
    fill_rect(wx + 1, wy + 1, ww - 2, 36, COLOR_WIN_HDR);
    draw_rect(wx + 1, wy + 37, ww - 2, 1, COLOR_TASK_ACC);
    draw_str_2x(wx + 14, wy + 3, "Clock & Stopwatch", COLOR_YELLOW, COLOR_WIN_HDR, true);
    draw_button(wx + ww - 44, wy + 5, 36, 26, "X", COLOR_RED, COLOR_WHITE, COLOR_RED, false);

    /* Section 1: Uptime Digital Clock */
    draw_str(wx + 30, wy + 56, "SYSTEM MONOTONIC TIME", COLOR_TEXT_DIM, COLOR_WIN_BG, true);
    render_clock_uptime(now_us);

    /* Section 2: Interactive Stopwatch */
    fill_rect(wx + 20, wy + 130, ww - 40, 2, COLOR_DOCK_BD);
    draw_str(wx + 30, wy + 146, "HIGH-PRECISION STOPWATCH", COLOR_TEXT_DIM, COLOR_WIN_BG, true);

    int sw_box_x = wx + 50, sw_box_y = wy + 175, sw_box_w = ww - 100, sw_box_h = 70;
    fill_rect(sw_box_x, sw_box_y, sw_box_w, sw_box_h, COLOR_BLACK);
    draw_rect(sw_box_x, sw_box_y, sw_box_w, sw_box_h, COLOR_YELLOW);
    render_clock_stopwatch_digits(now_us);

    /* Stopwatch Control Buttons */
    int btn_w = 100;
    int btn_h = 52;
    int btn_y = wy + 270;
    draw_button(wx + 30, btn_y, btn_w, btn_h, "START", COLOR_GREEN, COLOR_BLACK, COLOR_GREEN, g_sw_running);
    draw_button(wx + 160, btn_y, btn_w, btn_h, "STOP", COLOR_RED, COLOR_WHITE, COLOR_RED, !g_sw_running);
    draw_button(wx + 290, btn_y, btn_w, btn_h, "RESET", COLOR_BTN_BG, COLOR_WHITE, COLOR_BTN_BD, false);

    draw_str(wx + 50, wy + 350, "Tap START to measure elapsed time.", COLOR_TEXT_DIM, COLOR_WIN_BG, true);
    draw_str(wx + 50, wy + 372, "Measured via host get_time_us()", COLOR_TEXT_DIM, COLOR_WIN_BG, true);
}

static void render_sysinfo_window(void) {
    int wx = 380, wy = 90, ww = 520, wh = 510;

    /* Window Frame */
    fill_rect(wx, wy, ww, wh, COLOR_WIN_BG);
    draw_rect(wx, wy, ww, wh, COLOR_WIN_BD);

    /* Header Bar */
    fill_rect(wx + 1, wy + 1, ww - 2, 36, COLOR_WIN_HDR);
    draw_rect(wx + 1, wy + 37, ww - 2, 1, COLOR_TASK_ACC);
    draw_str_2x(wx + 14, wy + 3, "System Monitor (ESP32-P4)", COLOR_GREEN, COLOR_WIN_HDR, true);
    draw_button(wx + ww - 44, wy + 5, 36, 26, "X", COLOR_RED, COLOR_WHITE, COLOR_RED, false);

    int cur_y = wy + 54;
    draw_str_2x(wx + 24, cur_y, "Hardware Telemetry", COLOR_WHITE, COLOR_WIN_BG, true);
    cur_y += 38;

    draw_str(wx + 24, cur_y, "Processor Architecture:", COLOR_CYAN, COLOR_WIN_BG, true);
    cur_y += 20;
    draw_str(wx + 40, cur_y, "RISC-V HP Dual-Core @ 360 MHz (RV32IMAFCP)", COLOR_TEXT, COLOR_WIN_BG, true);
    cur_y += 18;
    draw_str(wx + 40, cur_y, "Hardware Extensions: FPU, DSP, 2D PPA, MIPI-DSI", COLOR_TEXT_DIM, COLOR_WIN_BG, true);
    cur_y += 28;

    draw_str(wx + 24, cur_y, "Memory Hierarchy:", COLOR_CYAN, COLOR_WIN_BG, true);
    cur_y += 20;
    draw_str(wx + 40, cur_y, "Octal PSRAM @ 200MHz: 30,328 / 32,768 KB (92% free)", COLOR_TEXT, COLOR_WIN_BG, true);
    cur_y += 18;
    /* PSRAM Bar */
    int bar_w = ww - 80;
    fill_rect(wx + 40, cur_y, bar_w, 14, COLOR_BLACK);
    fill_rect(wx + 40, cur_y, (bar_w * 92) / 100, 14, COLOR_GREEN);
    draw_rect(wx + 40, cur_y, bar_w, 14, COLOR_TEXT_DIM);
    cur_y += 24;

    draw_str(wx + 40, cur_y, "Internal L2 SRAM:     311 / 440 KB (70% free)", COLOR_TEXT, COLOR_WIN_BG, true);
    cur_y += 18;
    /* SRAM Bar */
    fill_rect(wx + 40, cur_y, bar_w, 14, COLOR_BLACK);
    fill_rect(wx + 40, cur_y, (bar_w * 70) / 100, 14, COLOR_YELLOW);
    draw_rect(wx + 40, cur_y, bar_w, 14, COLOR_TEXT_DIM);
    cur_y += 28;

    draw_str(wx + 24, cur_y, "Peripherals & I/O Subsystems:", COLOR_CYAN, COLOR_WIN_BG, true);
    cur_y += 20;
    draw_str(wx + 40, cur_y, "Display: ST7121 MIPI-DSI (1280x720 Landscape)", COLOR_TEXT, COLOR_WIN_BG, true);
    cur_y += 18;
    draw_str(wx + 40, cur_y, "Touch:   ST7123 Capacitive Multi-Touch (I2C 0x55)", COLOR_TEXT, COLOR_WIN_BG, true);
    cur_y += 18;
    draw_str(wx + 40, cur_y, "Storage: SDMMC 4-bit FAT32 (/sd) mounted", COLOR_TEXT, COLOR_WIN_BG, true);
    cur_y += 18;
    draw_str(wx + 40, cur_y, "Keybd:   Ext.Port1 TCA8418/STM32 Matrix (I2C 0x6D)", COLOR_TEXT, COLOR_WIN_BG, true);
}

static void render_banner_window(uint64_t now_us) {
    int wx = 360, wy = 100, ww = 560, wh = 480;

    /* Window Frame */
    fill_rect(wx, wy, ww, wh, COLOR_WIN_BG);
    draw_rect(wx, wy, ww, wh, COLOR_WIN_BD);

    /* Header Bar */
    fill_rect(wx + 1, wy + 1, ww - 2, 36, COLOR_WIN_HDR);
    draw_rect(wx + 1, wy + 37, ww - 2, 1, COLOR_TASK_ACC);
    draw_str_2x(wx + 14, wy + 3, "Banner Visual FX", COLOR_MAGENTA, COLOR_WIN_HDR, true);
    draw_button(wx + ww - 44, wy + 5, 36, 26, "X", COLOR_RED, COLOR_WHITE, COLOR_RED, false);

    /* Animated Wave Canvas */
    int cx = wx + 20, cy = wy + 50, cw = ww - 40, ch = wh - 70;
    fill_rect(cx, cy, cw, ch, COLOR_BLACK);
    draw_rect(cx, cy, cw, ch, COLOR_MAGENTA);

    uint32_t shift = (uint32_t)((now_us / 40000ULL) % cw);
    static const uint16_t wave_cols[] = {
        COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_MAGENTA
    };
    int n_cols = 7;

    for (int col = 0; col < cw; ++col) {
        int idx = (col + shift) / 20;
        uint16_t c = wave_cols[idx % n_cols];
        int bar_h = 40 + ((col * 3 + shift * 2) % (ch - 80));
        fill_rect(cx + col, cy + ch - bar_h - 10, 1, bar_h, c);
    }

    draw_str_2x(cx + 40, cy + 30, "REAL-TIME GRAPHICS ENGINE", COLOR_WHITE, COLOR_BLACK, true);
    draw_str(cx + 40, cy + 70, "Zero-Copy ST7121 MIPI-DSI Framebuffer", COLOR_CYAN, COLOR_BLACK, true);
}

static void render_canvas_background(void) {
    /* 1. Canvas Background with watermark */
    fill_rect(0, 36, SCREEN_W, SCREEN_H - 36, COLOR_BG);

    /* Subtle geometric grid / dots */
    for (int y = 50; y < SCREEN_H - 80; y += 40) {
        for (int x = 40; x < SCREEN_W; x += 40) {
            put_pixel(x, y, 0x18e5);
        }
    }

    /* Centered OS Logo Watermark */
    draw_str_2x(440, 260, "MAIA OPERATING SYSTEM", COLOR_TASKBAR, COLOR_BG, true);
    draw_str_2x(460, 300, "NATIVE RV32 DUAL-CORE", COLOR_TASKBAR, COLOR_BG, true);
    draw_str(510, 345, "Touch any app in the dock to begin", COLOR_TEXT_DIM, COLOR_BG, true);
}

static void render_desktop(uint64_t now_us) {
    bool need_sync = false;
    uint32_t sec = (uint32_t)(now_us / 1000000ULL);

    /* 1. Full Canvas Background Invalidation */
    if (g_dirty_canvas) {
        render_canvas_background();
        g_dirty_taskbar = true;
        g_dirty_dock    = true;
        g_dirty_window  = true;
        g_dirty_canvas  = false;
        need_sync = true;
    }

    /* 2. Taskbar Redraw or Partial Time Update */
    if (g_dirty_taskbar) {
        render_taskbar(now_us);
        g_dirty_taskbar = false;
        g_prev_sec = sec;
        need_sync = true;
    } else if (sec != g_prev_sec) {
        render_taskbar_time(now_us);
        if (g_active_win == WIN_CLOCK) {
            render_clock_uptime(now_us);
        }
        g_prev_sec = sec;
        need_sync = true;
    }

    /* 3. Dock Redraw */
    if (g_dirty_dock) {
        render_dock(g_active_win);
        g_dirty_dock = false;
        need_sync = true;
    }

    /* 4. Active Window Redraw */
    if (g_dirty_window) {
        switch (g_active_win) {
            case WIN_CALC:    render_calc_window(); break;
            case WIN_CLOCK:   render_clock_window(now_us); break;
            case WIN_SYSINFO: render_sysinfo_window(); break;
            case WIN_BANNER:  render_banner_window(now_us); break;
            default: break;
        }
        g_dirty_window = false;
        need_sync = true;
    } else {
        /* Continuous animations for specific windows */
        if (g_active_win == WIN_CLOCK && g_sw_running) {
            render_clock_stopwatch_digits(now_us);
            need_sync = true;
        } else if (g_active_win == WIN_BANNER) {
            render_banner_window(now_us);
            need_sync = true;
        }
    }

    /* 5. Sync Framebuffer ONLY if something changed */
    if (need_sync && g_ctx && g_ctx->host && g_ctx->host->display && g_ctx->host->display->sync) {
        g_ctx->host->display->sync();
    }
}

/* --- Touch & Input Event Handler --- */

static void handle_touch(int tx, int ty) {
    /* 1. Check Taskbar Exit Button */
    if (ty >= 0 && ty <= 36) {
        if (tx >= 1150 && tx <= 1270) {
            g_running = false;
            return;
        }
    }

    /* 2. Check Active Window Close Button [X] */
    if (g_active_win != WIN_NONE) {
        int close_x1 = 0, close_x2 = 0, close_y1 = 0, close_y2 = 0;
        if (g_active_win == WIN_CALC) {
            close_x1 = 415 + 450 - 46; close_x2 = close_x1 + 40;
            close_y1 = 70 + 4;         close_y2 = close_y1 + 30;
        } else if (g_active_win == WIN_CLOCK) {
            close_x1 = 430 + 420 - 46; close_x2 = close_x1 + 40;
            close_y1 = 120 + 4;        close_y2 = close_y1 + 30;
        } else if (g_active_win == WIN_SYSINFO) {
            close_x1 = 380 + 520 - 46; close_x2 = close_x1 + 40;
            close_y1 = 90 + 4;         close_y2 = close_y1 + 30;
        } else if (g_active_win == WIN_BANNER) {
            close_x1 = 360 + 560 - 46; close_x2 = close_x1 + 40;
            close_y1 = 100 + 4;        close_y2 = close_y1 + 30;
        }

        if (tx >= close_x1 && tx <= close_x2 && ty >= close_y1 && ty <= close_y2) {
            g_active_win = WIN_NONE;
            g_dirty_canvas = true;
            g_dirty_dock = true;
            return;
        }
    }

    /* 3. Check Calculator Window Keypad */
    if (g_active_win == WIN_CALC) {
        static const char keys_map[5][4] = {
            {'C', '(', ')', '/'},
            {'7', '8', '9', '*'},
            {'4', '5', '6', '-'},
            {'1', '2', '3', '+'},
            {'0', '.', 'D', '='}
        };
        int kx_start = 415 + 20;
        int ky_start = 70 + 132;
        int kw = 92;
        int kh = 70;
        int gap = 14;

        for (int r = 0; r < 5; ++r) {
            for (int c = 0; c < 4; ++c) {
                int kx = kx_start + c * (kw + gap);
                int ky = ky_start + r * (kh + gap);
                if (tx >= kx && tx <= kx + kw && ty >= ky && ty <= ky + kh) {
                    calc_input(keys_map[r][c]);
                    g_dirty_window = true;
                    return;
                }
            }
        }
    }

    /* 4. Check Clock & Stopwatch Buttons */
    if (g_active_win == WIN_CLOCK) {
        int wx = 430, wy = 120;
        int btn_w = 100, btn_h = 52, btn_y = wy + 270;
        if (ty >= btn_y && ty <= btn_y + btn_h) {
            if (tx >= wx + 30 && tx <= wx + 30 + btn_w) {
                /* START */
                if (!g_sw_running && g_ctx && g_ctx->host && g_ctx->host->get_time_us) {
                    g_sw_running = true;
                    g_sw_start_us = g_ctx->host->get_time_us();
                }
                g_dirty_window = true;
                return;
            } else if (tx >= wx + 160 && tx <= wx + 160 + btn_w) {
                /* STOP */
                if (g_sw_running && g_ctx && g_ctx->host && g_ctx->host->get_time_us) {
                    g_sw_running = false;
                    g_sw_elapsed_us += (g_ctx->host->get_time_us() - g_sw_start_us);
                }
                g_dirty_window = true;
                return;
            } else if (tx >= wx + 290 && tx <= wx + 290 + btn_w) {
                /* RESET */
                g_sw_running = false;
                g_sw_elapsed_us = 0;
                g_dirty_window = true;
                return;
            }
        }
    }

    /* 5. Check Bottom Dock Launchers */
    if (ty >= 642 && ty <= 714) {
        int dock_x = 340;
        int bx = dock_x + 16;
        int bw = 130;
        int clicked = WIN_NONE;
        if (tx >= bx && tx <= bx + bw) {
            clicked = WIN_CALC;
        }
        bx += bw + 14;
        if (tx >= bx && tx <= bx + bw) {
            clicked = WIN_CLOCK;
        }
        bx += bw + 14;
        if (tx >= bx && tx <= bx + bw) {
            clicked = WIN_SYSINFO;
        }
        bx += bw + 14;
        if (tx >= bx && tx <= bx + bw) {
            clicked = WIN_BANNER;
        }

        if (clicked != WIN_NONE) {
            if (g_active_win == clicked) {
                g_active_win = WIN_NONE;
            } else {
                g_active_win = clicked;
            }
            g_dirty_canvas = true;
            g_dirty_dock = true;
            g_dirty_window = true;
            return;
        }
    }
}

/* --- Entry Point --- */

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    g_ctx = ctx;

    /* 1. Request exclusive screen control (suppresses terminal output) */
    if (ctx->host->display && ctx->host->display->acquire_exclusive) {
        ctx->host->display->acquire_exclusive();
    }

    /* 2. Retrieve Direct Framebuffer if available */
    if (ctx->host->display && ctx->host->display->get_framebuffer) {
        g_fb = ctx->host->display->get_framebuffer();
    }

    /* 3. Main GUI Event Loop */
    g_running = true;
    bool was_pressed = false;

    while (g_running) {
        uint64_t now_us = (ctx->host->get_time_us) ? ctx->host->get_time_us() : 0;

        /* Poll Touch Input */
        if (ctx->host->input && ctx->host->input->get_touch) {
            int32_t tx = 0, ty = 0, pressed = 0;
            ctx->host->input->get_touch(&tx, &ty, &pressed);

            if (pressed && !was_pressed) {
                /* Touch Press Event */
                handle_touch(tx, ty);
                was_pressed = true;
            } else if (!pressed && was_pressed) {
                was_pressed = false;
            }
        }

        /* Poll Keyboard Input */
        if (ctx->host->input && ctx->host->input->read_key) {
            int32_t k = ctx->host->input->read_key();
            while (k >= 0) {
                if (k == 27 || k == 'q' || k == 3) { /* ESC or 'q' or Ctrl+C */
                    if (g_active_win != WIN_NONE) {
                        g_active_win = WIN_NONE;
                        g_dirty_canvas = true;
                        g_dirty_dock = true;
                    } else {
                        g_running = false;
                        break;
                    }
                } else if (g_active_win == WIN_CALC) {
                    if (k == '\r' || k == '\n' || k == '=') calc_input('=');
                    else if (k == '\b' || k == 127) calc_input('D');
                    else if (k >= 32 && k <= 126) calc_input((char)k);
                    g_dirty_window = true;
                }
                k = ctx->host->input->read_key();
            }
        }

        /* Render Desktop Frame */
        render_desktop(now_us);

        /* Sleep ~30ms to maintain ~33 FPS without hogging CPU */
        if (ctx->host->sleep_ms) {
            ctx->host->sleep_ms(30);
        }
    }

    /* 4. Release exclusive display control back to shell terminal */
    if (ctx->host->display && ctx->host->display->release_exclusive) {
        ctx->host->display->release_exclusive();
    }

    return 0;
}

