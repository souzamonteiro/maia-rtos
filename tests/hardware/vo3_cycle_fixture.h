/* Private diagnostic only; not a public LDO release or power-management API.
 * Original MaiaRTOS MIT, pinned sources: docs/audits/native-vo3-cycle.md. */
#ifndef MAIA_VO3_CYCLE_FIXTURE_H
#define MAIA_VO3_CYCLE_FIXTURE_H
#include <maia/soc/esp32p4_ldo3.h>
#include <maia/soc/esp32p4_mipi_park.h>
#include <maia/soc/esp32p4_regs.h>
typedef uint32_t (*vo3_read_fn)(uintptr_t);
typedef void (*vo3_write_fn)(uintptr_t,uint32_t);
static inline int vo3_dma_quiet(vo3_read_fn rd) {
    if ((rd(MAIA_ESP32P4_GDMA_BASE+0x10U)&1U) || (rd(MAIA_ESP32P4_GDMA_BASE+0x18U)&15U) ||
        (rd(MAIA_ESP32P4_GDMA_BASE+0x1CU)&15U) || (rd(MAIA_ESP32P4_GDMA_BASE+0x58U)&1U)) return 0;
    for (unsigned c=0;c<4U;++c) if (rd(MAIA_ESP32P4_GDMA_BASE+0x134U+0x100U*c)&0x7FFFU) return 0;
    return 1;
}
/* Prepare only unused CSI register/config clocks. Require accessible idle DMA,
 * disconnected ISP inputs; assert CSI RCC resets, enable its access/config
 * clocks, release those resets, then inspect host/analog defaults. DSI/DMA
 * clocks and unrelated reset bits are never repaired blindly. */
static inline int vo3_prepare_csi(vo3_read_fn rd,vo3_write_fn wr) {
    uintptr_t sysreg=MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG,phyreg=MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG;
    uint32_t sys=rd(sysreg),phy=rd(phyreg);
    if (!(rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)&(1U<<13U)) ||
        (sys&0x1020U) != 0x1020U || !(phy&1U) || !vo3_dma_quiet(rd)) return 0;
    uint32_t isp=rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xA8U),reset=rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U);
    if (rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xCCU)&0x7800U) return 0;
    if ((isp&(1U<<25U)) && !(reset&(1U<<29U))) {
        if ((rd(0x500A1008U)&0x18000001U) || (rd(0x500A1114U)&7U) != 4U) return 0;
    }
    const uintptr_t resetreg=MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U;
    const uint32_t csi_reset_mask=0x18000000U;
    uint32_t held=reset|csi_reset_mask,ready=sys|0xC00U;
    if (rd(sysreg) != sys || rd(phyreg) != phy || rd(resetreg) != reset) return 0;
    if (held != reset) wr(resetreg,held);
    if (rd(resetreg) != held) return 0;
    if (ready != sys) wr(sysreg,ready);
    if (rd(sysreg) != ready) return 0;
    if (!(phy&(1U<<4U))) wr(phyreg,phy|(1U<<4U));
    if (rd(phyreg) != (phy|(1U<<4U)) || !vo3_dma_quiet(rd)) return 0;
    uint32_t released=reset&~csi_reset_mask;
    wr(resetreg,released);
    if (rd(resetreg) != released || rd(sysreg) != ready || rd(phyreg) != (phy|(1U<<4U))) return 0;
    return !(rd(0x5009F008U)&1U) && !(rd(0x5009F040U)&1U) && !(rd(0x5009F044U)&1U) && vo3_dma_quiet(rd);
}
/* Native park/gate already succeeded in this boot. Immutable observations are
 * comparisons, not an ownership token. All buses/producers remain exclusive. */
static inline int vo3_cycle_guard(const maia_mipi_gate_report_t *g,uint32_t cpu,vo3_read_fn rd) {
    if (g->completed != MAIA_MIPI_GATE_VERIFIED || (g->sys_after&0x1C00U) ||
        (g->phy_after&0x93U) || (g->reset_after&0x3C000000U) != 0x3C000000U || (g->force_norst&0x7800U)) return 0;
    if (rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U) != cpu ||
        rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG) != g->sys_after ||
        rd(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG) != g->phy_after ||
        rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U) != g->reset_after ||
        rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xA8U) != g->isp_clock ||
        rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xCCU) != g->force_norst) return 0;
    if (!(cpu&(1U<<13U)) || !(g->sys_after&(1U<<5U))) return 0;
    return vo3_dma_quiet(rd);
}
/* Single masked XPD request, only after exact matching calibrated active state.
 * Failure can leave VO3 off; caller stops until reset, no compensating writes. */
static inline int vo3_cycle_disable(const maia_mipi_gate_report_t *g,uint32_t cpu,
                                    const maia_ldo3_snapshot_t *before,const maia_ldo3_plan_t *plan,
                                    vo3_read_fn rd,vo3_write_fn wr) {
    if (plan->dref>15U || plan->mul>7U) return 0;
    uint32_t desired_analog=((uint32_t)plan->dref<<28U)|((uint32_t)plan->mul<<23U)|(1U<<26U);
    if ((before->control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (before->analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) != desired_analog) return 0;
    if (!vo3_cycle_guard(g,cpu,rd) || rd(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG) != before->control ||
        rd(MAIA_ESP32P4_PMU_EXT_LDO_VO3_ANA_REG) != before->analog) return 0;
    uint32_t off=before->control&~MAIA_ESP32P4_LDO3_XPD;
    wr(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG,off);
    return rd(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG) == off;
}
#endif
