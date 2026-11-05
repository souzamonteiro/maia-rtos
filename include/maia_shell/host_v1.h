#ifndef MAIA_SHELL_HOST_V1_H
#define MAIA_SHELL_HOST_V1_H

/*
 * MaiaShellHost v1 — Portable Host Interface Contract (DESIGN / HOST-TESTED)
 *
 * Isolates the portable MaiaShell parser/AST/executor from the underlying
 * execution environment (MaiaRTOS Native, MaiaOS WASM, or Host Test Harness).
 *
 * Concurrency & Context Contract:
 * - Execution Context: THREAD CONTEXT ONLY. No callback or helper in this
 *   header may be invoked from an Interrupt Service Routine (ISR).
 * - Reentrancy: A single msh_host_v1 instance is bound to one shell session.
 *   Concurrent sessions require distinct host context instances (`user`).
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Interface version packed as (MAJOR << 16) | MINOR. */
#define MSH_HOST_V1_VERSION_MAJOR 1U
#define MSH_HOST_V1_VERSION_MINOR 0U
#define MSH_HOST_V1_VERSION       ((MSH_HOST_V1_VERSION_MAJOR << 16U) | MSH_HOST_V1_VERSION_MINOR)

/** Standard stream descriptor numbers within a shell session namespace. */
#define MSH_FD_STDIN  ((msh_fd_t)0U)
#define MSH_FD_STDOUT ((msh_fd_t)1U)
#define MSH_FD_STDERR ((msh_fd_t)2U)
#define MSH_FD_INVALID ((msh_fd_t)0xFFFFFFFFU)

/** Invalid job handle sentinel. */
#define MSH_JOB_INVALID ((msh_job_t)0U)

/**
 * Stable negative error codes aligned with observed MaiaShell VFS/stream
 * conventions (`maiaos/apps/shell/shell.cpp`):
 *  -1: invalid path, argument, or entry
 *  -2: no such file or directory
 *  -3: permission denied
 *  -4: descriptor/resource limit exceeded
 *  -6: non-blocking operation would block (yield and retry)
 *  -7: endpoint unbound / idle
 *  -9: not a directory
 */
typedef enum msh_status_e {
    MSH_OK          =  0,
    MSH_ERR_INVALID = -1,
    MSH_ERR_NOENT   = -2,
    MSH_ERR_PERM    = -3,
    MSH_ERR_LIMIT   = -4,
    MSH_ERR_IO      = -5,
    MSH_ERR_AGAIN   = -6,
    MSH_ERR_UNBOUND = -7,
    MSH_ERR_NOMEM   = -8,
    MSH_ERR_NOTDIR  = -9,
    MSH_ERR_NOSYS   = -10,
    MSH_ERR_ABI     = -11
} msh_status_t;

/**
 * Open flags aligned with observed MaiaShell redirection flags
 * (`shell.cpp:27, 520-522`: 1 = read `<`, 14 = write/create/trunc `>`,
 * 22 = write/create/append `>>`).
 */
typedef enum msh_open_flags_e {
    MSH_O_READ   = 1U << 0U, /* 0x01 */
    MSH_O_WRITE  = 1U << 1U, /* 0x02 */
    MSH_O_CREAT  = 1U << 2U, /* 0x04 */
    MSH_O_TRUNC  = 1U << 3U, /* 0x08 (WRITE|CREAT|TRUNC = 14) */
    MSH_O_APPEND = 1U << 4U  /* 0x10 (WRITE|CREAT|APPEND = 22) */
} msh_open_flags_t;

/** Directory entry types aligned with `shell-glob.hpp:40-49` (`1=file, 2=dir`). */
typedef enum msh_entry_type_e {
    MSH_ENTRY_FILE = 1U,
    MSH_ENTRY_DIR  = 2U
} msh_entry_type_t;

typedef uint32_t msh_fd_t;
typedef uint32_t msh_job_t;

/**
 * Sink descriptor passed to `msh_host_v1::glob`.
 *
 * Ownership: Caller owns `msh_glob_sink_t` and `ctx` for the duration of the
 * `glob` call. The host invokes `on_match` for each matching path.
 * Return 0 from `on_match` to continue enumeration, or a negative `msh_status_t`
 * (e.g., `MSH_ERR_LIMIT`) to abort early.
 */
typedef struct msh_glob_sink_t {
    void *ctx;
    int32_t (*on_match)(void *ctx, const char *path, uint32_t length, uint32_t entry_type);
} msh_glob_sink_t;

/**
 * Portable MaiaShellHost v1 dispatch table.
 *
 * Ownership: Caller retains ownership of the `msh_host_v1` struct and `user`
 * pointer. All pointers passed to callbacks are borrowed for the duration of
 * the call only.
 * Context: Thread context only (NEVER call from ISR).
 */
typedef struct msh_host_v1 {
    uint32_t struct_size;
    uint32_t version;
    void *user;

    /* Stream & VFS descriptor I/O: return >= 0 byte count or negative msh_status_t */
    int32_t (*read)(void *user, msh_fd_t fd, void *dst, uint32_t n);
    int32_t (*write)(void *user, msh_fd_t fd, const void *src, uint32_t n);
    int32_t (*open)(void *user, const char *path, uint32_t flags, msh_fd_t *out);
    int32_t (*close)(void *user, msh_fd_t fd);

    /* Pipe & descriptor duplication for redirections and multi-stage pipelines */
    int32_t (*pipe_create)(void *user, msh_fd_t *read_end, msh_fd_t *write_end);
    int32_t (*fd_dup)(void *user, msh_fd_t src, msh_fd_t *dst);

    /* External command / module task-group spawn and completion wait */
    int32_t (*spawn)(void *user, const char *path, const char *const *argv,
                     msh_fd_t in, msh_fd_t out, msh_fd_t err, msh_job_t *job);
    int32_t (*wait)(void *user, msh_job_t job, int32_t *status);

    /* Working directory query and update */
    int32_t (*cwd_get)(void *user, char *dst, uint32_t cap);
    int32_t (*cwd_set)(void *user, const char *path);

    /* Pathname expansion / directory enumeration callback (`sink` is `msh_glob_sink_t *`) */
    int32_t (*glob)(void *user, const char *pattern, void *sink);
} msh_host_v1;

/**
 * Validate a host dispatch table before binding it to a shell session.
 *
 * Context: Thread context only (pure validation, non-blocking).
 * Returns:
 * - `MSH_OK` (0) if `host` is non-NULL, `struct_size == sizeof(msh_host_v1)`,
 *   major version matches `MSH_HOST_V1_VERSION_MAJOR`, and all mandatory
 *   function pointers are non-NULL.
 * - `MSH_ERR_INVALID` (-1) if `host` or any required callback pointer is NULL.
 * - `MSH_ERR_ABI` (-11) if `struct_size` or major version is incompatible.
 */
#if defined(__GNUC__) || defined(__clang__)
#define MSH_MAYBE_UNUSED __attribute__((unused))
#else
#define MSH_MAYBE_UNUSED
#endif

static inline MSH_MAYBE_UNUSED int32_t msh_host_v1_validate(const msh_host_v1 *host) {
    if (host == NULL) {
        return MSH_ERR_INVALID;
    }
    if (host->struct_size != (uint32_t)sizeof(msh_host_v1)) {
        return MSH_ERR_ABI;
    }
    if ((host->version >> 16U) != MSH_HOST_V1_VERSION_MAJOR) {
        return MSH_ERR_ABI;
    }
    if (host->read == NULL ||
        host->write == NULL ||
        host->open == NULL ||
        host->close == NULL ||
        host->pipe_create == NULL ||
        host->fd_dup == NULL ||
        host->spawn == NULL ||
        host->wait == NULL ||
        host->cwd_get == NULL ||
        host->cwd_set == NULL ||
        host->glob == NULL) {
        return MSH_ERR_INVALID;
    }
    return MSH_OK;
}

#ifdef __cplusplus
}
#endif

#endif /* MAIA_SHELL_HOST_V1_H */
