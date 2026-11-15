/* Private owned SRAM tile/list diagnostic: memory construction only, no DMA
 * programming/publication. ROM/cache facts reused from native-vdma-transfer.md. */
#include "../../soc/espressif/esp32p4/dsi_dma_descriptor.h"
#define DPI_TILE_BYTES 11520U
#define DPI_LIST_BYTES 10240U
#define DPI_TILE_TOTAL 22272U
static unsigned char dpi_dma_reserved[DPI_TILE_TOTAL] __attribute__((section(".maia_dpi_dma_probe"),aligned(128),used));
extern unsigned char __maia_dpi_dma_start[],__maia_dpi_dma_end[],__bss_end[],__stack_bottom[];
extern int maia_probe_cache_writeback_invalidate(uint32_t,uint32_t,uint32_t);
extern uint32_t maia_probe_cache_line_size(void);
static uint32_t dpi_tile_hash(volatile const unsigned char *p,uint32_t size) {
    uint32_t h=2166136261U;for(uint32_t i=0;i<size;++i) h=(h^p[i])*16777619U;return h;
}
static void dpi_dma_tile(void) {
    uint32_t start=(uint32_t)(uintptr_t)__maia_dpi_dma_start,end=(uint32_t)(uintptr_t)__maia_dpi_dma_end;
    if ((start&127U) || start<DSI_DMA_SRAM_LOW || end>DSI_DMA_SRAM_END || end-start != DPI_TILE_TOTAL ||
        (uint32_t)(uintptr_t)dpi_dma_reserved != start || (uintptr_t)__bss_end>start || end>(uintptr_t)__stack_bottom)
        halt("dpi-dma-layout");
    mipi_snapshot_t before,after;
    if (!mipi_snapshot(&before,rd) || !(before.valid&MIPI_DMA) || !vo3_dma_quiet(rd)) halt("dpi-dma-initial");
    uint32_t line=maia_probe_cache_line_size();
    if (!(line==16U || line==32U || line==64U || line==128U) ||
        maia_probe_cache_writeback_invalidate(0x10U,start,DPI_TILE_TOTAL)) halt("dpi-dma-cache");
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    volatile unsigned char *mem=(volatile unsigned char *)(uintptr_t)(start+0x40000000U);
    dsi_dma_tile_request_t request={720,1280,8,start+128U,DPI_TILE_BYTES,start+11904U,DPI_LIST_BYTES};
    dsi_dma_tile_plan_t plan;
    if(!dsi_dma_tile_plan(&request,&plan)) halt("dpi-dma-plan");
    for(unsigned i=0;i<DPI_TILE_TOTAL;++i)mem[i]=0xA5;
    static const uint16_t colors[4]={0xF800,0x07E0,0x001F,0xFFE0};
    for(unsigned y=0;y<8U;++y)for(unsigned x=0;x<720U;++x){
        unsigned offset=128U+(y*720U+x)*2U;uint16_t color=colors[x/180U];
        mem[offset]=(unsigned char)color;mem[offset+1U]=(unsigned char)(color>>8U);
    }
    for(unsigned i=0;i<plan.count;++i)
        if(!dsi_dma_tile_encode(&request,i,(volatile uint32_t *)(mem+11904U+i*64U))) halt("dpi-dma-encode");
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    field("DPI_DMA_TILE_MEMORY start=",start);field(" end=",end);field(" alias=",start+0x40000000U);
    field(" source=",request.source);field(" list=",request.list);field(" source_bytes=",plan.source_bytes);
    field(" count=",plan.count);field(" list_bytes=",plan.list_bytes);field(" frame_bytes=",plan.frame_bytes);
    field(" cache_line=",line);field(" prepared=",1);maia_console_puts("\n");
    int guards=1;
    for(unsigned i=0;i<DPI_TILE_TOTAL;++i)
        if(!((i>=128U && i<11648U) || (i>=11904U && i<22144U)) && mem[i]!=0xA5) guards=0;
    for(unsigned i=0;i<plan.count;++i){
        volatile uint32_t *item=(volatile uint32_t *)(mem+11904U+i*64U);uint32_t expected[16];
        if(!dsi_dma_tile_encode(&request,i,expected)) halt("dpi-dma-verify-plan");
        for(unsigned w=0;w<16U;++w)if(item[w]!=expected[w])halt("dpi-dma-readback");
        if(i==0U || i==80U || i==159U){
            unsigned zero=1;for(unsigned w=0;w<16U;++w)if(w!=0U && w!=2U && w!=4U && w!=6U && w!=8U && w!=9U && item[w])zero=0;
            field("DPI_DMA_TILE_ITEM index=",i);field(" source=",item[0]);field(" destination=",item[2]);
            field(" items=",item[4]+1U);field(" next=",item[6]);field(" ctl0=",item[8]);field(" ctl1=",item[9]);field(" zero=",zero);maia_console_puts("\n");
        }
    }
    unsigned preserved=mipi_snapshot(&after,rd) && before.cpu==after.cpu && before.sys==after.sys && before.phy==after.phy &&
        before.valid==after.valid && before.csi_bridge==after.csi_bridge;
    for(unsigned i=0;i<7U;++i)if(before.dsi[i]!=after.dsi[i])preserved=0;
    for(unsigned i=0;i<4U;++i)if(before.csi[i]!=after.csi[i])preserved=0;
    for(unsigned i=0;i<3U;++i)if(before.dma[i]!=after.dma[i])preserved=0;
    for(unsigned c=0;c<4U;++c)for(unsigned i=0;i<7U;++i)if(before.channel[c][i]!=after.channel[c][i])preserved=0;
    field("DPI_DMA_TILE_VERIFY source_hash=",dpi_tile_hash(mem+128U,DPI_TILE_BYTES));
    field(" list_hash=",dpi_tile_hash(mem+11904U,DPI_LIST_BYTES));field(" guards=",(uint32_t)guards);
    field(" preserved=",preserved);field(" inert=",(uint32_t)vo3_dma_quiet(rd));field(" repeated_bytes=",plan.count*plan.source_bytes);maia_console_puts("\n");
    if(!guards || !preserved || !vo3_dma_quiet(rd))halt("dpi-dma-preserve");
    for(unsigned i=0;i<4U;++i){
        maia_dsi_read_report_t p;maia_dsi_access_result_t result=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DPI_DMA_TILE_REPLY index=",i);field(" result=",(uint32_t)result);field(" stage=",p.transfer.completed);
        field(" headers=",p.headers_attempted);field(" polls=",p.transfer.polls);field(" word=",p.word);field(" words=",p.words_received);
        field(" before=",p.transfer.command_before);field(" after=",p.transfer.command_after);field(" restored=",p.restore_attempted);
        field(" status=",p.transfer.status);field(" fifo=",p.transfer.fifo);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if(result!=MAIA_DSI_ACCESS_OK || p.transfer.completed!=10U || p.words_received!=1U || (p.word&0x14U)!=0x14U)halt("dpi-dma-reply");
    }
    maia_console_puts("MAIA_NATIVE_DPI_DMA_TILE_PASS\n");
}
