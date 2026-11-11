#include <maia/services/serial_input.h>
#include <maia/drivers/serial_rx.h>
#include <maia/arch/irq.h>
static maia_queue_t input;
static uint8_t storage[MAIA_SERIAL_INPUT_CAPACITY];
static maia_serial_input_stats_t stats;
static int initialized;
maia_status_t maia_serial_input_init(maia_task_id_t owner) {
    uintptr_t irq=maia_arch_irq_save();
    maia_status_t result=MAIA_OK;
    maia_task_info_t info;
    if (initialized) result=MAIA_ERR_BUSY;
    else if (irq || maia_task_current_id() >= 0) result=MAIA_ERR_PERM;
    else if (owner < 0 || maia_task_get_info((uint32_t)owner,&info) != MAIA_OK || info.state != MAIA_TASK_READY)
        result=MAIA_ERR_INVALID;
    else if (maia_serial_rx_claim(&input) != 0) result=MAIA_ERR_BUSY;
    else {
        (void)maia_queue_init(&input,storage,1U,sizeof storage);
        stats.owner=owner;
        initialized=1;
    }
    maia_arch_irq_restore(irq);
    return result;
}
maia_status_t maia_serial_input_poll(void) {
    uintptr_t irq=maia_arch_irq_save();
    if (!initialized || !irq || maia_task_current_id() != stats.owner) {
        maia_status_t status=initialized ? MAIA_ERR_PERM : MAIA_ERR_INVALID;
        maia_arch_irq_restore(irq); return status;
    }
    ++stats.polls;
    maia_arch_irq_restore(irq);
    unsigned moved=0U;
    for (;moved<MAIA_SERIAL_INPUT_BATCH;++moved) {
        if (maia_queue_count(&input) == MAIA_SERIAL_INPUT_CAPACITY) {
            irq=maia_arch_irq_save(); ++stats.full_events; maia_arch_irq_restore(irq);
            return MAIA_ERR_FULL;
        }
        int value=maia_serial_rx_read(&input);
        if (value == -1) break;
        if (value < 0) return MAIA_ERR_PERM;
        uint8_t byte=(uint8_t)value;
        /* Only this producer can fill input; consumers can only free space. */
        irq=maia_arch_irq_save();
        maia_status_t status=maia_queue_send(&input,&byte);
        if (status == MAIA_OK) ++stats.received;
        maia_arch_irq_restore(irq);
        if (status != MAIA_OK) return status;
    }
    return moved ? MAIA_OK : MAIA_ERR_EMPTY;
}
maia_status_t maia_serial_input_read(uint8_t *byte) {
    if (!byte) return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();
    maia_status_t status=initialized ? maia_queue_recv(&input,byte) : MAIA_ERR_INVALID;
    maia_arch_irq_restore(irq);
    return status;
}
maia_status_t maia_serial_input_get_stats(maia_serial_input_stats_t *out) {
    if (!out) return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();
    maia_status_t status=MAIA_ERR_INVALID;
    if (initialized) { *out=stats;out->queued=maia_queue_count(&input);status=MAIA_OK; }
    maia_arch_irq_restore(irq); return status;
}
