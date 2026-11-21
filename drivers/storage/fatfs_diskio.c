#include "fatfs_diskio.h"

#include "diskio.h"

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

_Static_assert(sizeof(LBA_t) == sizeof(uint32_t),
               "FatFs LBA_t must match the SD block-address width");
_Static_assert(sizeof(UINT) == sizeof(uint32_t),
               "FatFs UINT must match the SD multi-sector request width");

static maia_sd_block_device_t *bound_device;

static int device_ready(void) {
    return bound_device != NULL && bound_device->initialized == 1U &&
           bound_device->read != NULL && bound_device->write != NULL &&
           bound_device->geometry.block_count != 0U &&
           bound_device->geometry.block_count <= UINT32_MAX;
}

static DRESULT map_result(maia_sd_result_t result) {
    switch (result) {
    case MAIA_SD_OK:
        return RES_OK;
    case MAIA_SD_INVALID:
    case MAIA_SD_OUT_OF_RANGE:
        return RES_PARERR;
    case MAIA_SD_NOT_READY:
    case MAIA_SD_BUSY:
    case MAIA_SD_NO_MEDIA:
        return RES_NOTRDY;
    case MAIA_SD_UNSUPPORTED:
    case MAIA_SD_TIMEOUT:
    case MAIA_SD_IO_ERROR:
    default:
        return RES_ERROR;
    }
}

maia_sd_result_t maia_fatfs_disk_bind(maia_sd_block_device_t *device) {
    if (device == NULL || device->initialized != 1U ||
        device->read == NULL || device->write == NULL ||
        device->geometry.block_count == 0U ||
        device->geometry.block_count > UINT32_MAX) {
        return MAIA_SD_INVALID;
    }
    if (bound_device != NULL) return MAIA_SD_BUSY;
    bound_device = device;
    return MAIA_SD_OK;
}

void maia_fatfs_disk_unbind(void) {
    bound_device = NULL;
}

DSTATUS disk_initialize(BYTE pdrv) {
    if (pdrv != 0U || !device_ready()) return STA_NOINIT;
    return 0U;
}

DSTATUS disk_status(BYTE pdrv) {
    if (pdrv != 0U || !device_ready()) return STA_NOINIT;
    return 0U;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0U || buff == NULL || count == 0U) return RES_PARERR;
    if (!device_ready()) return RES_NOTRDY;

    uint32_t completed = 0U;
    return map_result(maia_sd_block_device_read(
        bound_device, (uint64_t)sector, (uint32_t)count, buff, &completed));
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count) {
    if (pdrv != 0U || buff == NULL || count == 0U) return RES_PARERR;
    if (!device_ready()) return RES_NOTRDY;

    uint32_t completed = 0U;
    return map_result(maia_sd_block_device_write(
        bound_device, (uint64_t)sector, (uint32_t)count, buff, &completed));
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    if (pdrv != 0U) return RES_PARERR;
    if (!device_ready()) return RES_NOTRDY;

    switch (cmd) {
    case CTRL_SYNC:
        return RES_OK;
    case GET_SECTOR_COUNT:
        if (buff == NULL) return RES_PARERR;
        *(LBA_t *)buff = (LBA_t)bound_device->geometry.block_count;
        return RES_OK;
    case GET_SECTOR_SIZE:
        if (buff == NULL) return RES_PARERR;
        *(WORD *)buff = (WORD)MAIA_SD_BLOCK_BYTES;
        return RES_OK;
    case GET_BLOCK_SIZE:
        if (buff == NULL) return RES_PARERR;
        *(DWORD *)buff = 1U;
        return RES_OK;
    default:
        return RES_PARERR;
    }
}
