#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t maia_handle_t;

#define MAIA_HOST_ABI_MAJOR 1
#define MAIA_HOST_ABI_MINOR 4

/**
 * @brief Directory entry structure for VFS navigation.
 */
typedef struct maia_dir_entry_t {
    char name[64];
    uint32_t size;
    uint8_t is_dir;
    uint8_t is_exec;
} maia_dir_entry_t;

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
    int32_t (*copy_file)(const char *src_path, const char *dst_path);
    int32_t (*apply_wallpaper)(const char *path);
    int32_t (*shell_exec)(const char *cmd, void (*out_cb)(const char *str, void *user_data), void *user_data);
    int32_t (*list_dir)(const char *path, maia_dir_entry_t *entries, uint32_t max_entries);
    int32_t (*remove_file)(const char *path);
    int32_t (*rename_file)(const char *old_path, const char *new_path);
    int32_t (*make_dir)(const char *path);
    int32_t (*read_text_file)(const char *path, char *buf, uint32_t max_len);
    int32_t (*write_text_file)(const char *path, const char *buf, uint32_t len);
    int32_t (*play_beep)(uint32_t freq_hz, uint32_t duration_ms);
    int32_t (*play_wav)(const char *path);
    int32_t (*record_wav)(const char *path, uint32_t duration_sec);
    int32_t (*set_volume)(uint32_t vol);
    int32_t (*get_volume)(void);
    int32_t (*is_usb_mounted)(void);
    int32_t (*usb_unmount)(void);
    int32_t (*wifi_scan)(char *buf, uint32_t max_len);
    int32_t (*wifi_connect)(const char *ssid, const char *pass);
    int32_t (*wifi_status)(char *buf, uint32_t max_len);
    int32_t (*wifi_disconnect)(void);
    int32_t (*sntp_sync)(const char *server);
    int32_t (*get_datetime)(char *buf, uint32_t max_len);
    int32_t (*camera_snap)(const char *path);
    int32_t (*camera_view)(uint32_t duration_sec);
    int32_t (*video_record)(const char *path, uint32_t duration_sec, uint32_t fps);
    int32_t (*video_play)(const char *path, uint32_t fps);
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

