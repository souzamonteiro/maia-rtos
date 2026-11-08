#include "gui_kit.h"
#include <string.h>

#define TERM_MAX_LINES 18
#define TERM_LINE_LEN  80

static char g_term_lines[TERM_MAX_LINES][TERM_LINE_LEN];
static int g_term_line_count = 0;
static char g_term_input[72] = "";
static int g_term_input_len = 0;

/* Terminal History */
#define TERM_HIST_MAX 16
static char g_term_history[TERM_HIST_MAX][72];
static int  g_term_hist_count = 0;
static int  g_term_hist_browse = -1;

static void term_hist_add(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;
    if (g_term_hist_count > 0 && strcmp(g_term_history[g_term_hist_count - 1], cmd) == 0) {
        g_term_hist_browse = -1;
        return;
    }
    if (g_term_hist_count < TERM_HIST_MAX) {
        strncpy(g_term_history[g_term_hist_count++], cmd, sizeof(g_term_history[0]) - 1);
    } else {
        for (int i = 0; i < TERM_HIST_MAX - 1; ++i) {
            strncpy(g_term_history[i], g_term_history[i + 1], sizeof(g_term_history[0]) - 1);
        }
        strncpy(g_term_history[TERM_HIST_MAX - 1], cmd, sizeof(g_term_history[0]) - 1);
    }
    g_term_hist_browse = -1;
}

static void term_hist_up(void) {
    if (g_term_hist_count == 0) return;
    if (g_term_hist_browse == -1) {
        g_term_hist_browse = g_term_hist_count - 1;
    } else if (g_term_hist_browse > 0) {
        g_term_hist_browse--;
    }
    strncpy(g_term_input, g_term_history[g_term_hist_browse], sizeof(g_term_input) - 1);
    g_term_input[sizeof(g_term_input) - 1] = '\0';
    g_term_input_len = (int)strlen(g_term_input);
}

static void term_hist_down(void) {
    if (g_term_hist_browse == -1) return;
    if (g_term_hist_browse + 1 < g_term_hist_count) {
        g_term_hist_browse++;
        strncpy(g_term_input, g_term_history[g_term_hist_browse], sizeof(g_term_input) - 1);
        g_term_input[sizeof(g_term_input) - 1] = '\0';
        g_term_input_len = (int)strlen(g_term_input);
    } else {
        g_term_hist_browse = -1;
        g_term_input[0] = '\0';
        g_term_input_len = 0;
    }
}

static void term_add_line(const char *str) {
    if (g_term_line_count < TERM_MAX_LINES) {
        int i = 0;
        while (str[i] && i < TERM_LINE_LEN - 1) {
            g_term_lines[g_term_line_count][i] = str[i];
            i++;
        }
        g_term_lines[g_term_line_count][i] = '\0';
        g_term_line_count++;
    } else {
        /* Scroll up */
        for (int l = 0; l < TERM_MAX_LINES - 1; ++l) {
            for (int c = 0; c < TERM_LINE_LEN; ++c) {
                g_term_lines[l][c] = g_term_lines[l + 1][c];
            }
        }
        int i = 0;
        while (str[i] && i < TERM_LINE_LEN - 1) {
            g_term_lines[TERM_MAX_LINES - 1][i] = str[i];
            i++;
        }
        g_term_lines[TERM_MAX_LINES - 1][i] = '\0';
    }
}

static char g_term_out_line[TERM_LINE_LEN] = {0};
static int  g_term_out_pos = 0;

static void term_out_cb(const char *str, void *user_data) {
    (void)user_data;
    if (!str) return;

    while (*str) {
        char c = *str++;
        if (c == '\r') continue;
        if (c == '\n') {
            g_term_out_line[g_term_out_pos] = '\0';
            term_add_line(g_term_out_line);
            g_term_out_pos = 0;
            g_term_out_line[0] = '\0';
        } else {
            if (g_term_out_pos < TERM_LINE_LEN - 1) {
                g_term_out_line[g_term_out_pos++] = c;
            } else {
                g_term_out_line[g_term_out_pos] = '\0';
                term_add_line(g_term_out_line);
                g_term_out_pos = 0;
                g_term_out_line[g_term_out_pos++] = c;
            }
        }
    }
}

static void render_term(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "Interactive MaiaShell Terminal (msh)", COLOR_GREEN);

    /* Terminal Black Screen Canvas */
    int cx = wx + 16, cy = wy + 44, cw = ww - 32, ch = wh - 108;
    gui_fill_rect(cx, cy, cw, ch, COLOR_BLACK);
    gui_draw_rect(cx, cy, cw, ch, COLOR_GREEN);

    int line_h = 28;
    int max_vis = (ch - 16) / line_h;
    int start_l = 0;
    if (g_term_line_count > max_vis) {
        start_l = g_term_line_count - max_vis;
    }

    int ty = cy + 8;
    for (int l = start_l; l < g_term_line_count; ++l) {
        gui_draw_str_2x(cx + 10, ty, g_term_lines[l], COLOR_GREEN, COLOR_BLACK, true);
        ty += line_h;
    }

    /* Command prompt input box */
    int iy = wy + wh - 52;
    gui_fill_rect(cx, iy, cw, 42, 0x10a2);
    gui_draw_rect(cx, iy, cw, 42, COLOR_CYAN);

    gui_draw_str_2x(cx + 10, iy + 6, "msh> ", COLOR_YELLOW, 0x10a2, true);
    gui_draw_str_2x(cx + 90, iy + 6, g_term_input, COLOR_WHITE, 0x10a2, true);

    /* Blinking cursor */
    int cur_x = cx + 90 + g_term_input_len * 16;
    gui_fill_rect(cur_x, iy + 6, 12, 28, COLOR_GREEN);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Terminal GUI (/sd/apps/term.mobj)...\n");

    term_add_line("MaiaRTOS MaiaShell Graphical Terminal");
    term_add_line("Type 'help' for available commands, 'ls', 'mem', 'ps'.");
    term_add_line("Press [ESC] to exit back to Desktop.");

    int wx = 220, wy = 60, ww = 840, wh = 580;
    render_term(wx, wy, ww, wh);
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
                    if (g_term_input_len > 0) {
                        term_hist_add(g_term_input);
                        char echo_line[96];
                        int ep = 0;
                        const char pfx[] = "msh> ";
                        while (pfx[ep]) { echo_line[ep] = pfx[ep]; ep++; }
                        for (int i = 0; i < g_term_input_len; ++i) echo_line[ep++] = g_term_input[i];
                        echo_line[ep] = '\0';
                        term_add_line(echo_line);

                        g_term_out_pos = 0;
                        g_term_out_line[0] = '\0';
                        if (ctx->host->shell_exec) {
                            ctx->host->shell_exec(g_term_input, term_out_cb, NULL);
                            if (g_term_out_pos > 0) {
                                g_term_out_line[g_term_out_pos] = '\0';
                                term_add_line(g_term_out_line);
                                g_term_out_pos = 0;
                                g_term_out_line[0] = '\0';
                            }
                        }
                        g_term_input[0] = '\0';
                        g_term_input_len = 0;
                    }
                    render_term(wx, wy, ww, wh);
                    gui_sync();
                } else if (k == 0x80) { /* Up Arrow */
                    term_hist_up();
                    render_term(wx, wy, ww, wh);
                    gui_sync();
                } else if (k == 0x81) { /* Down Arrow */
                    term_hist_down();
                    render_term(wx, wy, ww, wh);
                    gui_sync();
                } else if (k == 8 || k == 127) { /* Backspace */
                    if (g_term_input_len > 0) {
                        g_term_input_len--;
                        g_term_input[g_term_input_len] = '\0';
                        render_term(wx, wy, ww, wh);
                        gui_sync();
                    }
                } else if (k >= 32 && k <= 126) {
                    if (g_term_input_len < (int)sizeof(g_term_input) - 2) {
                        g_term_input[g_term_input_len++] = (char)k;
                        g_term_input[g_term_input_len] = '\0';
                        render_term(wx, wy, ww, wh);
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
                } else if (!pressed) {
                    was_touched = false;
                }
            }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(30);
    }

    gui_out_str("<<< Terminal GUI exited cleanly.\n");
    return 0;
}

