/* Private one-frame probe. Visual confirmation is independent of DMA completion. */
#include "dpi_backlight_fixture.h"
static void dpi_dma_frame(void){
    dpi_backlight_snapshot_t light;
    if(!dpi_bl_on(rd,wr,&light))halt("dma-frame-light");
    field("DPI_DMA_FRAME_LIGHT out=",rd(MAIA_ESP32P4_GPIO_OUT_REG));field(" enable=",rd(MAIA_ESP32P4_GPIO_ENABLE_REG));
    field(" pad=",rd(MAIA_ESP32P4_GPIO_PIN_REG(22)));field(" mux=",rd(MAIA_ESP32P4_IO_MUX_GPIO_REG(22)));
    field(" matrix=",rd(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22)));maia_console_puts("\n");
    mipi_snapshot_t before,after;if(!mipi_snapshot(&before,rd))halt("dma-frame-before");
    uint64_t began=maia_soc_time_us();
    maia_dsi_dma_frame_report_t r,again;
    maia_dsi_access_result_t result=maia_esp32p4_dsi_dma_solid_frame(100000,&r);
    uint32_t elapsed=(uint32_t)(maia_soc_time_us()-began);
    field("DPI_DMA_FRAME result=",(uint32_t)result);field(" stage=",r.completed);field(" attempted=",r.attempted_mask);
    field(" polls=",r.polls);field(" recovery_polls=",r.recovery_polls);field(" active=",r.saw_active);field(" done=",r.done);
    field(" count_raw=",r.count_raw);field(" items=",r.items);field(" channel_status=",r.channel_status);field(" common_status=",r.common_status);
    field(" bridge_status=",r.bridge_status);field(" dma_fifo=",r.dma_fifo);field(" bridge_fifo=",r.bridge_fifo);field(" recovered=",r.recovered);
    field(" transfer_result=",(uint32_t)r.transfer_result);field(" recovery_result=",(uint32_t)r.recovery_result);
    field(" dsi_status=",r.observation.status);field(" dsi_fifo=",r.observation.fifo);field(" error0=",r.observation.error0);field(" error1=",r.observation.error1);
    field(" elapsed_us=",elapsed);maia_console_puts("\n");
    if(!r.recovered)halt("dma-frame-unrecovered");
    if(!dpi_bl_restore(rd,wr,&light))halt("dma-frame-light-restore");
    int duplicate=maia_esp32p4_dsi_dma_solid_frame(1,&again)==MAIA_DSI_ACCESS_BUSY;
    unsigned preserved=mipi_snapshot(&after,rd)&&before.cpu==after.cpu&&before.sys==after.sys&&before.phy==after.phy&&before.valid==after.valid&&before.csi_bridge==after.csi_bridge;
    for(unsigned i=0;i<6U;++i)if(before.dsi[i]!=after.dsi[i])preserved=0; /* PHY live status separate */
    for(unsigned i=0;i<4U;++i)if(before.csi[i]!=after.csi[i])preserved=0;
    for(unsigned i=0;i<3U;++i)if(before.dma[i]!=after.dma[i])preserved=0;
    for(unsigned c=1;c<4U;++c)for(unsigned i=0;i<7U;++i)if(before.channel[c][i]!=after.channel[c][i])preserved=0;
    uint32_t start=(uint32_t)(uintptr_t)__maia_dpi_dma_start;
    volatile unsigned char *mem=(volatile unsigned char *)(uintptr_t)(start+0x40000000U);
    field("DPI_DMA_FRAME_RESTORE light=",1);field(" mode=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34U));field(" lp=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x94U));
    field(" dpi=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U));field(" cfg=",rd(MAIA_ESP32P4_GDMA_BASE+0x10U));field(" channels=",rd(MAIA_ESP32P4_GDMA_BASE+0x18U));field(" abort=",rd(MAIA_ESP32P4_GDMA_BASE+0x1CU));
    field(" source_hash=",dpi_tile_hash(mem+128U,DPI_TILE_BYTES));field(" list_hash=",dpi_tile_hash(mem+11904U,DPI_LIST_BYTES));
    field(" preserved=",preserved);field(" duplicate_busy=",(uint32_t)duplicate);maia_console_puts("\n");
    if(result!=MAIA_DSI_ACCESS_OK||!preserved||!duplicate)halt("dma-frame-result");
    for(unsigned i=0;i<4U;++i){
        maia_dsi_read_report_t p;result=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DPI_DMA_FRAME_REPLY index=",i);field(" result=",(uint32_t)result);field(" stage=",p.transfer.completed);
        field(" headers=",p.headers_attempted);field(" polls=",p.transfer.polls);field(" word=",p.word);field(" words=",p.words_received);
        field(" before=",p.transfer.command_before);field(" after=",p.transfer.command_after);field(" restored=",p.restore_attempted);
        field(" status=",p.transfer.status);field(" fifo=",p.transfer.fifo);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if(result!=MAIA_DSI_ACCESS_OK||p.transfer.completed!=10U||p.words_received!=1U||(p.word&0x14U)!=0x14U)halt("dma-frame-reply");
    }
    maia_console_puts("MAIA_NATIVE_DPI_DMA_FRAME_PASS\n");
}
