#ifndef MAIA_SOC_ESP32P4_CORE1_H
#define MAIA_SOC_ESP32P4_CORE1_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAIA_CORE1_PROBE_ROUNDS 1024U

typedef enum {
    MAIA_CORE1_OK = 0,
    MAIA_CORE1_INVALID = -1,
    MAIA_CORE1_CONTEXT = -2,
    MAIA_CORE1_BUSY = -3,
    MAIA_CORE1_VERIFY = -4,
    MAIA_CORE1_TIMEOUT = -5,
    MAIA_CORE1_FAULT = -6
} maia_core1_result_t;

typedef struct {
    uint32_t clock_before, reset_before, stall_before, boot_before;
    uint32_t completed, hart, stack_pointer, rounds;
    uint32_t cause, pc;
} maia_core1_report_t;

/* Isolated diagnostic only: core 0, boot-time, IRQs off, once per reset.
 * The secondary must still belong to ROM startup; not a runtime takeover API.
 * Starts an SRAM-only worker, checks bounded atomic exchanges, then parks it.
 * No scheduler, libc, drivers, TLS, or external memory may run on core 1.
 * Failure after release requires a chip reset; no retry or state rollback.
 * Returns an explicit error and partial report; does not print or allocate.
 */
maia_core1_result_t maia_esp32p4_core1_probe(maia_core1_report_t *report);

#ifdef __cplusplus
}
#endif
#endif
