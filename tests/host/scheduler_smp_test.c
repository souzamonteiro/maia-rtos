#include <maia/kernel.h>
#include <maia/scheduler_internal.h>
#include <stdio.h>

static uint8_t stacks[6][1024] __attribute__((aligned(16)));
static unsigned kicks[MAIA_SCHED_MAX_CORES];
void maia_arch_sched_kick(uint32_t core) { if (core < MAIA_SCHED_MAX_CORES) ++kicks[core]; }
static void task(void *arg) { (void)arg; }
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d\n", __LINE__); return 1; } } while (0)

static int8_t core_of(maia_task_id_t id) {
    maia_task_info_t info;
    return maia_task_get_info((uint32_t)id,&info) == MAIA_OK ? info.core : -2;
}

int main(void) {
    maia_task_id_t a, b, c, d, e;
    maia_task_info_t info;
    maia_sched_test_set_core(0U);
    CHECK(maia_sched_init() == MAIA_OK);
    CHECK(maia_task_create("A",task,NULL,2U,stacks[0],sizeof stacks[0],&a) == MAIA_OK);
    CHECK(maia_task_create("B",task,NULL,2U,stacks[1],sizeof stacks[1],&b) == MAIA_OK);
    CHECK(maia_task_get_info((uint32_t)a,&info) == MAIA_OK && info.affinity == 1U && info.core == -1);
    CHECK(maia_task_set_affinity(b,0U) == MAIA_ERR_INVALID);
    CHECK(maia_task_set_affinity(b,4U) == MAIA_ERR_INVALID);
    CHECK(maia_task_set_affinity(9,2U) == MAIA_ERR_INVALID);
    CHECK(maia_task_set_affinity(b,2U) == MAIA_OK);
    CHECK(kicks[0] == 0U && kicks[1] == 0U); /* Not started: nobody to kick. */

    maia_sched_start();
    CHECK(maia_task_current_id() == a && core_of(a) == 0);
    CHECK(maia_sched_online_cores() == 1U);
    /* Core 0 never runs a core-1-only task: B stays READY, A keeps running. */
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == a);

    maia_sched_test_set_core(1U);
    maia_sched_start_secondary();
    CHECK(maia_sched_online_cores() == 3U);
    CHECK(maia_task_current_id() == b && core_of(b) == 1 && core_of(a) == 0);
    /* Runtime creation from core 1 inherits core 1's mask. */
    CHECK(maia_task_create("C",task,NULL,2U,stacks[2],sizeof stacks[2],&c) == MAIA_OK);
    CHECK(maia_task_get_info((uint32_t)c,&info) == MAIA_OK && info.affinity == 2U);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == c && core_of(b) == -1);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == b);

    /* Ticks run on core 0 and account both cores' current tasks. */
    maia_sched_test_set_core(0U);
    (void)maia_sched_tick_isr();
    CHECK(maia_task_get_info((uint32_t)a,&info) == MAIA_OK && info.run_ticks == 1U);
    CHECK(maia_task_get_info((uint32_t)b,&info) == MAIA_OK && info.run_ticks == 1U);

    /* A boot-pinned task created from core 0 stays on core 0; core 1 idles
     * instead of taking it or A (RUNNING on core 0). */
    CHECK(maia_task_create("D",task,NULL,3U,stacks[3],sizeof stacks[3],&d) == MAIA_OK);
    CHECK(maia_task_get_info((uint32_t)d,&info) == MAIA_OK && info.affinity == 1U);
    maia_sched_test_set_core(1U);
    CHECK(maia_sched_delay_current_isr(5U) == MAIA_OK);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == c);
    CHECK(maia_sched_exit_current_isr() == MAIA_OK);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == -1);

    /* Moving a waiting task to core 1 kicks core 1 when it becomes READY. */
    unsigned before = kicks[1];
    maia_sched_test_set_core(0U);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == d);
    CHECK(maia_task_create("E",task,NULL,3U,stacks[4],sizeof stacks[4],&e) == MAIA_OK);
    CHECK(maia_task_set_affinity(e,MAIA_CORE_MASK_ALL) == MAIA_OK && kicks[1] == before+1U);
    maia_sched_test_set_core(1U);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == e && core_of(e) == 1);
    /* Re-pinning a task running on core 1 from core 0 kicks core 1. */
    maia_sched_test_set_core(0U);
    before = kicks[1];
    CHECK(maia_task_set_affinity(e,1U) == MAIA_OK && kicks[1] > before);
    maia_sched_test_set_core(1U);
    CHECK(maia_sched_select_next_sp(0U) && maia_task_current_id() == -1 && core_of(e) == -1);
    puts("PASS: per-core selection, affinity, inheritance, dual-core tick accounting and kicks");
    return 0;
}
