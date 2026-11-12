/* Private read-only diagnostic, not a production driver or quiescence API.
 * Register facts: ESP-IDF f5c3654, see docs/audits/native-mipi-state.md.
 * Original MaiaRTOS code, MIT. No vendor headers/runtime required. */
#ifndef MAIA_NATIVE_MIPI_SNAPSHOT_H
#define MAIA_NATIVE_MIPI_SNAPSHOT_H
#include <stdint.h>
#include <maia/soc/esp32p4_regs.h>
enum { MIPI_DSI=1, MIPI_CSI_HOST=2, MIPI_CSI_BRG=4, MIPI_DMA=8 };
typedef uint32_t (*mipi_read_fn)(uintptr_t);
typedef struct {
    uint32_t cpu,sys,phy,cpu_after,sys_after,phy_after,valid;
    uint32_t dsi[7],csi[4],csi_bridge,dma[3],channel[4][7];
} mipi_snapshot_t;
/* Read-only whitelist. No IRQ/status-clear, command, descriptor or memory reads.
 * Skipped blocks remain zero; valid distinguishes skipped from measured zero.
 * Exclusive boot access is required. Clock recheck detects some changes only;
 * this is a sequential observation, not an atomic snapshot or DMA idle proof. */
static inline int mipi_snapshot(mipi_snapshot_t *s,mipi_read_fn read32) {
    const uintptr_t cpu=MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U;
    const uintptr_t sys=MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG;
    const uintptr_t phy=MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG;
    const unsigned dsi_offsets[7]={0x04,0x40,0x14,0x04,0x34,0xA0,0xB0};
    const unsigned csi_offsets[4]={0x08,0x40,0x44,0x4C};
    const unsigned dma_offsets[3]={0x10,0x18,0x1C};
    const unsigned ch_offsets[7]={0x20,0x24,0x30,0x34,0x00,0x08,0x28};
    /* Volatile byte stores keep freestanding GCC from emitting libc memset. */
    volatile unsigned char *bytes=(volatile unsigned char *)s;
    for (unsigned i=0;i<sizeof *s;++i) bytes[i]=0;
    s->cpu=read32(cpu);s->sys=read32(sys);s->phy=read32(phy);
    if (s->sys & (1U<<12U)) {
        s->valid|=MIPI_DSI;
        for (unsigned i=0;i<7U;++i) s->dsi[i]=read32((i<3U ? MAIA_ESP32P4_DSI_BRG_BASE : MAIA_ESP32P4_DSI_HOST_BASE)+dsi_offsets[i]);
    }
    if (s->sys & (1U<<11U)) {
        s->valid|=MIPI_CSI_HOST;
        for (unsigned i=0;i<4U;++i) s->csi[i]=read32(0x5009F000U+csi_offsets[i]);
    }
    if (s->sys & (1U<<10U)) { s->valid|=MIPI_CSI_BRG;s->csi_bridge=read32(0x5009F804U); }
    if ((s->cpu & (1U<<13U)) && (s->sys & (1U<<5U))) {
        s->valid|=MIPI_DMA;
        for (unsigned i=0;i<3U;++i) s->dma[i]=read32(MAIA_ESP32P4_GDMA_BASE+dma_offsets[i]);
        for (unsigned c=0;c<4U;++c) for (unsigned i=0;i<7U;++i)
            s->channel[c][i]=read32(MAIA_ESP32P4_GDMA_BASE+0x100U*(c+1U)+ch_offsets[i]);
    }
    s->cpu_after=read32(cpu);s->sys_after=read32(sys);s->phy_after=read32(phy);
    return s->cpu == s->cpu_after && s->sys == s->sys_after && s->phy == s->phy_after;
}
#endif
