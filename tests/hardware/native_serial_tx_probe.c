/* RAM-only multiple producer records -> sole USB TX owner. */
#include <maia/services/serial_output.h>
#include <maia/console.h>
#include <maia/arch/irq.h>
static uint8_t stacks[3][4096] __attribute__((aligned(16)));
static volatile uint32_t errors,done,full;
static int reporting;
extern void maia_soc_systimer_init(uint32_t hz);
void maia_display_putc(char c) { (void)c;++errors; }
void maia_display_refresh_dma_if_needed(void) { }
static void add(char *buf, unsigned *n, const char *text) { while (*text && *n<64U) buf[(*n)++]=*text++; }
static void number(char *buf, unsigned *n, uint32_t value) {
    char digits[10];unsigned count=0U;
    do { digits[count++]=(char)('0'+value%10U);value/=10U; } while(value);
    while(count && *n<64U) buf[(*n)++]=digits[--count];
}
static void report(maia_serial_output_stats_t *stats, int pass) {
    char line[64];unsigned n=0U;
    add(line,&n,"TX_SUM records=");number(line,&n,stats->completed_records);
    add(line,&n," bytes=");number(line,&n,stats->written_bytes);
    add(line,&n," errors=");number(line,&n,errors);add(line,&n,"\n");
    if (maia_serial_output_write(line,n) != MAIA_OK) ++errors;
    n=0U;add(line,&n,"TX_HW irq=");number(line,&n,(uint32_t)maia_tick_get());
    add(line,&n," full=");number(line,&n,full);
    add(line,&n," busy=");number(line,&n,stats->busy_events);add(line,&n,"\n");
    if (maia_serial_output_write(line,n) != MAIA_OK) ++errors;
    const char *result=pass && !errors ? "MAIA_NATIVE_PREEMPT_PASS\n" : "MAIA_NATIVE_PREEMPT_FAIL\n";
    n=0U;while(result[n]) ++n;
    if (maia_serial_output_write(result,n) != MAIA_OK) ++errors;
}
static void owner(void *arg) {
    (void)arg;
    maia_task_delay_ticks(20U); /* Producers must observe queue saturation first. */
    for (;;) {
        for (unsigned attempt=0U;attempt<2U;++attempt) {
            maia_status_t result=maia_serial_output_poll();
            if (result != MAIA_OK && result != MAIA_ERR_EMPTY && result != MAIA_ERR_BUSY) ++errors;
        }
        maia_serial_output_stats_t stats;(void)maia_serial_output_get_stats(&stats);
        if (!reporting && ((done == 2U && stats.completed_records == 80U &&
            !stats.pending_bytes && !stats.queued_records && maia_tick_get()>=250U) || maia_tick_get()>4000U)) {
            maia_trap_stats_t traps;maia_trap_get_stats(&traps);
            int pass=!errors && done == 2U && full>0U && stats.busy_events>0U && stats.accepted_records == 80U &&
                stats.completed_records == 80U && stats.accepted_bytes == 3840U && stats.written_bytes == 3840U &&
                !stats.queued_records && !stats.pending_bytes && traps.timer_irqs>=250U;
            reporting=1;report(&stats,pass);
        } else if (reporting && !stats.queued_records && !stats.pending_bytes) {
            (void)maia_arch_irq_save();for (;;) __asm__ volatile("wfi");
        }
        maia_task_delay_ticks(1U);
    }
}
static void producer(void *arg) {
    unsigned source=(unsigned)(uintptr_t)arg;
    if (maia_serial_output_poll() != MAIA_ERR_PERM) ++errors;
    maia_console_puts("LEGACY_MUST_NOT_APPEAR\n");
    for (unsigned seq=0U;seq<40U;) {
        uint8_t record[48];
        record[0]='T';record[1]='X';record[2]='R';record[3]=':';
        record[4]=(uint8_t)('A'+source);record[5]=':';
        record[6]=(uint8_t)('0'+seq/10U);record[7]=(uint8_t)('0'+seq%10U);record[8]=':';
        for (unsigned i=9U;i<47U;++i) record[i]=(uint8_t)('a'+(source*7U+seq+i)%26U);
        record[47]='\n';
        maia_status_t status=maia_serial_output_write(record,sizeof record);
        if (status == MAIA_OK) ++seq;
        else if (status == MAIA_ERR_FULL) { uintptr_t irq=maia_arch_irq_save();++full;maia_arch_irq_restore(irq);maia_task_delay_ticks(1U); }
        else { ++errors;break; }
    }
    uintptr_t irq=maia_arch_irq_save();++done;maia_arch_irq_restore(irq);
}
void maia_boot_main(void) {
    maia_task_id_t id;
    if (maia_sched_init() != MAIA_OK ||
        maia_task_create("serial-tx",owner,NULL,3U,stacks[0],sizeof stacks[0],&id) != MAIA_OK ||
        maia_task_create("writerA",producer,NULL,2U,stacks[1],sizeof stacks[1],NULL) != MAIA_OK ||
        maia_task_create("writerB",producer,(void *)(uintptr_t)1U,2U,stacks[2],sizeof stacks[2],NULL) != MAIA_OK ||
        maia_serial_output_init(id,MAIA_SERIAL_TX_USB) != MAIA_OK) {
        maia_console_puts("MAIA_NATIVE_PREEMPT_FAIL\n");for (;;) __asm__ volatile("wfi");
    }
    maia_soc_systimer_init(1000U);maia_sched_start();
}
