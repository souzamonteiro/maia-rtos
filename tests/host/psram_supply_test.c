#include <assert.h>
#include <string.h>
#include "psram_mpll_internal.h"
void maia_psram_supply_test_reset(void);
static const uintptr_t addresses[8]={MAIA_ESP32P4_EFUSE_MAC_SYS2_REG,
    MAIA_ESP32P4_EFUSE_MAC_SYS3_REG,MAIA_ESP32P4_EFUSE_RS_ERR0_REG,
    PSRAM_PERI,MPLL_POWER,MPLL_CLOCK,MAIA_ESP32P4_PMU_BASE+0x1D0U,MAIA_ESP32P4_PMU_BASE+0x1D4U};
static uint32_t words[8],original[8];
static unsigned context,claim,reads,writes,drop,mutate,mutate_read;
int maia_psram_test_context(void){return (int)context;}
int maia_psram_clock_claimed(void){return (int)claim;}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;if(reads==mutate_read)words[3]^=1U;
    for(unsigned i=0;i<8U;++i)if(p==addresses[i])return words[i];
    assert(!"unexpected/gated controller read");return 0;
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
    assert(p==addresses[6]||p==addresses[7]);
    assert(!(words[3]&0xC000U)&&!(words[4]&0x1000000U)&&!(words[5]&0x10000000U));
    ++writes;assert(writes<=3U);
    if(writes!=drop)words[p==addresses[6]?6:7]=v;
    if(writes==mutate)words[3]^=1U;
}
static void setup(unsigned active){
    maia_psram_supply_test_reset();context=1;claim=reads=writes=drop=mutate=mutate_read=0;
    words[0]=0x7B054313;words[1]=0x3A184DEA;words[2]=0;
    words[3]=active?0xD00C:0x100C;words[4]=active?0x1000000:0;words[5]=active?0x10000000:0;
    words[6]=active?0x40200180:0x40204E00;words[7]=active?0x76800000:0x88001234;
    memcpy(original,words,sizeof words);
}
static void sticky(void){
    maia_psram_supply_report_t r;unsigned old=reads;
    assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_FAILED&&reads==old);
}
int main(void){
    maia_psram_supply_report_t r;
    setup(1);assert(maia_esp32p4_psram_supply1900(0)==MAIA_PSRAM_INVALID&&!reads&&!writes);
    context=0;assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_CONTEXT&&!reads&&!writes);
    context=1;claim=1;assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_BUSY&&!reads&&!writes);
    setup(1);assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_OK&&r.stage==4&&r.adopted&&!r.writes);
    assert(r.dref==7&&r.mul==5&&!writes&&!memcmp(words,original,sizeof words));
    unsigned old=reads;assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_BUSY&&reads==old);
    setup(0);mutate_read=9;assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_CHANGED&&!writes);
    for(unsigned bit=0;bit<32U;++bit){
        if(!(0x4F80U&(1UL<<bit))||bit==8U)continue;
        setup(1);words[6]^=1UL<<bit;
        assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_BUSY&&!writes);
    }
    for(unsigned bit=23;bit<32U;++bit){
        if(bit==27U)continue;
        setup(1);words[7]^=1UL<<bit;
        assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_BUSY&&!writes);
    }
    for(unsigned variant=0;variant<8U;++variant){
        setup(0);
        switch(variant){case 0:words[0]&=0xFFFFFFFU;break;case 1:words[1]&=~0x38U;break;
        case 2:words[2]=8;break;case 3:words[0]^=1;break;case 4:words[0]|=0x40;break;
        case 5:words[0]|=0x800;break;case 6:words[0]&=~0x700U;break;default:words[0]^=0x10;break;}
        assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_VERIFY&&!writes);
    }
    for(unsigned variant=0;variant<4U;++variant){
        setup(0);
        if(variant<2U)words[3]|=variant?0x8000:0x4000;
        else if(variant==2U)words[4]|=0x1000000;else words[5]|=0x10000000;
        assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_BUSY&&!writes);
    }
    setup(0);assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_OK&&r.stage==4&&!r.adopted&&r.writes==3&&writes==3);
    assert(words[6]==0x40200180&&words[7]==0x7E801234);
    assert(!memcmp(words,original,6*sizeof words[0]));
    assert((words[6]&~0x4F80U)==(original[6]&~0x4F80U));
    assert((words[7]&~0xF7800000U)==(original[7]&~0xF7800000U));
    assert(r.before[0]==original[6]&&r.before[1]==original[7]&&r.after[0]==words[6]&&r.after[1]==words[7]);
    for(unsigned failure=1;failure<=3U;++failure){
        setup(0);drop=failure;assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_VERIFY&&writes==failure);sticky();
        setup(0);mutate=failure;assert(maia_esp32p4_psram_supply1900(&r)==MAIA_PSRAM_VERIFY&&writes==failure);sticky();
    }
    return 0;
}
