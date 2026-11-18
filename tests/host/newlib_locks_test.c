#include <maia/arch/irq.h>
#include <maia/runtime/newlib_locks.h>

#include <assert.h>
#include <stdint.h>
#include <sys/lock.h>

static uint8_t heap[8192] __attribute__((aligned(16)));
static maia_task_id_t current_task = -1;
static uintptr_t irq_state = 8U;
static _LOCK_T yield_lock;
static unsigned yield_count;

extern struct __lock __lock___malloc_recursive_mutex;

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

void maia_task_yield(void) {
    ++yield_count;
    if (yield_lock != NULL) {
        maia_task_id_t waiting_task = current_task;
        current_task = 1;
        __retarget_lock_release_recursive(yield_lock);
        current_task = waiting_task;
        yield_lock = NULL;
    }
}

int main(void) {
    assert(maia_newlib_locks_init() == MAIA_ERR_INVALID);
    assert(maia_mem_init(heap, sizeof heap) == MAIA_OK);
    assert(maia_newlib_locks_init() == MAIA_OK);
    assert(maia_newlib_locks_init() == MAIA_ERR_BUSY);

    _LOCK_RECURSIVE_T recursive = NULL;
    __retarget_lock_init_recursive(&recursive);
    assert(recursive != NULL);

    current_task = 1;
    assert(__retarget_lock_try_acquire_recursive(recursive) == 1);
    assert(__retarget_lock_try_acquire_recursive(recursive) == 1);
    assert(irq_state == 8U);

    current_task = 2;
    assert(__retarget_lock_try_acquire_recursive(recursive) == 0);
    current_task = 1;
    __retarget_lock_release_recursive(recursive);
    __retarget_lock_release_recursive(recursive);

    irq_state = 0U;
    assert(__retarget_lock_try_acquire_recursive(recursive) == 1);
    assert(irq_state == 0U);
    __retarget_lock_release_recursive(recursive);
    assert(irq_state == 0U);
    irq_state = 8U;

    _LOCK_T normal = NULL;
    __retarget_lock_init(&normal);
    assert(normal != NULL);
    assert(__retarget_lock_try_acquire(normal) == 1);
    assert(__retarget_lock_try_acquire(normal) == 0);
    current_task = 2;
    assert(__retarget_lock_try_acquire(normal) == 0);
    current_task = 1;
    __retarget_lock_release(normal);

    assert(__retarget_lock_try_acquire_recursive(recursive) == 1);
    current_task = 2;
    yield_lock = recursive;
    unsigned before_yield = yield_count;
    __retarget_lock_acquire_recursive(recursive);
    assert(yield_count == before_yield + 1U);
    __retarget_lock_release_recursive(recursive);

    _LOCK_RECURSIVE_T static_lock = &__lock___malloc_recursive_mutex;
    __retarget_lock_init_recursive(&static_lock);
    assert(__retarget_lock_try_acquire_recursive(static_lock) == 1);
    __retarget_lock_release_recursive(static_lock);
    __retarget_lock_close_recursive(static_lock);

    __retarget_lock_close(normal);
    __retarget_lock_close_recursive(recursive);
    maia_mem_stats_t stats;
    assert(maia_mem_get_stats(&stats) == MAIA_OK);
    assert(stats.used_bytes == 0U);
    return 0;
}
