/* Original MaiaRTOS MIT implementation from register/protocol facts.
 * ESP32-P4 rev1.3 TRM v0.4, 5.5.7 pp337-338 and CFG0 p349;
 * ESP-IDF f5c3654 dw_gdma_ll channel-enable strobes. No foreign runtime/code.
 * Source URLs, scope and tests: docs/audits/native-vdma-stop.md. */
#include <maia/soc/esp32p4_vdma.h>
#include "vdma_internal.h"
enum { UNCLAIMED,OWNED,FAULTED };
static unsigned state;
#ifdef MAIA_VDMA_TEST
void maia_vdma_test_reset(void);
void maia_vdma_test_reset(void) { state=UNCLAIMED; }
#endif
static int clocks(uint32_t cpu,uint32_t sys) {
    return vdma_read(VDMA_CPU_CLOCK) == cpu && vdma_read(VDMA_SYS_CLOCK) == sys;
}
maia_vdma_result_t maia_esp32p4_vdma_stop_all(uint32_t limit,maia_vdma_stop_report_t *r) {
    if (!r) return MAIA_VDMA_INVALID;
    maia_vdma_stop_report_t empty={0};*r=empty;
    if (!limit || limit>4096U) return MAIA_VDMA_INVALID;
    if (!vdma_context()) return MAIA_VDMA_CONTEXT;
    if (state == OWNED) return MAIA_VDMA_BUSY;
    if (state == FAULTED) return MAIA_VDMA_FAILED;
    uint32_t cpu=vdma_read(VDMA_CPU_CLOCK),sys=vdma_read(VDMA_SYS_CLOCK);
    if (!(cpu&(1U<<13U)) || !(sys&(1U<<5U))) return MAIA_VDMA_BUSY;
    r->cfg=r->initial_cfg=vdma_read(VDMA_CFG);
    r->channels=r->initial_channels=vdma_read(VDMA_CHEN);
    r->abort=vdma_read(VDMA_ABORT);
    if ((vdma_read(VDMA_RESET)&1U) || (r->abort&15U) ||
        (!(r->cfg&1U) && (r->channels&15U))) return MAIA_VDMA_BUSY;
    if (!clocks(cpu,sys) || vdma_read(VDMA_CFG) != r->cfg ||
        vdma_read(VDMA_CHEN) != r->channels || vdma_read(VDMA_ABORT) != r->abort)
        return MAIA_VDMA_VERIFY;
    r->completed=MAIA_VDMA_STOP_INSPECTED;
    state=FAULTED; /* No rollback or reentry after entering the stop phase. */
    r->requested_channels=r->channels&15U;
    if (r->requested_channels) {
        r->may_have_changed=1U;
        vdma_write(VDMA_CHEN,r->requested_channels<<8U); /* EN_WE=1, EN=0 only. */
    }
    if (r->initial_cfg&1U) {
        r->may_have_changed=1U;
        vdma_write(VDMA_CFG,r->initial_cfg&~1U);
    }
    r->completed=MAIA_VDMA_STOP_REQUESTED;
    for (unsigned i=0;i<limit;++i) {
        r->polls=i+1U;
        if (!clocks(cpu,sys)) return MAIA_VDMA_VERIFY;
        r->cfg=vdma_read(VDMA_CFG);r->channels=vdma_read(VDMA_CHEN);r->abort=vdma_read(VDMA_ABORT);
        if ((r->cfg&~1U) != (r->initial_cfg&~1U) || (r->abort&15U) ||
            (vdma_read(VDMA_RESET)&1U)) return MAIA_VDMA_VERIFY;
        if ((r->cfg&1U) || (r->channels&15U)) continue;
        for (unsigned c=0;c<4U;++c) if (vdma_read(VDMA_FIFO(c))&0x7FFFU) r->fifo_nonempty|=1U<<c;
        /* Activity can change between observations only if caller violates
         * exclusive ownership; recheck, without claiming atomicity. */
        if (!clocks(cpu,sys) || vdma_read(VDMA_CFG) != r->cfg ||
            vdma_read(VDMA_CHEN) != r->channels || vdma_read(VDMA_ABORT) != r->abort)
            return MAIA_VDMA_VERIFY;
        r->adopted=!r->may_have_changed;r->completed=MAIA_VDMA_STOP_VERIFIED;
        state=OWNED;return MAIA_VDMA_OK;
    }
    return MAIA_VDMA_TIMEOUT;
}
