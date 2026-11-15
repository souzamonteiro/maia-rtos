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
    MAIA_PSRAM_GATED=-3,MAIA_PSRAM_CHANGED=-4
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
#ifdef __cplusplus
}
#endif
#endif
