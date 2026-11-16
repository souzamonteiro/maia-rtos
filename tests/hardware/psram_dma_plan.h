/* Original MaiaRTOS MIT. Boot diagnostic only, pinned SDK f5c3654 dw_gdma
 * master selection, width/burst/ARLEN and register-layout facts. No MMIO. */
#ifndef MAIA_PSRAM_DMA_PLAN_H
#define MAIA_PSRAM_DMA_PLAN_H
#include <stdint.h>
#define PSRAM_DMA_BYTES 4096U
#define PSRAM_DMA_TOTAL 4352U
static inline int psram_dma_plan(uint32_t source,uint32_t destination,unsigned burst,uint32_t out[12]){
    if(!out||(source&7U)||source<0x4B800000U||source>0x4BA00000U-PSRAM_DMA_BYTES||
       (destination&127U)||destination<0x4FF00080U||destination>0x4FF27F80U-PSRAM_DMA_BYTES||
       (burst!=1U&&burst!=4U&&burst!=16U))return 0;
    unsigned size=burst==1U?0U:burst==4U?1U:3U;
    /* Both memory master1, increment/64bit; explicit ARLEN_EN even for field0.
     * Destination single request; software M2M, contiguous, no LLP/IRQ. */
    const uint32_t values[12]={source,0,destination,0,511,0x1B05U|(size<<14U),0x40U|((burst-1U)<<7U),0,0x18U,0,0,0};
    for(unsigned i=0;i<12U;++i)out[i]=values[i];
    return 1;
}
#endif
