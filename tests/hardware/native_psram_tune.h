/* Native4-phase training adapted from SDK f5c3654, phase-selection helper
 * retains Apache2. Stronger actual DMA checks instead of accepting byte errors.
 * Warm profile only; this does not implement SDK31-point delayline calibration. */
#include "../../soc/espressif/esp32p4/psram_phase_plan.h"
static int fast_reference(void){
    maia_psram_window_report_t r;
    maia_psram_result_t result=maia_esp32p4_psram_window_start(10000,&r);if(result!=MAIA_PSRAM_OK)return 0;
    volatile uint32_t *p=(volatile uint32_t *)(uintptr_t)r.vaddr;
    for(unsigned i=0;i<MAIA_PSRAM_WINDOW_BYTES/4U;++i)p[i]=window_value(i);
    result=maia_esp32p4_psram_window_sync(10000,&r);if(result!=MAIA_PSRAM_OK)return 0;
    unsigned matched=0;for(unsigned i=0;i<MAIA_PSRAM_WINDOW_BYTES/4U;++i)if(p[i]==window_value(i))++matched;
    result=maia_esp32p4_psram_window_stop(10000,&r);
    field("PSRAM_TUNE_REFERENCE result=",(uint32_t)result);field(" matched=",matched);field(" restored=",r.restored);maia_console_puts("\n");
    return result==MAIA_PSRAM_OK&&r.restored&&matched==MAIA_PSRAM_WINDOW_BYTES/4U;
}
static int fast_tune(void){
    unsigned mask=0;
    for(unsigned phase=0;phase<4U;++phase){
        maia_psram_result_t result=maia_psram_fast_phase(phase);if(result!=MAIA_PSRAM_OK)return 0;
        maia_psram_window_report_t r;result=maia_esp32p4_psram_window_start(10000,&r);if(result!=MAIA_PSRAM_OK)return 0;
        int copied=psram_dma(1);if(copied<0)return 0;
        result=maia_esp32p4_psram_window_stop(10000,&r);
        field("PSRAM_TUNE_PHASE phase=",phase);field(" result=",(uint32_t)result);field(" copies=",psram_dma_passes);field(" matched=",psram_dma_matched);field(" restored=",r.restored);maia_console_puts("\n");
        if(result!=MAIA_PSRAM_OK||!r.restored)return 0;
        if(copied==1)mask|=1U<<phase;
    }
    int best=maia_psram_phase_best(mask);
    field("PSRAM_TUNE_SELECT mask=",mask);field(" best=",(uint32_t)best);maia_console_puts("\n");
    return best>=0&&maia_psram_fast_phase((unsigned)best)==MAIA_PSRAM_OK;
}
