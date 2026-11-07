#pragma once

#include <maia/exec/elf_loader.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "font_8x16.h"

#define SCREEN_W        1280
#define SCREEN_H        720

/* MaiaOS Canonical RGB565 Color Palette */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xffff
#define COLOR_CYAN      0x07ff
#define COLOR_GREEN     0x07e0
#define COLOR_YELLOW    0xffe0
#define COLOR_RED       0xf800
#define COLOR_ORANGE    0xfd20
#define COLOR_MAGENTA   0xf81f
#define COLOR_BLUE      0x001f
#define COLOR_GOLD      0xfe00
#define COLOR_FOLDER    0xfd60
#define COLOR_GRAY      0x7bef
#define COLOR_DARKGRAY  0x39e7
#define COLOR_BG        0x0842
#define COLOR_PANEL     0x10a2
#define COLOR_TASKBAR   0x18e3
#define COLOR_TASK_ACC  0x07ff

#define COLOR_WIN_BG    0x18e5
#define COLOR_WIN_HDR   0x2128
#define COLOR_WIN_BD    0x3a0c
#define COLOR_BTN_BG    0x2169
#define COLOR_BTN_PR    0x04bf
#define COLOR_BTN_BD    0x4a4d
#define COLOR_TEXT      0xffff
#define COLOR_TEXT_DIM  0x9cd3

static const maia_module_context_t *g_gui_ctx = NULL;
static int g_font_scale = 2; /* Default: doubled font (16x32) for high-res 1280x720 */

static int g_clip_x1 = 0, g_clip_y1 = 0, g_clip_x2 = SCREEN_W, g_clip_y2 = SCREEN_H;

static inline void gui_set_clip(int x, int y, int w, int h) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
    g_clip_x1 = x;
    g_clip_y1 = y;
    g_clip_x2 = x + w;
    g_clip_y2 = y + h;
}

static inline void gui_reset_clip(void) {
    g_clip_x1 = 0;
    g_clip_y1 = 0;
    g_clip_x2 = SCREEN_W;
    g_clip_y2 = SCREEN_H;
}

static inline void gui_set_font_scale(int scale) {
    if (scale < 1) scale = 1;
    if (scale > 2) scale = 2;
    g_font_scale = scale;
}

static inline int gui_get_font_scale(void) {
    return g_font_scale;
}

static inline void gui_init(const maia_module_context_t *ctx) {
    g_gui_ctx = ctx;
    gui_reset_clip();

    /* Automatically acquire exclusive graphics mode so keyboard and touch route to GUI */
    if (ctx && ctx->host && ctx->host->display && ctx->host->display->acquire_exclusive) {
        ctx->host->display->acquire_exclusive();
    }

    /* Check if custom font_scale is configured in /sd/desktop.json or /sd/apps/desktop.mnm.json */
    if (ctx && ctx->host && ctx->host->read_text_file) {
        char cfg_buf[256];
        int rd = ctx->host->read_text_file("/sd/desktop.json", cfg_buf, sizeof(cfg_buf) - 1);
        if (rd <= 0) {
            rd = ctx->host->read_text_file("/sd/apps/desktop.mnm.json", cfg_buf, sizeof(cfg_buf) - 1);
        }
        if (rd > 0) {
            cfg_buf[rd] = '\0';
            for (int i = 0; i < rd - 12; ++i) {
                if (cfg_buf[i] == 'f' && cfg_buf[i+1] == 'o' && cfg_buf[i+2] == 'n' &&
                    cfg_buf[i+3] == 't' && cfg_buf[i+4] == '_' && cfg_buf[i+5] == 's' &&
                    cfg_buf[i+6] == 'c' && cfg_buf[i+7] == 'a' && cfg_buf[i+8] == 'l' &&
                    cfg_buf[i+9] == 'e') {
                    int p = i + 10;
                    while (p < rd && (cfg_buf[p] == ' ' || cfg_buf[p] == ':' || cfg_buf[p] == '"')) p++;
                    if (p < rd) {
                        if (cfg_buf[p] == '1') g_font_scale = 1;
                        else if (cfg_buf[p] == '2') g_font_scale = 2;
                    }
                    break;
                }
            }
        }
    }
}

static inline void gui_out_str(const char *s) {
    (void)s;
    /* Suppress debug stdout so characters never corrupt graphical display */
}

static inline void gui_put_pixel(int x, int y, uint16_t c) {
    if (x < g_clip_x1 || x >= g_clip_x2 || y < g_clip_y1 || y >= g_clip_y2) return;
    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->display && g_gui_ctx->host->display->put_pixel) {
        g_gui_ctx->host->display->put_pixel(x, y, c);
    }
}

static inline void gui_fill_rect(int x, int y, int w, int h, uint16_t c) {
    if (x < g_clip_x1) { w -= (g_clip_x1 - x); x = g_clip_x1; }
    if (y < g_clip_y1) { h -= (g_clip_y1 - y); y = g_clip_y1; }
    if (x + w > g_clip_x2) w = g_clip_x2 - x;
    if (y + h > g_clip_y2) h = g_clip_y2 - y;
    if (w <= 0 || h <= 0) return;
    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->display && g_gui_ctx->host->display->fill_rect) {
        g_gui_ctx->host->display->fill_rect(x, y, w, h, c);
    }
}

static inline void gui_draw_rect(int x, int y, int w, int h, uint16_t c) {
    gui_fill_rect(x, y, w, 1, c);
    gui_fill_rect(x, y + h - 1, w, 1, c);
    gui_fill_rect(x, y, 1, h, c);
    gui_fill_rect(x + w - 1, y, 1, h, c);
}

static inline void gui_sync(void) {
    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->display && g_gui_ctx->host->display->sync) {
        g_gui_ctx->host->display->sync();
    }
}

static inline void gui_draw_char(int x, int y, char c, uint16_t fg, uint16_t bg, bool transparent) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = &g_font_8x16[(c - 32) * 16];
    for (int r = 0; r < 16; ++r) {
        uint8_t line = glyph[r];
        for (int bit = 0; bit < 8; ++bit) {
            if (line & (0x80 >> bit)) {
                gui_put_pixel(x + bit, y + r, fg);
            } else if (!transparent) {
                gui_put_pixel(x + bit, y + r, bg);
            }
        }
    }
}

static inline void gui_draw_str(int x, int y, const char *str, uint16_t fg, uint16_t bg, bool transparent) {
    if (!str) return;
    int cur_x = x;
    while (*str) {
        gui_draw_char(cur_x, y, *str++, fg, bg, transparent);
        cur_x += 8;
    }
}

static inline void gui_draw_char_1_5x(int x, int y, char c, uint16_t fg, uint16_t bg, bool transparent) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = &g_font_8x16[(c - 32) * 16];
    for (int cy = 0; cy < 24; ++cy) {
        int sy = (cy * 2) / 3;
        uint8_t line = glyph[sy];
        for (int cx = 0; cx < 12; ++cx) {
            int sx = (cx * 2) / 3;
            if (line & (0x80 >> sx)) {
                gui_put_pixel(x + cx, y + cy, fg);
            } else if (!transparent) {
                gui_put_pixel(x + cx, y + cy, bg);
            }
        }
    }
}

static inline void gui_draw_str_1_5x(int x, int y, const char *str, uint16_t fg, uint16_t bg, bool transparent) {
    if (!str) return;
    int cur_x = x;
    while (*str) {
        gui_draw_char_1_5x(cur_x, y, *str++, fg, bg, transparent);
        cur_x += 12;
    }
}

static inline void gui_draw_char_2x(int x, int y, char c, uint16_t fg, uint16_t bg, bool transparent) {
    if (c < 32 || c > 126) c = '?';
    const uint8_t *glyph = &g_font_8x16[(c - 32) * 16];
    for (int r = 0; r < 16; ++r) {
        uint8_t line = glyph[r];
        for (int bit = 0; bit < 8; ++bit) {
            uint16_t col = (line & (0x80 >> bit)) ? fg : bg;
            if ((line & (0x80 >> bit)) || !transparent) {
                gui_put_pixel(x + bit * 2,     y + r * 2,     col);
                gui_put_pixel(x + bit * 2 + 1, y + r * 2,     col);
                gui_put_pixel(x + bit * 2,     y + r * 2 + 1, col);
                gui_put_pixel(x + bit * 2 + 1, y + r * 2 + 1, col);
            }
        }
    }
}

static inline void gui_draw_str_2x(int x, int y, const char *str, uint16_t fg, uint16_t bg, bool transparent) {
    if (!str) return;
    int cur_x = x;
    while (*str) {
        gui_draw_char_2x(cur_x, y, *str++, fg, bg, transparent);
        cur_x += 16;
    }
}

static inline void gui_draw_text(int x, int y, const char *str, uint16_t fg, uint16_t bg, bool transparent) {
    if (!str) return;
    if (g_font_scale == 2) {
        gui_draw_str_2x(x, y, str, fg, bg, transparent);
    } else {
        gui_draw_str(x, y, str, fg, bg, transparent);
    }
}

static inline void gui_draw_button(int x, int y, int w, int h, const char *label, uint16_t bg, uint16_t fg, uint16_t border, bool pressed) {
    uint16_t fill = pressed ? COLOR_BTN_PR : bg;
    gui_fill_rect(x, y, w, h, fill);
    gui_draw_rect(x, y, w, h, border);

    int len = 0;
    while (label[len]) len++;

    /* Use 2x typography if button has enough height and width */
    if (g_font_scale == 2 && h >= 32 && (len * 16 <= w - 8)) {
        int tx = x + (w - len * 16) / 2;
        int ty = y + (h - 32) / 2;
        gui_draw_str_2x(tx, ty, label, fg, fill, true);
    } else {
        int tx = x + (w - len * 8) / 2;
        int ty = y + (h - 16) / 2;
        gui_draw_str(tx, ty, label, fg, fill, true);
    }
}

/* Standard MaiaOS Window Frame with Titlebar and Close Button [X] */
static inline void gui_draw_window_frame(int x, int y, int w, int h, const char *title, uint16_t accent_color) {
    /* Shadow */
    gui_fill_rect(x + 6, y + 6, w, h, 0x0821);

    /* Body */
    gui_fill_rect(x, y, w, h, COLOR_WIN_BG);
    gui_draw_rect(x, y, w, h, COLOR_WIN_BD);
    gui_draw_rect(x + 1, y + 1, w - 2, h - 2, accent_color);

    /* Titlebar */
    gui_fill_rect(x + 2, y + 2, w - 4, 34, COLOR_WIN_HDR);
    gui_fill_rect(x + 2, y + 36, w - 4, 2, accent_color);

    /* Accent dot */
    gui_fill_rect(x + 14, y + 12, 14, 14, accent_color);

    /* Title text */
    gui_draw_str(x + 36, y + 11, title, COLOR_WHITE, COLOR_WIN_HDR, true);

    /* Close Button [X] */
    int cx = x + w - 42;
    int cy = y + 6;
    gui_fill_rect(cx, cy, 32, 26, COLOR_RED);
    gui_draw_rect(cx, cy, 32, 26, COLOR_WHITE);
    gui_draw_str(cx + 12, cy + 5, "X", COLOR_WHITE, COLOR_RED, true);
}

static inline bool gui_is_close_clicked(int x, int y, int w, int tx, int ty) {
    int cx = x + w - 50;
    return (tx >= cx && tx <= x + w + 10 && ty >= y && ty <= y + 38);
}

/* Touch input poller that correctly handles driver return convention */
static inline bool gui_poll_touch(int32_t *out_x, int32_t *out_y, bool *out_pressed) {
    if (!g_gui_ctx || !g_gui_ctx->host || !g_gui_ctx->host->input || !g_gui_ctx->host->input->get_touch) {
        if (out_pressed) *out_pressed = false;
        return false;
    }
    int32_t x = 0, y = 0, p = 0;
    int32_t res = g_gui_ctx->host->input->get_touch(&x, &y, &p);
    if (res > 0 || p != 0) {
        if (out_x) *out_x = x;
        if (out_y) *out_y = y;
        if (out_pressed) *out_pressed = true;
        return true;
    }
    if (out_x) *out_x = x;
    if (out_y) *out_y = y;
    if (out_pressed) *out_pressed = false;
    return (res >= 0);
}

static inline int32_t gui_poll_key(void) {
    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->input && g_gui_ctx->host->input->read_key) {
        return g_gui_ctx->host->input->read_key();
    }
    return -1;
}

static inline void gui_delay_ms(uint32_t ms) {
    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->sleep_ms) {
        g_gui_ctx->host->sleep_ms(ms);
    }
}
