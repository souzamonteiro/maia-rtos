/* Private contiguous solid-frame diagnostic. Prior160-LLI tile list remains
 * untouched/unpublished; a fixed8-byte source covers a single full-frame block. */
static void dpi_dma_stage(void){
    uint32_t start=(uint32_t)(uintptr_t)__maia_dpi_dma_start,source=start+128U;
    volatile unsigned char *mem=(volatile unsigned char *)(uintptr_t)(start+0x40000000U);
    for(unsigned i=0;i<8U;++i)if(mem[128U+i]!=(i&1U ? 0xF8U : 0U))halt("dma-stage-source");
    mipi_snapshot_t before,after;if(!mipi_snapshot(&before,rd))halt("dma-stage-snapshot");
    maia_dsi_dma_stage_report_t r,again;
    maia_dsi_access_result_t result=maia_esp32p4_dsi_dma_solid_stage(source,&r);
    field("DPI_DMA_STAGE result=",(uint32_t)result);field(" stage=",r.completed);field(" attempted=",r.attempted_mask);
    field(" source=",r.source);field(" frame_bytes=",r.frame_bytes);field(" channel_status=",r.channel_status);field(" common_status=",r.common_status);
    field(" status=",r.observation.status);field(" fifo=",r.observation.fifo);field(" error0=",r.observation.error0);field(" error1=",r.observation.error1);maia_console_puts("\n");
    for(unsigned i=0;i<MAIA_DSI_DMA_STAGE_WORDS;++i){
        field("DPI_DMA_STAGE_WORD index=",i);field(" before=",r.before[i]);field(" after=",r.after[i]);maia_console_puts("\n");
    }
    if(result!=MAIA_DSI_ACCESS_OK || r.completed!=18U)halt("dma-stage-config");
    int busy=maia_esp32p4_dsi_dma_solid_stage(source,&again)==MAIA_DSI_ACCESS_BUSY;
    int preserved=mipi_snapshot(&after,rd)&&before.cpu==after.cpu&&before.sys==after.sys&&before.phy==after.phy&&before.valid==after.valid&&before.csi_bridge==after.csi_bridge;
    for(unsigned i=0;i<7U;++i)if(before.dsi[i]!=after.dsi[i])preserved=0;
    for(unsigned i=0;i<4U;++i)if(before.csi[i]!=after.csi[i])preserved=0;
    for(unsigned i=0;i<3U;++i)if(before.dma[i]!=after.dma[i])preserved=0;
    for(unsigned c=1;c<4U;++c)for(unsigned i=0;i<7U;++i)if(before.channel[c][i]!=after.channel[c][i])preserved=0;
    field("DPI_DMA_STAGE_IDLE mode=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34U));field(" lp=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x94U));
    field(" dpi=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U));field(" cfg=",rd(MAIA_ESP32P4_GDMA_BASE+0x10U));
    field(" channels=",rd(MAIA_ESP32P4_GDMA_BASE+0x18U));field(" abort=",rd(MAIA_ESP32P4_GDMA_BASE+0x1CU));
    field(" source_hash=",dpi_tile_hash(mem+128U,DPI_TILE_BYTES));field(" list_hash=",dpi_tile_hash(mem+11904U,DPI_LIST_BYTES));
    field(" preserved=",(uint32_t)preserved);field(" duplicate_busy=",(uint32_t)busy);maia_console_puts("\n");
    if(!preserved||!busy||!vo3_dma_quiet(rd))halt("dma-stage-preserve");
    for(unsigned i=0;i<4U;++i){
        maia_dsi_read_report_t p;result=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DPI_DMA_STAGE_REPLY index=",i);field(" result=",(uint32_t)result);field(" stage=",p.transfer.completed);
        field(" headers=",p.headers_attempted);field(" polls=",p.transfer.polls);field(" word=",p.word);field(" words=",p.words_received);
        field(" before=",p.transfer.command_before);field(" after=",p.transfer.command_after);field(" restored=",p.restore_attempted);
        field(" status=",p.transfer.status);field(" fifo=",p.transfer.fifo);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if(result!=MAIA_DSI_ACCESS_OK||p.transfer.completed!=10U||p.words_received!=1U||(p.word&0x14U)!=0x14U)halt("dma-stage-reply");
    }
    maia_console_puts("MAIA_NATIVE_DPI_DMA_STAGE_PASS\n");
}
