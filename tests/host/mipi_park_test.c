#include <assert.h>
#include <stdio.h>
#include <maia/soc/esp32p4_mipi_park.h>
#include "mipi_park_internal.h"
void maia_mipi_park_test_reset(void);
static const uintptr_t addresses[]={PARK_CPU,PARK_SYS,PARK_PHY,
  0x50081010,0x50081018,0x5008101C,0x50081058,0x50081134,0x50081234,0x50081334,0x50081434,
  PARK_DSI_POWER,PARK_CSI_HOST,PARK_CSI_POWER,PARK_DPI,PARK_FIFO,
  PARK_DSI_BRIDGE,PARK_CSI_BRIDGE,PARK_DSI_PHY,PARK_CSI_PHY};
static uint32_t regs[20];static unsigned reads,writes,context_ok,drop,late_dma,late_clock;
static uintptr_t write_addresses[4];static uint32_t write_values[4];
static unsigned index_of(uintptr_t p) {
    for (unsigned i=0;i<20U;++i) if (addresses[i] == p) return i;
    assert(!"unexpected register access");return 0;
}
uint32_t maia_mipi_park_test_read(uintptr_t p) {
    ++reads;
    if (writes && late_clock && p == PARK_CPU) return regs[0]^1U;
    if (writes && late_dma && p == 0x50081010) return 1U;
    return regs[index_of(p)];
}
void maia_mipi_park_test_write(uintptr_t p,uint32_t v) {
    assert(writes<4U);unsigned i=index_of(p);assert(i>=16U);
    unsigned mask=i == 18U ? 14U : 1U;
    assert(v == (regs[i]&~mask));
    write_addresses[writes]=p;write_values[writes++]=v;
    if (writes != drop) regs[i]=v;
}
int maia_mipi_park_test_context(void) { return (int)context_ok; }
static void setup(unsigned armed) {
    maia_mipi_park_test_reset();reads=writes=drop=late_dma=late_clock=0;context_ok=1;
    for (unsigned i=0;i<20U;++i) regs[i]=0;
    regs[0]=0xE6DFB7FF;regs[1]=0x0C0D5E78;regs[2]=0x2B3;
    for (unsigned i=16;i<20U;++i) regs[i]=0xA5000000U | ((armed&(1U<<(i-16U))) ? (i == 18U ? 14U : 1U) : 0U);
}
static void terminal(maia_mipi_park_result_t expected) {
    unsigned accesses=reads+writes;maia_mipi_park_report_t r;
    assert(maia_esp32p4_mipi_park_reset(&r) == expected);assert(reads+writes == accesses);
}
int main(void) {
    maia_mipi_park_report_t r;
    setup(15);assert(maia_esp32p4_mipi_park_reset(0) == MAIA_MIPI_PARK_INVALID);assert(!reads && !writes);
    context_ok=0;assert(maia_esp32p4_mipi_park_reset(&r) == MAIA_MIPI_PARK_CONTEXT);assert(!reads && !writes);
    for (unsigned mask=0;mask<16U;++mask) {
        setup(mask);assert(maia_esp32p4_mipi_park_reset(&r) == MAIA_MIPI_PARK_OK);
        assert(r.completed == MAIA_MIPI_PARK_VERIFIED && r.attempted_mask == mask);
        unsigned count=0;
        for (unsigned i=0;i<4U;++i) {
            assert(regs[16U+i] == 0xA5000000U);
            if (mask&(1U<<i)) { assert(write_addresses[count] == addresses[16U+i]);assert(write_values[count++] == 0xA5000000U); }
        }
        assert(writes == count);terminal(MAIA_MIPI_PARK_BUSY);
    }
    for (unsigned bad=3U;bad<=15U;++bad) {
        setup(15);regs[bad]=1U;
        assert(maia_esp32p4_mipi_park_reset(&r) == MAIA_MIPI_PARK_BUSY);assert(!writes);
    }
    setup(15);regs[18]|=1U;assert(maia_esp32p4_mipi_park_reset(&r) == MAIA_MIPI_PARK_BUSY);assert(!writes);
    const unsigned reg_index[]={0,1,1,1,1,2,2};
    const unsigned bit_index[]={13,5,10,11,12,0,4};
    for (unsigned i=0;i<7U;++i) {
        setup(15);regs[reg_index[i]]&=~(1U<<bit_index[i]);
        assert(maia_esp32p4_mipi_park_reset(&r) == MAIA_MIPI_PARK_BUSY);
        assert(reads == 3U && !writes); /* Never access a gated bus. */
    }
    for (unsigned d=1;d<=4U;++d) {
        setup(15);drop=d;assert(maia_esp32p4_mipi_park_reset(&r) == MAIA_MIPI_PARK_VERIFY);
        assert(writes == d && r.attempted_mask == (1U<<d)-1U);terminal(MAIA_MIPI_PARK_FAILED);
    }
    setup(15);late_dma=1;assert(maia_esp32p4_mipi_park_reset(&r) == MAIA_MIPI_PARK_VERIFY);
    assert(writes == 1U);terminal(MAIA_MIPI_PARK_FAILED);
    setup(15);late_clock=1;assert(maia_esp32p4_mipi_park_reset(&r) == MAIA_MIPI_PARK_VERIFY);
    assert(writes == 1U);terminal(MAIA_MIPI_PARK_FAILED);
    puts("PASS: native MIPI reset parking, all masks/order, live/gated refusal, preserved bits, readbacks and sticky failures");
    return 0;
}
