#include "gui_kit.h"

static char g_wifi_ssid[40] = "";
static char g_wifi_pass[40] = "";
static int g_wifi_active_field = 1; /* 1 = SSID, 2 = Password */
static char g_wifi_status_msg[96] = "Desconectado. Pressione CONECTAR ou ESC.";
static uint16_t g_wifi_status_col = COLOR_WHITE;

static void set_wifi_msg(const char *msg, uint16_t col) {
    int i = 0;
    while (msg[i] && i < 95) { g_wifi_status_msg[i] = msg[i]; i++; }
    g_wifi_status_msg[i] = '\0';
    g_wifi_status_col = col;
}

static void render_wifi(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "Configuracao de Rede & Wi-Fi", COLOR_CYAN);

    int ly = wy + 48;
    gui_draw_str(wx + 24, ly, "CONEXAO COM REDE SEM FIO (ESP32-C6 SDIO HOST):", COLOR_YELLOW, COLOR_WIN_BG, true);
    ly += 30;

    /* Field 1: SSID */
    gui_draw_str(wx + 24, ly + 6, "Nome da Rede (SSID):", COLOR_WHITE, COLOR_WIN_BG, true);
    int fx = wx + 230, fw = ww - 260, fh = 32;
    uint16_t f1_bd = (g_wifi_active_field == 1) ? COLOR_YELLOW : COLOR_WIN_BD;
    gui_fill_rect(fx, ly, fw, fh, 0x10a2);
    gui_draw_rect(fx, ly, fw, fh, f1_bd);
    gui_draw_str(fx + 10, ly + 8, g_wifi_ssid, COLOR_WHITE, 0x10a2, true);
    if (g_wifi_active_field == 1) {
        int sl = 0; while (g_wifi_ssid[sl]) sl++;
        gui_fill_rect(fx + 10 + sl * 8, ly + 6, 2, 20, COLOR_YELLOW);
    }
    ly += 42;

    /* Field 2: Password */
    gui_draw_str(wx + 24, ly + 6, "Senha de Acesso:", COLOR_WHITE, COLOR_WIN_BG, true);
    uint16_t f2_bd = (g_wifi_active_field == 2) ? COLOR_YELLOW : COLOR_WIN_BD;
    gui_fill_rect(fx, ly, fw, fh, 0x10a2);
    gui_draw_rect(fx, ly, fw, fh, f2_bd);
    /* Obfuscate password display */
    char pass_stars[40];
    int pl = 0;
    while (g_wifi_pass[pl] && pl < 39) { pass_stars[pl] = '*'; pl++; }
    pass_stars[pl] = '\0';
    gui_draw_str(fx + 10, ly + 8, pass_stars, COLOR_WHITE, 0x10a2, true);
    if (g_wifi_active_field == 2) {
        gui_fill_rect(fx + 10 + pl * 8, ly + 6, 2, 20, COLOR_YELLOW);
    }
    ly += 50;

    /* Current Network Status Info Box */
    gui_fill_rect(wx + 24, ly, ww - 48, 64, 0x10a2);
    gui_draw_rect(wx + 24, ly, ww - 48, 64, COLOR_CYAN);
    gui_draw_str(wx + 36, ly + 10, "Status Atual:", COLOR_CYAN, 0x10a2, true);
    gui_draw_str(wx + 36, ly + 32, g_wifi_status_msg, g_wifi_status_col, 0x10a2, true);
    ly += 80;

    /* Instruction text */
    gui_draw_str(wx + 24, ly, "Pressione [TAB] para alternar campos, [ENTER] para conectar.", COLOR_TEXT_DIM, COLOR_WIN_BG, true);
    ly += 30;

    /* Action Buttons */
    int bw = (ww - 72) / 3;
    int bh = 42;
    int b1 = wx + 24, b2 = b1 + bw + 12, b3 = b2 + bw + 12;

    gui_draw_button(b1, ly, bw, bh, "CONECTAR", 0x1b24, COLOR_GREEN, COLOR_GREEN, false);
    gui_draw_button(b2, ly, bw, bh, "DESCONECTAR", 0x8180, COLOR_RED, COLOR_RED, false);
    gui_draw_button(b3, ly, bw, bh, "ATUALIZAR IP", 0x03a8, COLOR_CYAN, COLOR_CYAN, false);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Wi-Fi GUI (/sd/apps/wifi.mobj)...\n");

    int wx = 280, wy = 100, ww = 720, wh = 380;

    /* Query initial status */
    char st_buf[96] = {0};
    if (ctx->host->wifi_status && ctx->host->wifi_status(st_buf, sizeof(st_buf)) == 0 && st_buf[0]) {
        set_wifi_msg(st_buf, COLOR_GREEN);
    }

    render_wifi(wx, wy, ww, wh);
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
                    render_wifi(wx, wy, ww, wh);
                    gui_sync();
                } else if (k == '\r' || k == '\n') {
                    if (ctx->host->wifi_connect) {
                        set_wifi_msg("Conectando ao AP...", COLOR_YELLOW);
                        render_wifi(wx, wy, ww, wh);
                        gui_sync();
                        int ret = ctx->host->wifi_connect(g_wifi_ssid, g_wifi_pass);
                        if (ret == 0) set_wifi_msg("[OK] Conectado! IP obtido com sucesso.", COLOR_GREEN);
                        else set_wifi_msg("[ERRO] Falha na autenticacao com AP.", COLOR_RED);
                        render_wifi(wx, wy, ww, wh);
                        gui_sync();
                    }
                } else if (k == 8 || k == 127) { /* Backspace */
                    char *tgt = (g_wifi_active_field == 1) ? g_wifi_ssid : g_wifi_pass;
                    int l = 0; while (tgt[l]) l++;
                    if (l > 0) tgt[l - 1] = '\0';
                    render_wifi(wx, wy, ww, wh);
                    gui_sync();
                } else if (k >= 32 && k <= 126) {
                    char *tgt = (g_wifi_active_field == 1) ? g_wifi_ssid : g_wifi_pass;
                    int l = 0; while (tgt[l]) l++;
                    if (l < 38) {
                        tgt[l] = (char)k;
                        tgt[l + 1] = '\0';
                        render_wifi(wx, wy, ww, wh);
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

                    int fx = wx + 230, fw = ww - 260, fh = 32;
                    int f1_y = wy + 48 + 30;
                    int f2_y = f1_y + 42;

                    /* Touch on SSID field */
                    if (tx >= fx && tx <= fx + fw && ty >= f1_y && ty <= f1_y + fh) {
                        g_wifi_active_field = 1;
                        render_wifi(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* Touch on Password field */
                    else if (tx >= fx && tx <= fx + fw && ty >= f2_y && ty <= f2_y + fh) {
                        g_wifi_active_field = 2;
                        render_wifi(wx, wy, ww, wh);
                        gui_sync();
                    }

                    /* Buttons */
                    int by = f2_y + 50 + 64 + 40;
                    int bw = (ww - 72) / 3;
                    int bh = 42;
                    int b1 = wx + 24, b2 = b1 + bw + 12, b3 = b2 + bw + 12;

                    /* CONECTAR */
                    if (tx >= b1 && tx <= b1 + bw && ty >= by && ty <= by + bh) {
                        if (ctx->host->wifi_connect) {
                            set_wifi_msg("Conectando ao AP...", COLOR_YELLOW);
                            render_wifi(wx, wy, ww, wh);
                            gui_sync();
                            int ret = ctx->host->wifi_connect(g_wifi_ssid, g_wifi_pass);
                            if (ret == 0) set_wifi_msg("[OK] Conectado! IP obtido com sucesso.", COLOR_GREEN);
                            else set_wifi_msg("[ERRO] Falha na autenticacao com AP.", COLOR_RED);
                            render_wifi(wx, wy, ww, wh);
                            gui_sync();
                        }
                    }
                    /* DESCONECTAR */
                    else if (tx >= b2 && tx <= b2 + bw && ty >= by && ty <= by + bh) {
                        if (ctx->host->wifi_disconnect) {
                            ctx->host->wifi_disconnect();
                            set_wifi_msg("Interface Wi-Fi desconectada.", COLOR_YELLOW);
                            render_wifi(wx, wy, ww, wh);
                            gui_sync();
                        }
                    }
                    /* ATUALIZAR IP */
                    else if (tx >= b3 && tx <= b3 + bw && ty >= by && ty <= by + bh) {
                        char st[96] = {0};
                        if (ctx->host->wifi_status && ctx->host->wifi_status(st, sizeof(st)) == 0 && st[0]) {
                            set_wifi_msg(st, COLOR_GREEN);
                        } else {
                            set_wifi_msg("Interface Wi-Fi inativa ou sem IP.", COLOR_YELLOW);
                        }
                        render_wifi(wx, wy, ww, wh);
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

