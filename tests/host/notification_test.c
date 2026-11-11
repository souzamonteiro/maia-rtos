#include <maia/kernel.h>
#include <maia/scheduler_internal.h>
#include <maia/arch/riscv_stack.h>
#include <maia/arch/irq.h>
#include <stdio.h>
static uint8_t stacks[2][1024] __attribute__((aligned(16)));
static uintptr_t irq_state=8U,a_sp,b_sp;
static maia_task_id_t a,b;
static unsigned saves,restores,inject,arch_calls,failure;
uintptr_t maia_arch_irq_save(void) { uintptr_t old=irq_state;irq_state=0U;++saves;return old; }
void maia_arch_irq_restore(uintptr_t old) {
    irq_state=old&8U;++restores;
    if (irq_state && inject) {
        inject=0U;irq_state=0U;
        if (maia_task_notify(a) != MAIA_OK || irq_state) ++failure;
        irq_state=8U;
    }
}
/* Only models immediate waits; blocked/native C resumption is hardware-tested. */
maia_status_t maia_arch_wait_notification_trap(uint32_t ticks) {
    ++arch_calls;
    if (maia_sched_wait_notification_isr(ticks) != MAIA_OK) ++failure;
    maia_task_info_t info;(void)maia_task_get_info((uint32_t)a,&info);
    if (info.state == MAIA_TASK_BLOCKED) { ++failure;return MAIA_ERR_BUSY; }
    uintptr_t sp=maia_sched_select_next_sp(a_sp);
    return (maia_status_t)(int32_t)((uint32_t *)sp)[6];
}
static void task(void *arg) { (void)arg; }
static int setup(void) {
    if (maia_sched_init() != MAIA_OK ||
        maia_task_create("A",task,NULL,3U,stacks[0],sizeof stacks[0],&a) != MAIA_OK ||
        maia_task_create("B",task,NULL,2U,stacks[1],sizeof stacks[1],&b) != MAIA_OK) return 0;
    a_sp=maia_sched_select_next_sp(0U);
    b_sp=maia_stack_top((uintptr_t)stacks[1],sizeof stacks[1])-MAIA_RISCV_CONTEXT_BYTES;
    return a_sp != 0U && maia_task_current_id() == a;
}
static maia_task_state_t state(maia_task_id_t id) {
    maia_task_info_t info;
    if (maia_task_get_info((uint32_t)id,&info) != MAIA_OK) ++failure;
    return info.state;
}
static int result_is(maia_status_t expected) { return (int32_t)((uint32_t *)a_sp)[6] == (int32_t)expected; }
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d\n",__LINE__);return 1; } } while(0)
int main(void) {
    CHECK(maia_sched_init() == MAIA_OK);
    CHECK(maia_task_wait_notification(1U) == MAIA_ERR_PERM && !arch_calls);
    CHECK(maia_task_notify(-1) == MAIA_ERR_INVALID && maia_task_notify(MAIA_MAX_TASKS) == MAIA_ERR_INVALID && maia_task_notify(7) == MAIA_ERR_INVALID);
    CHECK(setup());
    uintptr_t outer=maia_arch_irq_save();
    CHECK(maia_task_wait_notification(1U) == MAIA_ERR_PERM && !arch_calls && !irq_state);
    CHECK(maia_task_notify(a) == MAIA_OK && !irq_state);
    maia_arch_irq_restore(outer);
    CHECK(maia_task_notify(a) == MAIA_OK);
    CHECK(maia_task_wait_notification(0U) == MAIA_OK);
    ((uint32_t *)a_sp)[6]=0xcafebabeU;
    CHECK(maia_sched_select_next_sp(a_sp) == a_sp && ((uint32_t *)a_sp)[6] == 0xcafebabeU);
    CHECK(maia_task_wait_notification(0U) == MAIA_ERR_EMPTY);
    inject=1U;CHECK(maia_task_wait_notification(7U) == MAIA_OK && !inject); /* Between validation and trap. */
    CHECK(!failure);
    CHECK(setup());
    ((uint32_t *)a_sp)[6]=0xabcdef12U;((uint32_t *)b_sp)[6]=0x13572468U;
    CHECK(maia_sched_wait_notification_isr(2U) == MAIA_OK && state(a) == MAIA_TASK_BLOCKED);
    CHECK(maia_sched_select_next_sp(a_sp) == b_sp && ((uint32_t *)b_sp)[6] == 0x13572468U);
    CHECK(((uint32_t *)a_sp)[6] == 0xabcdef12U);
    CHECK(maia_task_notify(a) == MAIA_OK && state(a) == MAIA_TASK_READY);
    CHECK(maia_task_notify(a) == MAIA_OK); /* First completes, second remains pending. */
    (void)maia_sched_tick_isr();(void)maia_sched_tick_isr();
    CHECK(maia_sched_select_next_sp(b_sp) == a_sp && result_is(MAIA_OK));
    CHECK(maia_sched_wait_notification_isr(0U) == MAIA_OK);
    CHECK(maia_sched_select_next_sp(a_sp) == a_sp && result_is(MAIA_OK));
    CHECK(maia_sched_wait_notification_isr(0U) == MAIA_OK);
    CHECK(maia_sched_select_next_sp(a_sp) == a_sp && result_is(MAIA_ERR_EMPTY));
    CHECK(setup());
    CHECK(maia_sched_wait_notification_isr(1U) == MAIA_OK && maia_sched_select_next_sp(a_sp) == b_sp);
    (void)maia_sched_tick_isr();CHECK(state(a) == MAIA_TASK_READY);
    CHECK(maia_task_notify(a) == MAIA_OK); /* Timeout won before scheduling; retain late notification. */
    CHECK(maia_sched_select_next_sp(b_sp) == a_sp && result_is(MAIA_ERR_TIMEOUT));
    CHECK(maia_sched_wait_notification_isr(0U) == MAIA_OK && maia_sched_select_next_sp(a_sp) == a_sp && result_is(MAIA_OK));
    CHECK(setup());maia_sched_test_set_tick(UINT64_MAX-2U);
    CHECK(maia_sched_wait_notification_isr(4U) == MAIA_OK && maia_sched_select_next_sp(a_sp) == b_sp);
    for (unsigned i=0U;i<3U;++i) { (void)maia_sched_tick_isr();CHECK(state(a) == MAIA_TASK_BLOCKED); }
    (void)maia_sched_tick_isr();CHECK(maia_sched_select_next_sp(b_sp) == a_sp && result_is(MAIA_ERR_TIMEOUT));
    CHECK(setup());
    CHECK(maia_sched_wait_notification_isr(MAIA_TASK_WAIT_FOREVER) == MAIA_OK && maia_sched_select_next_sp(a_sp) == b_sp);
    maia_sched_test_set_tick(UINT32_MAX-1U);
    (void)maia_sched_tick_isr();CHECK(state(a) == MAIA_TASK_BLOCKED);
    (void)maia_sched_tick_isr();CHECK(state(a) == MAIA_TASK_BLOCKED);
    maia_sched_test_set_tick(UINT64_MAX-2U);
    for (unsigned i=0U;i<8U;++i) { (void)maia_sched_tick_isr();CHECK(state(a) == MAIA_TASK_BLOCKED); }
    CHECK(maia_task_notify(a) == MAIA_OK && maia_sched_select_next_sp(b_sp) == a_sp && result_is(MAIA_OK));
    CHECK(maia_sched_wait_notification_isr(MAIA_TASK_WAIT_FOREVER-1U) == MAIA_OK && maia_sched_select_next_sp(a_sp) == b_sp);
    (void)maia_sched_tick_isr();CHECK(state(a) == MAIA_TASK_BLOCKED);
    CHECK(maia_task_notify(a) == MAIA_OK && maia_sched_select_next_sp(b_sp) == a_sp && result_is(MAIA_OK));
    CHECK(setup());
    CHECK(maia_sched_delay_current_isr(5U) == MAIA_OK && maia_sched_select_next_sp(a_sp) == b_sp);
    CHECK(maia_task_notify(a) == MAIA_OK && state(a) == MAIA_TASK_SLEEPING);
    for (unsigned i=0U;i<4U;++i) { (void)maia_sched_tick_isr();CHECK(state(a) == MAIA_TASK_SLEEPING); }
    (void)maia_sched_tick_isr();CHECK(maia_sched_select_next_sp(b_sp) == a_sp);
    CHECK(maia_sched_wait_notification_isr(0U) == MAIA_OK && maia_sched_select_next_sp(a_sp) == a_sp && result_is(MAIA_OK));
    CHECK(setup());
    CHECK(maia_sched_wait_notification_isr(MAIA_TASK_WAIT_FOREVER) == MAIA_OK && maia_sched_select_next_sp(a_sp) == b_sp);
    CHECK(maia_sched_wait_notification_isr(MAIA_TASK_WAIT_FOREVER) == MAIA_OK);
    uintptr_t idle=maia_sched_select_next_sp(b_sp);CHECK(idle && maia_task_current_id() == -1);
    CHECK(maia_sched_wait_notification_isr(1U) == MAIA_ERR_INVALID);
    for (unsigned i=0U;i<3U;++i) { (void)maia_sched_tick_isr();CHECK(maia_sched_select_next_sp(idle) == idle); }
    CHECK(maia_task_notify(a) == MAIA_OK && maia_sched_select_next_sp(idle) == a_sp && result_is(MAIA_OK));
    CHECK(maia_task_notify(a) == MAIA_OK && maia_sched_exit_current_isr() == MAIA_OK);
    CHECK(maia_sched_select_next_sp(a_sp) == idle && maia_task_notify(a) == MAIA_ERR_INVALID);
    maia_task_id_t reused;
    CHECK(maia_task_create("reuse",task,NULL,3U,stacks[0],sizeof stacks[0],&reused) == MAIA_OK && reused == a);
    CHECK(maia_sched_select_next_sp(idle) == a_sp);
    CHECK(maia_sched_wait_notification_isr(0U) == MAIA_OK && maia_sched_select_next_sp(a_sp) == a_sp && result_is(MAIA_ERR_EMPTY));
    CHECK(setup());
    CHECK(maia_sched_wait_notification_isr(5U) == MAIA_OK && maia_sched_select_next_sp(a_sp) == b_sp);
    CHECK(maia_task_notify(a) == MAIA_OK);
    ((uint32_t *)a_sp)[6]=0xabcdef12U;
    ((uint32_t *)stacks[0])[0]=0U;
    CHECK(maia_sched_select_next_sp(b_sp) == 0U && ((uint32_t *)a_sp)[6] == 0xabcdef12U);
    CHECK(!failure && irq_state == 8U && saves == restores);
    puts("PASS: sticky notifications, no pre-trap lost wake, first-completion wins, late event, wrap/forever, idle/reuse and checked-frame result");
    return 0;
}
