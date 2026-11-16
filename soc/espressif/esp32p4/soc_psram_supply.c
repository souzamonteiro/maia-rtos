/* SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Fixed VO2/1900mV native adaptation of ESP-IDF f5c3654 ldo_ll and
 * esp_ldo_acquire_channel. No SDK headers/runtime/locks/allocation. Dedicated
 * eFuse pair required (no nominal fallback); boot lifetime, checked stores,
 * refuse live retuning, preserve unrelated fields. LICENSES/Apache-2.0.txt. */
#include "psram_mpll_internal.h"
#define VO2 (MAIA_ESP32P4_PMU_BASE+0x1D0U)
#define VO2_ANA (VO2+4U)
#define CONTROL_MASK 0x4F80U
#define ANALOG_MASK 0xF7800000U
static unsigned supply_state; /* 0 free,1 owned,2 uncertain until reset */
#ifdef MAIA_PSRAM_TEST
void maia_psram_supply_test_reset(void);
void maia_psram_supply_test_reset(void){supply_state=0;}
#endif
typedef struct {uint32_t fixed[6],control,analog;} supply_t;
static const uintptr_t supply_fixed[6]={MAIA_ESP32P4_EFUSE_MAC_SYS2_REG,
    MAIA_ESP32P4_EFUSE_MAC_SYS3_REG,MAIA_ESP32P4_EFUSE_RS_ERR0_REG,
    PSRAM_PERI,MPLL_POWER,MPLL_CLOCK};
static int supply_same(const supply_t *s){
    for(unsigned i=0;i<6U;++i)if(psram_read(supply_fixed[i])!=s->fixed[i])return 0;
    return psram_read(VO2)==s->control&&psram_read(VO2_ANA)==s->analog;
}
static int supply_store(supply_t *s,unsigned analog,uint32_t value,maia_psram_supply_report_t *r){
    if(!supply_same(s))return 0;
    uintptr_t address=analog?VO2_ANA:VO2;
    ++r->writes;psram_write(address,value);
    r->after[analog]=psram_read(address);
    if(r->after[analog]!=value)return 0;
    if(analog)s->analog=value;else s->control=value;
    return supply_same(s);
}
maia_psram_result_t maia_esp32p4_psram_supply1900(maia_psram_supply_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(supply_state)return supply_state==1U?MAIA_PSRAM_BUSY:MAIA_PSRAM_FAILED;
    if(maia_psram_clock_claimed())return MAIA_PSRAM_BUSY;
    supply_t s;
    for(unsigned i=0;i<6U;++i)s.fixed[i]=psram_read(supply_fixed[i]);
    s.control=r->before[0]=r->after[0]=psram_read(VO2);
    s.analog=r->before[1]=r->after[1]=psram_read(VO2_ANA);
    uint32_t sys2=s.fixed[0],sys3=s.fixed[1];
    r->dref=sys2>>28U;r->mul=(sys3>>3U)&7U;
    if(((sys2>>4U)&3U)!=1U||(sys2&15U)!=3U||(sys2&0xC0U)||
       ((sys2>>11U)&3U)||!((sys2>>8U)&7U)||(s.fixed[2]&8U)||!r->dref||!r->mul)return MAIA_PSRAM_VERIFY;
    if(!supply_same(&s))return MAIA_PSRAM_CHANGED;
    r->stage=1;
    uint32_t analog=(s.analog&~ANALOG_MASK)|(r->dref<<28U)|(r->mul<<23U)|0x4000000U;
    uint32_t routed=(s.control&~CONTROL_MASK)|0x80U,enabled=routed|0x100U;
    if(s.control&0x100U){
        if(s.control!=enabled||s.analog!=analog)return MAIA_PSRAM_BUSY;
        r->adopted=1;
    }else{
        /* No controller reads with gated clocks. VO2 also powers MPLL: require
         * its circuit and output clock off, both PSRAM core/PLL gates off. */
        if((s.fixed[3]&0xC000U)||(s.fixed[4]&0x1000000U)||(s.fixed[5]&0x10000000U))return MAIA_PSRAM_BUSY;
        supply_state=2;
        if(!supply_store(&s,1,analog,r))return MAIA_PSRAM_VERIFY;
        r->stage=2;
        if(!supply_store(&s,0,routed,r))return MAIA_PSRAM_VERIFY;
        r->stage=3;
        if(!supply_store(&s,0,enabled,r))return MAIA_PSRAM_VERIFY;
    }
    if(!supply_same(&s)){supply_state=2;return MAIA_PSRAM_CHANGED;}
    r->stage=4;supply_state=1;return MAIA_PSRAM_OK;
}
