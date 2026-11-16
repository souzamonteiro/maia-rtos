/* Original MaiaRTOS MIT. Split-phase native engine using qualified frame
 * guards/start/stop protocol, actual DW-GDMA IRQ wake instead of polling frame. */
enum { ASYNC_FREE,ASYNC_READY,ASYNC_ACTIVE,ASYNC_ERROR,ASYNC_STOPPED };
static unsigned async_phase,async_pending,async_irq_bad;
static maia_lcd_wake_t async_wake;
static void *async_arg;
static uint32_t async_signal_before;
static maia_lcd_async_report_t async_report;
#ifdef MAIA_DSI_ACCESS_TEST
static void lcd_async_reset_state(void){
    async_phase=ASYNC_FREE;async_pending=async_irq_bad=0;async_wake=0;
    for(unsigned i=0;i<sizeof async_report;++i)((volatile unsigned char *)&async_report)[i]=0;
}
#endif
static void lcd_irq(void *unused){
    (void)unused;++async_report.irqs;
    /* Mask the global IRQ before notifying; preserve status until owner checks
     * actual DMA-done/count/drain. No cache/PSRAM scan, blocking or logging here. */
    uint32_t cfg=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U);
    dsi_write(MAIA_ESP32P4_GDMA_BASE+0x10U,cfg&~2U);
    if((cfg&~3U)||!(cfg&2U)||async_phase!=ASYNC_ACTIVE||
       !(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U)&(FRAME_ERRORS|2U))||
       dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U)!=(cfg&~2U)){
        ++async_report.spurious;async_irq_bad=1;
    }
    async_pending=1;
    if(async_wake){++async_report.notifications;async_wake(async_arg);}
}
maia_dsi_access_result_t maia_lcd_async_prepare(maia_lcd_wake_t fn,void *arg){
    if(!fn)return MAIA_DSI_ACCESS_INVALID;
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase==ASYNC_ERROR)return MAIA_DSI_ACCESS_FAILED;
    if(async_phase!=ASYNC_FREE||framebuffer_claim||dpi_dma_state!=UNCLAIMED||dpi_bridge_state!=OWNED)return MAIA_DSI_ACCESS_BUSY;
    async_phase=ASYNC_ERROR;
    async_report.init_stage=1;
    uint32_t source=maia_psram_window_dma_acquire();if(source!=DSI_FB_SOURCE)return MAIA_DSI_ACCESS_BUSY;
    framebuffer_claim=1;lcd_stream_mode=1;
    async_report.init_stage=2;
    maia_dsi_dma_stage_report_t stage;
    maia_dsi_access_result_t result=dma_stage_body(source,0,&stage);if(result!=MAIA_DSI_ACCESS_OK)return result;
    async_report.init_stage=3;async_signal_before=dma_staged_words[11];
    uint32_t signal=(async_signal_before&~dma_stage_masks[11])|(FRAME_ERRORS|2U);
    dsi_write(dma_stage_address(11),signal);
    if(!frame_roots()||dsi_read(dma_stage_address(11))!=signal)return MAIA_DSI_ACCESS_VERIFY;
    dma_staged_words[11]=signal;
    async_wake=fn;async_arg=arg;
    async_report.init_stage=4;if(!maia_dw_irq_claim(lcd_irq,0))return MAIA_DSI_ACCESS_VERIFY;
    async_report.init_stage=5;async_phase=ASYNC_READY;return MAIA_DSI_ACCESS_OK;
}
maia_dsi_access_result_t maia_lcd_async_start(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase==ASYNC_ERROR)return MAIA_DSI_ACCESS_FAILED;
    if(async_phase!=ASYNC_READY)return MAIA_DSI_ACCESS_BUSY;
    if(!frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0)||!dma_quiet())return MAIA_DSI_ACCESS_VERIFY;
    uint32_t pending=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U);
    if(pending&~0x4000001BU)return MAIA_DSI_ACCESS_IO;
    dsi_write(MAIA_ESP32P4_GDMA_BASE+0x198U,0x4000001BU);
    dsi_write(dma_stage_address(0),DSI_FB_SOURCE);
    if(!frame_roots()||dsi_read(dma_stage_address(0))!=DSI_FB_SOURCE||dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U))return MAIA_DSI_ACCESS_VERIFY;
    for(unsigned i=0;i<sizeof async_report.last;++i)((volatile unsigned char *)&async_report.last)[i]=0;
    maia_dsi_dma_frame_report_t *r=&async_report.last;
    async_pending=async_irq_bad=0;async_phase=ASYNC_ACTIVE;dpi_frame_state=FAULTED;transfer_state=OWNED;
    uint32_t mode=saved_command_table[0],lp=dpi_staged_phy[3],dpi=dpi_staged_phy[4];
    if(!frame_store(r,0,MAIA_ESP32P4_GDMA_BASE+0x10U,3U))goto failed;
    dsi_write(MAIA_ESP32P4_GDMA_BASE+0x18U,0x101U);
    if(!frame_roots()||!(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U))goto failed;
    r->saw_active=1;
    if(!frame_guard(mode,lp,dpi,3U)||!frame_store(r,2,MAIA_ESP32P4_DSI_HOST_BASE+0x34U,mode&~1U))goto failed;
    mode&=~1U;
    if(!frame_guard(mode,lp,dpi,3U)||!frame_store(r,3,MAIA_ESP32P4_DSI_HOST_BASE+0x94U,lp|3U))goto failed;
    lp|=3U;
    if(!frame_guard(mode,lp,dpi,3U)||!frame_store(r,4,MAIA_ESP32P4_DSI_BRG_BASE+0x40U,dpi|1U))goto failed;
    dsi_write(MAIA_ESP32P4_DSI_BRG_BASE+0x44U,1U);r->completed=7;return MAIA_DSI_ACCESS_OK;
failed:
    async_phase=ASYNC_ERROR;return MAIA_DSI_ACCESS_VERIFY;
}
int maia_lcd_async_pending(void){return dsi_context()&&async_phase==ASYNC_ACTIVE&&async_pending;}
maia_dsi_access_result_t maia_lcd_async_finish(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase==ASYNC_ERROR)return MAIA_DSI_ACCESS_FAILED;
    if(async_phase!=ASYNC_ACTIVE||!async_pending)return MAIA_DSI_ACCESS_BUSY;
    maia_dsi_dma_frame_report_t *r=&async_report.last;
    if(async_irq_bad||!frame_guard(saved_command_table[0]&~1U,dpi_staged_phy[3]|3U,dpi_staged_phy[4]|1U,1U))goto bad;
    maia_dsi_access_result_t result=frame_observe(r);
    if(result!=MAIA_DSI_ACCESS_OK)goto bad;
    if(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&1U||r->dma_fifo||r->bridge_fifo){++async_report.drain_retries;return MAIA_DSI_ACCESS_BUSY;}
    if(!(r->channel_status&2U)||r->count_raw!=230399U)goto bad;
    r->done=1;r->items=230400U;r->completed=8;
    result=frame_cleanup(32,r);r->recovery_result=(int32_t)result;
    if(result!=MAIA_DSI_ACCESS_OK||(r->channel_status&FRAME_ERRORS)||r->common_status||(r->bridge_status&1U)||(r->observation.error0&0x1FFFFFU)||(r->observation.error1&0x81FFFU))goto bad;
    r->transfer_result=0;r->completed=9;++async_report.frames;async_phase=ASYNC_READY;
    dpi_frame_state=3U;transfer_state=UNCLAIMED;return MAIA_DSI_ACCESS_OK;
bad:
    async_phase=ASYNC_ERROR;return MAIA_DSI_ACCESS_VERIFY;
}
maia_dsi_access_result_t maia_lcd_async_cancel(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase!=ASYNC_ACTIVE&&async_phase!=ASYNC_ERROR)return MAIA_DSI_ACCESS_BUSY;
    async_phase=ASYNC_ERROR;
    maia_dsi_dma_frame_report_t *r=&async_report.last;r->transfer_result=MAIA_DSI_ACCESS_TIMEOUT;
    maia_dsi_access_result_t result=frame_cleanup(32,r);r->recovery_result=(int32_t)result;return result;
}
maia_dsi_access_result_t maia_lcd_async_stop(void){
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(async_phase!=ASYNC_READY&&async_phase!=ASYNC_ERROR)return MAIA_DSI_ACCESS_BUSY;
    if(!dma_quiet()||!frame_guard(saved_command_table[0],dpi_staged_phy[3],dpi_staged_phy[4],0))return MAIA_DSI_ACCESS_VERIFY;
    dsi_write(dma_stage_address(11),async_signal_before);
    if(dsi_read(dma_stage_address(11))!=async_signal_before||!maia_dw_irq_release()||!maia_psram_window_dma_release())return MAIA_DSI_ACCESS_VERIFY;
    async_report.released=1;framebuffer_claim=0;lcd_stream_mode=0;async_phase=ASYNC_STOPPED;return MAIA_DSI_ACCESS_OK;
}
void maia_lcd_async_report(maia_lcd_async_report_t *r){if(r&&dsi_context())for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=((const unsigned char *)&async_report)[i];}
