#ifndef MAIA_SOC_ESP32P4_CORE1_H
#define MAIA_SOC_ESP32P4_CORE1_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAIA_CORE1_PROBE_ROUNDS 1024U
#define MAIA_CORE1_START_ROUNDS 64U

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

typedef struct {
    uint32_t online, hart, pings;
    uint32_t heartbeat_before, heartbeat_after;
    uint32_t cause, pc;
} maia_core1_status_t;

/* Isolated diagnostic only: core 0, boot-time, IRQs off, once per reset.
 * The secondary must still belong to ROM startup; not a runtime takeover API.
 * Starts an SRAM-only worker, checks bounded atomic exchanges, then parks it.
 * No scheduler, libc, drivers, TLS, or external memory may run on core 1.
 * Failure after release requires a chip reset; no retry or state rollback.
 * Returns an explicit error and partial report; does not print or allocate.
 */
maia_core1_result_t maia_esp32p4_core1_probe(maia_core1_report_t *report);

/* Product start: same boot-only contract and SRAM-only worker as the probe,
 * but after MAIA_CORE1_START_ROUNDS verified exchanges the worker stays online
 * (heartbeat + mailbox service) instead of parking. Mutually exclusive with
 * the probe. Core 1 still runs no scheduler, libc, drivers, TLS, flash/PSRAM.
 */
maia_core1_result_t maia_esp32p4_core1_start(maia_core1_report_t *report);

/* Runtime health check from any core-0 task: one bounded mailbox exchange.
 * Returns BUSY on concurrent callers and CONTEXT when core 1 is not online;
 * a timeout/fault marks core 1 offline (a chip reset is required to recover).
 */
maia_core1_result_t maia_esp32p4_core1_ping(maia_core1_status_t *status);

/* SMP hand-off: boot-only on core 0, after a successful start. The SRAM
 * worker jumps to `entry` with sp = `stack_top` and never returns to the
 * mailbox; the entry then owns core 1 (trap port, CLIC, scheduler).
 * After launch, ping reports the scheduler's heartbeat instead. */
maia_core1_result_t maia_esp32p4_core1_launch(uintptr_t entry, uintptr_t stack_top);
int maia_esp32p4_core1_launched(void);

#ifdef __cplusplus
}
#endif
#endif
