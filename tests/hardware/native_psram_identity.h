/* Original MIT. Read-only device mode registers through temporary command-port
 * configuration; no PSRAM mode/array/clock/power/MMU write. Warm identity only. */
static void psram_identity(const maia_psram_snapshot_t *before){
    static const unsigned regs[4]={0,2,4,8};uint32_t values[4]={0,0,0,0};
    for(unsigned n=0;n<8U;++n){
        maia_psram_mode_report_t r;maia_psram_result_t result=maia_esp32p4_psram_read_mode(regs[n%4U],10000,&r);
        field("PSRAM_ID index=",n);field(" reg=",r.reg);field(" result=",(uint32_t)result);field(" stage=",r.stage);field(" bits=",r.bits);
        field(" setup=",r.setup_attempts);field(" restore=",r.restore_attempts);field(" triggered=",r.triggered);field(" polls=",r.polls);field(" busy=",r.saw_busy);
        field(" command=",r.command);field(" value=",r.value);field(" restored=",r.restored);maia_console_puts("\n");
        for(unsigned i=0;i<10U;++i){field("PSRAM_ID_WORD read=",n);field(" index=",i);field(" before=",r.before[i]);field(" configured=",r.after[i]);maia_console_puts("\n");}
        if(result!=MAIA_PSRAM_OK||r.stage!=5U||!r.restored){maia_console_puts("MAIA_NATIVE_PSRAM_ID_FAIL transfer\n");return;}
        if(n<4U)values[n]=r.value;
        else if(values[n%4U]!=r.value){maia_console_puts("MAIA_NATIVE_PSRAM_ID_FAIL unstable\n");return;}
    }
    unsigned vendor=(values[0]>>8U)&31U,density=values[1]&7U,device=(values[1]>>3U)&3U,kgd=(values[1]>>5U)&7U,x16=(values[3]>>6U)&1U;
    maia_psram_snapshot_t after;maia_psram_result_t result=maia_esp32p4_psram_snapshot(&after);unsigned roots=1,words=1;
    for(unsigned i=0;i<9U;++i)if(before->roots[i]!=after.roots[i])roots=0;
    for(unsigned i=0;i<27U;++i)if(before->before[i]!=after.before[i])words=0;
    field("PSRAM_ID_RESTORE result=",(uint32_t)result);field(" roots=",roots);field(" words=",words);field(" vendor=",vendor);
    field(" density=",density);field(" device=",device);field(" kgd=",kgd);field(" x16=",x16);maia_console_puts("\n");
    maia_console_puts(result==MAIA_PSRAM_OK&&roots&&words&&vendor==13U&&density==7U&&device==3U&&kgd==6U&&x16?"MAIA_NATIVE_PSRAM_ID_PASS\n":"MAIA_NATIVE_PSRAM_ID_FAIL identity-or-restore\n");
}
