#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t maia_handle_t;

#define MAIA_HOST_ABI_MAJOR 1
#define MAIA_HOST_ABI_MINOR 0

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

