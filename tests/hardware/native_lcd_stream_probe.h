#ifdef MAIA_NATIVE_PROBE_TOUCH_LCD
#include "native_touch_lcd_probe.h"
#elif defined(MAIA_NATIVE_PROBE_LCD_LIFECYCLE)
#include "native_lcd_lifecycle_probe.h"
#else
/* Kernel-driven LCD session; other tasks prove work during DMA IRQ waits.
 * Diagnostic shutdown stops all test clients before boot-style memory restore. */
#include <maia/services/lcd_output.h>
#include <maia/arch/irq.h>
#include "../../soc/espressif/esp32p4/lcd_async.h"
#include "../../soc/espressif/esp32p4/dw_dma_irq.h"
static uint8_t stream_stacks[2][8192] __attribute__((aligned(16)));
static uint8_t stream_small_stacks[2][4096] __attribute__((aligned(16)));
static maia_task_id_t stream_ids[4];
static volatile uint32_t stream_done,stream_beats,stream_work,stream_errors;
static uint32_t stream_visual,stream_hash,stream_duration,stream_map_before,stream_ctrl_before,stream_stop_active;
static maia_psram_snapshot_t stream_before;
static dpi_backlight_snapshot_t stream_light;
static maia_status_t stream_run_result;
extern void maia_soc_systimer_init(uint32_t);
static void lcd_worker(void *arg){(void)arg;stream_run_result=maia_lcd_output_run();stream_done=1;(void)maia_task_notify(stream_ids[1]);}
static void lcd_beat(void *arg){(void)arg;while(!stream_done){++stream_beats;maia_task_delay_ticks(1);}}
static void lcd_load(void *arg){(void)arg;while(!stream_done){++stream_work;__asm__ volatile("" ::: "memory");}}
static void lcd_controller(void *arg){
    (void)arg;uint64_t began=maia_tick_get();maia_task_delay_ticks(stream_duration);
    for(unsigned i=0;i<20U;++i){
        uintptr_t irq=maia_arch_irq_save();stream_stop_active=rd(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U;
        if(stream_stop_active&&maia_lcd_output_request_stop()!=MAIA_OK)++stream_errors;
        maia_arch_irq_restore(irq);if(stream_stop_active)break;maia_task_delay_ticks(1);
    }
    if(!stream_stop_active){++stream_errors;(void)maia_lcd_output_request_stop();}
    if(maia_task_wait_notification(200)!=MAIA_OK)++stream_errors;
    maia_task_delay_ticks(3);
    (void)maia_arch_irq_save();
    maia_lcd_output_stats_t stats;maia_lcd_async_report_t engine;maia_trap_stats_t traps;
    (void)maia_lcd_output_stats(&stats);maia_lcd_async_report(&engine);maia_trap_get_stats(&traps);
    unsigned exited=0;for(unsigned i=0;i<4U;++i)if(i!=1U){maia_task_info_t info;if(maia_task_get_info((uint32_t)stream_ids[i],&info)==MAIA_OK&&info.state==MAIA_TASK_EXITED)++exited;}
    field("LCD_STREAM_ROUTE map_before=",stream_map_before);field(" map_after=",rd(MAIA_ESP32P4_INTR_CORE0_MAP_REG(24)));field(" ctrl_before=",stream_ctrl_before);field(" ctrl_after=",rd(MAIA_ESP32P4_CLIC_INT_CTRL_REG(maia_dw_irq_line())));maia_console_puts("\n");
    field("LCD_STREAM_TASK result=",(uint32_t)stream_run_result);field(" frames=",stats.frames);field(" waits=",stats.waits);field(" timeouts=",stats.timeouts);field(" errors=",stats.errors);field(" stopped=",stats.stopped);field(" requested=",stats.stop_requested);field(" active_stop=",stream_stop_active);maia_console_puts("\n");
    field("LCD_STREAM_IRQ line=",maia_dw_irq_line());field(" source=",MAIA_DW_IRQ_SOURCE);field(" irqs=",engine.irqs);field(" posts=",engine.notifications);field(" spurious=",engine.spurious);field(" drains=",engine.drain_retries);field(" released=",engine.released);field(" done=",engine.last.done);field(" recovered=",engine.last.recovered);field(" raw=",engine.last.count_raw);field(" status=",engine.last.channel_status);field(" common=",engine.last.common_status);field(" bridge=",engine.last.bridge_status);field(" error0=",engine.last.observation.error0);field(" error1=",engine.last.observation.error1);maia_console_puts("\n");
    field("LCD_STREAM_KERNEL ticks=",(uint32_t)(maia_tick_get()-began));field(" timer=",traps.timer_irqs);field(" external=",traps.external_irqs);field(" ecalls=",traps.ecall_count);field(" beats=",stream_beats);field(" work=",stream_work);field(" exited=",exited);field(" errors=",stream_errors);maia_console_puts("\n");
    if(stream_run_result!=MAIA_OK||!stats.stopped||stats.errors||stats.timeouts||stats.frames<40U||stats.waits<stats.frames||engine.irqs!=stats.frames||engine.notifications!=engine.irqs||engine.spurious||!engine.released||traps.external_irqs!=engine.irqs||traps.timer_irqs<stream_duration||stream_beats<stream_duration/2U||stream_work<10000U||exited!=3U||stream_errors)halt("lcd-stream-result");
    if(stream_visual&&!dpi_bl_restore(rd,wr,&stream_light))halt("stream-light-restore");
    fb_finish(stream_hash,&stream_before);
    maia_console_puts("MAIA_NATIVE_LCD_STREAM_PASS\n");for(;;)__asm__ volatile("wfi");
}
static __attribute__((noreturn)) void lcd_stream_launch(unsigned visual,uint32_t hash,const maia_psram_snapshot_t *before,const dpi_backlight_snapshot_t *light){
    stream_visual=visual;stream_hash=hash;stream_before=*before;stream_light=*light;stream_duration=visual?12000U:1500U;
    stream_map_before=rd(MAIA_ESP32P4_INTR_CORE0_MAP_REG(24));stream_ctrl_before=0;
    maia_status_t result=maia_sched_init();
    field("LCD_STREAM_INIT part=",0);field(" result=",(uint32_t)result);maia_console_puts("\n");if(result!=MAIA_OK)halt("lcd-sched-init");
    const maia_task_entry_fn entries[4]={lcd_worker,lcd_controller,lcd_beat,lcd_load};
    const char *names[4]={"lcd","lcd-control","heartbeat","cpu-load"};
    for(unsigned i=0;i<4;++i){
        void *stack=i<2U?(void *)stream_stacks[i]:(void *)stream_small_stacks[i-2U];
        unsigned bytes=i<2U?sizeof stream_stacks[0]:sizeof stream_small_stacks[0];
        result=maia_task_create(names[i],entries[i],0,(uint8_t)(4U-i),stack,bytes,&stream_ids[i]);
        field("LCD_STREAM_INIT part=",i+1U);field(" result=",(uint32_t)result);maia_console_puts("\n");if(result!=MAIA_OK)halt("lcd-task-create");
    }
    result=maia_lcd_output_init(stream_ids[0]);stream_ctrl_before=maia_dw_irq_saved_ctrl();maia_lcd_async_report_t prepared;maia_lcd_async_report(&prepared);
    field("LCD_STREAM_INIT part=",5);field(" result=",(uint32_t)result);maia_console_puts("\n");
    field("LCD_STREAM_PREP engine=",prepared.init_stage);field(" irq=",maia_dw_irq_stage());field(" map=",stream_map_before);field(" ctrl=",stream_ctrl_before);maia_console_puts("\n");
    if(result!=MAIA_OK)halt("lcd-stream-init");
    maia_soc_systimer_init(1000);maia_sched_start();halt("lcd-stream-return");for(;;)__asm__ volatile("wfi");
}

#endif
