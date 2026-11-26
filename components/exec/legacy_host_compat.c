#include <maia/exec/legacy_host_compat.h>

#include <limits.h>
#include <math.h>
#include <stddef.h>
#include <string.h>

#include "../../platforms/tab5/main/font_8x16.h"

#define LEGACY_MAX_TEXT_CHARS 2048U
#define LEGACY_MAX_LINE_WIDTH 32

static maia_legacy_host_scope_t *s_active_scope;

static maia_legacy_host_scope_t *active_scope(void) {
    return s_active_scope;
}

static void report_error(maia_legacy_host_scope_t *scope, const char *message) {
    if (scope == NULL || scope->error_reported) return;
    scope->error_reported = 1U;
    if (scope->host != NULL && scope->host->write_fd != NULL) {
        (void)scope->host->write_fd(2U, message, (uint32_t)strlen(message));
    }
}

static void report_touch_unavailable(maia_legacy_host_scope_t *scope) {
    static const char message[] =
        "Legacy touch input unavailable; retaining prior state\n";
    if (scope == NULL || scope->touch_unavailable_reported) return;
    scope->touch_unavailable_reported = 1U;
    if (scope->host != NULL && scope->host->write_fd != NULL) {
        (void)scope->host->write_fd(2U, message, sizeof(message) - 1U);
    }
}

static int ensure_graphics(maia_legacy_host_scope_t *scope) {
    if (scope == NULL || scope->host == NULL || scope->host->display == NULL ||
        scope->host->display->fill_rect == NULL ||
        scope->host->display->sync == NULL ||
        scope->host->display->acquire_exclusive == NULL) {
        if (scope != NULL) {
            scope->graphics_error = 1U;
            report_error(scope, "Legacy graphics API unavailable\n");
        }
        return -1;
    }
    if (scope->graphics_acquired) return 0;
    if (scope->host->display->acquire_exclusive() < 0) {
        scope->graphics_error = 1U;
        report_error(scope, "Legacy graphics lease acquisition failed\n");
        return -1;
    }
    scope->graphics_acquired = 1U;
    return 0;
}

static void fill_rect_clipped(maia_legacy_host_scope_t *scope,
                              int64_t x, int64_t y, int64_t width,
                              int64_t height, uint16_t color) {
    if (scope == NULL || width <= 0 || height <= 0 ||
        ensure_graphics(scope) != 0) {
        return;
    }
    const maia_display_api_t *display = scope->host->display;
    if (display->width <= 0 || display->height <= 0) {
        scope->graphics_error = 1U;
        report_error(scope, "Legacy graphics dimensions invalid\n");
        return;
    }

    int64_t left = x < 0 ? 0 : x;
    int64_t top = y < 0 ? 0 : y;
    int64_t right = x + width;
    int64_t bottom = y + height;
    if (right > display->width) right = display->width;
    if (bottom > display->height) bottom = display->height;
    if (right <= left || bottom <= top) return;
    if (display->fill_rect((int32_t)left, (int32_t)top,
                           (int32_t)(right - left),
                           (int32_t)(bottom - top), color) < 0) {
        scope->graphics_error = 1U;
        report_error(scope, "Legacy graphics rectangle failed\n");
    }
}

static void legacy_fill_rect(int32_t x, int32_t y, int32_t width,
                             int32_t height, int32_t color) {
    fill_rect_clipped(active_scope(), x, y, width, height, (uint16_t)color);
}

static void legacy_display_sync(void) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope != NULL && scope->host != NULL && scope->host->display != NULL &&
        scope->host->display->sync != NULL) {
        scope->host->display->sync();
    }
}

static void legacy_draw_rect(int32_t x, int32_t y, int32_t width,
                             int32_t height, int32_t color) {
    if (width <= 0 || height <= 0) return;
    maia_legacy_host_scope_t *scope = active_scope();
    fill_rect_clipped(scope, x, y, width, 1, (uint16_t)color);
    fill_rect_clipped(scope, x, (int64_t)y + height - 1, width, 1,
                      (uint16_t)color);
    fill_rect_clipped(scope, x, y, 1, height, (uint16_t)color);
    fill_rect_clipped(scope, (int64_t)x + width - 1, y, 1, height,
                      (uint16_t)color);
}

static void legacy_draw_text(int32_t x, int32_t y, const char *text,
                             int32_t foreground, int32_t background) {
    if (text == NULL) return;
    maia_legacy_host_scope_t *scope = active_scope();
    size_t length = 0U;
    while (length < LEGACY_MAX_TEXT_CHARS && text[length] != '\0') ++length;
    if (length == LEGACY_MAX_TEXT_CHARS && text[length] != '\0') {
        report_error(scope, "Legacy text length limit exceeded\n");
        return;
    }

    int64_t origin_x = x;
    int64_t cursor_x = origin_x;
    int64_t cursor_y = y;
    for (size_t i = 0U; i < length; ++i) {
        unsigned char character = (unsigned char)text[i];
        if (character == '\n') {
            cursor_x = origin_x;
            cursor_y += 32;
            continue;
        }
        if (character < 32U || character > 126U) character = (unsigned char)' ';
        const uint8_t *glyph = &g_font_8x16[((size_t)character - 32U) * 16U];
        for (int32_t row = 0; row < 16; ++row) {
            uint8_t bits = glyph[row];
            for (int32_t column = 0; column < 8; ++column) {
                if ((bits & (uint8_t)(0x80U >> column)) != 0U) {
                    fill_rect_clipped(scope, cursor_x + column * 2,
                                      cursor_y + row * 2, 2, 2,
                                      (uint16_t)foreground);
                } else if (background != 0) {
                    fill_rect_clipped(scope, cursor_x + column * 2,
                                      cursor_y + row * 2, 2, 2,
                                      (uint16_t)background);
                }
            }
        }
        cursor_x += 16;
    }
}

static uint32_t line_outcode(int64_t x, int64_t y, int32_t width,
                             int32_t height) {
    uint32_t code = 0U;
    if (x < 0) code |= 1U;
    else if (x >= width) code |= 2U;
    if (y < 0) code |= 4U;
    else if (y >= height) code |= 8U;
    return code;
}

static int clip_line(int64_t *x0, int64_t *y0, int64_t *x1, int64_t *y1,
                     int32_t width, int32_t height) {
    for (unsigned iteration = 0U; iteration < 16U; ++iteration) {
        uint32_t code0 = line_outcode(*x0, *y0, width, height);
        uint32_t code1 = line_outcode(*x1, *y1, width, height);
        if ((code0 | code1) == 0U) return 1;
        if ((code0 & code1) != 0U) return 0;

        uint32_t outside = code0 != 0U ? code0 : code1;
        int64_t x = 0;
        int64_t y = 0;
        int64_t dx = *x1 - *x0;
        int64_t dy = *y1 - *y0;
        if ((outside & 4U) != 0U) {
            if (dy == 0) return 0;
            y = 0;
            x = *x0 + dx * (y - *y0) / dy;
        } else if ((outside & 8U) != 0U) {
            if (dy == 0) return 0;
            y = height - 1;
            x = *x0 + dx * (y - *y0) / dy;
        } else if ((outside & 2U) != 0U) {
            if (dx == 0) return 0;
            x = width - 1;
            y = *y0 + dy * (x - *x0) / dx;
        } else {
            if (dx == 0) return 0;
            x = 0;
            y = *y0 + dy * (x - *x0) / dx;
        }
        if (outside == code0) {
            *x0 = x;
            *y0 = y;
        } else {
            *x1 = x;
            *y1 = y;
        }
    }
    return 0;
}

static void legacy_draw_line(int32_t x0_arg, int32_t y0_arg,
                             int32_t x1_arg, int32_t y1_arg,
                             int32_t color, int32_t line_width) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope == NULL || scope->host == NULL || scope->host->display == NULL) return;
    const maia_display_api_t *display = scope->host->display;
    if (display->width <= 0 || display->height <= 0) return;

    int64_t x0 = x0_arg;
    int64_t y0 = y0_arg;
    int64_t x1 = x1_arg;
    int64_t y1 = y1_arg;
    if (!clip_line(&x0, &y0, &x1, &y1, display->width, display->height)) return;

    if (line_width <= 0) line_width = 1;
    if (line_width > LEGACY_MAX_LINE_WIDTH) {
        line_width = LEGACY_MAX_LINE_WIDTH;
        report_error(scope, "Legacy line width capped at 32 pixels\n");
    }
    int64_t dx = x1 >= x0 ? x1 - x0 : x0 - x1;
    int64_t sx = x0 < x1 ? 1 : -1;
    int64_t dy = -(y1 >= y0 ? y1 - y0 : y0 - y1);
    int64_t sy = y0 < y1 ? 1 : -1;
    int64_t error = dx + dy;
    int64_t half = line_width / 2;

    for (;;) {
        fill_rect_clipped(scope, x0 - half, y0 - half, line_width,
                          line_width, (uint16_t)color);
        if (x0 == x1 && y0 == y1) break;
        int64_t doubled_error = 2 * error;
        if (doubled_error >= dy) {
            error += dy;
            x0 += sx;
        }
        if (doubled_error <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

static void legacy_draw_window(int32_t x, int32_t y, int32_t width,
                               int32_t height, const char *title,
                               int32_t accent) {
    if (width <= 0 || height <= 0) return;
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope == NULL) return;
    scope->window_x = x;
    scope->window_y = y;
    scope->window_width = width;
    scope->window_height = height;

    fill_rect_clipped(scope, (int64_t)x + 6, (int64_t)y + 6, width, height, 0x0821U);
    fill_rect_clipped(scope, x, y, width, height, 0x18E5U);
    fill_rect_clipped(scope, x, y, width, 2, (uint16_t)accent);
    fill_rect_clipped(scope, x, y, 2, height, 0x3A0CU);
    fill_rect_clipped(scope, (int64_t)x + width - 2, y, 2, height, 0x3A0CU);
    fill_rect_clipped(scope, x, (int64_t)y + height - 2, width, 2, 0x3A0CU);
    fill_rect_clipped(scope, (int64_t)x + 2, (int64_t)y + 2,
                      (int64_t)width - 4, 38, 0x2128U);
    legacy_draw_text(x + 14, y + 8, title != NULL ? title : "Window",
                     0xFFFF, 0);
    fill_rect_clipped(scope, (int64_t)x + width - 36, (int64_t)y + 6,
                      28, 28, 0xF800U);
    legacy_draw_text((int32_t)((int64_t)x + width - 28), y + 8, "X", 0xFFFF, 0);
}

static maia_legacy_host_button_t *find_button(maia_legacy_host_scope_t *scope,
                                              int32_t x, int32_t y) {
    if (scope == NULL) return NULL;
    for (int32_t i = 0; i < scope->button_count; ++i) {
        if (scope->buttons[i].x == x && scope->buttons[i].y == y) {
            return &scope->buttons[i];
        }
    }
    return NULL;
}

static void legacy_bind_button(int32_t x, int32_t y, uintptr_t callback) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope == NULL) return;
    maia_legacy_host_button_t *button = find_button(scope, x, y);
    if (button != NULL) {
        button->callback = callback;
        return;
    }
    if ((uint32_t)scope->button_count >= MAIA_LEGACY_HOST_MAX_BUTTONS) {
        report_error(scope, "Legacy button limit exceeded\n");
        return;
    }
    button = &scope->buttons[scope->button_count++];
    button->x = x;
    button->y = y;
    button->width = 100;
    button->height = 32;
    button->label = "";
    button->callback = callback;
}

static void legacy_draw_button(int32_t x, int32_t y, int32_t width,
                               int32_t height, const char *label,
                               int32_t background, int32_t foreground,
                               int32_t pressed) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope == NULL || width <= 0 || height <= 0) return;
    fill_rect_clipped(scope, x, y, width, height,
                      pressed ? 0x04BFU : (uint16_t)background);
    legacy_draw_rect(x, y, width, height, 0x4A4D);
    if (label != NULL) {
        size_t length = 0U;
        while (length < 128U && label[length] != '\0') ++length;
        if (length == 128U) {
            report_error(scope, "Legacy button label length limit exceeded\n");
        } else {
            int64_t text_x = (int64_t)x +
                ((int64_t)width - (int64_t)length * 16) / 2;
            int64_t text_y = (int64_t)y + ((int64_t)height - 32) / 2;
            if (text_x >= INT32_MIN && text_x <= INT32_MAX &&
                text_y >= INT32_MIN && text_y <= INT32_MAX) {
                legacy_draw_text((int32_t)text_x, (int32_t)text_y, label,
                                 foreground, 0);
            }
        }
    }

    maia_legacy_host_button_t *button = find_button(scope, x, y);
    if (button == NULL) {
        if ((uint32_t)scope->button_count >= MAIA_LEGACY_HOST_MAX_BUTTONS) {
            report_error(scope, "Legacy button limit exceeded\n");
            return;
        }
        button = &scope->buttons[scope->button_count++];
        button->x = x;
        button->y = y;
        button->callback = 0U;
    }
    button->width = width;
    button->height = height;
    button->label = label != NULL ? label : "";
}

static void legacy_bind_key(uintptr_t callback) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope != NULL) {
        scope->key_callback = callback;
    }
}

static void legacy_bind_tick(uintptr_t callback) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope != NULL) scope->tick_callback = callback;
}

static void legacy_app_quit(int32_t code) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope == NULL) return;
    scope->app_running = 0;
    scope->exit_code = code;
}

static int32_t legacy_touch_poll(void) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope == NULL || scope->host == NULL || scope->host->input == NULL ||
        scope->host->input->get_touch == NULL) {
        return -1;
    }
    if (scope->host->input->get_touch(&scope->touch_x, &scope->touch_y,
                                      &scope->touch_pressed) < 0) {
        return -1;
    }
    return scope->touch_pressed != 0 ? 1 : 0;
}

static int32_t legacy_touch_x(void) {
    maia_legacy_host_scope_t *scope = active_scope();
    return scope != NULL ? scope->touch_x : 0;
}

static int32_t legacy_touch_y(void) {
    maia_legacy_host_scope_t *scope = active_scope();
    return scope != NULL ? scope->touch_y : 0;
}

static int32_t legacy_app_run(void) {
    maia_legacy_host_scope_t *scope = active_scope();
    if (scope == NULL || scope->host == NULL || scope->host->input == NULL ||
        scope->host->display == NULL || scope->host->display->sync == NULL ||
        scope->host->input->get_touch == NULL ||
        scope->host->input->read_key == NULL ||
        scope->host->sleep_ms == NULL) {
        report_error(scope, "Legacy app host API unavailable\n");
        return -1;
    }

    scope->app_running = 1;
    int was_touched = 1;
    uint64_t last_tick = scope->host->get_time_us != NULL
        ? scope->host->get_time_us() : 0U;
    uint64_t elapsed_without_clock = 0U;

    while (scope->app_running) {
        for (unsigned key_count = 0U; key_count < 64U; ++key_count) {
            int32_t key = scope->host->input->read_key();
            if (key < 0) break;
            if (key == 27 || (scope->key_callback == 0U &&
                              (key == 'q' || key == 'Q'))) {
                scope->app_running = 0;
                break;
            }
            if (scope->key_callback != 0U) {
                ((void (*)(int32_t))scope->key_callback)(key);
            }
            if (!scope->app_running) break;
        }
        if (!scope->app_running) break;

        int32_t touch_x = scope->touch_x;
        int32_t touch_y = scope->touch_y;
        int32_t touch_pressed = scope->touch_pressed;
        int32_t touch_result = scope->host->input->get_touch(
            &touch_x, &touch_y, &touch_pressed);
        if (touch_result < 0) {
            report_touch_unavailable(scope);
        } else {
            scope->touch_x = touch_x;
            scope->touch_y = touch_y;
            scope->touch_pressed = touch_pressed;
        }
        if (touch_result == 0 && scope->touch_pressed != 0 && !was_touched) {
            was_touched = 1;
            int64_t close_x = (int64_t)scope->window_x +
                              scope->window_width - 36;
            int64_t close_y = (int64_t)scope->window_y + 6;
            if (scope->window_width > 0 &&
                scope->touch_x >= close_x - 14 &&
                scope->touch_x <= close_x + 42 &&
                scope->touch_y >= close_y - 10 &&
                scope->touch_y <= close_y + 42) {
                if (scope->host->play_beep != NULL) {
                    (void)scope->host->play_beep(1200U, 40U);
                }
                scope->app_running = 0;
                break;
            }

            for (int32_t i = 0; i < scope->button_count; ++i) {
                maia_legacy_host_button_t button = scope->buttons[i];
                int64_t right = (int64_t)button.x + button.width;
                int64_t bottom = (int64_t)button.y + button.height;
                if (scope->touch_x < button.x || scope->touch_x > right ||
                    scope->touch_y < button.y || scope->touch_y > bottom) {
                    continue;
                }
                if (scope->host->play_beep != NULL) {
                    (void)scope->host->play_beep(2200U, 30U);
                }
                legacy_draw_button(button.x, button.y, button.width,
                                   button.height, button.label, 0x04BF,
                                   0xFFFF, 1);
                scope->host->display->sync();
                if (button.callback != 0U) {
                    ((void (*)(void))button.callback)();
                }
                (void)scope->host->sleep_ms(80U);
                legacy_draw_button(button.x, button.y, button.width,
                                   button.height, button.label, 0x2169,
                                   0xFFFF, 0);
                scope->host->display->sync();
                break;
            }
        } else if (touch_result == 0 && scope->touch_pressed == 0) {
            was_touched = 0;
        }
        if (!scope->app_running) break;

        uint64_t now = scope->host->get_time_us != NULL
            ? scope->host->get_time_us() : last_tick + elapsed_without_clock;
        if (scope->tick_callback != 0U && now - last_tick >= 500000U) {
            last_tick = now;
            ((void (*)(void))scope->tick_callback)();
        }
        if (scope->host->get_time_us == NULL) elapsed_without_clock += 20000U;
        if (scope->host->sleep_ms(20U) < 0) {
            report_error(scope, "Legacy app delay failed\n");
            scope->app_running = 0;
            return -1;
        }
    }

    scope->button_count = 0;
    scope->key_callback = 0U;
    scope->tick_callback = 0U;
    scope->window_width = 0;
    scope->window_height = 0;
    return scope->exit_code;
}

int maia_legacy_host_scope_begin(maia_legacy_host_scope_t *scope,
                                 const maia_host_api_v1 *host) {
    if (scope == NULL) return -1;
    maia_legacy_host_scope_t *previous = s_active_scope;
    memset(scope, 0, sizeof(*scope));
    scope->host = host;
    scope->previous = previous;
    s_active_scope = scope;
    return 0;
}

int maia_legacy_host_scope_end(maia_legacy_host_scope_t *scope) {
    if (scope == NULL || s_active_scope != scope) return -1;
    int result = scope->graphics_error != 0U ? -1 : 0;
    s_active_scope = scope->previous;
    return result;
}

uintptr_t maia_legacy_host_resolve_symbol(const char *name) {
    if (name == NULL) return 0U;
    if (strcmp(name, "fill_rect") == 0) return (uintptr_t)legacy_fill_rect;
    if (strcmp(name, "display_sync") == 0) return (uintptr_t)legacy_display_sync;
    if (strcmp(name, "draw_text") == 0) return (uintptr_t)legacy_draw_text;
    if (strcmp(name, "draw_rect") == 0) return (uintptr_t)legacy_draw_rect;
    if (strcmp(name, "draw_line") == 0) return (uintptr_t)legacy_draw_line;
    if (strcmp(name, "draw_window") == 0) return (uintptr_t)legacy_draw_window;
    if (strcmp(name, "draw_button") == 0) return (uintptr_t)legacy_draw_button;
    if (strcmp(name, "host_bind_button") == 0) return (uintptr_t)legacy_bind_button;
    if (strcmp(name, "host_bind_key") == 0) return (uintptr_t)legacy_bind_key;
    if (strcmp(name, "host_bind_tick") == 0) return (uintptr_t)legacy_bind_tick;
    if (strcmp(name, "host_app_quit") == 0) return (uintptr_t)legacy_app_quit;
    if (strcmp(name, "host_app_run") == 0) return (uintptr_t)legacy_app_run;
    if (strcmp(name, "touch_poll") == 0) return (uintptr_t)legacy_touch_poll;
    if (strcmp(name, "touch_x") == 0) return (uintptr_t)legacy_touch_x;
    if (strcmp(name, "touch_y") == 0) return (uintptr_t)legacy_touch_y;
    if (strcmp(name, "powf") == 0) return (uintptr_t)powf;
    return 0U;
}
