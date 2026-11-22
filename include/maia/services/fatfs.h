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

/* Mounts the single SD volume in boot or quiescent thread context. Mount and
 * unmount must not overlap VFS calls; VFS serializes its own FatFs operations
 * and returns BUSY on contention. Do not call FatFs APIs directly. Mount never
 * formats or writes media. The block device must remain valid until unmount. */
maia_fatfs_result_t maia_fatfs_mount(maia_sd_block_device_t *device);

/* Unmount after all VFS file/directory handles close and callers are quiescent. */
maia_fatfs_result_t maia_fatfs_unmount(void);

int maia_fatfs_is_mounted(void);

#if defined(MAIA_FATFS_DIAGNOSTIC_FAULT_INJECTION)
typedef enum maia_fatfs_diag_io {
    MAIA_FATFS_DIAG_READ = 1,
    MAIA_FATFS_DIAG_WRITE = 2
} maia_fatfs_diag_io_t;

/* Diagnostic-only one-shot diskio failure, consumed before SD hardware access. */
void maia_fatfs_diag_fail_next(maia_fatfs_diag_io_t operation);
/* Clear an unconsumed fault so it cannot affect a later disk operation. */
void maia_fatfs_diag_clear_pending_fault(void);
uint32_t maia_fatfs_diag_faults_triggered(void);
#endif

#ifdef __cplusplus
}
#endif
#endif
