#include "gui_kit.h"

static char g_calc_expr[64] = "0";
static char g_calc_res[32] = "= 0";
static bool g_calc_clear_on_next = true;

/* --- Recursive Descent Expression Parser with Precedence --- */
static void skip_whitespace(const char **p) {
    while (**p == ' ' || **p == '\t') (*p)++;
}

static float parse_expression(const char **p, bool *err);

static float parse_number(const char **p, bool *err) {
    skip_whitespace(p);
    if (!**p) { *err = true; return 0.0f; }

    float val = 0.0f;
    bool has_digits = false;
    while (**p >= '0' && **p <= '9') {
        val = val * 10.0f + (float)(**p - '0');
        (*p)++;
        has_digits = true;
    }
    if (**p == '.') {
        (*p)++;
        float frac = 0.1f;
        while (**p >= '0' && **p <= '9') {
            val += (float)(**p - '0') * frac;
            frac *= 0.1f;
            (*p)++;
            has_digits = true;
        }
    }
    if (!has_digits) {
        *err = true;
        return 0.0f;
    }
    return val;
}

static float parse_factor(const char **p, bool *err) {
    skip_whitespace(p);
    if (*err || !**p) { *err = true; return 0.0f; }

    if (**p == '+') {
        (*p)++;
        return parse_factor(p, err);
    }
    if (**p == '-') {
        (*p)++;
        return -parse_factor(p, err);
    }
    if (**p == '(') {
        (*p)++;
        float val = parse_expression(p, err);
        skip_whitespace(p);
        if (**p == ')') {
            (*p)++;
        } else {
            *err = true;
        }
        return val;
    }
    return parse_number(p, err);
}

static float parse_term(const char **p, bool *err) {
    float left = parse_factor(p, err);
    while (!*err) {
        skip_whitespace(p);
        char op = **p;
        if (op == '*' || op == '/' || op == '%') {
            (*p)++;
            float right = parse_factor(p, err);
            if (*err) break;
            if (op == '*') {
                left *= right;
            } else if (op == '/') {
                if (right == 0.0f) {
                    *err = true;
                    return 0.0f;
                }
                left /= right;
            } else if (op == '%') {
                int iright = (int)right;
                if (iright == 0) {
                    *err = true;
                    return 0.0f;
                }
                left = (float)((int)left % iright);
            }
        } else {
            break;
        }
    }
    return left;
}

static float parse_expression(const char **p, bool *err) {
    float left = parse_term(p, err);
    while (!*err) {
        skip_whitespace(p);
        char op = **p;
        if (op == '+' || op == '-') {
            (*p)++;
            float right = parse_term(p, err);
            if (*err) break;
            if (op == '+') left += right;
            else left -= right;
        } else {
            break;
        }
    }
    return left;
}

static float parse_calc(const char *expr, bool *err) {
    *err = false;
    const char *p = expr;
    float res = parse_expression(&p, err);
    skip_whitespace(&p);
    if (*p != '\0') {
        *err = true;
    }
    return res;
}

static void format_calc_result(float val, char *out, size_t out_sz) {
    if (out_sz < 4) return;
    int j = 0;
    out[j++] = '=';
    out[j++] = ' ';

    if (val < 0.0f) {
        out[j++] = '-';
        val = -val;
    }

    int int_part = (int)val;
    float frac_part = val - (float)int_part;

    char buf[16];
    int bi = 0;
    if (int_part == 0) {
        buf[bi++] = '0';
    } else {
        unsigned int uint_part = (unsigned int)int_part;
        while (uint_part > 0 && bi < 14) {
            buf[bi++] = '0' + (uint_part % 10);
            uint_part /= 10;
        }
    }
    for (int k = bi - 1; k >= 0 && j < (int)out_sz - 1; --k) {
        out[j++] = buf[k];
    }

    if (frac_part >= 0.0001f && j < (int)out_sz - 8) {
        out[j++] = '.';
        for (int d = 0; d < 4 && j < (int)out_sz - 1; ++d) {
            frac_part *= 10.0f;
            int digit = (int)frac_part;
            if (digit > 9) digit = 9;
            out[j++] = '0' + digit;
            frac_part -= (float)digit;
        }
        while (j > 0 && out[j - 1] == '0') j--;
        if (j > 0 && out[j - 1] == '.') j--;
    }

    out[j] = '\0';
}

static void calc_input(char key) {
    size_t len = 0;
    while (g_calc_expr[len]) len++;

    if (key == 'C' || key == 'c') {
        g_calc_expr[0] = '0';
        g_calc_expr[1] = '\0';
        g_calc_res[0] = '='; g_calc_res[1] = ' '; g_calc_res[2] = '0'; g_calc_res[3] = '\0';
        g_calc_clear_on_next = true;
        return;
    }

    if (key == 'D' || key == 8 || key == 127) { // DEL
        if (len > 1) {
            g_calc_expr[len - 1] = '\0';
        } else {
            g_calc_expr[0] = '0';
            g_calc_expr[1] = '\0';
            g_calc_clear_on_next = true;
        }
        g_calc_res[0] = '\0';
        return;
    }

    if (key == '=' || key == '\r' || key == '\n') {
        bool err = false;
        float res = parse_calc(g_calc_expr, &err);
        if (err) {
            g_calc_res[0] = '='; g_calc_res[1] = ' ';
            g_calc_res[2] = 'E'; g_calc_res[3] = 'R'; g_calc_res[4] = 'R';
            g_calc_res[5] = 'O'; g_calc_res[6] = '\0';
        } else {
            format_calc_result(res, g_calc_res, sizeof(g_calc_res));
        }
        g_calc_clear_on_next = true;
        return;
    }

    if ((key >= '0' && key <= '9') || key == '+' || key == '-' || key == '*' || key == '/' || key == '%' || key == '.' || key == '(' || key == ')') {
        if (g_calc_clear_on_next) {
            if (key >= '0' && key <= '9') {
                g_calc_expr[0] = key;
                g_calc_expr[1] = '\0';
                g_calc_res[0] = '\0';
                g_calc_clear_on_next = false;
                return;
            } else if (key == '(' || key == '.') {
                g_calc_expr[0] = key;
                g_calc_expr[1] = '\0';
                g_calc_res[0] = '\0';
                g_calc_clear_on_next = false;
                return;
            } else {
                g_calc_clear_on_next = false;
            }
        }
        if (len < sizeof(g_calc_expr) - 2) {
            g_calc_expr[len] = key;
            g_calc_expr[len + 1] = '\0';
        }
    }
}

/* Partial dirty redraw: only renders LCD display screen (Zero Flicker!) */
static void render_calc_display(int wx, int wy, int ww) {
    int disp_x = wx + 20, disp_y = wy + 48, disp_w = ww - 40, disp_h = 72;

    gui_set_clip(disp_x, disp_y, disp_w, disp_h);
    gui_fill_rect(disp_x, disp_y, disp_w, disp_h, COLOR_BLACK);
    gui_draw_rect(disp_x, disp_y, disp_w, disp_h, COLOR_CYAN);

    /* Top Line: Expression (Large 2x or 1x depending on length) */
    int elen = 0;
    while (g_calc_expr[elen]) elen++;
    if (elen <= 18) {
        gui_draw_str_2x(disp_x + 12, disp_y + 8, g_calc_expr, COLOR_WHITE, COLOR_BLACK, true);
    } else {
        gui_draw_str(disp_x + 12, disp_y + 12, g_calc_expr, COLOR_WHITE, COLOR_BLACK, true);
    }

    /* Bottom Line: Result in Neon Green (2x typography) */
    if (g_calc_res[0]) {
        int rlen = 0;
        while (g_calc_res[rlen]) rlen++;
        int rx = disp_x + disp_w - 14 - (rlen * 16);
        if (rx < disp_x + 12) rx = disp_x + 12;
        gui_draw_str_2x(rx, disp_y + 36, g_calc_res, COLOR_GREEN, COLOR_BLACK, true);
    }

    gui_reset_clip();
}

/* Full chrome render (drawn once on open) */
static void render_calc_chrome(int wx, int wy, int ww, int wh) {
    gui_set_clip(wx, wy, ww, wh);
    gui_draw_window_frame(wx, wy, ww, wh, "Calculator", COLOR_GREEN);

    /* Keypad Grid (4 cols x 5 rows) */
    static const char *keys[5][4] = {
        {"C", "(", ")", "/"},
        {"7", "8", "9", "*"},
        {"4", "5", "6", "-"},
        {"1", "2", "3", "+"},
        {"0", ".", "DEL", "="}
    };

    int kx_start = wx + 20;
    int ky_start = wy + 132;
    int kw = 92;
    int kh = 70;
    int gap = 14;

    for (int r = 0; r < 5; ++r) {
        for (int c = 0; c < 4; ++c) {
            int kx = kx_start + c * (kw + gap);
            int ky = ky_start + r * (kh + gap);
            const char *label = keys[r][c];
            uint16_t bg = COLOR_BTN_BG;
            uint16_t fg = COLOR_WHITE;
            if (label[0] == '=') {
                bg = COLOR_GREEN;
                fg = COLOR_BLACK;
            } else if (label[0] == 'C' || label[0] == 'D') {
                bg = COLOR_RED;
                fg = COLOR_WHITE;
            } else if (label[0] == '+' || label[0] == '-' || label[0] == '*' || label[0] == '/') {
                bg = COLOR_CYAN;
                fg = COLOR_BLACK;
            } else if (label[0] == '(' || label[0] == ')') {
                bg = COLOR_BTN_BG;
                fg = COLOR_YELLOW;
            }
            gui_draw_button(kx, ky, kw, kh, label, bg, fg, COLOR_BTN_BD, false);
        }
    }

    gui_reset_clip();
    render_calc_display(wx, wy, ww);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    int wx = 415, wy = 70, ww = 450, wh = 580;

    /* Render Chrome ONCE */
    render_calc_chrome(wx, wy, ww, wh);
    gui_sync();

    static const char keys_map[5][4] = {
        {'C', '(', ')', '/'},
        {'7', '8', '9', '*'},
        {'4', '5', '6', '-'},
        {'1', '2', '3', '+'},
        {'0', '.', 'D', '='}
    };

    bool running = true;
    bool was_touched = false;

    while (running) {
        /* Keyboard */
        int32_t k = gui_poll_key();
        while (k >= 0) {
            if (k == 27 || k == 'q' || k == 'Q') {
                running = false;
                break;
            } else if (k == '\r' || k == '\n' || k == '=') {
                calc_input('=');
                render_calc_display(wx, wy, ww);
                gui_sync();
            } else if (k == 8 || k == 127) {
                calc_input('D');
                render_calc_display(wx, wy, ww);
                gui_sync();
            } else if (k == 'c' || k == 'C') {
                calc_input('C');
                render_calc_display(wx, wy, ww);
                gui_sync();
            } else if ((k >= '0' && k <= '9') || k == '+' || k == '-' || k == '*' || k == '/' || k == '%' || k == '.' || k == '(' || k == ')') {
                calc_input((char)k);
                render_calc_display(wx, wy, ww);
                gui_sync();
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

                int kx_start = wx + 20, ky_start = wy + 132;
                int kw = 92, kh = 70, gap = 14;
                for (int r = 0; r < 5; ++r) {
                    for (int c = 0; c < 4; ++c) {
                        int kx = kx_start + c * (kw + gap);
                        int ky = ky_start + r * (kh + gap);
                        if (tx >= kx && tx <= kx + kw && ty >= ky && ty <= ky + kh) {
                            calc_input(keys_map[r][c]);
                            render_calc_display(wx, wy, ww);
                            gui_sync();
                        }
                    }
                }
            } else if (!pressed) {
                was_touched = false;
            }
        }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(20);
    }

    return 0;
}
