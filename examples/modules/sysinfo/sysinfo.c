#include "gui_kit.h"

static void render_sysinfo(int wx, int wy, int ww, int wh, uint64_t uptime_s) {
    gui_draw_window_frame(wx, wy, ww, wh, "Informacoes do Sistema - MaiaRTOS", COLOR_GREEN);

    int ly = wy + 48;
    int gap = 24;

    gui_draw_str(wx + 20, ly, "HARDWARE & ARQUITETURA", COLOR_YELLOW, COLOR_WIN_BG, true);
    ly += gap;

    gui_draw_str(wx + 30, ly, "- Processador : ESP32-P4 RISC-V Dual-Core @ 360 MHz (RV32IMAFC)", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += gap;

    gui_draw_str(wx + 30, ly, "- Display     : 7.0\" MIPI-DSI 1280x720 60Hz (PPA HW Accelerated)", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += gap;

    gui_draw_str(wx + 30, ly, "- Memoria     : 32 MB Octal PSRAM + 768 KB L2 SRAM Interna", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += gap;

    gui_draw_str(wx + 30, ly, "- Pool Exec   : 64 KB LIFO Exec Pool em sram_low (.iram.data)", COLOR_CYAN, COLOR_WIN_BG, true);
    ly += gap + 6;

    gui_draw_str(wx + 20, ly, "SISTEMA OPERACIONAL & KERNEL", COLOR_YELLOW, COLOR_WIN_BG, true);
    ly += gap;

    gui_draw_str(wx + 30, ly, "- Kernel      : MaiaRTOS Microkernel v1.0.0 (Freestanding / Zero-IDF)", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += gap;

    gui_draw_str(wx + 30, ly, "- Executaveis : Modulos Nativos ELF32 ET_REL Dinamicos (.mobj)", COLOR_GREEN, COLOR_WIN_BG, true);
    ly += gap;

    gui_draw_str(wx + 30, ly, "- Armazenamento: Cartao MicroSD FAT32 (/sd) + Host USB MSC (/usb)", COLOR_WHITE, COLOR_WIN_BG, true);
    ly += gap;

    char up_str[64];
    uint32_t hours = (uint32_t)(uptime_s / 3600);
    uint32_t mins  = (uint32_t)((uptime_s % 3600) / 60);
    uint32_t secs  = (uint32_t)(uptime_s % 60);

    /* Formatar uptime */
    char h_buf[16], m_buf[16], s_buf[16];
    int hp = 0, mp = 0, sp = 0;
    if (hours == 0) h_buf[hp++] = '0';
    else { uint32_t th = hours; while (th > 0) { h_buf[hp++] = '0' + (th % 10); th /= 10; } }
    if (mins == 0) m_buf[mp++] = '0';
    else { uint32_t tm = mins; while (tm > 0) { m_buf[mp++] = '0' + (tm % 10); tm /= 10; } }
    if (secs == 0) s_buf[sp++] = '0';
    else { uint32_t ts = secs; while (ts > 0) { s_buf[sp++] = '0' + (ts % 10); ts /= 10; } }

    int p = 0;
    const char pfx[] = "- Tempo Ativo : ";
    while (pfx[p]) { up_str[p] = pfx[p]; p++; }
    while (hp > 0) up_str[p++] = h_buf[--hp];
    up_str[p++] = 'h'; up_str[p++] = ' ';
    while (mp > 0) up_str[p++] = m_buf[--mp];
    up_str[p++] = 'm'; up_str[p++] = ' ';
    while (sp > 0) up_str[p++] = s_buf[--sp];
    up_str[p++] = 's';
    up_str[p] = '\0';

    gui_draw_str(wx + 30, ly, up_str, COLOR_YELLOW, COLOR_WIN_BG, true);
    ly += gap + 10;

    gui_draw_button(wx + (ww - 180) / 2, ly, 180, 36, "FECHAR", 0x2169, COLOR_WHITE, COLOR_BTN_BD, false);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Sysinfo GUI (/sd/apps/sysinfo.mobj)...\n");

    int wx = 320, wy = 100, ww = 640, wh = 420;
    bool running = true;
    bool was_touched = false;
    uint32_t last_s = 0;

    while (running) {
        uint64_t now_us = (ctx->host->get_time_us) ? ctx->host->get_time_us() : 0;
        uint32_t now_s = (uint32_t)(now_us / 1000000ULL);

        if (now_s != last_s) {
            last_s = now_s;
            render_sysinfo(wx, wy, ww, wh, now_s);
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
                    int btn_x = wx + (ww - 180) / 2;
                    int btn_y = wy + 48 + 24 * 8 + 16;
                    if (tx >= btn_x && tx <= btn_x + 180 && ty >= btn_y && ty <= btn_y + 36) {
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
