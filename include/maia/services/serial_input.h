#ifndef MAIA_SERVICES_SERIAL_INPUT_H
#define MAIA_SERVICES_SERIAL_INPUT_H
#include <maia/kernel.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAIA_SERIAL_INPUT_CAPACITY 64U
#define MAIA_SERIAL_INPUT_BATCH 16U
/* Optional single-core RX service; no task/heap created implicitly.
 * init: boot-only, IRQs disabled, scheduler not started, owner a created READY
 * task. Binding/port claim lasts until reset: owner must live for the service
 * lifetime and its slot must not be reused. INVALID for absent/non-READY owner,
 * PERM outside boot context, BUSY once initialized/port claimed, otherwise OK.
 * poll: owner thread only, IRQs enabled; at most BATCH FIFO reads/enqueues.
 * OK moved bytes, EMPTY no bytes, FULL queue saturated (possibly after moving
 * bytes); INVALID before init, PERM wrong task/masked context. Does not drop an
 * already queued byte or read FIFO while queue full. Not a blocking wait/wakeup.
 * Hardware/host buffering can still overflow if callers exceed service capacity.
 * read/stats: single-core thread/ISR-safe; INVALID for null output/before init.
 * read returns OK/EMPTY and leaves output unchanged on EMPTY. One logical stream;
 * applications coordinate consumers. stats counters wrap modulo 2^32.
 * Legacy console raw RX is denied after init. TX/keyboard/display are separate.
 */
typedef struct maia_serial_input_stats {
    uint32_t received, polls, full_events;
    size_t queued;
    maia_task_id_t owner;
} maia_serial_input_stats_t;
maia_status_t maia_serial_input_init(maia_task_id_t owner);
maia_status_t maia_serial_input_poll(void);
maia_status_t maia_serial_input_read(uint8_t *byte);
maia_status_t maia_serial_input_get_stats(maia_serial_input_stats_t *out);
#ifdef __cplusplus
}
#endif
#endif
