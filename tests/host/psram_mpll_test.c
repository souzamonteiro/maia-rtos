#include <assert.h>
#include "psram_mpll_internal.h"
#include "psram_phase_plan.h"
void maia_psram_mpll_test_reset(void);
static uint32_t root[5],route[2],control;
static unsigned context,reads,writes,commands,drop,stuck,loss,mismatch,delay;
int maia_psram_test_context(void){return (int)context;}
static uint32_t *ptr(uintptr_t p){
    const uintptr_t roots[]={MPLL_LPCLK,MPLL_LPRESET,MPLL_POWER,MPLL_CLOCK,MPLL_CAL};
    for(unsigned i=0;i<5;++i)if(p==roots[i])return &root[i];
    assert((root[0]&0x8000000U)&&!(root[1]&0x4000000U));
    if(p==MPLL_CONF1)return &route[0];if(p==MPLL_CONF2)return &route[1];
    assert(p==MPLL_ANA||p==MPLL_ANA+4U);return &control;
}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;
    if(p==MPLL_ANA&&commands&&(control&MPLL_BUSY)&&!stuck){if(delay)--delay;else control=0x263U|((mismatch&&commands==2U?0x91U:0x99U)<<16U);}
    return *ptr(p);
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
    ++writes;
    if(p==MPLL_ANA){assert(v==0x263U&&route[0]==0xA5000000U&&route[1]==0x5A000200U);++commands;control=v|MPLL_BUSY;delay=2;}
    else{assert(p==MPLL_CONF1||p==MPLL_CONF2);if(writes!=drop)*ptr(p)=v;}
    if(writes==loss)root[0]=0;
}
static void setup(void){
    maia_psram_mpll_test_reset();context=1;reads=writes=commands=drop=stuck=loss=mismatch=delay=0;
    root[0]=0x8000000U;root[1]=0;root[2]=0x1000000U;root[3]=0x10000000U;root[4]=0x300U;
    route[0]=0xA5123456U;route[1]=0x5A654321U;control=0;
}
int main(void){
    static const int best[16]={-1,0,1,0,2,0,1,0,3,0,1,0,2,2,1,0};
    for(unsigned i=0;i<16U;++i)assert(maia_psram_phase_best(i)==best[i]);
    assert(maia_psram_phase_best(16)==-1);
    maia_psram_mpll_report_t r;
    setup();assert(maia_psram_mpll_read(0,&r)==MAIA_PSRAM_INVALID&&!reads&&!writes);
    context=0;assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_CONTEXT&&!reads&&!writes);
    for(unsigned i=0;i<5;++i){setup();root[i]^=i==0?0x8000000U:i==1?0x4000000U:i==2?0x1000000U:i==3?0x10000000U:0x100U;assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_BUSY&&!writes);}
    setup();control=MPLL_BUSY;assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_BUSY&&!writes);
    for(unsigned cycle=0;cycle<2;++cycle){setup();assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_OK&&r.stage==3&&r.restored&&r.div==0x99&&r.repeat==0x99&&r.mhz==400&&r.polls==8&&commands==2&&writes==7);assert(route[0]==0xA5123456U&&route[1]==0x5A654321U);}
    for(unsigned failure=1;failure<=7;++failure){if(failure==4||failure==5)continue;setup();drop=failure;
        assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_VERIFY&&writes==failure);
        unsigned n=reads;assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_FAILED&&reads==n&&writes==failure);
    }
    setup();stuck=1;assert(maia_psram_mpll_read(5,&r)==MAIA_PSRAM_TIMEOUT&&r.polls==6&&!r.restored&&commands==1);
    unsigned n=reads;assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_FAILED&&reads==n);
    setup();loss=3;assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_VERIFY&&commands==0);
    setup();mismatch=1;assert(maia_psram_mpll_read(100,&r)==MAIA_PSRAM_VERIFY&&r.restored&&r.mhz==0);
    return 0;
}
