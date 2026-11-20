/* Original MaiaRTOS MIT. Bounded boot-exclusive warm HEX clock qualification.
 * Register facts: pinned ESP-IDF f5c3654 psram_ctrlr_ll, hp_sys_clkrst_reg,
 * spi_mem_s_reg, spi1_mem_s_reg and ldo_ll. No SDK headers/runtime or ROM calls. */
/* Consumer-gating sequence adapted from Espressif pmu_sleep.c/psram_ctrlr_ll/
 * clk_tree_ll at f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0.
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Modified: boot-exclusive checked park/resume, no sleep/VO2 writes, bounded
 * MMIO, preserved fields and sticky failures. LICENSES/Apache-2.0.txt. */
#include "psram_window_internal.h"
#include "psram_mpll_internal.h"
#define CLOCK_SELECT_DIV 0x00FF3000U
#define CLOCK_CORE_ENABLE 0x8000U
enum { CLOCK_FREE,CLOCK_OWNED,CLOCK_FAULTED };
static unsigned clock_state,fast_mhz,pad_owned,park_owned,boot_owned,boot_pll_ready;
static uint32_t boot_current[6];
static unsigned boot_ana_clock_owned;
static const uintptr_t boot_address[6]={MPLL_LPCLK,MPLL_LPRESET,MPLL_POWER,MPLL_CLOCK,MPLL_CAL,MPLL_ANA_CLOCK};
static uint32_t park_original[3],park_current[3];
static uint32_t saved_pads[20],current_pads[20],saved_dll[2];
unsigned maia_psram_clock_mhz(void){return clock_state==CLOCK_OWNED&&!park_owned?(fast_mhz?fast_mhz:20U):0U;}
int maia_psram_clock_claimed(void){return clock_state==CLOCK_OWNED;}
int maia_psram_boot_claimed(void){return boot_owned&&clock_state==CLOCK_OWNED;}
void maia_psram_boot_mpll_ready(void){if(maia_psram_boot_claimed())boot_pll_ready=1;}
static maia_psram_snapshot_t original,current;
#ifdef MAIA_PSRAM_TEST
void maia_psram_clock_test_reset(void);
void maia_psram_clock_test_reset(void){clock_state=CLOCK_FREE;fast_mhz=pad_owned=park_owned=boot_owned=boot_pll_ready=boot_ana_clock_owned=0;}
void maia_psram_clock_test_claim(void);
void maia_psram_clock_test_claim(void){clock_state=CLOCK_OWNED;}
#endif
static int roots_match(void){
    for(unsigned i=0;i<9U;++i)if(psram_read(psram_roots[i])!=current.roots[i])return 0;
    if(pad_owned)for(unsigned i=0;i<20U;++i)if(psram_read(window_plan[i].address)!=current_pads[i])return 0;
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
/* Vendor startup configures drive/DQS before selecting the fast source. These
 * pads remain exclusively owned across window lifetime and restore after XTAL. */
static int pad_change(unsigned i,uint32_t value){
    if(!guarded())return 0;
    psram_write(window_plan[i].address,value);
    for(unsigned n=0;n<9U;++n)if(psram_read(psram_roots[n])!=current.roots[n])return 0;
    if(psram_read(window_plan[i].address)!=value)return 0;
    current_pads[i]=value;return guarded();
}
static int dll_change(unsigned index,uint32_t value){
    if(!guarded())return 0;
    psram_write(psram_address(index),value);
    if(!roots_match()||psram_read(psram_address(index))!=value)return 0;
    current.before[index]=value;return guarded();
}
static int calibrated_supply(const maia_psram_snapshot_t *s){
    uint32_t sys2=s->roots[6],sys3=s->roots[7];
    unsigned d=sys2>>28U,m=(sys3>>3U)&7U;
    /* Exact rev1.3/eFuse layout already qualified by native LDO3 work. VO2's
     * dedicated nonzero pair is the SDK's1900mV calibration override. */
    return psram_supply_trim_valid(sys2,sys3,s->roots[8])&&
        (s->roots[4]&0x4F80U)==0x180U&&((s->roots[5]>>28U)&15U)==d&&((s->roots[5]>>23U)&7U)==m;
}
static void save_snapshot(const maia_psram_snapshot_t *s){
    for(unsigned i=0;i<sizeof *s;++i){unsigned char v=((const unsigned char *)s)[i];
        ((volatile unsigned char *)&original)[i]=v;((volatile unsigned char *)&current)[i]=v;}
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
    /* Explicit bounded boot copy: size optimization must not introduce a
     * compiler-generated libc memcpy dependency into the freestanding image. */
    save_snapshot(&s);
    evidence(r);if(!guarded())return MAIA_PSRAM_CHANGED;
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
    if(clock_state!=CLOCK_OWNED||fast_mhz||park_owned||boot_owned)return MAIA_PSRAM_BUSY;
    evidence(r);clock_state=CLOCK_FAULTED;
    if(!change(0,2,original.before[20],r)||!change(1,1,original.before[7],r)||
       !change(2,0,current.roots[1]&~CLOCK_CORE_ENABLE,r)||
       !change(3,0,original.roots[1]&~CLOCK_CORE_ENABLE,r)||
       !change(4,0,original.roots[1],r))return MAIA_PSRAM_VERIFY;
    r->restored=1;clock_state=CLOCK_FREE;return MAIA_PSRAM_OK;
}

/* Boot-only enable sequence from pinned ldo_ll/regi2c_ctrl_ll/clk_tree_ll.
 * Supply is initially disabled: no guessed off pulse or prior MPLL frequency. */
static int boot_live(void){
    for(unsigned i=0;i<(boot_ana_clock_owned?6U:5U);++i){uint32_t mask=i==4U?~0x100U:~0U;
        if((psram_read(boot_address[i])&mask)!=(boot_current[i]&mask))return 0;}
    return guarded();
}
static int boot_store(unsigned index,uint32_t value,maia_psram_boot_report_t *r){
    if(!boot_live())return 0;
    ++r->writes;psram_write(boot_address[index],value);
    r->after[index]=psram_read(boot_address[index]);
    uint32_t mask=index==4U?~0x100U:~0U;
    if((r->after[index]&mask)!=(value&mask))return 0;
    boot_current[index]=r->after[index];return boot_live();
}
maia_psram_result_t maia_psram_boot20(maia_psram_boot_report_t *r){
    r->failed_guards=0;r->entry_axi=0;r->entry_axi_valid=0;
    r->entry_mpll[0]=r->entry_mpll[1]=0;r->entry_mpll_valid=0;
    for(unsigned i=0;i<sizeof r->entry_snapshot;++i)
        ((volatile unsigned char *)&r->entry_snapshot)[i]=0;
    maia_psram_result_t result=initialize_report(&r->clock);
    if(result!=MAIA_PSRAM_OK){
        if(result==MAIA_PSRAM_FAILED)r->failed_guards=MAIA_PSRAM_BOOT_GUARD_PREVIOUS_FAILURE;
        return result;
    }
    if(clock_state!=CLOCK_FREE)r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_CLOCK_CLAIMED;
    if(maia_psram_x8_claimed())r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_X8_CLAIMED;
    if(r->failed_guards)return MAIA_PSRAM_BUSY;
    result=maia_esp32p4_psram_snapshot(&r->entry_snapshot);if(result!=MAIA_PSRAM_OK)return result;
    const maia_psram_snapshot_t *s=&r->entry_snapshot;
    r->entry_axi=psram_read(PSRAM_MSPI0+0x3CU);r->entry_axi_valid=1;
    if(s->before[0])r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_MSPI0_ACTIVE;
    if(s->before[16])r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_MSPI1_ACTIVE;
    if((r->entry_axi&0x80000001U)!=0x80000000U)r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_AXI_OPEN;
    if(s->before[2]&1U)r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_MSPI0_FSM_BUSY;
    if(s->roots[4]&0x100U)r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_VO2_ENABLED;
    if(!psram_supply_trim_valid(s->roots[6],s->roots[7],s->roots[8]))
        r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_EFUSE_INVALID;
    if(s->roots[1]&CLOCK_SELECT_DIV)r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_DIVIDER_NOT_XTAL;
    if(s->before[7]!=0x30103U)r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_MSPI0_CLK_CONFIG;
    if(s->before[20]!=0x30103U)r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_MSPI1_CLK_CONFIG;
    if(r->failed_guards)return MAIA_PSRAM_BUSY;
    for(unsigned i=0;i<5U;++i)r->before[i]=r->after[i]=boot_current[i]=psram_read(boot_address[i]);
    if((boot_current[0]&0x8000000U)&&!(boot_current[1]&0x4000000U)){
        r->entry_mpll[0]=psram_read(MPLL_ANA);r->entry_mpll[1]=psram_read(MPLL_ANA+4U);
        r->entry_mpll_valid=1;
        if((r->entry_mpll[0]|r->entry_mpll[1])&MPLL_BUSY){
            r->failed_guards|=MAIA_PSRAM_BOOT_GUARD_MPLL_BUSY;return MAIA_PSRAM_BUSY;
        }
    }
    save_snapshot(s);evidence(&r->clock);if(!boot_live())return MAIA_PSRAM_CHANGED;
    r->stage=1;clock_state=CLOCK_FAULTED;
    uint32_t peri=current.roots[1];
    if(!change(0,0,peri&~0x8000U,&r->clock)||!change(1,0,peri&~0xC000U,&r->clock)||
       !boot_store(3,boot_current[3]&~0x10000000U,r)||!boot_store(2,boot_current[2]&~0x1000000U,r))return MAIA_PSRAM_VERIFY;
    r->stage=2;
    result=maia_esp32p4_psram_supply1900(&r->supply);if(result!=MAIA_PSRAM_OK)return result;
    /* The supply owner changed only these two roots. Do not adopt any other
     * concurrent root change when joining its checked result. */
    for(unsigned i=0;i<9U;++i)if(i!=4U&&i!=5U&&psram_read(psram_roots[i])!=current.roots[i])return MAIA_PSRAM_CHANGED;
    current.roots[4]=r->supply.after[0];current.roots[5]=r->supply.after[1];
    if(!calibrated_supply(&current)||!boot_live())return MAIA_PSRAM_VERIFY;
    r->stage=3;
    if(!boot_store(0,boot_current[0]|0x8000000U,r)||!boot_store(1,boot_current[1]&~0x4000000U,r))return MAIA_PSRAM_VERIFY;
    if((psram_read(MPLL_ANA)|psram_read(MPLL_ANA+4U))&MPLL_BUSY)return MAIA_PSRAM_BUSY;
    r->before[5]=r->after[5]=boot_current[5]=psram_read(MPLL_ANA_CLOCK);boot_ana_clock_owned=1;
    if(!boot_store(5,boot_current[5]|1U,r)||!boot_store(4,boot_current[4]|0x200U,r)||!boot_store(2,boot_current[2]|0x1000000U,r)||
       !boot_store(3,boot_current[3]|0x10000000U,r))return MAIA_PSRAM_VERIFY;
    r->stage=4;
    if(!change(2,0,peri&~0x8000U,&r->clock)||!change(3,0,peri,&r->clock)||
       !change(4,1,0x10001U,&r->clock)||!change(5,2,0x10001U,&r->clock)||!boot_live())return MAIA_PSRAM_VERIFY;
    boot_owned=1;boot_pll_ready=0;clock_state=CLOCK_OWNED;r->stage=5;return MAIA_PSRAM_OK;
}

/* Pinned f5c3654 pmu_sleep_start/psram_ctrlr_ll/clk_ll_mpll_enable order.
 * Only the module PLL gate is changed; the shared register-bus gate stays on.
 * Never access controller registers after the core gate is removed. */
static int park_live(void){
    return psram_read(MPLL_POWER)==park_current[1]&&psram_read(MPLL_CLOCK)==park_current[2]&&guarded();
}
/* Keep one checked-store body in the bounded low-SRAM diagnostic image. */
static __attribute__((noinline)) int park_store(unsigned index,uint32_t value,maia_psram_park_report_t *r){
    if(!park_live())return 0;
    uintptr_t p=index==0U?PSRAM_PERI:index==1U?MPLL_POWER:MPLL_CLOCK;
    ++r->writes;psram_write(p,value);r->after[index]=psram_read(p);
    if(r->after[index]!=value)return 0;
    park_current[index]=value;if(!index)current.roots[1]=value;
    if(!park_live())return 0;
    ++r->stage;return 1;
}
maia_psram_result_t maia_esp32p4_psram_consumers_park(unsigned restore,maia_psram_park_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(restore>1U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(clock_state==CLOCK_FAULTED)return MAIA_PSRAM_FAILED;
    if(clock_state!=CLOCK_OWNED||fast_mhz||pad_owned||park_owned!=(restore?1U:0U)||maia_psram_x8_claimed())return MAIA_PSRAM_BUSY;
    if(!guarded())return MAIA_PSRAM_CHANGED;
    if(!restore){
        park_original[0]=park_current[0]=current.roots[1];
        park_original[1]=park_current[1]=psram_read(MPLL_POWER);
        park_original[2]=park_current[2]=psram_read(MPLL_CLOCK);
        if((current.roots[1]&0xFFB000U)!=0x8000U||!(current.roots[1]&0x4000U)||
           current.before[0]||current.before[16]||!(park_current[1]&0x1000000U)||!(park_current[2]&0x10000000U))return MAIA_PSRAM_BUSY;
    }
    for(unsigned i=0;i<3U;++i)r->before[i]=r->after[i]=park_current[i];
    if(!park_live())return MAIA_PSRAM_CHANGED;
    clock_state=CLOCK_FAULTED;if(boot_owned)boot_pll_ready=0;
    if(!restore){
        uint32_t peri=park_current[0];
        if(!park_store(0,peri&~0x8000U,r)||!park_store(0,peri&~0xC000U,r)||
           !park_store(2,park_current[2]&~0x10000000U,r)||!park_store(1,park_current[1]&~0x1000000U,r))return MAIA_PSRAM_VERIFY;
        park_owned=1;
    }else{
        if(!park_store(1,park_original[1],r)||!park_store(2,park_original[2],r)||
           !park_store(0,park_original[0]&~0x8000U,r)||!park_store(0,park_original[0],r))return MAIA_PSRAM_VERIFY;
        park_owned=0;r->restored=1;
    }
    clock_state=CLOCK_OWNED;return MAIA_PSRAM_OK;
}

maia_psram_result_t maia_esp32p4_psram_fast_start(uint32_t mhz,uint32_t budget,maia_psram_fast_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if((mhz!=100U&&mhz!=200U)||!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    maia_psram_result_t result=initialize_report(&r->clock);if(result!=MAIA_PSRAM_OK)return result;
    if(clock_state!=CLOCK_OWNED||fast_mhz||park_owned||(boot_owned&&!boot_pll_ready)||!maia_psram_x8_claimed())return MAIA_PSRAM_BUSY;
    if(!guarded())return MAIA_PSRAM_CHANGED;
    /* A failed analog command can retain shared-port ownership: sticky clock
     * fault as well, never proceed to mapping or restoration through it. */
    clock_state=CLOCK_FAULTED;
    result=maia_psram_mpll_read(budget,&r->mpll);if(result!=MAIA_PSRAM_OK)return result;
    if(r->mpll.mhz!=400U)return MAIA_PSRAM_VERIFY;
    saved_dll[0]=current.before[9];saved_dll[1]=current.before[13];
    if(!dll_change(9,saved_dll[0]|0x20U)||!dll_change(13,saved_dll[1]|0x20U))return MAIA_PSRAM_VERIFY;
    for(unsigned i=0;i<20U;++i)saved_pads[i]=current_pads[i]=psram_read(window_plan[i].address);
    pad_owned=1;
    for(unsigned i=0;i<20U;++i)if(!pad_change(i,(saved_pads[i]&~window_plan[i].mask)|window_plan[i].value))return MAIA_PSRAM_VERIFY;
    uint32_t peri=current.roots[1],bus=mhz==200U?0x10001U:0x30103U;
    if(!change(0,1,bus,&r->clock)||!change(1,2,bus,&r->clock)||
       !change(2,0,peri&~CLOCK_CORE_ENABLE,&r->clock)||
       !change(3,0,(peri&~(CLOCK_CORE_ENABLE|CLOCK_SELECT_DIV))|0x1000U,&r->clock)||
       !change(4,0,(peri&~CLOCK_SELECT_DIV)|0x1000U,&r->clock))return MAIA_PSRAM_VERIFY;
    r->mhz=mhz;fast_mhz=mhz;clock_state=CLOCK_OWNED;return MAIA_PSRAM_OK;
}
maia_psram_result_t maia_esp32p4_psram_fast_stop(maia_psram_clock_report_t *r){
    maia_psram_result_t result=initialize_report(r);if(result!=MAIA_PSRAM_OK)return result;
    if(clock_state!=CLOCK_OWNED||!fast_mhz)return MAIA_PSRAM_BUSY;
    uint32_t peri=current.roots[1];clock_state=CLOCK_FAULTED;
    if(!change(0,0,peri&~CLOCK_CORE_ENABLE,r)||
       !change(1,0,peri&~(CLOCK_CORE_ENABLE|CLOCK_SELECT_DIV),r)||
       !change(2,0,peri&~CLOCK_SELECT_DIV,r)||
       !change(3,1,0x10001U,r)||!change(4,2,0x10001U,r))return MAIA_PSRAM_VERIFY;
    for(unsigned n=20U;n>0U;--n)if(!pad_change(n-1U,saved_pads[n-1U]))return MAIA_PSRAM_VERIFY;
    if(!dll_change(9,saved_dll[0])||!dll_change(13,saved_dll[1]))return MAIA_PSRAM_VERIFY;
    pad_owned=0;r->restored=1;fast_mhz=0;clock_state=CLOCK_OWNED;return MAIA_PSRAM_OK;
}

/* SDK mspi_timing_ll_set_dqs_phase, native exclusive fast-pad ownership. */
maia_psram_result_t maia_psram_fast_phase(unsigned phase){
    if(phase>3U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(clock_state==CLOCK_FAULTED)return MAIA_PSRAM_FAILED;
    if(clock_state!=CLOCK_OWNED||!fast_mhz||!pad_owned)return MAIA_PSRAM_BUSY;
    if(!guarded())return MAIA_PSRAM_CHANGED;
    clock_state=CLOCK_FAULTED;
    if(!pad_change(8,(current_pads[8]&~6U)|(phase<<1U))||!pad_change(19,(current_pads[19]&~6U)|(phase<<1U)))return MAIA_PSRAM_VERIFY;
    clock_state=CLOCK_OWNED;return MAIA_PSRAM_OK;
}
