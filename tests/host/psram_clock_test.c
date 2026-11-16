#include <assert.h>
#include <string.h>
#include "psram_window_internal.h"
void maia_psram_clock_test_reset(void);
static uint32_t roots[9],words[27],saved_roots[9],saved_words[27];
static uint32_t axi_control,pads[20],initial_pads[20];
static unsigned pin_writes,drop_pad;
static unsigned reads,writes,context,drop,loss,mutation,fast_test,x8_claim;
static maia_psram_result_t mpll_result;
static unsigned mpll_mhz;
int maia_psram_x8_claimed(void){return (int)x8_claim;}
maia_psram_result_t maia_psram_mpll_read(uint32_t budget,maia_psram_mpll_report_t *r){
    assert(budget==10000U);r->mhz=mpll_mhz;r->restored=1;r->stage=3;return mpll_result;
}
int maia_psram_test_context(void){return (int)context;}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;
    for(unsigned i=0;i<9U;++i)if(p==psram_roots[i])return roots[i];
    for(unsigned i=0;i<20;++i)if(p==window_plan[i].address)return pads[i];
    assert((roots[1]&0xC000U)==0xC000U); /* no access while gated */
    assert((roots[0]&0x80000000U)&&!(roots[2]&0x02800000U));
    if(p==PSRAM_MSPI0+0x3CU)return axi_control;
    for(unsigned i=0;i<27U;++i)if(p==psram_address(i))return words[i];
    assert(!"unexpected read");return 0;
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
    for(unsigned i=0;i<20;++i)if(p==window_plan[i].address){
        ++pin_writes;if(pin_writes!=drop_pad)pads[i]=v;return;
    }
    ++writes;assert(writes<=30U);
    if(p==PSRAM_PERI){
        assert((v&~0xFFB000U)==(roots[1]&~0xFFB000U));
        if((v^roots[1])&0xFF3000U)assert(!(v&0x8000U)&&!(roots[1]&0x8000U));
        /* Before selecting original MPLL, both divisors must be restored. */
        if(fast_test&&pin_writes&&maia_psram_clock_mhz()!=0U&&pin_writes<40U&&(v&0xFFB000U)==0x9000U){for(unsigned i=0;i<20;++i)assert((pads[i]&~((i==8U||i==19U)?6U:0U))==(((initial_pads[i]&~window_plan[i].mask)|window_plan[i].value)&~((i==8U||i==19U)?6U:0U)));}
        if((v&0x3000U)==0x1000U)assert((fast_test?words[7]==words[20]:words[7]==0x30103U&&words[20]==0x30103U));
        if(writes!=drop)roots[1]=v;
    }else if(p==PSRAM_MSPI0+0x180U||p==PSRAM_MSPI0+0x190U){
        assert(fast_test&&(v==1U||v==0x21U));
        if(writes!=drop)words[p==PSRAM_MSPI0+0x180U?9U:13U]=v;
    }else{
        assert(fast_test?((roots[1]&0xFFB000U)==0x8000U||(roots[1]&0xFFB000U)==0x9000U):(roots[1]&0xFFB000U)==0x8000U); /* XTAL/div1, enabled */
        assert(v==0x10001U||v==0x30103U);
        assert(p==PSRAM_MSPI0+0x50U||p==PSRAM_MSPI1+0x14U);
        if(writes!=drop)words[p==PSRAM_MSPI0+0x50U?7U:20U]=v;
    }
    if(writes==loss)roots[0]^=0x80000000U;
    if(writes==mutation)words[3]^=1U;
}
static void setup(void){
    maia_psram_clock_test_reset();pin_writes=drop_pad=0;for(unsigned i=0;i<20;++i)initial_pads[i]=pads[i]=0x400U;fast_test=0;x8_claim=1;mpll_result=MAIA_PSRAM_OK;mpll_mhz=400;reads=writes=drop=loss=mutation=0;context=1;axi_control=0x80000000U;
    memset(roots,0,sizeof roots);memset(words,0,sizeof words);
    roots[0]=0xE6DFB7FF;roots[1]=0xD00C;roots[2]=0x38000040;
    roots[4]=0x40200180;roots[5]=0x76800000;roots[6]=0x7B054313;roots[7]=0x3A184DEA;
    words[1]=0x802C200C;words[2]=0x28E00000;words[7]=words[20]=0x30103;
    words[9]=words[13]=1U;words[3]=0x55C070;words[17]=0x2CA00C;words[21]=0x80000000;
    memcpy(saved_roots,roots,sizeof roots);memcpy(saved_words,words,sizeof words);
}
static void sticky(void){
    unsigned before_reads=reads,before_writes=writes;maia_psram_clock_report_t r;
    assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_FAILED);
    assert(maia_esp32p4_psram_xtal20_stop(&r)==MAIA_PSRAM_FAILED);
    assert(reads==before_reads&&writes==before_writes);
}
int main(void){
    maia_psram_clock_report_t r;
    setup();assert(maia_esp32p4_psram_xtal20_start(0)==MAIA_PSRAM_INVALID&&!reads&&!writes);
    assert(maia_esp32p4_psram_xtal20_stop(&r)==MAIA_PSRAM_BUSY&&!reads&&!writes);
    context=0;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_CONTEXT&&!reads&&!writes);
    setup();roots[0]=0;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_GATED&&!writes);
    setup();axi_control=0x80000001U;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_BUSY&&!writes);
    setup();axi_control=0U;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_BUSY&&!writes);
    for(unsigned variant=0;variant<13U;++variant){
        setup();
        switch(variant){
        case 0:words[0]=1;break;case 1:words[16]=1;break;case 2:words[2]|=1;break;
        case 3:roots[4]&=~0x100U;break;case 4:roots[4]|=0x4000;break;case 5:roots[4]|=0x200;break;
        case 6:roots[5]^=1U<<28U;break;case 7:roots[6]&=0xFFFFFFFU;break;
        case 8:roots[8]=8;break;case 9:roots[6]^=1;break;case 10:roots[1]|=0x10000;break;
        case 11:words[7]=0x10001;break;default:words[20]=0x10001;break;
        }
        assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_BUSY&&!writes);
    }
    for(unsigned cycle=0;cycle<2U;++cycle){
        if(!cycle)setup();writes=0;
        assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK&&r.stage==5&&r.attempts==31U&&!r.restored&&writes==5U);
        assert(roots[1]==0xC00CU&&words[7]==0x10001U&&words[20]==0x10001U);
        unsigned old_reads=reads;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_BUSY&&reads==old_reads&&writes==5U);
        assert(maia_esp32p4_psram_xtal20_stop(&r)==MAIA_PSRAM_OK&&r.stage==5&&r.attempts==31U&&r.restored&&writes==10U);
        assert(!memcmp(roots,saved_roots,sizeof roots)&&!memcmp(words,saved_words,sizeof words));
    }
    for(unsigned failure=1;failure<=10U;++failure){
        setup();drop=failure;
        if(failure<=5U)assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_VERIFY);
        else{assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);assert(maia_esp32p4_psram_xtal20_stop(&r)==MAIA_PSRAM_VERIFY&&!r.restored);}
        assert(writes==failure);sticky();
    }
    for(unsigned failure=1;failure<=10U;++failure){
        setup();loss=failure;
        if(failure<=5U)assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_VERIFY);
        else{assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);assert(maia_esp32p4_psram_xtal20_stop(&r)==MAIA_PSRAM_VERIFY);}
        assert(writes==failure);sticky();
    }
    setup();mutation=2;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_VERIFY&&writes==3U);sticky(); /* detected when reenabled */
    setup();assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);words[16]=0x40000;
    assert(maia_esp32p4_psram_xtal20_stop(&r)==MAIA_PSRAM_VERIFY&&writes==5U&&!r.restored);sticky();
    maia_psram_fast_report_t fast;
    setup();assert(maia_esp32p4_psram_fast_start(200,10000,&fast)==MAIA_PSRAM_BUSY&&!reads&&!writes);
    assert(maia_esp32p4_psram_fast_start(201,10000,&fast)==MAIA_PSRAM_INVALID&&!reads&&!writes);
    assert(maia_esp32p4_psram_fast_stop(&r)==MAIA_PSRAM_BUSY&&!reads&&!writes);
    for(unsigned rate=100;rate<=200;rate+=100){
        setup();fast_test=1;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);
        x8_claim=0;unsigned old=reads;assert(maia_esp32p4_psram_fast_start(rate,10000,&fast)==MAIA_PSRAM_BUSY&&reads==old);x8_claim=1;
        assert(maia_esp32p4_psram_fast_start(rate,10000,&fast)==MAIA_PSRAM_OK&&fast.mhz==rate&&fast.mpll.mhz==400U&&writes==12);
        assert(words[9]==0x21U&&words[13]==0x21U);
        assert(maia_psram_clock_claimed()&&maia_psram_clock_mhz()==rate&&roots[1]==0xD00CU&&words[7]==(rate==100?0x30103U:0x10001U));
        old=reads;assert(maia_esp32p4_psram_xtal20_stop(&r)==MAIA_PSRAM_BUSY&&reads==old);
        assert(maia_esp32p4_psram_fast_start(rate,10000,&fast)==MAIA_PSRAM_BUSY&&reads==old);
        assert(maia_esp32p4_psram_fast_stop(&r)==MAIA_PSRAM_OK&&r.restored&&writes==19&&maia_psram_clock_mhz()==20U&&pin_writes==40U);
        assert(!memcmp(pads,initial_pads,sizeof pads));
        assert(roots[1]==0xC00CU&&words[7]==0x10001U&&words[20]==0x10001U);
        assert(maia_esp32p4_psram_xtal20_stop(&r)==MAIA_PSRAM_OK&&writes==24);
        assert(!memcmp(roots,saved_roots,sizeof roots)&&!memcmp(words,saved_words,sizeof words));
    }
    for(unsigned failure=6;failure<=19;++failure){
        setup();fast_test=1;drop=failure;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);
        if(failure<=12)assert(maia_esp32p4_psram_fast_start(100,10000,&fast)==MAIA_PSRAM_VERIFY);
        else{assert(maia_esp32p4_psram_fast_start(100,10000,&fast)==MAIA_PSRAM_OK);assert(maia_esp32p4_psram_fast_stop(&r)==MAIA_PSRAM_VERIFY);}
        assert(writes==failure);sticky();
        unsigned old=reads;assert(maia_esp32p4_psram_fast_start(100,10000,&fast)==MAIA_PSRAM_FAILED&&reads==old);
        assert(maia_esp32p4_psram_fast_stop(&r)==MAIA_PSRAM_FAILED&&reads==old);
    }
    setup();fast_test=1;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);mpll_mhz=360;
    assert(maia_esp32p4_psram_fast_start(200,10000,&fast)==MAIA_PSRAM_VERIFY&&writes==5);sticky();
    setup();fast_test=1;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);mpll_result=MAIA_PSRAM_TIMEOUT;
    assert(maia_esp32p4_psram_fast_start(200,10000,&fast)==MAIA_PSRAM_TIMEOUT&&writes==5);sticky();
    for(unsigned failure=1;failure<=40;++failure){
        setup();fast_test=1;drop_pad=failure;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);
        if(failure<=20)assert(maia_esp32p4_psram_fast_start(200,10000,&fast)==MAIA_PSRAM_VERIFY);
        else{assert(maia_esp32p4_psram_fast_start(200,10000,&fast)==MAIA_PSRAM_OK);assert(maia_esp32p4_psram_fast_stop(&r)==MAIA_PSRAM_VERIFY);}
        assert(pin_writes==failure);sticky();
    }
    setup();fast_test=1;assert(maia_psram_fast_phase(4)==MAIA_PSRAM_INVALID&&!reads);
    assert(maia_psram_fast_phase(0)==MAIA_PSRAM_BUSY&&!reads);
    assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);
    assert(maia_esp32p4_psram_fast_start(200,10000,&fast)==MAIA_PSRAM_OK);
    for(unsigned phase=0;phase<4U;++phase){assert(maia_psram_fast_phase(phase)==MAIA_PSRAM_OK);assert((pads[8]&6U)==phase*2U&&(pads[19]&6U)==phase*2U);}
    assert(maia_esp32p4_psram_fast_stop(&r)==MAIA_PSRAM_OK&&!memcmp(pads,initial_pads,sizeof pads));
    for(unsigned failure=21;failure<=22;++failure){
        setup();fast_test=1;assert(maia_esp32p4_psram_xtal20_start(&r)==MAIA_PSRAM_OK);
        assert(maia_esp32p4_psram_fast_start(200,10000,&fast)==MAIA_PSRAM_OK);drop_pad=failure;
        assert(maia_psram_fast_phase(3)==MAIA_PSRAM_VERIFY&&pin_writes==failure);
        unsigned old=reads;assert(maia_psram_fast_phase(1)==MAIA_PSRAM_FAILED&&reads==old);
    }
    return 0;
}
