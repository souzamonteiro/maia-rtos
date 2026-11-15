/* Private native diagnostic plan/encoder. Original MaiaRTOS MIT. Register facts:
 * ESP-IDF f5c3654 dw_gdma_ll.h LLI layout/setters, dw_gdma_struct.h and types;
 * docs/audits/native-dpi-dma-tile.md. No SDK headers/runtime/allocator or MMIO.
 * Caller owns source/list/output storage exclusively. Address checks describe
 * the diagnostic SRAM window, not allocation proof or production capabilities. */
#ifndef MAIA_DSI_DMA_DESCRIPTOR_H
#define MAIA_DSI_DMA_DESCRIPTOR_H
#include <stdint.h>
#define DSI_DMA_SRAM_LOW 0x4FF00000U
#define DSI_DMA_SRAM_END 0x4FF28000U
#define DSI_DMA_DESTINATION 0x50105000U
#define DSI_DMA_LLI_BYTES 64U
#define DSI_DMA_CTL0 0x001E1B41U
#define DSI_DMA_CTL1 0x00108840U
/* RGB565, bounded geometry, repeated source tile, finite non-circular chain.
 * Source increments, destination fixed DSI/master0, source/list master1;
 * 64-bit width, source512/destination256 burst items, burst len16, no IRQ/status
 * fetch/writeback; each LLI valid, only final LLI last, final LLP exactly0. */
typedef struct dsi_dma_tile_request {
    uint32_t width,height,tile_rows,source,source_capacity,list,list_capacity;
} dsi_dma_tile_request_t;
typedef struct dsi_dma_tile_plan {
    uint32_t source_bytes,count,list_bytes,frame_bytes;
} dsi_dma_tile_plan_t;
static inline int dsi_dma_region(uint32_t base,uint32_t bytes) {
    return !(base&127U) && base>=DSI_DMA_SRAM_LOW && base<DSI_DMA_SRAM_END &&
        bytes && bytes<=DSI_DMA_SRAM_END-base;
}
static inline int dsi_dma_tile_plan(const dsi_dma_tile_request_t *r,dsi_dma_tile_plan_t *p) {
    if (!r || !p) return 0;
    p->source_bytes=p->count=p->list_bytes=p->frame_bytes=0;
    if (!r->width || r->width>4095U || (r->width&3U) || !r->height || r->height>4095U ||
        !r->tile_rows || r->tile_rows>r->height || r->height%r->tile_rows ||
        !dsi_dma_region(r->source,r->source_capacity) || !dsi_dma_region(r->list,r->list_capacity)) return 0;
    /* Bounds above make all geometry/products32-bit safe. Region bounds make
     * end sums safe. Capacities must be disjoint, not only the used portions. */
    uint32_t bytes=r->width*r->tile_rows*2U,count=r->height/r->tile_rows;
    if (bytes>r->source_capacity || count>r->list_capacity/DSI_DMA_LLI_BYTES ||
        (r->source<r->list+r->list_capacity && r->list<r->source+r->source_capacity)) return 0;
    p->source_bytes=bytes;p->count=count;p->list_bytes=count*DSI_DMA_LLI_BYTES;
    p->frame_bytes=r->width*r->height*2U;return 1;
}
/* Validates before writing any output. Output is16 words, caller-aligned/storage
 * guaranteed; volatile permits actual NC SRAM publication without cached access.
 * CPU fences/cache handoff and all hardware ownership belong to the caller. */
static inline int dsi_dma_tile_encode(const dsi_dma_tile_request_t *r,uint32_t index,volatile uint32_t *out) {
    dsi_dma_tile_plan_t p;
    if (!out || !dsi_dma_tile_plan(r,&p) || index>=p.count) return 0;
    for (unsigned i=0;i<16U;++i) out[i]=0;
    out[0]=r->source;out[2]=DSI_DMA_DESTINATION;out[4]=p.source_bytes/8U-1U;
    out[6]=index+1U<p.count ? (r->list+(index+1U)*DSI_DMA_LLI_BYTES)|1U : 0U;
    out[8]=DSI_DMA_CTL0;out[9]=DSI_DMA_CTL1|0x80000000U|(index+1U == p.count ? 0x40000000U : 0U);
    return 1;
}
#endif
