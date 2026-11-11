/* RAM-only event-driven TX: quiet empty periods, task writers and an IRQ writer. */
#include <maia/services/serial_output.h>
#include <maia/console.h>
#include <maia/arch/irq.h>
static uint8_t stacks[3][4096] __attribute__((aligned(16)));
static maia_task_id_t owner_id,writer_ids[2];
static volatile uint32_t stage,errors,done,full,head_checks,tail_checks,irq_records;
static uint32_t observed_polls,observed_run_ticks,empty_calls,empty_returns,retry_calls,retry_timeouts;
static int held,reporting;
static const char trigger[]="TX_EVENT_TRIGGER\n";
extern void maia_soc_systimer_init(uint32_t hz);
static void check(int condition) { if (!condition) { uintptr_t irq=maia_arch_irq_save();++errors;maia_arch_irq_restore(irq); } }
void maia_display_putc(char c) { (void)c;check(0); }
void maia_display_refresh_dma_if_needed(void) {
    maia_task_info_t owner,a,b;maia_serial_output_stats_t stats;
    if (maia_task_get_info((uint32_t)owner_id,&owner) != MAIA_OK ||
        maia_task_get_info((uint32_t)writer_ids[0],&a) != MAIA_OK ||
        maia_task_get_info((uint32_t)writer_ids[1],&b) != MAIA_OK ||
        maia_serial_output_get_stats(&stats) != MAIA_OK) { check(0);return; }
    if (owner.state != MAIA_TASK_BLOCKED || stats.queued_records || stats.pending_bytes) return;
    if (!stage && a.state == MAIA_TASK_BLOCKED && b.state == MAIA_TASK_BLOCKED) {
        if (!head_checks) { observed_polls=stats.polls;observed_run_ticks=owner.run_ticks; }
        check(stats.polls == observed_polls && owner.run_ticks == observed_run_ticks && !stats.accepted_records);
        if (++head_checks == 20U) {
            stage=1U;
            check(maia_task_notify(writer_ids[0]) == MAIA_OK);
            check(maia_task_notify(writer_ids[1]) == MAIA_OK);
        }
    } else if (stage == 1U && done == 2U && a.state == MAIA_TASK_EXITED && b.state == MAIA_TASK_EXITED && stats.completed_records == 80U) {
        if (!tail_checks) { observed_polls=stats.polls;observed_run_ticks=owner.run_ticks; }
        check(stats.polls == observed_polls && owner.run_ticks == observed_run_ticks &&
            stats.accepted_records == 80U && stats.accepted_bytes == 3840U && stats.written_bytes == 3840U);
        ++tail_checks;
        if (tail_checks>=20U && maia_tick_get()>=300U) {
            /* Only a published record wakes the owner: no timer poll or direct
             * notification to that task substitutes for the service write. */
            check(maia_serial_output_write(trigger,sizeof trigger-1U) == MAIA_OK);
            ++irq_records;stage=2U;
        }
    }
}
static void add(char *buf,unsigned *n,const char *s) { while (*s && *n<64U) buf[(*n)++]=*s++; }
static void number(char *buf,unsigned *n,uint32_t value) {
    char digits[10];unsigned count=0U;
    do { digits[count++]=(char)('0'+value%10U);value/=10U; } while(value);
    while(count && *n<64U) buf[(*n)++]=digits[--count];
}
static void send(const char *line,unsigned n) { check(maia_serial_output_write(line,n) == MAIA_OK); }
static void report(maia_serial_output_stats_t *stats,int pass) {
    char line[64];unsigned n=0U;maia_idle_stats_t idle;(void)maia_sched_get_idle_stats(&idle);
    add(line,&n,"TX_SUM records=80 bytes=3840 errors=");number(line,&n,errors);add(line,&n,"\n");send(line,n);
    n=0U;add(line,&n,"TX_HW irq=");number(line,&n,(uint32_t)maia_tick_get());
    add(line,&n," full=");number(line,&n,full);add(line,&n," busy=");number(line,&n,stats->busy_events);add(line,&n,"\n");send(line,n);
    n=0U;add(line,&n,"TX_EVENT head=");number(line,&n,head_checks);add(line,&n," tail=");number(line,&n,tail_checks);
    add(line,&n," idle=");number(line,&n,(uint32_t)idle.ticks);add(line,&n,"\n");send(line,n);
    n=0U;add(line,&n,"TX_WAIT empty=");number(line,&n,empty_calls);add(line,&n," retry=");number(line,&n,retry_calls);
    add(line,&n," timeout=");number(line,&n,retry_timeouts);add(line,&n," irq_write=");number(line,&n,irq_records);add(line,&n,"\n");send(line,n);
    const char *result=pass && !errors ? "MAIA_NATIVE_PREEMPT_PASS\n" : "MAIA_NATIVE_PREEMPT_FAIL\n";
    n=0U;while(result[n]) ++n;send(result,n);
}
static void owner(void *arg) {
    (void)arg;
    for (;;) {
        maia_status_t result=maia_serial_output_poll();
        check(result == MAIA_OK || result == MAIA_ERR_EMPTY || result == MAIA_ERR_BUSY);
        maia_serial_output_stats_t stats;(void)maia_serial_output_get_stats(&stats);
        if (!reporting && stage == 2U && stats.completed_records == 81U && !stats.pending_bytes && !stats.queued_records) {
            maia_trap_stats_t traps;maia_idle_stats_t idle;
            maia_trap_get_stats(&traps);(void)maia_sched_get_idle_stats(&idle);
            int pass=!errors && done == 2U && head_checks>=20U && tail_checks>=20U && irq_records == 1U &&
                empty_calls>=2U && empty_returns>=2U && retry_calls>0U && retry_timeouts>0U &&
                full>0U && stats.busy_events>0U && stats.accepted_records == 81U &&
                stats.accepted_bytes == 3840U+sizeof trigger-1U && stats.written_bytes == stats.accepted_bytes &&
                traps.timer_irqs>=300U && maia_tick_get() == traps.timer_irqs && idle.ticks>=39U;
            reporting=1;report(&stats,pass);continue;
        }
        if (reporting && !stats.pending_bytes && !stats.queued_records) {
            (void)maia_arch_irq_save();for (;;) __asm__ volatile("wfi");
        }
        if (result == MAIA_ERR_EMPTY) {
            ++empty_calls;check(maia_serial_output_wait() == MAIA_OK);++empty_returns;
            if (!held) { held=1;maia_task_delay_ticks(20U); } /* Force initial FULL. */
        } else if (result == MAIA_ERR_BUSY) {
            ++retry_calls;
            maia_status_t waited=maia_task_wait_notification(1U);
            check(waited == MAIA_OK || waited == MAIA_ERR_TIMEOUT);
            if (waited == MAIA_ERR_TIMEOUT) ++retry_timeouts;
        }
    }
}
static void producer(void *arg) {
    unsigned source=(unsigned)(uintptr_t)arg;
    check(maia_serial_output_wait() == MAIA_ERR_PERM);
    check(maia_task_wait_notification(MAIA_TASK_WAIT_FOREVER) == MAIA_OK);
    for (unsigned seq=0U;seq<40U;) {
        uint8_t record[48];
        record[0]='T';record[1]='X';record[2]='R';record[3]=':';record[4]=(uint8_t)('A'+source);record[5]=':';
        record[6]=(uint8_t)('0'+seq/10U);record[7]=(uint8_t)('0'+seq%10U);record[8]=':';
        for (unsigned i=9U;i<47U;++i) record[i]=(uint8_t)('a'+(source*7U+seq+i)%26U);
        record[47]='\n';maia_status_t result=maia_serial_output_write(record,sizeof record);
        if (result == MAIA_OK) ++seq;
        else if (result == MAIA_ERR_FULL) { uintptr_t irq=maia_arch_irq_save();++full;maia_arch_irq_restore(irq);maia_task_delay_ticks(1U); }
        else { check(0);break; }
    }
    uintptr_t irq=maia_arch_irq_save();++done;maia_arch_irq_restore(irq);
}
void maia_boot_main(void) {
    if (maia_sched_init() != MAIA_OK ||
        maia_task_create("event-tx",owner,NULL,4U,stacks[0],sizeof stacks[0],&owner_id) != MAIA_OK ||
        maia_task_create("writerA",producer,NULL,2U,stacks[1],sizeof stacks[1],&writer_ids[0]) != MAIA_OK ||
        maia_task_create("writerB",producer,(void *)(uintptr_t)1U,2U,stacks[2],sizeof stacks[2],&writer_ids[1]) != MAIA_OK ||
        maia_serial_output_init(owner_id,MAIA_SERIAL_TX_USB) != MAIA_OK) {
        maia_console_puts("MAIA_NATIVE_PREEMPT_FAIL\n");for (;;) __asm__ volatile("wfi");
    }
    maia_soc_systimer_init(1000U);maia_sched_start();
}
