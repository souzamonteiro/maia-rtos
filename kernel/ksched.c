#include <maia/kernel.h>

typedef struct maia_tcb {
    maia_task_id_t     id;
    char               name[MAIA_TASK_NAME_MAX];
    maia_task_entry_fn entry;
    void              *arg;
    uint8_t            priority;
    maia_task_state_t  state;
    uintptr_t          sp;
    uintptr_t          stack_base;
    size_t             stack_size;
    uint32_t           run_ticks;
    uint32_t           switches;
    uint64_t           wake_tick;
} maia_tcb_t;

static maia_tcb_t        g_tasks[MAIA_MAX_TASKS];
static int32_t           g_current_idx;
static uint32_t          g_task_count;
static uint32_t          g_switch_cnt;
static volatile uint64_t g_tick_count;
static maia_trap_stats_t g_trap_stats;

/* Weak platform hooks overridden by arch/soc on target hardware */
__attribute__((weak)) uint64_t maia_soc_time_us(void) {
    return g_tick_count * 1000ULL;
}

__attribute__((weak)) void maia_arch_yield_trap(void) {
#if defined(__riscv)
    __asm__ volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ecall\n\t"
        ".option pop"
        ::: "memory");
#endif
}

__attribute__((weak)) void maia_arch_start_first_task(uintptr_t first_sp) {
    (void)first_sp;
}

static void copy_name(char dst[MAIA_TASK_NAME_MAX], const char *src) {
    size_t i = 0U;
    if (src != NULL) {
        for (; i < (MAIA_TASK_NAME_MAX - 1U) && src[i] != '\0'; ++i) {
            dst[i] = src[i];
        }
    }
    for (; i < MAIA_TASK_NAME_MAX; ++i) {
        dst[i] = '\0';
    }
}

void maia_task_trampoline(uint32_t task_idx);
void maia_task_trampoline(uint32_t task_idx) {
    if (task_idx < MAIA_MAX_TASKS && g_tasks[task_idx].entry != NULL) {
        g_tasks[task_idx].entry(g_tasks[task_idx].arg);
    }
    if (task_idx < MAIA_MAX_TASKS) {
        g_tasks[task_idx].state = MAIA_TASK_EXITED;
    }
    for (;;) {
        maia_task_yield();
    }
}

static uintptr_t init_task_stack(void *stack_mem, size_t stack_size, uint32_t task_idx) {
    uintptr_t top = ((uintptr_t)stack_mem + stack_size) & ~(uintptr_t)0xFU;
    uint32_t *frame = (uint32_t *)(top - 128U);
    for (uint32_t i = 0U; i < 32U; ++i) {
        frame[i] = 0U;
    }
    frame[6]  = task_idx;                                    /* a0 (x10) = task_idx */
    frame[28] = (uint32_t)(uintptr_t)&maia_task_trampoline;  /* mepc */
    frame[29] = 0x00001880UL;                                /* mstatus: MPP=3 (M-mode), MPIE=1 */
    frame[30] = 0x38000000UL;                                /* mcause: CLIC MPP=3, MPIE=1, MPIL=0 */
    return (uintptr_t)frame;
}

uint64_t maia_tick_get(void) {
    return g_tick_count;
}

uint64_t maia_time_us(void) {
    return maia_soc_time_us();
}

void maia_trap_get_stats(maia_trap_stats_t *out_stats) {
    if (out_stats != NULL) {
        *out_stats = g_trap_stats;
    }
}

void maia_trap_record_event(uint32_t mcause, uint32_t mepc, uint32_t mtval) {
    g_trap_stats.total_traps += 1U;
    g_trap_stats.last_mcause  = mcause;
    g_trap_stats.last_mepc    = mepc;
    g_trap_stats.last_mtval   = mtval;
    if ((mcause & 0x80000000UL) != 0UL) {
        uint32_t code = mcause & 0xFFFUL;
        if (code == 16UL || code == 7UL) {
            g_trap_stats.timer_irqs += 1U;
        } else {
            g_trap_stats.external_irqs += 1U;
        }
    } else {
        g_trap_stats.sync_exceptions += 1U;
        if ((mcause & 0xFFUL) == 11UL) {
            g_trap_stats.ecall_count += 1U;
        }
    }
}

maia_status_t maia_sched_init(void) {
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        g_tasks[i].id         = (maia_task_id_t)i;
        g_tasks[i].name[0]    = '\0';
        g_tasks[i].entry      = NULL;
        g_tasks[i].arg        = NULL;
        g_tasks[i].priority   = 0U;
        g_tasks[i].state      = MAIA_TASK_UNUSED;
        g_tasks[i].sp         = 0U;
        g_tasks[i].stack_base = 0U;
        g_tasks[i].stack_size = 0U;
        g_tasks[i].run_ticks  = 0U;
        g_tasks[i].switches   = 0U;
        g_tasks[i].wake_tick  = 0ULL;
    }
    g_current_idx = -1;
    g_task_count  = 0U;
    g_switch_cnt  = 0U;
    g_tick_count  = 0ULL;
    g_trap_stats.total_traps     = 0U;
    g_trap_stats.timer_irqs      = 0U;
    g_trap_stats.external_irqs   = 0U;
    g_trap_stats.sync_exceptions = 0U;
    g_trap_stats.ecall_count     = 0U;
    g_trap_stats.last_mcause     = 0U;
    g_trap_stats.last_mepc       = 0U;
    g_trap_stats.last_mtval      = 0U;
    return MAIA_OK;
}

maia_status_t maia_task_create(const char *name,
                               maia_task_entry_fn entry,
                               void *arg,
                               uint8_t priority,
                               void *stack_mem,
                               size_t stack_size,
                               maia_task_id_t *out_id) {
    if (entry == NULL || stack_mem == NULL || stack_size < 256U) {
        return MAIA_ERR_INVALID;
    }
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_tasks[i].state == MAIA_TASK_UNUSED || g_tasks[i].state == MAIA_TASK_EXITED) {
            copy_name(g_tasks[i].name, name);
            g_tasks[i].entry      = entry;
            g_tasks[i].arg        = arg;
            g_tasks[i].priority   = priority;
            g_tasks[i].state      = MAIA_TASK_READY;
            g_tasks[i].stack_base = (uintptr_t)stack_mem;
            g_tasks[i].stack_size = stack_size;
            g_tasks[i].sp         = init_task_stack(stack_mem, stack_size, i);
            g_tasks[i].run_ticks  = 0U;
            g_tasks[i].switches   = 0U;
            g_tasks[i].wake_tick  = 0ULL;
            g_task_count         += 1U;
            if (out_id != NULL) {
                *out_id = (maia_task_id_t)i;
            }
            return MAIA_OK;
        }
    }
    return MAIA_ERR_NOMEM;
}

int maia_sched_tick_isr(void) {
    int woke_higher = 0;
    g_tick_count += 1ULL;

    if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS) {
        g_tasks[g_current_idx].run_ticks += 1U;
    }

    uint8_t cur_prio = 0U;
    if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS &&
        g_tasks[g_current_idx].state == MAIA_TASK_RUNNING) {
        cur_prio = g_tasks[g_current_idx].priority;
    }

    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_tasks[i].state == MAIA_TASK_SLEEPING && g_tick_count >= g_tasks[i].wake_tick) {
            g_tasks[i].state     = MAIA_TASK_READY;
            g_tasks[i].wake_tick = 0ULL;
            if (g_tasks[i].priority >= cur_prio) {
                woke_higher = 1;
            }
        } else if (g_tasks[i].state == MAIA_TASK_READY && g_tasks[i].priority >= cur_prio) {
            woke_higher = 1;
        }
    }
    return woke_higher;
}

uintptr_t maia_sched_select_next_sp(uintptr_t current_sp) {
    if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS) {
        if (current_sp != 0U) {
            g_tasks[g_current_idx].sp = current_sp;
        }
        if (g_tasks[g_current_idx].state == MAIA_TASK_RUNNING) {
            g_tasks[g_current_idx].state = MAIA_TASK_READY;
        }
    }

    /* Find highest priority among READY tasks, rotating round-robin after g_current_idx */
    int32_t best_idx  = -1;
    uint8_t best_prio = 0U;
    uint32_t start = (g_current_idx >= 0) ? (((uint32_t)g_current_idx + 1U) % MAIA_MAX_TASKS) : 0U;

    for (uint32_t step = 0U; step < MAIA_MAX_TASKS; ++step) {
        uint32_t idx = (start + step) % MAIA_MAX_TASKS;
        if (g_tasks[idx].state == MAIA_TASK_READY) {
            if (best_idx < 0 || g_tasks[idx].priority > best_prio) {
                best_idx  = (int32_t)idx;
                best_prio = g_tasks[idx].priority;
            }
        }
    }

    if (best_idx < 0) {
        /* If no task is READY, keep current if valid */
        if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS) {
            g_tasks[g_current_idx].state = MAIA_TASK_RUNNING;
            return g_tasks[g_current_idx].sp;
        }
        return current_sp;
    }

    if (best_idx != g_current_idx) {
        g_switch_cnt += 1U;
    }
    g_current_idx = best_idx;
    g_tasks[g_current_idx].state     = MAIA_TASK_RUNNING;
    g_tasks[g_current_idx].switches += 1U;
    return g_tasks[g_current_idx].sp;
}

void maia_task_yield(void) {
    maia_arch_yield_trap();
}

void maia_task_delay_ticks(uint32_t ticks) {
    if (ticks == 0U) {
        maia_task_yield();
        return;
    }
    if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS) {
        g_tasks[g_current_idx].wake_tick = g_tick_count + (uint64_t)ticks;
        g_tasks[g_current_idx].state     = MAIA_TASK_SLEEPING;
    }
    maia_arch_yield_trap();
}

maia_task_id_t maia_task_current_id(void) {
    return g_current_idx;
}

uint32_t maia_task_count(void) {
    return g_task_count;
}

uint32_t maia_sched_switch_count(void) {
    return g_switch_cnt;
}

maia_status_t maia_task_get_info(uint32_t index, maia_task_info_t *out_info) {
    if (index >= MAIA_MAX_TASKS || out_info == NULL) {
        return MAIA_ERR_INVALID;
    }
    if (g_tasks[index].state == MAIA_TASK_UNUSED) {
        return MAIA_ERR_EMPTY;
    }
    copy_name(out_info->name, g_tasks[index].name);
    out_info->id         = g_tasks[index].id;
    out_info->priority   = g_tasks[index].priority;
    out_info->state      = g_tasks[index].state;
    out_info->stack_size = g_tasks[index].stack_size;
    out_info->run_ticks  = g_tasks[index].run_ticks;
    out_info->switches   = g_tasks[index].switches;
    out_info->wake_tick  = g_tasks[index].wake_tick;
    return MAIA_OK;
}

void maia_sched_start(void) {
    uintptr_t first_sp = maia_sched_select_next_sp(0U);
    if (first_sp != 0U) {
        maia_arch_start_first_task(first_sp);
    }
}
