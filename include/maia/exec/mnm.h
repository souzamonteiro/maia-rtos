#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t maia_handle_t;

#define MAIA_HOST_ABI_MAJOR 1
#define MAIA_HOST_ABI_MINOR 1

/**
 * @brief Display primitive driver callbacks for native modules.
 */
typedef struct maia_display_api_t {
    int32_t width;
    int32_t height;
    int32_t (*put_pixel)(int32_t x, int32_t y, uint16_t color_rgb565);
    int32_t (*fill_rect)(int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color_rgb565);
    void (*sync)(void);
    int32_t (*acquire_exclusive)(void);
    int32_t (*release_exclusive)(void);
    uint16_t *(*get_framebuffer)(void);
} maia_display_api_t;

/**
 * @brief Input driver callbacks for native modules (Touchscreen & Keyboard).
 */
typedef struct maia_input_api_t {
    int32_t (*get_touch)(int32_t *x, int32_t *y, int32_t *pressed);
    int32_t (*read_key)(void); /* Non-blocking: returns char code (0..255) or -1 if empty */
} maia_input_api_t;

/**
 * @brief Maia Native Module ABI v1 (MNM-1) Host API Dispatch Table.
 * Modules use this table to interact with the host environment.
 */
typedef struct maia_host_api_v1 {
    uint32_t struct_size;
    uint16_t abi_major;
    uint16_t abi_minor;
    int32_t (*write_fd)(maia_handle_t fd, const void *buf, uint32_t n);
    int32_t (*read_fd)(maia_handle_t fd, void *buf, uint32_t n);
    int32_t (*close_fd)(maia_handle_t fd);
    int32_t (*sleep_ms)(uint32_t ms);
    void *(*alloc)(uint32_t bytes, uint32_t flags);
    void (*dealloc)(void *ptr);
    uint64_t (*get_time_us)(void);
    const maia_display_api_t *display;
    const maia_input_api_t *input;
} maia_host_api_v1;

/**
 * @brief Context passed to maia_module_main entry point.
 */
typedef struct maia_module_context_t {
    uint32_t struct_size;
    uint32_t argc;
    const char *const *argv;
    const maia_host_api_v1 *host;
    maia_handle_t stdin_fd, stdout_fd, stderr_fd;
    void *module_private;
} maia_module_context_t;

/**
 * @brief Mandatory entry symbol signature for trusted Maia Native Modules.
 */
typedef int (*maia_module_entry_fn)(const maia_module_context_t *ctx);

#ifdef __cplusplus
}
#endif

