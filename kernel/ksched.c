#include <maia/kernel.h>
#include <maia/arch/irq.h>
#include <maia/scheduler_internal.h>
#include <maia/arch/riscv_context.h>
#include <maia/arch/riscv_stack.h>

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
static int g_started;
static uint8_t g_idle_stack[2048] __attribute__((aligned(16)));
static uintptr_t g_idle_sp;
static int g_idle_running;
static maia_idle_stats_t g_idle_stats;
void maia_arch_yield_trap(void);

#if !defined(__riscv)
__attribute__((weak)) void maia_arch_stack_publish(uintptr_t low, uintptr_t high) { (void)low; (void)high; }
__attribute__((weak)) int maia_arch_stack_reserved(uintptr_t base, size_t size) { (void)base; (void)size; return 0; }
static uintptr_t g_host_irq = 8U;
__attribute__((weak)) uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous = g_host_irq;
    g_host_irq = 0U;
    return previous;
}
__attribute__((weak)) void maia_arch_irq_restore(uintptr_t previous) { g_host_irq = previous & 8U; }
__attribute__((weak)) void maia_arch_delay_trap(uint32_t ticks) {
    (void)maia_sched_delay_current_isr(ticks);
}
__attribute__((weak, noreturn)) void maia_arch_exit_trap(void) {
    (void)maia_sched_exit_current_isr();
    __builtin_trap(); /* Hosted model cannot execute a native context return. */
}
#endif

/* Weak platform hooks overridden by arch/soc on target hardware */
__attribute__((weak)) uint64_t maia_soc_time_us(void) {
    return maia_tick_get() * 1000ULL;
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

__attribute__((weak)) void maia_arch_idle_wait(void) {
#if defined(__riscv)
    __asm__ volatile("wfi" ::: "memory");
#else
    __builtin_trap(); /* Host model does not execute native task contexts. */
#endif
}

static void idle_entry(void) {
    for (;;) maia_arch_idle_wait();
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
    maia_arch_exit_trap();
}

static uintptr_t init_task_stack(void *stack_mem, size_t stack_size, uint32_t task_idx) {
    maia_stack_guard_init((uintptr_t)stack_mem);
    uintptr_t top = ((uintptr_t)stack_mem + stack_size) & ~(uintptr_t)0xFU;
    uint32_t *frame = (uint32_t *)(top - MAIA_RISCV_CONTEXT_BYTES);
    for (uint32_t i = 0U; i < MAIA_RISCV_CONTEXT_WORDS; ++i) {
        frame[i] = 0U;
    }
    /* frame[31] (tp) remains zero until a task/runtime establishes its TLS. */
    frame[6]  = task_idx;                                    /* a0 (x10) = task_idx */
    frame[28] = (uint32_t)(uintptr_t)&maia_task_trampoline;  /* mepc */
    /* MPP=M, MPIE=1; FS=Initial for RV32F. */
    frame[29] = MAIA_RISCV_INITIAL_MSTATUS;
    /* A fresh task can first be selected inside a timer ISR. On ESP32-P4,
     * exception-return state (INT=0) leaves the active interrupt level in place.
     * Interrupt-return state restores MPIL=0 when mret starts this task.
     * See docs/hardware/EVIDENCE.md and the RAM-only preemption regression. */
    frame[30] = MAIA_RISCV_TASK_RETURN_FLAGS;                                /* INT=1, MPP=3, MPIE=1, MPIL=0 */
    return (uintptr_t)frame;
}

uint64_t maia_tick_get(void) {
    uintptr_t irq = maia_arch_irq_save();
    uint64_t result = g_tick_count;
    maia_arch_irq_restore(irq);
    return result;
}

uint64_t maia_time_us(void) {
    return maia_soc_time_us();
}

void maia_trap_get_stats(maia_trap_stats_t *out_stats) {
    if (out_stats != NULL) {
        uintptr_t irq = maia_arch_irq_save();
        *out_stats = g_trap_stats;
        maia_arch_irq_restore(irq);
    }
}

void maia_trap_record_event(uint32_t mcause, uint32_t mepc, uint32_t mtval) {
    uintptr_t irq = maia_arch_irq_save();
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
        if ((mcause & 0xFFFUL) == 11UL) {
            g_trap_stats.ecall_count += 1U;
        }
    }
    maia_arch_irq_restore(irq);
}

static maia_status_t sched_init_locked(void) {
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
    g_idle_running = 0;
    g_idle_stats.ticks = 0U;
    g_idle_stats.entries = 0U;
    g_idle_sp = init_task_stack(g_idle_stack, sizeof g_idle_stack, 0U);
    uint32_t *idle_frame = (uint32_t *)g_idle_sp;
    idle_frame[28] = (uint32_t)(uintptr_t)idle_entry;
    idle_frame[29] &= ~MAIA_RISCV_FS_MASK; /* Internal idle never uses FP. */
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

maia_status_t maia_sched_init(void) {
    uintptr_t irq = maia_arch_irq_save();
    maia_status_t status = g_started ? MAIA_ERR_BUSY : sched_init_locked();
    maia_arch_irq_restore(irq);
    return status;
}

static maia_status_t task_create_locked(const char *name,
                               maia_task_entry_fn entry,
                               void *arg,
                               uint8_t priority,
                               void *stack_mem,
                               size_t stack_size,
                               maia_task_id_t *out_id) {
    if (entry == NULL || stack_mem == NULL || stack_size < MAIA_RISCV_MIN_STACK_BYTES ||
        stack_size > UINTPTR_MAX - (uintptr_t)stack_mem) {
        return MAIA_ERR_INVALID;
    }
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_tasks[i].state == MAIA_TASK_UNUSED || g_tasks[i].state == MAIA_TASK_EXITED) {
            if (maia_arch_stack_reserved((uintptr_t)stack_mem, stack_size) ||
                maia_stack_overlap((uintptr_t)stack_mem,stack_size,(uintptr_t)g_idle_stack,sizeof g_idle_stack)) {
                return MAIA_ERR_INVALID;
            }
            for (uint32_t j=0U; j<MAIA_MAX_TASKS; ++j) {
                if (g_tasks[j].state != MAIA_TASK_UNUSED && g_tasks[j].state != MAIA_TASK_EXITED &&
                    maia_stack_overlap((uintptr_t)stack_mem,stack_size,g_tasks[j].stack_base,g_tasks[j].stack_size)) {
                    return MAIA_ERR_INVALID;
                }
            }
            int fresh_slot = g_tasks[i].state == MAIA_TASK_UNUSED;
            copy_name(g_tasks[i].name, name);
            g_tasks[i].entry      = entry;
            g_tasks[i].arg        = arg;
            g_tasks[i].priority   = priority;
            g_tasks[i].stack_base = (uintptr_t)stack_mem;
            g_tasks[i].stack_size = stack_size;
            g_tasks[i].sp         = init_task_stack(stack_mem, stack_size, i);
            g_tasks[i].run_ticks  = 0U;
            g_tasks[i].switches   = 0U;
            g_tasks[i].wake_tick  = 0ULL;
            if (fresh_slot) g_task_count += 1U;
            /* Publish readiness only after every context/TCB field is valid. */
            g_tasks[i].state = MAIA_TASK_READY;
            if (out_id != NULL) {
                *out_id = (maia_task_id_t)i;
            }
            return MAIA_OK;
        }
    }
    return MAIA_ERR_NOMEM;
}

maia_status_t maia_task_create(const char *name, maia_task_entry_fn entry,
                               void *arg, uint8_t priority, void *stack_mem,
                               size_t stack_size, maia_task_id_t *out_id) {
    uintptr_t irq = maia_arch_irq_save();
    maia_status_t status = g_started && irq == 0U ? MAIA_ERR_PERM :
        task_create_locked(name, entry, arg, priority, stack_mem, stack_size, out_id);
    maia_arch_irq_restore(irq);
    return status;
}

static int tick_locked(void) {
    int woke_higher = 0;
    g_tick_count += 1ULL;

    if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS) {
        g_tasks[g_current_idx].run_ticks += 1U;
    } else if (g_idle_running) {
        g_idle_stats.ticks += 1U;
    }

    uint8_t cur_prio = 0U;
    if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS &&
        g_tasks[g_current_idx].state == MAIA_TASK_RUNNING) {
        cur_prio = g_tasks[g_current_idx].priority;
    }

    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_tasks[i].state == MAIA_TASK_SLEEPING && (uint64_t)(g_tick_count - g_tasks[i].wake_tick) < (UINT64_C(1) << 63U)) {
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

int maia_sched_tick_isr(void) {
    uintptr_t irq = maia_arch_irq_save();
    int result = tick_locked();
    maia_arch_irq_restore(irq);
    return result;
}

static uintptr_t select_next_locked(uintptr_t current_sp) {
    if (g_idle_running) {
        uintptr_t candidate = current_sp != 0U ? current_sp : g_idle_sp;
        if (!maia_stack_frame_valid((uintptr_t)g_idle_stack,sizeof g_idle_stack,candidate)) return 0U;
        g_idle_sp = candidate;
    } else if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS) {
        uintptr_t candidate = current_sp != 0U ? current_sp : g_tasks[g_current_idx].sp;
        if (!maia_stack_frame_valid(g_tasks[g_current_idx].stack_base,g_tasks[g_current_idx].stack_size,candidate)) return 0U;
        g_tasks[g_current_idx].sp = candidate;
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
        if (!maia_stack_frame_valid((uintptr_t)g_idle_stack,sizeof g_idle_stack,g_idle_sp)) return 0U;
        maia_arch_stack_publish(maia_stack_low((uintptr_t)g_idle_stack),maia_stack_top((uintptr_t)g_idle_stack,sizeof g_idle_stack));
        /* No app is runnable. Never revive a sleeping/blocked/exited task. */
        if (!g_idle_running) {
            g_switch_cnt += 1U;
            g_idle_stats.entries += 1U;
        }
        g_current_idx = -1;
        g_idle_running = 1;
        return g_idle_sp;
    }

    if (!maia_stack_frame_valid(g_tasks[best_idx].stack_base,g_tasks[best_idx].stack_size,g_tasks[best_idx].sp)) return 0U;
    maia_arch_stack_publish(maia_stack_low(g_tasks[best_idx].stack_base),maia_stack_top(g_tasks[best_idx].stack_base,g_tasks[best_idx].stack_size));
    if (best_idx != g_current_idx) {
        g_switch_cnt += 1U;
    }
    g_idle_running = 0;
    g_current_idx = best_idx;
    g_tasks[g_current_idx].state     = MAIA_TASK_RUNNING;
    g_tasks[g_current_idx].switches += 1U;
    return g_tasks[g_current_idx].sp;
}

uintptr_t maia_sched_select_next_sp(uintptr_t current_sp) {
    uintptr_t irq = maia_arch_irq_save();
    uintptr_t result = select_next_locked(current_sp);
    maia_arch_irq_restore(irq);
    return result;
}

void maia_task_yield(void) {
    maia_arch_yield_trap();
}

void maia_task_delay_ticks(uint32_t ticks) {
    if (ticks == 0U) maia_task_yield();
    else maia_arch_delay_trap(ticks);
}

maia_status_t maia_sched_delay_current_isr(uint32_t ticks) {
    uintptr_t irq = maia_arch_irq_save();
    maia_status_t status = MAIA_ERR_INVALID;
    if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS &&
        g_tasks[g_current_idx].state == MAIA_TASK_RUNNING) {
        if (ticks != 0U) {
            /* Unsigned modular deadline; uint32 delays are less than half-range. */
            g_tasks[g_current_idx].wake_tick = g_tick_count + (uint64_t)ticks;
            g_tasks[g_current_idx].state = MAIA_TASK_SLEEPING;
        }
        status = MAIA_OK;
    }
    maia_arch_irq_restore(irq);
    return status;
}

maia_status_t maia_sched_exit_current_isr(void) {
    uintptr_t irq = maia_arch_irq_save();
    maia_status_t status = MAIA_ERR_INVALID;
    if (g_current_idx >= 0 && (uint32_t)g_current_idx < MAIA_MAX_TASKS &&
        g_tasks[g_current_idx].state == MAIA_TASK_RUNNING) {
        g_tasks[g_current_idx].state = MAIA_TASK_EXITED;
        status = MAIA_OK;
    }
    maia_arch_irq_restore(irq);
    return status;
}

maia_task_id_t maia_task_current_id(void) {
    uintptr_t irq = maia_arch_irq_save();
    maia_task_id_t result = g_current_idx;
    maia_arch_irq_restore(irq);
    return result;
}

uint32_t maia_task_count(void) {
    uintptr_t irq = maia_arch_irq_save();
    uint32_t result = g_task_count;
    maia_arch_irq_restore(irq);
    return result;
}

uint32_t maia_sched_switch_count(void) {
    uintptr_t irq = maia_arch_irq_save();
    uint32_t result = g_switch_cnt;
    maia_arch_irq_restore(irq);
    return result;
}

static maia_status_t task_get_info_locked(uint32_t index, maia_task_info_t *out_info) {
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

maia_status_t maia_task_get_info(uint32_t index, maia_task_info_t *out_info) {
    uintptr_t irq = maia_arch_irq_save();
    maia_status_t status = task_get_info_locked(index, out_info);
    maia_arch_irq_restore(irq);
    return status;
}

maia_status_t maia_sched_get_idle_stats(maia_idle_stats_t *out_stats) {
    if (out_stats == NULL) return MAIA_ERR_INVALID;
    uintptr_t irq = maia_arch_irq_save();
    *out_stats = g_idle_stats;
    maia_arch_irq_restore(irq);
    return MAIA_OK;
}

#ifdef MAIA_SCHED_TEST
void maia_sched_test_set_tick(uint64_t tick) {
    uintptr_t irq = maia_arch_irq_save();
    g_tick_count = tick;
    maia_arch_irq_restore(irq);
}
#endif

void maia_sched_start(void) {
    uintptr_t irq = maia_arch_irq_save();
    if (g_started) { maia_arch_irq_restore(irq); return; }
    uintptr_t first_sp = select_next_locked(0U);
    if (first_sp != 0U) g_started = 1;
    if (first_sp != 0U) {
        maia_arch_start_first_task(first_sp);
    }
    maia_arch_irq_restore(irq); /* Host-only return / empty boot; native mret does not return. */
}
