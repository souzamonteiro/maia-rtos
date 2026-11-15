/* Original MIT. Private opt-in five-block spatial DMA probe, no flash/SD/IRQ.
 * Own tile/list from preceding CPU-only fixture, explicit finite publication.
 * No automatic visual acceptance; owner must be ready before RAM upload. */
#include "../../soc/espressif/esp32p4/dsi_dma_spatial_probe.h"
#include "dpi_backlight_fixture.h"
static void dpi_dma_spatial(void){
    uint32_t start=(uint32_t)(uintptr_t)__maia_dpi_dma_start,source=start+128U,list=start+11904U;
    volatile unsigned char *mem=(volatile unsigned char *)(uintptr_t)(start+0x40000000U);
    volatile uint32_t *nodes=(volatile uint32_t *)(mem+11904U);
    if(!dsi_spatial_layout(source,list)||!vo3_dma_quiet(rd))halt("spatial-layout");
    uint32_t source_hash=dpi_tile_hash(mem+128U,DPI_TILE_BYTES),tail_hash=dpi_tile_hash(mem+12224U,DPI_LIST_BYTES-320U);
    for(unsigned n=0;n<5U;++n)for(unsigned w=0;w<16U;++w)nodes[n*16U+w]=dsi_spatial_word(source,list,n,w);
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    for(unsigned n=0;n<5U;++n)for(unsigned w=0;w<16U;++w)if(nodes[n*16U+w]!=dsi_spatial_word(source,list,n,w))halt("spatial-readback");
    field("DPI_SPATIAL_MEMORY source=",source);field(" list=",list);field(" nodes=",5);field(" source_bytes=",11520);
    field(" list_bytes=",320);field(" frame_bytes=",1843200);maia_console_puts("\n");
    mipi_snapshot_t before,after;if(!mipi_snapshot(&before,rd))halt("spatial-before");
    dpi_backlight_snapshot_t light;if(!dpi_bl_on(rd,wr,&light))halt("spatial-light");
    maia_dsi_spatial_probe_report_t r,again;uint64_t began=maia_soc_time_us();
    maia_dsi_access_result_t result=maia_dsi_spatial_probe(source,list,400,100000,&r);
    uint32_t elapsed=(uint32_t)(maia_soc_time_us()-began);
    field("DPI_SPATIAL result=",(uint32_t)result);field(" stage=",r.stage.completed);
    field(" frame_stage=",r.refresh.last.completed);field(" attempted=",r.refresh.last.attempted_mask);field(" frames=",r.refresh.frames_completed);field(" polls=",r.refresh.polls);field(" items=",r.refresh.items);field(" rearms=",r.refresh.rearm_writes);
    field(" active=",r.refresh.last.saw_active);field(" done=",r.refresh.last.done);field(" count_raw=",r.refresh.last.count_raw);
    field(" status=",r.refresh.last.channel_status);field(" common=",r.refresh.last.common_status);field(" underrun=",r.refresh.last.bridge_status);
    field(" dma_fifo=",r.refresh.last.dma_fifo);field(" bridge_fifo=",r.refresh.last.bridge_fifo);
    field(" dsi_status=",r.refresh.last.observation.status);field(" dsi_fifo=",r.refresh.last.observation.fifo);field(" error0=",r.refresh.last.observation.error0);field(" error1=",r.refresh.last.observation.error1);
    field(" transfer=",(uint32_t)r.refresh.last.transfer_result);field(" recovery=",(uint32_t)r.refresh.last.recovery_result);field(" recovered=",r.refresh.last.recovered);field(" elapsed_us=",elapsed);maia_console_puts("\n");
    for(unsigned n=0;n<5U;++n)for(unsigned w=0;w<16U;++w){
        field("DPI_SPATIAL_WORD index=",n*16U+w);field(" before=",dsi_spatial_word(source,list,n,w));field(" after=",nodes[n*16U+w]);maia_console_puts("\n");
    }
    if(!r.refresh.last.recovered && !(r.stage.completed<18U&&vo3_dma_quiet(rd)))halt("spatial-unrecovered");
    if(!dpi_bl_restore(rd,wr,&light))halt("spatial-light-restore");
    unsigned duplicate=maia_dsi_spatial_probe(source,list,400,100000,&again)==MAIA_DSI_ACCESS_BUSY;
    unsigned preserved=mipi_snapshot(&after,rd)&&before.cpu==after.cpu&&before.sys==after.sys&&before.phy==after.phy&&before.valid==after.valid&&before.csi_bridge==after.csi_bridge;
    for(unsigned i=0;i<6U;++i)if(before.dsi[i]!=after.dsi[i])preserved=0;
    for(unsigned i=0;i<4U;++i)if(before.csi[i]!=after.csi[i])preserved=0;
    for(unsigned i=0;i<3U;++i)if(before.dma[i]!=after.dma[i])preserved=0;
    for(unsigned c=1;c<4U;++c)for(unsigned i=0;i<7U;++i)if(before.channel[c][i]!=after.channel[c][i])preserved=0;
    unsigned guards=1;for(unsigned i=0;i<DPI_TILE_TOTAL;++i)
        if(!((i>=128U&&i<11648U)||(i>=11904U&&i<22144U))&&mem[i]!=0xA5)guards=0;
    uint32_t hash=dpi_tile_hash(mem+128U,DPI_TILE_BYTES);
    field("DPI_SPATIAL_RESTORE light=",1);field(" mode=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34U));field(" lp=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x94U));
    field(" dpi=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U));field(" cfg=",rd(MAIA_ESP32P4_GDMA_BASE+0x10U));field(" channels=",rd(MAIA_ESP32P4_GDMA_BASE+0x18U));field(" abort=",rd(MAIA_ESP32P4_GDMA_BASE+0x1CU));
    field(" source_hash=",hash);field(" source_kept=",hash==source_hash);field(" tail_kept=",dpi_tile_hash(mem+12224U,DPI_LIST_BYTES-320U)==tail_hash);
    field(" guards=",guards);field(" preserved=",preserved);field(" duplicate_busy=",duplicate);maia_console_puts("\n");
    if(result!=MAIA_DSI_ACCESS_OK||!guards||!preserved||!duplicate||hash!=source_hash||dpi_tile_hash(mem+12224U,DPI_LIST_BYTES-320U)!=tail_hash)halt("spatial-result");
    for(unsigned i=0;i<4U;++i){
        maia_dsi_read_report_t p;result=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DPI_SPATIAL_REPLY index=",i);field(" result=",(uint32_t)result);field(" stage=",p.transfer.completed);
        field(" headers=",p.headers_attempted);field(" polls=",p.transfer.polls);field(" word=",p.word);field(" words=",p.words_received);
        field(" before=",p.transfer.command_before);field(" after=",p.transfer.command_after);field(" restored=",p.restore_attempted);
        field(" status=",p.transfer.status);field(" fifo=",p.transfer.fifo);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if(result!=MAIA_DSI_ACCESS_OK||p.transfer.completed!=10U||p.words_received!=1U||(p.word&0x14U)!=0x14U)halt("spatial-reply");
    }
    maia_console_puts("MAIA_NATIVE_DPI_SPATIAL_PASS\n");
}
