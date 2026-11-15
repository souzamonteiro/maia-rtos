/* Original MaiaRTOS MIT. Private two-block NC SRAM diagnostic, no MMIO.
 * Field sources: SDK f5c3654 dw_gdma_ll.h/registers; TRM chip1.3 v0.4
 * section5.5.4 pp334-336. See docs/audits/native-vdma-list.md.
 * Caller owns full DMA_FIXTURE_TOTAL reservation; not an allocation API. */
#ifndef MAIA_VDMA_LIST_FIXTURE_H
#define MAIA_VDMA_LIST_FIXTURE_H
#include "vdma_transfer_fixture.h"
#define DMA_LIST_TILE_BYTES 11520U
#define DMA_LIST_SOLID_OFFSET 12288U
#define DMA_LIST_OFFSET 16384U
#define DMA_LIST_BYTES 128U
/* Eight native720px rows: red/green/blue/yellow180px bands. */
static inline unsigned dma_list_pixel_byte(unsigned i) {
    static const uint16_t colors[4]={0xF800,0x07E0,0x001F,0xFFE0};
    uint16_t c=i<DMA_LIST_TILE_BYTES?colors[((i/2U)%720U)/180U]:0x8410U;
    return (c>>((i&1U)*8U))&255U;
}
/* Fully validate numeric span before any output mutation. Output32words, caller
 * supplies appropriately aligned writable storage, possibly NC/volatile. */
static inline int dma_list_encode(uint32_t base,volatile uint32_t *out) {
    if(!out || (base&127U) || base<0x4FF00000U || base>0x4FF28000U-DMA_FIXTURE_TOTAL)return 0;
    uint32_t src=base+DMA_FIXTURE_GUARD,dst=base+DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD;
    for(unsigned i=0;i<32U;++i)out[i]=0;
    out[0]=src;out[2]=dst;out[4]=DMA_LIST_TILE_BYTES/8U-1U;
    out[6]=(src+DMA_LIST_OFFSET+64U)|1U;out[8]=0x1B05U;out[9]=0x80000000U;
    out[16]=src+DMA_LIST_SOLID_OFFSET;out[18]=dst+DMA_LIST_TILE_BYTES;
    out[20]=(DMA_FIXTURE_BYTES-DMA_LIST_TILE_BYTES)/8U-1U;
    out[24]=0x1B15U;out[25]=0xC0000040U; /* ARLEN override enabled, field0: qualify one AXI beat */
    return 1;
}
/* LLI is engine-owned writable memory while active. TRM5.5.4 p335:
 * observed status writeback clears VALID and publishes count/done flags even
 * with peripheral SSTAT/DSTAT fetch disabled. Exact expected result, no masks. */
static inline uint32_t dma_list_completed_word(unsigned i,uint32_t before) {
    if(i==9U||i==25U)return before&~0x80000000U;
    if(i==12U)return 1439U;
    if(i==13U)return 0x40000000U;
    if(i==28U)return 2655U;
    if(i==29U)return 0xC0000000U;
    return before;
}
#endif
