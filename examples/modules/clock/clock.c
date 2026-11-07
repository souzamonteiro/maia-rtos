#include "gui_kit.h"

static bool g_sw_running = false;
static uint64_t g_sw_start_us = 0;
static uint64_t g_sw_accum_us = 0;
static char g_sntp_status[64] = "Pronto (SNTP disponivel)";

static void fmt_2digit(uint32_t v, char *dst) {
    dst[0] = '0' + ((v / 10) % 10);
    dst[1] = '0' + (v % 10);
    dst[2] = '\0';
}

static void render_clock(int wx, int wy, int ww, int wh, uint64_t now_us) {
    gui_draw_window_frame(wx, wy, ww, wh, "Relogio RTC e Cronometro", COLOR_YELLOW);

    /* RTC Date/Time query */
    char dt_buf[48] = "00:00:00  01/01/2026";
    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->get_datetime) {
        g_gui_ctx->host->get_datetime(dt_buf, sizeof(dt_buf));
    }

    /* Section 1: RTC Live Clock */
    int cy = wy + 46;
    gui_fill_rect(wx + 16, cy, ww - 32, 88, 0x10a2);
    gui_draw_rect(wx + 16, cy, ww - 32, 88, COLOR_YELLOW);

    gui_draw_str(wx + 28, cy + 8, "HORA ATUAL DO SISTEMA (RTC HW):", COLOR_YELLOW, 0x10a2, true);
    gui_draw_str_2x(wx + 28, cy + 30, dt_buf, COLOR_WHITE, 0x10a2, true);

    /* Button: Sincronizar SNTP */
    gui_draw_button(wx + ww - 190, cy + 10, 160, 30, "Sincronizar NTP", COLOR_BTN_BG, COLOR_CYAN, COLOR_BTN_BD, false);
    gui_draw_str(wx + 28, cy + 66, g_sntp_status, COLOR_TEXT_DIM, 0x10a2, true);

    /* Section 2: Stopwatch */
    int sy = cy + 102;
    gui_fill_rect(wx + 16, sy, ww - 32, 170, 0x10a2);
    gui_draw_rect(wx + 16, sy, ww - 32, 170, COLOR_CYAN);

    gui_draw_str(wx + 28, sy + 12, "CRONOMETRO DE ALTA PRECISAO:", COLOR_CYAN, 0x10a2, true);

    uint64_t elapsed_us = g_sw_accum_us;
    if (g_sw_running && now_us > g_sw_start_us) {
        elapsed_us += (now_us - g_sw_start_us);
    }

    uint32_t total_sec = (uint32_t)(elapsed_us / 1000000ULL);
    uint32_t ms_hundreds = (uint32_t)((elapsed_us % 1000000ULL) / 10000ULL);
    uint32_t sw_min = total_sec / 60;
    uint32_t sw_sec = total_sec % 60;

    char sw_buf[32];
    char m_str[4], s_str[4], ms_str[4];
    fmt_2digit(sw_min, m_str);
    fmt_2digit(sw_sec, s_str);
    fmt_2digit(ms_hundreds, ms_str);

    int pos = 0;
    sw_buf[pos++] = m_str[0]; sw_buf[pos++] = m_str[1];
    sw_buf[pos++] = ':';
    sw_buf[pos++] = s_str[0]; sw_buf[pos++] = s_str[1];
    sw_buf[pos++] = '.';
    sw_buf[pos++] = ms_str[0]; sw_buf[pos++] = ms_str[1];
    sw_buf[pos] = '\0';

    gui_draw_str_2x(wx + 170, sy + 44, sw_buf, COLOR_YELLOW, 0x10a2, true);

    /* Stopwatch Control Buttons */
    int bw = 140, bh = 42;
    int b1_x = wx + 36, b2_x = wx + 190, b3_x = wx + 344;
    int by = sy + 108;

    if (!g_sw_running) {
        gui_draw_button(b1_x, by, bw, bh, "INICIAR", 0x1b24, COLOR_GREEN, COLOR_GREEN, false);
    } else {
        gui_draw_button(b1_x, by, bw, bh, "PAUSAR", 0x8180, COLOR_WHITE, COLOR_RED, false);
    }
    gui_draw_button(b2_x, by, bw, bh, "ZERAR", COLOR_BTN_BG, COLOR_WHITE, COLOR_BTN_BD, false);
    gui_draw_button(b3_x, by, bw, bh, "FECHAR", 0x6000, COLOR_WHITE, COLOR_WHITE, false);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Clock GUI (/sd/apps/clock.mobj)...\n");

    int wx = 380, wy = 120, ww = 520, wh = 380;
    bool running = true;
    bool was_touched = false;
    uint32_t last_sec = 0;

    while (running) {
        uint64_t now_us = (ctx->host->get_time_us) ? ctx->host->get_time_us() : 0;
        uint32_t cur_sec = (uint32_t)(now_us / 1000000ULL);

        if (g_sw_running || cur_sec != last_sec) {
            last_sec = cur_sec;
            render_clock(wx, wy, ww, wh, now_us);
            gui_sync();
        }

        /* Keyboard */
        if (ctx->host->input && ctx->host->input->read_key) {
            int32_t k = ctx->host->input->read_key();
            while (k >= 0) {
                if (k == 27 || k == 'q' || k == 'Q') {
                    running = false;
                    break;
                } else if (k == ' ' || k == '\r' || k == '\n') {
                    /* Toggle stopwatch */
                    if (!g_sw_running) {
                        g_sw_running = true;
                        g_sw_start_us = now_us;
                    } else {
                        g_sw_running = false;
                        g_sw_accum_us += (now_us - g_sw_start_us);
                    }
                    render_clock(wx, wy, ww, wh, now_us);
                    gui_sync();
                } else if (k == 'z' || k == 'Z') {
                    g_sw_running = false;
                    g_sw_accum_us = 0;
                    render_clock(wx, wy, ww, wh, now_us);
                    gui_sync();
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

                    int cy = wy + 46;
                    /* Button: Sincronizar NTP */
                    if (tx >= wx + ww - 190 && tx <= wx + ww - 30 && ty >= cy + 10 && ty <= cy + 40) {
                        if (ctx->host->sntp_sync) {
                            ctx->host->sntp_sync("pool.ntp.org");
                            const char ok_msg[] = "[SNTP] Sincronizacao enviada ao NTP!";
                            for (int i = 0; ok_msg[i]; ++i) g_sntp_status[i] = ok_msg[i];
                            g_sntp_status[sizeof(ok_msg) - 1] = '\0';
                        }
                        render_clock(wx, wy, ww, wh, now_us);
                        gui_sync();
                    }

                    /* Stopwatch Buttons */
                    int sy = cy + 102;
                    int bw = 140, bh = 42;
                    int b1_x = wx + 36, b2_x = wx + 190, b3_x = wx + 344;
                    int by = sy + 108;

                    /* INICIAR / PAUSAR */
                    if (tx >= b1_x && tx <= b1_x + bw && ty >= by && ty <= by + bh) {
                        if (!g_sw_running) {
                            g_sw_running = true;
                            g_sw_start_us = now_us;
                        } else {
                            g_sw_running = false;
                            g_sw_accum_us += (now_us - g_sw_start_us);
                        }
                        render_clock(wx, wy, ww, wh, now_us);
                        gui_sync();
                    }
                    /* ZERAR */
                    else if (tx >= b2_x && tx <= b2_x + bw && ty >= by && ty <= by + bh) {
                        g_sw_running = false;
                        g_sw_accum_us = 0;
                        render_clock(wx, wy, ww, wh, now_us);
                        gui_sync();
                    }
                    /* FECHAR */
                    else if (tx >= b3_x && tx <= b3_x + bw && ty >= by && ty <= by + bh) {
                        running = false;
                        break;
                    }
                } else if (!pressed) {
                    was_touched = false;
                }
            }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(30);
    }

    gui_out_str("<<< Clock GUI exited cleanly.\n");
    return 0;
}

