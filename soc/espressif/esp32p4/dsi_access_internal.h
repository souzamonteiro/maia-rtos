#ifndef MAIA_DSI_ACCESS_INTERNAL_H
#define MAIA_DSI_ACCESS_INTERNAL_H
#include <maia/soc/esp32p4_regs.h>
#define DSI_CPU (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)
#define DSI_SYS MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG
#define DSI_PHY MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG
#define DSI_RESET (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U)
#define DSI_ISP (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xA8U)
#define DSI_NORST (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xCCU)
#ifdef MAIA_DSI_ACCESS_TEST
int maia_dsi_access_test_context(void);
uint32_t maia_dsi_access_test_read(uintptr_t address);
void maia_dsi_access_test_write(uintptr_t address,uint32_t value);
#define dsi_context maia_dsi_access_test_context
#define dsi_read maia_dsi_access_test_read
#define dsi_write maia_dsi_access_test_write
#else
static inline int dsi_context(void) {
    uint32_t hart,status;
    __asm__ volatile("csrr %0, mhartid":"=r"(hart));__asm__ volatile("csrr %0, mstatus":"=r"(status));
    return !hart && !(status&8U);
}
static inline uint32_t dsi_read(uintptr_t address) { return MAIA_MMIO32(address); }
static inline void dsi_write(uintptr_t address,uint32_t value) {
    MAIA_MMIO32(address)=value;__asm__ volatile("fence iorw, iorw" ::: "memory");
}
#endif
#endif
