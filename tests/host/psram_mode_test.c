#include <assert.h>
#include <string.h>
#include "psram_mode_internal.h"
void maia_psram_mode_test_reset(void);
static uint32_t roots[9],words[27],extra[6],initial[27],initial_extra[6];
static unsigned context,reads,writes,drop,trigger_drop,active,polled,finish,stuck,bad_reply,clock_loss;
static const uintptr_t extras[6]={PSRAM_MSPI1+4U,PSRAM_MSPI1+0x28U,PSRAM_MSPI1+0x34U,PSRAM_MSPI1+0x3CU,PSRAM_MSPI1+0x58U,PSRAM_MSPI0+0x3CU};
int maia_psram_test_context(void){return (int)context;}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;for(unsigned i=0;i<9U;++i)if(p==psram_roots[i])return roots[i]^(i==0U&&active&&clock_loss?0x80000000U:0U);
    if(p==PSRAM_MSPI0)return active?1U:0U;
    if(p==PSRAM_MSPI1&&active){
        ++polled;
        if(!stuck&&polled>=finish){
            active=0;words[16]=0;uint32_t reg=extra[0];extra[4]=bad_reply?0xBEEFU:reg==0U?0x0D30U:reg==2U?0x20DFU:reg==4U?0x20U:0x4BU;
        }
    }
    for(unsigned i=0;i<27U;++i)if(p==psram_address(i))return words[i];
    for(unsigned i=0;i<6U;++i)if(p==extras[i])return extra[i];
    assert(!"non-whitelisted read");return 0;
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
    ++writes;assert(writes<=21U);
    if(p==PSRAM_MSPI1){
        assert(v==0x40000U&&!active);
        assert((words[17]&0x2270U)==0x2270U);assert((words[21]&0xFF00F000U)==0xF0000000U);
        assert((words[22]&0xFC00003FU)==0x7C00000BU);assert((words[23]&0xF000FFFFU)==0xF0004040U);
        assert((extra[1]&0x3FFU)==(extra[0]<4U?15U:7U));assert((extra[2]&0x403U)==1U);assert(extra[3]&2U);assert((words[24]&0x11U)==1U);
        assert(extra[4]==PSRAM_MODE_POISON);
        if(!trigger_drop){words[16]=v;active=1;}return;
    }
    assert(!active);if(writes==drop)return;
    unsigned found=0;for(unsigned i=0;i<10U;++i)if(p==psram_mode_address(i))found=1;assert(found);
    for(unsigned i=0;i<27U;++i)if(p==psram_address(i)){words[i]=v;return;}
    for(unsigned i=0;i<6U;++i)if(p==extras[i]){extra[i]=v;return;}
    assert(!"non-whitelisted write");
}
static void setup(void){
    context=1;reads=writes=drop=trigger_drop=active=polled=stuck=bad_reply=clock_loss=0;finish=3;maia_psram_mode_test_reset();
    for(unsigned i=0;i<9U;++i)roots[i]=0;
    roots[0]=0x80002000;roots[1]=0xD000;roots[4]=0x180;
    for(unsigned i=0;i<27U;++i)words[i]=0;
    words[1]=0x802C200C;words[2]=0x28E00000;words[17]=0x2CA00C;words[21]=0x80000000;words[22]=0x5C000007;words[23]=0x70000000;words[24]=0x20;
    for(unsigned i=0;i<6U;++i)extra[i]=0;
    extra[5]=0x80000000U;extra[2]=2;extra[4]=0x0D30U; /* stale plausible ID must never pass a dropped trigger */
    memcpy(initial,words,sizeof words);memcpy(initial_extra,extra,sizeof extra);
}
static void unchanged(void){assert(!memcmp(words,initial,sizeof words)&&!memcmp(extra,initial_extra,sizeof extra)&&!active);}
static void sticky(void){unsigned saved_reads=reads,saved_writes=writes;maia_psram_mode_report_t r;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_FAILED&&reads==saved_reads&&writes==saved_writes);}
int main(void){
    maia_psram_mode_report_t r;setup();assert(maia_esp32p4_psram_read_mode(0,1,0)==MAIA_PSRAM_INVALID&&!reads&&!writes);
    assert(maia_esp32p4_psram_read_mode(1,20,&r)==MAIA_PSRAM_INVALID&&!reads&&!writes);
    assert(maia_esp32p4_psram_read_mode(0,0,&r)==MAIA_PSRAM_INVALID&&!reads&&!writes);
    context=0;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_CONTEXT&&!reads&&!writes);
    setup();words[2]|=1;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_BUSY&&!writes);
    setup();roots[4]=0x80U;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_BUSY&&!writes); /* SW select is not XPD */
    setup();extra[5]=0x80000001U;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_BUSY&&!writes);
    setup();extra[5]=0U;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_BUSY&&!writes);
    setup();roots[0]=0;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_GATED&&!writes);
    unsigned regs[]={0,2,4,8};uint32_t expected[]={0x0D30,0x20DF,0x20,0x4B};
    setup();for(unsigned i=0;i<4U;++i){writes=polled=0;assert(maia_esp32p4_psram_read_mode(regs[i],20,&r)==MAIA_PSRAM_OK&&r.stage==5&&r.restored&&r.value==expected[i]&&r.setup_attempts==0x3FFU&&r.restore_attempts==0x3FFU&&r.triggered&&r.polls==3&&r.saw_busy);unchanged();}
    setup();finish=1;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_OK&&!r.saw_busy&&r.restored);unchanged();
    for(unsigned n=1;n<=21U;++n){
        if(n==11U)continue;
        setup();drop=n;maia_psram_result_t result=maia_esp32p4_psram_read_mode(0,20,&r);
        /* ADDR0 and restoring an unchanged prior ID word are unobservable; use known
         * changed setup/restore fields below rather than inventing an ACK. */
        if(n==1U||n==2U||n==3U||n==4U||n==6U||n==7U||n==8U||n==9U||n==10U||n==13U||n==14U||n==15U||n==16U||n==18U||n==19U||n==20U||n==21U){assert(result==MAIA_PSRAM_VERIFY);sticky();}
    }
    setup();trigger_drop=1;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_IO&&r.restored&&r.value!=0x0D30U);unchanged();sticky();
    setup();bad_reply=1;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_IO&&r.restored);unchanged();sticky();
    setup();stuck=1;assert(maia_esp32p4_psram_read_mode(0,3,&r)==MAIA_PSRAM_TIMEOUT&&r.polls==3&&!r.restored&&writes==11U&&active);sticky();
    setup();clock_loss=1;assert(maia_esp32p4_psram_read_mode(0,20,&r)==MAIA_PSRAM_VERIFY&&!r.restored&&writes==11U);sticky();
    return 0;
}
