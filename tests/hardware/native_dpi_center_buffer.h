/* Original MIT. Private central32-row NC SRAM memory preparation only.
 * No descriptor publication/MMIO activation. Prior cache/ROM/NC evidence:
 * native-dpi-dma-tile.md and native-dpi-dma-center.md. */
#define DPI_TILE_BYTES 46080U
#define DPI_LIST_BYTES 384U
#define DPI_LIST_OFFSET 46336U
#define DPI_TILE_TOTAL 46848U
static unsigned char dpi_dma_reserved[DPI_TILE_TOTAL] __attribute__((section(".maia_dpi_dma_probe"),aligned(128),used));
extern unsigned char __maia_dpi_dma_start[],__maia_dpi_dma_end[],__bss_end[],__stack_bottom[];
extern int maia_probe_cache_writeback_invalidate(uint32_t,uint32_t,uint32_t);
extern uint32_t maia_probe_cache_line_size(void);
static uint32_t dpi_tile_hash(volatile const unsigned char *p,uint32_t size){
    uint32_t h=2166136261U;for(uint32_t i=0;i<size;++i)h=(h^p[i])*16777619U;return h;
}
static void dpi_center_buffer(void){
    uint32_t start=(uint32_t)(uintptr_t)__maia_dpi_dma_start,end=(uint32_t)(uintptr_t)__maia_dpi_dma_end;
    if((start&127U)||start<0x4FF00000U||end>0x4FF28000U||end-start!=DPI_TILE_TOTAL||
       (uintptr_t)dpi_dma_reserved!=start||(uintptr_t)__bss_end>start||end>(uintptr_t)__stack_bottom||!vo3_dma_quiet(rd))halt("center-layout");
    mipi_snapshot_t before,after;if(!mipi_snapshot(&before,rd))halt("center-before");
    uint32_t line=maia_probe_cache_line_size();
    if(!(line==16U||line==32U||line==64U||line==128U)||maia_probe_cache_writeback_invalidate(0x10U,start,DPI_TILE_TOTAL))halt("center-cache");
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    volatile unsigned char *mem=(volatile unsigned char *)(uintptr_t)(start+0x40000000U);
    for(unsigned i=0;i<DPI_TILE_TOTAL;++i)mem[i]=0xA5;
    static const uint16_t colors[4]={0xF800,0x07E0,0x001F,0xFFE0};
    for(unsigned y=0;y<32U;++y)for(unsigned x=0;x<720U;++x){
        unsigned offset=128U+(y*720U+x)*2U;uint16_t c=colors[x/180U];mem[offset]=(unsigned char)c;mem[offset+1U]=(unsigned char)(c>>8U);
    }
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    unsigned guards=1;
    for(unsigned i=0;i<DPI_TILE_TOTAL;++i){
        if(i>=128U&&i<128U+DPI_TILE_BYTES){
            unsigned byte=i-128U;uint16_t c=colors[((byte/2U)%720U)/180U];if(mem[i]!=(unsigned char)(c>>((byte&1U)*8U)))halt("center-pixels");
        }else if(mem[i]!=0xA5)guards=0;
    }
    unsigned preserved=mipi_snapshot(&after,rd)&&before.cpu==after.cpu&&before.sys==after.sys&&before.phy==after.phy&&before.valid==after.valid&&before.csi_bridge==after.csi_bridge;
    for(unsigned i=0;i<7U;++i)if(before.dsi[i]!=after.dsi[i])preserved=0;
    for(unsigned i=0;i<4U;++i)if(before.csi[i]!=after.csi[i])preserved=0;
    for(unsigned i=0;i<3U;++i)if(before.dma[i]!=after.dma[i])preserved=0;
    for(unsigned c=0;c<4U;++c)for(unsigned i=0;i<7U;++i)if(before.channel[c][i]!=after.channel[c][i])preserved=0;
    field("DPI_CENTER_BUFFER start=",start);field(" end=",end);field(" alias=",start+0x40000000U);field(" source=",start+128U);field(" list=",start+DPI_LIST_OFFSET);
    field(" source_bytes=",DPI_TILE_BYTES);field(" list_capacity=",DPI_LIST_BYTES);field(" span=",DPI_TILE_TOTAL);field(" cache_line=",line);field(" prepared=",1);maia_console_puts("\n");
    field("DPI_CENTER_BUFFER_VERIFY source_hash=",dpi_tile_hash(mem+128U,DPI_TILE_BYTES));field(" guards=",guards);field(" preserved=",preserved);field(" inert=",(uint32_t)vo3_dma_quiet(rd));maia_console_puts("\n");
    if(!guards||!preserved||!vo3_dma_quiet(rd))halt("center-memory");
    maia_console_puts("MAIA_NATIVE_DPI_CENTER_BUFFER_PASS\n");
}
