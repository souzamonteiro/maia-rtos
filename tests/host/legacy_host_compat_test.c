#include <assert.h>
#include <math.h>
#include <stdint.h>
#include <string.h>

#include <maia/exec/legacy_host_compat.h>

typedef void (*draw_rect_fn)(int32_t, int32_t, int32_t, int32_t, int32_t);
typedef void (*draw_line_fn)(int32_t, int32_t, int32_t, int32_t, int32_t, int32_t);
typedef void (*draw_text_fn)(int32_t, int32_t, const char *, int32_t, int32_t);
typedef void (*draw_window_fn)(int32_t, int32_t, int32_t, int32_t,
                               const char *, int32_t);
typedef void (*draw_button_fn)(int32_t, int32_t, int32_t, int32_t,
                               const char *, int32_t, int32_t, int32_t);
typedef void (*bind_button_fn)(int32_t, int32_t, uintptr_t);
typedef void (*bind_callback_fn)(uintptr_t);
typedef int32_t (*app_run_fn)(void);
typedef void (*app_quit_fn)(int32_t);
typedef float (*powf_fn)(float, float);

static unsigned fill_calls;
static unsigned pixel_calls;
static unsigned sync_calls;
static unsigned acquire_calls;
static unsigned sleep_calls;
static unsigned button_calls;
static unsigned key_calls;
static unsigned touch_error_writes;
static unsigned graphics_error_writes;
static int fail_next_fill;
static uint64_t now_us;
static int32_t touch_index;

static int32_t fake_put_pixel(int32_t x, int32_t y, uint16_t color) {
    assert(x >= 0 && x < 1280 && y >= 0 && y < 720);
    (void)color;
    ++pixel_calls;
    return 0;
}

static int32_t fake_fill_rect(int32_t x, int32_t y, int32_t width,
                              int32_t height, uint16_t color) {
    assert(x >= 0 && x < 1280 && y >= 0 && y < 720);
    assert(width > 0 && height > 0);
    (void)color;
    if (fail_next_fill) {
        fail_next_fill = 0;
        return -1;
    }
    ++fill_calls;
    return 0;
}

static void fake_sync(void) {
    ++sync_calls;
}

static int32_t fake_acquire(void) {
    ++acquire_calls;
    return 0;
}

static int32_t fake_release(void) {
    return 0;
}

static int32_t fake_read_key(void) {
    return -1;
}

static int32_t fake_get_touch(int32_t *x, int32_t *y, int32_t *pressed) {
    int32_t sample = touch_index++;
    if (sample == 0) return -1;
    if (sample == 1) {
        *x = 0;
        *y = 0;
        *pressed = 0;
    } else {
        *x = 30;
        *y = 30;
        *pressed = 1;
    }
    return 0;
}

static int32_t fake_write_fd(maia_handle_t fd, const void *buf, uint32_t size) {
    static const char touch_message[] =
        "Legacy touch input unavailable; retaining prior state\n";
    static const char graphics_message[] =
        "Legacy graphics rectangle failed\n";
    if (fd == 2U && size == sizeof(touch_message) - 1U &&
        memcmp(buf, touch_message, sizeof(touch_message) - 1U) == 0) {
        ++touch_error_writes;
    } else if (fd == 2U && size == sizeof(graphics_message) - 1U &&
               memcmp(buf, graphics_message, sizeof(graphics_message) - 1U) == 0) {
        ++graphics_error_writes;
    }
    return (int32_t)size;
}

static int32_t fake_sleep_ms(uint32_t duration) {
    ++sleep_calls;
    now_us += (uint64_t)duration * 1000U;
    return 0;
}

static uint64_t fake_get_time_us(void) {
    return now_us;
}

static void button_callback(void) {
    ++button_calls;
    fail_next_fill = 1;
    ((draw_rect_fn)maia_legacy_host_resolve_symbol("draw_rect"))(
        5, 6, 20, 10, 0x07E0);
    ((app_quit_fn)maia_legacy_host_resolve_symbol("host_app_quit"))(7);
}

static void key_callback(int32_t key) {
    (void)key;
    ++key_calls;
}

int main(void) {
    static const maia_display_api_t display = {
        .width = 1280,
        .height = 720,
        .put_pixel = fake_put_pixel,
        .fill_rect = fake_fill_rect,
        .sync = fake_sync,
        .acquire_exclusive = fake_acquire,
        .release_exclusive = fake_release,
    };
    static const maia_input_api_t input = {
        .get_touch = fake_get_touch,
        .read_key = fake_read_key,
    };
    static const maia_host_api_v1 host = {
        .struct_size = sizeof(maia_host_api_v1),
        .abi_major = MAIA_HOST_ABI_MAJOR,
        .abi_minor = MAIA_HOST_ABI_MINOR,
        .write_fd = fake_write_fd,
        .sleep_ms = fake_sleep_ms,
        .get_time_us = fake_get_time_us,
        .display = &display,
        .input = &input,
    };
    maia_legacy_host_scope_t scope;
    assert(maia_legacy_host_scope_begin(&scope, &host) == 0);

    uintptr_t draw_rect_address = maia_legacy_host_resolve_symbol("draw_rect");
    uintptr_t fill_rect_address = maia_legacy_host_resolve_symbol("fill_rect");
    uintptr_t display_sync_address = maia_legacy_host_resolve_symbol("display_sync");
    uintptr_t draw_line_address = maia_legacy_host_resolve_symbol("draw_line");
    uintptr_t draw_text_address = maia_legacy_host_resolve_symbol("draw_text");
    uintptr_t draw_window_address = maia_legacy_host_resolve_symbol("draw_window");
    uintptr_t draw_button_address = maia_legacy_host_resolve_symbol("draw_button");
    uintptr_t bind_button_address = maia_legacy_host_resolve_symbol("host_bind_button");
    uintptr_t bind_key_address = maia_legacy_host_resolve_symbol("host_bind_key");
    uintptr_t app_run_address = maia_legacy_host_resolve_symbol("host_app_run");
    uintptr_t powf_address = maia_legacy_host_resolve_symbol("powf");
    assert(draw_rect_address != 0U && fill_rect_address != 0U);
    assert(display_sync_address != 0U && draw_line_address != 0U);
    assert(draw_text_address != 0U && draw_window_address != 0U);
    assert(draw_button_address != 0U && bind_button_address != 0U);
    assert(bind_key_address != 0U && app_run_address != 0U && powf_address != 0U);
    assert(maia_legacy_host_resolve_symbol("unknown_legacy_symbol") == 0U);

    ((draw_rect_fn)fill_rect_address)(1, 2, 3, 4, 0x1234);
    ((void (*)(void))display_sync_address)();
    ((draw_rect_fn)draw_rect_address)(5, 6, 20, 10, 0x07E0);
    ((draw_line_fn)draw_line_address)(-10, 12, 12, 12, 0xFFFF, 1);
    ((draw_text_fn)draw_text_address)(10, 30, "MNM", 0xFFFF, 0);
    ((draw_window_fn)draw_window_address)(10, 10, 200, 120, "Test", 0x07FF);
    ((bind_button_fn)bind_button_address)(20, 20, (uintptr_t)button_callback);
    ((draw_button_fn)draw_button_address)(20, 20, 40, 20, "OK",
                                            0x2169, 0xFFFF, 0);
    ((bind_callback_fn)bind_key_address)((uintptr_t)key_callback);
    assert(((powf_fn)powf_address)(2.0f, 3.0f) == 8.0f);
    assert(fill_calls > 0U && acquire_calls == 1U);

    assert(((app_run_fn)app_run_address)() == 7);
    assert(button_calls == 1U && sync_calls == 3U);
    assert(sleep_calls == 3U && pixel_calls == 0U && key_calls == 0U);
    assert(touch_error_writes == 1U);
    assert(graphics_error_writes == 1U);
    assert(maia_legacy_host_scope_end(&scope) == -1);

    maia_legacy_host_scope_t parent;
    maia_legacy_host_scope_t child;
    assert(maia_legacy_host_scope_begin(&parent, &host) == 0);
    assert(maia_legacy_host_scope_begin(&child, NULL) == 0);
    assert(maia_legacy_host_scope_end(&child) == 0);
    ((bind_callback_fn)maia_legacy_host_resolve_symbol("host_bind_key"))(
        (uintptr_t)key_callback);
    assert(maia_legacy_host_scope_end(&parent) == 0);
    return 0;
}
