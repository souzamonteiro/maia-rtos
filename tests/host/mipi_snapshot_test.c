#include <assert.h>
#include <stdio.h>
#include "native_mipi_snapshot.h"
static unsigned calls,domains,unstable;
static uint32_t cpugate,sysgate;
static const uintptr_t allowed[]={
    0x500A0804,0x500A0840,0x500A0814,0x500A0004,0x500A0034,0x500A00A0,0x500A00B0,
    0x5009F008,0x5009F040,0x5009F044,0x5009F04C,0x5009F804,
    0x50081010,0x50081018,0x5008101C,
    0x50081120,0x50081124,0x50081130,0x50081134,0x50081100,0x50081108,0x50081128,
    0x50081220,0x50081224,0x50081230,0x50081234,0x50081200,0x50081208,0x50081228,
    0x50081320,0x50081324,0x50081330,0x50081334,0x50081300,0x50081308,0x50081328,
    0x50081420,0x50081424,0x50081430,0x50081434,0x50081400,0x50081408,0x50081428
};
static unsigned seen[sizeof allowed/sizeof *allowed];
static uint32_t read32(uintptr_t address) {
    ++calls;
    if (address == 0x500E6014) return cpugate ^ (unstable == 1U && calls>3U ? 1U : 0U);
    if (address == 0x500E6018) return sysgate ^ (unstable == 2U && calls>3U ? 1U : 0U);
    if (address == 0x500E603C) return 0x2B3 ^ (unstable == 3U && calls>3U ? 1U : 0U);
    unsigned i=0;
    while (i<sizeof allowed/sizeof *allowed && allowed[i] != address) ++i;
    assert(i<sizeof allowed/sizeof *allowed);
    unsigned domain=i<7U ? MIPI_DSI : i<11U ? MIPI_CSI_HOST : i<12U ? MIPI_CSI_BRG : MIPI_DMA;
    assert(domains&domain);assert(++seen[i] == 1U);
    return (uint32_t)address; /* Unique sentinel confirms every field and channel. */
}
static void run(unsigned expected,uint32_t cpu,uint32_t sys,unsigned changing) {
    domains=expected;cpugate=cpu;sysgate=sys;unstable=changing;calls=0;
    for (unsigned i=0;i<sizeof seen/sizeof *seen;++i) seen[i]=0;
    mipi_snapshot_t s;
    assert(mipi_snapshot(&s,read32) == !changing);assert(s.valid == expected);
    assert(s.cpu == cpu && s.sys == sys && s.phy == 0x2B3);
    unsigned total=6;
    for (unsigned i=0;i<sizeof seen/sizeof *seen;++i) {
        unsigned domain=i<7U ? MIPI_DSI : i<11U ? MIPI_CSI_HOST : i<12U ? MIPI_CSI_BRG : MIPI_DMA;
        assert(seen[i] == !!(expected&domain));total+=seen[i];
    }
    assert(calls == total);
    for (unsigned i=0;i<7U;++i) assert(s.dsi[i] == ((expected&MIPI_DSI) ? allowed[i] : 0U));
    for (unsigned i=0;i<4U;++i) assert(s.csi[i] == ((expected&MIPI_CSI_HOST) ? allowed[7U+i] : 0U));
    for (unsigned i=0;i<3U;++i) assert(s.dma[i] == ((expected&MIPI_DMA) ? allowed[12U+i] : 0U));
    for (unsigned c=0;c<4U;++c) for (unsigned i=0;i<7U;++i)
        assert(s.channel[c][i] == ((expected&MIPI_DMA) ? allowed[15U+7U*c+i] : 0U));
    assert(s.dsi[0] == ((expected&MIPI_DSI) ? 0x500A0804U : 0U));
    assert(s.csi[3] == ((expected&MIPI_CSI_HOST) ? 0x5009F04CU : 0U));
    assert(s.csi_bridge == ((expected&MIPI_CSI_BRG) ? 0x5009F804U : 0U));
    for (unsigned c=0;c<4U;++c) {
        assert(s.channel[c][4] == ((expected&MIPI_DMA) ? 0x50081100U+c*0x100U : 0U));
        assert(s.channel[c][6] == ((expected&MIPI_DMA) ? 0x50081128U+c*0x100U : 0U));
    }
}
int main(void) {
    /* Every clock-domain combination, including both half-enabled DMA gates. */
    for (unsigned mask=0;mask<32U;++mask) {
        uint32_t cpu=(mask&16U) ? 1U<<13U : 0U;
        uint32_t sys=((mask&1U) ? 1U<<12U : 0U) | ((mask&2U) ? 1U<<11U : 0U) |
            ((mask&4U) ? 1U<<10U : 0U) | ((mask&8U) ? 1U<<5U : 0U);
        run((mask&7U) | ((mask&24U) == 24U ? MIPI_DMA : 0U),cpu,sys,0);
    }
    for (unsigned changed=1;changed<=3U;++changed) run(15,1U<<13U,0x1C20,changed);
    puts("PASS: read-only whitelist, all gate combinations, four channels, clock-change rejection");
    return 0;
}
