/* Native sequencing from pinned ESP-IDF f5c3654 interrupt matrix/CLIC facts.
 * Original MaiaRTOS MIT. No SDK headers/runtime, no generalized IRQ allocator. */
#include "dw_dma_irq.h"
#include "psram_internal.h"
enum { FREE,OWNED,FAULTED };
static unsigned state,claim_stage,selected_line;
uint32_t maia_dw_irq_stage(void){return claim_stage;}
static uint32_t saved_map,saved_ctrl,owned_map,owned_ctrl;
uint32_t maia_dw_irq_line(void){return selected_line;}
uint32_t maia_dw_irq_saved_ctrl(void){return saved_ctrl;}
static maia_dw_irq_callback_t callback;
static void *callback_arg;
#define ROUTE MAIA_ESP32P4_INTR_CORE0_MAP_REG(MAIA_DW_IRQ_SOURCE)
#define CONTROL MAIA_ESP32P4_CLIC_INT_CTRL_REG(selected_line)
#ifdef MAIA_PSRAM_TEST
void maia_dw_irq_test_reset(void);
void maia_dw_irq_test_reset(void){state=FREE;claim_stage=selected_line=0;callback=0;}
#endif
static int idle(void){
    if(!(psram_read(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)&0x2000U)||!(psram_read(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG)&0x20U))return 0;
    return !psram_read(MAIA_ESP32P4_GDMA_BASE+0x10U)&&!psram_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&&!psram_read(MAIA_ESP32P4_GDMA_BASE+0x1CU);
}
static int routes_free(unsigned line){
    for(unsigned i=0;i<128U;++i)if(i!=MAIA_DW_IRQ_SOURCE&&(psram_read(MAIA_ESP32P4_INTR_CORE0_MAP_REG(i))&63U)==line)return 0;
    return 1;
}
int maia_dw_irq_claim(maia_dw_irq_callback_t fn,void *arg){
    claim_stage=1;
    if(!fn||!psram_context()||state!=FREE)return 0;
    claim_stage=2;if(!idle())return 0;
    saved_map=psram_read(ROUTE);unsigned found=0;claim_stage=3;
    for(unsigned line=MAIA_DW_IRQ_FIRST;line<MAIA_DW_IRQ_LIMIT;++line){
        uint32_t ctrl=psram_read(MAIA_ESP32P4_CLIC_INT_CTRL_REG(line));
        if(!(ctrl&0x101U)&&routes_free(line)){selected_line=line;saved_ctrl=ctrl;found=1;break;}
    }
    if(!found)return 0;
    state=FAULTED;callback=fn;callback_arg=arg;
    owned_map=(saved_map&~63U)|selected_line;
    owned_ctrl=(saved_ctrl&~0xFFC70100U)|0x3FC00100U;
    claim_stage=4;psram_write(ROUTE,owned_map);
    if(psram_read(ROUTE)!=owned_map||psram_read(CONTROL)!=saved_ctrl||!idle()||!routes_free(selected_line))return 0;
    claim_stage=5;psram_write(CONTROL,owned_ctrl);
    if(psram_read(ROUTE)!=owned_map||(psram_read(CONTROL)&~1U)!=(owned_ctrl&~1U)||!idle())return 0;
    claim_stage=6;state=OWNED;return 1;
}
int maia_dw_irq_release(void){
    if(!psram_context()||state!=OWNED||!idle()||psram_read(ROUTE)!=owned_map||(psram_read(CONTROL)&~1U)!=(owned_ctrl&~1U))return 0;
    state=FAULTED;
    psram_write(CONTROL,owned_ctrl&~0x100U);
    if(psram_read(CONTROL)&0x100U)return 0;
    psram_write(ROUTE,saved_map);if(psram_read(ROUTE)!=saved_map||!idle())return 0;
    psram_write(CONTROL,saved_ctrl);
    if((psram_read(CONTROL)&~1U)!=(saved_ctrl&~1U))return 0;
    callback=0;state=FREE;return 1;
}
int maia_soc_external_irq_dispatch(uint32_t id){
    if(id!=selected_line||state!=OWNED||!callback||!psram_context())return 0;
    callback(callback_arg);return 1;
}
