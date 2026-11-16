#include <assert.h>
#include <string.h>
#include "psram_internal.h"
void maia_psram_array_test_reset(void);
void maia_psram_mode_test_reset(void);
static uint32_t root[9],reg[2][256],saved[2][256];
static uint8_t memory[4096];
static unsigned device_mr8;
static unsigned context,reads,writes,active,delay,stuck,drop,loss,commands,bad_profile,stuck_after;
static uint32_t *ptr(uintptr_t p){
    for(unsigned i=0;i<9;++i)if(p==psram_roots[i])return &root[i];
    assert((root[0]&0x80000000U)&&(root[1]&0xC000U)==0xC000U&&!(root[2]&0x02800000U));
    assert(p>=PSRAM_MSPI0&&p<PSRAM_MSPI1+0x400U&&!(p&3U));
    return &reg[p>=PSRAM_MSPI1][(p&0xFFFU)/4U];
}
int maia_psram_test_context(void){return (int)context;}
static void complete(void){
    uint32_t *c=reg[1],cmd=c[0x20/4]&0xFFFFU,offset=c[1];
    if(cmd==0x4040U){
        c[0x58/4]=offset==0U?(bad_profile?0x0D20U:0x8D30U):offset==2U?0xA0DFU:offset==4U?0x20U:device_mr8;
    }else if(cmd==0xC0C0U){
        assert(offset==8U&&(c[0x24/4]&0x3FFU)==7U);
        assert((c[0x18/4]&0xFF00F000U)==0xC8000000U);
        assert((c[0x1C/4]&0xFC00003FU)==0x7C000000U);
        device_mr8=c[0x58/4]&255U;assert(device_mr8==0x0BU||device_mr8==0x4BU);
    }else{
        assert(device_mr8==0x0BU); /* no byte aperture while X16 */
        unsigned write=cmd==0x8080U;
        assert(cmd==0x8080U||cmd==0U);
        unsigned length=((c[(write?0x24:0x28)/4]&0x3FFU)+1U)/8U;
        assert(length&&length<=64U&&!(length&3U)&&offset+length<=4096U);
        assert((c[0x18/4]&0xFF00F000U)==(write?0xE8000000U:0xF0000000U));
        assert((c[0x1C/4]&0xFC00003FU)==(write?0x7C00000BU:0x7C000019U));
        assert((c[8/4]&0x2270U)==0x2270U&&(c[0x34/4]&0x403U)==1U);
        for(unsigned i=0;i<length;++i){unsigned word=0x58/4+i/4,shift=(i%4)*8;
            if(write)memory[offset+i]=(uint8_t)(c[word]>>shift);
            else c[word]=(c[word]&~(255U<<shift))|((uint32_t)memory[offset+i]<<shift);
        }
    }
    active=0;c[0]=0;
}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;
    if(p==PSRAM_MSPI1&&active&&!stuck&&!--delay)complete();
    return *ptr(p);
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
    ++writes;if(writes==drop)return;
    if(p==PSRAM_MSPI1){assert(!active&&v==0x40000U);active=1;delay=2;++commands;if(commands==stuck_after)stuck=1;}
    else assert(!active);
    *ptr(p)=v;
    if(writes==loss)root[0]=0;
}
static void setup(void){
    maia_psram_array_test_reset();maia_psram_mode_test_reset();context=1;
    reads=writes=active=stuck=drop=loss=commands=bad_profile=stuck_after=0;
    memset(root,0,sizeof root);memset(reg,0,sizeof reg);
    root[0]=0x80000000;root[1]=0xC00C;root[4]=0x180;
    reg[0][0x50/4]=reg[1][0x14/4]=0x10001;
    reg[1][8/4]=0x2CA00C;reg[1][0x18/4]=0x80000000;reg[1][0x1C/4]=0x5C000007;
    reg[1][0x20/4]=0x70000000;reg[1][0x34/4]=2;reg[1][0xD4/4]=0x20;
    for(unsigned i=0;i<16;++i)reg[1][0x58/4+i]=0x10203040U+i;
    for(unsigned i=0;i<4096;++i)memory[i]=(uint8_t)(i*17U+(i>>8U));
    memcpy(saved,reg,sizeof reg);device_mr8=0x4BU;
}
static void setup_array(void){
    setup();maia_psram_mode_switch_report_t r;
    assert(maia_esp32p4_psram_x8_mode(0,10,&r)==MAIA_PSRAM_OK&&r.before==0x4BU&&r.after==0x0BU&&r.stage==3U);
    assert(!memcmp(saved,reg,sizeof reg));reads=writes=commands=0;
}
static void sticky(void){maia_psram_transfer_report_t r;uint32_t data=0;unsigned n=reads,w=writes;
    assert(maia_esp32p4_psram_boot_transfer(0,&data,4,0,10,&r)==MAIA_PSRAM_FAILED&&reads==n&&writes==w);
}
int main(void){
    uint8_t data[4096],copy[4096];maia_psram_transfer_report_t r;setup();
    assert(maia_esp32p4_psram_boot_transfer(0,data,64,0,10,0)==MAIA_PSRAM_INVALID&&!reads);
    const unsigned offsets[]={1,4096,0xFFFFFFFCU},lengths[]={0,3,4100};
    for(unsigned i=0;i<3;++i){assert(maia_esp32p4_psram_boot_transfer(offsets[i],data,64,0,10,&r)==MAIA_PSRAM_INVALID&&!reads);assert(maia_esp32p4_psram_boot_transfer(0,data,lengths[i],0,10,&r)==MAIA_PSRAM_INVALID&&!reads);}
    assert(maia_esp32p4_psram_boot_transfer(4092,data,8,0,10,&r)==MAIA_PSRAM_INVALID&&!reads);
    context=0;assert(maia_esp32p4_psram_boot_transfer(0,data,64,0,10,&r)==MAIA_PSRAM_CONTEXT&&!reads);
    setup_array();bad_profile=1;assert(maia_esp32p4_psram_boot_transfer(0,data,64,1,10,&r)==MAIA_PSRAM_BUSY&&!r.write_attempted);
    setup_array();memcpy(copy,memory,sizeof copy);
    assert(maia_esp32p4_psram_boot_transfer(0,data,4096,0,10,&r)==MAIA_PSRAM_OK&&r.completed==4096&&r.chunks==64&&r.restored&&!memcmp(data,copy,4096)&&!memcmp(saved,reg,sizeof reg));
    for(unsigned n=4;n<=132;n+=4){setup_array();for(unsigned i=0;i<n;++i)data[i]=(uint8_t)(255U-i);
        assert(maia_esp32p4_psram_boot_transfer(4096U-n,data,n,1,10,&r)==MAIA_PSRAM_OK&&r.completed==n&&r.chunks==(n+63U)/64U&&r.write_attempted);
        assert(!memcmp(memory+4096U-n,data,n)&&!memcmp(saved,reg,sizeof reg));
        memset(copy,0,sizeof copy);assert(maia_esp32p4_psram_boot_transfer(4096U-n,copy,n,0,10,&r)==MAIA_PSRAM_OK&&!memcmp(data,copy,n));
    }
    /* Four mode reads use84 stores, then26 setup/start/26 restore. Drop changed
     * read-command fields/receive poison and every changed restore field. */
    for(unsigned n=85;n<=137;++n){
        setup_array();drop=n;memset(data,0xCC,64);
        maia_psram_result_t result=maia_esp32p4_psram_boot_transfer(64,data,64,0,10,&r);
        if(result!=MAIA_PSRAM_OK){assert(result==MAIA_PSRAM_VERIFY);assert(!r.completed&&data[0]==0xCCU);sticky();}
    }
    setup_array(); /* Make only the array command stick, by dropping its completion. */
    drop=111;memset(data,0xCC,64);
    /* Read drop is not an observable transport ACK: poison is returned; the
     * caller's pattern comparison must reject it. Never claim device ACK. */
    assert(maia_esp32p4_psram_boot_transfer(64,data,64,0,10,&r)==MAIA_PSRAM_OK&&data[0]==0xA5U);
    setup_array();stuck_after=6;memset(data,0xCC,128);
    assert(maia_esp32p4_psram_boot_transfer(0,data,128,0,3,&r)==MAIA_PSRAM_TIMEOUT&&r.completed==64U&&r.chunks==1U&&!r.restored&&active);
    assert(!memcmp(data,memory,64)&&data[64]==0xCCU);sticky();
    setup_array();drop=111;memset(data,0x5A,64);
    assert(maia_esp32p4_psram_boot_transfer(64,data,64,1,10,&r)==MAIA_PSRAM_OK&&r.write_attempted&&memcmp(data,memory+64,64)); /* needs device readback */
    setup_array();loss=90;assert(maia_esp32p4_psram_boot_transfer(0,data,64,0,10,&r)==MAIA_PSRAM_VERIFY&&!r.completed);sticky();
    setup();maia_psram_mode_switch_report_t mode;
    assert(maia_esp32p4_psram_boot_transfer(0,data,64,0,10,&r)==MAIA_PSRAM_BUSY&&!reads);
    assert(maia_esp32p4_psram_x8_mode(1,10,&mode)==MAIA_PSRAM_BUSY&&!reads);
    assert(maia_esp32p4_psram_x8_mode(0,10,&mode)==MAIA_PSRAM_OK);
    unsigned n=reads;assert(maia_esp32p4_psram_x8_mode(0,10,&mode)==MAIA_PSRAM_BUSY&&reads==n);
    assert(maia_esp32p4_psram_x8_mode(1,10,&mode)==MAIA_PSRAM_OK&&mode.after==0x4BU&&!memcmp(saved,reg,sizeof reg));
    setup();drop=132;assert(maia_esp32p4_psram_x8_mode(0,10,&mode)==MAIA_PSRAM_VERIFY&&mode.after==0x4BU);sticky();
    return 0;
}
