/* RAM-only native waits: real blocked tasks, ISR/thread posts and deadline expiry. */
#include <maia/kernel.h>
#include <maia/console.h>
#include <maia/arch/irq.h>
#include <maia/arch/riscv_stack.h>
static uint8_t stacks[2][4096] __attribute__((aligned(16)));
static maia_task_id_t worker_id,controller_id;
static volatile uint32_t phase,rounds,errors,isr_worker_posts,isr_controller_posts,thread_posts;
static volatile uint32_t timeouts,wait_returns,forever_checks,irq_sent,thread_requested;
static uint32_t blocked_checks,last_phase,last_run_ticks,irq_checks,irq_perms;
extern void maia_soc_systimer_init(uint32_t hz);
void maia_display_putc(char c) { (void)c; }
static void check(int condition) {
    if (!condition) { uintptr_t irq=maia_arch_irq_save();++errors;maia_arch_irq_restore(irq); }
}
void maia_display_refresh_dma_if_needed(void) {
    uintptr_t here,status;
    __asm__ volatile("mv %0,sp" : "=r"(here));
    __asm__ volatile("csrr %0,mstatus" : "=r"(status));
    uintptr_t top=maia_trap_stack_metadata.irq_top;
    ++irq_checks;
    check(!(status&8U) && here>=top-MAIA_IRQ_STACK_BYTES+MAIA_RISCV_STACK_GUARD_BYTES && here<top &&
          maia_stack_guard_valid(top-MAIA_IRQ_STACK_BYTES));
    if (maia_task_wait_notification(1U) == MAIA_ERR_PERM) ++irq_perms;else check(0);
    maia_task_info_t info;
    if (maia_task_get_info((uint32_t)worker_id,&info) != MAIA_OK) { check(0);return; }
    if (info.state == MAIA_TASK_BLOCKED) {
        ++blocked_checks;
        if (last_phase == phase) check(last_run_ticks == info.run_ticks);
        last_phase=phase;last_run_ticks=info.run_ticks;
        if (phase == 1U && !irq_sent) {
            check(maia_task_notify(worker_id) == MAIA_OK);irq_sent=1U;++isr_worker_posts;
        } else if (phase == 3U && ++forever_checks == 3U) {
            check(maia_task_notify(worker_id) == MAIA_OK);++isr_worker_posts;
        } else if (phase == 4U && !thread_requested) {
            check(maia_task_notify(controller_id) == MAIA_OK);thread_requested=1U;++isr_controller_posts;
        }
    } else if (phase == 6U && info.state == MAIA_TASK_EXITED) {
        check(maia_task_notify(worker_id) == MAIA_ERR_INVALID);
        phase=7U;check(maia_task_notify(controller_id) == MAIA_OK);++isr_controller_posts;
    }
}
static maia_status_t wait_for(uint32_t ticks) {
    maia_status_t result=maia_task_wait_notification(ticks);++wait_returns;return result;
}
static void worker(void *arg) {
    (void)arg;
    check(wait_for(0U) == MAIA_OK); /* Two boot posts coalesce to one. */
    check(wait_for(0U) == MAIA_ERR_EMPTY);
    for (unsigned iteration=0U;iteration<40U;++iteration) {
        irq_sent=thread_requested=0U;
        phase=1U;check(wait_for(50U) == MAIA_OK);
        phase=2U;uint64_t before=maia_tick_get();
        check(wait_for(5U) == MAIA_ERR_TIMEOUT);check(maia_tick_get()-before>=5U);++timeouts;
        forever_checks=0U;phase=3U;
        check(wait_for(MAIA_TASK_WAIT_FOREVER) == MAIA_OK);check(forever_checks>=3U);
        phase=4U;check(wait_for(50U) == MAIA_OK);
        check(wait_for(0U) == MAIA_OK); /* Second thread post is retained. */
        check(wait_for(0U) == MAIA_ERR_EMPTY);
        phase=5U;before=maia_tick_get();
        check(wait_for(3U) == MAIA_ERR_TIMEOUT);check(maia_tick_get()-before>=3U);++timeouts;
        ++rounds;
    }
    phase=6U; /* Return through the normal EXITED trap. */
}
static void number(uint32_t value) {
    char digits[10];unsigned n=0U;
    do { digits[n++]=(char)('0'+value%10U);value/=10U; } while(value);
    while(n) maia_console_putc(digits[--n]);
}
static void finish(void) {
    (void)maia_arch_irq_save();
    maia_trap_stats_t traps;maia_idle_stats_t idle;maia_task_info_t info;
    maia_trap_get_stats(&traps);(void)maia_sched_get_idle_stats(&idle);
    (void)maia_task_get_info((uint32_t)worker_id,&info);
    int pass=!errors && rounds == 40U && timeouts == 80U && wait_returns == 282U &&
        isr_worker_posts == 80U && isr_controller_posts == 41U && thread_posts == 80U &&
        blocked_checks>=400U && idle.entries>0U && idle.ticks>=250U && info.state == MAIA_TASK_EXITED &&
        traps.timer_irqs>=750U && traps.timer_irqs == maia_tick_get() &&
        traps.ecall_count == 325U && traps.sync_exceptions == traps.ecall_count &&
        irq_checks == traps.timer_irqs && irq_perms == irq_checks;
    maia_console_puts("NOTIFY rounds=");number(rounds);
    maia_console_puts(" waits=");number(wait_returns);
    maia_console_puts(" timeouts=");number(timeouts);
    maia_console_puts(" isr_worker=");number(isr_worker_posts);
    maia_console_puts(" isr_controller=");number(isr_controller_posts);
    maia_console_puts(" thread_posts=");number(thread_posts);
    maia_console_puts(" errors=");number(errors);maia_console_puts("\n");
    maia_console_puts("WAIT_HW irq=");number(traps.timer_irqs);
    maia_console_puts(" ecall=");number(traps.ecall_count);
    maia_console_puts(" blocked_checks=");number(blocked_checks);
    maia_console_puts(" idle_ticks=");number((uint32_t)idle.ticks);
    maia_console_puts(" irq_perms=");number(irq_perms);maia_console_puts("\n");
    maia_console_puts(pass ? "MAIA_NATIVE_PREEMPT_PASS\n" : "MAIA_NATIVE_PREEMPT_FAIL\n");
    for (;;) __asm__ volatile("wfi");
}
static void controller(void *arg) {
    (void)arg;
    for (;;) {
        check(maia_task_wait_notification(MAIA_TASK_WAIT_FOREVER) == MAIA_OK);
        if (phase == 4U) {
            /* This test requires two posts before the worker resumes; the public
             * API itself does not promise atomicity across separate calls. */
            uintptr_t irq=maia_arch_irq_save();
            check(maia_task_notify(worker_id) == MAIA_OK);
            check(maia_task_notify(worker_id) == MAIA_OK);thread_posts+=2U;
            maia_arch_irq_restore(irq);
        } else if (phase == 7U) {
            uint32_t now=(uint32_t)maia_tick_get();
            if (now<750U) maia_task_delay_ticks(750U-now);
            finish();
        } else check(0);
    }
}
void maia_boot_main(void) {
    if (maia_sched_init() != MAIA_OK ||
        maia_task_create("waiter",worker,NULL,4U,stacks[0],sizeof stacks[0],&worker_id) != MAIA_OK ||
        maia_task_create("poster",controller,NULL,2U,stacks[1],sizeof stacks[1],&controller_id) != MAIA_OK ||
        maia_task_notify(worker_id) != MAIA_OK || maia_task_notify(worker_id) != MAIA_OK) {
        maia_console_puts("MAIA_NATIVE_PREEMPT_FAIL\n");for (;;) __asm__ volatile("wfi");
    }
    maia_soc_systimer_init(1000U);maia_sched_start();
}
