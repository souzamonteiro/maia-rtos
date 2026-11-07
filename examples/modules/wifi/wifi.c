#include "gui_kit.h"

static char g_wifi_ssid[40] = "";
static char g_wifi_pass[40] = "";
static int g_wifi_active_field = 1; /* 1 = SSID, 2 = Password */
static char g_wifi_status_msg[96] = "Disconnected. Press CONNECT or ESC.";
static uint16_t g_wifi_status_col = COLOR_WHITE;

static int str_len(const char *s) {
    int l = 0;
    while (s && s[l]) l++;
    return l;
}

static void set_wifi_msg(const char *msg, uint16_t col) {
    int i = 0;
    while (msg[i] && i < 95) { g_wifi_status_msg[i] = msg[i]; i++; }
    g_wifi_status_msg[i] = '\0';
    g_wifi_status_col = col;
}

static void render_wifi_chrome(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "Network & Wi-Fi Configuration", COLOR_CYAN);

    int ly = wy + 44;
    gui_draw_str_2x(wx + 24, ly, "WIRELESS NETWORK CONNECTION (Wi-Fi)", COLOR_YELLOW, COLOR_WIN_BG, true);
    ly += 38;

    gui_draw_str_2x(wx + 24, ly + 4, "SSID :", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += 46;

    gui_draw_str_2x(wx + 24, ly + 4, "Password:", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += 54;

    /* Current Network Status Info Box outline (84px height for 1.5x status typography) */
    gui_fill_rect(wx + 24, ly, ww - 48, 84, 0x10a2);
    gui_draw_rect(wx + 24, ly, ww - 48, 84, COLOR_CYAN);
    gui_draw_str_2x(wx + 36, ly + 6, "Current Status:", COLOR_CYAN, 0x10a2, true);
    ly += 96;

    /* Instruction text in intermediate 1.5x font */
    gui_draw_str_1_5x(wx + 24, ly, "Press [TAB] to switch fields, [ENTER] to connect.", COLOR_TEXT_DIM, COLOR_WIN_BG, true);
    ly += 34;

    /* Action Buttons */
    int bw = (ww - 72) / 3;
    int bh = 44;
    int b1 = wx + 24, b2 = b1 + bw + 12, b3 = b2 + bw + 12;

    gui_draw_button(b1, ly, bw, bh, "CONNECT",    0x1b24, COLOR_GREEN, COLOR_GREEN, false);
    gui_draw_button(b2, ly, bw, bh, "DISCONNECT", 0x8180, COLOR_RED,   COLOR_RED,   false);
    gui_draw_button(b3, ly, bw, bh, "REFRESH IP", 0x03a8, COLOR_CYAN,  COLOR_CYAN,  false);
}

static void render_wifi_fields(int wx, int wy, int ww) {
    int ly = wy + 44 + 38;
    int fx = wx + 160, fw = ww - 190, fh = 36;

    /* Field 1: SSID */
    uint16_t f1_bd = (g_wifi_active_field == 1) ? COLOR_YELLOW : COLOR_WIN_BD;
    gui_fill_rect(fx, ly, fw, fh, 0x10a2);
    gui_draw_rect(fx, ly, fw, fh, f1_bd);
    gui_draw_str_2x(fx + 10, ly + 2, g_wifi_ssid, COLOR_WHITE, 0x10a2, true);
    if (g_wifi_active_field == 1) {
        int sl = 0; while (g_wifi_ssid[sl]) sl++;
        gui_fill_rect(fx + 10 + sl * 16, ly + 4, 3, 26, COLOR_YELLOW);
    }
    ly += 46;

    /* Field 2: Password */
    uint16_t f2_bd = (g_wifi_active_field == 2) ? COLOR_YELLOW : COLOR_WIN_BD;
    gui_fill_rect(fx, ly, fw, fh, 0x10a2);
    gui_draw_rect(fx, ly, fw, fh, f2_bd);
    char pass_stars[40];
    int pl = 0;
    while (g_wifi_pass[pl] && pl < 39) { pass_stars[pl] = '*'; pl++; }
    pass_stars[pl] = '\0';
    gui_draw_str_2x(fx + 10, ly + 2, pass_stars, COLOR_WHITE, 0x10a2, true);
    if (g_wifi_active_field == 2) {
        gui_fill_rect(fx + 10 + pl * 16, ly + 4, 3, 26, COLOR_YELLOW);
    }
}

static void render_wifi_status(int wx, int wy, int ww) {
    int ly = wy + 44 + 38 + 46 + 54;
    int box_x = wx + 24;
    int box_w = ww - 48;
    int text_x = wx + 36;
    int text_w = box_w - 24;

    /* Limpa a area de texto dentro da caixa */
    gui_fill_rect(box_x + 2, ly + 32, box_w - 4, 50, 0x10a2);

    /* Aplica clipping estrito */
    gui_set_clip(text_x, ly + 32, text_w, 50);

    int msg_len = str_len(g_wifi_status_msg);
    if (msg_len <= 52) {
        gui_draw_str_1_5x(text_x, ly + 42, g_wifi_status_msg, g_wifi_status_col, 0x10a2, true);
    } else {
        /* Quebra inteligente em duas linhas na escala 1.5x (12x24) */
        char line1[64] = {0};
        char line2[64] = {0};

        int split = 50;
        if (msg_len < split) split = msg_len;
        else {
            int s = split;
            while (s > 25 && g_wifi_status_msg[s] != ' ' && g_wifi_status_msg[s] != ',' && g_wifi_status_msg[s] != '|' && g_wifi_status_msg[s] != '(') s--;
            if (s > 25) split = s + 1;
        }

        int l1 = 0;
        for (; l1 < split && l1 < (int)sizeof(line1) - 1; ++l1) {
            line1[l1] = g_wifi_status_msg[l1];
        }
        line1[l1] = '\0';

        int l2 = 0;
        for (int p = split; g_wifi_status_msg[p] && l2 < (int)sizeof(line2) - 1; ++p) {
            line2[l2++] = g_wifi_status_msg[p];
        }
        line2[l2] = '\0';

        gui_draw_str_1_5x(text_x, ly + 34, line1, g_wifi_status_col, 0x10a2, true);
        gui_draw_str_1_5x(text_x, ly + 58, line2, g_wifi_status_col, 0x10a2, true);
    }

    gui_reset_clip();
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Wi-Fi GUI (/sd/apps/wifi.mobj)...\n");

    int wx = 280, wy = 80, ww = 720, wh = 400;

    /* Query initial status */
    char st_buf[96] = {0};
    if (ctx->host->wifi_status && ctx->host->wifi_status(st_buf, sizeof(st_buf)) == 0 && st_buf[0]) {
        set_wifi_msg(st_buf, COLOR_GREEN);
    }

    render_wifi_chrome(wx, wy, ww, wh);
    render_wifi_fields(wx, wy, ww);
    render_wifi_status(wx, wy, ww);
    gui_sync();

    bool running = true;
    bool was_touched = false;

    while (running) {
        /* Keyboard Input */
        if (ctx->host->input && ctx->host->input->read_key) {
            int32_t k = ctx->host->input->read_key();
            while (k >= 0) {
                if (k == 27) { /* ESC */
                    running = false;
                    break;
                } else if (k == '\t') {
                    g_wifi_active_field = (g_wifi_active_field == 1) ? 2 : 1;
                    render_wifi_fields(wx, wy, ww);
                    gui_sync();
                } else if (k == '\r' || k == '\n') {
                    if (ctx->host->wifi_connect) {
                        set_wifi_msg("Connecting to AP...", COLOR_YELLOW);
                        render_wifi_status(wx, wy, ww);
                        gui_sync();
                        int ret = ctx->host->wifi_connect(g_wifi_ssid, g_wifi_pass);
                        if (ret == 0) set_wifi_msg("[OK] Connected! IP acquired successfully.", COLOR_GREEN);
                        else set_wifi_msg("[ERROR] Authentication failed with AP.", COLOR_RED);
                        render_wifi_status(wx, wy, ww);
                        gui_sync();
                    }
                } else if (k == 8 || k == 127) { /* Backspace */
                    char *tgt = (g_wifi_active_field == 1) ? g_wifi_ssid : g_wifi_pass;
                    int l = 0; while (tgt[l]) l++;
                    if (l > 0) tgt[l - 1] = '\0';
                    render_wifi_fields(wx, wy, ww);
                    gui_sync();
                } else if (k >= 32 && k <= 126) {
                    char *tgt = (g_wifi_active_field == 1) ? g_wifi_ssid : g_wifi_pass;
                    int l = 0; while (tgt[l]) l++;
                    if (l < 38) {
                        tgt[l] = (char)k;
                        tgt[l + 1] = '\0';
                        render_wifi_fields(wx, wy, ww);
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

                    int fx = wx + 160, fw = ww - 190, fh = 36;
                    int f1_y = wy + 44 + 38;
                    int f2_y = f1_y + 46;

                    /* Touch on SSID field */
                    if (tx >= fx && tx <= fx + fw && ty >= f1_y && ty <= f1_y + fh) {
                        g_wifi_active_field = 1;
                        render_wifi_fields(wx, wy, ww);
                        gui_sync();
                    }
                    /* Touch on Password field */
                    else if (tx >= fx && tx <= fx + fw && ty >= f2_y && ty <= f2_y + fh) {
                        g_wifi_active_field = 2;
                        render_wifi_fields(wx, wy, ww);
                        gui_sync();
                    }

                    /* Buttons */
                    int by = wy + 44 + 38 + 46 + 54 + 96 + 34;
                    int bw = (ww - 72) / 3;
                    int bh = 44;
                    int b1 = wx + 24, b2 = b1 + bw + 12, b3 = b2 + bw + 12;

                    /* CONNECT */
                    if (tx >= b1 && tx <= b1 + bw && ty >= by && ty <= by + bh) {
                        if (ctx->host->wifi_connect) {
                            set_wifi_msg("Connecting to AP...", COLOR_YELLOW);
                            render_wifi_status(wx, wy, ww);
                            gui_sync();
                            int ret = ctx->host->wifi_connect(g_wifi_ssid, g_wifi_pass);
                            if (ret == 0) set_wifi_msg("[OK] Connected! IP acquired successfully.", COLOR_GREEN);
                            else set_wifi_msg("[ERROR] Authentication failed with AP.", COLOR_RED);
                            render_wifi_status(wx, wy, ww);
                            gui_sync();
                        }
                    }
                    /* DISCONNECT */
                    else if (tx >= b2 && tx <= b2 + bw && ty >= by && ty <= by + bh) {
                        if (ctx->host->wifi_disconnect) {
                            ctx->host->wifi_disconnect();
                            set_wifi_msg("Wi-Fi interface disconnected.", COLOR_YELLOW);
                            render_wifi_status(wx, wy, ww);
                            gui_sync();
                        }
                    }
                    /* REFRESH IP */
                    else if (tx >= b3 && tx <= b3 + bw && ty >= by && ty <= by + bh) {
                        char st[96] = {0};
                        if (ctx->host->wifi_status && ctx->host->wifi_status(st, sizeof(st)) == 0 && st[0]) {
                            set_wifi_msg(st, COLOR_GREEN);
                        } else {
                            set_wifi_msg("Wi-Fi interface idle or no IP assigned.", COLOR_YELLOW);
                        }
                        render_wifi_status(wx, wy, ww);
                        gui_sync();
                    }
                } else if (!pressed) {
                    was_touched = false;
                }
            }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(30);
    }

    gui_out_str("<<< Wi-Fi GUI exited cleanly.\n");
    return 0;
}

