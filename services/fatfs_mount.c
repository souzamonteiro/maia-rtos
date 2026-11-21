#include <maia/services/fatfs.h>

#include "fatfs_diskio.h"
#include "ff.h"

#include <stddef.h>
#include <stdint.h>

static FATFS sd_volume;
static int sd_mounted;

static maia_fatfs_result_t map_mount_result(FRESULT result) {
    switch (result) {
    case FR_OK:
        return MAIA_FATFS_OK;
    case FR_NOT_READY:
        return MAIA_FATFS_NOT_READY;
    case FR_NO_FILESYSTEM:
        return MAIA_FATFS_NO_FILESYSTEM;
    case FR_DISK_ERR:
    case FR_INT_ERR:
        return MAIA_FATFS_IO_ERROR;
    default:
        return MAIA_FATFS_FAILURE;
    }
}

maia_fatfs_result_t maia_fatfs_mount(maia_sd_block_device_t *device) {
    if (device == NULL || device->initialized != 1U ||
        device->read == NULL || device->write == NULL ||
        device->geometry.block_count == 0U ||
        device->geometry.block_count > UINT32_MAX) {
        return MAIA_FATFS_INVALID;
    }
    if (sd_mounted != 0) return MAIA_FATFS_BUSY;

    maia_sd_result_t bind_result = maia_fatfs_disk_bind(device);
    if (bind_result == MAIA_SD_BUSY) return MAIA_FATFS_BUSY;
    if (bind_result != MAIA_SD_OK) return MAIA_FATFS_INVALID;

    FRESULT result = f_mount(&sd_volume, "0:", 1U);
    if (result != FR_OK) {
        (void)f_mount(NULL, "0:", 0U);
        maia_fatfs_disk_unbind();
        return map_mount_result(result);
    }

    sd_mounted = 1;
    return MAIA_FATFS_OK;
}

maia_fatfs_result_t maia_fatfs_unmount(void) {
    if (sd_mounted == 0) return MAIA_FATFS_NOT_READY;
    FRESULT result = f_mount(NULL, "0:", 0U);
    if (result != FR_OK) return map_mount_result(result);
    maia_fatfs_disk_unbind();
    sd_mounted = 0;
    return MAIA_FATFS_OK;
}

int maia_fatfs_is_mounted(void) {
    return sd_mounted;
}
