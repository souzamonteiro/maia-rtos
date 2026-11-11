#include <maia/services/serial_output.h>
#include <maia/arch/irq.h>
typedef struct output_record { uint8_t length, bytes[MAIA_SERIAL_OUTPUT_RECORD_BYTES]; } output_record_t;
static maia_queue_t output;
static output_record_t storage[MAIA_SERIAL_OUTPUT_CAPACITY], pending;
static unsigned offset;
static maia_serial_output_stats_t stats;
static int initialized;
maia_status_t maia_serial_output_init(maia_task_id_t owner, maia_serial_tx_sink_t sink) {
    uintptr_t irq=maia_arch_irq_save();maia_status_t result=MAIA_OK;maia_task_info_t info;
    if (initialized) result=MAIA_ERR_BUSY;
    else if (irq || maia_task_current_id()>=0) result=MAIA_ERR_PERM;
    else if ((sink != MAIA_SERIAL_TX_USB && sink != MAIA_SERIAL_TX_UART0) || owner<0 ||
        maia_task_get_info((uint32_t)owner,&info) != MAIA_OK || info.state != MAIA_TASK_READY) result=MAIA_ERR_INVALID;
    else if (maia_serial_tx_claim(&output) != 0) result=MAIA_ERR_BUSY;
    else {
        (void)maia_queue_init(&output,storage,sizeof(output_record_t),MAIA_SERIAL_OUTPUT_CAPACITY);
        stats.owner=owner;stats.sink=sink;initialized=1;
    }
    maia_arch_irq_restore(irq);return result;
}
maia_status_t maia_serial_output_write(const void *data, size_t length) {
    if (!data || !length || length>MAIA_SERIAL_OUTPUT_RECORD_BYTES) return MAIA_ERR_INVALID;
    output_record_t record;record.length=(uint8_t)length;
    /* Explicit bounded fill: no compiler-generated libc aggregate initializer. */
    for (size_t i=0U;i<MAIA_SERIAL_OUTPUT_RECORD_BYTES;++i)
        record.bytes[i]=i<length ? ((const uint8_t *)data)[i] : 0U;
    uintptr_t irq=maia_arch_irq_save();
    maia_status_t status=initialized ? maia_queue_send(&output,&record) : MAIA_ERR_INVALID;
    if (status == MAIA_OK) {
        ++stats.accepted_records;stats.accepted_bytes+=(uint32_t)length;
        /* Publish first, signal second; owner lifetime is an init contract.
         * Keeping both under local exclusion also covers ISR producers. */
        (void)maia_task_notify(stats.owner);
    }
    maia_arch_irq_restore(irq);return status;
}
maia_status_t maia_serial_output_poll(void) {
    uintptr_t irq=maia_arch_irq_save();
    if (!initialized || !irq || maia_task_current_id() != stats.owner) {
        maia_status_t result=initialized ? MAIA_ERR_PERM : MAIA_ERR_INVALID;
        maia_arch_irq_restore(irq);return result;
    }
    ++stats.polls;maia_arch_irq_restore(irq);
    maia_status_t result=MAIA_ERR_EMPTY;
    for (unsigned attempt=0U;attempt<MAIA_SERIAL_OUTPUT_BATCH;++attempt) {
        irq=maia_arch_irq_save();
        if (!stats.pending_bytes) {
            maia_status_t status=maia_queue_recv(&output,&pending);
            if (status == MAIA_ERR_EMPTY) { maia_arch_irq_restore(irq);break; }
            if (status != MAIA_OK) { maia_arch_irq_restore(irq);return status; }
            offset=0U;stats.pending_bytes=pending.length;
        }
        maia_arch_irq_restore(irq);
        int status=maia_serial_tx_try_put(&output,stats.sink,pending.bytes[offset]);
        if (status == -1) {
            irq=maia_arch_irq_save();++stats.busy_events;maia_arch_irq_restore(irq);
            result=MAIA_ERR_BUSY;break;
        }
        if (status != 0) return MAIA_ERR_PERM;
        irq=maia_arch_irq_save();
        ++offset;--stats.pending_bytes;++stats.written_bytes;
        if (!stats.pending_bytes) ++stats.completed_records;
        int complete=stats.pending_bytes == 0U;
        maia_arch_irq_restore(irq);
        if (complete && maia_serial_tx_flush(&output,stats.sink) != 0) return MAIA_ERR_PERM;
        result=MAIA_OK;
    }
    if (maia_serial_tx_flush(&output,stats.sink) != 0) return MAIA_ERR_PERM;
    return result;
}
maia_status_t maia_serial_output_wait(void) {
    uintptr_t irq=maia_arch_irq_save();
    if (!initialized || !irq || maia_task_current_id() != stats.owner) {
        maia_status_t result=initialized ? MAIA_ERR_PERM : MAIA_ERR_INVALID;
        maia_arch_irq_restore(irq);return result;
    }
    int work=stats.pending_bytes != 0U || maia_queue_count(&output) != 0U;
    maia_arch_irq_restore(irq);
    if (work) return MAIA_OK;
    /* If a producer runs here, its sticky notification survives until the trap.
     * Waking is a hint: poll/recheck the predicate, including coalesced events. */
    return maia_task_wait_notification(MAIA_TASK_WAIT_FOREVER);
}
maia_status_t maia_serial_output_get_stats(maia_serial_output_stats_t *out) {
    if (!out) return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();maia_status_t result=MAIA_ERR_INVALID;
    if (initialized) { *out=stats;out->queued_records=maia_queue_count(&output);result=MAIA_OK; }
    maia_arch_irq_restore(irq);return result;
}
