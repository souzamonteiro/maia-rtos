#include <maia/boards/tab5_maiascript_fs.h>
#include <maia/exec/elf_loader.h>
#include <maia/services/vfs.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static char empty_text[1];
static char last_item_name[MAIA_VFS_PATH_MAX + 1U];

static int32_t host_fs_exists(const char *path) {
    maia_vfs_info_t info;
    if (path == NULL) return 0;
    return maia_vfs_stat(path, &info) == MAIA_VFS_OK ? 1 : 0;
}

/* The returned text is owned by the module (MaiaScript strings are never
 * freed); failures return a shared empty string, matching the IDF host. */
static char *host_fs_read_text(const char *path) {
    maia_vfs_info_t info;
    maia_vfs_fd_t fd;
    if (path == NULL || maia_vfs_stat(path, &info) != MAIA_VFS_OK ||
        info.is_directory || info.size == UINT32_MAX) return empty_text;
    char *text = (char *)malloc((size_t)info.size + 1U);
    if (text == NULL) return empty_text;
    if (maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) != MAIA_VFS_OK) {
        free(text);
        return empty_text;
    }
    uint32_t used = 0U;
    while (used < info.size) {
        uint32_t got = 0U;
        if (maia_vfs_read(fd, text + used, info.size - used, &got) != MAIA_VFS_OK ||
            got == 0U) break;
        used += got;
    }
    (void)maia_vfs_close(fd);
    text[used] = '\0';
    return text;
}

static int32_t host_fs_write_text(const char *path, const char *content) {
    maia_vfs_fd_t fd;
    if (path == NULL) return MAIA_VFS_INVALID;
    size_t length = content != NULL ? strlen(content) : 0U;
    if (length > INT32_MAX) return MAIA_VFS_INVALID;
    maia_vfs_result_t result = maia_vfs_open(
        path, MAIA_VFS_OPEN_WRITE | MAIA_VFS_OPEN_CREATE | MAIA_VFS_OPEN_TRUNCATE, &fd);
    if (result != MAIA_VFS_OK) return result;
    size_t used = 0U;
    while (used < length) {
        uint32_t wrote = 0U;
        result = maia_vfs_write(fd, content + used, (uint32_t)(length - used), &wrote);
        if (result != MAIA_VFS_OK) break;
        if (wrote == 0U) {
            result = MAIA_VFS_IO_ERROR;
            break;
        }
        used += wrote;
    }
    maia_vfs_result_t closed = maia_vfs_close(fd);
    if (result != MAIA_VFS_OK) return result;
    if (closed != MAIA_VFS_OK) return closed;
    return (int32_t)length;
}

static int32_t host_fs_remove(const char *path) {
    if (path == NULL) return MAIA_VFS_INVALID;
    return maia_vfs_unlink(path);
}

/* Walks the directory to entry idx (or counts all when idx < 0). */
static int32_t visit(const char *path, int32_t idx, maia_vfs_dirent_t *found) {
    maia_vfs_dir_t dir;
    if (path == NULL || maia_vfs_opendir(path, &dir) != MAIA_VFS_OK) return 0;
    static maia_vfs_dirent_t entry;
    int32_t count = 0;
    int has_entry = 0;
    while (maia_vfs_readdir(dir, &entry, &has_entry) == MAIA_VFS_OK && has_entry) {
        if (count == idx) {
            *found = entry;
            (void)maia_vfs_closedir(dir);
            return 1;
        }
        ++count;
    }
    (void)maia_vfs_closedir(dir);
    return idx < 0 ? count : 0;
}

static int32_t host_fs_list_count(const char *path) {
    return visit(path, -1, NULL);
}

static const char *host_fs_list_item_name(const char *path, int32_t idx) {
    static maia_vfs_dirent_t entry;
    last_item_name[0] = '\0';
    if (idx >= 0 && visit(path, idx, &entry))
        memcpy(last_item_name, entry.name, sizeof(last_item_name));
    return last_item_name;
}

static int32_t host_fs_list_item_is_dir(const char *path, int32_t idx) {
    static maia_vfs_dirent_t entry;
    return idx >= 0 && visit(path, idx, &entry) && entry.info.is_directory ? 1 : 0;
}

int maia_tab5_fs_register_maiascript_symbols(void) {
    const struct {
        const char *name;
        uintptr_t address;
    } symbols[] = {
        { "host_fs_exists", (uintptr_t)host_fs_exists },
        { "host_fs_read_text", (uintptr_t)host_fs_read_text },
        { "host_fs_write_text", (uintptr_t)host_fs_write_text },
        { "host_fs_remove", (uintptr_t)host_fs_remove },
        { "host_fs_list_count", (uintptr_t)host_fs_list_count },
        { "host_fs_list_item_name", (uintptr_t)host_fs_list_item_name },
        { "host_fs_list_item_is_dir", (uintptr_t)host_fs_list_item_is_dir },
    };
    for (size_t i = 0U; i < sizeof(symbols) / sizeof(symbols[0]); ++i) {
        int result = mnm_register_symbol(symbols[i].name, symbols[i].address);
        if (result != 0) return result;
    }
    return 0;
}
