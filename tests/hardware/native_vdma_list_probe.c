/* Original MaiaRTOS MIT. Native two-block SRAM fetch/copy proof, no display
 * writes/IRQs/SDK runtime. See native-vdma-list audit for ownership and sources. */
#include <maia/console.h>
#include "vdma_list_fixture.h"
#include "native_mipi_snapshot.h"
static unsigned char reserved[DMA_FIXTURE_TOTAL] __attribute__((section(".maia_dma_probe"),aligned(128),used));
extern unsigned char __maia_dma_start[],__maia_dma_end[],__bss_end[],__stack_bottom[];
extern int maia_probe_cache_writeback_invalidate(uint32_t,uint32_t,uint32_t);
extern uint32_t maia_probe_cache_line_size(void);
static uint32_t rd(uintptr_t p){return MAIA_MMIO32(p);}
static void wr(uintptr_t p,uint32_t v){MAIA_MMIO32(p)=v;__asm__ volatile("fence iorw, iorw" ::: "memory");}
static void field(const char *k,uint32_t n){char s[9];const char *hex="0123456789abcdef";for(unsigned i=0;i<8;++i)s[i]=hex[(n>>(28U-4U*i))&15U];s[8]=0;maia_console_puts(k);maia_console_puts(s);}
static void halt(const char *reason){maia_console_puts("MAIA_VDMA_LIST_FAIL ");maia_console_puts(reason);maia_console_puts("\n");for(;;)__asm__ volatile("wfi");}
void maia_display_putc(char c){(void)c;}
void maia_display_refresh_dma_if_needed(void){}
void maia_boot_main(void){
    uint32_t hart,status;__asm__ volatile("csrr %0,mhartid":"=r"(hart));__asm__ volatile("csrr %0,mstatus":"=r"(status));if(hart||(status&8U))halt("context");
    uint32_t start=(uint32_t)(uintptr_t)__maia_dma_start,end=(uint32_t)(uintptr_t)__maia_dma_end;
    if(!dma_fixture_layout(start,end,(uint32_t)(uintptr_t)__bss_end,(uint32_t)(uintptr_t)__stack_bottom)||(uintptr_t)reserved!=start)halt("layout");
    mipi_snapshot_t before,after;if(!mipi_snapshot(&before,rd)||!(before.valid&MIPI_DMA)||before.dma[0]||before.dma[1]||before.dma[2]||rd(DMA_RESET)&1U)halt("initial");
    for(unsigned c=0;c<4;++c)if(rd(DMA_CH0+c*0x100U+0x34U)&0x7FFFU)halt("fifo");
    uint32_t line=maia_probe_cache_line_size();
    if(!(line==16||line==32||line==64||line==128)||maia_probe_cache_writeback_invalidate(0x10U,start,DMA_FIXTURE_TOTAL))halt("cache");
    volatile unsigned char *mem=(volatile unsigned char *)(uintptr_t)(start+DMA_FIXTURE_ALIAS),*src=mem+DMA_FIXTURE_GUARD,*dst=mem+DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD;
    volatile uint32_t *list=(volatile uint32_t *)(src+DMA_LIST_OFFSET);uint32_t expected[32];
    for(unsigned i=0;i<DMA_FIXTURE_TOTAL;++i)mem[i]=0xA5;
    for(unsigned i=0;i<DMA_FIXTURE_BYTES;++i)dst[i]=0;
    for(unsigned i=0;i<DMA_LIST_TILE_BYTES;++i)src[i]=(unsigned char)dma_list_pixel_byte(i);
    for(unsigned i=0;i<8U;++i)src[DMA_LIST_SOLID_OFFSET+i]=(unsigned char)dma_list_pixel_byte(DMA_LIST_TILE_BYTES+i);
    if(!dma_list_encode(start,list)||!dma_list_encode(start,expected))halt("encode");
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    for(unsigned i=0;i<32U;++i)if(list[i]!=expected[i])halt("encode-readback");
    if(!dma_fixture_configure(start,rd,wr))halt("config");
    wr(DMA_CH0+0x20U,15U);wr(DMA_CH0+0x28U,(start+DMA_FIXTURE_GUARD+DMA_LIST_OFFSET)|1U);
    if(rd(DMA_CH0+0x20U)!=15U||rd(DMA_CH0+0x28U)!=((start+DMA_FIXTURE_GUARD+DMA_LIST_OFFSET)|1U)||rd(DMA_CFG)||rd(DMA_CHEN))halt("list-config");
    wr(DMA_CH0+0x98U,0x4000001BU);if(rd(DMA_CH0+0x88U)||rd(DMA_COMMON_STATUS))halt("status-before");
    field("VDMA_LIST_MEMORY start=",start);field(" end=",end);field(" source=",start+DMA_FIXTURE_GUARD);field(" destination=",start+DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD);
    field(" list=",start+DMA_FIXTURE_GUARD+DMA_LIST_OFFSET);field(" cache_line=",line);maia_console_puts("\n");
    wr(DMA_CFG,1U);if(rd(DMA_CFG)!=1U)halt("enable");wr(DMA_CHEN,0x101U);
    unsigned done=0,polls=0;for(unsigned i=0;i<1000000U;++i){++polls;if(!(rd(DMA_CHEN)&1U)){done=1;break;}}
    /* Bounded stop/drain even on completion timeout; no unbounded active spin. */
    wr(DMA_CHEN,0x100U);wr(DMA_CFG,0);unsigned quiet=0;
    for(unsigned i=0;i<100000U;++i)if(!rd(DMA_CFG)&&!rd(DMA_CHEN)&&!(rd(DMA_CH0+0x34U)&0x7FFFU)){quiet=1;break;}
    if(!quiet)halt("stop");
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    for(unsigned i=0;i<64U;i+=4U){
        unsigned v=0;for(unsigned j=0;j<4U;++j)v|=(unsigned)dst[DMA_LIST_TILE_BYTES+i+j]<<(j*8U);
        field("VDMA_LIST_SAMPLE offset=",i);field(" word=",v);maia_console_puts("\n");
    }
    unsigned matched=0,kept=1,guards=1,list_valid=1;
    for(unsigned i=0;i<DMA_FIXTURE_BYTES;++i)if(dst[i]==dma_list_pixel_byte(i))++matched;
    for(unsigned i=0;i<DMA_FIXTURE_BYTES;++i){
        if(i<DMA_LIST_TILE_BYTES){if(src[i]!=dma_list_pixel_byte(i))kept=0;}
        else if(i>=DMA_LIST_SOLID_OFFSET&&i<DMA_LIST_SOLID_OFFSET+8U){if(src[i]!=dma_list_pixel_byte(DMA_LIST_TILE_BYTES+i-DMA_LIST_SOLID_OFFSET))kept=0;}
        else if(!(i>=DMA_LIST_OFFSET&&i<DMA_LIST_OFFSET+DMA_LIST_BYTES)&&src[i]!=0xA5)guards=0;
    }
    for(unsigned b=0;b<2U;++b)for(unsigned i=0;i<DMA_FIXTURE_GUARD;++i)
        if(mem[b*DMA_FIXTURE_SPAN+i]!=0xA5||mem[b*DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD+DMA_FIXTURE_BYTES+i]!=0xA5)guards=0;
    for(unsigned i=0;i<32U;++i){
        field("VDMA_LIST_WORD index=",i);field(" before=",expected[i]);field(" after=",list[i]);maia_console_puts("\n");
        if(list[i]!=dma_list_completed_word(i,expected[i]))list_valid=0;
    }
    field("VDMA_LIST_COPY done=",done);field(" polls=",polls);field(" matched=",matched);field(" expected=",DMA_FIXTURE_BYTES);
    field(" source_kept=",kept);field(" guards=",guards);field(" list_valid=",list_valid);field(" raw=",rd(DMA_CH0+0x30U));field(" fifo=",rd(DMA_CH0+0x34U));
    field(" status=",rd(DMA_CH0+0x88U));field(" common=",rd(DMA_COMMON_STATUS));maia_console_puts("\n");
    unsigned preserved=mipi_snapshot(&after,rd)&&before.cpu==after.cpu&&before.sys==after.sys&&before.phy==after.phy&&before.valid==after.valid&&before.csi_bridge==after.csi_bridge;
    for(unsigned i=0;i<7;++i)if(before.dsi[i]!=after.dsi[i])preserved=0;
    for(unsigned i=0;i<4;++i)if(before.csi[i]!=after.csi[i])preserved=0;
    for(unsigned c=1;c<4;++c)for(unsigned i=0;i<7;++i)if(before.channel[c][i]!=after.channel[c][i])preserved=0;
    field("VDMA_LIST_IDLE cfg=",rd(DMA_CFG));field(" channels=",rd(DMA_CHEN));field(" abort=",rd(DMA_ABORT));field(" preserved=",preserved);maia_console_puts("\n");
    if(!done||matched!=DMA_FIXTURE_BYTES||!kept||!guards||!list_valid||!preserved||(rd(DMA_CH0+0x88U)&DMA_FIXTURE_ERRORS)||rd(DMA_COMMON_STATUS))halt("data");
    maia_console_puts("MAIA_NATIVE_VDMA_LIST_PASS\n");for(;;)__asm__ volatile("wfi");
}
