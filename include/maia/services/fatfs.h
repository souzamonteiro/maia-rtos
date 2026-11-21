#ifndef MAIA_SERVICES_FATFS_H
#define MAIA_SERVICES_FATFS_H

#include <maia/drivers/sd_read.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum maia_fatfs_result {
    MAIA_FATFS_OK = 0,
    MAIA_FATFS_INVALID = -1,
    MAIA_FATFS_NOT_READY = -2,
    MAIA_FATFS_NO_FILESYSTEM = -3,
    MAIA_FATFS_IO_ERROR = -4,
    MAIA_FATFS_BUSY = -5,
    MAIA_FATFS_FAILURE = -6
} maia_fatfs_result_t;

/* Mounts the single SD volume in thread context. The caller must serialize
 * mount, unmount and all FatFs file/directory calls; this API is not
 * SMP-, ISR- or concurrently callable. Mount never formats or writes media.
 * The block device must remain stable and initialized until unmount. */
maia_fatfs_result_t maia_fatfs_mount(maia_sd_block_device_t *device);

/* Unmount only after every FatFs file and directory object has been closed. */
maia_fatfs_result_t maia_fatfs_unmount(void);

int maia_fatfs_is_mounted(void);

#ifdef __cplusplus
}
#endif
#endif
