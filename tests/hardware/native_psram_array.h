/* Native boot-exclusive reservation physical[0,4096), all buffers internal SRAM.
 * Save/restore only after successful operations; uncertain writes require reset. */
static struct {uint32_t first;uint8_t original[4096],pattern[4096],readback[4096];uint32_t last;} array_memory;
static uint32_t array_hash(const uint8_t *p){uint32_t h=2166136261U;for(unsigned i=0;i<4096;++i)h=(h^p[i])*16777619U;return h;}
static int array_transfer(unsigned index,void *data,unsigned write){
    maia_psram_transfer_report_t r;maia_psram_result_t result=maia_esp32p4_psram_boot_transfer(0,data,4096,write,10000,&r);
    field("PSRAM_ARRAY_TRANSFER index=",index);field(" write=",write);field(" result=",(uint32_t)result);
    field(" completed=",r.completed);field(" chunks=",r.chunks);field(" polls=",r.polls);field(" command=",r.command);
    field(" restored=",r.restored);field(" attempted=",r.write_attempted);maia_console_puts("\n");
    return result==MAIA_PSRAM_OK&&r.completed==4096U&&r.chunks==64U&&r.restored&&!r.command;
}
static int array_mode(unsigned restore){
    maia_psram_mode_switch_report_t r;maia_psram_result_t result=maia_esp32p4_psram_x8_mode(restore,10000,&r);
    field("PSRAM_ARRAY_MODE phase=",restore);field(" result=",(uint32_t)result);field(" stage=",r.stage);field(" before=",r.before);field(" after=",r.after);maia_console_puts("\n");
    return result==MAIA_PSRAM_OK&&r.stage==3U;
}
static int psram_array(void){
    if(!array_mode(0))return 0;
    array_memory.first=0x12345678U;array_memory.last=0x89ABCDEFU;
    if(!array_transfer(0,array_memory.original,0))return 0;
    uint32_t original=array_hash(array_memory.original);
    for(unsigned pass=0;pass<4U;++pass){
        uint32_t state=0x783A1B25U;
        for(unsigned i=0;i<4096U;++i){state=state*1664525U+1013904223U;array_memory.pattern[i]=pass==0U?0U:pass==1U?255U:pass==2U?(i&1U?0x55U:0xAAU):(uint8_t)(state>>24U);}
        if(!array_transfer(1U+pass*2U,array_memory.pattern,1)||!array_transfer(2U+pass*2U,array_memory.readback,0))return 0;
        unsigned matched=0;for(unsigned i=0;i<4096U;++i)if(array_memory.pattern[i]==array_memory.readback[i])++matched;
        field("PSRAM_ARRAY_PATTERN index=",pass);field(" matched=",matched);field(" expected=",array_hash(array_memory.pattern));field(" actual=",array_hash(array_memory.readback));maia_console_puts("\n");
        if(matched!=4096U){
            for(unsigned i=0;i<128U;++i){field("PSRAM_ARRAY_SAMPLE index=",i);field(" expected=",array_memory.pattern[i]);field(" actual=",array_memory.readback[i]);maia_console_puts("\n");}
            return 0;
        }
    }
    if(!array_transfer(9,array_memory.original,1)||!array_transfer(10,array_memory.readback,0))return 0;
    unsigned matched=0;for(unsigned i=0;i<4096U;++i)if(array_memory.original[i]==array_memory.readback[i])++matched;
    unsigned guards=array_memory.first==0x12345678U&&array_memory.last==0x89ABCDEFU;
    field("PSRAM_ARRAY_RESTORE matched=",matched);field(" before=",original);field(" after=",array_hash(array_memory.readback));field(" guards=",guards);maia_console_puts("\n");
    if(matched!=4096U||!guards||original!=array_hash(array_memory.readback))return 0;
    return array_mode(1);
}
