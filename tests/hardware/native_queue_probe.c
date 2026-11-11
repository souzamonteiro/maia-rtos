/* RAM-only queue stress: two periodic burst producers, a real timer-IRQ producer and a
 * busy consumer. Only the diagnostic overrides the existing display callback. */
#include <maia/kernel.h>
#include <maia/console.h>
#include <maia/arch/irq.h>
#include <maia/arch/riscv_stack.h>
#include <maia/soc/esp32p4_display.h>
#include <stdint.h>

typedef struct packet { uint32_t source, sequence, words[62]; } packet_t;
_Static_assert(sizeof(packet_t) == MAIA_QUEUE_MAX_ITEM_BYTES,"exercise maximum copy");
static maia_queue_t queue;
static packet_t storage[7];
static uint8_t stacks[3][4096] __attribute__((aligned(16)));
static volatile uint32_t sent[3], received[3], full[3], errors, stop, done, irq_checks;
extern void maia_soc_systimer_init(uint32_t hz);
void maia_display_putc(char c) { (void)c; }
static uint32_t pattern(uint32_t source, uint32_t sequence, unsigned index) {
    return 0x5a71c900U ^ (source*0x03030303U) ^ (sequence*31U) ^ (index*0x01010101U);
}
static void offer(unsigned source) {
    packet_t p; p.source=source; p.sequence=sent[source];
    for (unsigned i=0U;i<62U;++i) p.words[i]=pattern(source,p.sequence,i);
    maia_status_t status=maia_queue_send(&queue,&p);
    if (status == MAIA_OK) ++sent[source];
    else if (status == MAIA_ERR_FULL) ++full[source];
    else ++errors;
}
void maia_display_refresh_dma_if_needed(void) {
    uintptr_t here, status;
    __asm__ volatile("mv %0, sp" : "=r"(here));
    __asm__ volatile("csrr %0, mstatus" : "=r"(status));
    uintptr_t top=maia_trap_stack_metadata.irq_top;
    if (here < top-MAIA_IRQ_STACK_BYTES+MAIA_RISCV_STACK_GUARD_BYTES || here >= top ||
        !maia_stack_guard_valid(top-MAIA_IRQ_STACK_BYTES) || (status&8U)) ++errors;
    ++irq_checks;
    if (!stop) offer(0U);
    __asm__ volatile("csrr %0, mstatus" : "=r"(status));
    if ((status&8U) || maia_queue_count(&queue)>7U) ++errors;
}
static void number(uint32_t value) {
    char digits[10];unsigned n=0U;
    do { digits[n++]=(char)('0'+value%10U);value/=10U; } while(value);
    while(n) maia_console_putc(digits[--n]);
}
static void producer(void *arg) {
    unsigned source=(unsigned)(uintptr_t)arg;
    while (!stop) {
        for (unsigned burst=0U; burst<3U; ++burst) offer(source);
        maia_task_delay_ticks(2U+source);
        uintptr_t status; __asm__ volatile("csrr %0, mstatus" : "=r"(status));
        if (!(status&8U)) ++errors;
    }
    uintptr_t irq=maia_arch_irq_save(); ++done; maia_arch_irq_restore(irq);
}
static void finish(void) {
    (void)maia_arch_irq_save();
    maia_trap_stats_t traps; maia_trap_get_stats(&traps);
    int pass=!errors && done == 2U && traps.timer_irqs >= 250U &&
             traps.timer_irqs == maia_tick_get() && irq_checks == traps.timer_irqs &&
             traps.sync_exceptions == traps.ecall_count && maia_queue_count(&queue) == 0U;
    maia_console_puts("QUEUE irq=");number(traps.timer_irqs);
    maia_console_puts(" switches=");number(maia_sched_switch_count());
    maia_console_puts(" errors=");number(errors);
    maia_console_puts(" done=");number(done);maia_console_puts("\n");
    for (unsigned i=0U;i<3U;++i) {
        pass=pass && sent[i] == received[i] && received[i] >= 20U;
        maia_console_puts("SOURCE id=");number(i);
        maia_console_puts(" sent=");number(sent[i]);
        maia_console_puts(" received=");number(received[i]);
        maia_console_puts(" full=");number(full[i]);maia_console_puts("\n");
    }
    maia_console_puts(pass ? "MAIA_NATIVE_PREEMPT_PASS\n" : "MAIA_NATIVE_PREEMPT_FAIL\n");
    for (;;) __asm__ volatile("wfi");
}
static void consumer(void *arg) {
    (void)arg;
    for (;;) {
        packet_t p;
        maia_status_t status=maia_queue_recv(&queue,&p);
        if (status == MAIA_OK) {
            if (p.source > 2U) ++errors;
            else {
                if (p.sequence != received[p.source]) ++errors;
                ++received[p.source];
                for (unsigned i=0U;i<62U;++i) if (p.words[i] != pattern(p.source,p.sequence,i)) ++errors;
            }
        } else if (status != MAIA_ERR_EMPTY) ++errors;
        if (maia_tick_get() >= 250U) stop=1U;
        if (stop && done == 2U && maia_queue_count(&queue) == 0U) finish();
        if (maia_tick_get() > 400U) { ++errors; finish(); }
    }
}
void maia_boot_main(void) {
    maia_console_puts("MAIA_QUEUE_PROBE: two task producers, timer IRQ producer, 256-byte packets\n");
    if (maia_sched_init() != MAIA_OK || maia_queue_init(&queue,storage,sizeof(packet_t),7U) != MAIA_OK ||
        maia_task_create("queueA",producer,(void *)(uintptr_t)1U,2U,stacks[0],sizeof stacks[0],NULL) != MAIA_OK ||
        maia_task_create("queueB",producer,(void *)(uintptr_t)2U,2U,stacks[1],sizeof stacks[1],NULL) != MAIA_OK ||
        maia_task_create("consumer",consumer,NULL,2U,stacks[2],sizeof stacks[2],NULL) != MAIA_OK) {
        ++errors; finish();
    }
    maia_soc_systimer_init(1000U); maia_sched_start();
    ++errors; finish();
}
