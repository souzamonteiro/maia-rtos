/* Original MaiaRTOS MIT. Conservative read-only diagnostic, not PSRAM init. */
#include "psram_internal.h"
static void roots_read(uint32_t *out){for(unsigned i=0;i<MAIA_PSRAM_ROOT_WORDS;++i)out[i]=psram_read(psram_roots[i]);}
static int roots_match(const maia_psram_snapshot_t *r){
    for(unsigned i=0;i<MAIA_PSRAM_ROOT_WORDS;++i)if(r->roots[i]!=r->roots_after[i])return 0;
    return 1;
}
maia_psram_result_t maia_esp32p4_psram_snapshot(maia_psram_snapshot_t *r){
    if(!r)return MAIA_PSRAM_INVALID;
    for(unsigned i=0;i<sizeof *r;++i)((volatile unsigned char *)r)[i]=0;
    if(!psram_context())return MAIA_PSRAM_CONTEXT;
    roots_read(r->roots);r->roots_valid=1;
    if(!psram_accessible(r->roots)){
        roots_read(r->roots_after);return roots_match(r)?MAIA_PSRAM_GATED:MAIA_PSRAM_CHANGED;
    }
    for(unsigned pass=0;pass<2U;++pass)for(unsigned i=0;i<MAIA_PSRAM_CONTROLLER_WORDS;++i){
        /* Boot exclusivity mandatory: this only detects changes between reads. */
        if(psram_read(PSRAM_SYS)!=r->roots[0]||psram_read(PSRAM_PERI)!=r->roots[1]||psram_read(PSRAM_RESET)!=r->roots[2]){
            roots_read(r->roots_after);return MAIA_PSRAM_CHANGED;
        }
        uint32_t word=psram_read(psram_address(i));
        if(pass)r->after[i]=word;else r->before[i]=word;
        ++r->controller_samples;
    }
    roots_read(r->roots_after);if(!roots_match(r))return MAIA_PSRAM_CHANGED;
    for(unsigned i=0;i<MAIA_PSRAM_CONTROLLER_WORDS;++i)if(r->before[i]!=r->after[i])return MAIA_PSRAM_CHANGED;
    return MAIA_PSRAM_OK;
}
