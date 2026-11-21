#ifndef MAIA_SERVICES_VFS_H
#define MAIA_SERVICES_VFS_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAIA_VFS_PATH_MAX 255U
#define MAIA_VFS_MAX_OPEN_FILES 8U
#define MAIA_VFS_MAX_OPEN_DIRS 4U

typedef uint32_t maia_vfs_fd_t;
typedef uint32_t maia_vfs_dir_t;

typedef struct maia_vfs_info {
    uint32_t size;
    uint8_t is_directory;
} maia_vfs_info_t;

typedef struct maia_vfs_dirent {
    char name[MAIA_VFS_PATH_MAX + 1U];
    maia_vfs_info_t info;
} maia_vfs_dirent_t;

typedef enum maia_vfs_result {
    MAIA_VFS_OK = 0,
    MAIA_VFS_INVALID = -1,
    MAIA_VFS_BAD_CONTEXT = -2,
    MAIA_VFS_NOT_MOUNTED = -3,
    MAIA_VFS_NOT_FOUND = -4,
    MAIA_VFS_EXISTS = -5,
    MAIA_VFS_ACCESS = -6,
    MAIA_VFS_BUSY = -7,
    MAIA_VFS_NO_HANDLES = -8,
    MAIA_VFS_NO_MEMORY = -9,
    MAIA_VFS_IO_ERROR = -10,
    MAIA_VFS_FAILURE = -11,
    MAIA_VFS_DENIED = -12,
    MAIA_VFS_IS_DIRECTORY = -13,
    MAIA_VFS_NOT_DIRECTORY = -14
} maia_vfs_result_t;

#define MAIA_VFS_OPEN_READ       0x0001U
#define MAIA_VFS_OPEN_WRITE      0x0002U
#define MAIA_VFS_OPEN_CREATE     0x0004U
#define MAIA_VFS_OPEN_CREATE_NEW 0x0008U
#define MAIA_VFS_OPEN_TRUNCATE   0x0010U
#define MAIA_VFS_OPEN_APPEND     0x0020U

/*
 * The only mounted path is /sd. File calls are thread-context-only, use fixed
 * storage, and fail immediately with BUSY if another task owns the VFS lock.
 * The caller decides whether and when to retry; this API never polls or yields.
 * Mount/unmount require quiescence with no open file or directory handles.
 */
/*
 * Paths must name a file below /sd; empty, dot and parent components are
 * rejected. CREATE preserves an existing file, CREATE_NEW never overwrites,
 * and TRUNCATE is valid only with CREATE. APPEND positions each write at EOF.
 * Reads at EOF succeed with zero bytes; successful reads/writes may be partial.
 * A failed close leaves the descriptor valid so the caller can retry.
 */
maia_vfs_result_t maia_vfs_open(const char *path, uint32_t flags,
                                maia_vfs_fd_t *out_fd);
maia_vfs_result_t maia_vfs_read(maia_vfs_fd_t fd, void *buffer,
                                uint32_t count, uint32_t *out_read);
maia_vfs_result_t maia_vfs_write(maia_vfs_fd_t fd, const void *buffer,
                                 uint32_t count, uint32_t *out_written);
maia_vfs_result_t maia_vfs_seek(maia_vfs_fd_t fd, uint32_t offset);
maia_vfs_result_t maia_vfs_sync(maia_vfs_fd_t fd);
maia_vfs_result_t maia_vfs_close(maia_vfs_fd_t fd);
maia_vfs_result_t maia_vfs_unlink(const char *path);
/*
 * Metadata/directory calls follow the same context and BUSY contract.
 * /sd and /sd/ name the mount root; other paths retain the file path rules.
 * Directory EOF is OK with out_has_entry=0. Returned entries are caller-owned.
 * file_info snapshots size and position together; it does not perform I/O.
 */
maia_vfs_result_t maia_vfs_stat(const char *path, maia_vfs_info_t *out_info);
maia_vfs_result_t maia_vfs_file_info(maia_vfs_fd_t fd,
                                     maia_vfs_info_t *out_info,
                                     uint32_t *out_position);
maia_vfs_result_t maia_vfs_mkdir(const char *path);
maia_vfs_result_t maia_vfs_rmdir(const char *path);
maia_vfs_result_t maia_vfs_opendir(const char *path, maia_vfs_dir_t *out_dir);
maia_vfs_result_t maia_vfs_readdir(maia_vfs_dir_t dir,
                                   maia_vfs_dirent_t *out_entry,
                                   int *out_has_entry);
maia_vfs_result_t maia_vfs_closedir(maia_vfs_dir_t dir);

#ifdef __cplusplus
}
#endif

#endif
