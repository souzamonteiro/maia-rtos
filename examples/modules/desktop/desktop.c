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
#define COLOR_GOLD      0xfe00
#define COLOR_FOLDER    0xfd60

/* Window Identifiers */
enum {
    WIN_NONE = 0,
    WIN_FILES,
    WIN_EDITOR,
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

    if (w >= 60 && h >= 48 && (len * 16) <= (w - 8)) {
        int text_x = x + (w - len * 16) / 2;
        int text_y = y + (h - 32) / 2;
        draw_str_2x(text_x, text_y, label, fg, pressed ? COLOR_BTN_PR : bg, true);
    } else {
        int text_x = x + (w - len * 8) / 2;
        int text_y = y + (h - 16) / 2;
        draw_str(text_x, text_y, label, fg, pressed ? COLOR_BTN_PR : bg, true);
    }
}

/* --- Math Expression Evaluator for Calculator (Recursive Descent Parser) --- */

static float parse_expression(const char **p, bool *err);

static void skip_whitespace(const char **p) {
    while (**p == ' ' || **p == '\t') (*p)++;
}

static float parse_number(const char **p, bool *err) {
    skip_whitespace(p);
    if (!**p) { *err = true; return 0.0f; }

    float val = 0.0f;
    bool has_digits = false;
    while (**p >= '0' && **p <= '9') {
        val = val * 10.0f + (float)(**p - '0');
        (*p)++;
        has_digits = true;
    }
    if (**p == '.') {
        (*p)++;
        float frac = 0.1f;
        while (**p >= '0' && **p <= '9') {
            val += (float)(**p - '0') * frac;
            frac *= 0.1f;
            (*p)++;
            has_digits = true;
        }
    }
    if (!has_digits) {
        *err = true;
        return 0.0f;
    }
    return val;
}

static float parse_factor(const char **p, bool *err) {
    skip_whitespace(p);
    if (*err || !**p) { *err = true; return 0.0f; }

    /* Unary operators */
    if (**p == '+') {
        (*p)++;
        return parse_factor(p, err);
    }
    if (**p == '-') {
        (*p)++;
        return -parse_factor(p, err);
    }

    /* Sub-expression in parentheses */
    if (**p == '(') {
        (*p)++;
        float val = parse_expression(p, err);
        skip_whitespace(p);
        if (**p == ')') {
            (*p)++;
        } else {
            *err = true;
        }
        return val;
    }

    return parse_number(p, err);
}

static float parse_term(const char **p, bool *err) {
    float left = parse_factor(p, err);
    while (!*err) {
        skip_whitespace(p);
        char op = **p;
        if (op == '*' || op == '/' || op == '%') {
            (*p)++;
            float right = parse_factor(p, err);
            if (*err) break;
            if (op == '*') {
                left *= right;
            } else if (op == '/') {
                if (right == 0.0f) {
                    *err = true;
                    return 0.0f;
                }
                left /= right;
            } else if (op == '%') {
                int iright = (int)right;
                if (iright == 0) {
                    *err = true;
                    return 0.0f;
                }
                left = (float)((int)left % iright);
            }
        } else {
            break;
        }
    }
    return left;
}

static float parse_expression(const char **p, bool *err) {
    float left = parse_term(p, err);
    while (!*err) {
        skip_whitespace(p);
        char op = **p;
        if (op == '+' || op == '-') {
            (*p)++;
            float right = parse_term(p, err);
            if (*err) break;
            if (op == '+') left += right;
            else left -= right;
        } else {
            break;
        }
    }
    return left;
}

static float parse_calc(const char *expr, bool *err) {
    *err = false;
    const char *p = expr;
    float res = parse_expression(&p, err);
    skip_whitespace(&p);
    if (*p != '\0') {
        *err = true;
    }
    return res;
}

static void format_calc_result(float val, char *out, size_t out_sz) {
    if (out_sz < 4) return;
    int j = 0;
    out[j++] = '=';
    out[j++] = ' ';

    if (val < 0.0f) {
        out[j++] = '-';
        val = -val;
    }

    int int_part = (int)val;
    float frac_part = val - (float)int_part;

    char buf[16];
    int bi = 0;
    if (int_part == 0) {
        buf[bi++] = '0';
    } else {
        unsigned int uint_part = (unsigned int)int_part;
        while (uint_part > 0 && bi < 14) {
            buf[bi++] = '0' + (uint_part % 10);
            uint_part /= 10;
        }
    }
    for (int k = bi - 1; k >= 0 && j < (int)out_sz - 1; --k) {
        out[j++] = buf[k];
    }

    /* Fractional part up to 4 decimal places */
    if (frac_part >= 0.0001f && j < (int)out_sz - 8) {
        out[j++] = '.';
        for (int d = 0; d < 4 && j < (int)out_sz - 1; ++d) {
            frac_part *= 10.0f;
            int digit = (int)frac_part;
            if (digit > 9) digit = 9;
            out[j++] = '0' + digit;
            frac_part -= (float)digit;
        }
        while (j > 0 && out[j - 1] == '0') {
            j--;
        }
        if (j > 0 && out[j - 1] == '.') {
            j--;
        }
    }

    out[j] = '\0';
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
        float val = parse_calc(g_calc_expr, &err);
        if (err) {
            const char err_str[] = "= Error";
            for (int i = 0; err_str[i] && i < (int)sizeof(g_calc_res) - 1; ++i) {
                g_calc_res[i] = err_str[i];
                g_calc_res[i + 1] = '\0';
            }
        } else {
            format_calc_result(val, g_calc_res, sizeof(g_calc_res));
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
    int dock_x = 280;
    int dock_y = 642;
    int dock_w = 720;
    int dock_h = 68;

    /* Floating Dock Panel */
    fill_rect(dock_x, dock_y, dock_w, dock_h, COLOR_DOCK_BG);
    draw_rect(dock_x, dock_y, dock_w, dock_h, COLOR_DOCK_BD);
    draw_rect(dock_x + 1, dock_y + 1, dock_w - 2, dock_h - 2, COLOR_DOCK_BD);

    /* 6 App Launcher Buttons (Width=104, Height=48, Gap=14) */
    int bx = dock_x + 18;
    int by = dock_y + 10;
    int bw = 104;
    int bh = 48;
    int gap = 14;

    draw_button(bx, by, bw, bh, "FILES", (active_win == WIN_FILES) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_FILES) ? COLOR_GOLD : COLOR_WHITE, COLOR_BTN_BD, false);
    bx += bw + gap;
    draw_button(bx, by, bw, bh, "EDITOR", (active_win == WIN_EDITOR) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_EDITOR) ? COLOR_CYAN : COLOR_WHITE, COLOR_BTN_BD, false);
    bx += bw + gap;
    draw_button(bx, by, bw, bh, "CALC", (active_win == WIN_CALC) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_CALC) ? COLOR_GREEN : COLOR_WHITE, COLOR_BTN_BD, false);
    bx += bw + gap;
    draw_button(bx, by, bw, bh, "CLOCK", (active_win == WIN_CLOCK) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_CLOCK) ? COLOR_YELLOW : COLOR_WHITE, COLOR_BTN_BD, false);
    bx += bw + gap;
    draw_button(bx, by, bw, bh, "SYSINFO", (active_win == WIN_SYSINFO) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_SYSINFO) ? COLOR_GREEN : COLOR_WHITE, COLOR_BTN_BD, false);
    bx += bw + gap;
    draw_button(bx, by, bw, bh, "BANNER", (active_win == WIN_BANNER) ? COLOR_BTN_PR : COLOR_BTN_BG, (active_win == WIN_BANNER) ? COLOR_MAGENTA : COLOR_WHITE, COLOR_BTN_BD, false);
}

static void render_files_window(void) {
    int wx = 280, wy = 70, ww = 720, wh = 540;

    /* Window Frame */
    fill_rect(wx, wy, ww, wh, COLOR_WIN_BG);
    draw_rect(wx, wy, ww, wh, COLOR_WIN_BD);

    /* Header Bar */
    fill_rect(wx + 1, wy + 1, ww - 2, 36, COLOR_WIN_HDR);
    draw_rect(wx + 1, wy + 37, ww - 2, 1, COLOR_TASK_ACC);
    draw_str_2x(wx + 14, wy + 3, "File Manager (/sd)", COLOR_GOLD, COLOR_WIN_HDR, true);
    draw_button(wx + ww - 44, wy + 5, 36, 26, "X", COLOR_RED, COLOR_WHITE, COLOR_RED, false);

    /* Path / Toolbar */
    int ty = wy + 46;
    draw_button(wx + 16, ty, 86, 30, "[^] UP", COLOR_BTN_BG, COLOR_WHITE, COLOR_BTN_BD, false);
    fill_rect(wx + 112, ty, ww - 260, 30, COLOR_BLACK);
    draw_rect(wx + 112, ty, ww - 260, 30, COLOR_TASK_ACC);
    draw_str(wx + 124, ty + 7, "Path: /sd (FAT32 SDMMC 4-bit)", COLOR_CYAN, COLOR_BLACK, true);
    draw_button(wx + ww - 138, ty, 122, 30, "REFRESH", COLOR_BTN_BG, COLOR_GREEN, COLOR_BTN_BD, false);

    /* File List Header */
    int ly = ty + 40;
    fill_rect(wx + 16, ly, ww - 32, 26, COLOR_TASKBAR);
    draw_rect(wx + 16, ly, ww - 32, 26, COLOR_BTN_BD);
    draw_str(wx + 32,  ly + 5, "NAME", COLOR_TEXT_DIM, COLOR_TASKBAR, true);
    draw_str(wx + 340, ly + 5, "TYPE", COLOR_TEXT_DIM, COLOR_TASKBAR, true);
    draw_str(wx + 490, ly + 5, "SIZE", COLOR_TEXT_DIM, COLOR_TASKBAR, true);
    draw_str(wx + 600, ly + 5, "ACTION", COLOR_TEXT_DIM, COLOR_TASKBAR, true);

    /* File List Rows */
    static const struct {
        const char *name;
        const char *type;
        const char *size;
        uint16_t color;
        bool is_exec;
    } files[] = {
        {"[DIR] testdir",     "Directory",  "-",        COLOR_GOLD,   false},
        {"desktop.mobj",      "MNM-1 Exec", "27.3 KB",  COLOR_CYAN,   true},
        {"calc.mobj",         "MNM-1 Exec", "2.6 KB",   COLOR_CYAN,   true},
        {"hello.mobj",        "MNM-1 Exec", "1.6 KB",   COLOR_CYAN,   true},
        {"sysinfo.mobj",      "MNM-1 Exec", "6.5 KB",   COLOR_CYAN,   true},
        {"banner.mobj",       "MNM-1 Exec", "8.2 KB",   COLOR_CYAN,   true},
        {"notas.txt",         "Plain Text", "512 B",    COLOR_WHITE,  false},
        {"teste_tab5.txt",    "Plain Text", "104 B",    COLOR_WHITE,  false},
        {"desktop.mnm.json",  "JSON Meta",  "425 B",    COLOR_YELLOW, false}
    };

    int row_y = ly + 32;
    for (int i = 0; i < 9; ++i) {
        fill_rect(wx + 16, row_y, ww - 32, 34, (i % 2 == 0) ? COLOR_WIN_BG : 0x1464);
        draw_rect(wx + 16, row_y, ww - 32, 34, 0x2128);

        draw_str(wx + 32,  row_y + 9, files[i].name, files[i].color, (i % 2 == 0) ? COLOR_WIN_BG : 0x1464, true);
        draw_str(wx + 340, row_y + 9, files[i].type, COLOR_TEXT_DIM, (i % 2 == 0) ? COLOR_WIN_BG : 0x1464, true);
        draw_str(wx + 490, row_y + 9, files[i].size, COLOR_TEXT, (i % 2 == 0) ? COLOR_WIN_BG : 0x1464, true);

        if (files[i].is_exec) {
            draw_button(wx + 596, row_y + 4, 76, 26, "RUN", COLOR_GREEN, COLOR_BLACK, COLOR_GREEN, false);
        } else {
            draw_button(wx + 596, row_y + 4, 76, 26, "OPEN", COLOR_BTN_BG, COLOR_CYAN, COLOR_BTN_BD, false);
        }
        row_y += 38;
    }
}

static void render_editor_window(void) {
    int wx = 300, wy = 80, ww = 680, wh = 520;

    /* Window Frame */
    fill_rect(wx, wy, ww, wh, COLOR_WIN_BG);
    draw_rect(wx, wy, ww, wh, COLOR_WIN_BD);

    /* Header Bar */
    fill_rect(wx + 1, wy + 1, ww - 2, 36, COLOR_WIN_HDR);
    draw_rect(wx + 1, wy + 37, ww - 2, 1, COLOR_TASK_ACC);
    draw_str_2x(wx + 14, wy + 3, "Text Editor - /sd/notas.txt", COLOR_CYAN, COLOR_WIN_HDR, true);
    draw_button(wx + ww - 44, wy + 5, 36, 26, "X", COLOR_RED, COLOR_WHITE, COLOR_RED, false);

    /* Toolbar */
    int ty = wy + 46;
    draw_button(wx + 16, ty, 86, 30, "NEW", COLOR_BTN_BG, COLOR_WHITE, COLOR_BTN_BD, false);
    draw_button(wx + 112, ty, 86, 30, "SAVE", COLOR_GREEN, COLOR_BLACK, COLOR_GREEN, false);
    draw_button(wx + 208, ty, 86, 30, "CLEAR", COLOR_RED, COLOR_WHITE, COLOR_RED, false);
    draw_str(wx + 320, ty + 7, "Lines: 8 | Chars: 245 | UTF-8", COLOR_TEXT_DIM, COLOR_WIN_BG, true);

    /* Text Canvas Area */
    int ey = ty + 40;
    int ew = ww - 32;
    int eh = wh - 100;
    fill_rect(wx + 16, ey, ew, eh, COLOR_BLACK);
    draw_rect(wx + 16, ey, ew, eh, COLOR_TASK_ACC);

    /* Line Numbers Gutter */
    fill_rect(wx + 17, ey + 1, 44, eh - 2, COLOR_TASKBAR);
    draw_rect(wx + 61, ey + 1, 1, eh - 2, COLOR_TASK_ACC);

    static const char *editor_lines[] = {
        "# MaiaRTOS Native Text Document",
        "# Hardware: M5Stack Tab5 (ESP32-P4 v1.3)",
        "",
        "void main() {",
        "    maia_desktop_init();",
        "    maia_println(\"Hello MaiaOS!\");",
        "}",
        "/* Type via Tab5 physical keyboard */"
    };

    int cy = ey + 10;
    for (int i = 0; i < 8; ++i) {
        char lnum[6];
        lnum[0] = '0' + ((i + 1) / 10);
        lnum[1] = '0' + ((i + 1) % 10);
        lnum[2] = ' ';
        lnum[3] = '|';
        lnum[4] = '\0';
        draw_str(wx + 22, cy, lnum, COLOR_TEXT_DIM, COLOR_TASKBAR, true);
        draw_str(wx + 72, cy, editor_lines[i], (i == 0 || i == 1) ? COLOR_GOLD : (i == 3 || i == 6) ? COLOR_CYAN : COLOR_WHITE, COLOR_BLACK, true);
        cy += 22;
    }

    /* Blinking-style cursor block at line 7 */
    fill_rect(wx + 72 + 37 * 8, ey + 10 + 7 * 22, 9, 16, COLOR_CYAN);
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
            if (label[0] == '=') {
                bg = COLOR_GREEN;
                fg = COLOR_BLACK;
            } else if (label[0] == 'C' || label[0] == 'D') {
                bg = COLOR_RED;
                fg = COLOR_WHITE;
            } else if (label[0] == '+' || label[0] == '-' || label[0] == '*' || label[0] == '/') {
                bg = COLOR_CYAN;
                fg = COLOR_BLACK;
            } else if (label[0] == '(' || label[0] == ')') {
                bg = COLOR_BTN_BG;
                fg = COLOR_YELLOW;
            }
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

static void render_banner_window(void) {
    int wx = 360, wy = 100, ww = 560, wh = 480;

    /* Window Frame */
    fill_rect(wx, wy, ww, wh, COLOR_WIN_BG);
    draw_rect(wx, wy, ww, wh, COLOR_WIN_BD);

    /* Header Bar */
    fill_rect(wx + 1, wy + 1, ww - 2, 36, COLOR_WIN_HDR);
    draw_rect(wx + 1, wy + 37, ww - 2, 1, COLOR_TASK_ACC);
    draw_str_2x(wx + 14, wy + 3, "Banner Visual FX", COLOR_MAGENTA, COLOR_WIN_HDR, true);
    draw_button(wx + ww - 44, wy + 5, 36, 26, "X", COLOR_RED, COLOR_WHITE, COLOR_RED, false);

    /* Canvas Frame */
    int cx = wx + 20, cy = wy + 50, cw = ww - 40, ch = wh - 70;
    fill_rect(cx, cy, cw, ch, COLOR_BLACK);
    draw_rect(cx, cy, cw, ch, COLOR_MAGENTA);

    /* Static Banner Typography */
    draw_str_2x(cx + 40, cy + 24, "REAL-TIME GRAPHICS ENGINE", COLOR_WHITE, COLOR_BLACK, true);
    draw_str(cx + 40, cy + 62, "Zero-Copy ST7121 MIPI-DSI Framebuffer", COLOR_CYAN, COLOR_BLACK, true);
}

static void render_banner_wave(uint64_t now_us) {
    int wx = 360, wy = 100, ww = 560, wh = 480;
    int cx = wx + 20, cy = wy + 50, cw = ww - 40, ch = wh - 70;

    int wave_x = cx + 8;
    int wave_y = cy + 90;
    int wave_w = cw - 16;
    int wave_h = ch - 100;

    uint32_t shift = (uint32_t)((now_us / 40000ULL) % wave_w);
    static const uint16_t wave_cols[] = {
        COLOR_RED, COLOR_ORANGE, COLOR_YELLOW, COLOR_GREEN, COLOR_CYAN, COLOR_BLUE, COLOR_MAGENTA
    };
    int n_cols = 7;

    /* In-place column update: top is black, bottom is colored bar. Zero flicker! */
    for (int col = 0; col < wave_w; ++col) {
        int idx = (col + shift) / 20;
        uint16_t c = wave_cols[idx % n_cols];
        int bar_h = 24 + ((col * 3 + shift * 2) % (wave_h - 40));
        int top_h = wave_h - bar_h;
        fill_rect(wave_x + col, wave_y, 1, top_h, COLOR_BLACK);
        fill_rect(wave_x + col, wave_y + top_h, 1, bar_h, c);
    }
}

static void draw_desktop_icon(int x, int y, int win_id, const char *label, uint16_t accent_color) {
    /* Subtle acrylic tile for icon */
    fill_rect(x + 8, y, 52, 52, COLOR_WIN_BG);
    draw_rect(x + 8, y, 52, 52, accent_color);

    int gx = x + 18;
    int gy = y + 10;
    switch (win_id) {
        case WIN_FILES:
            fill_rect(gx, gy, 12, 4, COLOR_GOLD);
            fill_rect(gx, gy + 4, 32, 24, COLOR_FOLDER);
            fill_rect(gx + 3, gy + 8, 26, 17, COLOR_GOLD);
            break;
        case WIN_EDITOR:
            fill_rect(gx + 4, gy, 24, 30, COLOR_WHITE);
            draw_rect(gx + 4, gy, 24, 30, COLOR_CYAN);
            fill_rect(gx + 8, gy + 6, 16, 2, COLOR_BLACK);
            fill_rect(gx + 8, gy + 12, 16, 2, COLOR_BLACK);
            fill_rect(gx + 8, gy + 18, 12, 2, COLOR_CYAN);
            break;
        case WIN_CALC:
            fill_rect(gx + 2, gy + 2, 28, 28, COLOR_TASKBAR);
            draw_rect(gx + 2, gy + 2, 28, 28, COLOR_CYAN);
            fill_rect(gx + 5, gy + 5, 22, 6, COLOR_BLACK);
            fill_rect(gx + 5, gy + 13, 5, 4, COLOR_WHITE);
            fill_rect(gx + 13, gy + 13, 5, 4, COLOR_WHITE);
            fill_rect(gx + 21, gy + 13, 5, 4, COLOR_CYAN);
            fill_rect(gx + 5, gy + 20, 5, 4, COLOR_WHITE);
            fill_rect(gx + 13, gy + 20, 5, 4, COLOR_WHITE);
            fill_rect(gx + 21, gy + 20, 5, 4, COLOR_GREEN);
            break;
        case WIN_CLOCK:
            fill_rect(gx + 2, gy + 2, 28, 28, COLOR_WIN_HDR);
            draw_rect(gx + 2, gy + 2, 28, 28, COLOR_YELLOW);
            fill_rect(gx + 15, gy + 6, 2, 11, COLOR_YELLOW);
            fill_rect(gx + 15, gy + 15, 9, 2, COLOR_YELLOW);
            break;
        case WIN_SYSINFO:
            fill_rect(gx + 4, gy + 4, 24, 24, COLOR_TASKBAR);
            draw_rect(gx + 4, gy + 4, 24, 24, COLOR_GREEN);
            fill_rect(gx + 9, gy + 9, 14, 14, COLOR_GREEN);
            for (int p = 0; p < 4; ++p) {
                fill_rect(gx + 6 + p * 5, gy, 2, 4, COLOR_CYAN);
                fill_rect(gx + 6 + p * 5, gy + 28, 2, 4, COLOR_CYAN);
                fill_rect(gx, gy + 6 + p * 5, 4, 2, COLOR_CYAN);
                fill_rect(gx + 28, gy + 6 + p * 5, 4, 2, COLOR_CYAN);
            }
            break;
        case WIN_BANNER:
            fill_rect(gx + 2, gy + 4, 28, 22, COLOR_BLACK);
            draw_rect(gx + 2, gy + 4, 28, 22, COLOR_MAGENTA);
            fill_rect(gx + 5, gy + 16, 4, 6, COLOR_RED);
            fill_rect(gx + 11, gy + 11, 4, 11, COLOR_YELLOW);
            fill_rect(gx + 17, gy + 7, 4, 15, COLOR_GREEN);
            fill_rect(gx + 23, gy + 13, 4, 9, COLOR_CYAN);
            break;
        default: break;
    }

    int len = 0;
    const char *p = label;
    while (*p++) len++;
    int lx = x + (68 - len * 8) / 2;
    draw_str(lx, y + 58, label, COLOR_TEXT, 0x1107, true);
}

static void render_canvas_background(void) {
    /* 1. Vertical Gradient Wallpaper from Midnight Blue (top) to Slate Blue (bottom) */
    for (int y = 36; y < SCREEN_H; ++y) {
        int r = 1 + (y - 36) * 3 / 684;
        int g = 3 + (y - 36) * 6 / 684;
        int b = 6 + (y - 36) * 7 / 684;
        uint16_t line_col = (uint16_t)((r << 11) | (g << 5) | b);
        fill_rect(0, y, SCREEN_W, 1, line_col);
    }

    /* Subtle geometric grid / accent dots */
    for (int y = 60; y < SCREEN_H - 90; y += 48) {
        for (int x = 140; x < SCREEN_W - 40; x += 48) {
            put_pixel(x, y, 0x21a8);
        }
    }

    /* Centered OS Logo Watermark */
    draw_str_2x(440, 240, "[ MAIA OS DESKTOP ]", COLOR_TASK_ACC, 0x1509, true);
    draw_str_2x(450, 280, "NATIVE RV32 DUAL-CORE", COLOR_TEXT, 0x1509, true);
    draw_str(470, 325, "Touch any shortcut or dock icon to open", COLOR_TEXT_DIM, 0x1509, true);

    /* 2. Desktop Shortcut Icons (Left Column) */
    draw_desktop_icon(24, 60,  WIN_FILES,   "Arquivos", COLOR_GOLD);
    draw_desktop_icon(24, 152, WIN_EDITOR,  "Editor",   COLOR_CYAN);
    draw_desktop_icon(24, 244, WIN_CALC,    "Calc",     COLOR_GREEN);
    draw_desktop_icon(24, 336, WIN_CLOCK,   "Relogio",  COLOR_YELLOW);
    draw_desktop_icon(24, 428, WIN_SYSINFO, "Sistema",  COLOR_GREEN);
    draw_desktop_icon(24, 520, WIN_BANNER,  "Banner",   COLOR_MAGENTA);
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
            case WIN_FILES:   render_files_window(); break;
            case WIN_EDITOR:  render_editor_window(); break;
            case WIN_CALC:    render_calc_window(); break;
            case WIN_CLOCK:   render_clock_window(now_us); break;
            case WIN_SYSINFO: render_sysinfo_window(); break;
            case WIN_BANNER:
                render_banner_window();
                render_banner_wave(now_us);
                break;
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
            render_banner_wave(now_us);
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
        if (g_active_win == WIN_FILES) {
            close_x1 = 280 + 720 - 46; close_x2 = close_x1 + 40;
            close_y1 = 70 + 4;         close_y2 = close_y1 + 30;
        } else if (g_active_win == WIN_EDITOR) {
            close_x1 = 300 + 680 - 46; close_x2 = close_x1 + 40;
            close_y1 = 80 + 4;         close_y2 = close_y1 + 30;
        } else if (g_active_win == WIN_CALC) {
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

    /* 3. Check Desktop Left Column Shortcut Icons (When no window captures input) */
    if (g_active_win == WIN_NONE && tx >= 16 && tx <= 104) {
        int clicked = WIN_NONE;
        if (ty >= 55 && ty <= 135) clicked = WIN_FILES;
        else if (ty >= 147 && ty <= 227) clicked = WIN_EDITOR;
        else if (ty >= 239 && ty <= 319) clicked = WIN_CALC;
        else if (ty >= 331 && ty <= 411) clicked = WIN_CLOCK;
        else if (ty >= 423 && ty <= 503) clicked = WIN_SYSINFO;
        else if (ty >= 515 && ty <= 595) clicked = WIN_BANNER;

        if (clicked != WIN_NONE) {
            g_active_win = clicked;
            g_dirty_canvas = true;
            g_dirty_dock = true;
            g_dirty_window = true;
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

    /* 5. Check Bottom Dock Launchers (6 Buttons) */
    if (ty >= 642 && ty <= 714) {
        int dock_x = 280;
        int bx = dock_x + 18;
        int bw = 104;
        int gap = 14;
        int clicked = WIN_NONE;

        if (tx >= bx && tx <= bx + bw) clicked = WIN_FILES;
        bx += bw + gap;
        if (tx >= bx && tx <= bx + bw) clicked = WIN_EDITOR;
        bx += bw + gap;
        if (tx >= bx && tx <= bx + bw) clicked = WIN_CALC;
        bx += bw + gap;
        if (tx >= bx && tx <= bx + bw) clicked = WIN_CLOCK;
        bx += bw + gap;
        if (tx >= bx && tx <= bx + bw) clicked = WIN_SYSINFO;
        bx += bw + gap;
        if (tx >= bx && tx <= bx + bw) clicked = WIN_BANNER;

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
                if (k == 27 || (k == 'q' && g_active_win == WIN_NONE) || k == 3) { /* ESC or 'q' or Ctrl+C */
                    if (g_active_win != WIN_NONE) {
                        g_active_win = WIN_NONE;
                        g_dirty_canvas = true;
                        g_dirty_dock = true;
                    } else {
                        g_running = false;
                        break;
                    }
                } else if (g_active_win == WIN_NONE) {
                    int next_win = WIN_NONE;
                    if (k == 'f' || k == 'F' || k == '1') next_win = WIN_FILES;
                    else if (k == 'e' || k == 'E' || k == '2') next_win = WIN_EDITOR;
                    else if (k == 'c' || k == 'C' || k == '3') next_win = WIN_CALC;
                    else if (k == 't' || k == 'T' || k == '4') next_win = WIN_CLOCK;
                    else if (k == 's' || k == 'S' || k == '5') next_win = WIN_SYSINFO;
                    else if (k == 'b' || k == 'B' || k == '6') next_win = WIN_BANNER;
                    if (next_win != WIN_NONE) {
                        g_active_win = next_win;
                        g_dirty_canvas = true;
                        g_dirty_dock = true;
                        g_dirty_window = true;
                    }
                } else if (g_active_win == WIN_CALC) {
                    if (k == '\r' || k == '\n' || k == '=') calc_input('=');
                    else if (k == '\b' || k == 127) calc_input('D');
                    else if (k == 'c' || k == 'C') calc_input('C');
                    else if (k >= 32 && k <= 126) calc_input((char)k);
                    g_dirty_window = true;
                } else if (g_active_win == WIN_CLOCK) {
                    if (k == ' ' || k == '\r' || k == '\n') {
                        if (!g_sw_running) {
                            g_sw_start_us = now_us;
                            g_sw_running = true;
                        } else {
                            g_sw_elapsed_us += (now_us - g_sw_start_us);
                            g_sw_running = false;
                        }
                        g_dirty_window = true;
                    } else if (k == 'r' || k == 'R') {
                        g_sw_running = false;
                        g_sw_elapsed_us = 0;
                        g_dirty_window = true;
                    }
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

