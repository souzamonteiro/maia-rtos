#include "gui_kit.h"

static void render_sysinfo_chrome(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "System Information - MaiaRTOS", COLOR_GREEN);

    int ly = wy + 44;
    gui_draw_str_2x(wx + 24, ly, "HARDWARE & ARCHITECTURE", COLOR_YELLOW, COLOR_WIN_BG, true);
    ly += 34;

    gui_draw_str_2x(wx + 24, ly, "CPU : ESP32-P4 Dual-Core @ 360 MHz", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += 30;

    gui_draw_str_2x(wx + 24, ly, "LCD : 7.0\" MIPI 1280x720 (PPA Accel)", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += 30;

    gui_draw_str_2x(wx + 24, ly, "RAM : 32 MB PSRAM + 768 KB L2 SRAM", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += 30;

    gui_draw_str_2x(wx + 24, ly, "POOL: 64 KB LIFO Exec Pool in SRAM", COLOR_CYAN, COLOR_WIN_BG, true);
    ly += 38;

    gui_draw_str_2x(wx + 24, ly, "OPERATING SYSTEM & KERNEL", COLOR_YELLOW, COLOR_WIN_BG, true);
    ly += 34;

    gui_draw_str_2x(wx + 24, ly, "OS  : MaiaRTOS Microkernel v1.0.0", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += 30;

    gui_draw_str_2x(wx + 24, ly, "MOD : Dynamic ELF32 ET_REL (.mobj)", COLOR_GREEN, COLOR_WIN_BG, true);
    ly += 30;

    gui_draw_str_2x(wx + 24, ly, "VFS : MicroSD (/sd) + USB MSC (/usb)", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += 34;

    gui_draw_str_2x(wx + 24, ly, "Uptime:", COLOR_TEXT_DIM, COLOR_WIN_BG, true);

    gui_draw_button(wx + (ww - 200) / 2, wy + wh - 56, 200, 42, "CLOSE", 0x2169, COLOR_WHITE, COLOR_BTN_BD, false);
}

static void render_sysinfo_uptime(int wx, int wy, int ww, uint64_t uptime_s) {
    char up_str[64];
    uint32_t hours = (uint32_t)(uptime_s / 3600);
    uint32_t mins  = (uint32_t)((uptime_s % 3600) / 60);
    uint32_t secs  = (uint32_t)(uptime_s % 60);

    char h_buf[16], m_buf[16], s_buf[16];
    int hp = 0, mp = 0, sp = 0;
    if (hours == 0) h_buf[hp++] = '0';
    else { uint32_t th = hours; while (th > 0) { h_buf[hp++] = '0' + (th % 10); th /= 10; } }
    if (mins == 0) m_buf[mp++] = '0';
    else { uint32_t tm = mins; while (tm > 0) { m_buf[mp++] = '0' + (tm % 10); tm /= 10; } }
    if (secs == 0) s_buf[sp++] = '0';
    else { uint32_t ts = secs; while (ts > 0) { s_buf[sp++] = '0' + (ts % 10); ts /= 10; } }

    int p = 0;
    while (hp > 0) up_str[p++] = h_buf[--hp];
    up_str[p++] = 'h'; up_str[p++] = ' ';
    while (mp > 0) up_str[p++] = m_buf[--mp];
    up_str[p++] = 'm'; up_str[p++] = ' ';
    while (sp > 0) up_str[p++] = s_buf[--sp];
    up_str[p++] = 's';
    up_str[p] = '\0';

    int uy = wy + 44 + 34 + 30 * 3 + 38 + 34 + 30 * 2 + 34;
    gui_fill_rect(wx + 160, uy, ww - 200, 28, COLOR_WIN_BG);
    gui_draw_str_2x(wx + 160, uy, up_str, COLOR_YELLOW, COLOR_WIN_BG, true);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Sysinfo GUI (/sd/apps/sysinfo.mobj)...\n");

    int wx = 260, wy = 70, ww = 760, wh = 540;
    render_sysinfo_chrome(wx, wy, ww, wh);
    render_sysinfo_uptime(wx, wy, ww, 0);
    gui_sync();

    bool running = true;
    bool was_touched = false;
    uint32_t last_s = 0;

    while (running) {
        uint64_t now_us = (ctx->host->get_time_us) ? ctx->host->get_time_us() : 0;
        uint32_t now_s = (uint32_t)(now_us / 1000000ULL);

        if (now_s != last_s) {
            last_s = now_s;
            render_sysinfo_uptime(wx, wy, ww, now_s);
            gui_sync();
        }

        /* Keyboard */
        if (ctx->host->input && ctx->host->input->read_key) {
            int32_t k = ctx->host->input->read_key();
            while (k >= 0) {
                if (k == 27 || k == 'q' || k == 'Q') {
                    running = false;
                    break;
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
                    /* Check Fechar button */
                    int btn_x = wx + (ww - 200) / 2;
                    int btn_y = wy + wh - 56;
                    if (tx >= btn_x && tx <= btn_x + 200 && ty >= btn_y && ty <= btn_y + 42) {
                        running = false;
                        break;
                    }
                } else if (!pressed) {
                    was_touched = false;
                }
            }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(40);
    }

    gui_out_str("<<< Sysinfo GUI exited cleanly.\n");
    return 0;
}
