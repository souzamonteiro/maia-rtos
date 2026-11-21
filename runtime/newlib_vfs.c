#define _DEFAULT_SOURCE
#define _POSIX_C_SOURCE 200809L

#include "newlib_vfs.h"

#include <maia/kernel.h>
#include <maia/services/vfs.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define FILE_FD_BASE 3

typedef struct newlib_directory {
    maia_vfs_dir_t handle;
    struct dirent entry;
} newlib_directory_t;

static maia_mutex_t g_adapter_mutex = { 0U, -1, 0U };
static maia_vfs_fd_t g_fds[MAIA_VFS_MAX_OPEN_FILES];
static newlib_directory_t g_dirs[MAIA_VFS_MAX_OPEN_DIRS];

static int fail(int error) {
    errno = error;
    return -1;
}

static int result_errno(maia_vfs_result_t result) {
    switch (result) {
    case MAIA_VFS_OK: return 0;
    case MAIA_VFS_INVALID: return EINVAL;
    case MAIA_VFS_BAD_CONTEXT: return ENOTSUP;
    case MAIA_VFS_NOT_MOUNTED: return ENODEV;
    case MAIA_VFS_NOT_FOUND: return ENOENT;
    case MAIA_VFS_EXISTS: return EEXIST;
    case MAIA_VFS_ACCESS:
    case MAIA_VFS_DENIED: return EACCES;
    case MAIA_VFS_BUSY: return EBUSY;
    case MAIA_VFS_NO_HANDLES: return EMFILE;
    case MAIA_VFS_NO_MEMORY: return ENOMEM;
    case MAIA_VFS_IS_DIRECTORY: return EISDIR;
    case MAIA_VFS_NOT_DIRECTORY: return ENOTDIR;
    default: return EIO;
    }
}

static int lock_adapter(maia_task_id_t *task) {
    *task = maia_task_current_id();
    if (*task < 0) return fail(ENOTSUP);
    maia_status_t result = maia_mutex_trylock(&g_adapter_mutex, *task);
    if (result != MAIA_OK) return fail(result == MAIA_ERR_BUSY ? EBUSY : EIO);
    return 0;
}

static int unlock_adapter(maia_task_id_t task, int result) {
    if (maia_mutex_unlock(&g_adapter_mutex, task) != MAIA_OK) return fail(EIO);
    return result;
}

static maia_vfs_fd_t find_fd(int fd) {
    if (fd < FILE_FD_BASE ||
        (unsigned)(fd - FILE_FD_BASE) >= MAIA_VFS_MAX_OPEN_FILES) return 0U;
    return g_fds[fd - FILE_FD_BASE];
}

static int fill_stat(const maia_vfs_info_t *info, struct stat *st) {
    if (info->size > INT32_MAX) return fail(EOVERFLOW);
    memset(st, 0, sizeof(*st));
    st->st_mode = (info->is_directory ? S_IFDIR : S_IFREG) | 0777;
    st->st_size = (off_t)info->size;
    st->st_nlink = 1;
    st->st_blksize = 512;
    return 0;
}

int _open(const char *path, int flags, int mode) {
    (void)mode;
    int supported = O_ACCMODE | O_CREAT | O_EXCL | O_TRUNC | O_APPEND;
#ifdef O_BINARY
    supported |= O_BINARY;
#endif
    if ((flags & ~supported) != 0) return fail(ENOTSUP);
    int access = flags & O_ACCMODE;
    if (access != O_RDONLY && access != O_WRONLY && access != O_RDWR)
        return fail(EINVAL);
    if ((flags & O_EXCL) != 0 && (flags & O_CREAT) == 0) return fail(EINVAL);
    if (((flags & O_TRUNC) != 0 && (flags & O_CREAT) == 0) ||
        (access == O_RDONLY && (flags & (O_CREAT | O_TRUNC | O_APPEND)) != 0))
        return fail(ENOTSUP);

    uint32_t vflags = access == O_RDONLY ? MAIA_VFS_OPEN_READ :
                      access == O_WRONLY ? MAIA_VFS_OPEN_WRITE :
                      MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE;
    if ((flags & O_CREAT) != 0) {
        vflags |= (flags & O_EXCL) != 0 ? MAIA_VFS_OPEN_CREATE_NEW :
                                        MAIA_VFS_OPEN_CREATE;
    }
    if ((flags & O_TRUNC) != 0 && (flags & O_EXCL) == 0)
        vflags |= MAIA_VFS_OPEN_TRUNCATE;
    if ((flags & O_APPEND) != 0) vflags |= MAIA_VFS_OPEN_APPEND;

    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return -1;
    maia_vfs_info_t info;
    maia_vfs_result_t status = maia_vfs_stat(path, &info);
    if (status == MAIA_VFS_OK && (flags & O_EXCL) != 0)
        return unlock_adapter(task, fail(EEXIST));
    if (status == MAIA_VFS_OK && info.is_directory)
        return unlock_adapter(task, fail(EISDIR));
    if (status != MAIA_VFS_OK && status != MAIA_VFS_NOT_FOUND)
        return unlock_adapter(task, fail(result_errno(status)));
    for (unsigned i = 0U; i < MAIA_VFS_MAX_OPEN_FILES; ++i) {
        if (g_fds[i] != 0U) continue;
        status = maia_vfs_open(path, vflags, &g_fds[i]);
        return unlock_adapter(task, status == MAIA_VFS_OK ?
                              (int)i + FILE_FD_BASE : fail(result_errno(status)));
    }
    return unlock_adapter(task, fail(EMFILE));
}

int _close(int fd) {
    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return -1;
    maia_vfs_fd_t handle = find_fd(fd);
    if (handle == 0U) return unlock_adapter(task, fail(EBADF));
    maia_vfs_result_t status = maia_vfs_close(handle);
    if (status == MAIA_VFS_OK) g_fds[fd - FILE_FD_BASE] = 0U;
    return unlock_adapter(task, status == MAIA_VFS_OK ? 0 : fail(result_errno(status)));
}

static int transfer(int fd, char *read_buffer, const char *write_buffer,
                    int length, int writing) {
    if (length < 0) return fail(EINVAL);
    if (length != 0 && (writing ? write_buffer == NULL : read_buffer == NULL))
        return fail(EFAULT);
    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return -1;
    maia_vfs_fd_t handle = find_fd(fd);
    if (handle == 0U) return unlock_adapter(task, fail(EBADF));
    uint32_t count = 0U;
    maia_vfs_result_t status = writing ?
        maia_vfs_write(handle, write_buffer, (uint32_t)length, &count) :
        maia_vfs_read(handle, read_buffer, (uint32_t)length, &count);
    if (status != MAIA_VFS_OK) {
        errno = status == MAIA_VFS_ACCESS ? EBADF : result_errno(status);
    }
    return unlock_adapter(task, status == MAIA_VFS_OK || count != 0U ?
                          (int)count : -1);
}

int maia_newlib_vfs_read(int fd, char *buffer, int length) {
    return transfer(fd, buffer, NULL, length, 0);
}

int maia_newlib_vfs_write(int fd, const char *buffer, int length) {
    return transfer(fd, NULL, buffer, length, 1);
}

int fsync(int fd) {
    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return -1;
    maia_vfs_fd_t handle = find_fd(fd);
    maia_vfs_result_t status = handle == 0U ? MAIA_VFS_INVALID :
                              maia_vfs_sync(handle);
    return unlock_adapter(task, status == MAIA_VFS_OK ? 0 :
                          fail(handle == 0U ? EBADF : result_errno(status)));
}

off_t _lseek(int fd, off_t offset, int whence) {
    if (fd >= STDIN_FILENO && fd <= STDERR_FILENO) return (off_t)fail(ESPIPE);
    if (whence != SEEK_SET && whence != SEEK_CUR && whence != SEEK_END)
        return (off_t)fail(EINVAL);
    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return (off_t)-1;
    maia_vfs_fd_t handle = find_fd(fd);
    if (handle == 0U) return (off_t)unlock_adapter(task, fail(EBADF));
    maia_vfs_info_t info;
    uint32_t position;
    maia_vfs_result_t status = maia_vfs_file_info(handle, &info, &position);
    if (status != MAIA_VFS_OK)
        return (off_t)unlock_adapter(task, fail(result_errno(status)));
    int64_t base = whence == SEEK_CUR ? position : whence == SEEK_END ? info.size : 0U;
    /* Newlib on this target uses signed 32-bit off_t. Check before addition. */
    if (offset < -base) return (off_t)unlock_adapter(task, fail(EINVAL));
    if (offset > INT32_MAX - base) return (off_t)unlock_adapter(task, fail(EOVERFLOW));
    status = maia_vfs_seek(handle, (uint32_t)(base + offset));
    if (status == MAIA_VFS_OK) status = maia_vfs_file_info(handle, &info, &position);
    return (off_t)unlock_adapter(task, status == MAIA_VFS_OK ?
                                (int)position : fail(result_errno(status)));
}

int maia_newlib_vfs_fstat(int fd, struct stat *st) {
    if (st == NULL) return fail(EFAULT);
    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return -1;
    maia_vfs_fd_t handle = find_fd(fd);
    if (handle == 0U) return unlock_adapter(task, fail(EBADF));
    maia_vfs_info_t info;
    uint32_t position;
    maia_vfs_result_t status = maia_vfs_file_info(handle, &info, &position);
    return unlock_adapter(task, status == MAIA_VFS_OK ?
                          fill_stat(&info, st) : fail(result_errno(status)));
}

int _stat(const char *path, struct stat *st) {
    if (st == NULL) return fail(EFAULT);
    maia_vfs_info_t info;
    maia_vfs_result_t status = maia_vfs_stat(path, &info);
    return status == MAIA_VFS_OK ? fill_stat(&info, st) : fail(result_errno(status));
}

int _unlink(const char *path) {
    maia_vfs_result_t status = maia_vfs_unlink(path);
    return status == MAIA_VFS_OK ? 0 : fail(result_errno(status));
}

int mkdir(const char *path, mode_t mode) {
    (void)mode;
    maia_vfs_result_t status = maia_vfs_mkdir(path);
    return status == MAIA_VFS_OK ? 0 : fail(result_errno(status));
}

int rmdir(const char *path) {
    maia_vfs_result_t status = maia_vfs_rmdir(path);
    return status == MAIA_VFS_OK ? 0 : fail(result_errno(status));
}

static newlib_directory_t *find_directory(DIR *directory) {
    for (unsigned i = 0U; i < MAIA_VFS_MAX_OPEN_DIRS; ++i) {
        if (directory == (DIR *)&g_dirs[i] && g_dirs[i].handle != 0U)
            return &g_dirs[i];
    }
    return NULL;
}

DIR *opendir(const char *path) {
    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return NULL;
    maia_vfs_info_t info;
    maia_vfs_result_t status = maia_vfs_stat(path, &info);
    if (status != MAIA_VFS_OK || !info.is_directory) {
        (void)unlock_adapter(task, fail(status == MAIA_VFS_OK ? ENOTDIR : result_errno(status)));
        return NULL;
    }
    for (unsigned i = 0U; i < MAIA_VFS_MAX_OPEN_DIRS; ++i) {
        if (g_dirs[i].handle != 0U) continue;
        status = maia_vfs_opendir(path, &g_dirs[i].handle);
        if (unlock_adapter(task, status == MAIA_VFS_OK ? 0 :
                           fail(result_errno(status))) != 0) return NULL;
        return (DIR *)&g_dirs[i];
    }
    (void)unlock_adapter(task, fail(EMFILE));
    return NULL;
}

struct dirent *readdir(DIR *directory) {
    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return NULL;
    newlib_directory_t *slot = find_directory(directory);
    if (slot == NULL) {
        (void)unlock_adapter(task, fail(EBADF));
        return NULL;
    }
    maia_vfs_dirent_t entry;
    int has_entry;
    maia_vfs_result_t status = maia_vfs_readdir(slot->handle, &entry, &has_entry);
    if (status != MAIA_VFS_OK) {
        (void)unlock_adapter(task, fail(result_errno(status)));
        return NULL;
    }
    if (has_entry) {
        size_t length = strlen(entry.name);
        if (length >= sizeof(slot->entry.d_name)) {
            (void)unlock_adapter(task, fail(ENAMETOOLONG));
            return NULL;
        }
        memset(&slot->entry, 0, sizeof(slot->entry));
        memcpy(slot->entry.d_name, entry.name, length + 1U);
#ifdef DT_DIR
        slot->entry.d_type = entry.info.is_directory ? DT_DIR : DT_REG;
#endif
    }
    if (unlock_adapter(task, 0) != 0) return NULL;
    return has_entry ? &slot->entry : NULL;
}

int closedir(DIR *directory) {
    maia_task_id_t task;
    if (lock_adapter(&task) != 0) return -1;
    newlib_directory_t *slot = find_directory(directory);
    if (slot == NULL) return unlock_adapter(task, fail(EBADF));
    maia_vfs_result_t status = maia_vfs_closedir(slot->handle);
    if (status == MAIA_VFS_OK) slot->handle = 0U;
    return unlock_adapter(task, status == MAIA_VFS_OK ? 0 : fail(result_errno(status)));
}
