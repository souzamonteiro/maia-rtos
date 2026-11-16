/* Original MaiaRTOS MIT. One native worker, coalescing notifications represent
 * the SoC completion predicate, never a frame counter. No foreign runtime. */
#include <maia/services/lcd_output.h>
#include <maia/arch/irq.h>
#include "../soc/espressif/esp32p4/lcd_async.h"
static maia_lcd_output_stats_t stats;
static unsigned initialized,running;
static void wake(void *arg){(void)arg;(void)maia_task_notify(stats.owner);}
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
    if(!initialized||running||!irq||maia_task_current_id()!=stats.owner){maia_arch_irq_restore(irq);return MAIA_ERR_PERM;}
    running=1;maia_arch_irq_restore(irq);unsigned failed=0;
    for(;;){
        irq=maia_arch_irq_save();unsigned stop=stats.stop_requested;maia_arch_irq_restore(irq);if(stop)break;
        irq=maia_arch_irq_save();maia_dsi_access_result_t result=maia_lcd_async_start();maia_arch_irq_restore(irq);
        if(result!=MAIA_DSI_ACCESS_OK){failed=1;break;}
        uint64_t began=maia_tick_get();
        for(;;){
            if(maia_tick_get()-began>=100U){++stats.timeouts;failed=1;break;}
            irq=maia_arch_irq_save();int pending=maia_lcd_async_pending();
            result=pending?maia_lcd_async_finish():MAIA_DSI_ACCESS_BUSY;maia_arch_irq_restore(irq);
            if(result==MAIA_DSI_ACCESS_OK){++stats.frames;break;}
            if(result!=MAIA_DSI_ACCESS_BUSY){failed=1;break;}
            if(pending)maia_task_delay_ticks(1);
            else {uint64_t elapsed=maia_tick_get()-began;if(elapsed>=100U)continue;
                ++stats.waits;uint32_t left=100U-(uint32_t)elapsed;
                maia_status_t waited=maia_task_wait_notification(left);
                if(waited!=MAIA_OK&&waited!=MAIA_ERR_TIMEOUT){failed=1;break;}}
        }
        if(failed)break;
    }
    irq=maia_arch_irq_save();
    if(failed){++stats.errors;(void)maia_lcd_async_cancel();}
    maia_dsi_access_result_t stopped=maia_lcd_async_stop();stats.stopped=stopped==MAIA_DSI_ACCESS_OK;
    if(!stats.stopped)++stats.errors;
    maia_arch_irq_restore(irq);return failed||!stats.stopped?MAIA_ERR_BUSY:MAIA_OK;
}
#ifdef MAIA_LCD_OUTPUT_TEST
void maia_lcd_output_test_reset(void);
void maia_lcd_output_test_reset(void){initialized=running=0;for(unsigned i=0;i<sizeof stats;++i)((unsigned char *)&stats)[i]=0;}
#endif
