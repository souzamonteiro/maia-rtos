/* Original MIT. Read-only inherited PSRAM controller state; no PSRAM array access. */
#include <maia/console.h>
#include "../../soc/espressif/esp32p4/psram_internal.h"
#ifdef MAIA_NATIVE_PROBE_PSRAM_CLOCK
#include <maia/drivers/serial_rx.h>
#endif
void maia_display_putc(char c){(void)c;}
void maia_display_refresh_dma_if_needed(void){}
static void field(const char *k,uint32_t n){char b[9];const char *d="0123456789abcdef";for(unsigned i=0;i<8;++i)b[i]=d[(n>>(28U-i*4U))&15U];b[8]=0;maia_console_puts(k);maia_console_puts(b);}
#ifdef MAIA_NATIVE_PROBE_PSRAM_ID
#include "native_psram_identity.h"
#endif
#ifdef MAIA_NATIVE_PROBE_PSRAM_CLOCK
#ifdef MAIA_NATIVE_PROBE_PSRAM_ARRAY
#include "native_psram_array.h"
#ifdef MAIA_NATIVE_PROBE_PSRAM_WINDOW
#include "native_psram_window.h"
#endif
#endif
#include "native_psram_clock.h"
#endif
void maia_boot_main(void){
#ifdef MAIA_NATIVE_PROBE_PSRAM_CLOCK
    unsigned ready=0;
    for(unsigned i=0;i<10000000U;++i)if(maia_serial_rx_read(NULL)=='C'){ready=1;break;}
    if(!ready){maia_console_puts("MAIA_NATIVE_PSRAM_CLOCK_FAIL host-timeout\n");for(;;)__asm__ volatile("wfi");}
    maia_console_puts("MAIA_NATIVE_PSRAM_CLOCK_HOST_READY\n");
#ifdef MAIA_NATIVE_PROBE_PSRAM_WINDOW
    if(!window_cache_boot()){maia_console_puts("MAIA_NATIVE_PSRAM_WINDOW_FAIL cache-boot\n");for(;;)__asm__ volatile("wfi");}
#endif
#endif
    maia_psram_snapshot_t r;maia_psram_result_t result=maia_esp32p4_psram_snapshot(&r);
    field("PSRAM_STATE result=",(uint32_t)result);field(" roots_valid=",r.roots_valid);field(" samples=",r.controller_samples);maia_console_puts("\n");
    for(unsigned i=0;i<9U;++i){field("PSRAM_ROOT index=",i);field(" address=",(uint32_t)psram_roots[i]);field(" before=",r.roots[i]);field(" after=",r.roots_after[i]);maia_console_puts("\n");}
    if(result==MAIA_PSRAM_OK)for(unsigned i=0;i<27U;++i){field("PSRAM_WORD index=",i);field(" address=",(uint32_t)psram_address(i));field(" before=",r.before[i]);field(" after=",r.after[i]);maia_console_puts("\n");}
    maia_console_puts((result==MAIA_PSRAM_OK||result==MAIA_PSRAM_GATED)?"MAIA_NATIVE_PSRAM_STATE_OBSERVED\n":"MAIA_NATIVE_PSRAM_STATE_FAIL\n");
#ifdef MAIA_NATIVE_PROBE_PSRAM_ID
    if(result==MAIA_PSRAM_OK){
#ifdef MAIA_NATIVE_PROBE_PSRAM_CLOCK
        psram_clock(&r);
#else
        psram_identity(&r);
#endif
    }
    else maia_console_puts("MAIA_NATIVE_PSRAM_ID_FAIL inaccessible\n");
#endif
    for(;;)__asm__ volatile("wfi");
}
