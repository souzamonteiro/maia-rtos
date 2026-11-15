/* Original MaiaRTOS MIT. ESP32-P4 PSRAM controller observation only.
 * Pinned primary sources: docs/audits/native-psram-state.md. */
#ifndef MAIA_ESP32P4_PSRAM_H
#define MAIA_ESP32P4_PSRAM_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAIA_PSRAM_ROOT_WORDS 9U
#define MAIA_PSRAM_CONTROLLER_WORDS 27U
typedef enum maia_psram_result {
    MAIA_PSRAM_OK=0,MAIA_PSRAM_INVALID=-1,MAIA_PSRAM_CONTEXT=-2,
    MAIA_PSRAM_GATED=-3,MAIA_PSRAM_CHANGED=-4,MAIA_PSRAM_BUSY=-5,
    MAIA_PSRAM_FAILED=-6,MAIA_PSRAM_VERIFY=-7,MAIA_PSRAM_TIMEOUT=-8,MAIA_PSRAM_IO=-9
} maia_psram_result_t;
typedef struct maia_psram_snapshot {
    uint32_t roots[MAIA_PSRAM_ROOT_WORDS],roots_after[MAIA_PSRAM_ROOT_WORDS];
    uint32_t before[MAIA_PSRAM_CONTROLLER_WORDS],after[MAIA_PSRAM_CONTROLLER_WORDS];
    uint32_t roots_valid,controller_samples;
} maia_psram_snapshot_t;
/* Boot/core0/IRQs-off/exclusive access only, not thread/ISR-safe. Required report
 * initialized before context/MMIO checks. INVALID(null)/CONTEXT cause no MMIO.
 * Read-only bounded whitelist: RCC, VO2/eFuse and PSRAM controllers only; no flash
 * controller, PSRAM array, cache/MMU table/selector, FIFO pop or clear-on-read.
 * Conservative access policy requires PSRAM SYS/PLL/core gates on, dual MSPI
 * resets off. Otherwise GATED with root evidence and zero controller reads.
 * Fresh RCC comparison before each controller read; roots rechecked at end.
 * OK means two matching observations, not atomic snapshot/readiness/ownership/
 * initialization/PSRAM identity, frequency, allocation or coherency proof.
 * CHANGED retains partial evidence and sample count; no retry/repair/store.
 * No polling/waits/heap or changes to controller/power/cache/clock state. */
maia_psram_result_t maia_esp32p4_psram_snapshot(maia_psram_snapshot_t *);
typedef struct maia_psram_mode_report {
    uint32_t reg,bits,stage,setup_attempts,restore_attempts,triggered,polls,saw_busy,command,value,restored;
    uint32_t before[10],after[10];
} maia_psram_mode_report_t;
/* Boot/core0/IRQs-off/exclusive, all code/data internal SRAM, no PSRAM AXI users.
 * INHERITED powered/timed HEX device only: not cold PSRAM initialization.
 * Accept only mode-reg addresses0,2,4,8 and budget1..100000. Read4040/16-bit cmd,
 * 32-bit address,12 dummy bits as preserved200M HEX register protocol; reg0/2 read
 * 16bits, reg4/8 read8. Caller must first qualify device/latency applicability.
 * No PSRAM device write, array, clock, LDO, MMU/cache or IRQ changes. Temporarily
 * configure only PSRAM command port, poison receive word, issue bounded read;
 * compare roots/untouched AXI side and restore saved registers only after idle.
 * MSPI2 AXI requests disabled and both FSMs idle required. No ROM SPI helpers.
 * Triggered means a start-write attempt, not an ACK; response must replace poison.
 * Report initialized; invalid/context and prior FAILED cause no MMIO. BUSY/
 * GATED/CHANGED before claim do not store. Partial write/timeout/lost ownership
 * sticky until reboot; no retry/abort/unsafe active restore. Bad/stale/foreign
 * vendor response returns IO after guarded restore, remains sticky. Successful
 * fully restored calls may repeat. No deadline, cold-init or memory-ready claim. */
maia_psram_result_t maia_esp32p4_psram_read_mode(uint32_t reg,uint32_t poll_budget,maia_psram_mode_report_t *);
#ifdef __cplusplus
}
#endif
#endif
