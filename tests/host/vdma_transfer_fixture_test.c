#include <assert.h>
#include <stdio.h>
#include "vdma_transfer_fixture.h"
static unsigned count,drop;
static uintptr_t last_address;static uint32_t last_value;
static const unsigned offsets[]={0,4,8,12,16,24,28,32,36,40,44,144};
static const uint32_t values[]={0x4ff08080,0,0x4ff10180,0,32767,5,0,0,24,0,0,0x07C08004U};
static void write32(uintptr_t address,uint32_t value) {
    assert(count<12U);assert(address == 0x50081100U+offsets[count]);assert(value == values[count]);
    ++count;last_address=address;last_value=value;
}
static uint32_t read32(uintptr_t address) { if (address == 0x50081190U && count == 11U) return 0xFFFFFFFFU;assert(address == last_address);return last_value ^ (drop == count ? 1U : 0U); }
int main(void) {
    assert(DMA_FIXTURE_TOTAL == 66048U);
    assert(dma_fixture_layout(0x4ff08000,0x4ff18200,0x4ff08000,0x4ff18200));
    assert(!dma_fixture_layout(0x4ff08001,0x4ff18201,0,0x4ff28000));
    assert(!dma_fixture_layout(0x4ff08000,0x4ff18200,0x4ff08001,0x4ff28000));
    assert(!dma_fixture_layout(0x4ff08000,0x4ff18200,0,0x4ff181ff));
    assert(!dma_fixture_layout(0x4ff18000,0x4ff28200,0,0xffffffff));
    assert(!dma_fixture_layout(0xffffff80,0x10180,0,0xffffffff));
    assert(!dma_fixture_layout(0x4ff08000,0x4ff18280,0,0xffffffff));
    for (drop=0;drop<=12U;++drop) {
        count=0;assert(dma_fixture_configure(0x4ff08000,read32,write32) == !drop);
        assert(count == (drop ? drop : 12U));
    }
    for (unsigned i=0;i<32768U;++i) {
        assert(dma_fixture_pattern(i,0)>0 && dma_fixture_pattern(i,0)<252U);
        assert(dma_fixture_pattern(i,73) != dma_fixture_pattern(i,0));
    }
    puts("PASS: reserved ranges, exact M2M setup, readback rejection at all12 writes and unambiguous patterns");
    return 0;
}
