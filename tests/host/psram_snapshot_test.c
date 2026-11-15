#include <assert.h>
#include <string.h>
#include "psram_internal.h"
static unsigned context,reads,controllers,drop_after,drift_word,root_change;
static uint32_t roots[9];
int maia_psram_test_context(void){return (int)context;}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;
    for(unsigned i=0;i<9U;++i)if(p==psram_roots[i]){
        if(i==0U&&drop_after&&controllers>=drop_after)return roots[i]^0x80000000U;
        if(i==4U&&root_change&&controllers==54U)return roots[i]^1U;
        return roots[i];
    }
    assert(psram_accessible(roots));assert(!(drop_after&&controllers>=drop_after));
    /* Independent whitelist/order, no arbitrary peripheral or array reads. */
    static const unsigned off[27]={0,8,12,64,68,72,76,80,216,384,388,392,396,400,512,304,0,8,12,16,20,24,28,32,212,384,512};
    unsigned index=controllers%27U;assert(p==(index<16U?0x5008E000U:0x5008F000U)+off[index]);
    ++controllers;return index+(drift_word==index+1U&&controllers>27U?0x100U:0U);
}
static void setup(void){
    context=1;reads=controllers=drop_after=drift_word=root_change=0;
    for(unsigned i=0;i<9U;++i)roots[i]=i;
    roots[0]=0x80002000U;roots[1]=0xC100U;roots[2]=0;
}
int main(void){
    maia_psram_snapshot_t r;setup();assert(maia_esp32p4_psram_snapshot(0)==MAIA_PSRAM_INVALID&&!reads);
    context=0;memset(&r,0xA5,sizeof r);assert(maia_esp32p4_psram_snapshot(&r)==MAIA_PSRAM_CONTEXT&&!reads&&!r.roots_valid&&!r.controller_samples);
    for(unsigned i=0;i<sizeof r;++i)assert(!((unsigned char *)&r)[i]);
    for(unsigned n=0;n<5U;++n){
        setup();if(n==0)roots[0]&=~0x80000000U;if(n==1)roots[1]&=~0x4000U;if(n==2)roots[1]&=~0x8000U;if(n==3)roots[2]|=0x800000U;if(n==4)roots[2]|=0x2000000U;
        assert(maia_esp32p4_psram_snapshot(&r)==MAIA_PSRAM_GATED&&r.roots_valid&&reads==18U&&!controllers&&!r.controller_samples);
        for(unsigned i=0;i<9;++i)assert(r.roots[i]==roots[i]&&r.roots_after[i]==roots[i]);
    }
    setup();assert(maia_esp32p4_psram_snapshot(&r)==MAIA_PSRAM_OK&&r.roots_valid&&controllers==54U&&r.controller_samples==54U);
    for(unsigned i=0;i<27U;++i)assert(r.before[i]==i&&r.after[i]==i);
    for(unsigned n=1;n<=53U;++n){setup();drop_after=n;assert(maia_esp32p4_psram_snapshot(&r)==MAIA_PSRAM_CHANGED&&controllers==n&&r.controller_samples==n);}
    for(unsigned n=1;n<=27U;++n){setup();drift_word=n;assert(maia_esp32p4_psram_snapshot(&r)==MAIA_PSRAM_CHANGED&&controllers==54U);}
    setup();root_change=1;assert(maia_esp32p4_psram_snapshot(&r)==MAIA_PSRAM_CHANGED&&controllers==54U);
    return 0;
}
