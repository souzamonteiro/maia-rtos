#ifndef MAIA_BOARDS_TAB5_SDMMC_BOOT_H
#define MAIA_BOARDS_TAB5_SDMMC_BOOT_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum maia_sdmmc_boot_result {
    MAIA_SDMMC_BOOT_OK = 0,
    MAIA_SDMMC_BOOT_INVALID = -1,
    MAIA_SDMMC_BOOT_CONTEXT = -2,
    MAIA_SDMMC_BOOT_BUSY = -3,
    MAIA_SDMMC_BOOT_POWER = -4,
    MAIA_SDMMC_BOOT_HOST_STATE = -5,
    MAIA_SDMMC_BOOT_VERIFY = -6,
    MAIA_SDMMC_BOOT_TIMEOUT = -7,
    MAIA_SDMMC_BOOT_IO = -8,
    MAIA_SDMMC_BOOT_RESPONSE = -9,
    MAIA_SDMMC_BOOT_FAILED = -10,
    MAIA_SDMMC_BOOT_UNSUPPORTED = -11
} maia_sdmmc_boot_result_t;

typedef enum maia_sdmmc_boot_stage {
    MAIA_SDMMC_BOOT_NONE = 0,
    MAIA_SDMMC_BOOT_GUARDED,
    MAIA_SDMMC_BOOT_TIMER,
    MAIA_SDMMC_BOOT_PINS,
    MAIA_SDMMC_BOOT_RESET,
    MAIA_SDMMC_BOOT_CLOCK,
    MAIA_SDMMC_BOOT_CMD0,
    MAIA_SDMMC_BOOT_CMD8,
    MAIA_SDMMC_BOOT_OCR,
    MAIA_SDMMC_BOOT_CID,
    MAIA_SDMMC_BOOT_RCA,
    MAIA_SDMMC_BOOT_CSD,
    MAIA_SDMMC_BOOT_SELECTED,
    MAIA_SDMMC_BOOT_BLOCK
} maia_sdmmc_boot_stage_t;

typedef struct maia_sdmmc_boot_report {
    maia_sdmmc_boot_stage_t completed;
    uint32_t last_command;
    uint32_t cmd0_raw_status;
    uint32_t raw_status;
    uint32_t response;
    uint32_t host_status;
    uint32_t ocr;
    uint32_t rca;
    uint32_t cid[4];
    uint32_t csd[4];
    uint64_t block_count;
    uint32_t block_lba;
    uint32_t second_block_lba;
    uint32_t block_read_count;
    uint32_t block_write_count;
    uint32_t block_raw_status;
    uint32_t block_host_status;
    uint32_t block_fifoth_before;
    uint32_t block_fifoth;
    uint32_t block_intmask;
    uint32_t block_cardthrctl_before;
    uint32_t block_cardthrctl;
    uint32_t block_dma_cache_line;
    uint32_t block_dma_cache_status;
    uint32_t block_dma_status;
    uint32_t block_dma_ctrl;
    uint32_t block_dma_bmod;
    uint32_t block_dma_inten;
    uint32_t block_dma_dbaddr;
    uint32_t block_dma_desc0;
    uint32_t block_dma_desc1;
    uint32_t block_dma_desc2;
    uint32_t block_dma_desc3;
    uint8_t block[512];
    uint8_t second_block[512];
} maia_sdmmc_boot_report_t;

/* Tab5 slot-0 diagnostic, core 0 before the scheduler with interrupts disabled.
 * The caller must own SDHOST, GPIO39/42/43/44, VO4 and SYSTIMER UNIT0 and keep
 * the other core stopped. Only an already enabled, software-selected VO4 in
 * bypass and an idle, clocked controller are accepted. It configures a nominal
 * 400 kHz, 1-bit bus, samples D3 high and sends CMD0 and CMD8, validating the
 * response index, R7 voltage/pattern and controller CRC.
 * It does not initialize/select a card, transfer data, write the medium, or
 * initialize VO4. This is not a storage driver. SDHC support is not established
 * by CMD8 alone.
 *
 * A failure after the first write can leave clocks, pins or the card changed;
 * no retry or teardown is supported before reset. A successful call also keeps
 * ownership until reset. Every wait has a finite poll bound. The report is
 * initialized for valid pointers and records the last completed stage/status.
 * Not ISR-safe or SMP-safe. */
maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_cmd8(maia_sdmmc_boot_report_t *report);

/* Isolated Tab5 slot-0 bring-up. Performs CMD0/CMD8, bounded
 * ACMD41, CID/RCA/CSD identification, selects the card, validates SDHC/SDXC
 * geometry and reads exactly one caller-selected LBA using the internal DMA
 * controller. The descriptor and buffer use a private, 128-byte-aligned SRAM
 * reservation, ROM cache writeback/invalidate and the P4 non-cacheable alias.
 * The 512-byte block is returned only after controller and IDMAC completion,
 * card-error checks, descriptor ownership return, 512-byte card and DMA
 * transfer counts, and DMA shutdown. This is a
 * one-block diagnostic, not yet a general storage driver. The LBA must fit
 * the SDHC 32-bit block-address argument and be within the decoded capacity.
 * It does not write the medium, access a filesystem, change bus width/frequency,
 * or initialize VO4. Same boot-context,
 * exclusive-ownership, failure-sticky, timeout and no-teardown constraints
 * as maia_tab5_sdmmc_boot_cmd8 apply. */
maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_read_block(
    maia_sdmmc_boot_report_t *report, uint64_t lba);

maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_read_two_blocks(
    maia_sdmmc_boot_report_t *report, uint64_t first_lba, uint64_t second_lba);

maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_read_block0(
    maia_sdmmc_boot_report_t *report);

/* Writes exactly one 512-byte block using CMD24 and IDMAC TX. Success requires
 * exact controller/DMA byte counts, returned descriptor ownership, DMA
 * shutdown, and a subsequent CMD13 reporting READY_FOR_DATA in TRAN. A failed
 * operation may still have changed the card; block_write_count counts only
 * writes confirmed ready by CMD13. This is a boot-only diagnostic, not a
 * general storage driver. The caller must provide a safe test LBA and preserve
 * its original contents before invoking it. The input buffer must not overlap
 * the report object. */
maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_write_block(
    maia_sdmmc_boot_report_t *report, uint64_t lba, const uint8_t block[512]);

#ifdef __cplusplus
}
#endif
#endif
