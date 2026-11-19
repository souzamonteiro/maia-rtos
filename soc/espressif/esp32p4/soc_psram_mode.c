/* Original MaiaRTOS MIT, boot-exclusive bounded HEX mode-register read.
 * Inherited powered/timed device only; not native cold init/PSRAM mapping. */
#include "psram_mode_internal.h"
static unsigned mode_faulted;
#ifdef MAIA_PSRAM_TEST
void maia_psram_mode_test_reset(void);
void maia_psram_mode_test_reset(void){mode_faulted=0;}
#endif
static int mode_roots(const maia_psram_snapshot_t *s){
    for(unsigned i=0;i<MAIA_PSRAM_ROOT_WORDS;++i)if(psram_read(psram_roots[i])!=s->roots[i])return 0;
    return 1;
}
static int mode_bus(const maia_psram_snapshot_t *s){
    if(!mode_roots(s)||!psram_axi_quiet())return 0;
    /* Cache/AXI side must remain idle and untouched throughout command traffic. */
    for(unsigned i=0;i<16U;++i){
        uint32_t word=psram_read(psram_address(i));
        if(i==0U){if(word&~0xFFU)return 0;} /* shared arbitration status may move */
        else if(word!=s->before[i])return 0;
    }
    static const unsigned fixed[]={18,19,20,25,26};
    for(unsigned i=0;i<5U;++i)if(psram_read(psram_address(fixed[i]))!=s->before[fixed[i]])return 0;
    return 1;
}
static int mode_idle(void){return !psram_read(PSRAM_MSPI0)&&!psram_read(PSRAM_MSPI1);}
static int mode_owned(const maia_psram_snapshot_t *s,const uint32_t current[PSRAM_MODE_WORDS],unsigned ignore_fifo){
    if(!mode_bus(s))return 0;
    for(unsigned i=0;i<PSRAM_MODE_WORDS-(ignore_fifo?1U:0U);++i)if(psram_read(psram_mode_address(i))!=current[i])return 0;
    return 1;
}
maia_psram_result_t maia_esp32p4_psram_read_mode(uint32_t reg,uint32_t budget,maia_psram_mode_report_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if((reg!=0U&&reg!=2U&&reg!=4U&&reg!=8U)||!budget||budget>100000U)return MAIA_PSRAM_INVALID;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    if(mode_faulted)return MAIA_PSRAM_FAILED;
    maia_psram_snapshot_t s;maia_psram_result_t result=maia_esp32p4_psram_snapshot(&s);
    if(result!=MAIA_PSRAM_OK)return result;
    if(s.before[0]||s.before[16]||!psram_axi_quiet()||(s.before[2]&1U)||!(s.roots[4]&0x100U)||
       (s.before[17]&0x01904100U)||(s.before[24]&0x0020000EU))return MAIA_PSRAM_BUSY;
    uint32_t current[PSRAM_MODE_WORDS];
    for(unsigned i=0;i<PSRAM_MODE_WORDS;++i)r->before[i]=r->after[i]=current[i]=psram_read(psram_mode_address(i));
    if(!mode_owned(&s,current,0)||!mode_idle())return MAIA_PSRAM_CHANGED;
    r->reg=reg;r->bits=reg<4U?16U:8U;r->stage=1;mode_faulted=1;
    for(unsigned i=0;i<PSRAM_MODE_WORDS;++i){
        if(!mode_owned(&s,current,0)||!mode_idle())return MAIA_PSRAM_VERIFY;
        uint32_t desired=psram_mode_value(i,current[i],reg);r->setup_attempts|=1U<<i;
        psram_write(psram_mode_address(i),desired);
        if(!mode_roots(&s))return MAIA_PSRAM_VERIFY;
        r->after[i]=psram_read(psram_mode_address(i));if(r->after[i]!=desired)return MAIA_PSRAM_VERIFY;
        current[i]=desired;
    }
    r->stage=2;
    if(!mode_owned(&s,current,0)||!mode_idle())return MAIA_PSRAM_VERIFY;
    /* Literal single read command; never assert any erase/program trigger. */
    r->triggered=1;psram_write(PSRAM_MSPI1,0x40000U);r->stage=3;
    unsigned idle=0;
    for(unsigned i=0;i<budget;++i){
        ++r->polls;if(!mode_owned(&s,current,1))return MAIA_PSRAM_VERIFY;
        uint32_t command=psram_read(PSRAM_MSPI1);r->command=command;
        if(command&~0x400FFU)return MAIA_PSRAM_VERIFY;
        if(command&0x40000U)r->saw_busy=1;
        if(!command&&!psram_read(PSRAM_MSPI0)){idle=1;break;}
    }
    if(!idle)return MAIA_PSRAM_TIMEOUT; /* Never restore while command remains active. */
    r->value=psram_read(PSRAM_MSPI1+0x58U)&(reg<4U?0xFFFFU:0xFFU);r->stage=4;
    result=MAIA_PSRAM_OK;
    if(r->value==(PSRAM_MODE_POISON&(reg<4U?0xFFFFU:0xFFU))||(reg==0U&&((r->value>>8U)&31U)!=13U))result=MAIA_PSRAM_IO;
    /* Restore exact saved command-port values only with roots/bus/ownership intact.
     * FIFO is engine-produced after transfer; verify all other configuration. */
    if(!mode_owned(&s,current,1)||!mode_idle())return MAIA_PSRAM_VERIFY;
    current[9]=psram_read(PSRAM_MSPI1+0x58U);
    for(unsigned n=PSRAM_MODE_WORDS;n>0U;--n){
        unsigned i=n-1U;
        if(!mode_owned(&s,current,0)||!mode_idle())return MAIA_PSRAM_VERIFY;
        r->restore_attempts|=1U<<i;psram_write(psram_mode_address(i),r->before[i]);
        if(!mode_roots(&s)||psram_read(psram_mode_address(i))!=r->before[i])return MAIA_PSRAM_VERIFY;
        current[i]=r->before[i];
    }
    if(!mode_owned(&s,current,0)||!mode_idle())return MAIA_PSRAM_VERIFY;
    r->restored=1;r->stage=5;
    if(result==MAIA_PSRAM_OK)mode_faulted=0;
    return result;
}
