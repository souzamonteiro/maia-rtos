/* Original MaiaRTOS MIT. Three warm sessions, two complete image edits each.
 * RAM-only diagnostic; no other PSRAM clients. Timer/heartbeat/load stay live. */
#include <maia/services/lcd_output.h>
#include <maia/arch/irq.h>
#include "../../soc/espressif/esp32p4/lcd_async.h"
#include "../../soc/espressif/esp32p4/dw_dma_irq.h"
static uint8_t life_stacks[2][8192] __attribute__((aligned(16)));
static uint8_t life_small[2][4096] __attribute__((aligned(16)));
static maia_task_id_t life_ids[4];
static maia_lcd_output_stats_t life_stats[3];
static maia_lcd_async_report_t life_engines[3];
static volatile unsigned life_ready,life_restart_allowed,life_finished,life_done,life_beats,life_work,life_errors,life_draws;
static uint32_t life_hashes[6],life_draw_ticks[6],life_draw_beats[6];
static uint32_t life_visual,life_hash,life_duration,life_map,life_ctrl,life_active_stops;
static maia_psram_snapshot_t life_before;
static dpi_backlight_snapshot_t life_light;
extern void maia_soc_systimer_init(uint32_t);
static void life_draw(uint16_t *pixels,uint32_t width,uint32_t height,void *arg){
    (void)arg;unsigned n=life_draws;uint64_t began=maia_tick_get();unsigned beats=life_beats;
    uintptr_t irq=maia_arch_irq_save();
    if(!irq||pixels!=(uint16_t *)(uintptr_t)DSI_FB_SOURCE||width!=720U||height!=1280U||n>=6U||
       rd(MAIA_ESP32P4_GDMA_BASE+0x10U)||rd(MAIA_ESP32P4_GDMA_BASE+0x18U))++life_errors;
    maia_psram_window_report_t check;
    if(maia_esp32p4_psram_window_sync(1,&check)!=MAIA_PSRAM_BUSY||
       maia_esp32p4_psram_window_stop(1,&check)!=MAIA_PSRAM_BUSY)++life_errors;
    maia_arch_irq_restore(irq);
    if(n>=6U)return;
    for(unsigned i=0;i<720U*1280U;++i)pixels[i]^=0xFFFFU;
    /* Deliberately yield during CPU ownership to prove the edit lease survives
     * actual scheduling, rather than relying on a frame-long IRQ exclusion. */
    maia_task_delay_ticks(3);
    life_hashes[n]=fb_hash((const volatile uint8_t *)pixels,FB_BYTES);
    life_draw_ticks[n]=(uint32_t)(maia_tick_get()-began);life_draw_beats[n]=life_beats-beats;
    life_draws=n+1U;
}
static void life_worker(void *arg){
    (void)arg;
    for(unsigned c=0;c<3U;++c){
        if(c&&maia_lcd_output_restart()!=MAIA_OK){++life_errors;break;}
        life_ready=c+1U;
        if(maia_lcd_output_run()!=MAIA_OK)++life_errors;
        uintptr_t irq=maia_arch_irq_save();
        (void)maia_lcd_output_stats(&life_stats[c]);maia_lcd_async_report(&life_engines[c]);
        maia_arch_irq_restore(irq);life_finished=c+1U;(void)maia_task_notify(life_ids[1]);
        while(c<2U&&life_restart_allowed<c+1U)if(maia_task_wait_notification(1000)!=MAIA_OK){++life_errors;break;}
    }
    life_done=1;(void)maia_task_notify(life_ids[1]);
}
static void life_beat(void *arg){(void)arg;while(!life_done){++life_beats;maia_task_delay_ticks(1);}}
static void life_load(void *arg){(void)arg;while(!life_done){++life_work;__asm__ volatile("" ::: "memory");}}
static int life_presented(unsigned count){
    for(unsigned i=0;i<200;++i){maia_lcd_output_stats_t s;(void)maia_lcd_output_stats(&s);if(s.presented>=count)return 1;maia_task_delay_ticks(1);}return 0;
}
static void life_controller(void *arg){
    (void)arg;uint64_t began=maia_tick_get();
    for(unsigned c=0;c<3U;++c){
        for(unsigned i=0;i<200U&&life_ready<c+1U;++i)maia_task_delay_ticks(1);
        if(life_ready!=c+1U)halt("life-ready");
        uint64_t session=maia_tick_get();
        for(unsigned edit=1;edit<=2;++edit){
            maia_task_delay_ticks(life_duration/4U);
            uintptr_t irq=maia_arch_irq_save();
            if(maia_lcd_output_request_update(life_draw,0)!=MAIA_OK)++life_errors;
            if(maia_lcd_output_request_update(life_draw,0)!=MAIA_ERR_BUSY)++life_errors;
            maia_arch_irq_restore(irq);
            if(!life_presented(edit))halt("life-publish");
        }
        uint64_t elapsed=maia_tick_get()-session;if(elapsed<life_duration)maia_task_delay_ticks((uint32_t)(life_duration-elapsed));
        unsigned active=0;
        for(unsigned i=0;i<20U;++i){uintptr_t irq=maia_arch_irq_save();active=rd(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U;
            if(active&&maia_lcd_output_request_stop()!=MAIA_OK)++life_errors;
            maia_arch_irq_restore(irq);if(active)break;maia_task_delay_ticks(1);}
        if(!active)halt("life-active-stop");
        ++life_active_stops;
        for(unsigned i=0;i<200U&&life_finished<c+1U;++i)maia_task_delay_ticks(1);
        if(life_finished!=c+1U)halt("life-stopped");
        if(c<2U){life_restart_allowed=c+1U;(void)maia_task_notify(life_ids[0]);}
    }
    maia_task_delay_ticks(3);(void)maia_arch_irq_save();
    maia_trap_stats_t traps;maia_trap_get_stats(&traps);
    unsigned exited=0;for(unsigned i=0;i<4U;++i)if(i!=1U){maia_task_info_t info;if(maia_task_get_info((uint32_t)life_ids[i],&info)==MAIA_OK&&info.state==MAIA_TASK_EXITED)++exited;}
    unsigned frames=0,waits=0,irqs=0,posts=0,drains=0;
    for(unsigned c=0;c<3U;++c){maia_lcd_output_stats_t *s=&life_stats[c];maia_lcd_async_report_t *e=&life_engines[c];
        field("LCD_CYCLE index=",c);field(" frames=",s->frames);field(" updates=",s->updates);field(" presented=",s->presented);field(" irqs=",e->irqs);field(" published=",e->updates);field(" stopped=",s->stopped);field(" released=",e->released);field(" errors=",s->errors);field(" timeouts=",s->timeouts);maia_console_puts("\n");
        if(s->frames<20U||s->updates!=2U||s->presented!=2U||e->updates!=2U||s->frames!=e->irqs||e->irqs!=e->notifications||!s->stopped||!e->released||s->errors||s->timeouts||e->spurious)++life_errors;
        frames+=s->frames;waits+=s->waits;irqs+=e->irqs;posts+=e->notifications;drains+=e->drain_retries;
    }
    for(unsigned n=0;n<6U;++n){field("LCD_EDIT index=",n);field(" hash=",life_hashes[n]);field(" ticks=",life_draw_ticks[n]);field(" beats=",life_draw_beats[n]);maia_console_puts("\n");if(!life_draw_beats[n]||life_draw_ticks[n]<3U)++life_errors;}
    maia_lcd_async_report_t *e=&life_engines[2];
    field("LCD_STREAM_ROUTE map_before=",life_map);field(" map_after=",rd(MAIA_ESP32P4_INTR_CORE0_MAP_REG(24)));field(" ctrl_before=",life_ctrl);field(" ctrl_after=",rd(MAIA_ESP32P4_CLIC_INT_CTRL_REG(maia_dw_irq_line())));maia_console_puts("\n");
    field("LCD_STREAM_TASK result=",life_errors?1U:0U);field(" frames=",frames);field(" waits=",waits);field(" timeouts=",0);field(" errors=",life_errors);field(" stopped=",1);field(" requested=",1);field(" active_stop=",life_active_stops==3U);maia_console_puts("\n");
    field("LCD_STREAM_IRQ line=",maia_dw_irq_line());field(" source=",MAIA_DW_IRQ_SOURCE);field(" irqs=",irqs);field(" posts=",posts);field(" spurious=",0);field(" drains=",drains);field(" released=",e->released);field(" done=",e->last.done);field(" recovered=",e->last.recovered);field(" raw=",e->last.count_raw);field(" status=",e->last.channel_status);field(" common=",e->last.common_status);field(" bridge=",e->last.bridge_status);field(" error0=",e->last.observation.error0);field(" error1=",e->last.observation.error1);maia_console_puts("\n");
    field("LCD_STREAM_KERNEL ticks=",(uint32_t)(maia_tick_get()-began));field(" timer=",traps.timer_irqs);field(" external=",traps.external_irqs);field(" ecalls=",traps.ecall_count);field(" beats=",life_beats);field(" work=",life_work);field(" exited=",exited);field(" errors=",life_errors);maia_console_puts("\n");
    if(life_errors||life_draws!=6U||exited!=3U||traps.external_irqs!=irqs||traps.timer_irqs<life_duration*3U||life_beats<life_duration||life_work<10000U)halt("life-result");
    if(life_visual&&!dpi_bl_restore(rd,wr,&life_light))halt("life-light");
    fb_finish(life_hash,&life_before);maia_console_puts("MAIA_NATIVE_LCD_LIFECYCLE_PASS\n");for(;;)__asm__ volatile("wfi");
}
static __attribute__((noreturn)) void lcd_stream_launch(unsigned visual,uint32_t hash,const maia_psram_snapshot_t *before,const dpi_backlight_snapshot_t *light){
    life_visual=visual;life_hash=hash;life_before=*before;life_light=*light;life_duration=visual?4000U:500U;
    life_map=rd(MAIA_ESP32P4_INTR_CORE0_MAP_REG(24));
    maia_status_t result=maia_sched_init();field("LCD_STREAM_INIT part=",0);field(" result=",(uint32_t)result);maia_console_puts("\n");if(result!=MAIA_OK)halt("life-sched");
    const maia_task_entry_fn entries[4]={life_worker,life_controller,life_beat,life_load};
    const char *names[4]={"lcd","lcd-control","heartbeat","cpu-load"};
    for(unsigned i=0;i<4;++i){void *stack=i<2U?(void *)life_stacks[i]:(void *)life_small[i-2U];unsigned bytes=i<2U?sizeof life_stacks[0]:sizeof life_small[0];
        result=maia_task_create(names[i],entries[i],0,(uint8_t)(4U-i),stack,bytes,&life_ids[i]);field("LCD_STREAM_INIT part=",i+1U);field(" result=",(uint32_t)result);maia_console_puts("\n");if(result!=MAIA_OK)halt("life-task");}
    result=maia_lcd_output_init(life_ids[0]);life_ctrl=maia_dw_irq_saved_ctrl();maia_lcd_async_report_t prepared;maia_lcd_async_report(&prepared);
    field("LCD_STREAM_INIT part=",5);field(" result=",(uint32_t)result);maia_console_puts("\n");field("LCD_STREAM_PREP engine=",prepared.init_stage);field(" irq=",maia_dw_irq_stage());field(" map=",life_map);field(" ctrl=",life_ctrl);maia_console_puts("\n");
    if(result!=MAIA_OK)halt("life-init");
    maia_soc_systimer_init(1000);maia_sched_start();halt("life-return");for(;;)__asm__ volatile("wfi");
}
