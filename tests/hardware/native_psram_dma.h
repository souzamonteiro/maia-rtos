/* Boot-only PSRAM->SRAM byte qualification. Full PSRAM/cache/DW-GDMA exclusive
 * ownership; native window immutable and synchronized before entry. Uses only
 * existing L2MEM NC alias, never invents a PSRAM NC alias. No display writes. */
#include "psram_dma_plan.h"
#include "vdma_transfer_fixture.h"
#include "native_mipi_snapshot.h"
extern uint64_t maia_soc_time_us(void);
static uint8_t psram_dma_memory[PSRAM_DMA_TOTAL] __attribute__((aligned(128)));
static const unsigned psram_dma_offsets[12]={0,4,8,12,16,24,28,32,36,40,44,144};
static uint32_t psram_dma_read(uintptr_t p){return psram_read(p);}
static int psram_dma_roots(const mipi_snapshot_t *s){
    return psram_read(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)==s->cpu&&
        psram_read(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG)==s->sys&&psram_read(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG)==s->phy;
}
static int psram_dma(void){
    mipi_snapshot_t before,after;
    if(!mipi_snapshot(&before,psram_dma_read)||!(before.valid&MIPI_DMA)||before.dma[0]||before.dma[1]||before.dma[2]||psram_read(DMA_RESET)&1U)return 0;
    for(unsigned c=0;c<4U;++c)if(psram_read(DMA_CH0+c*0x100U+0x34U)&0x7FFFU)return 0;
    uint32_t base=(uint32_t)(uintptr_t)psram_dma_memory;
    if(base<0x4FF00000U||base>0x4FF28000U-PSRAM_DMA_TOTAL)return 0;
    if(((int(*)(uint32_t,uint32_t,uint32_t))(uintptr_t)0x4FC003FCU)(0x10U,base,PSRAM_DMA_TOTAL))return 0;
    volatile uint8_t *mem=(volatile uint8_t *)(uintptr_t)(base+0x40000000U);
    uint32_t saved[12],plan[12];
    for(unsigned j=0;j<12U;++j)saved[j]=psram_read(DMA_CH0+psram_dma_offsets[j]);
    for(unsigned i=0;i<9U;++i){
        unsigned offset=i/3U==0U?0U:i/3U==1U?65408U:0x1FF000U,burst=i%3U==0U?1U:i%3U==1U?4U:16U;
        for(unsigned j=0;j<PSRAM_DMA_TOTAL;++j)mem[j]=0xA5;
        if(!psram_dma_roots(&before)||psram_read(DMA_CFG)||psram_read(DMA_CHEN)||psram_read(DMA_ABORT)||!psram_dma_plan(MAIA_PSRAM_WINDOW_VADDR+offset,base+128U,burst,plan))return 0;
        plan[11]=saved[11]&~DMA_FIXTURE_SIGNAL_RW;
        for(unsigned j=0;j<12U;++j){
            if(!psram_dma_roots(&before))return 0;
            psram_write(DMA_CH0+psram_dma_offsets[j],plan[j]);
            if(!psram_dma_roots(&before)||psram_read(DMA_CH0+psram_dma_offsets[j])!=plan[j])return 0;
        }
        psram_write(DMA_CH0+0x98U,0x4000001BU);
        if(psram_read(DMA_CH0+0x88U)||psram_read(DMA_COMMON_STATUS))return 0;
        uint64_t start=maia_soc_time_us();
        psram_write(DMA_CFG,1);if(psram_read(DMA_CFG)!=1U)return 0;
        psram_write(DMA_CHEN,0x101U);
        unsigned done=0,polls=0;
        for(unsigned n=0;n<1000000U;++n){
            if(!psram_dma_roots(&before))return 0;
            ++polls;if(!(psram_read(DMA_CHEN)&1U)){done=1;break;}
        }
        uint32_t elapsed=(uint32_t)(maia_soc_time_us()-start);
        /* Never release ownership of destination/window on uncertain stop. */
        if(!psram_dma_roots(&before))return 0;
        psram_write(DMA_CHEN,0x100U);psram_write(DMA_CFG,0);
        unsigned quiet=0;
        for(unsigned n=0;n<100000U;++n){if(!psram_dma_roots(&before))return 0;
            if(!psram_read(DMA_CFG)&&!psram_read(DMA_CHEN)&&!(psram_read(DMA_CH0+0x34U)&0x7FFFU)){quiet=1;break;}}
        if(!quiet)return 0;
        unsigned matched=0,guards=1;
        for(unsigned j=0;j<PSRAM_DMA_BYTES;++j)if(mem[128U+j]==(uint8_t)(window_value((offset+j)/4U)>>(((offset+j)%4U)*8U)))++matched;
        for(unsigned j=0;j<128U;++j)if(mem[j]!=0xA5||mem[128U+PSRAM_DMA_BYTES+j]!=0xA5)guards=0;
        uint32_t raw=psram_read(DMA_CH0+0x30U),status=psram_read(DMA_CH0+0x88U),common=psram_read(DMA_COMMON_STATUS);
        field("PSRAM_DMA_COPY index=",i);field(" offset=",offset);field(" bytes=",PSRAM_DMA_BYTES);field(" burst=",burst);field(" done=",done);field(" polls=",polls);field(" matched=",matched);field(" guards=",guards);field(" raw=",raw);field(" status=",status);field(" common=",common);field(" us=",elapsed);maia_console_puts("\n");
        if(!done||matched!=PSRAM_DMA_BYTES||!guards||raw!=511U||(status&DMA_FIXTURE_ERRORS)||common)return 0;
    }
    unsigned restored=1;
    for(unsigned n=12U;n>0U;--n){unsigned j=n-1U;if(!psram_dma_roots(&before))return 0;
        psram_write(DMA_CH0+psram_dma_offsets[j],saved[j]);if(psram_read(DMA_CH0+psram_dma_offsets[j])!=saved[j])restored=0;}
    unsigned preserved=mipi_snapshot(&after,psram_dma_read)&&before.cpu==after.cpu&&before.sys==after.sys&&before.phy==after.phy&&before.valid==after.valid&&before.csi_bridge==after.csi_bridge;
    for(unsigned j=0;j<7U;++j)if(before.dsi[j]!=after.dsi[j])preserved=0;
    for(unsigned j=0;j<4U;++j)if(before.csi[j]!=after.csi[j])preserved=0;
    for(unsigned c=1;c<4U;++c)for(unsigned j=0;j<7U;++j)if(before.channel[c][j]!=after.channel[c][j])preserved=0;
    field("PSRAM_DMA_FINAL cfg=",psram_read(DMA_CFG));field(" channels=",psram_read(DMA_CHEN));field(" fifo=",psram_read(DMA_CH0+0x34U));field(" restored=",restored);field(" preserved=",preserved);maia_console_puts("\n");
    return restored&&preserved&&!psram_read(DMA_CFG)&&!psram_read(DMA_CHEN)&&!psram_read(DMA_CH0+0x34U);
}
