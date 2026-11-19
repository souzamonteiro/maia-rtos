#include <assert.h>
#include <string.h>
#include "psram_window_internal.h"
void maia_psram_window_test_reset(void);
void maia_psram_mode_test_reset(void);
static uint32_t root[9],reg[2][256],pins[20],mmu[1024],initial_mmu[1024],initial_reg[2][256],initial_pins[20],index_word;
static unsigned reads,writes,context,claims,active,rom_calls,rom_fail,drop_mmu,drop_reg,loss_reg,busy,sync_busy;
static uint32_t crypt_word;
static unsigned clock_rate,dma_active;
static uint32_t dma_words[0x500U/4U],dma_clock;
int maia_psram_test_context(void){return (int)context;}
int maia_psram_x8_claimed(void){return (int)claims;}
int maia_psram_clock_claimed(void){return (int)claims;}
unsigned maia_psram_clock_mhz(void){return claims?clock_rate:0U;}
static uint32_t *pointer(uintptr_t p){
    for(unsigned i=0;i<9U;++i)if(p==psram_roots[i])return &root[i];
    if(p==WINDOW_CRYPT)return &crypt_word;
    if(p==MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG)return &dma_clock;
    if(p>=MAIA_ESP32P4_GDMA_BASE&&p<MAIA_ESP32P4_GDMA_BASE+0x500U)return &dma_words[(p-MAIA_ESP32P4_GDMA_BASE)/4U];
    assert((root[0]&0x80000000U)&&(root[1]&0xC000U)==0xC000U);
    for(unsigned i=0;i<20U;++i)if(p==window_plan[i].address)return &pins[i];
    if(p==WINDOW_MMU_INDEX)return &index_word;
    if(p==WINDOW_MMU_CONTENT){assert(index_word<1024U);return &mmu[index_word];}
    assert(p>=PSRAM_MSPI0&&p<PSRAM_MSPI1+0x400U&&!(p&3U));
    return &reg[p>=PSRAM_MSPI1][(p&0xFFFU)/4U];
}
uint32_t maia_psram_test_read(uintptr_t p){
    ++reads;
    if(p==MAIA_ESP32P4_GDMA_BASE+0x10U&&dma_active)return 1;
    if(p==PSRAM_MSPI1&&active){active=0;reg[1][0]=0;reg[1][0x58/4]=0x0BU;}
    if(p==WINDOW_FIFO&&busy)return 0;
    return *pointer(p);
}
void maia_psram_test_write(uintptr_t p,uint32_t value){
    ++writes;
    if(p==WINDOW_MMU_CONTENT)assert(!(reg[0][0x3C/4]&1U));
    if(p==PSRAM_MSPI1){assert(!active&&value==0x40000U);active=1;}
    if(p==WINDOW_MMU_CONTENT&&index_word==drop_mmu)return;
    for(unsigned i=0;i<WINDOW_REGS;++i)if(p==window_plan[i].address&&i==drop_reg)return;
    if(p==PSRAM_MSPI0+0x3CU&&(value&1U)){
        for(unsigned i=0;i<32U;++i)assert(mmu[896U+i]==0xC00U+i);
        for(unsigned i=0;i<27U;++i)assert(*pointer(window_plan[i].address)==((i<20U?initial_pins[i]:initial_reg[0][(window_plan[i].address&0xFFFU)/4U])&~window_plan[i].mask|window_plan[i].value));
    }
    *pointer(p)=value;
    for(unsigned i=0;i<WINDOW_REGS;++i)if(p==window_plan[i].address&&i==loss_reg)root[0]=0;
}
int maia_psram_window_test_rom(unsigned op,void *arg){
    ++rom_calls;if(op==rom_fail)return -1;
    if(op==3U){struct mode{uint32_t size;uint16_t line;uint8_t ways;uint32_t type;} *m=arg;assert(m->type==5U);m->size=131072U;m->line=128;m->ways=8;}
    if(op==4U){assert((reg[0][0x3C/4]&0x80000001U)==1U);if(sync_busy)busy=1;}
    return 0;
}
static void setup(void){
    maia_psram_window_test_reset();maia_psram_mode_test_reset();
    reads=writes=active=rom_calls=busy=sync_busy=0;context=claims=1;clock_rate=20;dma_active=0;dma_clock=0x20U;memset(dma_words,0,sizeof dma_words);
    drop_mmu=drop_reg=loss_reg=rom_fail=~0U;index_word=7;crypt_word=0;
    memset(root,0,sizeof root);memset(reg,0,sizeof reg);
    root[0]=0x80000000;root[1]=0xC00C;root[4]=0x180;
    reg[0][0x3C/4]=0xC0000000U;reg[0][0xC/4]=0x28E00000;
    reg[0][0x40/4]=0x55C070;reg[0][0x44/4]=0x80C00000;
    reg[0][0x50/4]=reg[1][0x14/4]=0x10001;
    reg[0][0xD8/4]=0x3020;reg[0][0x150/4]=4096;
    reg[0][0x178/4]=0xFC000000;
    for(unsigned i=0;i<4U;++i)reg[0][0x130/4+i]=3;
    reg[1][8/4]=0x2CA00C;reg[1][0x18/4]=0x80000000;
    reg[1][0x1C/4]=0x5C000007;reg[1][0x20/4]=0x70000000;reg[1][0x34/4]=2;reg[1][0xD4/4]=0x20;
    for(unsigned i=0;i<20U;++i)pins[i]=0x400U;
    for(unsigned i=0;i<1024U;++i)mmu[i]=i<512U?0xC00U+i:0;
    memcpy(initial_reg,reg,sizeof reg);memcpy(initial_pins,pins,sizeof pins);memcpy(initial_mmu,mmu,sizeof mmu);
}
static void cache_ready(void){maia_psram_cache_report_t r;assert(maia_esp32p4_psram_cache_boot(&r)==MAIA_PSRAM_OK&&r.stage==3U&&r.size==131072U&&rom_calls==4U);}
static void unchanged(void){assert(!memcmp(reg,initial_reg,sizeof reg)&&!memcmp(pins,initial_pins,sizeof pins)&&!memcmp(mmu,initial_mmu,sizeof mmu)&&index_word==7U);}
static void sticky(void){maia_psram_window_report_t r;unsigned n=reads,w=writes;assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_FAILED&&reads==n&&writes==w);}
int main(void){
    maia_psram_cache_report_t c;maia_psram_window_report_t r;setup();
    assert(maia_esp32p4_psram_window_start(3,0)==MAIA_PSRAM_INVALID&&!reads);
    assert(maia_esp32p4_psram_window_start(0,&r)==MAIA_PSRAM_INVALID&&!reads);
    assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_BUSY&&!reads);
    context=0;assert(maia_esp32p4_psram_cache_boot(&c)==MAIA_PSRAM_CONTEXT&&!reads&&!rom_calls);
    setup();crypt_word=1U<<18U;assert(maia_esp32p4_psram_cache_boot(&c)==MAIA_PSRAM_BUSY&&!rom_calls);
    setup();crypt_word=3U<<18U;cache_ready();
    setup();cache_ready();unsigned n=reads;assert(maia_esp32p4_psram_cache_boot(&c)==MAIA_PSRAM_BUSY&&reads==n);
    assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK&&r.stage==4&&r.vaddr==0x4B800000U&&r.bytes==0x200000U);
    n=reads;assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_BUSY&&reads==n);
    assert(maia_esp32p4_psram_window_sync(3,&r)==MAIA_PSRAM_OK&&r.stage==5);
    assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_OK&&r.stage==6&&r.restored);unchanged();
    assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK);assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_OK);unchanged();
    for(unsigned i=0;i<4U;++i){setup();rom_fail=i;assert(maia_esp32p4_psram_cache_boot(&c)==MAIA_PSRAM_IO);n=reads;assert(maia_esp32p4_psram_cache_boot(&c)==MAIA_PSRAM_FAILED&&reads==n);}
    /* XIP adopt: only the cache-mode query reaches ROM, then the window works. */
    setup();context=0;assert(maia_esp32p4_psram_cache_adopt(&c)==MAIA_PSRAM_CONTEXT&&!reads&&!rom_calls);
    setup();crypt_word=1U<<18U;assert(maia_esp32p4_psram_cache_adopt(&c)==MAIA_PSRAM_BUSY&&!rom_calls);
    setup();assert(maia_esp32p4_psram_cache_adopt(&c)==MAIA_PSRAM_OK&&c.stage==3U&&c.size==131072U&&rom_calls==1U);
    assert(maia_esp32p4_psram_cache_adopt(&c)==MAIA_PSRAM_BUSY&&maia_esp32p4_psram_cache_boot(&c)==MAIA_PSRAM_BUSY&&rom_calls==1U);
    assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK&&maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_OK);unchanged();
    setup();rom_fail=3;assert(maia_esp32p4_psram_cache_adopt(&c)==MAIA_PSRAM_IO);assert(maia_esp32p4_psram_cache_adopt(&c)==MAIA_PSRAM_FAILED);
    setup();cache_ready();busy=1;assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_TIMEOUT&&r.polls==3U);sticky();
    setup();cache_ready();drop_mmu=900;assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_VERIFY&&!(reg[0][0x3C/4]&1U));sticky();
    for(unsigned i=0;i<28U;++i){
        setup();cache_ready();drop_reg=i;
        assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_VERIFY);sticky();
    }
    setup();cache_ready();loss_reg=0;assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_VERIFY);sticky();
    setup();cache_ready();assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK);sync_busy=1;
    assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_TIMEOUT&&r.polls==3U&&(reg[0][0x3C/4]&1U));sticky();
    setup();cache_ready();assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK);rom_fail=4;
    assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_VERIFY&&(reg[0][0x3C/4]&1U));sticky();
    setup();cache_ready();assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK);drop_reg=27;
    assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_VERIFY&&(reg[0][0x3C/4]&1U));sticky();
    setup();cache_ready();claims=0;n=reads;assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_BUSY&&reads==n);
    for(unsigned rate=100;rate<=200;rate+=100){
        setup();clock_rate=rate;root[1]=0xD00CU;reg[0][0x50/4]=reg[1][0x14/4]=rate==100U?0x30103U:0x10001U;
        memcpy(initial_reg,reg,sizeof reg);cache_ready();
        assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK);
        assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_OK);unchanged();
    }
    setup();cache_ready();clock_rate=200;assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_BUSY);
    setup();cache_ready();clock_rate=80;assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_BUSY);
    setup();clock_rate=200;root[0]|=0x2000U;root[1]=0xD00CU;cache_ready();
    assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK);
    dma_active=1;assert(!maia_psram_window_dma_acquire());dma_active=0;
    assert(maia_psram_window_dma_acquire()==0x4B800080U&&maia_psram_window_dma_live());
    n=reads;assert(!maia_psram_window_dma_acquire()&&reads==n);
    assert(maia_esp32p4_psram_window_sync(3,&r)==MAIA_PSRAM_BUSY&&reads==n);
    assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_BUSY&&reads==n);
    unsigned before_sync=rom_calls;
    assert(maia_psram_window_edit_begin()==0x4B800080U&&!maia_psram_window_dma_live());
    assert(!maia_psram_window_edit_begin()&&!maia_psram_window_dma_release()&&!maia_psram_window_dma_acquire());
    assert(maia_esp32p4_psram_window_sync(3,&r)==MAIA_PSRAM_BUSY);
    assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_BUSY);
    assert(maia_psram_window_edit_publish()&&maia_psram_window_dma_live()&&rom_calls==before_sync+1);
    assert(!maia_psram_window_edit_publish());
    assert(maia_psram_window_dma_release()&&!maia_psram_window_dma_live());
    assert(maia_esp32p4_psram_window_stop(3,&r)==MAIA_PSRAM_OK);
    setup();clock_rate=200;root[0]|=0x2000U;root[1]=0xD00CU;cache_ready();
    assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK&&maia_psram_window_dma_acquire());
    dma_active=1;assert(!maia_psram_window_dma_release());sticky();
    setup();clock_rate=200;root[0]|=0x2000U;root[1]=0xD00CU;cache_ready();
    assert(maia_esp32p4_psram_window_start(3,&r)==MAIA_PSRAM_OK&&maia_psram_window_dma_acquire());
    dma_active=1;assert(!maia_psram_window_edit_begin());dma_active=0;
    assert(maia_psram_window_edit_begin());rom_fail=4;assert(!maia_psram_window_edit_publish());
    assert(!maia_psram_window_dma_release());sticky();
    return 0;
}
