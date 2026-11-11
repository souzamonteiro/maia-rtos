#include <maia/kernel.h>
#include <maia/scheduler_internal.h>
#include <maia/arch/riscv_context.h>
#include <stdio.h>
static uint8_t stacks[2][1024];
static void task(void *arg) { (void)arg; }
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d\n", __LINE__); return 1; } } while (0)
int main(void) {
    CHECK(maia_sched_init() == MAIA_OK);
    uintptr_t idle_sp = maia_sched_select_next_sp(0U);
    CHECK(idle_sp != 0U && (idle_sp & 15U) == 0U);
    CHECK(maia_task_current_id() == -1 && maia_task_count() == 0U);
    CHECK((((const uint32_t *)idle_sp)[29] & MAIA_RISCV_FS_MASK) == 0U);
    maia_idle_stats_t idle;
    CHECK(maia_sched_get_idle_stats(NULL) == MAIA_ERR_INVALID);
    CHECK(maia_sched_get_idle_stats(&idle) == MAIA_OK && idle.entries == 1U);
    maia_task_id_t a,b;
    CHECK(maia_task_create("A",task,NULL,3U,stacks[0],sizeof stacks[0],&a) == MAIA_OK);
    CHECK(maia_task_create("B",task,NULL,2U,stacks[1],sizeof stacks[1],&b) == MAIA_OK);
    uintptr_t a_sp = maia_sched_select_next_sp(idle_sp);
    CHECK(maia_task_current_id() == a);
    CHECK(maia_sched_delay_current_isr(3U) == MAIA_OK);
    uintptr_t b_sp = maia_sched_select_next_sp(a_sp);
    CHECK(maia_task_current_id() == b);
    CHECK(maia_sched_delay_current_isr(5U) == MAIA_OK);
    CHECK(maia_sched_select_next_sp(b_sp) == idle_sp && maia_task_current_id() == -1);
    maia_task_info_t info;
    for (unsigned i=0U; i<2U; ++i) {
        (void)maia_sched_tick_isr(); CHECK(maia_sched_select_next_sp(idle_sp) == idle_sp);
        CHECK(maia_task_get_info((uint32_t)a,&info) == MAIA_OK && info.state == MAIA_TASK_SLEEPING);
    }
    (void)maia_sched_tick_isr(); CHECK(maia_sched_select_next_sp(idle_sp) == a_sp);
    CHECK(maia_task_current_id() == a);
    CHECK(maia_sched_exit_current_isr() == MAIA_OK);
    CHECK(maia_sched_select_next_sp(a_sp) == idle_sp);
    CHECK(maia_task_get_info((uint32_t)a,&info) == MAIA_OK && info.state == MAIA_TASK_EXITED);
    (void)maia_sched_tick_isr(); CHECK(maia_sched_select_next_sp(idle_sp) == idle_sp);
    (void)maia_sched_tick_isr(); CHECK(maia_sched_select_next_sp(idle_sp) == b_sp);
    CHECK(maia_task_current_id() == b);
    CHECK(maia_sched_exit_current_isr() == MAIA_OK);
    CHECK(maia_sched_select_next_sp(b_sp) == idle_sp && maia_task_current_id() == -1);
    for (unsigned i=0U; i<3U; ++i) {
        (void)maia_sched_tick_isr(); CHECK(maia_sched_select_next_sp(idle_sp) == idle_sp);
    }
    CHECK(maia_sched_get_idle_stats(&idle) == MAIA_OK && idle.ticks == 8U && idle.entries == 4U);
    CHECK(maia_task_count() == 2U);
    CHECK(maia_task_get_info((uint32_t)b,&info) == MAIA_OK && info.state == MAIA_TASK_EXITED);
    CHECK(maia_sched_delay_current_isr(1U) == MAIA_ERR_INVALID);
    CHECK(maia_sched_exit_current_isr() == MAIA_ERR_INVALID);
    puts("PASS: empty, all-sleeping and all-exited use private idle; deadlines wake correct task without revival");
    return 0;
}
