#include <maia/kernel.h>
#include <maia/services/fatfs.h>
#include <maia/services/vfs.h>

#include "ff.h"

#include <stddef.h>
#include <stdint.h>

#define VFS_SLOT_BITS 3U
#define VFS_SLOT_MASK (MAIA_VFS_MAX_OPEN_FILES - 1U)
#define VFS_GENERATION_MAX 0x0FFFFFFFU
#define VFS_VALID_FLAGS (MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE | \
                         MAIA_VFS_OPEN_CREATE | MAIA_VFS_OPEN_CREATE_NEW | \
                         MAIA_VFS_OPEN_TRUNCATE | MAIA_VFS_OPEN_APPEND)

_Static_assert(MAIA_VFS_MAX_OPEN_FILES == 8U,
               "VFS handle encoding requires eight file slots");
_Static_assert(FF_FS_LOCK == MAIA_VFS_MAX_OPEN_FILES,
               "FatFs share-lock capacity must match VFS file slots");

typedef struct maia_vfs_file {
    FIL file;
    uint32_t generation;
    uint32_t flags;
    uint8_t open;
} maia_vfs_file_t;

static maia_mutex_t g_vfs_mutex = { 0U, -1, 0U };
static maia_vfs_file_t g_files[MAIA_VFS_MAX_OPEN_FILES];

static maia_vfs_result_t map_fatfs_result(FRESULT result) {
    switch (result) {
    case FR_OK:
        return MAIA_VFS_OK;
    case FR_NO_FILE:
    case FR_NO_PATH:
        return MAIA_VFS_NOT_FOUND;
    case FR_EXIST:
        return MAIA_VFS_EXISTS;
    case FR_DENIED:
        return MAIA_VFS_DENIED;
    case FR_WRITE_PROTECTED:
        return MAIA_VFS_ACCESS;
    case FR_NOT_READY:
    case FR_NOT_ENABLED:
    case FR_NO_FILESYSTEM:
        return MAIA_VFS_NOT_MOUNTED;
    case FR_TIMEOUT:
    case FR_LOCKED:
        return MAIA_VFS_BUSY;
    case FR_TOO_MANY_OPEN_FILES:
        return MAIA_VFS_NO_HANDLES;
    case FR_NOT_ENOUGH_CORE:
        return MAIA_VFS_NO_MEMORY;
    case FR_DISK_ERR:
    case FR_INT_ERR:
        return MAIA_VFS_IO_ERROR;
    case FR_INVALID_NAME:
    case FR_INVALID_DRIVE:
    case FR_INVALID_PARAMETER:
        return MAIA_VFS_INVALID;
    case FR_INVALID_OBJECT:
    default:
        return MAIA_VFS_FAILURE;
    }
}

static int is_dot_component(const char *path, size_t start, size_t length) {
    return (length == 1U && path[start] == '.') ||
           (length == 2U && path[start] == '.' && path[start + 1U] == '.');
}

static maia_vfs_result_t make_fatfs_path(const char *path,
                                         char out_path[MAIA_VFS_PATH_MAX + 1U]) {
    size_t length = 0U;
    size_t component_start = 4U;
    size_t component_length = 0U;

    if (path == NULL || out_path == NULL) return MAIA_VFS_INVALID;
    while (length <= MAIA_VFS_PATH_MAX && path[length] != '\0') ++length;
    if (length > MAIA_VFS_PATH_MAX || length < 5U ||
        path[0] != '/' || path[1] != 's' || path[2] != 'd' ||
        path[3] != '/') {
        return MAIA_VFS_INVALID;
    }

    out_path[0] = '0';
    out_path[1] = ':';
    out_path[2] = '/';
    for (size_t i = 4U; i < length; ++i) {
        unsigned char ch = (unsigned char)path[i];
        if (ch == '/' || ch == '\0') {
            if (component_length == 0U ||
                is_dot_component(path, component_start, component_length)) {
                return MAIA_VFS_INVALID;
            }
            if (ch == '/') {
                component_start = i + 1U;
                component_length = 0U;
            }
        } else {
            if (ch == '\\' || ch == ':' || ch < 0x20U) {
                return MAIA_VFS_INVALID;
            }
            ++component_length;
        }
        out_path[i - 1U] = (char)ch;
    }

    if (component_length == 0U ||
        is_dot_component(path, component_start, component_length)) {
        return MAIA_VFS_INVALID;
    }
    out_path[length - 1U] = '\0';
    return MAIA_VFS_OK;
}

static maia_vfs_result_t lock_vfs(int32_t *out_task_id) {
    maia_task_id_t task_id = maia_task_current_id();
    if (task_id < 0) return MAIA_VFS_BAD_CONTEXT;
    maia_status_t status = maia_mutex_trylock(&g_vfs_mutex, task_id);
    if (status == MAIA_ERR_BUSY) return MAIA_VFS_BUSY;
    if (status != MAIA_OK) return MAIA_VFS_FAILURE;
    *out_task_id = task_id;
    return MAIA_VFS_OK;
}

static maia_vfs_result_t unlock_vfs(int32_t task_id,
                                    maia_vfs_result_t result) {
    if (maia_mutex_unlock(&g_vfs_mutex, task_id) != MAIA_OK) {
        return MAIA_VFS_FAILURE;
    }
    return result;
}

static maia_vfs_file_t *find_file(maia_vfs_fd_t fd) {
    uint32_t index;
    uint32_t generation;
    if (fd == 0U) return NULL;
    index = fd & VFS_SLOT_MASK;
    generation = fd >> VFS_SLOT_BITS;
    if (generation == 0U || g_files[index].open == 0U ||
        g_files[index].generation != generation) {
        return NULL;
    }
    return &g_files[index];
}

static int find_free_slot(void) {
    for (uint32_t i = 0U; i < MAIA_VFS_MAX_OPEN_FILES; ++i) {
        if (g_files[i].open == 0U) return (int)i;
    }
    return -1;
}

static maia_vfs_result_t check_mounted(void) {
    return maia_fatfs_is_mounted() != 0 ? MAIA_VFS_OK : MAIA_VFS_NOT_MOUNTED;
}

maia_vfs_result_t maia_vfs_open(const char *path, uint32_t flags,
                                maia_vfs_fd_t *out_fd) {
    char fatfs_path[MAIA_VFS_PATH_MAX + 1U];
    BYTE mode = 0U;
    int32_t task_id;
    int slot_index;
    maia_vfs_result_t result;

    if (out_fd != NULL) *out_fd = 0U;
    if (out_fd == NULL || (flags & ~VFS_VALID_FLAGS) != 0U ||
        (flags & (MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE)) == 0U ||
        ((flags & (MAIA_VFS_OPEN_CREATE | MAIA_VFS_OPEN_CREATE_NEW |
                   MAIA_VFS_OPEN_TRUNCATE | MAIA_VFS_OPEN_APPEND)) != 0U &&
         (flags & MAIA_VFS_OPEN_WRITE) == 0U) ||
        ((flags & MAIA_VFS_OPEN_CREATE) != 0U &&
         (flags & MAIA_VFS_OPEN_CREATE_NEW) != 0U) ||
        ((flags & MAIA_VFS_OPEN_TRUNCATE) != 0U &&
         ((flags & MAIA_VFS_OPEN_CREATE) == 0U ||
          (flags & MAIA_VFS_OPEN_CREATE_NEW) != 0U))) {
        return MAIA_VFS_INVALID;
    }
    result = make_fatfs_path(path, fatfs_path);
    if (result != MAIA_VFS_OK) return result;
    if ((flags & MAIA_VFS_OPEN_READ) != 0U) mode |= FA_READ;
    if ((flags & MAIA_VFS_OPEN_WRITE) != 0U) mode |= FA_WRITE;
    if ((flags & MAIA_VFS_OPEN_CREATE_NEW) != 0U) {
        mode |= FA_CREATE_NEW;
    } else if ((flags & MAIA_VFS_OPEN_TRUNCATE) != 0U) {
        mode |= FA_CREATE_ALWAYS;
    } else if ((flags & MAIA_VFS_OPEN_CREATE) != 0U) {
        mode |= FA_OPEN_ALWAYS;
    }

    result = lock_vfs(&task_id);
    if (result != MAIA_VFS_OK) return result;
    result = check_mounted();
    if (result != MAIA_VFS_OK) return unlock_vfs(task_id, result);
    slot_index = find_free_slot();
    if (slot_index < 0) return unlock_vfs(task_id, MAIA_VFS_NO_HANDLES);

    FRESULT fatfs_result = f_open(&g_files[slot_index].file, fatfs_path, mode);
    if (fatfs_result != FR_OK) {
        return unlock_vfs(task_id, map_fatfs_result(fatfs_result));
    }

    uint32_t generation =
        (g_files[slot_index].generation + 1U) & VFS_GENERATION_MAX;
    if (generation == 0U) generation = 1U;
    g_files[slot_index].generation = generation;
    g_files[slot_index].flags = flags;
    g_files[slot_index].open = 1U;
    *out_fd = (generation << VFS_SLOT_BITS) | (uint32_t)slot_index;
    return unlock_vfs(task_id, MAIA_VFS_OK);
}

maia_vfs_result_t maia_vfs_read(maia_vfs_fd_t fd, void *buffer,
                                uint32_t count, uint32_t *out_read) {
    int32_t task_id;
    maia_vfs_result_t result;
    if (out_read != NULL) *out_read = 0U;
    if (out_read == NULL || (count != 0U && buffer == NULL)) {
        return MAIA_VFS_INVALID;
    }
    result = lock_vfs(&task_id);
    if (result != MAIA_VFS_OK) return result;
    result = check_mounted();
    if (result != MAIA_VFS_OK) return unlock_vfs(task_id, result);
    maia_vfs_file_t *entry = find_file(fd);
    if (entry == NULL) return unlock_vfs(task_id, MAIA_VFS_INVALID);
    if ((entry->flags & MAIA_VFS_OPEN_READ) == 0U) {
        return unlock_vfs(task_id, MAIA_VFS_ACCESS);
    }
    if (count != 0U) {
        UINT transferred = 0U;
        FRESULT fatfs_result = f_read(&entry->file, buffer, (UINT)count,
                                      &transferred);
        *out_read = (uint32_t)transferred;
        result = map_fatfs_result(fatfs_result);
    }
    return unlock_vfs(task_id, result);
}

maia_vfs_result_t maia_vfs_write(maia_vfs_fd_t fd, const void *buffer,
                                 uint32_t count, uint32_t *out_written) {
    int32_t task_id;
    maia_vfs_result_t result;
    if (out_written != NULL) *out_written = 0U;
    if (out_written == NULL || (count != 0U && buffer == NULL)) {
        return MAIA_VFS_INVALID;
    }
    result = lock_vfs(&task_id);
    if (result != MAIA_VFS_OK) return result;
    result = check_mounted();
    if (result != MAIA_VFS_OK) return unlock_vfs(task_id, result);
    maia_vfs_file_t *entry = find_file(fd);
    if (entry == NULL) return unlock_vfs(task_id, MAIA_VFS_INVALID);
    if ((entry->flags & MAIA_VFS_OPEN_WRITE) == 0U) {
        return unlock_vfs(task_id, MAIA_VFS_ACCESS);
    }
    if (count != 0U) {
        if ((entry->flags & MAIA_VFS_OPEN_APPEND) != 0U) {
            FRESULT seek_result = f_lseek(&entry->file, f_size(&entry->file));
            if (seek_result != FR_OK) {
                return unlock_vfs(task_id, map_fatfs_result(seek_result));
            }
        }
        UINT transferred = 0U;
        FRESULT fatfs_result = f_write(&entry->file, buffer, (UINT)count,
                                       &transferred);
        *out_written = (uint32_t)transferred;
        result = map_fatfs_result(fatfs_result);
    }
    return unlock_vfs(task_id, result);
}

maia_vfs_result_t maia_vfs_seek(maia_vfs_fd_t fd, uint32_t offset) {
    int32_t task_id;
    maia_vfs_result_t result = lock_vfs(&task_id);
    if (result != MAIA_VFS_OK) return result;
    result = check_mounted();
    if (result != MAIA_VFS_OK) return unlock_vfs(task_id, result);
    maia_vfs_file_t *entry = find_file(fd);
    if (entry == NULL) return unlock_vfs(task_id, MAIA_VFS_INVALID);
    return unlock_vfs(task_id,
                      map_fatfs_result(f_lseek(&entry->file, (FSIZE_t)offset)));
}

maia_vfs_result_t maia_vfs_sync(maia_vfs_fd_t fd) {
    int32_t task_id;
    maia_vfs_result_t result = lock_vfs(&task_id);
    if (result != MAIA_VFS_OK) return result;
    result = check_mounted();
    if (result != MAIA_VFS_OK) return unlock_vfs(task_id, result);
    maia_vfs_file_t *entry = find_file(fd);
    if (entry == NULL) return unlock_vfs(task_id, MAIA_VFS_INVALID);
    return unlock_vfs(task_id, map_fatfs_result(f_sync(&entry->file)));
}

maia_vfs_result_t maia_vfs_close(maia_vfs_fd_t fd) {
    int32_t task_id;
    maia_vfs_result_t result = lock_vfs(&task_id);
    if (result != MAIA_VFS_OK) return result;
    result = check_mounted();
    if (result != MAIA_VFS_OK) return unlock_vfs(task_id, result);
    maia_vfs_file_t *entry = find_file(fd);
    if (entry == NULL) return unlock_vfs(task_id, MAIA_VFS_INVALID);
    FRESULT fatfs_result = f_close(&entry->file);
    if (fatfs_result == FR_OK) {
        entry->open = 0U;
        entry->flags = 0U;
    }
    return unlock_vfs(task_id, map_fatfs_result(fatfs_result));
}

maia_vfs_result_t maia_vfs_unlink(const char *path) {
    char fatfs_path[MAIA_VFS_PATH_MAX + 1U];
    int32_t task_id;
    maia_vfs_result_t result = make_fatfs_path(path, fatfs_path);
    if (result != MAIA_VFS_OK) return result;
    result = lock_vfs(&task_id);
    if (result != MAIA_VFS_OK) return result;
    result = check_mounted();
    if (result != MAIA_VFS_OK) return unlock_vfs(task_id, result);
    return unlock_vfs(task_id, map_fatfs_result(f_unlink(fatfs_path)));
}
