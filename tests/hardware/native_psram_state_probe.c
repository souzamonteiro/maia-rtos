/* Original MIT. Read-only inherited PSRAM controller state; no PSRAM array access. */
#include <maia/console.h>
#include "../../soc/espressif/esp32p4/psram_internal.h"
void maia_display_putc(char c){(void)c;}
void maia_display_refresh_dma_if_needed(void){}
static void field(const char *k,uint32_t n){char b[9];const char *d="0123456789abcdef";for(unsigned i=0;i<8;++i)b[i]=d[(n>>(28U-i*4U))&15U];b[8]=0;maia_console_puts(k);maia_console_puts(b);}
#ifdef MAIA_NATIVE_PROBE_PSRAM_ID
#include "native_psram_identity.h"
#endif
void maia_boot_main(void){
    maia_psram_snapshot_t r;maia_psram_result_t result=maia_esp32p4_psram_snapshot(&r);
    field("PSRAM_STATE result=",(uint32_t)result);field(" roots_valid=",r.roots_valid);field(" samples=",r.controller_samples);maia_console_puts("\n");
    for(unsigned i=0;i<9U;++i){field("PSRAM_ROOT index=",i);field(" address=",(uint32_t)psram_roots[i]);field(" before=",r.roots[i]);field(" after=",r.roots_after[i]);maia_console_puts("\n");}
    if(result==MAIA_PSRAM_OK)for(unsigned i=0;i<27U;++i){field("PSRAM_WORD index=",i);field(" address=",(uint32_t)psram_address(i));field(" before=",r.before[i]);field(" after=",r.after[i]);maia_console_puts("\n");}
    maia_console_puts((result==MAIA_PSRAM_OK||result==MAIA_PSRAM_GATED)?"MAIA_NATIVE_PSRAM_STATE_OBSERVED\n":"MAIA_NATIVE_PSRAM_STATE_FAIL\n");
#ifdef MAIA_NATIVE_PROBE_PSRAM_ID
    if(result==MAIA_PSRAM_OK)psram_identity(&r);
    else maia_console_puts("MAIA_NATIVE_PSRAM_ID_FAIL inaccessible\n");
#endif
    for(;;)__asm__ volatile("wfi");
}
