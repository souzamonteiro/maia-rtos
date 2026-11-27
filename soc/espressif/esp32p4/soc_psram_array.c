/* SPDX-FileCopyrightText: 2019-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Native adaptation of mspi_timing_config_psram_read_data/write_data in
 * ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0 mspi_timing_by_dqs.c.
 * Modified: native bounded command engine instead of SDK/ROM transactions;
 * explicit boot training aperture, profile checks, guarded register/FIFO restore,
 * byte-safe packing, error/partial-progress contracts and sticky failure policy.
 * License: LICENSES/Apache-2.0.txt. No SDK/FreeRTOS headers/runtime. */
#include "psram_mode_internal.h"
static unsigned array_fault;
static unsigned mode_claim,profile_state;
static uint32_t original_profile[3],configured_profile[3];
/* Bounded boot-local initialization; -Os must not introduce libc helpers. */
static void clear_transfer(maia_psram_transfer_report_t *r){
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
}
int maia_psram_x8_claimed(void){return mode_claim==1U&&!array_fault&&profile_state!=2U;}
#ifdef MAIA_PSRAM_TEST
void maia_psram_array_test_reset(void);
void maia_psram_array_test_reset(void){array_fault=mode_claim=profile_state=0;}
#endif
static uintptr_t address(unsigned i){return i<9U?psram_mode_address(i):i==9U?PSRAM_MSPI1+0x24U:PSRAM_MSPI1+0x58U+(i-10U)*4U;}
static int roots(const maia_psram_snapshot_t *s){for(unsigned i=0;i<9U;++i)if(psram_read(psram_roots[i])!=s->roots[i])return 0;return 1;}
static int owned(const maia_psram_snapshot_t *s,const uint32_t *v,unsigned fifo){
    if(!roots(s)||!psram_axi_quiet())return 0;
    for(unsigned i=0;i<16U;++i){uint32_t x=psram_read(psram_address(i));if(i?x!=s->before[i]:!!(x&~255U))return 0;}
    static const unsigned fixed[]={18,19,20,25,26};
    for(unsigned i=0;i<5U;++i)if(psram_read(psram_address(fixed[i]))!=s->before[fixed[i]])return 0;
    for(unsigned i=0;i<(fifo?26U:10U);++i)if(psram_read(address(i))!=v[i])return 0;
    return 1;
}
static int idle(void){return !psram_read(PSRAM_MSPI0)&&!psram_read(PSRAM_MSPI1);}
static maia_psram_result_t chunk(uint32_t offset,uint8_t *data,unsigned len,unsigned write,
    unsigned budget,maia_psram_transfer_report_t *r){
    maia_psram_snapshot_t s;maia_psram_result_t result=maia_esp32p4_psram_snapshot(&s);
    if(result!=MAIA_PSRAM_OK)return result;
    if(s.before[0]||s.before[16]||!psram_axi_quiet()||(s.before[2]&1U)||!(s.roots[4]&0x100U)||
       (s.roots[1]&0xFFB000U)!=0x8000U||s.before[7]!=0x10001U||s.before[20]!=0x10001U||
       (s.before[17]&0x01904100U)||(s.before[24]&0x0020000EU))return MAIA_PSRAM_BUSY;
    uint32_t saved[26],current[26],desired[26];uint8_t received[64];
    for(unsigned i=0;i<26U;++i)saved[i]=current[i]=desired[i]=psram_read(address(i));
    for(unsigned i=0;i<9U;++i)desired[i]=psram_mode_value(i,saved[i],offset);
    desired[1]=(saved[1]&~psram_mode_masks[1])|(write==2U?0xC8000000U:write?0xE8000000U:0xF0000000U);
    desired[2]=(saved[2]&~psram_mode_masks[2])|0x7C000000U|(write==2U?0U:write?11U:25U);
    desired[3]=(saved[3]&~psram_mode_masks[3])|0xF0000000U|(write==2U?0xC0C0U:write?0x8080U:0U);
    desired[5]=(saved[5]&~0x3FFU)|(write?0U:len*8U-1U);
    desired[9]=(saved[9]&~0x3FFU)|(write?len*8U-1U:0U);
    for(unsigned i=0;i<16U;++i){
        uint32_t value=0xA55A5AA5U;
        if(write){value=0;for(unsigned b=0;b<4U;++b)if(i*4U+b<len)value|=(uint32_t)data[i*4U+b]<<(b*8U);}
        desired[10U+i]=value;
    }
    if(!owned(&s,current,1)||!idle())return MAIA_PSRAM_CHANGED;
    array_fault=1;r->restored=0;
    for(unsigned i=0;i<26U;++i){
        if(!owned(&s,current,1)||!idle())return MAIA_PSRAM_VERIFY;
        psram_write(address(i),desired[i]);
        if(!roots(&s)||psram_read(address(i))!=desired[i])return MAIA_PSRAM_VERIFY;
        current[i]=desired[i];
    }
    if(!owned(&s,current,1)||!idle())return MAIA_PSRAM_VERIFY;
    if(write)r->write_attempted=1;
    psram_write(PSRAM_MSPI1,0x40000U);unsigned done=0;
    for(unsigned n=0;n<budget;++n){
        ++r->polls;if(!owned(&s,current,0))return MAIA_PSRAM_VERIFY;
        r->command=psram_read(PSRAM_MSPI1);
        if(r->command&~0x400FFU)return MAIA_PSRAM_VERIFY;
        if(!r->command&&!psram_read(PSRAM_MSPI0)){done=1;break;}
    }
    if(!done)return MAIA_PSRAM_TIMEOUT;
    if(!owned(&s,current,0)||!idle())return MAIA_PSRAM_VERIFY;
    for(unsigned i=0;i<16U;++i){
        if(!roots(&s))return MAIA_PSRAM_VERIFY;
        uint32_t v=psram_read(address(10U+i));current[10U+i]=v;
        for(unsigned b=0;b<4U;++b)received[i*4U+b]=(uint8_t)(v>>(b*8U));
    }
    for(unsigned n=26U;n>0U;--n){unsigned i=n-1U;
        if(!owned(&s,current,1)||!idle())return MAIA_PSRAM_VERIFY;
        psram_write(address(i),saved[i]);
        if(!roots(&s)||psram_read(address(i))!=saved[i])return MAIA_PSRAM_VERIFY;
        current[i]=saved[i];
    }
    if(!owned(&s,current,1)||!idle())return MAIA_PSRAM_VERIFY;
    if(!write)for(unsigned i=0;i<len;++i)data[i]=received[i];
    r->restored=1;array_fault=0;return MAIA_PSRAM_OK;
}
static maia_psram_result_t profile(unsigned x16,unsigned budget){
    static const unsigned regs[]={0,2,4,8},masks[]={0x1F3FU,0xFFU,0xE0U,0x4FU},values[]={0x0D30U,0xDFU,0x20U,0x4BU};
    /* MR0/1 uses little-endian word: vendor upper byte, latency lower byte. */
    for(unsigned i=0;i<4U;++i){maia_psram_mode_report_t m;
        maia_psram_result_t result=maia_esp32p4_psram_read_mode(regs[i],budget,&m);
        if(result!=MAIA_PSRAM_OK)return result;
        if((m.value&masks[i])!=(i==3U&&!x16?0x0BU:values[i]))return MAIA_PSRAM_BUSY;
    }
    return MAIA_PSRAM_OK;
}
maia_psram_result_t maia_esp32p4_psram_x8_mode(unsigned restore,uint32_t budget,maia_psram_mode_switch_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    r->before=r->after=r->stage=0;
    if(restore>1U||!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(array_fault||mode_claim==2U||profile_state==2U)return MAIA_PSRAM_FAILED;
    if(mode_claim!=(restore?1U:0U))return MAIA_PSRAM_BUSY;
    maia_psram_result_t result=profile(!restore,budget);
    if(result==MAIA_PSRAM_BUSY&&!restore){
        /* Warm reset (RTS/software) keeps the powered device in the X8 state
         * left by the previous native boot; adopt it only with the full X8
         * profile and an unchanged MR8 reread. */
        result=profile(0,budget);if(result!=MAIA_PSRAM_OK)return result;
        maia_psram_mode_report_t m;result=maia_esp32p4_psram_read_mode(8,budget,&m);
        if(result!=MAIA_PSRAM_OK)return result;
        if(m.value!=0x0BU)return MAIA_PSRAM_BUSY;
        r->before=r->after=m.value;r->stage=4;mode_claim=1;return MAIA_PSRAM_OK;
    }
    if(result!=MAIA_PSRAM_OK)return result;
    maia_psram_mode_report_t m;result=maia_esp32p4_psram_read_mode(8,budget,&m);
    if(result!=MAIA_PSRAM_OK)return result;
    if(m.value!=(restore?0x0BU:0x4BU))return MAIA_PSRAM_BUSY;
    r->before=m.value;r->stage=1;mode_claim=2;
    uint8_t desired=restore?0x4BU:0x0BU;maia_psram_transfer_report_t transfer;clear_transfer(&transfer);
    result=chunk(8,&desired,1,2,budget,&transfer);if(result!=MAIA_PSRAM_OK)return result;
    r->stage=2;result=maia_esp32p4_psram_read_mode(8,budget,&m);if(result!=MAIA_PSRAM_OK)return result;
    r->after=m.value;if(m.value!=desired)return MAIA_PSRAM_VERIFY;
    r->stage=3;mode_claim=restore?0U:1U;return MAIA_PSRAM_OK;
}
maia_psram_result_t maia_esp32p4_psram_boot_transfer(uint32_t offset,void *data,
    uint32_t length,unsigned write,uint32_t budget,maia_psram_transfer_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!data||!length||offset>=4096U||length>4096U-offset||(offset&3U)||(length&3U)||write>1U||!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(array_fault||mode_claim==2U||profile_state==2U)return MAIA_PSRAM_FAILED;
    if(mode_claim!=1U)return MAIA_PSRAM_BUSY;
    maia_psram_result_t checked=profile(0,budget);if(checked!=MAIA_PSRAM_OK)return checked;
    uint8_t *p=data;
    /* Port of reference's FIFO-sized streaming algorithm; subtraction bounds
     * address arithmetic before any I/O and preserves completed-prefix evidence. */
    while(r->completed<length){
        unsigned remaining=length-r->completed,n=remaining<64U?remaining:64U;
        maia_psram_result_t result=chunk(offset+r->completed,p+r->completed,n,write,budget,r);
        if(result!=MAIA_PSRAM_OK)return result;
        r->completed+=n;++r->chunks;
    }
    return MAIA_PSRAM_OK;
}

/* Native adaptation of s_init_psram_mode_reg, esp_psram_impl_ap_hex.c at
 * f5c3654. Read validated bytes separately instead of assuming an unqualified
 * MR4/8 paired read. Preserve MR1/other fields and keep X16 throughout profile
 * programming; the independently qualified X8 API handles interface handoff. */
static maia_psram_result_t read_profile(uint32_t out[3],unsigned budget,maia_psram_profile_report_t *r){
    static const unsigned regs[3]={0,4,8};
    for(unsigned i=0;i<3U;++i){maia_psram_mode_report_t m;maia_psram_result_t result=maia_esp32p4_psram_read_mode(regs[i],budget,&m);
        r->polls+=m.polls;if(result!=MAIA_PSRAM_OK)return result;out[i]=m.value;}
    return MAIA_PSRAM_OK;
}
static maia_psram_result_t write_profile(const uint32_t values[3],unsigned budget,maia_psram_profile_report_t *r){
    static const unsigned regs[3]={0,4,8};
    for(unsigned i=0;i<3U;++i){uint8_t bytes[2]={(uint8_t)values[i],(uint8_t)(i?values[2]:values[0]>>8U)};
        maia_psram_transfer_report_t transfer;clear_transfer(&transfer);r->write_attempts|=1U<<i;
        maia_psram_result_t result=chunk(regs[i],bytes,i==2U?1U:2U,2,budget,&transfer);
        r->polls+=transfer.polls;if(result!=MAIA_PSRAM_OK)return result;
    }
    return MAIA_PSRAM_OK;
}
maia_psram_result_t maia_esp32p4_psram_profile200(unsigned restore,uint32_t budget,maia_psram_profile_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile uint8_t *)r)[i]=0;
    if(restore>1U||!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(array_fault||profile_state==2U||mode_claim==2U)return MAIA_PSRAM_FAILED;
    if(mode_claim||profile_state!=(restore?1U:0U)||maia_psram_clock_mhz()!=20U)return MAIA_PSRAM_BUSY;
    maia_psram_result_t result=read_profile(r->before,budget,r);if(result!=MAIA_PSRAM_OK)return result;
    if(restore){for(unsigned i=0;i<3U;++i)if(r->before[i]!=configured_profile[i]){profile_state=2;return MAIA_PSRAM_CHANGED;}}
    else {
        maia_psram_mode_report_t identity;result=maia_esp32p4_psram_read_mode(2,budget,&identity);r->polls+=identity.polls;
        if(result!=MAIA_PSRAM_OK)return result;
        if((identity.value&255U)!=0xDFU||(r->before[2]&~0x4FU)||
           (!(r->before[2]&0x40U)&&!maia_psram_boot_claimed()))return MAIA_PSRAM_BUSY;
        for(unsigned i=0;i<3U;++i)original_profile[i]=r->before[i];
        configured_profile[0]=(r->before[0]&~0x3FU)|0x30U;
        configured_profile[1]=(r->before[1]&~0xE0U)|0x20U;
        configured_profile[2]=0x4BU;
    }
    r->stage=1;profile_state=2;
    const uint32_t *desired=restore?original_profile:configured_profile;
    result=write_profile(desired,budget,r);if(result!=MAIA_PSRAM_OK)return result;
    r->stage=2;result=read_profile(r->after,budget,r);if(result!=MAIA_PSRAM_OK)return result;
    for(unsigned i=0;i<3U;++i)if(r->after[i]!=desired[i])return MAIA_PSRAM_VERIFY;
    r->stage=3;r->restored=restore;profile_state=restore?0U:1U;return MAIA_PSRAM_OK;
}
