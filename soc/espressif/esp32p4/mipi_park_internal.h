/* Private boot/MMIO seam, original MaiaRTOS MIT. Primary facts: native-mipi-park.md. */
#ifndef MAIA_MIPI_PARK_INTERNAL_H
#define MAIA_MIPI_PARK_INTERNAL_H
#include <maia/soc/esp32p4_regs.h>
#define PARK_CPU (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)
#define PARK_SYS MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG
#define PARK_PHY MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG
#define PARK_DSI_BRIDGE (MAIA_ESP32P4_DSI_BRG_BASE+4U)
#define PARK_CSI_BRIDGE 0x5009F804U
#define PARK_DSI_PHY (MAIA_ESP32P4_DSI_HOST_BASE+0xA0U)
#define PARK_CSI_PHY 0x5009F044U
#define PARK_DSI_POWER (MAIA_ESP32P4_DSI_HOST_BASE+4U)
#define PARK_CSI_HOST 0x5009F008U
#define PARK_CSI_POWER 0x5009F040U
#define PARK_DPI (MAIA_ESP32P4_DSI_BRG_BASE+0x40U)
#define PARK_FIFO (MAIA_ESP32P4_DSI_BRG_BASE+0x14U)
#ifdef MAIA_MIPI_PARK_TEST
int maia_mipi_park_test_context(void);
uint32_t maia_mipi_park_test_read(uintptr_t);
void maia_mipi_park_test_write(uintptr_t,uint32_t);
#define park_context maia_mipi_park_test_context
#define park_read maia_mipi_park_test_read
#define park_write maia_mipi_park_test_write
#else
static inline int park_context(void) {
    uint32_t hart,status;
    __asm__ volatile("csrr %0, mhartid":"=r"(hart));__asm__ volatile("csrr %0, mstatus":"=r"(status));
    return !hart && !(status&8U);
}
static inline uint32_t park_read(uintptr_t p) { return MAIA_MMIO32(p); }
static inline void park_write(uintptr_t p,uint32_t v) {
    MAIA_MMIO32(p)=v;__asm__ volatile("fence iorw, iorw" ::: "memory");
}
#endif
#endif
