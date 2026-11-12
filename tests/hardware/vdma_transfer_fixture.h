/* Private diagnostic plan only. Original MaiaRTOS MIT, pinned source ledger:
 * docs/audits/native-vdma-transfer.md. No production transfer API. */
#ifndef MAIA_VDMA_TRANSFER_FIXTURE_H
#define MAIA_VDMA_TRANSFER_FIXTURE_H
#include <stdint.h>
#include <maia/soc/esp32p4_regs.h>
#define DMA_FIXTURE_BYTES 32768U
#define DMA_FIXTURE_GUARD 128U
#define DMA_FIXTURE_SPAN (DMA_FIXTURE_BYTES+2U*DMA_FIXTURE_GUARD)
#define DMA_FIXTURE_TOTAL (2U*DMA_FIXTURE_SPAN)
#define DMA_FIXTURE_ALIAS 0x40000000U
#define DMA_CH0 (MAIA_ESP32P4_GDMA_BASE+0x100U)
#define DMA_CFG (MAIA_ESP32P4_GDMA_BASE+0x10U)
#define DMA_CHEN (MAIA_ESP32P4_GDMA_BASE+0x18U)
#define DMA_ABORT (MAIA_ESP32P4_GDMA_BASE+0x1CU)
#define DMA_RESET (MAIA_ESP32P4_GDMA_BASE+0x58U)
#define DMA_COMMON_STATUS (MAIA_ESP32P4_GDMA_BASE+0x50U)
/* M2M, 8-bit, increment both, single item/burst, master1 for both memories;
 * contiguous single block, one outstanding request each, no status writeback. */
#define DMA_FIXTURE_SIGNAL_RW 0xF83F7FFBU
#define DMA_FIXTURE_ERRORS 0x023F7FE0U
#define DMA_FIXTURE_CTL0 5U
#define DMA_FIXTURE_CFG1 0x18U
static inline unsigned dma_fixture_pattern(unsigned i,unsigned seed) { return 1U+(i+seed)%251U; }
static inline int dma_fixture_layout(uint32_t start,uint32_t end,uint32_t bss_end,uint32_t stack_start) {
    return !(start&127U) && start>=MAIA_ESP32P4_L2MEM_LOW && end>start &&
        end<=0x4FF28000U && end-start == DMA_FIXTURE_TOTAL && bss_end<=start && end<=stack_start;
}
typedef uint32_t (*dma_fixture_read_fn)(uintptr_t);
typedef void (*dma_fixture_write_fn)(uintptr_t,uint32_t);
/* Controller and channel already disabled; caller checks clocks/reset/all-channel
 * ownership. Read back exact configuration before any global/channel enable. */
static inline int dma_fixture_configure(uint32_t base,dma_fixture_read_fn read32,dma_fixture_write_fn write32) {
    const unsigned offsets[]={0x00,0x04,0x08,0x0C,0x10,0x18,0x1C,0x20,0x24,0x28,0x2C,0x90};
    const uint32_t values[]={base+DMA_FIXTURE_GUARD,0,base+DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD,0,
        DMA_FIXTURE_BYTES-1U,DMA_FIXTURE_CTL0,0,0,DMA_FIXTURE_CFG1,0,0,0};
    for (unsigned i=0;i<sizeof offsets/sizeof *offsets;++i) {
        uint32_t desired=offsets[i] == 0x90U ? read32(DMA_CH0+offsets[i])&~DMA_FIXTURE_SIGNAL_RW : values[i];
        write32(DMA_CH0+offsets[i],desired);
        if (read32(DMA_CH0+offsets[i]) != desired) return 0;
    }
    return 1;
}
#endif
