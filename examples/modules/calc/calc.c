#include "gui_kit.h"

static char g_calc_disp[32] = "0";
static double g_calc_val1 = 0;
static char g_calc_op = 0;
static bool g_calc_new_num = true;

static void calc_input(char k) {
    if (k >= '0' && k <= '9') {
        if (g_calc_new_num || (g_calc_disp[0] == '0' && g_calc_disp[1] == '\0')) {
            g_calc_disp[0] = k;
            g_calc_disp[1] = '\0';
            g_calc_new_num = false;
        } else {
            int len = 0;
            while (g_calc_disp[len]) len++;
            if (len < 16) {
                g_calc_disp[len] = k;
                g_calc_disp[len + 1] = '\0';
            }
        }
    } else if (k == '.') {
        if (g_calc_new_num) {
            g_calc_disp[0] = '0';
            g_calc_disp[1] = '.';
            g_calc_disp[2] = '\0';
            g_calc_new_num = false;
        } else {
            bool has_dot = false;
            for (int i = 0; g_calc_disp[i]; ++i) if (g_calc_disp[i] == '.') has_dot = true;
            if (!has_dot) {
                int len = 0;
                while (g_calc_disp[len]) len++;
                if (len < 16) {
                    g_calc_disp[len] = '.';
                    g_calc_disp[len + 1] = '\0';
                }
            }
        }
    } else if (k == 'C' || k == 'c') {
        g_calc_disp[0] = '0';
        g_calc_disp[1] = '\0';
        g_calc_val1 = 0;
        g_calc_op = 0;
        g_calc_new_num = true;
    } else if (k == '+' || k == '-' || k == '*' || k == '/') {
        /* Parse current display to double */
        double val = 0;
        double frac = 0.1;
        bool in_frac = false;
        int s = 0;
        if (g_calc_disp[0] == '-') s = 1;
        for (int i = s; g_calc_disp[i]; ++i) {
            if (g_calc_disp[i] == '.') {
                in_frac = true;
            } else if (g_calc_disp[i] >= '0' && g_calc_disp[i] <= '9') {
                if (!in_frac) val = val * 10 + (g_calc_disp[i] - '0');
                else { val += (g_calc_disp[i] - '0') * frac; frac *= 0.1; }
            }
        }
        if (s) val = -val;

        g_calc_val1 = val;
        g_calc_op = k;
        g_calc_new_num = true;
    } else if (k == '=' || k == '\r' || k == '\n') {
        if (g_calc_op) {
            double val2 = 0;
            double frac = 0.1;
            bool in_frac = false;
            int s = 0;
            if (g_calc_disp[0] == '-') s = 1;
            for (int i = s; g_calc_disp[i]; ++i) {
                if (g_calc_disp[i] == '.') {
                    in_frac = true;
                } else if (g_calc_disp[i] >= '0' && g_calc_disp[i] <= '9') {
                    if (!in_frac) val2 = val2 * 10 + (g_calc_disp[i] - '0');
                    else { val2 += (g_calc_disp[i] - '0') * frac; frac *= 0.1; }
                }
            }
            if (s) val2 = -val2;

            double res = 0;
            if (g_calc_op == '+') res = g_calc_val1 + val2;
            else if (g_calc_op == '-') res = g_calc_val1 - val2;
            else if (g_calc_op == '*') res = g_calc_val1 * val2;
            else if (g_calc_op == '/') res = (val2 != 0) ? (g_calc_val1 / val2) : 0;

            /* Format double back to string */
            int ival = (int)res;
            int dec = (int)((res - ival) * 100);
            if (dec < 0) dec = -dec;

            char buf[32];
            int p = 0;
            if (res < 0 && ival == 0) buf[p++] = '-';
            if (ival < 0) { buf[p++] = '-'; ival = -ival; }
            char tmp[16];
            int tp = 0;
            if (ival == 0) tmp[tp++] = '0';
            while (ival > 0) { tmp[tp++] = '0' + (ival % 10); ival /= 10; }
            while (tp > 0) buf[p++] = tmp[--tp];
            if (dec > 0) {
                buf[p++] = '.';
                buf[p++] = '0' + (dec / 10);
                if (dec % 10 != 0) buf[p++] = '0' + (dec % 10);
            }
            buf[p] = '\0';

            int i = 0;
            while (buf[i] && i < 31) { g_calc_disp[i] = buf[i]; i++; }
            g_calc_disp[i] = '\0';
            g_calc_op = 0;
            g_calc_new_num = true;
        }
    }
}

static void render_calc(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "Calculadora Cientifica", COLOR_GREEN);

    /* LCD Display */
    int disp_x = wx + 20, disp_y = wy + 48, disp_w = ww - 40, disp_h = 70;
    gui_fill_rect(disp_x, disp_y, disp_w, disp_h, 0x0182);
    gui_draw_rect(disp_x, disp_y, disp_w, disp_h, COLOR_GREEN);

    int dlen = 0;
    while (g_calc_disp[dlen]) dlen++;
    int tx = disp_x + disp_w - 24 - (dlen * 16);
    if (tx < disp_x + 10) tx = disp_x + 10;
    gui_draw_str_2x(tx, disp_y + 18, g_calc_disp, COLOR_GREEN, 0x0182, true);

    if (g_calc_op) {
        char op_s[2] = { g_calc_op, '\0' };
        gui_draw_str_2x(disp_x + 16, disp_y + 18, op_s, COLOR_YELLOW, 0x0182, true);
    }

    /* Keypad */
    static const char *labels[5][4] = {
        { "C", "+/-", "%", "/" },
        { "7", "8",   "9", "*" },
        { "4", "5",   "6", "-" },
        { "1", "2",   "3", "+" },
        { "0", ".",   "=", "OK" }
    };
    int kx_start = wx + 20, ky_start = wy + 132;
    int kw = 92, kh = 70, gap = 14;

    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 4; ++c) {
            int kx = kx_start + c * (kw + gap);
            int ky = ky_start + r * (kh + gap);
            uint16_t bg = COLOR_BTN_BG;
            uint16_t fg = COLOR_WHITE;
            if (c == 3) { bg = 0xa280; fg = COLOR_WHITE; }
            if (r == 0) { bg = 0x3186; fg = COLOR_CYAN; }
            gui_draw_button(kx, ky, kw, kh, labels[r][c], bg, fg, COLOR_BTN_BD, false);
        }
    }
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Calc GUI (/sd/apps/calc.mobj)...\n");

    int wx = 415, wy = 70, ww = 450, wh = 580;
    render_calc(wx, wy, ww, wh);
    gui_sync();

    static const char keys_map[5][4] = {
        { 'C', 0, '%', '/' },
        { '7', '8', '9', '*' },
        { '4', '5', '6', '-' },
        { '1', '2', '3', '+' },
        { '0', '.', '=', '=' }
    };

    bool running = true;
    bool was_touched = false;

    while (running) {
        /* Keyboard */
        if (ctx->host->input && ctx->host->input->read_key) {
            int32_t k = ctx->host->input->read_key();
            while (k >= 0) {
                if (k == 27 || k == 'q' || k == 'Q') {
                    running = false;
                    break;
                } else if ((k >= '0' && k <= '9') || k == '+' || k == '-' || k == '*' || k == '/' || k == '=' || k == '\r' || k == '\n' || k == '.' || k == 'c' || k == 'C') {
                    calc_input((char)k);
                    render_calc(wx, wy, ww, wh);
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

                    int kx_start = wx + 20, ky_start = wy + 132;
                    int kw = 92, kh = 70, gap = 14;
                    for (int r = 0; r < 5; ++r) {
                        for (int c = 0; c < 4; ++c) {
                            int kx = kx_start + c * (kw + gap);
                            int ky = ky_start + r * (kh + gap);
                            if (tx >= kx && tx <= kx + kw && ty >= ky && ty <= ky + kh) {
                                if (keys_map[r][c]) {
                                    calc_input(keys_map[r][c]);
                                    render_calc(wx, wy, ww, wh);
                                    gui_sync();
                                }
                            }
                        }
                    }
                } else if (!pressed) {
                    was_touched = false;
                }
            }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(20);
    }

    gui_out_str("<<< Calc GUI exited cleanly.\n");
    return 0;
}
