/* SPDX-FileCopyrightText: 2019-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Native adaptation of esp_rom_regi2c_esp32p4.c regi2c_read_impl and
 * clk_tree_ll.h clk_ll_mpll_get_freq_mhz, ESP-IDF f5c3654. Modified: fixed MPLL
 * read only, budgeted waits, boot ownership/guards/readback/routing restore.
 * No SDK headers/runtime/ROM helper/PLL tuning. LICENSES/Apache-2.0.txt. */
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
