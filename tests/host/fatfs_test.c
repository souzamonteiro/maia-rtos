#include <maia/services/fatfs.h>
#include <maia/services/vfs.h>
#include <maia/kernel.h>

#include "diskio.h"
#include "ff.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <unistd.h>
#include "../../runtime/newlib_vfs.h"

int _open(const char *path, int flags, int mode);
int _close(int fd);
off_t _lseek(int fd, off_t offset, int whence);
int _stat(const char *path, struct stat *st);
int _unlink(const char *path);

#define TEST_SECTOR_COUNT 2880U
#define TEST_IMAGE_BYTES ((size_t)TEST_SECTOR_COUNT * MAIA_SD_BLOCK_BYTES)

typedef struct test_disk {
    uint8_t image[TEST_IMAGE_BYTES];
    uint32_t reads;
    uint32_t writes;
    uint32_t read_calls;
    uint32_t write_calls;
    maia_sd_result_t read_error;
    maia_sd_result_t write_error;
} test_disk_t;

static test_disk_t disk;
static maia_sd_block_device_t device;
static maia_task_id_t test_task_id = 1;
static int force_vfs_busy;

maia_task_id_t maia_task_current_id(void) {
    return test_task_id;
}

maia_status_t maia_mutex_trylock(maia_mutex_t *mutex, int32_t task_id) {
    if (mutex == NULL || task_id < 0) return MAIA_ERR_INVALID;
    if (force_vfs_busy != 0) return MAIA_ERR_BUSY;
    if (mutex->locked == 0U) {
        mutex->locked = 1U;
        mutex->owner_task_id = task_id;
        mutex->lock_count = 1U;
        return MAIA_OK;
    }
    if (mutex->owner_task_id == task_id) {
        ++mutex->lock_count;
        return MAIA_OK;
    }
    return MAIA_ERR_BUSY;
}

maia_status_t maia_mutex_unlock(maia_mutex_t *mutex, int32_t task_id) {
    if (mutex == NULL || task_id < 0) return MAIA_ERR_INVALID;
    if (mutex->locked == 0U || mutex->owner_task_id != task_id) {
        return MAIA_ERR_PERM;
    }
    if (mutex->lock_count > 1U) {
        --mutex->lock_count;
    } else {
        mutex->lock_count = 0U;
        mutex->owner_task_id = -1;
        mutex->locked = 0U;
    }
    return MAIA_OK;
}

static void set_u16(uint8_t *buffer, size_t offset, uint16_t value) {
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8U);
}

static void set_u32(uint8_t *buffer, size_t offset, uint32_t value) {
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8U);
    buffer[offset + 2U] = (uint8_t)(value >> 16U);
    buffer[offset + 3U] = (uint8_t)(value >> 24U);
}

static maia_sd_result_t test_read(void *context, uint32_t first_block,
                                  uint32_t count, uint8_t *buffer,
                                  uint32_t *completed_blocks) {
    test_disk_t *storage = (test_disk_t *)context;
    *completed_blocks = 0U;
    ++storage->read_calls;
    if (storage->read_error != MAIA_SD_OK) return storage->read_error;
    if (count == 0U || first_block >= TEST_SECTOR_COUNT ||
        count > TEST_SECTOR_COUNT - first_block) {
        return MAIA_SD_OUT_OF_RANGE;
    }
    memcpy(buffer, storage->image + (size_t)first_block * MAIA_SD_BLOCK_BYTES,
           (size_t)count * MAIA_SD_BLOCK_BYTES);
    storage->reads += count;
    *completed_blocks = count;
    return MAIA_SD_OK;
}

static maia_sd_result_t test_write(void *context, uint32_t first_block,
                                   uint32_t count, const uint8_t *buffer,
                                   uint32_t *completed_blocks) {
    test_disk_t *storage = (test_disk_t *)context;
    *completed_blocks = 0U;
    ++storage->write_calls;
    if (storage->write_error != MAIA_SD_OK) return storage->write_error;
    if (count == 0U || first_block >= TEST_SECTOR_COUNT ||
        count > TEST_SECTOR_COUNT - first_block) {
        return MAIA_SD_OUT_OF_RANGE;
    }
    memcpy(storage->image + (size_t)first_block * MAIA_SD_BLOCK_BYTES,
           buffer, (size_t)count * MAIA_SD_BLOCK_BYTES);
    storage->writes += count;
    *completed_blocks = count;
    return MAIA_SD_OK;
}

static void create_fat12_volume(void) {
    uint8_t *boot = disk.image;
    boot[0] = 0xEBU;
    boot[1] = 0x3CU;
    boot[2] = 0x90U;
    memcpy(boot + 3U, "MAIARTOS", 8U);
    set_u16(boot, 11U, MAIA_SD_BLOCK_BYTES);
    boot[13U] = 1U;
    set_u16(boot, 14U, 1U);
    boot[16U] = 2U;
    set_u16(boot, 17U, 224U);
    set_u16(boot, 19U, TEST_SECTOR_COUNT);
    boot[21U] = 0xF0U;
    set_u16(boot, 22U, 9U);
    set_u16(boot, 24U, 18U);
    set_u16(boot, 26U, 2U);
    set_u32(boot, 28U, 0U);
    set_u32(boot, 32U, 0U);
    boot[38U] = 0x29U;
    set_u32(boot, 39U, 0x4D414941U);
    memcpy(boot + 43U, "MAIARTOS   ", 11U);
    memcpy(boot + 54U, "FAT12   ", 8U);
    boot[510U] = 0x55U;
    boot[511U] = 0xAAU;

    for (uint32_t fat_sector = 1U; fat_sector <= 10U; fat_sector += 9U) {
        uint8_t *fat = disk.image +
            (size_t)fat_sector * MAIA_SD_BLOCK_BYTES;
        fat[0] = 0xF0U;
        fat[1] = 0xFFU;
        fat[2] = 0xFFU;
    }
}

static void test_disk_interface(void) {
    uint8_t sector[MAIA_SD_BLOCK_BYTES] = { 0 };
    LBA_t sector_count = 0U;
    WORD sector_size = 0U;
    DWORD block_size = 0U;
    uint32_t reads_before = disk.reads;
    assert(disk_status(1U) == STA_NOINIT);
    assert(disk_read(1U, sector, 0U, 1U) == RES_PARERR);
    assert(disk_read(0U, sector, TEST_SECTOR_COUNT, 1U) == RES_PARERR);
    assert(disk.reads == reads_before);

    assert(disk_status(0U) == 0U);
    assert(disk_initialize(0U) == 0U);
    assert(disk_ioctl(0U, GET_SECTOR_COUNT, &sector_count) == RES_OK);
    assert(sector_count == TEST_SECTOR_COUNT);
    assert(disk_ioctl(0U, GET_SECTOR_SIZE, &sector_size) == RES_OK);
    assert(sector_size == MAIA_SD_BLOCK_BYTES);
    assert(disk_ioctl(0U, GET_BLOCK_SIZE, &block_size) == RES_OK);
    assert(block_size == 1U);
    assert(disk_ioctl(0U, CTRL_SYNC, NULL) == RES_OK);

    const struct {
        maia_sd_result_t transport_error;
        DRESULT diskio_result;
    } failures[] = {
        { MAIA_SD_TIMEOUT, RES_ERROR },
        { MAIA_SD_IO_ERROR, RES_ERROR },
        { MAIA_SD_NOT_READY, RES_NOTRDY },
    };
    uint32_t writes_before = disk.writes;
    for (size_t i = 0U; i < sizeof(failures) / sizeof(failures[0]); ++i) {
        disk.read_error = failures[i].transport_error;
        assert(disk_read(0U, sector, 0U, 1U) == failures[i].diskio_result);
        disk.read_error = MAIA_SD_OK;
        disk.write_error = failures[i].transport_error;
        assert(disk_write(0U, sector, 0U, 1U) == failures[i].diskio_result);
        disk.write_error = MAIA_SD_OK;
    }
    assert(disk.reads == reads_before && disk.writes == writes_before);
}

static void test_file_operations(void) {
    static const char path[] = "0:/MaiaRTOS FatFs test.txt";
    uint8_t written[700];
    uint8_t readback[700];
    for (size_t i = 0U; i < sizeof(written); ++i) {
        written[i] = (uint8_t)(i * 37U + 11U);
    }

    FIL file;
    UINT transferred = 0U;
    assert(f_open(&file, path, FA_CREATE_NEW | FA_WRITE) == FR_OK);
    assert(f_write(&file, written, sizeof(written), &transferred) == FR_OK);
    assert(transferred == sizeof(written));
    assert(f_sync(&file) == FR_OK);
    assert(f_close(&file) == FR_OK);

    uint32_t writes_before = disk.writes;
    FIL existing;
    assert(f_open(&existing, path, FA_CREATE_NEW | FA_WRITE) == FR_EXIST);
    assert(disk.writes == writes_before);

    assert(f_open(&file, path, FA_READ) == FR_OK);
    assert(f_read(&file, readback, sizeof(readback), &transferred) == FR_OK);
    assert(transferred == sizeof(readback));
    assert(memcmp(written, readback, sizeof(written)) == 0);
    assert(f_close(&file) == FR_OK);

    assert(f_open(&file, path, FA_READ | FA_WRITE) == FR_OK);
    assert(f_lseek(&file, 19U) == FR_OK);
    written[19U] ^= 0x5AU;
    assert(f_write(&file, &written[19U], 1U, &transferred) == FR_OK);
    assert(transferred == 1U);
    assert(f_sync(&file) == FR_OK);
    assert(f_close(&file) == FR_OK);

    assert(f_open(&file, path, FA_READ) == FR_OK);
    assert(f_read(&file, readback, sizeof(readback), &transferred) == FR_OK);
    assert(transferred == sizeof(readback));
    assert(memcmp(written, readback, sizeof(written)) == 0);
    assert(f_close(&file) == FR_OK);
    assert(f_unlink(path) == FR_OK);
}

static void test_vfs_file_operations(void) {
    static const char path[] = "/sd/MaiaRTOS VFS test.txt";
    static const uint8_t suffix[] = { 0xA5U, 0x5AU, 0xC3U };
    uint8_t expected[703];
    uint8_t readback[703];
    maia_vfs_fd_t fd = 0U;
    maia_vfs_fd_t second_fd = 0U;
    uint32_t transferred = 99U;

    for (size_t i = 0U; i < 700U; ++i) {
        expected[i] = (uint8_t)(i * 19U + 7U);
    }
    memcpy(expected + 700U, suffix, sizeof(suffix));

    assert(maia_vfs_open("/sdb/file", MAIA_VFS_OPEN_READ, &fd) ==
           MAIA_VFS_INVALID);
    assert(maia_vfs_open("/sd/../file", MAIA_VFS_OPEN_READ, &fd) ==
           MAIA_VFS_INVALID);
    assert(maia_vfs_open("/sd//file", MAIA_VFS_OPEN_READ, &fd) ==
           MAIA_VFS_INVALID);
    assert(maia_vfs_open("/sd/file/", MAIA_VFS_OPEN_READ, &fd) ==
           MAIA_VFS_INVALID);
    assert(maia_vfs_open("/sd/file", MAIA_VFS_OPEN_READ |
                         MAIA_VFS_OPEN_TRUNCATE, &fd) == MAIA_VFS_INVALID);
    assert(maia_vfs_open("/sd/file", MAIA_VFS_OPEN_READ, NULL) ==
           MAIA_VFS_INVALID);

    force_vfs_busy = 1;
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_BUSY);
    assert(fd == 0U);
    force_vfs_busy = 0;

    test_task_id = -1;
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) ==
           MAIA_VFS_BAD_CONTEXT);
    assert(fd == 0U);
    test_task_id = 1;

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                         MAIA_VFS_OPEN_CREATE_NEW, &fd) == MAIA_VFS_OK);
    assert(fd != 0U);
    assert(maia_vfs_write(fd, expected, 700U, &transferred) == MAIA_VFS_OK);
    assert(transferred == 700U);
    disk.write_error = MAIA_SD_IO_ERROR;
    assert(maia_vfs_close(fd) == MAIA_VFS_IO_ERROR);
    assert(maia_vfs_sync(fd) == MAIA_VFS_IO_ERROR);
    disk.write_error = MAIA_SD_OK;
    assert(maia_vfs_sync(fd) == MAIA_VFS_OK);
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE,
                         &second_fd) == MAIA_VFS_BUSY);
    assert(second_fd == 0U);
    assert(maia_vfs_unlink(path) == MAIA_VFS_BUSY);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                         MAIA_VFS_OPEN_CREATE_NEW, &fd) == MAIA_VFS_EXISTS);
    assert(fd == 0U);
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                         MAIA_VFS_OPEN_CREATE, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, NULL, 1U, &transferred) == MAIA_VFS_INVALID);
    assert(maia_vfs_read(fd, readback, 700U, &transferred) == MAIA_VFS_OK);
    assert(transferred == 700U);
    assert(memcmp(expected, readback, 700U) == 0);
    assert(maia_vfs_seek(fd, 19U) == MAIA_VFS_OK);
    expected[19U] ^= 0x6DU;
    assert(maia_vfs_write(fd, expected + 19U, 1U, &transferred) == MAIA_VFS_OK);
    assert(transferred == 1U);
    assert(maia_vfs_sync(fd) == MAIA_VFS_OK);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, readback, 703U, &transferred) == MAIA_VFS_OK);
    assert(transferred == 700U);
    assert(memcmp(expected, readback, 700U) == 0);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_WRITE |
                         MAIA_VFS_OPEN_APPEND, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_write(fd, suffix, sizeof(suffix), &transferred) ==
           MAIA_VFS_OK);
    assert(transferred == sizeof(suffix));
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, readback, sizeof(readback), &transferred) ==
           MAIA_VFS_OK);
    assert(transferred == sizeof(expected));
    assert(memcmp(expected, readback, sizeof(expected)) == 0);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, readback, 1U, &transferred) == MAIA_VFS_INVALID);
    assert(transferred == 0U);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_WRITE, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, readback, 1U, &transferred) == MAIA_VFS_ACCESS);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    maia_vfs_fd_t readers[MAIA_VFS_MAX_OPEN_FILES];
    for (size_t i = 0U; i < MAIA_VFS_MAX_OPEN_FILES; ++i) {
        assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &readers[i]) ==
               MAIA_VFS_OK);
    }
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &second_fd) ==
           MAIA_VFS_NO_HANDLES);
    assert(second_fd == 0U);
    for (size_t i = 0U; i < MAIA_VFS_MAX_OPEN_FILES; ++i) {
        assert(maia_vfs_close(readers[i]) == MAIA_VFS_OK);
    }

    assert(maia_vfs_unlink(path) == MAIA_VFS_OK);
    assert(maia_vfs_unlink(path) == MAIA_VFS_NOT_FOUND);
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) ==
           MAIA_VFS_NOT_FOUND);
    assert(fd == 0U);
}

static void test_vfs_directories(void) {
    maia_vfs_info_t info;
    maia_vfs_dir_t dir;
    maia_vfs_dirent_t entry;
    int has_entry;
    assert(maia_vfs_stat("/sd", &info) == MAIA_VFS_OK);
    assert(info.is_directory && info.size == 0U);
    uint32_t writes_before = disk.writes;
    assert(maia_vfs_mkdir("/sd/../bad") == MAIA_VFS_INVALID);
    assert(maia_vfs_mkdir("/sd/") == MAIA_VFS_INVALID);
    assert(disk.writes == writes_before);
    assert(maia_vfs_mkdir("/sd/Native directory") == MAIA_VFS_OK);
    assert(maia_vfs_mkdir("/sd/Native directory") == MAIA_VFS_EXISTS);
    assert(maia_vfs_stat("/sd/Native directory", &info) == MAIA_VFS_OK);
    assert(info.is_directory);
    assert(maia_vfs_unlink("/sd/Native directory") == MAIA_VFS_IS_DIRECTORY);
    assert(maia_vfs_opendir("/sd/", &dir) == MAIA_VFS_OK);
    force_vfs_busy = 1;
    assert(maia_vfs_readdir(dir, &entry, &has_entry) == MAIA_VFS_BUSY);
    assert(has_entry == 0);
    force_vfs_busy = 0;
    assert(maia_vfs_readdir(dir, &entry, &has_entry) == MAIA_VFS_OK);
    assert(has_entry && entry.info.is_directory);
    assert(strcmp(entry.name, "Native directory") == 0);
    assert(maia_vfs_readdir(dir, &entry, &has_entry) == MAIA_VFS_OK);
    assert(!has_entry);
    assert(maia_vfs_closedir(dir) == MAIA_VFS_OK);
    assert(maia_vfs_readdir(dir, &entry, &has_entry) == MAIA_VFS_INVALID);
    maia_vfs_dir_t dirs[MAIA_VFS_MAX_OPEN_DIRS];
    for (unsigned i = 0U; i < MAIA_VFS_MAX_OPEN_DIRS; ++i)
        assert(maia_vfs_opendir("/sd", &dirs[i]) == MAIA_VFS_OK);
    assert(maia_vfs_opendir("/sd", &dir) == MAIA_VFS_NO_HANDLES);
    for (unsigned i = 0U; i < MAIA_VFS_MAX_OPEN_DIRS; ++i)
        assert(maia_vfs_closedir(dirs[i]) == MAIA_VFS_OK);
    assert(maia_vfs_rmdir("/sd/Native directory") == MAIA_VFS_OK);
}

static void test_newlib_adapter(void) {
    static const char path[] = "/sd/Newlib file.txt";
    struct stat st;
    char buffer[16];
    int fd = _open(path, O_RDWR | O_CREAT | O_EXCL, 0666);
    assert(fd >= 3);
    assert(_open(path, O_WRONLY | O_CREAT | O_EXCL, 0666) == -1);
    assert(errno == EEXIST);
    assert(maia_newlib_vfs_write(fd, "hello", 5) == 5);
    assert(fsync(fd) == 0);
    assert(maia_newlib_vfs_fstat(fd, &st) == 0);
    assert(S_ISREG(st.st_mode) && st.st_size == 5);
    assert(_lseek(fd, -2, SEEK_CUR) == 3);
    assert(maia_newlib_vfs_read(fd, buffer, sizeof(buffer)) == 2);
    assert(memcmp(buffer, "lo", 2) == 0);
    assert(_lseek(fd, -5, SEEK_END) == 0);
    assert(maia_newlib_vfs_read(fd, buffer, sizeof(buffer)) == 5);
    assert(memcmp(buffer, "hello", 5) == 0);
    assert(maia_newlib_vfs_read(fd, buffer, sizeof(buffer)) == 0);
    assert(_lseek(fd, -1, SEEK_SET) == -1 && errno == EINVAL);
    assert(_lseek(fd, INT32_MAX, SEEK_END) == -1 && errno == EOVERFLOW);
    assert(_lseek(fd, 0, 123) == -1 && errno == EINVAL);
    assert(_close(fd) == 0);
    assert(_close(fd) == -1 && errno == EBADF);
    assert(maia_newlib_vfs_read(fd, buffer, 0) == -1 && errno == EBADF);
    assert(_stat(path, &st) == 0 && st.st_size == 5);
    assert(_open(path, O_WRONLY | O_TRUNC, 0666) == -1 && errno == ENOTSUP);
    assert(_open("/sd", O_RDONLY, 0) == -1 && errno == EISDIR);
    assert(_open("/sd/../escape", O_RDONLY, 0) == -1 && errno == EINVAL);
    assert(_lseek(1, 0, SEEK_SET) == -1 && errno == ESPIPE);
    force_vfs_busy = 1;
    assert(_open(path, O_RDONLY, 0) == -1 && errno == EBUSY);
    force_vfs_busy = 0;
    test_task_id = -1;
    assert(_open(path, O_RDONLY, 0) == -1 && errno == ENOTSUP);
    test_task_id = 1;

    fd = _open(path, O_WRONLY | O_APPEND, 0);
    assert(fd >= 3);
    assert(_lseek(fd, 0, SEEK_SET) == 0);
    assert(maia_newlib_vfs_write(fd, "!", 1) == 1);
    assert(maia_newlib_vfs_read(fd, buffer, 1) == -1 && errno == EBADF);
    disk.write_error = MAIA_SD_IO_ERROR;
    assert(_close(fd) == -1 && errno == EIO);
    disk.write_error = MAIA_SD_OK;
    assert(_close(fd) == 0);
    assert(_stat(path, &st) == 0 && st.st_size == 6);
    int readers[MAIA_VFS_MAX_OPEN_FILES];
    for (unsigned i = 0U; i < MAIA_VFS_MAX_OPEN_FILES; ++i)
        assert((readers[i] = _open(path, O_RDONLY, 0)) >= 3);
    assert(_open(path, O_RDONLY, 0) == -1 && errno == EMFILE);
    for (unsigned i = 0U; i < MAIA_VFS_MAX_OPEN_FILES; ++i)
        assert(_close(readers[i]) == 0);

    assert(mkdir("/sd/Newlib directory", 0777) == 0);
    DIR *directory = opendir("/sd");
    assert(directory != NULL);
    struct dirent *entry;
    int count = 0;
    errno = 0;
    while ((entry = readdir(directory)) != NULL) {
        assert(strcmp(entry->d_name, "Newlib file.txt") == 0 ||
               strcmp(entry->d_name, "Newlib directory") == 0);
        ++count;
        errno = 0;
    }
    assert(errno == 0 && count == 2);
    assert(closedir(directory) == 0);
    assert(closedir(directory) == -1 && errno == EBADF);
    assert(opendir(path) == NULL && errno == ENOTDIR);
    assert(_unlink("/sd/Newlib directory") == -1 && errno == EISDIR);
    assert(rmdir(path) == -1 && errno == ENOTDIR);
    assert(_unlink(path) == 0);
    assert(rmdir("/sd/Newlib directory") == 0);
    assert(_stat(path, &st) == -1 && errno == ENOENT);
    assert(_open(path, O_RDONLY, 0) == -1 && errno == ENOENT);
    fd = _open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    assert(fd >= 3);
    assert(maia_newlib_vfs_write(fd, "abc", 3) == 3);
    assert(_close(fd) == 0);
    fd = _open(path, O_WRONLY | O_CREAT | O_TRUNC, 0666);
    assert(fd >= 3);
    assert(_close(fd) == 0);
    assert(_stat(path, &st) == 0 && st.st_size == 0);
    assert(_unlink(path) == 0);
}

static void test_diskio_fault_recovery(void) {
    static const char path[] = "/sd/one-shot-io-fault.bin";
    uint8_t expected[MAIA_SD_BLOCK_BYTES];
    uint8_t actual[MAIA_SD_BLOCK_BYTES];
    maia_vfs_fd_t fd;
    uint32_t transferred;
    for (size_t i = 0U; i < sizeof(expected); ++i)
        expected[i] = (uint8_t)(i * 13U + 0x31U);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                         MAIA_VFS_OPEN_CREATE_NEW, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_write(fd, expected, sizeof(expected), &transferred) ==
           MAIA_VFS_OK && transferred == sizeof(expected));
    assert(maia_vfs_sync(fd) == MAIA_VFS_OK);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, actual, sizeof(actual), &transferred) ==
           MAIA_VFS_OK && transferred == sizeof(actual));
    assert(memcmp(expected, actual, sizeof(expected)) == 0);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE,
                         &fd) == MAIA_VFS_OK);
    assert(maia_vfs_seek(fd, 7U) == MAIA_VFS_OK);
    expected[7U] ^= 0xA5U;
    assert(maia_vfs_write(fd, expected + 7U, 1U, &transferred) ==
           MAIA_VFS_OK && transferred == 1U);
    uint32_t fault_count = maia_fatfs_diag_faults_triggered();
    maia_fatfs_diag_fail_next(MAIA_FATFS_DIAG_WRITE);
    maia_vfs_result_t result = maia_vfs_sync(fd);
    maia_fatfs_diag_clear_pending_fault();
    assert(result == MAIA_VFS_IO_ERROR);
    assert(maia_fatfs_diag_faults_triggered() == fault_count + 1U);
    assert(maia_vfs_sync(fd) == MAIA_VFS_OK);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
    fault_count = maia_fatfs_diag_faults_triggered();
    maia_fatfs_diag_fail_next(MAIA_FATFS_DIAG_READ);
    result = maia_vfs_read(fd, actual, sizeof(actual), &transferred);
    maia_fatfs_diag_clear_pending_fault();
    assert(result == MAIA_VFS_IO_ERROR);
    assert(transferred == 0U);
    assert(maia_fatfs_diag_faults_triggered() == fault_count + 1U);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, actual, sizeof(actual), &transferred) ==
           MAIA_VFS_OK);
    assert(transferred == sizeof(actual));
    if (memcmp(expected, actual, sizeof(expected)) != 0) {
        for (size_t i = 0U; i < sizeof(expected); ++i) {
            if (expected[i] != actual[i]) {
                fprintf(stderr, "fault recovery mismatch at %zu: expected=%u actual=%u\n",
                        i, expected[i], actual[i]);
                break;
            }
        }
        assert(0);
    }
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    maia_fatfs_diag_fail_next(MAIA_FATFS_DIAG_READ);
    maia_fatfs_diag_clear_pending_fault();
    fault_count = maia_fatfs_diag_faults_triggered();
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, actual, sizeof(actual), &transferred) ==
           MAIA_VFS_OK && transferred == sizeof(actual));
    assert(maia_fatfs_diag_faults_triggered() == fault_count);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);
    assert(maia_vfs_unlink(path) == MAIA_VFS_OK);
}

static void test_vfs_transport_faults(void) {
    static const char path[] = "/sd/transport-fault.bin";
    static const maia_sd_result_t read_errors[] = {
        MAIA_SD_TIMEOUT, MAIA_SD_NOT_READY
    };
    uint8_t expected[MAIA_SD_BLOCK_BYTES];
    uint8_t actual[MAIA_SD_BLOCK_BYTES];
    maia_vfs_fd_t fd;
    uint32_t transferred;

    for (size_t i = 0U; i < sizeof(expected); ++i)
        expected[i] = (uint8_t)(i * 11U + 0x37U);
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                         MAIA_VFS_OPEN_CREATE_NEW, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_write(fd, expected, sizeof(expected), &transferred) ==
           MAIA_VFS_OK && transferred == sizeof(expected));
    assert(maia_vfs_sync(fd) == MAIA_VFS_OK);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    for (size_t i = 0U; i < sizeof(read_errors) / sizeof(read_errors[0]); ++i) {
        assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
        disk.read_error = read_errors[i];
        uint32_t calls_before = disk.read_calls;
        transferred = UINT32_MAX;
        assert(maia_vfs_read(fd, actual, sizeof(actual), &transferred) ==
               MAIA_VFS_IO_ERROR);
        assert(transferred == 0U && disk.read_calls == calls_before + 1U);
        assert(maia_vfs_close(fd) == MAIA_VFS_OK);
        disk.read_error = MAIA_SD_OK;
    }

    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE,
                         &fd) == MAIA_VFS_OK);
    assert(maia_vfs_seek(fd, 7U) == MAIA_VFS_OK);
    expected[7U] ^= 0xA5U;
    assert(maia_vfs_write(fd, expected + 7U, 1U, &transferred) ==
           MAIA_VFS_OK && transferred == 1U);
    uint32_t calls_before = disk.write_calls;
    uint32_t writes_before = disk.writes;
    disk.write_error = MAIA_SD_TIMEOUT;
    assert(maia_vfs_sync(fd) == MAIA_VFS_IO_ERROR);
    disk.write_error = MAIA_SD_NOT_READY;
    assert(maia_vfs_sync(fd) == MAIA_VFS_IO_ERROR);
    assert(maia_vfs_close(fd) == MAIA_VFS_IO_ERROR);
    assert(disk.write_calls == calls_before + 3U && disk.writes == writes_before);

    /* Reset the host mock; the real Tab5 service remains faulted until reset. */
    disk.write_error = MAIA_SD_OK;
    assert(maia_vfs_sync(fd) == MAIA_VFS_OK);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, actual, sizeof(actual), &transferred) ==
           MAIA_VFS_OK && transferred == sizeof(actual));
    assert(memcmp(expected, actual, sizeof(expected)) == 0);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);
    assert(maia_vfs_unlink(path) == MAIA_VFS_OK);
}

static void test_remount_persistence(void) {
    static const char path[] = "/sd/remount-persistence.bin";
    uint8_t expected[MAIA_SD_BLOCK_BYTES];
    uint8_t actual[MAIA_SD_BLOCK_BYTES];
    maia_vfs_fd_t fd;
    uint32_t transferred;

    for (size_t i = 0U; i < sizeof(expected); ++i)
        expected[i] = (uint8_t)(i * 19U + 0x27U);
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                         MAIA_VFS_OPEN_CREATE_NEW, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_write(fd, expected, sizeof(expected), &transferred) ==
           MAIA_VFS_OK && transferred == sizeof(expected));
    assert(maia_vfs_sync(fd) == MAIA_VFS_OK);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);

    assert(maia_fatfs_unmount() == MAIA_FATFS_OK);
    assert(maia_fatfs_is_mounted() == 0);
    assert(disk_status(0U) == STA_NOINIT);
    assert(maia_fatfs_mount(&device) == MAIA_FATFS_OK);
    assert(maia_fatfs_is_mounted() == 1);
    assert(maia_vfs_open(path, MAIA_VFS_OPEN_READ, &fd) == MAIA_VFS_OK);
    assert(maia_vfs_read(fd, actual, sizeof(actual), &transferred) ==
           MAIA_VFS_OK && transferred == sizeof(actual));
    assert(memcmp(expected, actual, sizeof(expected)) == 0);
    assert(maia_vfs_close(fd) == MAIA_VFS_OK);
    assert(maia_vfs_unlink(path) == MAIA_VFS_OK);
}

int main(void) {
    static const maia_sd_geometry_t geometry = { TEST_SECTOR_COUNT };
    memset(&disk, 0, sizeof(disk));
    assert(maia_sd_block_device_init(
               &device, &geometry, test_read, test_write, &disk) == MAIA_SD_OK);

    uint32_t writes_before = disk.writes;
    assert(maia_fatfs_mount(&device) == MAIA_FATFS_NO_FILESYSTEM);
    assert(maia_fatfs_is_mounted() == 0);
    assert(disk.writes == writes_before);
    assert(disk_status(0U) == STA_NOINIT);
    maia_vfs_fd_t fd = 99U;
    assert(maia_vfs_open("/sd/not-mounted", MAIA_VFS_OPEN_READ, &fd) ==
           MAIA_VFS_NOT_MOUNTED);
    assert(fd == 0U);

    create_fat12_volume();
    writes_before = disk.writes;
    assert(maia_fatfs_mount(&device) == MAIA_FATFS_OK);
    assert(maia_fatfs_is_mounted() == 1);
    assert(disk.writes == writes_before);
    assert(maia_fatfs_mount(&device) == MAIA_FATFS_BUSY);
    assert(disk.writes == writes_before);

    test_disk_interface();
    test_file_operations();
    test_vfs_file_operations();
    test_vfs_directories();
    test_newlib_adapter();
    test_diskio_fault_recovery();
    test_vfs_transport_faults();
    test_remount_persistence();

    assert(maia_fatfs_unmount() == MAIA_FATFS_OK);
    assert(maia_fatfs_is_mounted() == 0);
    assert(disk_status(0U) == STA_NOINIT);
    assert(_open("/sd/file", O_RDONLY, 0) == -1 && errno == ENODEV);
    puts("FatFs, MaiaVFS files/directories and Newlib adapter: PASS");
    return 0;
}
