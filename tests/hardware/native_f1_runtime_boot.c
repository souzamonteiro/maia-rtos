#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/runtime/newlib_locks.h>

#include <stddef.h>
#include <stdint.h>

extern void maia_soc_systimer_init(uint32_t tick_hz);
extern int maia_f1_runtime_smoke_start(
    void *stack_a, size_t stack_a_size,
    void *stack_b, size_t stack_b_size,
    void *stack_report, size_t stack_report_size);

static uint8_t g_heap_pool[32768] __attribute__((aligned(16)));
static uint8_t g_stack_a[4096] __attribute__((aligned(16)));
static uint8_t g_stack_b[4096] __attribute__((aligned(16)));
static uint8_t g_stack_report[8192] __attribute__((aligned(16)));

void maia_display_putc(char c) {
    (void)c;
}

void maia_display_refresh_dma_if_needed(void) {
}

int maia_tab5_keyboard_getc_nonblocking(void) {
    return -1;
}

void maia_boot_main(void) {
    if (maia_sched_init() != MAIA_OK ||
        maia_mem_init(g_heap_pool, sizeof(g_heap_pool)) != MAIA_OK ||
        maia_newlib_locks_init() != MAIA_OK) {
        maia_console_puts("[F1] runtime heap/lock initialization failed\n");
        for (;;) __asm__ volatile("wfi");
    }

    maia_soc_systimer_init(1000U);
    maia_console_puts("[F1] Newlib/gthread runtime probe starting\n");
    int start_result = maia_f1_runtime_smoke_start(
        g_stack_a, sizeof(g_stack_a),
        g_stack_b, sizeof(g_stack_b),
        g_stack_report, sizeof(g_stack_report));
    if (start_result != 0) {
        if (start_result == 3) {
            maia_console_puts("[F1] C++ global constructor/init array failed\n");
        } else {
            maia_console_puts("[F1] runtime smoke task setup failed\n");
        }
        for (;;) __asm__ volatile("wfi");
    }

    maia_sched_start();
    for (;;) __asm__ volatile("wfi");
}
