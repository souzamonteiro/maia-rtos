#include "gui_kit.h"

#define ED_MAX_LINES 18
#define ED_LINE_LEN  80

static char g_ed_buf[ED_MAX_LINES][ED_LINE_LEN];
static int g_ed_line_count = 1;
static int g_ed_cur_line = 0;
static int g_ed_cur_col = 0;
static char g_ed_filepath[64] = "/sd/notes.txt";
static char g_ed_status[80] = "Pronto para edicao. Teclado fisico ativo.";
static uint16_t g_ed_status_col = COLOR_WHITE;

static void set_ed_status(const char *msg, uint16_t col) {
    int i = 0;
    while (msg[i] && i < 79) { g_ed_status[i] = msg[i]; i++; }
    g_ed_status[i] = '\0';
    g_ed_status_col = col;
}

static void ed_load_file(const char *path) {
    if (!g_gui_ctx || !g_gui_ctx->host || !g_gui_ctx->host->read_text_file) return;
    char tmp[ED_MAX_LINES * ED_LINE_LEN + 1];
    int rd = g_gui_ctx->host->read_text_file(path, tmp, sizeof(tmp));
    if (rd > 0) {
        g_ed_line_count = 0;
        int line_idx = 0;
        int col_idx = 0;
        for (int i = 0; i < rd && line_idx < ED_MAX_LINES; ++i) {
            char c = tmp[i];
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
        set_ed_status("[OK] Arquivo carregado.", COLOR_GREEN);
    } else {
        set_ed_status("[INFO] Arquivo novo iniciado.", COLOR_YELLOW);
    }
}

static void ed_save_file(const char *path) {
    if (!g_gui_ctx || !g_gui_ctx->host || !g_gui_ctx->host->write_text_file) return;
    char tmp[ED_MAX_LINES * ED_LINE_LEN + 1];
    int pos = 0;
    for (int l = 0; l < g_ed_line_count; ++l) {
        int c = 0;
        while (g_ed_buf[l][c] && pos < (int)sizeof(tmp) - 2) {
            tmp[pos++] = g_ed_buf[l][c++];
        }
        tmp[pos++] = '\n';
    }
    tmp[pos] = '\0';
    int wr = g_gui_ctx->host->write_text_file(path, tmp, (uint32_t)pos);
    if (wr >= 0) {
        set_ed_status("[SUCESSO] Arquivo salvo no SD Card!", COLOR_GREEN);
    } else {
        set_ed_status("[ERRO] Falha ao salvar arquivo no SD.", COLOR_RED);
    }
}

static void render_editor(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "Editor de Codigo & Texto - MaiaEdit", COLOR_CYAN);

    /* Toolbar / Path display */
    int ty = wy + 44;
    gui_fill_rect(wx + 16, ty, ww - 32, 34, 0x10a2);
    gui_draw_rect(wx + 16, ty, ww - 32, 34, COLOR_WIN_BD);

    gui_draw_str(wx + 26, ty + 9, "Arquivo: ", COLOR_YELLOW, 0x10a2, true);
    gui_draw_str(wx + 98, ty + 9, g_ed_filepath, COLOR_WHITE, 0x10a2, true);

    /* Buttons on toolbar */
    int bw = 120, bh = 28;
    int b1_x = wx + ww - 270;
    int b2_x = wx + ww - 140;
    gui_draw_button(b1_x, ty + 3, bw, bh, "SALVAR", 0x1b24, COLOR_GREEN, COLOR_GREEN, false);
    gui_draw_button(b2_x, ty + 3, bw, bh, "LIMPAR", COLOR_BTN_BG, COLOR_WHITE, COLOR_BTN_BD, false);

    /* Text Canvas Area */
    int cy = ty + 42;
    int ch = wh - 134;
    int cw = ww - 32;
    gui_fill_rect(wx + 16, cy, cw, ch, 0x0821);
    gui_draw_rect(wx + 16, cy, cw, ch, COLOR_CYAN);

    /* Line Numbers Gutter */
    int gw = 40;
    gui_fill_rect(wx + 16, cy, gw, ch, 0x1082);
    gui_fill_rect(wx + 16 + gw, cy, 1, ch, COLOR_WIN_BD);

    for (int l = 0; l < g_ed_line_count && l < 18; ++l) {
        int ry = cy + 8 + l * 20;

        /* Line number */
        char num_str[8];
        num_str[0] = '0' + ((l + 1) / 10);
        num_str[1] = '0' + ((l + 1) % 10);
        num_str[2] = '\0';
        gui_draw_str(wx + 26, ry, num_str, COLOR_TEXT_DIM, 0x1082, true);

        /* Line content */
        gui_draw_str(wx + 16 + gw + 10, ry, g_ed_buf[l], COLOR_TEXT, 0x0821, true);

        /* Cursor */
        if (l == g_ed_cur_line) {
            int cx = wx + 16 + gw + 10 + g_ed_cur_col * 8;
            gui_fill_rect(cx, ry, 2, 16, COLOR_YELLOW);
        }
    }

    /* Footer status */
    int fy = wy + wh - 38;
    gui_fill_rect(wx + 16, fy, ww - 32, 28, 0x10a2);
    gui_draw_str(wx + 26, fy + 6, g_ed_status, g_ed_status_col, 0x10a2, true);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Text Editor GUI (/sd/apps/editor.mobj)...\n");

    if (ctx->argc > 1 && ctx->argv[1]) {
        int i = 0;
        while (ctx->argv[1][i] && i < 63) { g_ed_filepath[i] = ctx->argv[1][i]; i++; }
        g_ed_filepath[i] = '\0';
    }

    ed_load_file(g_ed_filepath);

    int wx = 200, wy = 60, ww = 880, wh = 580;
    render_editor(wx, wy, ww, wh);
    gui_sync();

    bool running = true;
    bool was_touched = false;

    while (running) {
        /* Keyboard */
        if (ctx->host->input && ctx->host->input->read_key) {
            int32_t k = ctx->host->input->read_key();
            while (k >= 0) {
                if (k == 27) { /* ESC */
                    running = false;
                    break;
                } else if (k == '\r' || k == '\n') {
                    if (g_ed_line_count < ED_MAX_LINES - 1) {
                        g_ed_line_count++;
                        g_ed_cur_line++;
                        g_ed_cur_col = 0;
                        render_editor(wx, wy, ww, wh);
                        gui_sync();
                    }
                } else if (k == 8 || k == 127) { /* Backspace */
                    if (g_ed_cur_col > 0) {
                        g_ed_cur_col--;
                        g_ed_buf[g_ed_cur_line][g_ed_cur_col] = '\0';
                        render_editor(wx, wy, ww, wh);
                        gui_sync();
                    } else if (g_ed_cur_line > 0) {
                        g_ed_cur_line--;
                        int len = 0;
                        while (g_ed_buf[g_ed_cur_line][len]) len++;
                        g_ed_cur_col = len;
                        render_editor(wx, wy, ww, wh);
                        gui_sync();
                    }
                } else if (k >= 32 && k <= 126) {
                    if (g_ed_cur_col < ED_LINE_LEN - 2) {
                        g_ed_buf[g_ed_cur_line][g_ed_cur_col] = (char)k;
                        g_ed_cur_col++;
                        g_ed_buf[g_ed_cur_line][g_ed_cur_col] = '\0';
                        render_editor(wx, wy, ww, wh);
                        gui_sync();
                    }
                }
                k = ctx->host->input->read_key();
            }
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
                    int bw = 120, bh = 28;
                    int b1_x = wx + ww - 270;
                    int b2_x = wx + ww - 140;

                    /* SALVAR */
                    if (tx >= b1_x && tx <= b1_x + bw && ty >= ty_tb + 3 && ty <= ty_tb + 3 + bh) {
                        ed_save_file(g_ed_filepath);
                        render_editor(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* LIMPAR */
                    else if (tx >= b2_x && tx <= b2_x + bw && ty >= ty_tb + 3 && ty <= ty_tb + 3 + bh) {
                        for (int l = 0; l < ED_MAX_LINES; ++l) g_ed_buf[l][0] = '\0';
                        g_ed_line_count = 1;
                        g_ed_cur_line = 0;
                        g_ed_cur_col = 0;
                        set_ed_status("[INFO] Buffer limpo.", COLOR_YELLOW);
                        render_editor(wx, wy, ww, wh);
                        gui_sync();
                    }
                } else if (!pressed) {
                    was_touched = false;
                }
            }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(30);
    }

    gui_out_str("<<< Editor GUI exited cleanly.\n");
    return 0;
}

