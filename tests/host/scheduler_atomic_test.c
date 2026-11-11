#include <maia/kernel.h>
#include <maia/arch/irq.h>
#include <maia/arch/riscv_context.h>
#include <maia/scheduler_internal.h>
#include <stdio.h>

static uintptr_t irq_state = 8U;
static unsigned saves, restores, inject, in_irq, injected, failure;
static uint8_t stacks[MAIA_MAX_TASKS][1024];

uintptr_t maia_arch_irq_save(void) {
    uintptr_t old = irq_state; irq_state = 0U; ++saves; return old;
}
void maia_arch_irq_restore(uintptr_t old) {
    irq_state = old & 8U; ++restores;
    if (irq_state && inject && !in_irq) {
        --inject; in_irq = 1U; irq_state = 0U;
        (void)maia_sched_tick_isr();
        uintptr_t sp = maia_sched_select_next_sp(0U);
        if (sp == 0U || (sp & 15U)) failure = 1U;
        else {
            const uint32_t *frame = (const uint32_t *)sp;
            if (frame[29] != MAIA_RISCV_INITIAL_MSTATUS ||
                frame[30] != MAIA_RISCV_TASK_RETURN_FLAGS) failure = 2U;
        }
        if (irq_state) failure = 3U;
        ++injected; in_irq = 0U; irq_state = 8U;
    }
}
void maia_arch_start_first_task(uintptr_t sp);
void maia_arch_start_first_task(uintptr_t sp) { if (!sp || irq_state) failure = 4U; }
static void task(void *arg) { (void)arg; }
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d\n", __LINE__); return 1; } } while (0)

int main(void) {
    uintptr_t outer = maia_arch_irq_save(), inner = maia_arch_irq_save();
    CHECK(outer == 8U && inner == 0U && irq_state == 0U);
    maia_arch_irq_restore(inner); CHECK(irq_state == 0U);
    maia_arch_irq_restore(outer); CHECK(irq_state == 8U);
    CHECK(maia_sched_init() == MAIA_OK);
    maia_task_id_t a, b, c;
    CHECK(maia_task_create("A",task,NULL,3U,stacks[0],sizeof stacks[0],&a) == MAIA_OK);
    CHECK(maia_task_create("B",task,NULL,1U,stacks[1],sizeof stacks[1],&b) == MAIA_OK);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == a);
    maia_sched_test_set_tick(UINT64_MAX - 1U);
    CHECK(maia_sched_delay_current_isr(3U) == MAIA_OK);
    maia_task_info_t info;
    CHECK(maia_task_get_info((uint32_t)a,&info) == MAIA_OK && info.wake_tick == 1U);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == b);
    for (unsigned i=0U; i<2U; ++i) {
        (void)maia_sched_tick_isr();
        CHECK(maia_task_get_info((uint32_t)a,&info) == MAIA_OK && info.state == MAIA_TASK_SLEEPING);
    }
    (void)maia_sched_tick_isr();
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == a);
    CHECK(maia_sched_exit_current_isr() == MAIA_OK);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == b);
    CHECK(maia_task_create("C",task,NULL,3U,stacks[0],sizeof stacks[0],&c) == MAIA_OK);
    CHECK(c == a && maia_task_count() == 2U);
    CHECK(maia_sched_init() == MAIA_OK); /* Scheduler has not started in this model. */
    for (unsigned i=0U; i<MAIA_MAX_TASKS; ++i) {
        inject = 1U;
        CHECK(maia_task_create("publish",task,NULL,2U,stacks[i],sizeof stacks[i],NULL) == MAIA_OK);
        CHECK(!failure && irq_state == 8U);
    }
    CHECK(injected == MAIA_MAX_TASKS && maia_task_count() == MAIA_MAX_TASKS);
    CHECK(maia_task_create("full",task,NULL,2U,stacks[0],sizeof stacks[0],NULL) == MAIA_ERR_NOMEM);
    maia_sched_test_set_tick(UINT64_C(0xffffffff));
    unsigned old_saves = saves;
    inject = 1U;
    uint64_t snapshot = maia_tick_get();
    CHECK(saves > old_saves && snapshot == UINT64_C(0xffffffff));
    CHECK(maia_tick_get() == UINT64_C(0x100000000));
    outer = maia_arch_irq_save();
    maia_trap_stats_t stats;
    maia_trap_get_stats(&stats); (void)maia_tick_get();
    CHECK(maia_task_get_info(0U,&info) == MAIA_OK && irq_state == 0U);
    maia_arch_irq_restore(outer);
    CHECK(irq_state == 8U && saves == restores);
    maia_sched_start(); CHECK(!failure && irq_state == 8U);
    CHECK(maia_sched_init() == MAIA_ERR_BUSY);
    outer = maia_arch_irq_save();
    CHECK(maia_task_create("masked",task,NULL,2U,stacks[0],sizeof stacks[0],NULL) == MAIA_ERR_PERM);
    CHECK(irq_state == 0U);
    maia_arch_irq_restore(outer);
    CHECK(saves == restores && !failure);
    puts("PASS: local IRQ nesting, atomic publication/snapshots, rollover deadlines, slot reuse and context contracts");
    return 0;
}
