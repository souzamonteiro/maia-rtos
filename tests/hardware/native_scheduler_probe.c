/* Single-core integer/optional RV32F context diagnostic. No yield or synthetic tick calls.
 * This is RAM-only test firmware, not a replacement for the working desktop.
 */
#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/soc/esp32p4_regs.h>
#include <maia/soc/esp32p4_display.h>

#define PROBE_TICKS 250U
#define PROBE_CYCLE_BUDGET 120000000U

#ifdef MAIA_NATIVE_PROBE_MIXED
#define PROBE_TASKS 3
static volatile uint32_t s_off_checks, s_delays, s_priority_checks;
#else
#define PROBE_TASKS 2
#endif
static uint8_t s_stacks[PROBE_TASKS][4096] __attribute__((aligned(16)));
static volatile uint32_t s_work[2];
static uint32_t s_start_cycle;
static volatile uint32_t s_register_errors;
#if defined(__riscv_flen) && __riscv_flen == 32
extern float maia_probe_fp_add(float left, float right);
static volatile uint32_t s_fp_abi_checks[2];
#endif

/* Explicitly console-only: no display initialization or DMA service here. */
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }

static uint32_t cycle_now(void) {
    uint32_t value;
    __asm__ volatile("rdcycle %0" : "=r"(value));
    return value;
}

static void print_number(uint32_t value) {
    char digits[10];
    unsigned count = 0U;
    do {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U);
    while (count != 0U) maia_console_putc(digits[--count]);
}

static void finish(void) {
    uint32_t mstatus, mcause, mintstatus;
    __asm__ volatile("csrr %0, mstatus" : "=r"(mstatus));
    /* Freeze the single-core result before inspecting CSRs or counters. */
    __asm__ volatile("csrci mstatus, 8" ::: "memory");
    __asm__ volatile("csrr %0, mcause" : "=r"(mcause));
    /* ESP32-P4 CLIC CSR per hardware/EVIDENCE.md; not standard CLIC 0xfb1. */
    __asm__ volatile("csrr %0, 0x346" : "=r"(mintstatus));
    maia_trap_stats_t stats;
    maia_trap_get_stats(&stats);
    uint32_t switches = maia_sched_switch_count();
#ifdef MAIA_NATIVE_PROBE_MIXED
    int traps_ok = stats.ecall_count >= 100U && stats.sync_exceptions == stats.ecall_count;
#else
    int traps_ok = stats.sync_exceptions == 0U && stats.ecall_count == 0U;
#endif
    int pass = stats.timer_irqs >= PROBE_TICKS && maia_tick_get() >= PROBE_TICKS &&
               traps_ok &&
               switches >= PROBE_TICKS && s_work[0] != 0U && s_work[1] != 0U &&
               (mstatus & 8U) != 0U && (mintstatus >> 24U) == 0U &&
               s_register_errors == 0U;
#ifdef MAIA_NATIVE_PROBE_MIXED
    pass = pass && s_off_checks >= 20U && s_delays >= 10U && s_priority_checks >= 10U;
    maia_console_puts("MIXED off_checks="); print_number(s_off_checks);
    maia_console_puts(" delays="); print_number(s_delays);
    maia_console_puts(" priority_checks="); print_number(s_priority_checks);
    maia_console_puts("\n");
#endif
    maia_console_puts("PROBE irq="); print_number(stats.timer_irqs);
    maia_console_puts(" ticks="); print_number((uint32_t)maia_tick_get());
    maia_console_puts(" ecall="); print_number(stats.ecall_count);
    maia_console_puts(" sync="); print_number(stats.sync_exceptions);
    maia_console_puts(" switches="); print_number(switches);
    maia_console_puts(" workA="); print_number(s_work[0]);
    maia_console_puts(" workB="); print_number(s_work[1]);
    maia_console_puts(" reg_errors="); print_number(s_register_errors);
    maia_console_puts("\n");
#if defined(__riscv_flen) && __riscv_flen == 32
    maia_console_puts("FP abi_checks="); print_number(s_fp_abi_checks[0] + s_fp_abi_checks[1]);
    maia_console_puts(" taskA="); print_number(s_fp_abi_checks[0]);
    maia_console_puts(" taskB="); print_number(s_fp_abi_checks[1]);
    maia_console_puts("\n");
#endif
    maia_console_puts("HW mstatus="); print_number(mstatus);
    maia_console_puts(" mcause="); print_number(mcause);
    maia_console_puts(" mintstatus="); print_number(mintstatus);
    maia_console_puts(" last_mcause="); print_number(stats.last_mcause);
    maia_console_puts(" conf="); print_number(MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_CONF_REG));
    maia_console_puts(" raw="); print_number(MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_INT_RAW_REG));
    maia_console_puts(" ena="); print_number(MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_INT_ENA_REG));
    maia_console_puts(" st="); print_number(MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_INT_ST_REG));
    maia_console_puts(" map="); print_number(MAIA_MMIO32(MAIA_ESP32P4_INTR_CORE0_SYSTIMER0_MAP));
    maia_console_puts(" clic="); print_number(MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_CTRL_REG(MAIA_ESP32P4_CLIC_SYSTIMER_INTR_ID)));
    maia_console_puts(" config="); print_number(MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_CONFIG_REG));
    maia_console_puts(" threshold="); print_number(MAIA_MMIO32(MAIA_ESP32P4_CLIC_INT_THRESH_REG));
    maia_console_puts("\n");
    maia_console_puts(pass ? "MAIA_NATIVE_PREEMPT_PASS\n" : "MAIA_NATIVE_PREEMPT_FAIL\n");
    maia_console_flush();
    for (;;) __asm__ volatile("wfi");
}

void maia_probe_register_error(uint32_t reg, uint32_t actual,
                               uint32_t expected, uint32_t task);
void maia_probe_register_error(uint32_t reg, uint32_t actual,
                               uint32_t expected, uint32_t task) {
    __asm__ volatile("csrci mstatus, 8" ::: "memory");
    ++s_register_errors;
    maia_console_puts("REG_FAIL task="); print_number(task);
    maia_console_puts(" reg="); print_number(reg);
    maia_console_puts(" expected="); print_number(expected);
    maia_console_puts(" actual="); print_number(actual);
    maia_console_puts("\n");
    finish();
}

void maia_probe_register_checkpoint(uint32_t index);
void maia_probe_register_checkpoint(uint32_t index) {
#if defined(__riscv_flen) && __riscv_flen == 32
    union { float value; uint32_t bits; } sum;
    sum.value = maia_probe_fp_add(2.0f, 3.0f);
    if (sum.bits != 0x40A00000U) {
        maia_probe_register_error(513U, sum.bits, 0x40A00000U, index);
    }
    ++s_fp_abi_checks[index];
#endif
    ++s_work[index];
    if (maia_tick_get() >= PROBE_TICKS ||
        (uint32_t)(cycle_now() - s_start_cycle) >= PROBE_CYCLE_BUDGET) {
        finish();
    }
}

extern void maia_probe_register_worker_a(void *arg);
extern void maia_probe_register_worker_b(void *arg);

#ifdef MAIA_NATIVE_PROBE_MIXED
static void check_fp_off(void) {
    uint32_t status;
    __asm__ volatile("csrr %0, mstatus" : "=r"(status));
    if ((status & 0x6000U) != 0U) maia_probe_register_error(900U, status & 0x6000U, 0U, 2U);
    ++s_off_checks;
}

static void off_priority_worker(void *arg) {
    (void)arg;
    __asm__ volatile("csrc mstatus, %0" :: "r"(0x6000U) : "memory");
    for (;;) {
        check_fp_off();
        uint32_t before_a = s_work[0], before_b = s_work[1];
        maia_task_yield();
        check_fp_off();
        if (before_a != s_work[0] || before_b != s_work[1]) {
            maia_probe_register_error(901U, s_work[0] + s_work[1], before_a + before_b, 2U);
        }
        ++s_priority_checks;
        uint64_t start_tick = maia_tick_get();
        maia_task_delay_ticks(2U);
        uint64_t elapsed = maia_tick_get() - start_tick;
        if (elapsed < 2U) maia_probe_register_error(902U, (uint32_t)elapsed, 2U, 2U);
        check_fp_off();
        if (s_work[0] == before_a || s_work[1] == before_b) {
            maia_probe_register_error(903U, 0U, 1U, 2U);
        }
        ++s_delays;
    }
}
#endif

extern void maia_soc_systimer_init(uint32_t tick_hz);

void maia_boot_main(void) {
#ifdef MAIA_NATIVE_PROBE_MIXED
    maia_console_puts("MAIA_MIXED_PROBE: FS on/off, high priority yield/delay, live register ecalls\n");
#else
    maia_console_puts("MAIA_NATIVE_PROBE: live integer registers, two busy tasks, no yields or synthetic ticks\n");
#endif
    if (maia_sched_init() != MAIA_OK) {
        maia_console_puts("MAIA_NATIVE_PROBE_SETUP_FAIL\n");
        for (;;) __asm__ volatile("wfi");
    }
    for (unsigned i = 0U; i < 2U; ++i) {
        if (maia_task_create(i == 0U ? "probeA" : "probeB",
                             i == 0U ? maia_probe_register_worker_a : maia_probe_register_worker_b,
                             (void *)(uintptr_t)i, MAIA_PRIO_NORMAL,
                             s_stacks[i], sizeof(s_stacks[i]), NULL) != MAIA_OK) {
            maia_console_puts("MAIA_NATIVE_PROBE_SETUP_FAIL\n");
            for (;;) __asm__ volatile("wfi");
        }
    }
#ifdef MAIA_NATIVE_PROBE_MIXED
    if (maia_task_create("fpOffHigh", off_priority_worker, NULL, MAIA_PRIO_NORMAL + 1U,
                         s_stacks[2], sizeof(s_stacks[2]), NULL) != MAIA_OK) {
        maia_console_puts("MAIA_NATIVE_PROBE_SETUP_FAIL\n");
        for (;;) __asm__ volatile("wfi");
    }
#endif
    maia_soc_systimer_init(1000U);
    s_start_cycle = cycle_now();
    maia_sched_start();
    maia_console_puts("MAIA_NATIVE_PROBE_START_RETURNED\n");
    for (;;) __asm__ volatile("wfi");
}
