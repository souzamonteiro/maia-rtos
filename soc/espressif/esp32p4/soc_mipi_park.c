/* Original MaiaRTOS MIT. Register semantics from ESP-IDF f5c3654 LL/headers;
 * boot/reset-state policy, not a copied SDK teardown. docs/audits/native-mipi-park.md. */
#include <maia/soc/esp32p4_mipi_park.h>
#include "mipi_park_internal.h"
enum { UNCLAIMED,OWNED,FAULTED };
static unsigned state,gate_state;
#ifdef MAIA_MIPI_PARK_TEST
void maia_mipi_park_test_reset(void);
void maia_mipi_park_test_reset(void) { state=UNCLAIMED;gate_state=UNCLAIMED; }
#endif
static int clocks(uint32_t cpu,uint32_t sys,uint32_t phy) {
    return park_read(PARK_CPU) == cpu && park_read(PARK_SYS) == sys && park_read(PARK_PHY) == phy;
}
static int dma_inactive(void) {
    if ((park_read(MAIA_ESP32P4_GDMA_BASE+0x10U)&1U) ||
        (park_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&15U) ||
        (park_read(MAIA_ESP32P4_GDMA_BASE+0x1CU)&15U) ||
        (park_read(MAIA_ESP32P4_GDMA_BASE+0x58U)&1U)) return 0;
    for (unsigned c=0;c<4U;++c) if (park_read(MAIA_ESP32P4_GDMA_BASE+0x134U+c*0x100U)&0x7FFFU) return 0;
    return 1;
}
static int inactive(void) {
    return dma_inactive() && !(park_read(PARK_DSI_POWER)&1U) && !(park_read(PARK_DSI_PHY)&1U) &&
        !(park_read(PARK_CSI_HOST)&1U) && !(park_read(PARK_CSI_POWER)&1U) &&
        !(park_read(PARK_DPI)&1U) && !(park_read(PARK_FIFO)&0x3FFFU);
}
maia_mipi_park_result_t maia_esp32p4_mipi_park_reset(maia_mipi_park_report_t *r) {
    if (!r) return MAIA_MIPI_PARK_INVALID;
    maia_mipi_park_report_t empty={0};*r=empty;
    if (!park_context()) return MAIA_MIPI_PARK_CONTEXT;
    if (state == OWNED) return MAIA_MIPI_PARK_BUSY;
    if (state == FAULTED) return MAIA_MIPI_PARK_FAILED;
    uint32_t cpu=park_read(PARK_CPU),sys=park_read(PARK_SYS),phy=park_read(PARK_PHY);
    if (!(cpu&(1U<<13U)) || (sys&0x1C20U) != 0x1C20U || (phy&0x11U) != 0x11U) return MAIA_MIPI_PARK_BUSY;
    if (!inactive()) return MAIA_MIPI_PARK_BUSY;
    const uintptr_t addresses[4]={PARK_DSI_BRIDGE,PARK_CSI_BRIDGE,PARK_DSI_PHY,PARK_CSI_PHY};
    const uint32_t masks[4]={1U,1U,14U,1U};uint32_t words[4];
    for (unsigned i=0;i<4U;++i) words[i]=park_read(addresses[i]);
    r->dsi_bridge=words[0];r->csi_bridge=words[1];r->dsi_phy=words[2];r->csi_phy=words[3];
    if (!clocks(cpu,sys,phy) || !inactive()) return MAIA_MIPI_PARK_VERIFY;
    for (unsigned i=0;i<4U;++i) if (park_read(addresses[i]) != words[i]) return MAIA_MIPI_PARK_VERIFY;
    r->completed=MAIA_MIPI_PARK_INSPECTED;state=FAULTED;
    for (unsigned i=0;i<4U;++i) {
        if (!clocks(cpu,sys,phy) || !inactive() || park_read(addresses[i]) != words[i]) return MAIA_MIPI_PARK_VERIFY;
        uint32_t desired=words[i]&~masks[i];
        if (desired != words[i]) { r->attempted_mask|=1U<<i;park_write(addresses[i],desired); }
        words[i]=park_read(addresses[i]);
        if (i == 0U) r->dsi_bridge=words[i];
        if (i == 1U) r->csi_bridge=words[i];
        if (i == 2U) r->dsi_phy=words[i];
        if (i == 3U) r->csi_phy=words[i];
        if (words[i] != desired) return MAIA_MIPI_PARK_VERIFY;
        if (i == 1U) r->completed=MAIA_MIPI_PARK_BRIDGES;
        if (i == 3U) r->completed=MAIA_MIPI_PARK_PHYS;
    }
    if (!clocks(cpu,sys,phy) || !inactive()) return MAIA_MIPI_PARK_VERIFY;
    for (unsigned i=0;i<4U;++i) if (park_read(addresses[i]) != words[i]) return MAIA_MIPI_PARK_VERIFY;
    r->completed=MAIA_MIPI_PARK_VERIFIED;state=OWNED;return MAIA_MIPI_PARK_OK;
}

/* Clock gate policy from the pinned SDK clock/reset primitives and TRM CSI stop
 * prerequisites. See docs/audits/native-mipi-gate.md. No foreign runtime. */
static int gate_guard(uint32_t cpu,uint32_t sys,uint32_t phy,uint32_t isp,uint32_t reset,uint32_t norst) {
    return clocks(cpu,sys,phy) && park_read(PARK_ISP_CLOCK) == isp &&
        park_read(PARK_RESETS) == reset && park_read(PARK_NORST) == norst && dma_inactive();
}
maia_mipi_park_result_t maia_esp32p4_mipi_gate_clocks(maia_mipi_gate_report_t *r) {
    if (!r) return MAIA_MIPI_PARK_INVALID;
    /* Explicit stores avoid freestanding compiler-emitted memset. */
    r->completed=MAIA_MIPI_GATE_NONE;r->attempted_mask=0;r->isp_clock=0;r->force_norst=0;
    r->isp_inspected=r->isp_control=r->isp_camera=0;
    r->reset_before=r->reset_after=0;r->phy_before=r->phy_after=0;r->sys_before=r->sys_after=0;
    if (!park_context()) return MAIA_MIPI_PARK_CONTEXT;
    if (gate_state == OWNED) return MAIA_MIPI_PARK_BUSY;
    if (gate_state == FAULTED) return MAIA_MIPI_PARK_FAILED;
    if (state != OWNED) return MAIA_MIPI_PARK_BUSY;
    uint32_t cpu=park_read(PARK_CPU),sys=park_read(PARK_SYS),phy=park_read(PARK_PHY);
    uint32_t isp=park_read(PARK_ISP_CLOCK),reset=park_read(PARK_RESETS),norst=park_read(PARK_NORST);
    r->isp_clock=isp;r->force_norst=norst;r->reset_before=r->reset_after=reset;
    r->phy_before=r->phy_after=phy;r->sys_before=r->sys_after=sys;
    if (!(cpu&(1U<<13U)) || (sys&0x1C20U) != 0x1C20U || (phy&0x11U) != 0x11U ||
        (norst&PARK_NORST_MASK)) return MAIA_MIPI_PARK_BUSY;
    if ((isp&(1U<<25U)) && !(reset&(1U<<29U))) {
        r->isp_inspected=1;r->isp_control=park_read(PARK_ISP_CONTROL);r->isp_camera=park_read(PARK_ISP_CAMERA);
        if ((r->isp_control&0x18000001U) || (r->isp_camera&7U) != 4U) return MAIA_MIPI_PARK_BUSY;
    }
    if (!inactive() || (park_read(PARK_DSI_BRIDGE)&1U) || (park_read(PARK_CSI_BRIDGE)&1U) ||
        (park_read(PARK_DSI_PHY)&14U) || (park_read(PARK_CSI_PHY)&1U)) return MAIA_MIPI_PARK_BUSY;
    if (!gate_guard(cpu,sys,phy,isp,reset,norst)) return MAIA_MIPI_PARK_VERIFY;
    if (r->isp_inspected && (park_read(PARK_ISP_CONTROL) != r->isp_control ||
        park_read(PARK_ISP_CAMERA) != r->isp_camera)) return MAIA_MIPI_PARK_VERIFY;
    r->completed=MAIA_MIPI_GATE_GUARDED;gate_state=FAULTED;
    uint32_t desired=reset|PARK_RESET_MASK;
    if (reset != desired) { r->attempted_mask|=1U;park_write(PARK_RESETS,desired); }
    r->reset_after=park_read(PARK_RESETS);
    if (r->reset_after != desired) return MAIA_MIPI_PARK_VERIFY;
    reset=desired;r->completed=MAIA_MIPI_GATE_RESET;
    /* No MIPI internal registers beyond this point: reset/bus gate safety. */
    if (!gate_guard(cpu,sys,phy,isp,reset,norst)) return MAIA_MIPI_PARK_VERIFY;
    desired=phy&~MAIA_ESP32P4_HP_MIPI_PHY_CLK_MASK;
    if (phy != desired) { r->attempted_mask|=2U;park_write(PARK_PHY,desired); }
    r->phy_after=park_read(PARK_PHY);
    if (r->phy_after != desired) return MAIA_MIPI_PARK_VERIFY;
    phy=desired;r->completed=MAIA_MIPI_GATE_PHY;
    if (!gate_guard(cpu,sys,phy,isp,reset,norst)) return MAIA_MIPI_PARK_VERIFY;
    desired=sys&~MAIA_ESP32P4_HP_MIPI_SYS_CLK_MASK;
    if (sys != desired) { r->attempted_mask|=4U;park_write(PARK_SYS,desired); }
    r->sys_after=park_read(PARK_SYS);
    if (r->sys_after != desired) return MAIA_MIPI_PARK_VERIFY;
    sys=desired;r->completed=MAIA_MIPI_GATE_SYS;
    if (!gate_guard(cpu,sys,phy,isp,reset,norst)) return MAIA_MIPI_PARK_VERIFY;
    r->completed=MAIA_MIPI_GATE_VERIFIED;gate_state=OWNED;return MAIA_MIPI_PARK_OK;
}
