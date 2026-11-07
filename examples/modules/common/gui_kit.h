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

#define COLOR_WIN_BG    0x18e5
#define COLOR_WIN_HDR   0x2128
#define COLOR_WIN_BD    0x3a0c
#define COLOR_BTN_BG    0x2169
#define COLOR_BTN_PR    0x04bf
#define COLOR_BTN_BD    0x4a4d
#define COLOR_TEXT      0xffff
#define COLOR_TEXT_DIM  0x9cd3

static const maia_module_context_t *g_gui_ctx = NULL;

static inline void gui_init(const maia_module_context_t *ctx) {
    g_gui_ctx = ctx;
}

static inline void gui_out_str(const char *s) {
    if (!g_gui_ctx || !g_gui_ctx->host || !g_gui_ctx->host->write_fd || !s) return;
    uint32_t len = 0;
    const char *p = s;
    while (*p++) len++;
    g_gui_ctx->host->write_fd(g_gui_ctx->stdout_fd, s, len);
}

static inline void gui_put_pixel(int x, int y, uint16_t c) {
    if (x < 0 || x >= SCREEN_W || y < 0 || y >= SCREEN_H) return;
    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->display && g_gui_ctx->host->display->put_pixel) {
        g_gui_ctx->host->display->put_pixel(x, y, c);
    }
}

static inline void gui_fill_rect(int x, int y, int w, int h, uint16_t c) {
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > SCREEN_W) w = SCREEN_W - x;
    if (y + h > SCREEN_H) h = SCREEN_H - y;
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

static inline void gui_draw_button(int x, int y, int w, int h, const char *label, uint16_t bg, uint16_t fg, uint16_t border, bool pressed) {
    uint16_t fill = pressed ? COLOR_BTN_PR : bg;
    gui_fill_rect(x, y, w, h, fill);
    gui_draw_rect(x, y, w, h, border);

    int len = 0;
    while (label[len]) len++;
    int tx = x + (w - len * 8) / 2;
    int ty = y + (h - 16) / 2;
    gui_draw_str(tx, ty, label, fg, fill, true);
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
    int cx = x + w - 46;
    int cy = y + 4;
    return (tx >= cx && tx <= cx + 44 && ty >= cy && ty <= cy + 32);
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
