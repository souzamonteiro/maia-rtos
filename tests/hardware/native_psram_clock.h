/* Original MIT. Warm native clock qualification, no PSRAM array/mode writes. */
static void clock_report(unsigned phase,maia_psram_result_t result,const maia_psram_clock_report_t *r){
    field("PSRAM_CLOCK phase=",phase);field(" result=",(uint32_t)result);field(" stage=",r->stage);
    field(" attempts=",r->attempts);field(" restored=",r->restored);maia_console_puts("\n");
    for(unsigned i=0;i<3U;++i){field("PSRAM_CLOCK_WORD phase=",phase);field(" index=",i);field(" before=",r->before[i]);field(" after=",r->after[i]);maia_console_puts("\n");}
}
static void psram_clock(const maia_psram_snapshot_t *before){
    field("PSRAM_AXI control=",psram_read(PSRAM_MSPI0+0x3CU));maia_console_puts("\n");
    maia_psram_clock_report_t r;maia_psram_result_t result=maia_esp32p4_psram_xtal20_start(&r);
    clock_report(0,result,&r);
    if(result!=MAIA_PSRAM_OK){maia_console_puts("MAIA_NATIVE_PSRAM_CLOCK_FAIL start\n");return;}
    maia_psram_snapshot_t active;
    if(maia_esp32p4_psram_snapshot(&active)!=MAIA_PSRAM_OK){maia_console_puts("MAIA_NATIVE_PSRAM_CLOCK_FAIL snapshot\n");return;}
    psram_identity(&active);
#ifdef MAIA_NATIVE_PROBE_PSRAM_ARRAY
    if(!psram_array()){maia_console_puts("MAIA_NATIVE_PSRAM_ARRAY_FAIL transfer-or-data\n");return;}
    maia_console_puts("MAIA_NATIVE_PSRAM_ARRAY_PASS\n");
#endif
    result=maia_esp32p4_psram_xtal20_stop(&r);clock_report(1,result,&r);
    if(result!=MAIA_PSRAM_OK){maia_console_puts("MAIA_NATIVE_PSRAM_CLOCK_FAIL stop\n");return;}
    maia_psram_snapshot_t after;result=maia_esp32p4_psram_snapshot(&after);unsigned roots=1,words=1;
    for(unsigned i=0;i<9U;++i)if(before->roots[i]!=after.roots[i])roots=0;
    for(unsigned i=0;i<27U;++i)if(before->before[i]!=after.before[i])words=0;
    field("PSRAM_CLOCK_FINAL result=",(uint32_t)result);field(" roots=",roots);field(" words=",words);maia_console_puts("\n");
    if(result==MAIA_PSRAM_OK){field("PSRAM_AXI_FINAL control=",psram_read(PSRAM_MSPI0+0x3CU));maia_console_puts("\n");}
    maia_console_puts(result==MAIA_PSRAM_OK&&roots&&words?"MAIA_NATIVE_PSRAM_CLOCK_PASS\n":"MAIA_NATIVE_PSRAM_CLOCK_FAIL restore\n");
}
