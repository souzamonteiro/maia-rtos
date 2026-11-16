#include <assert.h>
#include <string.h>
#include "psram_mpll_internal.h"
void maia_psram_clock_test_reset(void);
static uint32_t roots[9],words[27],original[9],power,output;
static unsigned context,reads,writes,drop,x8,mutate;
int maia_psram_test_context(void){return (int)context;}
int maia_psram_x8_claimed(void){return (int)x8;}
maia_psram_result_t maia_psram_mpll_read(uint32_t n,maia_psram_mpll_report_t *r){(void)n;(void)r;assert(!"unexpected MPLL read");return MAIA_PSRAM_BUSY;}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;
    for(unsigned i=0;i<9U;++i)if(p==psram_roots[i])return roots[i];
    if(p==MPLL_POWER)return power;if(p==MPLL_CLOCK)return output;
    assert((roots[1]&0xC000U)==0xC000U); /* every gated controller read is a failure */
    if(p==PSRAM_MSPI0+0x3CU)return 0x80000000U;
    for(unsigned i=0;i<27U;++i)if(p==psram_address(i))return words[i];
    assert(!"unexpected MMIO read");return 0;
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
    ++writes;
    if(p==PSRAM_PERI){
        assert((v&~0xFFF000U)==(roots[1]&~0xFFF000U));
        if(v&0x8000U)assert(v&0x4000U);
        if((v&0x8000U)&&!(roots[1]&0x8000U))assert(roots[1]&0x4000U);
        if((v&0xC000U)>(roots[1]&0xC000U))assert((power&0x1000000U)&&(output&0x10000000U));
        if(writes!=drop)roots[1]=v;
    }else if(p==MPLL_POWER||p==MPLL_CLOCK){
        assert(!(roots[1]&0xC000U));
        if(p==MPLL_POWER){assert(!(output&0x10000000U));if(writes!=drop)power=v;}
        else {assert(power&0x1000000U);if(writes!=drop)output=v;}
    }else{
        assert((roots[1]&0xC000U)==0xC000U);
        assert(p==PSRAM_MSPI0+0x50U||p==PSRAM_MSPI1+0x14U);
        if(writes!=drop)words[p==PSRAM_MSPI0+0x50U?7:20]=v;
    }
    if(writes==mutate)roots[4]^=1U;
}
static void setup(void){
    maia_psram_clock_test_reset();context=1;reads=writes=drop=x8=mutate=0;
    memset(roots,0,sizeof roots);memset(words,0,sizeof words);
    roots[0]=0xE6DFB7FF;roots[1]=0xD00C;roots[2]=0x38000040;
    roots[4]=0x40200180;roots[5]=0x76800000;roots[6]=0x7B054313;roots[7]=0x3A184DEA;
    words[7]=words[20]=0x30103;power=0xA1000001;output=0xB0000001;
    memcpy(original,roots,sizeof roots);
}
int main(void){
    maia_psram_park_report_t p;maia_psram_clock_report_t c;
    setup();assert(maia_esp32p4_psram_consumers_park(0,0)==MAIA_PSRAM_INVALID&&!reads);
    assert(maia_esp32p4_psram_consumers_park(2,&p)==MAIA_PSRAM_INVALID&&!reads);
    context=0;assert(maia_esp32p4_psram_consumers_park(0,&p)==MAIA_PSRAM_CONTEXT&&!reads);
    context=1;assert(maia_esp32p4_psram_consumers_park(0,&p)==MAIA_PSRAM_BUSY&&!reads);
    for(unsigned cycle=0;cycle<2U;++cycle){
        setup();assert(maia_esp32p4_psram_xtal20_start(&c)==MAIA_PSRAM_OK);
        unsigned old=reads;assert(maia_esp32p4_psram_consumers_park(1,&p)==MAIA_PSRAM_BUSY&&reads==old);
        x8=1;assert(maia_esp32p4_psram_consumers_park(0,&p)==MAIA_PSRAM_BUSY&&reads==old);x8=0;
        assert(maia_esp32p4_psram_consumers_park(0,&p)==MAIA_PSRAM_OK&&p.stage==4&&p.writes==4&&!p.restored);
        assert((p.before[0]^p.after[0])==0xC000U&&(p.before[1]^p.after[1])==0x1000000U&&(p.before[2]^p.after[2])==0x10000000U);
        assert(!maia_psram_clock_mhz()&&maia_psram_clock_claimed()&&!(roots[1]&0xC000U)&&!(power&0x1000000U)&&!(output&0x10000000U));
        old=reads;assert(maia_esp32p4_psram_consumers_park(0,&p)==MAIA_PSRAM_BUSY&&reads==old);
        assert(maia_esp32p4_psram_xtal20_stop(&c)==MAIA_PSRAM_BUSY&&reads==old);
        maia_psram_fast_report_t f;assert(maia_esp32p4_psram_fast_start(200,10000,&f)==MAIA_PSRAM_BUSY&&reads==old);
        assert(maia_esp32p4_psram_consumers_park(1,&p)==MAIA_PSRAM_OK&&p.stage==4&&p.writes==4&&p.restored);
        assert(maia_psram_clock_mhz()==20&&power==0xA1000001&&output==0xB0000001);
        assert(maia_esp32p4_psram_xtal20_stop(&c)==MAIA_PSRAM_OK&&!memcmp(roots,original,sizeof roots));
    }
    for(unsigned mode=0;mode<2U;++mode)for(unsigned failure=6;failure<=13U;++failure){
        setup();assert(maia_esp32p4_psram_xtal20_start(&c)==MAIA_PSRAM_OK);
        if(mode)mutate=failure;else drop=failure;
        if(failure<=9U)assert(maia_esp32p4_psram_consumers_park(0,&p)==MAIA_PSRAM_VERIFY);
        else{assert(maia_esp32p4_psram_consumers_park(0,&p)==MAIA_PSRAM_OK);assert(maia_esp32p4_psram_consumers_park(1,&p)==MAIA_PSRAM_VERIFY);}
        assert(writes==failure);unsigned old=reads;
        assert(maia_esp32p4_psram_consumers_park(0,&p)==MAIA_PSRAM_FAILED&&reads==old);
        assert(maia_esp32p4_psram_consumers_park(1,&p)==MAIA_PSRAM_FAILED&&reads==old);
        assert(maia_esp32p4_psram_xtal20_stop(&c)==MAIA_PSRAM_FAILED&&reads==old);
    }
    return 0;
}
