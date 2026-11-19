#include <maia/arch/irq.h>
#include <maia/arch/riscv_stack.h>
#include <maia/runtime/newlib_reent.h>

#include <reent.h>
#include <string.h>

#define NEWLIB_REENT_PANIC 0x4D52454EU
#define NEWLIB_REENT_IDLE_SLOT MAIA_MAX_TASKS

static struct _reent g_task_reent[MAIA_MAX_TASKS + 1U];
static uint8_t g_task_reent_initialized[MAIA_MAX_TASKS + 1U];

static __attribute__((noreturn)) void reent_fault(uint32_t reason) {
#if defined(__riscv)
    uintptr_t sp;
    __asm__ volatile("mv %0, sp" : "=r"(sp));
    maia_arch_stack_panic(NEWLIB_REENT_PANIC, 0U, reason, sp, MAIA_STACK_BAD_CONTEXT);
#else
    (void)reason;
    __builtin_trap();
#endif
}

static uint32_t current_reent_slot(void) {
    maia_task_id_t task_id = maia_task_current_id();
    if (task_id == -1) return NEWLIB_REENT_IDLE_SLOT;
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) reent_fault(1U);
    return (uint32_t)task_id;
}

struct _reent *__getreent(void) {
    uint32_t slot = current_reent_slot();
    uintptr_t irq = maia_arch_irq_save();
    if (!g_task_reent_initialized[slot]) {
        _REENT_INIT_PTR(&g_task_reent[slot]);
        g_task_reent_initialized[slot] = 1U;
    }
    maia_arch_irq_restore(irq);
    return &g_task_reent[slot];
}

void maia_newlib_task_reclaim(maia_task_id_t task_id) {
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) reent_fault(2U);
    uint32_t slot = (uint32_t)task_id;
    uintptr_t irq = maia_arch_irq_save();
    uint8_t initialized = g_task_reent_initialized[slot];
    maia_arch_irq_restore(irq);
    if (!initialized) return;

    _reclaim_reent(&g_task_reent[slot]);

    irq = maia_arch_irq_save();
    g_task_reent_initialized[slot] = 0U;
    maia_arch_irq_restore(irq);
}
