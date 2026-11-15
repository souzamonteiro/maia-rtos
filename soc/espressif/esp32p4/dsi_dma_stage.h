/* Original native MIT policy. Field facts: pinned SDK f5c3654 dw_gdma LL/
 * register types and DSI bridge LL. docs/audits/native-dpi-dma-stage.md.
 * No SDK runtime/headers/control flow. No engine activation or WT publication. */
static const uint16_t dma_stage_offsets[MAIA_DSI_DMA_STAGE_WORDS]={
    0,4,8,12,0x10,0x18,0x1C,0x20,0x24,0x28,0x2C,0x90,0x88,0x6C,0x8C,8};
static const uint32_t dma_stage_masks[MAIA_DSI_DMA_STAGE_WORDS]={
    0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0xFFFFFFFFU,0x3FFFFFU,0x7FFFFF55U,0xC7FFFFFFU,
    15,0x7F8E319FU,0xFFFFFFC1U,0xFFFFFFFFU,
    0xF83F7FFBU,0xF1,0x10000000U,0x7FF,0xFFF};
static uint32_t dma_staged_words[MAIA_DSI_DMA_STAGE_WORDS],dma_staged_source;
#ifdef MAIA_DSI_SPATIAL_PROBE
#include "dsi_dma_spatial_probe.h"
static int spatial_memory(unsigned done){
    for(unsigned n=0;n<5U;++n)for(unsigned w=0;w<16U;++w){
        uint32_t expected=done?dsi_spatial_done_word(dma_staged_source,dma_staged_list,n,w):dsi_spatial_word(dma_staged_source,dma_staged_list,n,w);
        if(dsi_read((uintptr_t)dma_staged_list+0x40000000U+n*64U+w*4U)!=expected)return 0;
    }
    return 1;
}
#endif
static uintptr_t dma_stage_address(unsigned i) {
    return (i<12U ? MAIA_ESP32P4_GDMA_BASE+0x100U : MAIA_ESP32P4_DSI_BRG_BASE)+dma_stage_offsets[i];
}
static maia_dsi_access_result_t dma_stage_check(uint32_t bridge,const uint32_t *expected,maia_dsi_dma_stage_report_t *r) {
    maia_dsi_dpi_bridge_report_t check;
    check.irq_raw=0;check.observation=r->observation;
    maia_dsi_access_result_t result=bridge_check(bridge,&check);r->observation=check.observation;
    if(result!=MAIA_DSI_ACCESS_OK)return result;
    if(dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U) || dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U) ||
        dsi_read(MAIA_ESP32P4_GDMA_BASE+0x1CU) || (dsi_read(MAIA_ESP32P4_GDMA_BASE+0x58U)&1U))return MAIA_DSI_ACCESS_BUSY;
    r->channel_status|=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x188U);
    r->common_status|=dsi_read(MAIA_ESP32P4_GDMA_BASE+0x50U);
    if(r->channel_status || r->common_status)return MAIA_DSI_ACCESS_IO;
    if(expected)for(unsigned i=0;i<MAIA_DSI_DMA_STAGE_WORDS;++i)
        if(dsi_read(dma_stage_address(i))!=expected[i])return MAIA_DSI_ACCESS_VERIFY;
    return MAIA_DSI_ACCESS_OK;
}
static maia_dsi_access_result_t dma_stage_body(uint32_t source,uint32_t list,maia_dsi_dma_stage_report_t *r) {
    (void)list;
    if(!r)return MAIA_DSI_ACCESS_INVALID;
    r->completed=r->attempted_mask=r->source=r->frame_bytes=r->channel_status=r->common_status=0;
    maia_dsi_short_report_t *io=&r->observation;
    io->completed=io->header=io->header_attempted=io->command_before=io->command_after=0;
    io->polls=io->status=io->fifo=io->error0=io->error1=io->saw_busy=0;
    for(unsigned i=0;i<MAIA_DSI_DMA_STAGE_WORDS;++i)r->before[i]=r->after[i]=0;
    if((source&127U) || source<0x4FF00000U || source>0x4FF27F80U)return MAIA_DSI_ACCESS_INVALID;
#ifdef MAIA_DSI_SPATIAL_PROBE
    if(list && !dsi_spatial_layout(source,list))return MAIA_DSI_ACCESS_INVALID;
#endif
    if(!dsi_context())return MAIA_DSI_ACCESS_CONTEXT;
    if(dpi_dma_state==FAULTED || transfer_state==FAULTED || dpi_bridge_state==FAULTED)return MAIA_DSI_ACCESS_FAILED;
    if(dpi_dma_state!=UNCLAIMED || transfer_state!=UNCLAIMED || dpi_bridge_state!=OWNED)return MAIA_DSI_ACCESS_BUSY;
    if(!dpi_guard(&dpi_staged_guard,dpi_staged_phy,dpi_staged_mode,dpi_staged_words))return MAIA_DSI_ACCESS_VERIFY;
    uint32_t width=dpi_staged_words[4]&0x3FFFU,height=dpi_staged_words[13]&0x3FFFU;
    uint32_t bytes=width*height*2U;
    if(!width || width>4095U || !height || height>4095U || (bytes&7U))return MAIA_DSI_ACCESS_VERIFY;
    uint32_t bridge=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U);
    maia_dsi_access_result_t result=dma_stage_check(bridge,0,r);
    if(result!=MAIA_DSI_ACCESS_OK){if(result==MAIA_DSI_ACCESS_IO)dpi_dma_state=transfer_state=FAULTED;return result;}
    for(unsigned i=0;i<MAIA_DSI_DMA_STAGE_WORDS;++i)r->before[i]=r->after[i]=dsi_read(dma_stage_address(i));
    result=dma_stage_check(bridge,r->after,r);if(result!=MAIA_DSI_ACCESS_OK)return result;
    uint32_t values[MAIA_DSI_DMA_STAGE_WORDS]={source,0,0x50105000U,0,bytes/8U-1U,0x001C1B51U,0x000F8000U,
        0,0x0A020001U,0,0,0,0x10,0,768,256};
#ifdef MAIA_DSI_SPATIAL_PROBE
    if(list){
        if(width!=720U||height!=1280U)return MAIA_DSI_ACCESS_VERIFY;
        dma_staged_source=source;dma_staged_list=list;
        if(!spatial_memory(0)){dma_staged_list=0;return MAIA_DSI_ACCESS_VERIFY;}
        values[5]=dsi_spatial_word(source,list,0,8);values[6]=0x000F8040U;values[7]=15U;values[9]=list|1U;
        values[12]=0x50U;values[13]=0x10000000U;
    }else dma_staged_list=0;
#endif
    r->source=source;r->frame_bytes=bytes;r->completed=1;dpi_dma_state=transfer_state=FAULTED;
    for(unsigned i=0;i<MAIA_DSI_DMA_STAGE_WORDS;++i){
        result=dma_stage_check(bridge,r->after,r);if(result!=MAIA_DSI_ACCESS_OK)return result;
        uint32_t desired=(r->after[i]&~dma_stage_masks[i])|values[i];
        if(desired!=r->after[i]){r->attempted_mask|=1U<<i;dsi_write(dma_stage_address(i),desired);}
        if(!transfer_bus_guard(&dpi_staged_guard))return MAIA_DSI_ACCESS_VERIFY;
        r->after[i]=dsi_read(dma_stage_address(i));
        if(r->after[i]!=desired)return MAIA_DSI_ACCESS_VERIFY;
        r->completed=i+2U;
    }
    result=dma_stage_check(bridge,r->after,r);if(result!=MAIA_DSI_ACCESS_OK)return result;
    for(unsigned i=0;i<MAIA_DSI_DMA_STAGE_WORDS;++i)dma_staged_words[i]=r->after[i];
    dma_staged_source=source;r->completed=18;dpi_dma_state=OWNED;transfer_state=UNCLAIMED;
    return MAIA_DSI_ACCESS_OK;
}

maia_dsi_access_result_t maia_esp32p4_dsi_dma_solid_stage(uint32_t source,maia_dsi_dma_stage_report_t *r){
    return dma_stage_body(source,0,r);
}
