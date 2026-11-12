#include <assert.h>
#include <stdio.h>
#include <maia/soc/esp32p4_mipi_park.h>
#include "mipi_park_internal.h"
void maia_mipi_park_test_reset(void);
static const uintptr_t addresses[]={PARK_CPU,PARK_SYS,PARK_PHY,
  0x50081010,0x50081018,0x5008101C,0x50081058,0x50081134,0x50081234,0x50081334,0x50081434,
  PARK_DSI_POWER,PARK_CSI_HOST,PARK_CSI_POWER,PARK_DPI,PARK_FIFO,
  PARK_DSI_BRIDGE,PARK_CSI_BRIDGE,PARK_DSI_PHY,PARK_CSI_PHY,PARK_ISP_CLOCK,PARK_RESETS,PARK_NORST,PARK_ISP_CONTROL,PARK_ISP_CAMERA};
static uint32_t regs[25];static unsigned reads,writes,ctx,drop,gate_phase,late_isp,late_dma,late_norst;
static uintptr_t sequence[3];static uint32_t values[3];
static unsigned index_of(uintptr_t p) { for (unsigned i=0;i<25U;++i) if (addresses[i] == p) return i;assert(!"unexpected peripheral read");return 0; }
uint32_t maia_mipi_park_test_read(uintptr_t p) {
    ++reads;unsigned i=index_of(p);
    if (gate_phase && writes) assert(i<11U || (i>=20U && i<23U)); /* No MIPI MMIO after reset. */
    if (i>=23U) assert((regs[20]&(1U<<25U)) && !(regs[21]&(1U<<29U)) && !writes);
    if (writes && late_isp && p == PARK_ISP_CLOCK) return regs[i]|(1U<<25U);
    if (writes && late_dma && p == 0x50081010) return 1U;
    if (writes && late_norst && p == PARK_NORST) return regs[i]|(1U<<14U);
    return regs[i];
}
void maia_mipi_park_test_write(uintptr_t p,uint32_t v) {
    assert(gate_phase && writes<3U);
    sequence[writes]=p;values[writes++]=v;
    assert(p == PARK_RESETS || p == PARK_PHY || p == PARK_SYS);
    if (writes != drop) regs[index_of(p)]=v;
}
int maia_mipi_park_test_context(void) { return (int)ctx; }
static void setup(unsigned claim) {
    maia_mipi_park_test_reset();reads=writes=drop=gate_phase=late_isp=late_dma=late_norst=0;ctx=1;
    for (unsigned i=0;i<25U;++i) regs[i]=0;
    regs[0]=0xE6DFB7FF;regs[1]=0x0C0D5E78;regs[2]=0x2B3;
    regs[20]=0x807FFFFF;regs[21]=0x80000001;regs[22]=0x80000001;
    if (claim) { maia_mipi_park_report_t p;assert(maia_esp32p4_mipi_park_reset(&p) == MAIA_MIPI_PARK_OK);assert(!writes); }
    reads=writes=0;gate_phase=1;
}
static void terminal(maia_mipi_park_result_t result) {
    unsigned before=reads+writes;maia_mipi_gate_report_t r;
    assert(maia_esp32p4_mipi_gate_clocks(&r) == result);assert(before == reads+writes);
    maia_mipi_park_report_t p;
    assert(maia_esp32p4_mipi_park_reset(&p) == MAIA_MIPI_PARK_BUSY);assert(before == reads+writes);
}
int main(void) {
    maia_mipi_gate_report_t r;
    setup(0);assert(maia_esp32p4_mipi_gate_clocks(0) == MAIA_MIPI_PARK_INVALID);assert(!reads && !writes);
    assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_BUSY);assert(!reads && !writes);
    setup(1);ctx=0;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_CONTEXT);assert(!reads && !writes);
    setup(1);assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_OK);
    assert(r.completed == MAIA_MIPI_GATE_VERIFIED && r.attempted_mask == 7U && writes == 3U);
    assert(sequence[0] == 0x500E60C0 && values[0] == 0xBC000001);
    assert(sequence[1] == 0x500E603C && values[1] == 0x220);
    assert(sequence[2] == 0x500E6018 && values[2] == 0x0C0D4278);
    assert(r.reset_before == 0x80000001 && r.reset_after == 0xBC000001);
    assert(r.isp_clock == 0x807FFFFF && r.force_norst == 0x80000001);
    assert(regs[0] == 0xE6DFB7FF && regs[20] == r.isp_clock && regs[22] == r.force_norst);terminal(MAIA_MIPI_PARK_BUSY);
    setup(1);regs[21]|=PARK_RESET_MASK;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_OK);
    assert(writes == 2U && r.attempted_mask == 6U);terminal(MAIA_MIPI_PARK_BUSY);
    const unsigned clock_regs[]={0,1,1,1,1,2,2};
    const unsigned clock_bits[]={13,5,10,11,12,0,4};
    for (unsigned i=0;i<7U;++i) {
        setup(1);regs[clock_regs[i]]&=~(1U<<clock_bits[i]);
        assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_BUSY);assert(reads == 6U && !writes);
    }
    setup(1);regs[20]=0x02E82000;regs[23]=0x40002442;regs[24]=4;
    assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_OK);assert(r.isp_inspected == 1U && writes == 3U);terminal(MAIA_MIPI_PARK_BUSY);
    const uint32_t bad_ctrl[]={0x40002443,0x48002442,0x50002442,0x58002442};
    for (unsigned i=0;i<4U;++i) {
        setup(1);regs[20]|=1U<<25U;regs[23]=bad_ctrl[i];regs[24]=4;
        assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_BUSY);assert(!writes);
    }
    for (unsigned camera=0;camera<8U;++camera) if (camera != 4U) {
        setup(1);regs[20]|=1U<<25U;regs[23]=0x40002442;regs[24]=camera;
        assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_BUSY);assert(!writes);
    }
    setup(1);regs[20]|=1U<<25U;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_BUSY);assert(!writes);
    for (unsigned bit=11;bit<=14U;++bit) { setup(1);regs[22]|=1U<<bit;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_BUSY);assert(!writes); }
    for (unsigned bad=3U;bad<=19U;++bad) {
        setup(1);regs[bad]=1U;
        assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_BUSY);assert(!writes);
    }
    setup(1);regs[18]=2U;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_BUSY);assert(!writes);
    for (unsigned d=1;d<=3U;++d) {
        setup(1);drop=d;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_VERIFY);
        assert(writes == d && r.attempted_mask == (1U<<d)-1U);
        assert(r.completed == (maia_mipi_gate_stage_t)d);terminal(MAIA_MIPI_PARK_FAILED);
    }
    setup(1);late_isp=1;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_VERIFY);assert(writes == 1U);terminal(MAIA_MIPI_PARK_FAILED);
    setup(1);late_dma=1;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_VERIFY);assert(writes == 1U);terminal(MAIA_MIPI_PARK_FAILED);
    setup(1);late_norst=1;assert(maia_esp32p4_mipi_gate_clocks(&r) == MAIA_MIPI_PARK_VERIFY);assert(writes == 1U);terminal(MAIA_MIPI_PARK_FAILED);
    puts("PASS: ISP/reset guards, native claim, reset/PHY/bus order, no gated MMIO, preserved bits and sticky failures");
    return 0;
}
