#include "../common/gui_kit.h"

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Launching MaiaRTOS Banner FX Standalone (/sd/apps/banner.mobj)...\n");

    /* Clear screen with deep cosmic background */
    gui_fill_rect(0, 0, 1280, 720, COLOR_BG);

    /* Draw container box */
    int bx = 140, by = 80, bw = 1000, bh = 560;
    gui_fill_rect(bx, by, bw, bh, COLOR_PANEL);
    gui_draw_rect(bx, by, bw, bh, COLOR_CYAN);
    gui_draw_rect(bx + 2, by + 2, bw - 4, bh - 4, COLOR_BLUE);

    /* Header */
    gui_draw_str_2x(bx + 40, by + 24, "MAIA-RTOS BANNER FX", COLOR_YELLOW, COLOR_PANEL, true);
    gui_draw_str_2x(bx + 40, by + 60, "Standalone RISC-V Modular App", COLOR_WHITE, COLOR_PANEL, true);

    /* Banner wave canvas */
    int wave_x = bx + 40;
    int wave_y = by + 104;
    int wave_w = bw - 80; /* 920 */
    int wave_h = 330;
    gui_fill_rect(wave_x, wave_y, wave_w, wave_h, COLOR_BLACK);
    gui_draw_rect(wave_x - 1, wave_y - 1, wave_w + 2, wave_h + 2, COLOR_MAGENTA);

    /* Footer instruction */
    int f_w = 580, f_h = 42;
    int f_x = bx + (bw - f_w) / 2;
    int f_y = by + bh - 54;
    gui_fill_rect(f_x, f_y, f_w, f_h, COLOR_RED);
    gui_draw_rect(f_x, f_y, f_w, f_h, COLOR_WHITE);
    gui_draw_str_2x(f_x + 18, f_y + 8, "Tap screen or press [ESC] to exit", COLOR_WHITE, COLOR_RED, true);

    gui_sync();

    static const uint16_t wave_cols[] = {
        0xf800, 0xfc00, 0xffe0, 0x07e0, 0x07ff, 0x001f, 0xf81f, 0xfc10
    };
    int n_cols = sizeof(wave_cols) / sizeof(wave_cols[0]);

    bool running = true;
    bool was_touched = false;

    while (running) {
        uint64_t now_us = (ctx->host->get_time_us) ? ctx->host->get_time_us() : 0;
        int shift = (int)((now_us / 25000ULL) % 1000000ULL);

        /* Render dynamic wave bars */
        for (int col = 0; col < wave_w; ++col) {
            int idx = (col + shift) / 24;
            uint16_t c = wave_cols[idx % n_cols];
            int bar_h = 30 + ((col * 5 + shift * 3) % (wave_h - 60));
            int top_h = wave_h - bar_h;
            gui_fill_rect(wave_x + col, wave_y, 1, top_h, COLOR_BLACK);
            gui_fill_rect(wave_x + col, wave_y + top_h, 1, bar_h, c);
        }

        gui_sync();

        /* Check Keyboard */
        if (ctx->host->input && ctx->host->input->read_key) {
            int32_t k = ctx->host->input->read_key();
            while (k >= 0) {
                if (k == 27 || k == 'q' || k == 'Q' || k == 3) {
                    running = false;
                    break;
                }
                k = ctx->host->input->read_key();
            }
        }

        /* Check Touchscreen */
        int32_t tx = 0, ty = 0;
        bool pressed = false;
        if (gui_poll_touch(&tx, &ty, &pressed)) {
            if (pressed) {
                was_touched = true;
            } else if (!pressed && was_touched) {
                running = false;
                break;
            }
        }

        if (ctx->host->sleep_ms) {
            ctx->host->sleep_ms(16);
        }
    }

    gui_out_str("<<< Banner FX Standalone exited cleanly.\n");
    return 0;
}
