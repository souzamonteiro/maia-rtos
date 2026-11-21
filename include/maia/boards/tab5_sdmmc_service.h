#ifndef MAIA_BOARDS_TAB5_SDMMC_SERVICE_H
#define MAIA_BOARDS_TAB5_SDMMC_SERVICE_H

#include <maia/boards/tab5_sdmmc_boot.h>
#include <maia/drivers/sd_read.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAIA_TAB5_SDMMC_DMA_ALIGNMENT 128U
#define MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES 640U

/* Initialize the single slot-0 service from core 0, before the scheduler and
 * with interrupts disabled. The caller supplies a persistent, 128-byte-aligned
 * internal-SRAM buffer; it remains owned by this service until reset. VO4 must
 * already be verified enabled in bypass and the SD host must be idle and
 * clocked. This initializes/selects the card and claims the SD host and pins.
 * Failure after hardware access is sticky; no retry or teardown is supported.
 *
 * The Tab5 pin map has no connected card-detect signal. Initialization errors
 * therefore cannot distinguish absent media from other communication faults.
 * The service uses bounded synchronous requests in task context and returns
 * BUSY for overlapping callers; it is not ISR- or SMP-safe. */
maia_sdmmc_boot_result_t maia_tab5_sdmmc_service_init(
    void *dma_storage, size_t dma_storage_bytes,
    maia_sdmmc_boot_report_t *report);

/* Returns the stable service device after successful initialization, else NULL. */
maia_sd_block_device_t *maia_tab5_sdmmc_service_device(void);

#ifdef __cplusplus
}
#endif
#endif
