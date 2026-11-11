/* RAM-only real USB input -> sole owner -> native queue -> consumer. */
#include <maia/services/serial_input.h>
#include <maia/console.h>
#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_display.h>
static uint8_t stacks[3][4096] __attribute__((aligned(16)));
static volatile uint32_t errors, heartbeat;
static unsigned received, paused;
extern void maia_soc_systimer_init(uint32_t hz);
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) {
    uintptr_t state;__asm__ volatile("csrr %0,mstatus" : "=r"(state));
    if (state&8U) ++errors; /* Timer-only stub; legacy RX must not call this. */
}
int maia_tab5_keyboard_getc_nonblocking(void) { ++errors;return -1; }
static void number(uint32_t value) {
    char digits[10];unsigned n=0U;
    do { digits[n++]=(char)('0'+value%10U);value/=10U; } while(value);
    while(n) maia_console_putc(digits[--n]);
}
static void finish(int success) {
    (void)maia_arch_irq_save();
    maia_serial_input_stats_t stats;maia_trap_stats_t traps;
    (void)maia_serial_input_get_stats(&stats);maia_trap_get_stats(&traps);
    success=success && !errors && received == 256U && stats.received == 256U &&
        stats.queued == 0U && stats.full_events>0U && heartbeat>0U &&
        traps.timer_irqs>=250U && maia_tick_get() == traps.timer_irqs;
    maia_console_puts("SERIAL received=");number(received);
    maia_console_puts(" published=");number(stats.received);
    maia_console_puts(" full=");number(stats.full_events);
    maia_console_puts(" polls=");number(stats.polls);
    maia_console_puts(" irq=");number(traps.timer_irqs);
    maia_console_puts(" heartbeat=");number(heartbeat);
    maia_console_puts(" errors=");number(errors);maia_console_puts("\n");
    maia_console_puts(success ? "MAIA_NATIVE_PREEMPT_PASS\n" : "MAIA_NATIVE_PREEMPT_FAIL\n");
    for (;;) __asm__ volatile("wfi");
}
static void owner(void *arg) {
    (void)arg;
    for (;;) {
        maia_status_t status=maia_serial_input_poll();
        if (status != MAIA_OK && status != MAIA_ERR_EMPTY && status != MAIA_ERR_FULL) ++errors;
        maia_task_delay_ticks(1U);
    }
}
static void busy(void *arg) { (void)arg;for (;;) ++heartbeat; }
static void consume(void *arg) {
    (void)arg;
    if (maia_serial_input_poll() != MAIA_ERR_PERM || maia_console_getc_nonblocking() != -1) ++errors;
    maia_console_puts("MAIA_SERIAL_RX_READY fill\n");
    for (;;) {
        maia_serial_input_stats_t stats;
        if (maia_serial_input_get_stats(&stats) != MAIA_OK) { ++errors;finish(0); }
        if (!paused && stats.queued == MAIA_SERIAL_INPUT_CAPACITY && stats.full_events) {
            paused=1U;maia_console_puts("MAIA_SERIAL_RX_READY drain\n");
        }
        if (paused) {
            uint8_t byte;
            maia_status_t status=maia_serial_input_read(&byte);
            if (status == MAIA_OK) {
                if (received>=256U || byte != (uint8_t)received) ++errors;
                ++received;
            } else if (status == MAIA_ERR_EMPTY) maia_task_delay_ticks(1U);
            else ++errors;
        } else maia_task_delay_ticks(1U);
        if (received == 256U && stats.received == 256U && maia_tick_get()>=250U) finish(1);
        if (maia_tick_get()>4000U) { ++errors;finish(0); }
    }
}
void maia_boot_main(void) {
    maia_task_id_t id;
    if (maia_sched_init() != MAIA_OK ||
        maia_task_create("serial-rx",owner,NULL,3U,stacks[0],sizeof stacks[0],&id) != MAIA_OK ||
        maia_task_create("consumer",consume,NULL,2U,stacks[1],sizeof stacks[1],NULL) != MAIA_OK ||
        maia_task_create("busy",busy,NULL,2U,stacks[2],sizeof stacks[2],NULL) != MAIA_OK ||
        maia_serial_input_init(id) != MAIA_OK) {
        maia_console_puts("MAIA_NATIVE_PREEMPT_FAIL\n");for (;;) __asm__ volatile("wfi");
    }
    maia_soc_systimer_init(1000U);maia_sched_start();
}
