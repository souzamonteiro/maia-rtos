#include <assert.h>
#include <stdio.h>
#include "vo3_cycle_fixture.h"
static const uintptr_t addresses[]={0x500E6014,0x500E6018,0x500E603C,0x500E60C0,0x500E60A8,0x500E60CC,
 0x50081010,0x50081018,0x5008101C,0x50081058,0x50081134,0x50081234,0x50081334,0x50081434,0x501151C0,0x501151C4};
static uint32_t regs[16];static unsigned reads,writes,drop;
static maia_mipi_gate_report_t gate;
static maia_ldo3_snapshot_t before;
static maia_ldo3_plan_t plan;
static uint32_t read32(uintptr_t p) { ++reads;for (unsigned i=0;i<16U;++i) if (addresses[i] == p) return regs[i];assert(!"unexpected MMIO");return 0; }
static void write32(uintptr_t p,uint32_t v) {
    assert(p == 0x501151C0U && v == 0xCD000080U);++writes;
    if (!drop) regs[14]=v;
}
static void setup(void) {
    reads=writes=drop=0;
    for (unsigned i=0;i<16U;++i) regs[i]=0;
    maia_mipi_gate_report_t empty={0};gate=empty;
    gate.completed=MAIA_MIPI_GATE_VERIFIED;gate.sys_after=0x0C0D4278;gate.phy_after=0x220;
    gate.reset_after=0xBC000001;gate.isp_clock=0x02E82000;gate.force_norst=0;
    regs[0]=0xE6DFB7FF;regs[1]=gate.sys_after;regs[2]=gate.phy_after;regs[3]=gate.reset_after;regs[4]=gate.isp_clock;
    maia_ldo3_snapshot_t s={0};before=s;before.control=regs[14]=0xCD000180;before.analog=regs[15]=0xDD912345;
    maia_ldo3_plan_t p={0};plan=p;plan.dref=13;plan.mul=3;
}
static int disable(void) { return vo3_cycle_disable(&gate,0xE6DFB7FF,&before,&plan,read32,write32); }
int main(void) {
    setup();assert(disable());assert(writes == 1U && regs[14] == 0xCD000080 && regs[15] == before.analog);
    assert(!disable() && writes == 1U); /* Observed control no longer matches original. */
    setup();drop=1;assert(!disable() && writes == 1U); /* No retry/compensation. */
    for (unsigned i=0;i<14U;++i) {
        setup();regs[i]^=1U;assert(!disable() && !writes);
    }
    setup();gate.completed=MAIA_MIPI_GATE_RESET;assert(!disable() && !writes && !reads);
    setup();gate.sys_after|=0x400;assert(!disable() && !writes && !reads);
    setup();gate.phy_after|=1U;assert(!disable() && !writes && !reads);
    setup();gate.reset_after&=~(1U<<29U);assert(!disable() && !writes && !reads);
    setup();gate.force_norst=0x4000;assert(!disable() && !writes && !reads);
    setup();regs[15]^=1U;assert(!disable() && !writes);
    setup();before.control|=1U<<14U;assert(!disable() && !writes && !reads);
    setup();before.analog^=1U<<28U;assert(!disable() && !writes && !reads);
    setup();plan.dref=16;assert(!disable() && !writes && !reads);
    setup();plan.mul=8;assert(!disable() && !writes && !reads);
    puts("PASS: VO3 disable only under matching calibrated state, held resets/gated clocks, inactive DMA; preserved fields and dropped-write failure");
    return 0;
}
