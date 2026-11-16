/* Native fixed2MiB CPU window; buffers/code in internal SRAM. */
static int window_cache_boot(void){
    extern void maia_soc_trap_init(void);maia_soc_trap_init();
    maia_console_puts("PSRAM_CACHE_BOOT_START\n");
    maia_psram_cache_report_t r;maia_psram_result_t result=maia_esp32p4_psram_cache_boot(&r);
    field("PSRAM_CACHE_BOOT result=",(uint32_t)result);field(" stage=",r.stage);field(" size=",r.size);field(" line=",r.line);field(" ways=",r.ways);maia_console_puts("\n");
    return result==MAIA_PSRAM_OK&&r.stage==3U;
}
static int window_mode(unsigned restore){
    maia_psram_mode_switch_report_t r;maia_psram_result_t result=maia_esp32p4_psram_x8_mode(restore,10000,&r);
    field("PSRAM_WINDOW_MODE phase=",restore);field(" result=",(uint32_t)result);field(" stage=",r.stage);field(" before=",r.before);field(" after=",r.after);maia_console_puts("\n");
    return result==MAIA_PSRAM_OK&&r.stage==3U;
}
static int window_raw(unsigned phase,void *data,unsigned write){
    maia_psram_transfer_report_t r;maia_psram_result_t result=maia_esp32p4_psram_boot_transfer(0,data,4096,write,10000,&r);
    field("PSRAM_WINDOW_RAW phase=",phase);field(" result=",(uint32_t)result);field(" write=",write);field(" completed=",r.completed);field(" chunks=",r.chunks);field(" restored=",r.restored);maia_console_puts("\n");
    return result==MAIA_PSRAM_OK&&r.completed==4096U&&r.restored;
}
static uint32_t window_value(uint32_t index){return (0x9E3779B9U*(index+1U))^0xA55A1234U;}
static void window_report(unsigned phase,maia_psram_result_t result,const maia_psram_window_report_t *r){
    field("PSRAM_WINDOW phase=",phase);field(" result=",(uint32_t)result);field(" stage=",r->stage);field(" vaddr=",r->vaddr);field(" bytes=",r->bytes);field(" physical=",r->physical);field(" restored=",r->restored);field(" polls=",r->polls);maia_console_puts("\n");
    if(result==MAIA_PSRAM_OK)for(unsigned i=0;i<32U;++i){field("PSRAM_WINDOW_MMU phase=",phase);field(" index=",i);field(" before=",r->mmu_before[i]);field(" after=",r->mmu_after[i]);maia_console_puts("\n");}
}
#ifdef MAIA_NATIVE_PROBE_PSRAM_DMA
#include "native_psram_dma.h"
#endif
static int psram_window(void){
    if(!window_mode(0)||!window_raw(0,array_memory.original,0))return 0;
    maia_psram_window_report_t r;maia_psram_result_t result=maia_esp32p4_psram_window_start(10000,&r);window_report(0,result,&r);
    if(result!=MAIA_PSRAM_OK)return 0;
    volatile uint32_t *memory=(volatile uint32_t *)(uintptr_t)r.vaddr;
    for(uint32_t i=0;i<MAIA_PSRAM_WINDOW_BYTES/4U;++i)memory[i]=window_value(i);
    __asm__ volatile("fence iorw,iorw" ::: "memory");
    result=maia_esp32p4_psram_window_sync(10000,&r);window_report(1,result,&r);if(result!=MAIA_PSRAM_OK)return 0;
    uint32_t matched=0,expected=2166136261U,actual=2166136261U;
    for(uint32_t i=0;i<MAIA_PSRAM_WINDOW_BYTES/4U;++i){uint32_t value=memory[i],want=window_value(i);if(value==want)++matched;
        for(unsigned j=0;j<4U;++j){expected=(expected^((want>>(j*8U))&255U))*16777619U;actual=(actual^((value>>(j*8U))&255U))*16777619U;}}
    field("PSRAM_WINDOW_DATA words=",MAIA_PSRAM_WINDOW_BYTES/4U);field(" matched=",matched);field(" expected=",expected);field(" actual=",actual);maia_console_puts("\n");
#ifdef MAIA_NATIVE_PROBE_PSRAM_DMA
    if(!psram_dma())return 0;
#endif
    result=maia_esp32p4_psram_window_stop(10000,&r);window_report(2,result,&r);if(result!=MAIA_PSRAM_OK)return 0;
    if(!window_raw(1,array_memory.readback,0))return 0;
    unsigned direct=0;for(unsigned i=0;i<4096U;++i)if(array_memory.readback[i]==(uint8_t)(window_value(i/4U)>>((i%4U)*8U)))++direct;
    field("PSRAM_WINDOW_DIRECT matched=",direct);maia_console_puts("\n");
    if(!window_raw(2,array_memory.original,1)||!window_raw(3,array_memory.readback,0))return 0;
    unsigned restored=0;for(unsigned i=0;i<4096U;++i)if(array_memory.original[i]==array_memory.readback[i])++restored;
    field("PSRAM_WINDOW_RESTORE matched=",restored);maia_console_puts("\n");
    if(!window_mode(1))return 0;
    return matched==MAIA_PSRAM_WINDOW_BYTES/4U&&expected==actual&&direct==4096U&&restored==4096U;
}
