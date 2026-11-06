#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t msh_fd_t;
typedef uint32_t msh_job_t;

#define MSH_HOST_VERSION_1 0x00010000U

/* Standard file descriptor constants for MaiaShellHost */
#define MSH_STDIN_FD  0U
#define MSH_STDOUT_FD 1U
#define MSH_STDERR_FD 2U

/* Open flags */
#define MSH_O_RDONLY 0x0001U
#define MSH_O_WRONLY 0x0002U
#define MSH_O_RDWR   0x0003U
#define MSH_O_CREAT  0x0100U
#define MSH_O_TRUNC  0x0200U
#define MSH_O_APPEND 0x0400U

/* Error codes */
#define MSH_OK            0
#define MSH_ERR_INVALID  -1
#define MSH_ERR_NOTFOUND -2
#define MSH_ERR_NOMEM    -3
#define MSH_ERR_IO       -4
#define MSH_ERR_NOTSUP   -5

/**
 * @brief Portable MaiaShellHost v1 interface.
 * Decouples the shell parser/execution engine from underlying OS/loader/VFS.
 */
typedef struct msh_host_v1 {
    uint32_t struct_size;
    uint32_t version;
    void *user;
    int32_t (*read)(void *user, msh_fd_t fd, void *dst, uint32_t n);
    int32_t (*write)(void *user, msh_fd_t fd, const void *src, uint32_t n);
    int32_t (*open)(void *user, const char *path, uint32_t flags, msh_fd_t *out);
    int32_t (*close)(void *user, msh_fd_t fd);
    int32_t (*pipe_create)(void *user, msh_fd_t *read_end, msh_fd_t *write_end);
    int32_t (*fd_dup)(void *user, msh_fd_t src, msh_fd_t *dst);
    int32_t (*spawn)(void *user, const char *path, const char *const *argv,
                     msh_fd_t in, msh_fd_t out, msh_fd_t err, msh_job_t *job);
    int32_t (*wait)(void *user, msh_job_t job, int32_t *status);
    int32_t (*cwd_get)(void *user, char *dst, uint32_t cap);
    int32_t (*cwd_set)(void *user, const char *path);
    int32_t (*glob)(void *user, const char *pattern, void *sink);
} msh_host_v1;

#ifdef __cplusplus
}
#endif
