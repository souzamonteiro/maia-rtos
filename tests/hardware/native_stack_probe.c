/* RAM-only deliberate stack faults. Production paths must halt, never resume. */
#include <maia/kernel.h>
#include <maia/console.h>
#include <maia/arch/riscv_stack.h>
#include <maia/soc/esp32p4_display.h>
#include <stdint.h>
uint8_t maia_probe_stack_a[4096] __attribute__((aligned(16)));
static uint8_t stack_b[4096] __attribute__((aligned(16)));
extern void maia_soc_trap_init(void);
extern void maia_probe_stack_entry(uintptr_t new_sp);
extern const uint8_t maia_probe_fault_instruction[];
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }
static void hex32(uint32_t value) {
    static const char digits[]="0123456789abcdef";
    for (unsigned i=0U;i<8U;++i) maia_console_putc(digits[(value>>(28U-4U*i))&15U]);
}
void maia_probe_stack_resumed(void);
void maia_probe_stack_resumed(void) {
    maia_console_puts("FAULT_RESUMED\n");
    for (;;) __asm__ volatile("wfi");
}
static void other(void *arg) { (void)arg; maia_probe_stack_resumed(); }
static void corrupt(void *arg) {
    (void)arg;
    uintptr_t irq_base=maia_trap_stack_metadata.irq_top-MAIA_IRQ_STACK_BYTES;
    if (maia_task_create("reserved",other,NULL,2U,(void *)(irq_base+128U),1024U,NULL) != MAIA_ERR_INVALID ||
        maia_task_create("overlap",other,NULL,2U,maia_probe_stack_a+128U,1024U,NULL) != MAIA_ERR_INVALID ||
        !maia_stack_guard_valid(irq_base)) {
        maia_console_puts("MAIA_NATIVE_PREEMPT_FAIL\n");
        for (;;) __asm__ volatile("wfi");
    }
    maia_console_puts("STACK_PREFLIGHT reserved/overlap rejected\n");
    maia_console_puts("FAULT_ARMED pc="); hex32((uint32_t)(uintptr_t)maia_probe_fault_instruction);
    maia_console_puts("\n");
    uintptr_t bad_sp=0U;
#if MAIA_PROBE_STACK_KIND == 1
    bad_sp=maia_stack_low((uintptr_t)maia_probe_stack_a)+MAIA_RISCV_CONTEXT_BYTES-16U;
#elif MAIA_PROBE_STACK_KIND == 2
    ((volatile uint32_t *)maia_probe_stack_a)[0]=0U;
#elif MAIA_PROBE_STACK_KIND == 3
    ((volatile uint32_t *)stack_b)[0]=0U;
#elif MAIA_PROBE_STACK_KIND == 4
    ((volatile uint32_t *)irq_base)[0]=0U;
#elif MAIA_PROBE_STACK_KIND == 5
    bad_sp=(uintptr_t)maia_probe_stack_a+2049U;
#elif MAIA_PROBE_STACK_KIND != 6
#error "Choose a defined stack fault"
#endif
    maia_probe_stack_entry(bad_sp);
    maia_probe_stack_resumed();
}
void maia_boot_main(void) {
    if (maia_sched_init() != MAIA_OK) maia_probe_stack_resumed();
    maia_soc_trap_init();
    if (maia_task_create("corrupt",corrupt,NULL,2U,maia_probe_stack_a,sizeof maia_probe_stack_a,NULL) != MAIA_OK ||
        maia_task_create("other",other,NULL,2U,stack_b,sizeof stack_b,NULL) != MAIA_OK) maia_probe_stack_resumed();
    maia_sched_start(); maia_probe_stack_resumed();
}
