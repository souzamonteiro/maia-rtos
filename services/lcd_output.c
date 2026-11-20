/* Original MaiaRTOS MIT. One native worker, coalescing notifications represent
 * the SoC completion predicate, never a frame counter. No foreign runtime. */
#include <maia/services/lcd_output.h>
#include <maia/arch/irq.h>
#include "../soc/espressif/esp32p4/lcd_async.h"
static maia_lcd_output_stats_t stats;
static unsigned initialized,running;
static unsigned editing;
static maia_lcd_draw_fn pending_draw;
static void *pending_arg;
static void wake(void *arg){(void)arg;(void)maia_task_notify(stats.owner);}
static maia_dsi_access_result_t wait_frame(void){
    uint64_t began=maia_tick_get();
    for(;;){
        if(maia_tick_get()-began>=100U)return MAIA_DSI_ACCESS_TIMEOUT;
        uintptr_t irq=maia_arch_irq_save();
        int pending=maia_lcd_async_pending();
        maia_dsi_access_result_t result=pending?maia_lcd_async_finish():MAIA_DSI_ACCESS_BUSY;
        if(result==MAIA_DSI_ACCESS_OK){
            maia_lcd_async_report_t report;
            maia_lcd_async_report(&report);
            stats.frames=report.frames;
            stats.presented=report.presented;
        }
        maia_arch_irq_restore(irq);
        if(result==MAIA_DSI_ACCESS_OK)return result;
        if(result!=MAIA_DSI_ACCESS_BUSY)return result;
        if(pending)maia_task_delay_ticks(1);
        else{
            uint64_t elapsed=maia_tick_get()-began;
            if(elapsed>=100U)continue;
            irq=maia_arch_irq_save();++stats.waits;maia_arch_irq_restore(irq);
            maia_status_t waited=maia_task_wait_notification(100U-(uint32_t)elapsed);
            if(waited!=MAIA_OK&&waited!=MAIA_ERR_TIMEOUT)return MAIA_DSI_ACCESS_IO;
        }
    }
}
maia_status_t maia_lcd_output_request_update(maia_lcd_draw_fn draw,void *arg){
    if(!draw)return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();maia_status_t result=MAIA_OK;
    if(!initialized||stats.stopped||stats.stop_requested)result=MAIA_ERR_INVALID;
    else if(pending_draw||editing||stats.presented<stats.updates)result=MAIA_ERR_BUSY;
    else {pending_arg=arg;pending_draw=draw;(void)maia_task_notify(stats.owner);}
    maia_arch_irq_restore(irq);return result;
}
maia_status_t maia_lcd_output_restart(void){
    uintptr_t irq=maia_arch_irq_save();maia_status_t result=MAIA_OK;
    if(!initialized||!irq||maia_task_current_id()!=stats.owner)result=MAIA_ERR_PERM;
    else if(running||!stats.stopped||stats.errors)result=MAIA_ERR_BUSY;
    else if(maia_lcd_async_prepare(wake,0)!=MAIA_DSI_ACCESS_OK){++stats.errors;result=MAIA_ERR_BUSY;}
    else {
        maia_task_id_t owner=stats.owner;
        for(unsigned i=0;i<sizeof stats;++i)((volatile unsigned char *)&stats)[i]=0;
        stats.owner=owner;
    }
    maia_arch_irq_restore(irq);return result;
}
maia_status_t maia_lcd_output_init(maia_task_id_t owner){
    uintptr_t irq=maia_arch_irq_save();maia_status_t result=MAIA_OK;maia_task_info_t info;
    if(initialized)result=MAIA_ERR_BUSY;
    else if(irq||maia_task_current_id()>=0)result=MAIA_ERR_PERM;
    else if(owner<0||maia_task_get_info((uint32_t)owner,&info)!=MAIA_OK||info.state!=MAIA_TASK_READY)result=MAIA_ERR_INVALID;
    else {stats.owner=owner;if(maia_lcd_async_prepare(wake,0)!=MAIA_DSI_ACCESS_OK)result=MAIA_ERR_BUSY;else initialized=1;}
    maia_arch_irq_restore(irq);return result;
}
maia_status_t maia_lcd_output_request_stop(void){
    uintptr_t irq=maia_arch_irq_save();maia_status_t result=initialized&&!stats.stopped?MAIA_OK:MAIA_ERR_INVALID;
    if(result==MAIA_OK){stats.stop_requested=1;(void)maia_task_notify(stats.owner);}
    maia_arch_irq_restore(irq);return result;
}
maia_status_t maia_lcd_output_stats(maia_lcd_output_stats_t *out){
    if(!out)return MAIA_ERR_INVALID;
    uintptr_t irq=maia_arch_irq_save();*out=stats;maia_arch_irq_restore(irq);return initialized?MAIA_OK:MAIA_ERR_INVALID;
}
maia_status_t maia_lcd_output_run(void){
    uintptr_t irq=maia_arch_irq_save();
    if(!initialized||running||stats.stopped||!irq||maia_task_current_id()!=stats.owner){maia_arch_irq_restore(irq);return MAIA_ERR_PERM;}
    running=1;maia_arch_irq_restore(irq);unsigned failed=0,started=0;
    for(;;){
        irq=maia_arch_irq_save();unsigned stop=stats.stop_requested;
        if(stop){pending_draw=0;pending_arg=0;}
        unsigned paused=started&&maia_lcd_async_is_paused();
        maia_lcd_draw_fn draw=stop?0:pending_draw;
        maia_arch_irq_restore(irq);
        if(draw){
            irq=maia_arch_irq_save();
            draw=pending_draw;void *arg=pending_arg;pending_draw=0;pending_arg=0;editing=1;
            uint16_t *pixels=draw?maia_lcd_async_edit_begin():0;
            maia_arch_irq_restore(irq);
            if(!draw)continue;
            if(!pixels){failed=1;break;}
            draw(pixels,720U,1280U,arg);
            maia_dsi_access_result_t published=maia_lcd_async_edit_publish();
            irq=maia_arch_irq_save();editing=0;
            if(published==MAIA_DSI_ACCESS_OK)++stats.updates;
            maia_arch_irq_restore(irq);if(published!=MAIA_DSI_ACCESS_OK){failed=1;break;}
            continue;
        }
        if(stop){
            irq=maia_arch_irq_save();unsigned needs_present=stats.presented<stats.updates;maia_arch_irq_restore(irq);
            if(needs_present){
                if(!started||paused){
                    irq=maia_arch_irq_save();maia_dsi_access_result_t result=maia_lcd_async_start();maia_arch_irq_restore(irq);
                    if(result!=MAIA_DSI_ACCESS_OK){failed=1;break;}
                    started=1;
                }
                maia_dsi_access_result_t result=wait_frame();
                if(result==MAIA_DSI_ACCESS_TIMEOUT){irq=maia_arch_irq_save();++stats.timeouts;maia_arch_irq_restore(irq);}
                if(result!=MAIA_DSI_ACCESS_OK){failed=1;break;}
                continue;
            }
            if(started&&!paused){
                irq=maia_arch_irq_save();maia_dsi_access_result_t requested=maia_lcd_async_request_pause();maia_arch_irq_restore(irq);
                if(requested!=MAIA_DSI_ACCESS_OK){failed=1;break;}
                maia_dsi_access_result_t result=wait_frame();
                if(result==MAIA_DSI_ACCESS_TIMEOUT){irq=maia_arch_irq_save();++stats.timeouts;maia_arch_irq_restore(irq);}
                if(result!=MAIA_DSI_ACCESS_OK){failed=1;break;}
                continue;
            }
            break;
        }
        if(!started||paused){
            irq=maia_arch_irq_save();maia_dsi_access_result_t result=maia_lcd_async_start();maia_arch_irq_restore(irq);
            if(result!=MAIA_DSI_ACCESS_OK){failed=1;break;}
            started=1;
        }
        maia_dsi_access_result_t result=wait_frame();
        if(result==MAIA_DSI_ACCESS_TIMEOUT){irq=maia_arch_irq_save();++stats.timeouts;maia_arch_irq_restore(irq);}
        if(result!=MAIA_DSI_ACCESS_OK){failed=1;break;}
    }
    irq=maia_arch_irq_save();
    if(failed){++stats.errors;(void)maia_lcd_async_cancel();}
    maia_dsi_access_result_t stopped=maia_lcd_async_stop();stats.stopped=stopped==MAIA_DSI_ACCESS_OK;
    if(!stats.stopped)++stats.errors;
    pending_draw=0;pending_arg=0;editing=0;running=0;
    maia_arch_irq_restore(irq);return failed||!stats.stopped?MAIA_ERR_BUSY:MAIA_OK;
}
#ifdef MAIA_LCD_OUTPUT_TEST
void maia_lcd_output_test_reset(void);
void maia_lcd_output_test_reset(void){initialized=running=editing=0;pending_draw=0;pending_arg=0;for(unsigned i=0;i<sizeof stats;++i)((unsigned char *)&stats)[i]=0;}
#endif
