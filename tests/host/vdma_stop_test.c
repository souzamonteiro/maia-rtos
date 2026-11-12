#include <assert.h>
#include <stdio.h>
#include <maia/soc/esp32p4_vdma.h>
#include "vdma_internal.h"
void maia_vdma_test_reset(void);
static uint32_t cfg,chen,abortreg,resetreg,cpu,sys,fifo[4],disable_pending;
static unsigned reads,writes,context_ok,delay,drop_chen,drop_cfg,changed_clock,changed_cfg,changed_reset;
static unsigned cfg_clear_requested,initial_reads,never_global_done;
static uintptr_t written[4];static uint32_t values[4];
uint32_t maia_vdma_test_read(uintptr_t address) {
    ++reads;
    if (address == VDMA_CPU_CLOCK) return cpu ^ (changed_clock && writes ? 1U : 0U);
    if (address == VDMA_SYS_CLOCK) return sys;
    if (address == VDMA_CFG) {
        if (disable_pending && delay && !--delay) { chen&=~disable_pending;disable_pending=0; }
        if (cfg_clear_requested && !(chen&15U) && !never_global_done) cfg&=~1U;
        return cfg ^ (changed_cfg && writes ? 2U : 0U);
    }
    if (address == VDMA_CHEN) return chen ^ (initial_reads && !writes && reads>6U ? 1U : 0U);
    if (address == VDMA_ABORT) return abortreg;
    if (address == VDMA_RESET) return resetreg | (changed_reset && writes ? 1U : 0U);
    for (unsigned c=0;c<4U;++c) if (address == VDMA_FIFO(c)) return fifo[c];
    assert(!"unexpected MMIO read");return 0;
}
void maia_vdma_test_write(uintptr_t address,uint32_t value) {
    assert(writes<4U);written[writes]=address;values[writes++]=value;
    if (address == VDMA_CHEN) {
        assert(!(value&~0xF00U)); /* Only EN_WE strobes, never EN/SUSP/other channels. */
        if (!drop_chen) { disable_pending=(value>>8U)&15U;delay=3; }
    } else {
        assert(address == VDMA_CFG);
        if (!drop_cfg) { assert(!(value&1U));cfg_clear_requested=1;cfg=(cfg&1U)|(value&~1U); }
    }
}
int maia_vdma_test_context(void) { return (int)context_ok; }
static void setup(uint32_t enabled,uint32_t channels) {
    maia_vdma_test_reset();cfg=0xA2U|enabled;chen=channels;abortreg=resetreg=0;
    cpu=0x2000;sys=0x20;disable_pending=0;
    reads=writes=delay=drop_chen=drop_cfg=changed_clock=changed_cfg=changed_reset=0;
    cfg_clear_requested=initial_reads=never_global_done=0;context_ok=1;
    for (unsigned c=0;c<4U;++c) fifo[c]=0;
}
static void terminal(maia_vdma_result_t result) {
    maia_vdma_stop_report_t r;unsigned before=reads+writes;
    assert(maia_esp32p4_vdma_stop_all(4,&r) == result);assert(reads+writes == before);
}
int main(void) {
    maia_vdma_stop_report_t r;
    setup(0,0);assert(maia_esp32p4_vdma_stop_all(0,&r) == MAIA_VDMA_INVALID);
    assert(maia_esp32p4_vdma_stop_all(4097,&r) == MAIA_VDMA_INVALID);
    assert(maia_esp32p4_vdma_stop_all(1,0) == MAIA_VDMA_INVALID);assert(!reads && !writes);
    context_ok=0;assert(maia_esp32p4_vdma_stop_all(1,&r) == MAIA_VDMA_CONTEXT);assert(!reads && !writes);
    for (unsigned gates=0;gates<3U;++gates) {
        setup(0,0);cpu=(gates&1U) ? 0x2000U : 0;sys=(gates&2U) ? 0x20U : 0;
        assert(maia_esp32p4_vdma_stop_all(1,&r) == MAIA_VDMA_BUSY);assert(reads == 2U && !writes);
    }
    setup(0,0);assert(maia_esp32p4_vdma_stop_all(1,&r) == MAIA_VDMA_OK);
    assert(r.adopted && !r.may_have_changed && !writes && r.polls == 1U);
    assert(r.completed == MAIA_VDMA_STOP_VERIFIED && r.cfg == 0xA2U);terminal(MAIA_VDMA_BUSY);
    for (unsigned c=0;c<16U;++c) {
        setup(1,c);fifo[2]=3;
        assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_OK);
        assert(r.requested_channels == c && !r.adopted && r.may_have_changed && r.fifo_nonempty == 4U);
        assert(r.initial_cfg == 0xA3U && r.cfg == 0xA2U && !(r.channels&15U) && !r.abort);
        assert(r.polls == (c ? 3U : 1U));assert(writes == (c ? 2U : 1U));
        if (c) assert(written[0] == VDMA_CHEN && values[0] == c<<8U);
        assert(written[writes-1] == VDMA_CFG && values[writes-1] == 0xA2U);
        terminal(MAIA_VDMA_BUSY);
    }
    /* No global enable or rearm is ever written, even on timeout/fault. */
    setup(1,15);assert(maia_esp32p4_vdma_stop_all(2,&r) == MAIA_VDMA_TIMEOUT);
    assert(r.polls == 2U && r.completed == MAIA_VDMA_STOP_REQUESTED);terminal(MAIA_VDMA_FAILED);
    setup(1,15);drop_chen=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_TIMEOUT);terminal(MAIA_VDMA_FAILED);
    setup(1,0);drop_cfg=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_TIMEOUT);terminal(MAIA_VDMA_FAILED);
    setup(1,15);never_global_done=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_TIMEOUT);
    assert(!(r.channels&15U) && (r.cfg&1U));terminal(MAIA_VDMA_FAILED);
    setup(1,15);changed_clock=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_VERIFY);terminal(MAIA_VDMA_FAILED);
    setup(1,15);changed_cfg=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_VERIFY);terminal(MAIA_VDMA_FAILED);
    setup(1,15);changed_reset=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_VERIFY);terminal(MAIA_VDMA_FAILED);
    setup(0,1);assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_BUSY);assert(!writes);
    setup(1,1);abortreg=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_BUSY);assert(!writes);
    setup(1,1);resetreg=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_BUSY);assert(!writes);
    setup(1,1);initial_reads=1;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_VERIFY);assert(!writes);
    /* Pre-stop rejections do not claim lifetime reservation. */
    initial_reads=0;assert(maia_esp32p4_vdma_stop_all(4,&r) == MAIA_VDMA_OK);
    puts("PASS: VDMA stop, exact strobes/order, all channel masks, completion handshake, budget, sticky failures");
    return 0;
}
