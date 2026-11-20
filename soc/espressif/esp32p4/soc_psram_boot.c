/* Original MaiaRTOS MIT. Boot-lifetime composition of native supply, clock,
 * calibration and AP HEX mode ports; no SDK/FreeRTOS header or runtime. */
#include "psram_internal.h"
static unsigned boot_state;
#ifdef MAIA_PSRAM_TEST
void maia_psram_boot_test_reset(void);
void maia_psram_boot_test_reset(void){boot_state=0;}
#endif
maia_psram_result_t maia_esp32p4_psram_boot_initialize200(uint32_t budget,maia_psram_boot_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(boot_state==1U){r->failed_guards=MAIA_PSRAM_BOOT_GUARD_ALREADY_STARTED;return MAIA_PSRAM_BUSY;}
    if(boot_state==2U){r->failed_guards=MAIA_PSRAM_BOOT_GUARD_PREVIOUS_FAILURE;return MAIA_PSRAM_FAILED;}
    maia_psram_result_t result=maia_psram_boot20(r);if(result!=MAIA_PSRAM_OK)return result;
    boot_state=2;
    result=maia_esp32p4_psram_mpll_initialize400(budget,&r->mpll);if(result!=MAIA_PSRAM_OK)return result;
    r->stage=6;
    result=maia_esp32p4_psram_profile200(0,budget,&r->profile);if(result!=MAIA_PSRAM_OK)return result;
    r->stage=7;boot_state=1;return MAIA_PSRAM_OK;
}
