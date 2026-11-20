/* Original MaiaRTOS MIT. Native stream engine; the DMA source changes only at
 * frame boundaries. DPI/DSI video remains active while the backbuffer is drawn. */
#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_psram.h>
enum { ASYNC_FREE,ASYNC_READY,ASYNC_ACTIVE,ASYNC_ERROR,ASYNC_STOPPED,ASYNC_PAUSED };
static unsigned async_phase,async_pending,async_irq_bad,async_pause_requested,async_video_active;
static unsigned async_editing,async_swap_pending,async_first_frame_pending;
static uint32_t async_source,async_back_source;
static maia_lcd_wake_t async_wake;
static void *async_arg;
static uint32_t async_signal_before;
static maia_lcd_async_report_t async_report;
#ifdef MAIA_DSI_ACCESS_TEST
static void lcd_async_reset_state(void){
    async_phase=ASYNC_FREE;async_pending=async_irq_bad=async_pause_requested=async_video_active=0;
    async_editing=async_swap_pending=async_first_frame_pending=0;async_source=async_back_source=0;async_wake=0;
    for(unsigned i=0;i<sizeof async_report;++i)((volatile unsigned char *)&async_report)[i]=0;
}
#endif
static int async_frame_snapshot(maia_dsi_dma_frame_report_t *r){
    r->observation.error0=r->observation.error1=0;
    int host_ok=transfer_observe(&r->observation);
    r->channel_status=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U);
    r->common_status=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x50U);
    r->bridge_status=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x58U);
    r->count_raw=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x130U)&0x3FFFFFU;
    r->dma_fifo=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x134U)&0x7FFFU;
    r->bridge_fifo=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x14U)&0x3FFFU;
    return host_ok && (r->channel_status&FRAME_ERRORS)==0U && (r->channel_status&2U)!=0U &&
        r->common_status==0U && (r->bridge_status&1U)==0U &&
        r->count_raw==230399U && (r->observation.status&1U)!=0U &&
        (r->observation.error0&0x1FFFFFU)==0U && (r->observation.error1&0x81FFFU)==0U;
}
static void lcd_irq(void *unused){
    (void)unused;++async_report.irqs;
    /* Rearm at the completed-frame boundary unless the owner requested a safe
     * pause for CPU drawing or shutdown. No cache scan, blocking or logging. */
    uint32_t cfg=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U);
    dsi_write(MAIA_ESP32P4_GDMA_BASE+0x10U,cfg&~2U);
    maia_dsi_dma_frame_report_t *r=&async_report.last;
    uint32_t channels=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U);
    int irq_valid=(cfg==3U && async_phase==ASYNC_ACTIVE && (channels&1U)==0U &&
        dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U)==1U);
    int frame_valid=irq_valid?async_frame_snapshot(r):0;
    if(!frame_valid){
        if(!irq_valid||!(r->channel_status&2U))++async_report.spurious;
        async_irq_bad=1;async_phase=ASYNC_ERROR;
    }else{
        r->done=1;r->items=230400U;r->completed=8U;++async_report.frames;
        if(async_pause_requested){
            dsi_write(MAIA_ESP32P4_GDMA_BASE+0x10U,0U);
            if(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U)!=0U){
                async_irq_bad=1;async_phase=ASYNC_ERROR;
            }else async_phase=ASYNC_PAUSED;
        }else{
            dsi_write(MAIA_ESP32P4_GDMA_BASE+0x198U,0x4000001BU);
            if(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U)!=0U){
                async_irq_bad=1;async_phase=ASYNC_ERROR;
            }else{
                if(async_swap_pending){
                    uint32_t source=async_source;async_source=async_back_source;async_back_source=source;
                    async_swap_pending=0;async_first_frame_pending=1;
                }else if(async_first_frame_pending){
                    async_first_frame_pending=0;++async_report.presented;
                }
                dsi_write(dma_stage_address(0),async_source);
                if(dsi_read(dma_stage_address(0))!=async_source){
                    async_irq_bad=1;async_phase=ASYNC_ERROR;
                }else{
                    dsi_write(MAIA_ESP32P4_GDMA_BASE+0x10U,3U);
                    if(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U)!=3U){
                        async_irq_bad=1;async_phase=ASYNC_ERROR;
                    }else{
                        dsi_write(MAIA_ESP32P4_GDMA_BASE+0x18U,0x101U);
                        if(!(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U)){
                            async_irq_bad=1;async_phase=ASYNC_ERROR;
                        }
                    }
                }
            }
        }
    }
    async_pending=1;
    if(async_wake){++async_report.notifications;async_wake(async_arg);}
}
maia_dsi_access_result_t maia_lcd_async_prepare(maia_lcd_wake_t fn,void *arg){
    if(!fn)return MAIA_DSI_ACCESS_INVALID;
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase==ASYNC_ERROR)return MAIA_DSI_ACCESS_FAILED;
    unsigned reuse=async_phase==ASYNC_STOPPED;
    if((async_phase!=ASYNC_FREE&&!reuse)||framebuffer_claim||dpi_dma_state!=(reuse?OWNED:UNCLAIMED)||dpi_bridge_state!=OWNED)return MAIA_DSI_ACCESS_BUSY;
    for(unsigned i=0;i<sizeof async_report;++i)((volatile unsigned char *)&async_report)[i]=0;
    async_phase=ASYNC_ERROR;
    async_report.init_stage=1;
    uint32_t source=maia_psram_window_dma_acquire();
    if(source!=DSI_FB_SOURCE||source+2U*DSI_FB_BYTES>MAIA_PSRAM_WINDOW_VADDR+MAIA_PSRAM_WINDOW_BYTES)return MAIA_DSI_ACCESS_BUSY;
    framebuffer_claim=1;lcd_stream_mode=1;
    async_video_active=async_pause_requested=async_pending=async_irq_bad=0;
    async_source=source;async_back_source=source+DSI_FB_BYTES;
    async_editing=async_swap_pending=async_first_frame_pending=0;
    async_report.init_stage=2;
    maia_dsi_dma_stage_report_t stage;
    if(!reuse){maia_dsi_access_result_t result=dma_stage_body(source,0,&stage);if(result!=MAIA_DSI_ACCESS_OK)return result;}
    else if(!frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0)||!dma_quiet())return MAIA_DSI_ACCESS_VERIFY;
    async_report.init_stage=3;async_signal_before=dma_staged_words[11];
    uint32_t signal=(async_signal_before&~dma_stage_masks[11])|(FRAME_ERRORS|2U);
    dsi_write(dma_stage_address(11),signal);
    if(!frame_roots()||dsi_read(dma_stage_address(11))!=signal)return MAIA_DSI_ACCESS_VERIFY;
    dma_staged_words[11]=signal;
    async_wake=fn;async_arg=arg;
    async_report.init_stage=4;if(!maia_dw_irq_claim(lcd_irq,0))return MAIA_DSI_ACCESS_VERIFY;
    async_report.init_stage=5;async_phase=ASYNC_READY;return MAIA_DSI_ACCESS_OK;
}
uint16_t *maia_lcd_async_edit_begin(void){
    if(!dsi_context()||(async_phase!=ASYNC_READY&&async_phase!=ASYNC_ACTIVE)||
       async_editing||async_swap_pending||async_first_frame_pending||async_pause_requested)return 0;
    if(async_phase==ASYNC_READY){
        if(async_video_active||!frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0)||!dma_quiet())return 0;
    }else if(!async_video_active||async_source==async_back_source)return 0;
    uint32_t source=async_back_source;
    if(maia_psram_window_edit_begin(source,DSI_FB_BYTES)!=source)return 0;
    async_editing=1;return (uint16_t *)(uintptr_t)source;
}
maia_dsi_access_result_t maia_lcd_async_edit_publish(void){
    uintptr_t irq=maia_arch_irq_save();
    if(!async_editing||(async_phase!=ASYNC_READY&&async_phase!=ASYNC_ACTIVE)){
        maia_arch_irq_restore(irq);return MAIA_DSI_ACCESS_BUSY;
    }
    uint32_t source=async_back_source;
    maia_arch_irq_restore(irq);
    if(!maia_psram_window_edit_publish(source,DSI_FB_BYTES)){
        irq=maia_arch_irq_save();async_editing=0;async_irq_bad=1;async_phase=ASYNC_ERROR;maia_arch_irq_restore(irq);
        return MAIA_DSI_ACCESS_VERIFY;
    }
    irq=maia_arch_irq_save();
    if(async_phase!=ASYNC_READY&&async_phase!=ASYNC_ACTIVE){
        async_editing=0;async_irq_bad=1;async_phase=ASYNC_ERROR;maia_arch_irq_restore(irq);
        return MAIA_DSI_ACCESS_FAILED;
    }
    ++async_report.updates;
    if(async_video_active)async_swap_pending=1;
    else{
        uint32_t old_source=async_source;async_source=async_back_source;async_back_source=old_source;
        async_first_frame_pending=1;
    }
    async_editing=0;maia_arch_irq_restore(irq);return MAIA_DSI_ACCESS_OK;
}
maia_dsi_access_result_t maia_lcd_async_start(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase==ASYNC_ERROR)return MAIA_DSI_ACCESS_FAILED;
    if(async_phase!=ASYNC_READY&&async_phase!=ASYNC_PAUSED)return MAIA_DSI_ACCESS_BUSY;
    unsigned resume=async_video_active;
    uint32_t mode=resume?(saved_command_table[0]&~1U):saved_command_table[0];
    uint32_t lp=resume?(dpi_staged_phy[3]|3U):dpi_staged_phy[3];
    uint32_t dpi=resume?(dpi_staged_phy[4]|1U):dpi_staged_phy[4];
    if(!frame_guard(mode,lp,dpi,0)||!dma_quiet())return MAIA_DSI_ACCESS_VERIFY;
    uint32_t pending=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U);
    if(pending&~0x4000001BU)return MAIA_DSI_ACCESS_IO;
    dsi_write(MAIA_ESP32P4_GDMA_BASE+0x198U,0x4000001BU);
    dsi_write(dma_stage_address(0),async_source);
    if(!frame_roots()||dsi_read(dma_stage_address(0))!=async_source||dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U))return MAIA_DSI_ACCESS_VERIFY;
    for(unsigned i=0;i<sizeof async_report.last;++i)((volatile unsigned char *)&async_report.last)[i]=0;
    maia_dsi_dma_frame_report_t *r=&async_report.last;
    async_pending=async_irq_bad=async_pause_requested=0;async_phase=ASYNC_ERROR;dpi_frame_state=FAULTED;transfer_state=OWNED;
    if(resume){
        async_phase=ASYNC_ACTIVE;
        if(!frame_store(r,0,MAIA_ESP32P4_GDMA_BASE+0x10U,3U))goto failed;
        dsi_write(MAIA_ESP32P4_GDMA_BASE+0x18U,0x101U);
        if(!frame_roots()||!(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U))goto failed;
        r->saw_active=1;return MAIA_DSI_ACCESS_OK;
    }
    async_phase=ASYNC_ACTIVE;
    if(!frame_store(r,0,MAIA_ESP32P4_GDMA_BASE+0x10U,3U))goto failed;
    dsi_write(MAIA_ESP32P4_GDMA_BASE+0x18U,0x101U);
    if(!frame_roots()||!(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U))goto failed;
    r->saw_active=1;
    if(!frame_guard(mode,lp,dpi,3U)||!frame_store(r,2,MAIA_ESP32P4_DSI_HOST_BASE+0x34U,mode&~1U))goto failed;
    mode&=~1U;
    if(!frame_guard(mode,lp,dpi,3U)||!frame_store(r,3,MAIA_ESP32P4_DSI_HOST_BASE+0x94U,lp|3U))goto failed;
    lp|=3U;
    async_video_active=1;
    if(!frame_guard(mode,lp,dpi,3U)||!frame_store(r,4,MAIA_ESP32P4_DSI_BRG_BASE+0x40U,dpi|1U))goto failed;
    dsi_write(MAIA_ESP32P4_DSI_BRG_BASE+0x44U,1U);r->completed=7;return MAIA_DSI_ACCESS_OK;
failed:
    async_phase=ASYNC_ERROR;return MAIA_DSI_ACCESS_VERIFY;
}
maia_dsi_access_result_t maia_lcd_async_request_pause(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase==ASYNC_ERROR)return MAIA_DSI_ACCESS_FAILED;
    if(async_phase==ASYNC_ACTIVE){
        if(async_editing||async_swap_pending||async_first_frame_pending)return MAIA_DSI_ACCESS_BUSY;
        async_pause_requested=1;
    }
    return MAIA_DSI_ACCESS_OK;
}
int maia_lcd_async_is_paused(void){return dsi_context()&&async_phase==ASYNC_PAUSED;}
int maia_lcd_async_pending(void){return dsi_context()&&async_pending;}
maia_dsi_access_result_t maia_lcd_async_finish(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase==ASYNC_ERROR)return MAIA_DSI_ACCESS_FAILED;
    if(!async_pending)return MAIA_DSI_ACCESS_BUSY;
    maia_dsi_dma_frame_report_t *r=&async_report.last;
    if(async_irq_bad||!r->done||!r->items||
       (r->channel_status&FRAME_ERRORS)||r->common_status||(r->bridge_status&1U)||
       (r->observation.error0&0x1FFFFFU)||(r->observation.error1&0x81FFFU))goto bad;
    if(async_phase==ASYNC_ACTIVE){
        if(!frame_guard(saved_command_table[0]&~1U,dpi_staged_phy[3]|3U,dpi_staged_phy[4]|1U,3U))goto bad;
        async_pending=0;return MAIA_DSI_ACCESS_OK;
    }
    if(async_phase!=ASYNC_PAUSED||!frame_guard(saved_command_table[0]&~1U,dpi_staged_phy[3]|3U,dpi_staged_phy[4]|1U,0U))goto bad;
    for(unsigned i=0;i<32U;++i){
        if(!frame_roots())goto bad;
        r->dma_fifo=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x134U)&0x7FFFU;
        r->bridge_fifo=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x14U)&0x3FFFU;
        if(dma_quiet()&&!r->bridge_fifo){async_pending=0;return MAIA_DSI_ACCESS_OK;}
        ++async_report.drain_retries;
    }
    return MAIA_DSI_ACCESS_BUSY;
bad:
    async_phase=ASYNC_ERROR;return MAIA_DSI_ACCESS_VERIFY;
}
maia_dsi_access_result_t maia_lcd_async_cancel(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase!=ASYNC_ACTIVE&&async_phase!=ASYNC_PAUSED&&async_phase!=ASYNC_ERROR)return MAIA_DSI_ACCESS_BUSY;
    async_phase=ASYNC_ERROR;
    maia_dsi_dma_frame_report_t *r=&async_report.last;r->transfer_result=MAIA_DSI_ACCESS_TIMEOUT;
    maia_dsi_access_result_t result=frame_cleanup(32,r);r->recovery_result=(int32_t)result;
    if(result==MAIA_DSI_ACCESS_OK){async_video_active=0;dpi_frame_state=3U;transfer_state=UNCLAIMED;}
    return result;
}
maia_dsi_access_result_t maia_lcd_async_stop(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase==ASYNC_PAUSED){
        maia_dsi_dma_frame_report_t *r=&async_report.last;
        maia_dsi_access_result_t stopped=frame_cleanup(32,r);r->recovery_result=(int32_t)stopped;
        if(stopped!=MAIA_DSI_ACCESS_OK){async_phase=ASYNC_ERROR;return stopped;}
        async_video_active=0;dpi_frame_state=3U;transfer_state=UNCLAIMED;async_phase=ASYNC_READY;
    }
    if(async_phase!=ASYNC_READY&&async_phase!=ASYNC_ERROR)return MAIA_DSI_ACCESS_BUSY;
    if(!dma_quiet()||!frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0))return MAIA_DSI_ACCESS_VERIFY;
    dsi_write(dma_stage_address(11),async_signal_before);
    if(dsi_read(dma_stage_address(11))!=async_signal_before)return MAIA_DSI_ACCESS_VERIFY;
    dma_staged_words[11]=async_signal_before;
    if(!maia_dw_irq_release()||!maia_psram_window_dma_release())return MAIA_DSI_ACCESS_VERIFY;
    async_report.released=1;framebuffer_claim=0;lcd_stream_mode=0;async_phase=ASYNC_STOPPED;return MAIA_DSI_ACCESS_OK;
}
void maia_lcd_async_report(maia_lcd_async_report_t *r){if(r&&dsi_context())for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=((const unsigned char *)&async_report)[i];}
