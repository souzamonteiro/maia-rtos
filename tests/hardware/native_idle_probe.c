/* RAM-only instrumented idle port: observe real WFI/timer wakeups without apps. */
#include <maia/kernel.h>
#include <maia/console.h>
#include <maia/arch/irq.h>
#include <maia/scheduler_internal.h>
#include <maia/soc/esp32p4_display.h>
#include <stdint.h>

static volatile uint32_t steps[2], returned;
static uint32_t both_sleeping_checks, all_exited_checks, errors, start_cycle;
#ifndef MAIA_NATIVE_PROBE_EMPTY
static uint8_t stacks[2][4096] __attribute__((aligned(16)));
static maia_task_id_t ids[2];
#if defined(__riscv_flen) && __riscv_flen == 32
extern float maia_probe_fp_add(float, float);
#endif
#endif
extern void maia_soc_systimer_init(uint32_t hz);
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }
static uint32_t cycles(void) {
    uint32_t result; __asm__ volatile("rdcycle %0" : "=r"(result)); return result;
}
static void number(uint32_t value) {
    char digits[10]; unsigned n=0U;
    do { digits[n++] = (char)('0' + value % 10U); value /= 10U; } while (value);
    while (n) maia_console_putc(digits[--n]);
}
static void fail(void) {
    __asm__ volatile("csrci mstatus, 8" ::: "memory");
    maia_console_puts("MAIA_NATIVE_PREEMPT_FAIL\n");
    for (;;) __asm__ volatile("wfi");
}
#ifndef MAIA_NATIVE_PROBE_EMPTY
static void worker(void *arg) {
    unsigned index = (unsigned)(uintptr_t)arg;
    for (unsigned i=0U; i<20U; ++i) {
        uint64_t before = maia_tick_get();
        uint32_t duration = index == 0U ? 3U : 5U;
        maia_task_delay_ticks(duration);
        if (maia_tick_get() - before < duration) {
            maia_console_puts("IDLE_EARLY_WAKE\n"); fail();
        }
#if defined(__riscv_flen) && __riscv_flen == 32
        union { float f; uint32_t u; } value;
        value.f = maia_probe_fp_add(2.0f,3.0f);
        if (value.u != 0x40A00000U) fail();
#endif
        ++steps[index];
    }
    ++returned;
}
#endif

/* Override the architecture wait hook only in this diagnostic. IRQs are enabled
 * on entry; short coherent observation precedes the same WFI used by the port.
 * No scheduler decision, synthetic tick or artificial task wake is injected.
 */
void maia_arch_idle_wait(void) {
    uint32_t status; __asm__ volatile("csrr %0, mstatus" : "=r"(status));
    uintptr_t irq = maia_arch_irq_save();
    if ((status & 8U) == 0U || (status & 0x6000U) != 0U || maia_task_current_id() != -1) ++errors;
    maia_idle_stats_t idle; maia_trap_stats_t traps;
    (void)maia_sched_get_idle_stats(&idle); maia_trap_get_stats(&traps);
    uint64_t tick = maia_tick_get();
#ifndef MAIA_NATIVE_PROBE_EMPTY
    maia_task_info_t a = {0}, b = {0};
    if (maia_task_get_info((uint32_t)ids[0],&a) != MAIA_OK ||
        maia_task_get_info((uint32_t)ids[1],&b) != MAIA_OK) ++errors;
    if (a.state == MAIA_TASK_SLEEPING && b.state == MAIA_TASK_SLEEPING) ++both_sleeping_checks;
    if (a.state == MAIA_TASK_EXITED && b.state == MAIA_TASK_EXITED) ++all_exited_checks;
#endif
    if (tick >= 150U || (uint32_t)(cycles()-start_cycle) >= 120000000U) {
        int pass = tick >= 150U && traps.timer_irqs >= 150U && idle.ticks >= 50U &&
                   idle.entries != 0U && traps.sync_exceptions == traps.ecall_count && errors == 0U;
#ifdef MAIA_NATIVE_PROBE_EMPTY
        pass = pass && maia_task_count() == 0U && traps.ecall_count == 0U && idle.entries == 1U;
#else
        pass = pass && steps[0] == 20U && steps[1] == 20U && returned == 2U &&
               both_sleeping_checks > 0U && all_exited_checks >= 10U && traps.ecall_count == 42U;
#endif
        maia_console_puts("IDLE ticks="); number((uint32_t)idle.ticks);
        maia_console_puts(" entries="); number(idle.entries);
        maia_console_puts(" appA="); number(steps[0]);
        maia_console_puts(" appB="); number(steps[1]);
        maia_console_puts(" returned="); number(returned);
        maia_console_puts(" sleeping_checks="); number(both_sleeping_checks);
        maia_console_puts(" exited_checks="); number(all_exited_checks);
        maia_console_puts(" irq="); number(traps.timer_irqs);
        maia_console_puts(" ecall="); number(traps.ecall_count);
        maia_console_puts(" errors="); number(errors);
        maia_console_puts("\n");
        maia_console_puts(pass ? "MAIA_NATIVE_PREEMPT_PASS\n" : "MAIA_NATIVE_PREEMPT_FAIL\n");
        for (;;) __asm__ volatile("wfi");
    }
    maia_arch_irq_restore(irq);
    __asm__ volatile("wfi" ::: "memory");
}

void maia_boot_main(void) {
    maia_console_puts("MAIA_IDLE_PROBE: private idle, real timer wakeups, no forced READY\n");
    if (maia_sched_init() != MAIA_OK) fail();
#ifndef MAIA_NATIVE_PROBE_EMPTY
    for (unsigned i=0U; i<2U; ++i) {
        if (maia_task_create(i == 0U ? "sleep3" : "sleep5",worker,(void *)(uintptr_t)i,
            i == 0U ? MAIA_PRIO_HIGH : MAIA_PRIO_NORMAL,stacks[i],sizeof stacks[i],&ids[i]) != MAIA_OK) fail();
    }
#endif
    maia_soc_systimer_init(1000U); start_cycle = cycles();
    maia_sched_start(); fail();
}
