#ifndef MAIA_DRIVERS_STORAGE_FATFS_DISKIO_H
#define MAIA_DRIVERS_STORAGE_FATFS_DISKIO_H

#include <maia/drivers/sd_read.h>

maia_sd_result_t maia_fatfs_disk_bind(maia_sd_block_device_t *device);
void maia_fatfs_disk_unbind(void);

#endif
