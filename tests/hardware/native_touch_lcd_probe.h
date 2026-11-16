/* Original MaiaRTOS MIT. RAM-only native touch/LCD integration; all callbacks,
 * tasks and buffers stationary, CPU1 parked, one exclusive PSRAM client. */
#include <maia/services/lcd_output.h>
#include <maia/services/i2c_bus.h>
#include <maia/services/touch_input.h>
#include <maia/arch/irq.h>
#include "../../soc/espressif/esp32p4/lcd_async.h"
#include "../../soc/espressif/esp32p4/dw_dma_irq.h"
static uint8_t ti_big[2][8192] __attribute__((aligned(16)));
static uint8_t ti_touch_stack[4096] __attribute__((aligned(16)));
static uint8_t ti_small[2][2048] __attribute__((aligned(16)));
static maia_task_id_t ti_ids[5];
static maia_i2c_bus_t ti_bus;
static maia_st712x_info_t ti_info;
static volatile unsigned ti_finish_input,ti_input_exited,ti_lcd_exited,ti_done,ti_beats,ti_work,ti_errors,ti_mask,ti_draws;
static uint32_t ti_visual,ti_hash,ti_map,ti_ctrl;
static maia_status_t ti_lcd_result;
static maia_psram_snapshot_t ti_before;
static dpi_backlight_snapshot_t ti_light;
static maia_touch_frame_t ti_old;
static struct {uint32_t tick,x,y,count;} ti_events[32];
static unsigned ti_event_count;
extern void maia_soc_systimer_init(uint32_t);
static maia_i2c_result_t ti_read_boot(void *ctx,uint16_t reg,uint8_t *p,uint32_t n){(void)ctx;return maia_i2c_read_register16_block(&reset_bus.lines,0x55,reg,p,n);}
static maia_i2c_result_t ti_read_task(void *ctx,uint16_t reg,uint8_t *p,uint32_t n){return maia_i2c_bus_read16(ctx,0x55,reg,p,n);}
static void ti_marker(uint16_t *p,unsigned x,unsigned y,unsigned erase){
    /* Logical90 -> native; clipped white cross outside typical fingertip. */
    unsigned nx=y,ny=1279U-x;
    unsigned x0=nx>35U?nx-35U:0U,y0=ny>35U?ny-35U:0U;
    unsigned x1=nx+36U<720U?nx+36U:720U,y1=ny+36U<1280U?ny+36U:1280U;
    for(unsigned v=y0;v<y1;++v)for(unsigned u=x0;u<x1;++u){
        unsigned dx=u>nx?u-nx:nx-u,dy=v>ny?v-ny:ny-v;
        if(erase)p[v*720U+u]=fb_native_pixel(u,v);
        else if(dx<3U||dy<3U)p[v*720U+u]=0xFFFFU;
        else if(dx<5U||dy<5U)p[v*720U+u]=0;
    }
}
static void ti_draw(uint16_t *p,uint32_t w,uint32_t h,void *arg){
    (void)arg;if(w!=720U||h!=1280U){++ti_errors;return;}
    for(unsigned i=0;i<ti_old.count;++i)ti_marker(p,ti_old.points[i].x,ti_old.points[i].y,1);
    ti_old.count=0;maia_touch_snapshot_t s;
    if(!ti_finish_input&&maia_touch_input_snapshot(&s)==MAIA_OK){
        ti_old=s.frame;
        for(unsigned i=0;i<s.frame.count;++i)ti_marker(p,s.frame.points[i].x,s.frame.points[i].y,0);
    }
    ++ti_draws;
}
static void ti_lcd(void *arg){(void)arg;ti_lcd_result=maia_lcd_output_run();ti_lcd_exited=1;}
static void ti_touch(void *arg){
    (void)arg;unsigned previous=0;
    while(!ti_finish_input){
        (void)maia_touch_input_poll();maia_touch_snapshot_t s;
        maia_status_t result=maia_touch_input_snapshot(&s);
        if(s.sequence!=previous){previous=s.sequence;
            if(result==MAIA_OK&&s.frame.count){unsigned x=s.frame.points[0].x,y=s.frame.points[0].y;
                if(x<200U&&y<180U)ti_mask|=1U;
                if(x>=1080U&&y<180U)ti_mask|=2U;
                if(x<200U&&y>=540U)ti_mask|=4U;
                if(x>=1080U&&y>=540U)ti_mask|=8U;
            }
            if(ti_event_count<32U){unsigned n=ti_event_count++;ti_events[n].tick=(uint32_t)maia_tick_get();ti_events[n].count=result==MAIA_OK?s.frame.count:0xFFFFFFFFU;
                ti_events[n].x=s.frame.count?s.frame.points[0].x:0;ti_events[n].y=s.frame.count?s.frame.points[0].y:0;}
            maia_status_t queued=maia_lcd_output_request_update(ti_draw,0);if(queued!=MAIA_OK&&queued!=MAIA_ERR_BUSY)++ti_errors;
        }
        maia_task_delay_ticks(5);
    }
    ti_input_exited=1;
}
static void ti_beat(void *arg){(void)arg;while(!ti_done){++ti_beats;maia_task_delay_ticks(1);}}
static void ti_load(void *arg){(void)arg;while(!ti_done){++ti_work;__asm__ volatile("" ::: "memory");}}
static void ti_controller(void *arg){
    (void)arg;uint64_t began=maia_tick_get();unsigned duration=ti_visual?20000U:1500U;
    maia_task_delay_ticks(duration);ti_finish_input=1;
    for(unsigned i=0;i<200U&&!ti_input_exited;++i)maia_task_delay_ticks(1);
    if(!ti_input_exited)halt("touch-stop");
    maia_lcd_output_stats_t stats;maia_touch_snapshot_t touch;
    (void)maia_lcd_output_stats(&stats);unsigned target=stats.updates+1U;
    for(unsigned i=0;i<200U;++i){maia_status_t r=maia_lcd_output_request_update(ti_draw,0);if(r==MAIA_OK)break;if(r!=MAIA_ERR_BUSY)halt("touch-cleanup-queue");maia_task_delay_ticks(1);if(i==199U)halt("touch-cleanup-busy");}
    for(unsigned i=0;i<200U;++i){(void)maia_lcd_output_stats(&stats);if(stats.presented>=target&&!ti_old.count)break;maia_task_delay_ticks(1);if(i==199U)halt("touch-cleanup-present");}
    unsigned active=0;
    for(unsigned i=0;i<20U;++i){uintptr_t irq=maia_arch_irq_save();active=rd(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U;
        if(active&&maia_lcd_output_request_stop()!=MAIA_OK)++ti_errors;
        maia_arch_irq_restore(irq);if(active)break;maia_task_delay_ticks(1);}
    if(!active)halt("touch-lcd-active-stop");
    for(unsigned i=0;i<200U&&!ti_lcd_exited;++i)maia_task_delay_ticks(1);
    if(!ti_lcd_exited)halt("touch-lcd-exit");
    ti_done=1;maia_task_delay_ticks(3);(void)maia_arch_irq_save();
    maia_lcd_async_report_t engine;maia_trap_stats_t traps;
    (void)maia_lcd_output_stats(&stats);maia_lcd_async_report(&engine);maia_trap_get_stats(&traps);(void)maia_touch_input_snapshot(&touch);
    unsigned exited=0;for(unsigned i=0;i<5U;++i)if(i!=2U){maia_task_info_t info;if(maia_task_get_info((uint32_t)ti_ids[i],&info)==MAIA_OK&&info.state==MAIA_TASK_EXITED)++exited;}
    field("TOUCH_INPUT polls=",touch.polls);field(" reports=",touch.reports);field(" errors=",touch.errors);field(" downs=",touch.downs);field(" moves=",touch.moves);field(" ups=",touch.ups);field(" corners=",ti_mask);field(" draws=",ti_draws);field(" updates=",stats.updates);field(" presented=",stats.presented);field(" bus_calls=",ti_bus.transactions);field(" bus_errors=",ti_bus.errors);field(" contentions=",ti_bus.contentions);maia_console_puts("\n");
    for(unsigned i=0;i<ti_event_count;++i){field("TOUCH_EVENT index=",i);field(" tick=",ti_events[i].tick);field(" count=",ti_events[i].count);field(" x=",ti_events[i].x);field(" y=",ti_events[i].y);maia_console_puts("\n");}
    field("LCD_STREAM_ROUTE map_before=",ti_map);field(" map_after=",rd(MAIA_ESP32P4_INTR_CORE0_MAP_REG(24)));field(" ctrl_before=",ti_ctrl);field(" ctrl_after=",rd(MAIA_ESP32P4_CLIC_INT_CTRL_REG(maia_dw_irq_line())));maia_console_puts("\n");
    field("LCD_STREAM_TASK result=",(uint32_t)ti_lcd_result);field(" frames=",stats.frames);field(" waits=",stats.waits);field(" timeouts=",stats.timeouts);field(" errors=",stats.errors);field(" stopped=",stats.stopped);field(" requested=",stats.stop_requested);field(" active_stop=",active);maia_console_puts("\n");
    field("LCD_STREAM_IRQ line=",maia_dw_irq_line());field(" source=",MAIA_DW_IRQ_SOURCE);field(" irqs=",engine.irqs);field(" posts=",engine.notifications);field(" spurious=",engine.spurious);field(" drains=",engine.drain_retries);field(" released=",engine.released);field(" done=",engine.last.done);field(" recovered=",engine.last.recovered);field(" raw=",engine.last.count_raw);field(" status=",engine.last.channel_status);field(" common=",engine.last.common_status);field(" bridge=",engine.last.bridge_status);field(" error0=",engine.last.observation.error0);field(" error1=",engine.last.observation.error1);maia_console_puts("\n");
    field("LCD_STREAM_KERNEL ticks=",(uint32_t)(maia_tick_get()-began));field(" timer=",traps.timer_irqs);field(" external=",traps.external_irqs);field(" ecalls=",traps.ecall_count);field(" beats=",ti_beats);field(" work=",ti_work);field(" exited=",exited);field(" errors=",ti_errors);maia_console_puts("\n");
    unsigned gesture=!ti_visual||(ti_mask==15U&&touch.downs>=4U&&touch.ups>=4U&&touch.moves>=3U&&stats.presented>=6U);
    unsigned okay=gesture&&!ti_errors&&!touch.errors&&!ti_bus.errors&&!ti_bus.contentions&&touch.polls>=10U&&exited==4U&&ti_lcd_result==MAIA_OK&&stats.stopped&&!stats.errors&&!stats.timeouts&&stats.frames>=40U&&engine.irqs==stats.frames&&traps.external_irqs==engine.irqs&&!engine.spurious&&engine.released;
    if(ti_visual&&!dpi_bl_restore(rd,wr,&ti_light))halt("touch-light-restore");
    fb_finish(ti_hash,&ti_before);
    maia_console_puts(okay?"MAIA_NATIVE_TOUCH_LCD_PASS\n":"MAIA_NATIVE_TOUCH_LCD_INCOMPLETE\n");for(;;)__asm__ volatile("wfi");
}
static __attribute__((noreturn)) void lcd_stream_launch(unsigned visual,uint32_t hash,const maia_psram_snapshot_t *before,const dpi_backlight_snapshot_t *light){
    ti_visual=visual;ti_hash=hash;ti_before=*before;ti_light=*light;ti_map=rd(MAIA_ESP32P4_INTR_CORE0_MAP_REG(24));
    maia_i2c_result_t identified=maia_st712x_identify(ti_read_boot,0,&ti_info);
    field("TOUCH_INFO result=",(uint32_t)identified);field(" firmware=",ti_info.firmware);field(" max_x=",ti_info.max_x);field(" max_y=",ti_info.max_y);field(" slots=",ti_info.slots);maia_console_puts("\n");if(identified!=MAIA_I2C_OK)halt("touch-info");
    maia_status_t r=maia_sched_init();field("LCD_STREAM_INIT part=",0);field(" result=",(uint32_t)r);maia_console_puts("\n");if(r!=MAIA_OK)halt("touch-sched-init");
    const maia_task_entry_fn entries[5]={ti_lcd,ti_touch,ti_controller,ti_beat,ti_load};
    const char *names[5]={"lcd","touch","controller","heartbeat","cpu-load"};
    const uint8_t priorities[5]={5,2,3,4,1};
    for(unsigned i=0;i<5U;++i){void *stack=i==0U?(void *)ti_big[0]:i==2U?(void *)ti_big[1]:i==1U?(void *)ti_touch_stack:(void *)ti_small[i-3U];
        unsigned bytes=i==0U||i==2U?sizeof ti_big[0]:i==1U?sizeof ti_touch_stack:sizeof ti_small[0];
        r=maia_task_create(names[i],entries[i],0,priorities[i],stack,bytes,&ti_ids[i]);field(i<4U?"LCD_STREAM_INIT part=":"TOUCH_INIT part=",i<4U?i+1U:0U);field(" result=",(uint32_t)r);maia_console_puts("\n");if(r!=MAIA_OK)halt("touch-task-create");}
    r=maia_i2c_bus_init(&ti_bus,&reset_bus.lines);field("TOUCH_INIT part=",1);field(" result=",(uint32_t)r);maia_console_puts("\n");if(r!=MAIA_OK)halt("touch-bus-init");
    r=maia_touch_input_init(ti_ids[1],ti_read_task,&ti_bus,&ti_info,720,1280,90);field("TOUCH_INIT part=",2);field(" result=",(uint32_t)r);maia_console_puts("\n");if(r!=MAIA_OK)halt("touch-service-init");
    r=maia_lcd_output_init(ti_ids[0]);ti_ctrl=maia_dw_irq_saved_ctrl();maia_lcd_async_report_t prepared;maia_lcd_async_report(&prepared);
    field("LCD_STREAM_INIT part=",5);field(" result=",(uint32_t)r);maia_console_puts("\n");field("LCD_STREAM_PREP engine=",prepared.init_stage);field(" irq=",maia_dw_irq_stage());field(" map=",ti_map);field(" ctrl=",ti_ctrl);maia_console_puts("\n");if(r!=MAIA_OK)halt("touch-lcd-init");
    maia_soc_systimer_init(1000);maia_sched_start();halt("touch-return");for(;;)__asm__ volatile("wfi");
}
