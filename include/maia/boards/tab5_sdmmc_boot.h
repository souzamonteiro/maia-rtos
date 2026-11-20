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
    MAIA_SDMMC_BOOT_FAILED = -10
} maia_sdmmc_boot_result_t;

typedef enum maia_sdmmc_boot_stage {
    MAIA_SDMMC_BOOT_NONE = 0,
    MAIA_SDMMC_BOOT_GUARDED,
    MAIA_SDMMC_BOOT_TIMER,
    MAIA_SDMMC_BOOT_PINS,
    MAIA_SDMMC_BOOT_RESET,
    MAIA_SDMMC_BOOT_CLOCK,
    MAIA_SDMMC_BOOT_CMD0,
    MAIA_SDMMC_BOOT_CMD8
} maia_sdmmc_boot_stage_t;

typedef struct maia_sdmmc_boot_report {
    maia_sdmmc_boot_stage_t completed;
    uint32_t last_command;
    uint32_t cmd0_raw_status;
    uint32_t raw_status;
    uint32_t response;
    uint32_t host_status;
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

#ifdef __cplusplus
}
#endif
#endif
