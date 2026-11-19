#include <maia/arch/irq.h>
#include <maia/runtime/newlib_reent.h>

#include <reent.h>

static maia_task_id_t current_task = -1;
static uintptr_t irq_state = 8U;
static unsigned reclaim_count;

uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous = irq_state;
    irq_state = 0U;
    return previous;
}

void maia_arch_irq_restore(uintptr_t previous) {
    irq_state = previous & 8U;
}

maia_task_id_t maia_task_current_id(void) {
    return current_task;
}

void _reclaim_reent(struct _reent *reent) {
    ++reclaim_count;
    reent->initialized = 0U;
}

int main(void) {
    struct _reent *idle = __getreent();
    if (idle == NULL || idle->_errno != 0 || idle->initialized != 1U) return 1;
    idle->_errno = 13;

    current_task = 3;
    struct _reent *first = __getreent();
    if (first == NULL || first == idle || first->_errno != 0) return 2;
    first->_errno = 27;
    if (__getreent() != first || first->_errno != 27) return 3;

    current_task = 4;
    struct _reent *second = __getreent();
    if (second == first || second == idle || second->_errno != 0) return 4;
    second->_errno = 31;

    maia_newlib_task_reclaim(3);
    if (reclaim_count != 1U || first->initialized != 0U) return 5;
    current_task = 3;
    if (__getreent() != first || first->_errno != 0 || first->initialized != 1U) return 6;

    current_task = -1;
    if (__getreent() != idle || idle->_errno != 13) return 7;
    if (irq_state != 8U) return 8;
    return 0;
}
