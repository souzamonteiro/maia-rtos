/* Actual native snapshot/clock/supply/MPLL/composition; profile device calls
 * stubbed here, real mode transactions covered by psram_array_test separately. */
#include <assert.h>
#include <string.h>
#include "psram_mpll_internal.h"
void maia_psram_clock_test_reset(void);
void maia_psram_supply_test_reset(void);
void maia_psram_mpll_test_reset(void);
void maia_psram_boot_test_reset(void);
void maia_psram_clock_test_claim(void);
static uint32_t roots[9],words[27],extra[6],route[2],control,regs[4],axi;
static unsigned context,reads,writes,drop,mutation,busy,stuck,profile_calls,x8,profile_fail;
static unsigned changed[256];
int maia_psram_test_context(void){return (int)context;}
int maia_psram_x8_claimed(void){return (int)x8;}
static uint32_t *ptr(uintptr_t p){
    for(unsigned i=0;i<9U;++i)if(p==psram_roots[i])return &roots[i];
    const uintptr_t a[6]={MPLL_LPCLK,MPLL_LPRESET,MPLL_POWER,MPLL_CLOCK,MPLL_CAL,MPLL_ANA_CLOCK};
    for(unsigned i=0;i<6U;++i)if(p==a[i])return &extra[i];
    if(p==MPLL_CONF1)return &route[0];if(p==MPLL_CONF2)return &route[1];
    if(p==MPLL_ANA||p==MPLL_ANA+4U)return &control;
    if(p==PSRAM_MSPI0+0x3CU)return &axi;
    assert((roots[1]&0xC000U)==0xC000U); /* no gated controller access */
    for(unsigned i=0;i<27U;++i)if(p==psram_address(i))return &words[i];
    assert(!"unexpected MMIO/pad/array access");return 0;
}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;
    if(p==MPLL_ANA||p==MPLL_ANA+4U){
        assert((extra[0]&0x8000000U)&&!(extra[1]&0x4000000U));
        if(busy&&!stuck){busy=0;unsigned reg=(control>>8U)&255U;
            if(control&0x1000000U){regs[reg]=(control>>16U)&255U;if(reg==2U)extra[4]|=0x100U;}
            control=(control&0x100FFFFU)|(regs[reg]<<16U);}
    }else if(p==MPLL_ANA_CLOCK)assert((extra[0]&0x8000000U)&&!(extra[1]&0x4000000U));
    return *ptr(p);
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
    ++writes;assert(writes<256U);uint32_t *word=ptr(p),old=*word;
    changed[writes]=(old!=v);
    if(writes==drop)return;
    if(p==PSRAM_PERI){
        assert(!((v^old)&~0xC000U));
        if((v&0x8000U)&&!(old&0x8000U))assert((old&0x4000U)&&(roots[4]&0x180U)==0x180U);
        if(!(v&0x4000U))assert(!(old&0x8000U)&&!(v&0x8000U));
    }else if(p==psram_roots[4]||p==psram_roots[5]){
        assert(!(roots[1]&0xC000U)&&!(extra[2]&0x1000000U)&&!(extra[3]&0x10000000U));
        assert(!((v^old)&~(p==psram_roots[4]?0x4F80U:0xF7800000U)));
    }else if(p==MPLL_POWER){
        assert(!((v^old)&~0x1000000U)&&!(roots[1]&0xFF3000U));
        if(!(roots[1]&0x8000U)&&!(v&0x1000000U))assert(!(extra[3]&0x10000000U));
        if(!(v&0x1000000U))extra[4]&=~0x100U;
        else assert(roots[4]&0x100U);
    }else if(p==MPLL_CLOCK){
        assert(!((v^old)&~0x10000000U)&&!(roots[1]&0x8000U));
        if(v&0x10000000U)assert(extra[2]&0x1000000U);
    }else if(p==MPLL_CAL){
        assert(!((v^old)&~0x300U));
        v=(v&~0x100U)|(old&0x100U);if(!(v&0x200U))v&=~0x100U;
    }else if(p==MPLL_ANA_CLOCK){assert(!((v^old)&~1U)&&(extra[0]&0x8000000U)&&!(extra[1]&0x4000000U));
    }else if(p==MPLL_ANA){
        unsigned reg=(v>>8U)&255U;
        assert((extra[0]&0x8000000U)&&!(extra[1]&0x4000000U)&&(extra[2]&0x1000000U)&&(extra[3]&0x10000000U));
        assert(reg>=1U&&reg<=3U&&(v&255U)==0x63U&&route[0]==0xA5000000U&&route[1]==0x5A000200U);
        busy=1;v|=MPLL_BUSY;
    }else if(p==PSRAM_MSPI0+0x50U||p==PSRAM_MSPI1+0x14U){
        assert((roots[1]&0xFFB000U)==0x8000U&&v==0x10001U);
    }else assert(p==MPLL_LPCLK||p==MPLL_LPRESET||p==MPLL_CONF1||p==MPLL_CONF2);
    *word=v;if(writes==mutation)roots[2]^=1U;
}
maia_psram_result_t maia_esp32p4_psram_profile200(unsigned restore,uint32_t budget,maia_psram_profile_report_t *r){
    ++profile_calls;assert(!restore&&budget==10U&&maia_psram_boot_claimed()&&maia_psram_clock_mhz()==20U);
    assert(regs[2]==0x99U&&(extra[4]&0x300U)==0x300U);
    memset(r,0,sizeof *r);r->stage=3;
    return profile_fail?MAIA_PSRAM_IO:MAIA_PSRAM_OK;
}
static void setup(void){
    maia_psram_clock_test_reset();maia_psram_supply_test_reset();maia_psram_mpll_test_reset();maia_psram_boot_test_reset();
    context=1;reads=writes=drop=mutation=busy=stuck=profile_calls=x8=profile_fail=0;
    memset(changed,0,sizeof changed);memset(roots,0,sizeof roots);memset(words,0,sizeof words);
    roots[0]=0xE6DF97AF;roots[1]=0xC00C;roots[2]=0x100;roots[4]=0x40200000;roots[5]=0xA0000000;
    roots[6]=0x7B054313;roots[7]=0x3A184DEA;words[7]=words[20]=0x30103;axi=0x80000000;
    extra[0]=0;extra[1]=0x4000000;extra[2]=0x8000000;extra[3]=0xFFcfffd;extra[4]=0x4C;extra[5]=0xA4;
    route[0]=0xA5123456;route[1]=0x5A654321;control=0;regs[1]=0x31;regs[2]=0x08;regs[3]=0x50;
}
int main(void){
    maia_psram_boot_report_t r;maia_psram_clock_report_t c;
    setup();assert(maia_esp32p4_psram_boot_initialize200(10,0)==MAIA_PSRAM_INVALID&&!reads&&!writes);
    assert(maia_esp32p4_psram_boot_initialize200(0,&r)==MAIA_PSRAM_INVALID&&!reads);
    context=0;assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_CONTEXT&&!reads);
    const uint32_t rejected[10]={
        MAIA_PSRAM_BOOT_GUARD_VO2_ENABLED,MAIA_PSRAM_BOOT_GUARD_DIVIDER_NOT_XTAL,
        MAIA_PSRAM_BOOT_GUARD_EFUSE_INVALID,MAIA_PSRAM_BOOT_GUARD_AXI_OPEN,
        MAIA_PSRAM_BOOT_GUARD_MSPI0_ACTIVE,MAIA_PSRAM_BOOT_GUARD_MSPI0_CLK_CONFIG,
        MAIA_PSRAM_BOOT_GUARD_EFUSE_INVALID,MAIA_PSRAM_BOOT_GUARD_MSPI1_ACTIVE,
        MAIA_PSRAM_BOOT_GUARD_MSPI0_FSM_BUSY,MAIA_PSRAM_BOOT_GUARD_MSPI1_CLK_CONFIG};
    for(unsigned variant=0;variant<10U;++variant){setup();
        switch(variant){case 0:roots[4]|=0x100;break;case 1:roots[1]|=0x1000;break;
        case 2:roots[6]&=0xFFFFFFF;break;case 3:axi|=1;break;case 4:words[0]=1;break;
        case 5:words[7]=0x10001;break;case 6:roots[8]=8;break;case 7:words[16]=1;break;
        case 8:words[2]=1;break;default:words[20]=0x10001;break;}
        assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_BUSY&&!writes&&!profile_calls);
        assert(r.failed_guards==rejected[variant]&&r.entry_snapshot.roots_valid&&
               r.entry_snapshot.controller_samples==54U&&r.entry_axi_valid&&r.entry_axi==axi);
        assert(r.entry_snapshot.roots[4]==roots[4]&&r.entry_snapshot.roots_after[4]==roots[4]);
    }
    setup();roots[1]|=0x1000;roots[4]|=0x100;
    assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_BUSY&&!writes&&
           r.failed_guards==(MAIA_PSRAM_BOOT_GUARD_VO2_ENABLED|
                             MAIA_PSRAM_BOOT_GUARD_DIVIDER_NOT_XTAL));
    setup();maia_psram_clock_test_claim();
    assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_BUSY&&!reads&&
           r.failed_guards==MAIA_PSRAM_BOOT_GUARD_CLOCK_CLAIMED&&!r.entry_snapshot.roots_valid);
    setup();x8=1;
    assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_BUSY&&!reads&&
           r.failed_guards==MAIA_PSRAM_BOOT_GUARD_X8_CLAIMED&&!r.entry_snapshot.roots_valid);
    setup();extra[0]=0x8000000;extra[1]=0;control=MPLL_BUSY;
    assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_BUSY&&
           r.failed_guards==MAIA_PSRAM_BOOT_GUARD_MPLL_BUSY&&r.entry_mpll_valid&&
           r.entry_mpll[0]==MPLL_BUSY&&r.entry_mpll[1]==MPLL_BUSY&&!writes);
    setup();assert(maia_psram_boot20(&r)==MAIA_PSRAM_OK&&r.stage==5&&r.writes==8&&r.supply.writes==3&&extra[5]==0xA5);
    x8=1;unsigned n=reads;maia_psram_fast_report_t f;
    assert(maia_esp32p4_psram_fast_start(200,10,&f)==MAIA_PSRAM_BUSY&&reads==n);
    assert(maia_esp32p4_psram_xtal20_stop(&c)==MAIA_PSRAM_BUSY&&reads==n);
    for(unsigned variant=0;variant<2U;++variant){setup();if(variant){extra[0]=0x8000000;extra[1]=0;extra[2]|=0x1000000;extra[3]|=0x10000000;regs[2]=0x99;}
        assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_OK&&r.stage==7&&profile_calls==1&&r.mpll.mhz==400&&r.mpll.saw_not_done);
        assert(roots[1]==0xC00C&&roots[4]==0x40200180&&roots[5]==0x76800000&&words[7]==0x10001&&words[20]==0x10001);
        assert(route[0]==0xA5123456&&route[1]==0x5A654321);
        n=reads;assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_BUSY&&reads==n&&
                       r.failed_guards==MAIA_PSRAM_BOOT_GUARD_ALREADY_STARTED&&!r.entry_snapshot.roots_valid);
    }
    setup();assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_OK);unsigned total=writes,sensitive[256];memcpy(sensitive,changed,sizeof changed);
    for(unsigned failure=1;failure<=total;++failure){
        if(sensitive[failure]){setup();drop=failure;assert(maia_esp32p4_psram_boot_initialize200(4,&r)!=MAIA_PSRAM_OK&&!profile_calls);
            n=reads;assert(maia_esp32p4_psram_boot_initialize200(4,&r)==MAIA_PSRAM_FAILED&&reads==n);}
        setup();mutation=failure;assert(maia_esp32p4_psram_boot_initialize200(4,&r)!=MAIA_PSRAM_OK&&!profile_calls);
    }
    setup();profile_fail=1;assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_IO&&r.stage==6&&profile_calls==1);
    n=reads;assert(maia_esp32p4_psram_boot_initialize200(10,&r)==MAIA_PSRAM_FAILED&&reads==n&&
                   r.failed_guards==MAIA_PSRAM_BOOT_GUARD_PREVIOUS_FAILURE);
    setup();stuck=1;assert(maia_esp32p4_psram_boot_initialize200(3,&r)==MAIA_PSRAM_TIMEOUT&&!profile_calls);
    return 0;
}
