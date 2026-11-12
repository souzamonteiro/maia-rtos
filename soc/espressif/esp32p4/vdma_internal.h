/* Private native boot access / host fixture seam. Original MaiaRTOS MIT. */
#ifndef MAIA_ESP32P4_VDMA_INTERNAL_H
#define MAIA_ESP32P4_VDMA_INTERNAL_H
#include <maia/soc/esp32p4_regs.h>
#define VDMA_CFG (MAIA_ESP32P4_GDMA_BASE+0x10U)
#define VDMA_CHEN (MAIA_ESP32P4_GDMA_BASE+0x18U)
#define VDMA_ABORT (MAIA_ESP32P4_GDMA_BASE+0x1CU)
#define VDMA_RESET (MAIA_ESP32P4_GDMA_BASE+0x58U)
#define VDMA_FIFO(c) (MAIA_ESP32P4_GDMA_BASE+0x134U+0x100U*(c))
#define VDMA_CPU_CLOCK (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)
#define VDMA_SYS_CLOCK MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG
#ifdef MAIA_VDMA_TEST
uint32_t maia_vdma_test_read(uintptr_t address);
void maia_vdma_test_write(uintptr_t address,uint32_t value);
int maia_vdma_test_context(void);
#define vdma_read maia_vdma_test_read
#define vdma_write maia_vdma_test_write
#define vdma_context maia_vdma_test_context
#else
static inline int vdma_context(void) {
    uint32_t hart,status;
    __asm__ volatile("csrr %0, mhartid":"=r"(hart));
    __asm__ volatile("csrr %0, mstatus":"=r"(status));
    return hart == 0U && !(status&8U);
}
static inline uint32_t vdma_read(uintptr_t address) { return MAIA_MMIO32(address); }
static inline void vdma_write(uintptr_t address,uint32_t value) {
    MAIA_MMIO32(address)=value;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}
#endif
#endif
