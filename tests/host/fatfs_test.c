#include <maia/services/fatfs.h>
#include <maia/services/vfs.h>
#include <maia/kernel.h>

#include "diskio.h"
#include "ff.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define TEST_SECTOR_COUNT 2880U
#define TEST_IMAGE_BYTES ((size_t)TEST_SECTOR_COUNT * MAIA_SD_BLOCK_BYTES)

typedef struct test_disk {
    uint8_t image[TEST_IMAGE_BYTES];
    uint32_t reads;
    uint32_t writes;
    int fail_read;
    int fail_write;
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
    if (storage->fail_read != 0) return MAIA_SD_IO_ERROR;
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
    if (storage->fail_write != 0) return MAIA_SD_IO_ERROR;
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
    uint8_t sector[MAIA_SD_BLOCK_BYTES];
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

    disk.fail_read = 1;
    assert(disk_read(0U, sector, 0U, 1U) == RES_ERROR);
    disk.fail_read = 0;
    disk.fail_write = 1;
    assert(disk_write(0U, sector, 0U, 1U) == RES_ERROR);
    disk.fail_write = 0;
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
    disk.fail_write = 1;
    assert(maia_vfs_close(fd) == MAIA_VFS_IO_ERROR);
    assert(maia_vfs_sync(fd) == MAIA_VFS_IO_ERROR);
    disk.fail_write = 0;
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

    assert(maia_fatfs_unmount() == MAIA_FATFS_OK);
    assert(maia_fatfs_is_mounted() == 0);
    assert(disk_status(0U) == STA_NOINIT);
    puts("FatFs mount, no-format, disk I/O and MaiaVFS file operations: PASS");
    return 0;
}
