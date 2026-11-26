#ifndef MAIA_EXEC_LEGACY_HOST_COMPAT_H
#define MAIA_EXEC_LEGACY_HOST_COMPAT_H

#include <stdint.h>

#include <maia/exec/mnm.h>

#define MAIA_LEGACY_HOST_MAX_BUTTONS 32U

typedef struct {
    int32_t x;
    int32_t y;
    int32_t width;
    int32_t height;
    const char *label;
    uintptr_t callback;
} maia_legacy_host_button_t;

typedef struct maia_legacy_host_scope {
    const maia_host_api_v1 *host;
    struct maia_legacy_host_scope *previous;
    maia_legacy_host_button_t buttons[MAIA_LEGACY_HOST_MAX_BUTTONS];
    uintptr_t key_callback;
    uintptr_t tick_callback;
    int32_t button_count;
    int32_t window_x;
    int32_t window_y;
    int32_t window_width;
    int32_t window_height;
    int32_t touch_x;
    int32_t touch_y;
    int32_t touch_pressed;
    int32_t app_running;
    int32_t exit_code;
    uint8_t graphics_acquired;
    uint8_t graphics_error;
    uint8_t error_reported;
    uint8_t touch_unavailable_reported;
} maia_legacy_host_scope_t;

int maia_legacy_host_scope_begin(maia_legacy_host_scope_t *scope,
                                 const maia_host_api_v1 *host);
int maia_legacy_host_scope_end(maia_legacy_host_scope_t *scope);
uintptr_t maia_legacy_host_resolve_symbol(const char *name);

#endif
