#include "gui_kit.h"

#define ED_MAX_LINES       512
#define ED_LINE_LEN        128
#define ED_VISIBLE_LINES    16
#define ED_LOAD_BUF_SIZE  65536

static char (*g_ed_buf)[ED_LINE_LEN] = NULL;
static int g_ed_line_count = 1;
static int g_ed_cur_line = 0;
static int g_ed_cur_col = 0;
static int g_ed_scroll_top = 0;
static char g_ed_filepath[96] = "/sd/notes.txt";
static char g_ed_status[96] = "Ready for editing. Physical keyboard active.";
static uint16_t g_ed_status_col = COLOR_WHITE;

static inline int ed_strlen(const char *s) {
    int l = 0;
    while (s && s[l]) l++;
    return l;
}

static inline void ed_strcpy(char *dst, const char *src, int max_len) {
    int i = 0;
    if (!dst || max_len <= 0) return;
    if (src) {
        while (src[i] && i < max_len - 1) {
            dst[i] = src[i];
            i++;
        }
    }
    dst[i] = '\0';
}

static void set_ed_status(const char *msg, uint16_t col) {
    ed_strcpy(g_ed_status, msg, sizeof(g_ed_status));
    g_ed_status_col = col;
}

static void ed_load_file(const char *path) {
    if (!g_gui_ctx || !g_gui_ctx->host || !g_gui_ctx->host->read_text_file || !g_ed_buf) return;
    char *load_buf = NULL;
    if (g_gui_ctx->host->alloc) {
        load_buf = (char *)g_gui_ctx->host->alloc(ED_LOAD_BUF_SIZE, 1);
    }
    if (!load_buf) return;

    int rd = g_gui_ctx->host->read_text_file(path, load_buf, ED_LOAD_BUF_SIZE - 1);
    if (rd > 0) {
        load_buf[rd] = '\0';
        for (int l = 0; l < ED_MAX_LINES; ++l) g_ed_buf[l][0] = '\0';
        g_ed_line_count = 0;
        int line_idx = 0;
        int col_idx = 0;
        for (int i = 0; i < rd && line_idx < ED_MAX_LINES; ++i) {
            char c = load_buf[i];
            if (c == '\r') continue;
            if (c == '\n') {
                g_ed_buf[line_idx][col_idx] = '\0';
                line_idx++;
                col_idx = 0;
            } else if (col_idx < ED_LINE_LEN - 1) {
                g_ed_buf[line_idx][col_idx++] = c;
            }
        }
        if (col_idx > 0 && line_idx < ED_MAX_LINES) {
            g_ed_buf[line_idx][col_idx] = '\0';
            line_idx++;
        }
        g_ed_line_count = (line_idx > 0) ? line_idx : 1;
        g_ed_cur_line = 0;
        g_ed_cur_col = 0;
        g_ed_scroll_top = 0;
        set_ed_status("[OK] File loaded successfully.", COLOR_GREEN);
    } else {
        g_ed_line_count = 1;
        g_ed_buf[0][0] = '\0';
        g_ed_cur_line = 0;
        g_ed_cur_col = 0;
        g_ed_scroll_top = 0;
        set_ed_status("[INFO] New empty file initialized.", COLOR_YELLOW);
    }

    if (g_gui_ctx->host->dealloc) {
        g_gui_ctx->host->dealloc(load_buf);
    }
}

static void ed_save_file(const char *path) {
    if (!g_gui_ctx || !g_gui_ctx->host || !g_gui_ctx->host->write_text_file || !g_ed_buf) return;
    char *save_buf = NULL;
    if (g_gui_ctx->host->alloc) {
        save_buf = (char *)g_gui_ctx->host->alloc(ED_LOAD_BUF_SIZE, 1);
    }
    if (!save_buf) {
        set_ed_status("[ERROR] Out of memory.", COLOR_RED);
        return;
    }

    int pos = 0;
    for (int l = 0; l < g_ed_line_count; ++l) {
        int c = 0;
        while (g_ed_buf[l][c] && pos < ED_LOAD_BUF_SIZE - 2) {
            save_buf[pos++] = g_ed_buf[l][c++];
        }
        save_buf[pos++] = '\n';
    }
    save_buf[pos] = '\0';
    int wr = g_gui_ctx->host->write_text_file(path, save_buf, (uint32_t)pos);
    if (wr >= 0) {
        set_ed_status("[SUCCESS] File saved to SD Card!", COLOR_GREEN);
    } else {
        set_ed_status("[ERROR] Failed to save file to SD.", COLOR_RED);
    }

    if (g_gui_ctx->host->dealloc) {
        g_gui_ctx->host->dealloc(save_buf);
    }
}

static void render_editor_status(int wx, int wy, int ww, int wh) {
    int fy = wy + wh - 38;
    gui_set_clip(wx + 16, fy, ww - 32, 28);
    gui_fill_rect(wx + 16, fy, ww - 32, 28, 0x10a2);

    char full_status[128];
    char num_buf[32];
    int p = 0;

    /* "Ln X/Y, Col Z | " */
    full_status[p++] = 'L'; full_status[p++] = 'n'; full_status[p++] = ' ';
    int cl = g_ed_cur_line + 1;
    int np = 0;
    if (cl == 0) num_buf[np++] = '0';
    else { int tc = cl; while (tc > 0) { num_buf[np++] = '0' + (tc % 10); tc /= 10; } }
    while (np > 0) full_status[p++] = num_buf[--np];

    full_status[p++] = '/';
    int tl = g_ed_line_count;
    if (tl == 0) num_buf[np++] = '0';
    else { int tc = tl; while (tc > 0) { num_buf[np++] = '0' + (tc % 10); tc /= 10; } }
    while (np > 0) full_status[p++] = num_buf[--np];

    full_status[p++] = ','; full_status[p++] = ' '; full_status[p++] = 'C'; full_status[p++] = 'o'; full_status[p++] = 'l'; full_status[p++] = ' ';
    int cc = g_ed_cur_col;
    if (cc == 0) num_buf[np++] = '0';
    else { int tc = cc; while (tc > 0) { num_buf[np++] = '0' + (tc % 10); tc /= 10; } }
    while (np > 0) full_status[p++] = num_buf[--np];

    full_status[p++] = ' '; full_status[p++] = '|'; full_status[p++] = ' ';

    int si = 0;
    while (g_ed_status[si] && p < (int)sizeof(full_status) - 1) {
        full_status[p++] = g_ed_status[si++];
    }
    full_status[p] = '\0';

    gui_draw_str_2x(wx + 22, fy + 3, full_status, g_ed_status_col, 0x10a2, true);
    gui_reset_clip();
}

/* Partial dirty redraw of ONLY the editor text area (Zero Flicker!) */
static void render_editor_text(int wx, int wy, int ww, int wh) {
    int ty = wy + 44;
    int cy = ty + 42;
    int ch = wh - 134;
    int cw = ww - 32;
    int gw = 56; // Gutter width for line numbers

    gui_set_clip(wx + 16, cy, cw, ch);
    gui_fill_rect(wx + 16, cy, cw, ch, 0x0821);
    gui_draw_rect(wx + 16, cy, cw, ch, COLOR_CYAN);

    /* Line Numbers Gutter */
    gui_fill_rect(wx + 16, cy, gw, ch, 0x1082);
    gui_fill_rect(wx + 16 + gw, cy, 1, ch, COLOR_WIN_BD);

    for (int vi = 0; vi < ED_VISIBLE_LINES; ++vi) {
        int l = g_ed_scroll_top + vi;
        if (l >= g_ed_line_count) break;

        int ry = cy + 6 + vi * 28;
        if (ry + 24 > cy + ch) break;

        /* Line number in 2x (3 digits support) */
        char num_str[6];
        int num = l + 1;
        num_str[0] = '0' + ((num / 100) % 10);
        num_str[1] = '0' + ((num / 10) % 10);
        num_str[2] = '0' + (num % 10);
        num_str[3] = '\0';
        gui_draw_str_2x(wx + 20, ry, num_str, (l == g_ed_cur_line) ? COLOR_CYAN : COLOR_GRAY, 0x1082, true);

        /* Line content in 2x */
        gui_set_clip(wx + 16 + gw + 4, cy + 2, cw - gw - 8, ch - 4);
        gui_draw_str_2x(wx + 16 + gw + 10, ry, g_ed_buf[l], COLOR_TEXT, 0x0821, true);

        /* Cursor */
        if (l == g_ed_cur_line) {
            int cx_pos = wx + 16 + gw + 10 + g_ed_cur_col * 16;
            gui_fill_rect(cx_pos, ry, 3, 24, COLOR_YELLOW);
        }
        gui_set_clip(wx + 16, cy, cw, ch);
    }
    gui_reset_clip();
}

/* Full chrome render (drawn once on open) */
static void render_editor_chrome(int wx, int wy, int ww, int wh) {
    gui_set_clip(wx, wy, ww, wh);
    gui_draw_window_frame(wx, wy, ww, wh, "MaiaEdit Text & Code Editor", COLOR_CYAN);

    /* Toolbar / Path display */
    int ty = wy + 44;
    gui_fill_rect(wx + 16, ty, ww - 32, 34, 0x10a2);
    gui_draw_rect(wx + 16, ty, ww - 32, 34, COLOR_WIN_BD);

    gui_draw_str_2x(wx + 26, ty + 5, "File: ", COLOR_YELLOW, 0x10a2, true);
    gui_draw_str_2x(wx + 110, ty + 5, g_ed_filepath, COLOR_WHITE, 0x10a2, true);

    /* Buttons on toolbar (2x typography) */
    int bw = 90, bh = 28;
    int b1_x = wx + ww - 390;
    int b2_x = wx + ww - 290;
    int b3_x = wx + ww - 190;
    int b4_x = wx + ww - 95;

    gui_draw_button(b1_x, ty + 3, bw, bh, "SAVE",  0x1b24, COLOR_GREEN, COLOR_GREEN, false);
    gui_draw_button(b2_x, ty + 3, bw, bh, "CLEAR", COLOR_BTN_BG, COLOR_WHITE, COLOR_BTN_BD, false);
    gui_draw_button(b3_x, ty + 3, bw, bh, "UP",    COLOR_BTN_BG, COLOR_CYAN, COLOR_BTN_BD, false);
    gui_draw_button(b4_x, ty + 3, 76, bh, "DOWN",  COLOR_BTN_BG, COLOR_CYAN, COLOR_BTN_BD, false);

    gui_reset_clip();

    render_editor_text(wx, wy, ww, wh);
    render_editor_status(wx, wy, ww, wh);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    if (ctx->host->alloc) {
        g_ed_buf = (char (*)[ED_LINE_LEN])ctx->host->alloc(ED_MAX_LINES * ED_LINE_LEN, 1);
    }
    if (!g_ed_buf) return 1;

    for (int l = 0; l < ED_MAX_LINES; ++l) {
        g_ed_buf[l][0] = '\0';
    }
    g_ed_line_count = 1;
    g_ed_cur_line = 0;
    g_ed_cur_col = 0;
    g_ed_scroll_top = 0;

    if (ctx->argc > 1 && ctx->argv[1]) {
        ed_strcpy(g_ed_filepath, ctx->argv[1], sizeof(g_ed_filepath));
    }

    ed_load_file(g_ed_filepath);

    /* Drain lingering touch or keys from previous launcher window */
    while (gui_poll_key() > 0) {}
    int32_t itx = 0, ity = 0;
    bool ipr = false;
    while (gui_poll_touch(&itx, &ity, &ipr) && ipr) {
        gui_delay_ms(10);
    }

    int wx = 120, wy = 45, ww = 1040, wh = 630;

    /* Render Chrome ONCE */
    render_editor_chrome(wx, wy, ww, wh);
    gui_sync();

    bool running = true;
    bool was_touched = false;

    while (running) {
        /* Keyboard */
        int32_t k = gui_poll_key();
        while (k >= 0) {
            if (k == 27) { /* ESC */
                running = false;
                break;
            }
            /* Arrow UP */
            else if (k == 0x80) {
                if (g_ed_cur_line > 0) {
                    g_ed_cur_line--;
                    int len = ed_strlen(g_ed_buf[g_ed_cur_line]);
                    if (g_ed_cur_col > len) g_ed_cur_col = len;
                    if (g_ed_cur_line < g_ed_scroll_top) g_ed_scroll_top = g_ed_cur_line;
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                }
            }
            /* Arrow DOWN */
            else if (k == 0x81) {
                if (g_ed_cur_line + 1 < g_ed_line_count) {
                    g_ed_cur_line++;
                    int len = ed_strlen(g_ed_buf[g_ed_cur_line]);
                    if (g_ed_cur_col > len) g_ed_cur_col = len;
                    if (g_ed_cur_line >= g_ed_scroll_top + ED_VISIBLE_LINES) {
                        g_ed_scroll_top = g_ed_cur_line - ED_VISIBLE_LINES + 1;
                    }
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                }
            }
            /* Arrow LEFT */
            else if (k == 0x82) {
                if (g_ed_cur_col > 0) {
                    g_ed_cur_col--;
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                } else if (g_ed_cur_line > 0) {
                    g_ed_cur_line--;
                    g_ed_cur_col = ed_strlen(g_ed_buf[g_ed_cur_line]);
                    if (g_ed_cur_line < g_ed_scroll_top) g_ed_scroll_top = g_ed_cur_line;
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                }
            }
            /* Arrow RIGHT */
            else if (k == 0x83) {
                int len = ed_strlen(g_ed_buf[g_ed_cur_line]);
                if (g_ed_cur_col < len) {
                    g_ed_cur_col++;
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                } else if (g_ed_cur_line + 1 < g_ed_line_count) {
                    g_ed_cur_line++;
                    g_ed_cur_col = 0;
                    if (g_ed_cur_line >= g_ed_scroll_top + ED_VISIBLE_LINES) {
                        g_ed_scroll_top = g_ed_cur_line - ED_VISIBLE_LINES + 1;
                    }
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                }
            }
            /* Enter: split line */
            else if (k == '\r' || k == '\n') {
                if (g_ed_line_count < ED_MAX_LINES - 1) {
                    for (int l = g_ed_line_count; l > g_ed_cur_line + 1; --l) {
                        ed_strcpy(g_ed_buf[l], g_ed_buf[l - 1], ED_LINE_LEN);
                    }
                    int new_col = 0;
                    for (int p = g_ed_cur_col; g_ed_buf[g_ed_cur_line][p]; ++p) {
                        g_ed_buf[g_ed_cur_line + 1][new_col++] = g_ed_buf[g_ed_cur_line][p];
                    }
                    g_ed_buf[g_ed_cur_line + 1][new_col] = '\0';
                    g_ed_buf[g_ed_cur_line][g_ed_cur_col] = '\0';
                    g_ed_line_count++;
                    g_ed_cur_line++;
                    g_ed_cur_col = 0;
                    if (g_ed_cur_line >= g_ed_scroll_top + ED_VISIBLE_LINES) {
                        g_ed_scroll_top = g_ed_cur_line - ED_VISIBLE_LINES + 1;
                    }
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                }
            }
            /* Tab: insert 4 spaces */
            else if (k == '\t') {
                for (int s = 0; s < 4; ++s) {
                    int len = ed_strlen(g_ed_buf[g_ed_cur_line]);
                    if (len < ED_LINE_LEN - 2) {
                        for (int p = len; p >= g_ed_cur_col; --p) {
                            g_ed_buf[g_ed_cur_line][p + 1] = g_ed_buf[g_ed_cur_line][p];
                        }
                        g_ed_buf[g_ed_cur_line][g_ed_cur_col++] = ' ';
                    }
                }
                render_editor_text(wx, wy, ww, wh);
                render_editor_status(wx, wy, ww, wh);
                gui_sync();
            }
            /* Backspace */
            else if (k == 8 || k == 127) {
                if (g_ed_cur_col > 0) {
                    int len = ed_strlen(g_ed_buf[g_ed_cur_line]);
                    for (int p = g_ed_cur_col - 1; p < len; ++p) {
                        g_ed_buf[g_ed_cur_line][p] = g_ed_buf[g_ed_cur_line][p + 1];
                    }
                    g_ed_cur_col--;
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                } else if (g_ed_cur_line > 0) {
                    int prev_len = ed_strlen(g_ed_buf[g_ed_cur_line - 1]);
                    int cur_len = ed_strlen(g_ed_buf[g_ed_cur_line]);
                    if (prev_len + cur_len < ED_LINE_LEN - 1) {
                        for (int p = 0; p <= cur_len; ++p) {
                            g_ed_buf[g_ed_cur_line - 1][prev_len + p] = g_ed_buf[g_ed_cur_line][p];
                        }
                        for (int l = g_ed_cur_line; l < g_ed_line_count - 1; ++l) {
                            ed_strcpy(g_ed_buf[l], g_ed_buf[l + 1], ED_LINE_LEN);
                        }
                        g_ed_buf[g_ed_line_count - 1][0] = '\0';
                        g_ed_line_count--;
                        g_ed_cur_line--;
                        g_ed_cur_col = prev_len;
                        if (g_ed_cur_line < g_ed_scroll_top) g_ed_scroll_top = g_ed_cur_line;
                        render_editor_text(wx, wy, ww, wh);
                        render_editor_status(wx, wy, ww, wh);
                        gui_sync();
                    }
                }
            }
            /* Printable Characters: Insert */
            else if (k >= 32 && k <= 126) {
                int len = ed_strlen(g_ed_buf[g_ed_cur_line]);
                if (len < ED_LINE_LEN - 2) {
                    for (int p = len; p >= g_ed_cur_col; --p) {
                        g_ed_buf[g_ed_cur_line][p + 1] = g_ed_buf[g_ed_cur_line][p];
                    }
                    g_ed_buf[g_ed_cur_line][g_ed_cur_col++] = (char)k;
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                }
            }
            k = gui_poll_key();
        }

        /* Touch */
        int32_t tx = 0, ty = 0;
        bool pressed = false;
        if (gui_poll_touch(&tx, &ty, &pressed)) {
            if (pressed && !was_touched) {
                was_touched = true;
                if (gui_is_close_clicked(wx, wy, ww, tx, ty)) {
                    running = false;
                    break;
                }

                int ty_tb = wy + 44;
                int bw = 90, bh = 28;
                int b1_x = wx + ww - 390;
                int b2_x = wx + ww - 290;
                int b3_x = wx + ww - 190;
                int b4_x = wx + ww - 95;

                /* SALVAR */
                if (tx >= b1_x && tx <= b1_x + bw && ty >= ty_tb + 3 && ty <= ty_tb + 3 + bh) {
                    ed_save_file(g_ed_filepath);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                }
                /* LIMPAR */
                else if (tx >= b2_x && tx <= b2_x + bw && ty >= ty_tb + 3 && ty <= ty_tb + 3 + bh) {
                    for (int l = 0; l < ED_MAX_LINES; ++l) g_ed_buf[l][0] = '\0';
                    g_ed_line_count = 1;
                    g_ed_cur_line = 0;
                    g_ed_cur_col = 0;
                    g_ed_scroll_top = 0;
                    set_ed_status("[INFO] Buffer cleared.", COLOR_YELLOW);
                    render_editor_text(wx, wy, ww, wh);
                    render_editor_status(wx, wy, ww, wh);
                    gui_sync();
                }
                /* SUBIR */
                else if (tx >= b3_x && tx <= b3_x + bw && ty >= ty_tb + 3 && ty <= ty_tb + 3 + bh) {
                    if (g_ed_scroll_top > 0) {
                        g_ed_scroll_top -= 5;
                        if (g_ed_scroll_top < 0) g_ed_scroll_top = 0;
                        if (g_ed_cur_line < g_ed_scroll_top) g_ed_cur_line = g_ed_scroll_top;
                        render_editor_text(wx, wy, ww, wh);
                        render_editor_status(wx, wy, ww, wh);
                        gui_sync();
                    }
                }
                /* DESCER */
                else if (tx >= b4_x && tx <= b4_x + 76 && ty >= ty_tb + 3 && ty <= ty_tb + 3 + bh) {
                    if (g_ed_scroll_top + ED_VISIBLE_LINES < g_ed_line_count) {
                        g_ed_scroll_top += 5;
                        if (g_ed_scroll_top > g_ed_line_count - 1) g_ed_scroll_top = g_ed_line_count - 1;
                        if (g_ed_cur_line < g_ed_scroll_top) g_ed_cur_line = g_ed_scroll_top;
                        render_editor_text(wx, wy, ww, wh);
                        render_editor_status(wx, wy, ww, wh);
                        gui_sync();
                    }
                }
                /* Touch in Text Area: Place Cursor */
                else {
                    int text_cy = ty_tb + 42;
                    int text_ch = wh - 134;
                    int text_gw = 56;
                    if (ty >= text_cy + 6 && ty < text_cy + text_ch && tx >= wx + 16 + text_gw) {
                        int tapped_vi = (ty - (text_cy + 6)) / 28;
                        int tapped_line = g_ed_scroll_top + tapped_vi;
                        if (tapped_line < g_ed_line_count) {
                            g_ed_cur_line = tapped_line;
                            int tapped_col = (tx - (wx + 16 + text_gw + 10)) / 16;
                            if (tapped_col < 0) tapped_col = 0;
                            int cur_len = ed_strlen(g_ed_buf[g_ed_cur_line]);
                            if (tapped_col > cur_len) tapped_col = cur_len;
                            g_ed_cur_col = tapped_col;
                            render_editor_text(wx, wy, ww, wh);
                            render_editor_status(wx, wy, ww, wh);
                            gui_sync();
                        }
                    }
                }
            } else if (!pressed) {
                was_touched = false;
            }
        }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(25);
    }

    if (g_ed_buf && ctx->host->dealloc) {
        ctx->host->dealloc(g_ed_buf);
        g_ed_buf = NULL;
    }

    return 0;
}
