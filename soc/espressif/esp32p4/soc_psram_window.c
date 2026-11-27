/* SPDX-FileCopyrightText: 2021-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Native adaptation of pinned ESP-IDF f5c3654 HEX s_config_mspi_for_psram,
 * mspi_timing LL pad/CS setup, mmu_hal_map_region/mmu_ll and cache_hal_init.
 * Modified: fixed owned4MiB window, exact native guards/readback/restoration,
 * bounded FIFO waits, private boot claims, no allocator/SDK/RTOS dependencies.
 * Immutable ROM cache API bindings from esp32p4.rom.ld/cache.h; no ROM SPI calls.
 * License: LICENSES/Apache-2.0.txt. */
#include "psram_window_internal.h"
typedef struct {uint32_t size;uint16_t line;uint8_t ways;uint32_t type;} cache_mode;
enum { FREE,OWNED,FAULTED,DMA_OWNED,DMA_EDIT_OWNED };
static unsigned cache_state,window_state;
static maia_psram_snapshot_t reference;
static uint32_t saved[WINDOW_REGS],current[WINDOW_REGS],table[WINDOW_MMU_PAGES],saved_index,crypt,ecc,pms[4],pms_base,pms_size;
#ifdef MAIA_PSRAM_TEST
int maia_psram_window_test_rom(unsigned,void *);
void maia_psram_window_test_reset(void);
void maia_psram_window_test_reset(void){cache_state=window_state=FREE;}
static int cache_call(unsigned op,cache_mode *mode){return maia_psram_window_test_rom(op,mode);}
#else
static int cache_call(unsigned op,cache_mode *mode){
    if(op==0U)((void(*)(void))(uintptr_t)0x4FC003DCU)();
    else if(op==1U)((void(*)(uint32_t))(uintptr_t)0x4FC004F4U)(0);
    else if(op==2U)((void(*)(uint32_t))(uintptr_t)0x4FC00504U)(0);
    else if(op==3U)((void(*)(cache_mode *))(uintptr_t)0x4FC003D0U)(mode);
    else if(op==4U)return ((int(*)(uint32_t,uint32_t,uint32_t))(uintptr_t)0x4FC003FCU)(0x30U,MAIA_PSRAM_WINDOW_VADDR,MAIA_PSRAM_WINDOW_BYTES);
    else return ((int(*)(uint32_t,uint32_t,uint32_t))(uintptr_t)0x4FC003E4U)(0x30U,MAIA_PSRAM_WINDOW_VADDR,MAIA_PSRAM_WINDOW_BYTES);
    return 0;
}
#endif
static int cache_range(uint32_t address,uint32_t bytes){
#ifdef MAIA_PSRAM_TEST
    maia_psram_cache_range_test_t range={address,bytes};
    return maia_psram_window_test_rom(6U,&range);
#else
    return ((int(*)(uint32_t,uint32_t,uint32_t))(uintptr_t)0x4FC003FCU)(0x30U,address,bytes);
#endif
}
static unsigned encrypted(uint32_t word){unsigned n=(word>>18U)&7U;return (n^(n>>1U)^(n>>2U))&1U;}
static maia_psram_result_t cache_finish(const maia_psram_snapshot_t *,maia_psram_cache_report_t *);
maia_psram_result_t maia_esp32p4_psram_cache_boot(maia_psram_cache_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    r->stage=r->size=r->line=r->ways=0;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(cache_state!=FREE)return cache_state==OWNED?MAIA_PSRAM_BUSY:MAIA_PSRAM_FAILED;
    maia_psram_snapshot_t s;maia_psram_result_t result=maia_esp32p4_psram_snapshot(&s);
    if(result!=MAIA_PSRAM_OK)return result;
    if(!psram_axi_quiet()||s.before[0]||s.before[16]||encrypted(psram_read(WINDOW_CRYPT)))return MAIA_PSRAM_BUSY;
    cache_state=FAULTED;r->stage=1;
    for(unsigned i=0;i<3U;++i)if(cache_call(i,0))return MAIA_PSRAM_IO;
    return cache_finish(&s,r);
}
/* XIP boot: the bootloader already enabled L1/L2 caches that are executing this
 * image, so ROM cache re-initialization is forbidden. Query/verify only. */
maia_psram_result_t maia_esp32p4_psram_cache_adopt(maia_psram_cache_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    r->stage=r->size=r->line=r->ways=0;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(cache_state!=FREE)return cache_state==OWNED?MAIA_PSRAM_BUSY:MAIA_PSRAM_FAILED;
    maia_psram_snapshot_t s;maia_psram_result_t result=maia_esp32p4_psram_snapshot(&s);
    if(result!=MAIA_PSRAM_OK)return result;
    if(!psram_axi_quiet()||s.before[0]||s.before[16]||encrypted(psram_read(WINDOW_CRYPT)))return MAIA_PSRAM_BUSY;
    cache_state=FAULTED;r->stage=1;
    return cache_finish(&s,r);
}
static maia_psram_result_t cache_finish(const maia_psram_snapshot_t *sp,maia_psram_cache_report_t *r){
    maia_psram_result_t result;
    cache_mode m={0,0,0,5};if(cache_call(3,&m))return MAIA_PSRAM_IO;
    r->size=m.size;r->line=m.line;r->ways=m.ways;r->stage=2;
    if((m.size!=131072U&&m.size!=262144U&&m.size!=524288U)||(m.line!=64U&&m.line!=128U)||(m.ways!=4U&&m.ways!=8U))return MAIA_PSRAM_VERIFY;
    maia_psram_snapshot_t after;result=maia_esp32p4_psram_snapshot(&after);if(result!=MAIA_PSRAM_OK)return result;
    for(unsigned i=0;i<9U;++i)if(sp->roots[i]!=after.roots[i])return MAIA_PSRAM_CHANGED;
    for(unsigned i=0;i<27U;++i)if(sp->before[i]!=after.before[i])return MAIA_PSRAM_CHANGED;
    if(!psram_axi_quiet())return MAIA_PSRAM_CHANGED;
    r->stage=3;cache_state=OWNED;return MAIA_PSRAM_OK;
}
static int guards(void){
    for(unsigned i=0;i<9U;++i)if(psram_read(psram_roots[i])!=reference.roots[i])return 0;
    if(psram_read(WINDOW_CRYPT)!=crypt||psram_read(WINDOW_ECC)!=ecc)return 0;
    if(psram_read(PSRAM_MSPI0+0x140U)!=pms_base||psram_read(PSRAM_MSPI0+0x150U)!=pms_size)return 0;
    for(unsigned i=0;i<4U;++i)if(psram_read(PSRAM_MSPI0+0x130U+i*4U)!=pms[i])return 0;
    for(unsigned i=0;i<WINDOW_REGS;++i)if(psram_read(window_plan[i].address)!=current[i])return 0;
    for(unsigned i=0;i<27U;++i){
        uint32_t expected=reference.before[i];uintptr_t a=psram_address(i);
        for(unsigned j=0;j<WINDOW_REGS;++j)if(a==window_plan[j].address)expected=current[j];
        uint32_t actual=psram_read(a);
        if(i==0U){if(actual&~255U)return 0;}else if(actual!=expected)return 0;
    }
    return 1;
}
static int reg_write(unsigned i,uint32_t value){
    if(!guards())return 0;
    psram_write(window_plan[i].address,value);
    /* Roots before controller readback; no access through a newly lost gate. */
    for(unsigned j=0;j<3U;++j)if(psram_read(psram_roots[j])!=reference.roots[j])return 0;
    if(psram_read(window_plan[i].address)!=value)return 0;
    current[i]=value;return guards();
}
static int select_entry(unsigned i){
    if(!guards())return 0;
    psram_write(WINDOW_MMU_INDEX,896U+i);
    return guards()&&psram_read(WINDOW_MMU_INDEX)==896U+i;
}
static int table_matches(unsigned mapped){
    for(unsigned i=0;i<WINDOW_MMU_PAGES;++i){if(!select_entry(i)||psram_read(WINDOW_MMU_CONTENT)!=(mapped?0xC00U+i:table[i]))return 0;}
    return 1;
}
static void report(maia_psram_window_report_t *r,unsigned mapped){
    r->vaddr=MAIA_PSRAM_WINDOW_VADDR;r->bytes=MAIA_PSRAM_WINDOW_BYTES;r->physical=0;
    for(unsigned i=0;i<WINDOW_MMU_PAGES;++i){r->mmu_before[i]=table[i];r->mmu_after[i]=mapped?0xC00U+i:table[i];}
}
static maia_psram_result_t init_report(uint32_t budget,maia_psram_window_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    return window_state==FAULTED?MAIA_PSRAM_FAILED:MAIA_PSRAM_OK;
}
static int drained(uint32_t budget,maia_psram_window_report_t *r){
    for(unsigned i=0;i<budget;++i){
        if(!guards())return 0;
        ++r->polls;
        if((psram_read(WINDOW_FIFO)&0xFC000000U)==0xFC000000U&&!psram_read(PSRAM_MSPI0)&&!psram_read(PSRAM_MSPI1))return 1;
    }
    return 0;
}
maia_psram_result_t maia_esp32p4_psram_window_start(uint32_t budget,maia_psram_window_report_t *r){
    maia_psram_result_t result=init_report(budget,r);if(result!=MAIA_PSRAM_OK)return result;
    if(window_state!=FREE)return MAIA_PSRAM_BUSY;
    if(cache_state==FAULTED)return MAIA_PSRAM_FAILED;
    if(cache_state!=OWNED||!maia_psram_x8_claimed()||!maia_psram_clock_claimed())return MAIA_PSRAM_BUSY;
    /* Genuine X8 claim already checked the device profile at safe XTAL20. Fast
     * training may deliberately test a failing DQS phase; do not issue a mode
     * command there and poison the command engine before read-only qualification.
     * Exclusive owner must not modify device mode outside the native API. */
    if(maia_psram_clock_mhz()==20U){
        maia_psram_mode_report_t mode;result=maia_esp32p4_psram_read_mode(8,budget,&mode);
        if(result!=MAIA_PSRAM_OK)return result;
        if(mode.value!=0x0BU)return MAIA_PSRAM_BUSY;
    }
    result=maia_esp32p4_psram_snapshot(&reference);if(result!=MAIA_PSRAM_OK)return result;
    unsigned mhz=maia_psram_clock_mhz();
    uint32_t bus=mhz==100U?0x30103U:0x10001U,root=mhz==20U?0x8000U:0x9000U;
    if((mhz!=20U&&mhz!=100U&&mhz!=200U)||!psram_axi_quiet()||reference.before[0]||reference.before[16]||reference.before[7]!=bus||
       reference.before[20]!=bus||(reference.roots[1]&0xFFB000U)!=root||(reference.before[3]&6U)||
       (reference.before[4]&0x3DC00U))return MAIA_PSRAM_BUSY;
    crypt=psram_read(WINDOW_CRYPT);ecc=psram_read(WINDOW_ECC);
    if(encrypted(crypt)||(ecc&0x100000U))return MAIA_PSRAM_BUSY;
    pms_base=psram_read(PSRAM_MSPI0+0x140U);pms_size=psram_read(PSRAM_MSPI0+0x150U);
    if(pms_base||pms_size!=4096U||(reference.before[8]&0x6020001CU))return MAIA_PSRAM_BUSY;
    for(unsigned i=0;i<4U;++i){pms[i]=psram_read(PSRAM_MSPI0+0x130U+i*4U);if((pms[i]&7U)!=3U)return MAIA_PSRAM_BUSY;}
    saved_index=psram_read(WINDOW_MMU_INDEX);if(saved_index>=1024U)return MAIA_PSRAM_BUSY;
    for(unsigned i=0;i<WINDOW_REGS;++i)saved[i]=current[i]=psram_read(window_plan[i].address);
    if(saved[26]&0x10000U)return MAIA_PSRAM_BUSY;
    if(!guards())return MAIA_PSRAM_CHANGED;
    window_state=FAULTED;r->stage=1;
    if(!drained(budget,r))return MAIA_PSRAM_TIMEOUT;
    for(unsigned i=0;i<WINDOW_MMU_PAGES;++i){if(!select_entry(i))return MAIA_PSRAM_VERIFY;table[i]=psram_read(WINDOW_MMU_CONTENT);}
    for(unsigned i=0;i<WINDOW_REGS-1U;++i){
        uint32_t v=(saved[i]&~window_plan[i].mask)|window_plan[i].value;
        if(!reg_write(i,v))return MAIA_PSRAM_VERIFY;
    }
    r->stage=2;
    for(unsigned i=0;i<WINDOW_MMU_PAGES;++i){
        if(!select_entry(i))return MAIA_PSRAM_VERIFY;
        psram_write(WINDOW_MMU_CONTENT,0xC00U+i);
        if(!guards()||psram_read(WINDOW_MMU_CONTENT)!=0xC00U+i)return MAIA_PSRAM_VERIFY;
    }
    if(!table_matches(1)||cache_call(5,0))return MAIA_PSRAM_VERIFY;
    r->stage=3;
    if(!reg_write(27,(saved[27]&~0x80000001U)|1U))return MAIA_PSRAM_VERIFY;
    if(!drained(budget,r))return MAIA_PSRAM_TIMEOUT;
    report(r,1);r->stage=4;window_state=OWNED;return MAIA_PSRAM_OK;
}
maia_psram_result_t maia_esp32p4_psram_window_sync(uint32_t budget,maia_psram_window_report_t *r){
    maia_psram_result_t result=init_report(budget,r);if(result!=MAIA_PSRAM_OK)return result;
    if(window_state!=OWNED)return MAIA_PSRAM_BUSY;
    window_state=FAULTED;
    if(!guards()||!table_matches(1)||cache_call(4,0))return MAIA_PSRAM_VERIFY;
    if(!drained(budget,r))return MAIA_PSRAM_TIMEOUT;
    report(r,1);r->stage=5;window_state=OWNED;return MAIA_PSRAM_OK;
}
maia_psram_result_t maia_esp32p4_psram_window_stop(uint32_t budget,maia_psram_window_report_t *r){
    maia_psram_result_t result=maia_esp32p4_psram_window_sync(budget,r);if(result!=MAIA_PSRAM_OK)return result;
    window_state=FAULTED;
    if(!reg_write(27,saved[27]))return MAIA_PSRAM_VERIFY;
    for(unsigned i=0;i<WINDOW_MMU_PAGES;++i){
        if(!select_entry(i))return MAIA_PSRAM_VERIFY;
        psram_write(WINDOW_MMU_CONTENT,table[i]);
        if(!guards()||psram_read(WINDOW_MMU_CONTENT)!=table[i])return MAIA_PSRAM_VERIFY;
    }
    if(!table_matches(0))return MAIA_PSRAM_VERIFY;
    for(unsigned n=27U;n>0U;--n)if(!reg_write(n-1U,saved[n-1U]))return MAIA_PSRAM_VERIFY;
    if(!guards())return MAIA_PSRAM_VERIFY;
    psram_write(WINDOW_MMU_INDEX,saved_index);
    if(!guards()||psram_read(WINDOW_MMU_INDEX)!=saved_index)return MAIA_PSRAM_VERIFY;
    report(r,0);r->stage=6;r->restored=1;window_state=FREE;return MAIA_PSRAM_OK;
}

static int dma_idle(void){
    if(!(psram_read(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)&0x2000U)||
       !(psram_read(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG)&0x20U))return 0;
    uintptr_t base=MAIA_ESP32P4_GDMA_BASE;
    if(psram_read(base+0x10U)||psram_read(base+0x18U)||psram_read(base+0x1CU)||(psram_read(base+0x58U)&1U))return 0;
    for(unsigned i=0;i<4U;++i)if(psram_read(base+0x134U+i*0x100U)&0x7FFFU)return 0;
    return 1;
}
uint32_t maia_psram_window_dma_acquire(void){
    if(!psram_context()||window_state!=OWNED||maia_psram_clock_mhz()!=200U)return 0;
    if(!dma_idle())return 0;
    maia_psram_window_report_t r;
    if(maia_esp32p4_psram_window_sync(10000,&r)!=MAIA_PSRAM_OK)return 0;
    if(!dma_idle())return 0;
    window_state=DMA_OWNED;return MAIA_PSRAM_WINDOW_VADDR+128U;
}
int maia_psram_window_dma_live(void){return psram_context()&&(window_state==DMA_OWNED||window_state==DMA_EDIT_OWNED)&&guards();}
uint32_t maia_psram_window_edit_begin(uint32_t source,uint32_t bytes){
    uint32_t first=MAIA_PSRAM_WINDOW_VADDR+128U;
    if(!psram_context()||window_state!=DMA_OWNED||!bytes||(source&127U)||(bytes&127U)||
       source<first||source>MAIA_PSRAM_WINDOW_VADDR+MAIA_PSRAM_WINDOW_BYTES||
       bytes>MAIA_PSRAM_WINDOW_VADDR+MAIA_PSRAM_WINDOW_BYTES-source)return 0;
    if(!guards()||!table_matches(1))return 0;
    window_state=DMA_EDIT_OWNED;return source;
}
int maia_psram_window_edit_publish(uint32_t source,uint32_t bytes){
    if(!psram_core0_context()||window_state!=DMA_EDIT_OWNED||!bytes||(source&127U)||(bytes&127U)||
       source<MAIA_PSRAM_WINDOW_VADDR+128U||source>MAIA_PSRAM_WINDOW_VADDR+MAIA_PSRAM_WINDOW_BYTES||
       bytes>MAIA_PSRAM_WINDOW_VADDR+MAIA_PSRAM_WINDOW_BYTES-source)return 0;
    if(!guards()||!table_matches(1)||cache_range(source,bytes)||!guards()||!table_matches(1)){
        window_state=FAULTED;return 0;
    }
#ifdef MAIA_PSRAM_TEST
    __asm__ volatile("" ::: "memory");
#else
    __asm__ volatile("fence iorw, iorw" ::: "memory");
#endif
    window_state=DMA_OWNED;return 1;
}
int maia_psram_window_direct_flush(uint32_t address,uint32_t bytes){
    uint32_t first=MAIA_PSRAM_WINDOW_VADDR+128U,end=MAIA_PSRAM_WINDOW_VADDR+MAIA_PSRAM_WINDOW_BYTES;
    if(window_state!=DMA_OWNED||!bytes||address<first||address>=end||bytes>end-address)return 0;
    uint32_t start=address&~63U,stop=(address+bytes+63U)&~63U;
    if(stop>end)stop=end;
    if(cache_range(start,stop-start))return 0;
#ifdef MAIA_PSRAM_TEST
    __asm__ volatile("" ::: "memory");
#else
    __asm__ volatile("fence iorw, iorw" ::: "memory");
#endif
    return 1;
}
int maia_psram_window_dma_release(void){
    if(!psram_context()||window_state!=DMA_OWNED)return 0;
    window_state=FAULTED;
    if(!guards()||!dma_idle()||!table_matches(1))return 0;
    window_state=OWNED;return 1;
}
