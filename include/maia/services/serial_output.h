#ifndef MAIA_SERVICES_SERIAL_OUTPUT_H
#define MAIA_SERVICES_SERIAL_OUTPUT_H
#include <maia/kernel.h>
#include <maia/drivers/serial_tx.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAIA_SERIAL_OUTPUT_RECORD_BYTES 64U
#define MAIA_SERIAL_OUTPUT_CAPACITY 8U
#define MAIA_SERIAL_OUTPUT_BATCH 16U
/* Optional single-core record FIFO -> one TX owner, one selected sink.
 * init: boot-only/IRQs off, owner is a created READY task; INVALID owner/sink,
 * PERM outside boot context, BUSY on second init/claimed port, otherwise OK.
 * Owner/token binding lasts until reset; owner cannot exit or reuse its slot.
 * write: nonblocking boot/thread/ISR enqueue after init, 1..64 raw bytes copied
 * atomically as one record, no CR/LF conversion. OK accepted, FULL no mutation;
 * INVALID null/zero/oversized/before init. Caller keeps source stable for call.
 * Multiple writers use this API, never driver FIFO. Source must not alias state.
 * poll: owner thread only, IRQs enabled; up to 16 try_put attempts. Pending
 * record/offset survives BUSY. OK bytes accepted, EMPTY no work, BUSY peripheral
 * stalled (possibly after partial progress), PERM wrong context, INVALID before
 * init. Flushed per record/end of poll; FIFO acceptance is not host delivery.
 * stats: thread/ISR-safe local snapshot, INVALID null/before init. Queue has
 * 8 records plus one possible active record; counters wrap uint32. No heap,
 * blocking wait, producer wakeup, timeout/drop policy, teardown or SMP support.
 * TX is exclusive after claim: legacy console writes (including display) are
 * suppressed. Fatal trap emergency sink remains independent and may interrupt
 * the stream on fatal failure; it is not routed through this service.
 */
typedef struct maia_serial_output_stats {
    uint32_t accepted_records, completed_records, accepted_bytes, written_bytes;
    uint32_t polls, busy_events, pending_bytes;
    size_t queued_records;
    maia_task_id_t owner;
    maia_serial_tx_sink_t sink;
} maia_serial_output_stats_t;
maia_status_t maia_serial_output_init(maia_task_id_t owner, maia_serial_tx_sink_t sink);
maia_status_t maia_serial_output_write(const void *data, size_t length);
maia_status_t maia_serial_output_poll(void);
maia_status_t maia_serial_output_get_stats(maia_serial_output_stats_t *out);
#ifdef __cplusplus
}
#endif
#endif
