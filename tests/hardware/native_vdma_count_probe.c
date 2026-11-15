/* Native SRAM-copy counter qualification. Original MaiaRTOS MIT. No display
 * writes, IRQs or SDK runtime. Reuses verified owned NC memory fixture. */
#include <maia/console.h>
#include "vdma_transfer_fixture.h"
#include "native_mipi_snapshot.h"
static unsigned char reserved[DMA_FIXTURE_TOTAL] __attribute__((section(".maia_dma_probe"),aligned(128),used));
extern unsigned char __maia_dma_start[],__maia_dma_end[],__bss_end[],__stack_bottom[];
extern int maia_probe_cache_writeback_invalidate(uint32_t,uint32_t,uint32_t);
extern uint32_t maia_probe_cache_line_size(void);
static uint32_t rd(uintptr_t p){return MAIA_MMIO32(p);}
static void wr(uintptr_t p,uint32_t v){MAIA_MMIO32(p)=v;__asm__ volatile("fence iorw, iorw" ::: "memory");}
static void field(const char *k,uint32_t n){char s[9];const char *hex="0123456789abcdef";for(unsigned i=0;i<8;++i)s[i]=hex[(n>>(28U-4U*i))&15U];s[8]=0;maia_console_puts(k);maia_console_puts(s);}
static void halt(const char *reason){maia_console_puts("MAIA_VDMA_COUNT_FAIL ");maia_console_puts(reason);maia_console_puts("\n");for(;;)__asm__ volatile("wfi");}
void maia_display_putc(char c){(void)c;}
void maia_display_refresh_dma_if_needed(void){}
void maia_boot_main(void){
    uint32_t hart,status;__asm__ volatile("csrr %0,mhartid":"=r"(hart));__asm__ volatile("csrr %0,mstatus":"=r"(status));if(hart||(status&8U))halt("context");
    uint32_t start=(uint32_t)(uintptr_t)__maia_dma_start,end=(uint32_t)(uintptr_t)__maia_dma_end;
    if(!dma_fixture_layout(start,end,(uint32_t)(uintptr_t)__bss_end,(uint32_t)(uintptr_t)__stack_bottom)||(uintptr_t)reserved!=start)halt("layout");
    mipi_snapshot_t before,after;if(!mipi_snapshot(&before,rd)||!(before.valid&MIPI_DMA)||before.dma[0]||before.dma[1]||before.dma[2])halt("initial");
    for(unsigned c=0;c<4;++c)if(rd(DMA_CH0+c*0x100U+0x34U)&0x7FFFU)halt("fifo");
    uint32_t line=maia_probe_cache_line_size();
    if(!(line==16||line==32||line==64||line==128)||maia_probe_cache_writeback_invalidate(0x10U,start,DMA_FIXTURE_TOTAL))halt("cache");
    volatile unsigned char *mem=(volatile unsigned char *)(uintptr_t)(start+DMA_FIXTURE_ALIAS),*src=mem+DMA_FIXTURE_GUARD,*dst=mem+DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD;
    static const unsigned widths[4]={8,64,8,64},counts[4]={8,8,256,4096};
    for(unsigned sample=0;sample<4;++sample){
        unsigned bytes=counts[sample]*(widths[sample]/8U);
        for(unsigned i=0;i<DMA_FIXTURE_TOTAL;++i)mem[i]=0xA5;
        for(unsigned i=0;i<DMA_FIXTURE_BYTES;++i){src[i]=(unsigned char)dma_fixture_pattern(i,sample*31U);dst[i]=0;}
        __asm__ volatile("fence iorw, iorw" ::: "memory");
        if(rd(DMA_CFG)||rd(DMA_CHEN)||rd(DMA_ABORT)||(rd(DMA_RESET)&1U)||!dma_fixture_configure(start,rd,wr))halt("config");
        uint32_t ctl=widths[sample]==64?0x1B05U:5U;
        wr(DMA_CH0+0x18U,ctl);wr(DMA_CH0+0x10U,counts[sample]-1U);
        if(rd(DMA_CH0+0x18U)!=ctl||rd(DMA_CH0+0x10U)!=counts[sample]-1U)halt("config-readback");
        wr(DMA_CH0+0x98U,0x4000001BU);
        if(rd(DMA_CH0+0x88U)||rd(DMA_COMMON_STATUS))halt("status-before");
        wr(DMA_CFG,1U);if(rd(DMA_CFG)!=1U)halt("enable");wr(DMA_CHEN,0x101U);
        unsigned done=0;for(unsigned i=0;i<1000000U;++i)if(!(rd(DMA_CHEN)&1U)){done=1;break;}
        if(!done)halt("timeout");
        wr(DMA_CFG,0);if(rd(DMA_CFG)||rd(DMA_CHEN))halt("stop");
        __asm__ volatile("fence iorw, iorw" ::: "memory");
        unsigned matched=0,tail=1,guards=1;
        for(unsigned i=0;i<DMA_FIXTURE_BYTES;++i){if(i<bytes){if(dst[i]==src[i])++matched;}else if(dst[i])tail=0;}
        for(unsigned b=0;b<2;++b)for(unsigned i=0;i<DMA_FIXTURE_GUARD;++i)
            if(mem[b*DMA_FIXTURE_SPAN+i]!=0xA5||mem[b*DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD+DMA_FIXTURE_BYTES+i]!=0xA5)guards=0;
        field("VDMA_COUNT index=",sample);field(" width=",widths[sample]);field(" items=",counts[sample]);field(" bytes=",bytes);field(" matched=",matched);
        field(" raw=",rd(DMA_CH0+0x30U));field(" status=",rd(DMA_CH0+0x88U));field(" common=",rd(DMA_COMMON_STATUS));field(" guards=",guards);field(" tail=",tail);maia_console_puts("\n");
        if(matched!=bytes||!tail||!guards||(rd(DMA_CH0+0x88U)&DMA_FIXTURE_ERRORS)||rd(DMA_COMMON_STATUS))halt("data");
    }
    unsigned kept=mipi_snapshot(&after,rd)&&before.cpu==after.cpu&&before.sys==after.sys&&before.phy==after.phy&&before.valid==after.valid&&before.csi_bridge==after.csi_bridge;
    for(unsigned i=0;i<7;++i)if(before.dsi[i]!=after.dsi[i])kept=0;
    for(unsigned i=0;i<4;++i)if(before.csi[i]!=after.csi[i])kept=0;
    for(unsigned c=1;c<4;++c)for(unsigned i=0;i<7;++i)if(before.channel[c][i]!=after.channel[c][i])kept=0;
    field("VDMA_COUNT_IDLE cfg=",rd(DMA_CFG));field(" channels=",rd(DMA_CHEN));field(" preserved=",kept);maia_console_puts("\n");
    if(!kept)halt("preserve");
    maia_console_puts("MAIA_NATIVE_VDMA_COUNT_PASS\n");for(;;)__asm__ volatile("wfi");
}
