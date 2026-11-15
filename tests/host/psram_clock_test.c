#include <assert.h>
#include <string.h>
#include "psram_internal.h"
void maia_psram_clock_test_reset(void);
static uint32_t roots[9],words[27],saved_roots[9],saved_words[27];
static unsigned reads,writes,context,drop,loss,mutation;
int maia_psram_test_context(void){return (int)context;}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;
    for(unsigned i=0;i<9U;++i)if(p==psram_roots[i])return roots[i];
    assert((roots[1]&0xC000U)==0xC000U); /* no access while gated */
    assert((roots[0]&0x80000000U)&&!(roots[2]&0x02800000U));
    for(unsigned i=0;i<27U;++i)if(p==psram_address(i))return words[i];
    assert(!"unexpected read");return 0;
}
void maia_psram_test_write(uintptr_t p,uint32_t v){
    ++writes;assert(writes<=10U);
    if(p==PSRAM_PERI){
        assert((v&~0xFFB000U)==(roots[1]&~0xFFB000U));
        if((v^roots[1])&0xFF3000U)assert(!(v&0x8000U)&&!(roots[1]&0x8000U));
        /* Before selecting original MPLL, both divisors must be restored. */
        if((v&0x3000U)==0x1000U)assert(words[7]==0x30103U&&words[20]==0x30103U);
        if(writes!=drop)roots[1]=v;
    }else{
        assert((roots[1]&0xFFB000U)==0x8000U); /* XTAL/div1, enabled */
        assert(v==0x10001U||v==0x30103U);
        assert(p==PSRAM_MSPI0+0x50U||p==PSRAM_MSPI1+0x14U);
        if(writes!=drop)words[p==PSRAM_MSPI0+0x50U?7U:20U]=v;
    }
    if(writes==loss)roots[0]^=0x80000000U;
    if(writes==mutation)words[3]^=1U;
}
static void setup(void){
    maia_psram_clock_test_reset();reads=writes=drop=loss=mutation=0;context=1;
    memset(roots,0,sizeof roots);memset(words,0,sizeof words);
    roots[0]=0xE6DFB7FF;roots[1]=0xD00C;roots[2]=0x38000040;
    roots[4]=0x40200180;roots[5]=0x76800000;roots[6]=0x7B054313;roots[7]=0x3A184DEA;
    words[1]=0x802C200C;words[2]=0x28E00000;words[7]=words[20]=0x30103;
    words[3]=0x55C070;words[17]=0x2CA00C;words[21]=0x80000000;
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
    return 0;
}
