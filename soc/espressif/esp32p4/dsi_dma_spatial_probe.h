/* Original MIT, private boot-only diagnostic, not a production driver API.
 * Primary facts: native-vdma-list.md and native-dpi-dma-spatial.md.
 * Own source DSI_SPATIAL_SOURCE_BYTES/list320bytes exclusively,128-byte aligned physical SRAM;
 * disjoint spans inside tested window, cache handed off, CPU uses NC alias.
 * Source immutable720x8 RGB565, or720x32 in central profile. DMA owns writable LLIs until drain. No IRQ/SMP/
 * heap/cache management in this interface. Call once/core0/IRQs off after bridge.
 * Count1..400/budget1..100000; failed stage/frame/rearm sticky until reboot.
 * Report initialized; returns native access result; separate cleanup status.
 * Expected source: four180px color bands, reused for large solid bands.
 * No visual/streaming/WCET guarantee. Entire operation blocks with IRQs off. */
#ifndef MAIA_DSI_DMA_SPATIAL_PROBE_H
#define MAIA_DSI_DMA_SPATIAL_PROBE_H
#include <maia/soc/esp32p4_dsi_access.h>
#define DSI_SPATIAL_NODES 5U
#ifdef MAIA_DSI_SPATIAL_CENTER
#define DSI_SPATIAL_SOURCE_BYTES 46080U
#define DSI_SPATIAL_INCREMENT_NODE 2U
#else
#define DSI_SPATIAL_SOURCE_BYTES 11520U
#define DSI_SPATIAL_INCREMENT_NODE 0U
#endif
#define DSI_SPATIAL_LIST_BYTES 320U
static inline int dsi_spatial_layout(uint32_t source,uint32_t list){
    return !(source&127U)&&!(list&127U)&&source>=0x4FF00000U&&source<=0x4FF28000U-DSI_SPATIAL_SOURCE_BYTES&&
        list>=0x4FF00000U&&list<=0x4FF28000U-320U&&
        (source+DSI_SPATIAL_SOURCE_BYTES<=list||list+320U<=source);
}
static inline uint32_t dsi_spatial_word(uint32_t source,uint32_t list,unsigned node,unsigned word){
#ifdef MAIA_DSI_SPATIAL_CENTER
    static const uint32_t counts[5]={57600,54720,5760,54720,57600};
    static const uint32_t offsets[5]={0,360,0,720,1080};
#else
    static const uint32_t counts[5]={1440,56160,57600,57600,57600};
    static const uint32_t offsets[5]={0,0,360,720,1080};
#endif
    if(node>=5U||word>=16U)return 0;
    if(word==0U)return source+offsets[node];
    if(word==2U)return 0x50105000U;
    if(word==4U)return counts[node]-1U;
    if(word==6U)return node<4U?(list+(node+1U)*64U)|1U:0U;
    if(word==8U)return node==DSI_SPATIAL_INCREMENT_NODE?0x001C1B41U:0x001C1B51U;
    if(word==9U)return 0x800F8040U|(node==4U?0x40000000U:0U);
    return 0;
}
static inline uint32_t dsi_spatial_done_word(uint32_t source,uint32_t list,unsigned n,unsigned w){
    uint32_t v=dsi_spatial_word(source,list,n,w);
    if(w==9U)return v&~0x80000000U;
    if(w==12U)return dsi_spatial_word(source,list,n,4);
    if(w==13U)return n==4U?0xC0000000U:0x40000000U;
    return v;
}
typedef struct maia_dsi_spatial_probe_report {
    maia_dsi_dma_stage_report_t stage;
    maia_dsi_dma_refresh_report_t refresh;
} maia_dsi_spatial_probe_report_t;
#ifdef __cplusplus
extern "C" {
#endif
maia_dsi_access_result_t maia_dsi_spatial_probe(uint32_t source,uint32_t list,uint32_t frames,uint32_t budget,maia_dsi_spatial_probe_report_t *);
#ifdef __cplusplus
}
#endif
#endif
