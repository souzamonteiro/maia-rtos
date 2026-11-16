/* Original MaiaRTOS MIT. Bounded boot-exclusive warm HEX clock qualification.
 * Register facts: pinned ESP-IDF f5c3654 psram_ctrlr_ll, hp_sys_clkrst_reg,
 * spi_mem_s_reg, spi1_mem_s_reg and ldo_ll. No SDK headers/runtime or ROM calls. */
#include "psram_internal.h"
#define CLOCK_SELECT_DIV 0x00FF3000U
#define CLOCK_CORE_ENABLE 0x8000U
enum { CLOCK_FREE,CLOCK_OWNED,CLOCK_FAULTED };
static unsigned clock_state;
static maia_psram_snapshot_t original,current;
#ifdef MAIA_PSRAM_TEST
void maia_psram_clock_test_reset(void);
void maia_psram_clock_test_reset(void){clock_state=CLOCK_FREE;}
#endif
static int roots_match(void){
    for(unsigned i=0;i<9U;++i)if(psram_read(psram_roots[i])!=current.roots[i])return 0;
    return 1;
}
static int guarded(void){
    if(!roots_match())return 0;
    if(!(current.roots[1]&CLOCK_CORE_ENABLE))return 1; /* no controller access */
    if(!psram_axi_quiet())return 0;
    maia_psram_snapshot_t s;
    if(maia_esp32p4_psram_snapshot(&s)!=MAIA_PSRAM_OK)return 0;
    for(unsigned i=0;i<9U;++i)if(s.roots[i]!=current.roots[i])return 0;
    for(unsigned i=0;i<27U;++i)if(s.before[i]!=current.before[i])return 0;
    return 1;
}
static void evidence(maia_psram_clock_report_t *r){
    r->before[0]=original.roots[1];r->before[1]=original.before[7];r->before[2]=original.before[20];
    r->after[0]=current.roots[1];r->after[1]=current.before[7];r->after[2]=current.before[20];
}
static int change(unsigned step,unsigned word,uint32_t value,maia_psram_clock_report_t *r){
    if(!guarded())return 0;
    uintptr_t address=word==0U?PSRAM_PERI:word==1U?PSRAM_MSPI0+0x50U:PSRAM_MSPI1+0x14U;
    r->attempts|=1U<<step;
    psram_write(address,value);
    if(word!=0U&&!roots_match())return 0; /* no gated controller readback */
    if(psram_read(address)!=value)return 0;
    if(word==0U)current.roots[1]=value;else current.before[word==1U?7U:20U]=value;
    evidence(r);
    if(!guarded())return 0;
    r->stage=step+1U;return 1;
}
static int calibrated_supply(const maia_psram_snapshot_t *s){
    uint32_t sys2=s->roots[6],sys3=s->roots[7];
    unsigned d=sys2>>28U,m=(sys3>>3U)&7U;
    /* Exact rev1.3/eFuse layout already qualified by native LDO3 work. VO2's
     * dedicated nonzero pair is the SDK's1900mV calibration override. */
    return ((sys2>>4U)&3U)==1U&&(sys2&15U)==3U&&!(sys2&0xC0U)&&
        !((sys2>>11U)&3U)&&((sys2>>8U)&7U)&&!(s->roots[8]&8U)&&d&&m&&
        (s->roots[4]&0x4F80U)==0x180U&&((s->roots[5]>>28U)&15U)==d&&((s->roots[5]>>23U)&7U)==m;
}
static maia_psram_result_t initialize_report(maia_psram_clock_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    return clock_state==CLOCK_FAULTED?MAIA_PSRAM_FAILED:MAIA_PSRAM_OK;
}
maia_psram_result_t maia_esp32p4_psram_xtal20_start(maia_psram_clock_report_t *r){
    maia_psram_result_t result=initialize_report(r);if(result!=MAIA_PSRAM_OK)return result;
    if(clock_state!=CLOCK_FREE)return MAIA_PSRAM_BUSY;
    maia_psram_snapshot_t s;result=maia_esp32p4_psram_snapshot(&s);if(result!=MAIA_PSRAM_OK)return result;
    if(s.before[0]||s.before[16]||!psram_axi_quiet()||(s.before[2]&1U)||!calibrated_supply(&s)||
       (s.roots[1]&CLOCK_SELECT_DIV)!=0x1000U||s.before[7]!=0x30103U||s.before[20]!=0x30103U)return MAIA_PSRAM_BUSY;
    original=current=s;evidence(r);if(!guarded())return MAIA_PSRAM_CHANGED;
    clock_state=CLOCK_FAULTED;
    uint32_t peri=s.roots[1];
    if(!change(0,0,peri&~CLOCK_CORE_ENABLE,r)||
       !change(1,0,peri&~(CLOCK_CORE_ENABLE|CLOCK_SELECT_DIV),r)||
       !change(2,0,peri&~CLOCK_SELECT_DIV,r)||
       !change(3,1,0x10001U,r)||!change(4,2,0x10001U,r))return MAIA_PSRAM_VERIFY;
    clock_state=CLOCK_OWNED;return MAIA_PSRAM_OK;
}
maia_psram_result_t maia_esp32p4_psram_xtal20_stop(maia_psram_clock_report_t *r){
    maia_psram_result_t result=initialize_report(r);if(result!=MAIA_PSRAM_OK)return result;
    if(clock_state!=CLOCK_OWNED)return MAIA_PSRAM_BUSY;
    evidence(r);clock_state=CLOCK_FAULTED;
    if(!change(0,2,original.before[20],r)||!change(1,1,original.before[7],r)||
       !change(2,0,current.roots[1]&~CLOCK_CORE_ENABLE,r)||
       !change(3,0,original.roots[1]&~CLOCK_CORE_ENABLE,r)||
       !change(4,0,original.roots[1],r))return MAIA_PSRAM_VERIFY;
    r->restored=1;clock_state=CLOCK_FREE;return MAIA_PSRAM_OK;
}
