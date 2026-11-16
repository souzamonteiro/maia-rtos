/* SPDX-FileCopyrightText: 2019-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Native adaptation of esp_rom_regi2c_esp32p4.c regi2c_read_impl and
 * clk_tree_ll.h clk_ll_mpll_get_freq_mhz, ESP-IDF f5c3654. Modified: fixed MPLL
 * bounded read plus powered400MHz calibration, native boot ownership/guards,
 * readback and routing restore. No SDK headers/runtime or ROM analog helper. LICENSES/Apache-2.0.txt. */
#include "psram_mpll_internal.h"
static unsigned fault;
static uint32_t lpclk,reset,power,clock,cal,route1,route2;
#ifdef MAIA_PSRAM_TEST
void maia_psram_mpll_test_reset(void);
void maia_psram_mpll_test_reset(void){fault=0;}
#endif
static int live(void){
    return psram_read(MPLL_LPCLK)==lpclk&&psram_read(MPLL_LPRESET)==reset&&
        psram_read(MPLL_POWER)==power&&psram_read(MPLL_CLOCK)==clock&&psram_read(MPLL_CAL)==cal;
}
static int owned(void){return live()&&psram_read(MPLL_CONF1)==route1&&psram_read(MPLL_CONF2)==route2;}
static int wait_idle(unsigned budget,maia_psram_mpll_report_t *r){
    for(unsigned i=0;i<budget;++i){if(!owned())return 0;++r->polls;if(!(psram_read(MPLL_ANA)&MPLL_BUSY))return 1;}
    return 0;
}
static int configure(uintptr_t p,uint32_t v){
    if(!owned())return 0;
    psram_write(p,v);
    if(!live()||psram_read(p)!=v)return 0;
    if(p==MPLL_CONF1)route1=v;else route2=v;
    return owned();
}
maia_psram_result_t maia_psram_mpll_read(uint32_t budget,maia_psram_mpll_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(fault)return MAIA_PSRAM_FAILED;
    lpclk=psram_read(MPLL_LPCLK);reset=psram_read(MPLL_LPRESET);power=psram_read(MPLL_POWER);clock=psram_read(MPLL_CLOCK);cal=psram_read(MPLL_CAL);
    r->power=power;r->clock=clock;r->calibration=cal;
    if(!(lpclk&0x8000000U)||(reset&0x4000000U)||!(power&0x1000000U)||!(clock&0x10000000U)||(cal&0x300U)!=0x300U)return MAIA_PSRAM_BUSY;
    if(!live()||(psram_read(MPLL_ANA)&MPLL_BUSY)||(psram_read(MPLL_ANA+4U)&MPLL_BUSY))return MAIA_PSRAM_BUSY;
    uint32_t saved1=route1=psram_read(MPLL_CONF1),saved2=route2=psram_read(MPLL_CONF2);
    if(!owned())return MAIA_PSRAM_CHANGED;
    fault=1;r->stage=1;
    /* SDK routing reset/selection, reserved high byte preserved; no gate/reset
     * writes, other analog clients excluded by caller throughout transaction. */
    if(!configure(MPLL_CONF2,saved2&0xFF000000U)||!configure(MPLL_CONF1,saved1&0xFF000000U)||
       !configure(MPLL_CONF2,(saved2&0xFF000000U)|0x200U))return MAIA_PSRAM_VERIFY;
    for(unsigned n=0;n<2U;++n){
        if(!wait_idle(budget,r))return MAIA_PSRAM_TIMEOUT;
        /* bit24 WR_CNTL stays0: fixed read only, slave63/address2. */
        psram_write(MPLL_ANA,MPLL_READ_DIV);
        if(!wait_idle(budget,r))return MAIA_PSRAM_TIMEOUT;
        unsigned value=(psram_read(MPLL_ANA)>>16U)&255U;
        if(n)r->repeat=value;else r->div=value;
    }
    r->stage=2;
    if(!configure(MPLL_CONF2,saved2)||!configure(MPLL_CONF1,saved1))return MAIA_PSRAM_VERIFY;
    r->restored=1;r->stage=3;
    if(r->repeat!=r->div)return MAIA_PSRAM_VERIFY;
    r->mhz=40U*((r->div>>3U)+1U)/((r->div&7U)+1U);
    fault=0;return MAIA_PSRAM_OK;
}

/* Native calibration/programming subset of rtc_clk_mpll_configure and
 * clk_ll_mpll_set_config, pinned f5c3654. Powered domain only, no clock retuning
 * while PSRAM uses MPLL; exact native20MHz ownership is a prerequisite. */
typedef struct {
    maia_psram_snapshot_t memory;
    uint32_t fixed[4],route[2],cal;
    maia_psram_mpll_init_report_t *report;
} mpll_init_t;
static int init_live(const mpll_init_t *c){
    const uintptr_t a[4]={MPLL_LPCLK,MPLL_LPRESET,MPLL_POWER,MPLL_CLOCK};
    if(maia_psram_clock_mhz()!=20U)return 0;
    for(unsigned i=0;i<4U;++i)if(psram_read(a[i])!=c->fixed[i])return 0;
    if((psram_read(MPLL_CAL)&~0x100U)!=(c->cal&~0x100U))return 0;
    if(psram_read(MPLL_CONF1)!=c->route[0]||psram_read(MPLL_CONF2)!=c->route[1])return 0;
    maia_psram_snapshot_t now;if(maia_esp32p4_psram_snapshot(&now)!=MAIA_PSRAM_OK)return 0;
    for(unsigned i=0;i<9U;++i)if(now.roots[i]!=c->memory.roots[i])return 0;
    for(unsigned i=0;i<27U;++i)if(now.before[i]!=c->memory.before[i])return 0;
    return psram_axi_quiet()&&!now.before[0]&&!now.before[16];
}
static int init_route(mpll_init_t *c,unsigned index,uint32_t value){
    if(!init_live(c))return 0;
    uintptr_t a=index?MPLL_CONF2:MPLL_CONF1;psram_write(a,value);
    if(psram_read(a)!=value)return 0;
    c->route[index]=value;return init_live(c);
}
static int init_wait(mpll_init_t *c,unsigned budget){
    for(unsigned i=0;i<budget;++i){++c->report->polls;if(!init_live(c))return 0;if(!(psram_read(MPLL_ANA)&MPLL_BUSY))return 1;}return 0;
}
static int init_analog(mpll_init_t *c,unsigned reg,unsigned write,uint32_t *value,unsigned budget){
    if(reg<1U||reg>3U||!init_wait(c,budget))return 0;
    uint32_t command=0x63U|(reg<<8U)|(write?(0x1000000U|((*value&255U)<<16U)):0U);
    psram_write(MPLL_ANA,command);if(write)++c->report->writes;
    if(!init_wait(c,budget))return 0;
    uint32_t observed=psram_read(MPLL_ANA);
    if((observed&0x100FFFFU)!=(command&0x100FFFFU))return 0;
    if(!write)*value=(observed>>16U)&255U;
    if(!(psram_read(MPLL_CAL)&0x100U))c->report->saw_not_done=1;
    return 1;
}
static int init_cal(mpll_init_t *c,uint32_t value){
    if(!init_live(c))return 0;
    psram_write(MPLL_CAL,value);
    uint32_t observed=psram_read(MPLL_CAL);
    if(!(value&0x200U)&&!(observed&0x100U))c->report->saw_not_done=1;
    if((observed&~0x100U)!=(value&~0x100U))return 0;
    c->cal=value;return init_live(c);
}
/* clk_ll_mpll_disable/enable: toggle only PMU_MSPI_PHY_XPD. The LDO2
 * supply and PSRAM clocks remain untouched, other MPLL clients excluded. */
static int init_power(mpll_init_t *c,uint32_t value){
    if(!init_live(c))return 0;
    psram_write(MPLL_POWER,value);
    if(psram_read(MPLL_POWER)!=value)return 0;
    c->fixed[2]=value;
    if(!(psram_read(MPLL_CAL)&0x100U))c->report->saw_not_done=1;
    return init_live(c);
}
maia_psram_result_t maia_esp32p4_psram_mpll_initialize400(uint32_t budget,maia_psram_mpll_init_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile uint8_t *)r)[i]=0;
    if(!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(fault)return MAIA_PSRAM_FAILED;
    if(maia_psram_clock_mhz()!=20U)return MAIA_PSRAM_BUSY;
    mpll_init_t c; c.report=r;
    if(maia_esp32p4_psram_snapshot(&c.memory)!=MAIA_PSRAM_OK||c.memory.before[0]||c.memory.before[16]||!psram_axi_quiet())return MAIA_PSRAM_BUSY;
    if((c.memory.roots[1]&0xFFB000U)!=0x8000U||c.memory.before[7]!=0x10001U||c.memory.before[20]!=0x10001U)return MAIA_PSRAM_BUSY;
    const uintptr_t a[4]={MPLL_LPCLK,MPLL_LPRESET,MPLL_POWER,MPLL_CLOCK};
    for(unsigned i=0;i<4U;++i)c.fixed[i]=psram_read(a[i]);
    c.cal=r->cal_before=psram_read(MPLL_CAL);
    if(!(c.fixed[0]&0x8000000U)||(c.fixed[1]&0x4000000U)||!(c.fixed[2]&0x1000000U)||!(c.fixed[3]&0x10000000U)||!(c.cal&0x200U))return MAIA_PSRAM_BUSY;
    if((psram_read(MPLL_ANA)|psram_read(MPLL_ANA+4U))&MPLL_BUSY)return MAIA_PSRAM_BUSY;
    uint32_t saved[2];for(unsigned i=0;i<2U;++i)saved[i]=c.route[i]=psram_read(i?MPLL_CONF2:MPLL_CONF1);
    if(!init_live(&c))return MAIA_PSRAM_CHANGED;
    fault=1;r->stage=1;
    if(!init_route(&c,1,saved[1]&0xFF000000U)||!init_route(&c,0,saved[0]&0xFF000000U)||!init_route(&c,1,(saved[1]&0xFF000000U)|0x200U))return MAIA_PSRAM_VERIFY;
    for(unsigned i=0;i<3U;++i)if(!init_analog(&c,i+1U,0,&r->before[i],budget))return MAIA_PSRAM_TIMEOUT;
    /* Preserve the known installed400M reference for recovery; future power-on
     * profiles need separate validation, not a guessed alternate baseline. */
    if(r->before[1]!=0x99U)return MAIA_PSRAM_VERIFY;
    uint32_t powered=c.fixed[2];
    if(!init_power(&c,powered&~0x1000000U)||!init_power(&c,powered))return MAIA_PSRAM_VERIFY;
    for(unsigned i=0;i<3U;++i)if(!init_analog(&c,i+1U,0,&r->before[i],budget))return MAIA_PSRAM_TIMEOUT;
    if(r->before[1]!=0x99U)return MAIA_PSRAM_VERIFY;
    if(!init_cal(&c,c.cal&~0x200U))return MAIA_PSRAM_VERIFY;
    if(!(psram_read(MPLL_CAL)&0x100U))r->saw_not_done=1;
    r->stage=2;
    uint32_t v=r->before[2]|0x30U;
    if(!init_analog(&c,3,1,&v,budget)||!init_analog(&c,3,0,&r->after[2],budget)||(r->after[2]&0x30U)!=0x30U)return MAIA_PSRAM_VERIFY;
    v=r->before[0]&~0x20U;
    uint32_t check;
    if(!init_analog(&c,1,1,&v,budget)||!init_analog(&c,1,0,&check,budget)||(check&0x20U))return MAIA_PSRAM_VERIFY;
    v=r->before[0]|0x20U;
    if(!init_analog(&c,1,1,&v,budget)||!init_analog(&c,1,0,&r->after[0],budget)||!(r->after[0]&0x20U))return MAIA_PSRAM_VERIFY;
    v=0x99U;
    if(!init_analog(&c,2,1,&v,budget)||!init_analog(&c,2,0,&r->after[1],budget)||r->after[1]!=0x99U)return MAIA_PSRAM_VERIFY;
    r->stage=3;unsigned done=0;
    for(unsigned i=0;i<budget;++i){++r->polls;if(!init_live(&c))return MAIA_PSRAM_VERIFY;
        uint32_t status=psram_read(MPLL_CAL);if(!(status&0x100U))r->saw_not_done=1;else {done=1;break;}}
    if(!done)return MAIA_PSRAM_TIMEOUT;
    if(!r->saw_not_done)return MAIA_PSRAM_VERIFY;
    if(!init_cal(&c,c.cal|0x200U))return MAIA_PSRAM_VERIFY;
    r->cal_after=psram_read(MPLL_CAL);r->stage=4;
    if(!init_route(&c,1,saved[1])||!init_route(&c,0,saved[0]))return MAIA_PSRAM_VERIFY;
    r->route_restored=1;r->mhz=40U*((r->after[1]>>3U)+1U)/((r->after[1]&7U)+1U);r->stage=5;fault=0;
    return MAIA_PSRAM_OK;
}
