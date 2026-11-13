/* Original MaiaRTOS MIT. Sequencing policy from pinned Espressif register/LL
 * definitions, no imported runtime. docs/audits/native-dsi-access.md. */
#include <maia/soc/esp32p4_dsi_access.h>
#include <maia/soc/esp32p4_ldo3.h>
#include "dsi_access_internal.h"
enum { UNCLAIMED,OWNED,FAULTED };
static unsigned state;
#ifdef MAIA_DSI_ACCESS_TEST
void maia_dsi_access_test_reset(void);
void maia_dsi_access_test_reset(void) { state=UNCLAIMED; }
#endif
static int dma_quiet(void) {
    if ((dsi_read(MAIA_ESP32P4_GDMA_BASE+0x10U)&1U) || (dsi_read(MAIA_ESP32P4_GDMA_BASE+0x18U)&15U) ||
        (dsi_read(MAIA_ESP32P4_GDMA_BASE+0x1CU)&15U) || (dsi_read(MAIA_ESP32P4_GDMA_BASE+0x58U)&1U)) return 0;
    for (unsigned c=0;c<4U;++c) if (dsi_read(MAIA_ESP32P4_GDMA_BASE+0x134U+0x100U*c)&0x7FFFU) return 0;
    return 1;
}
static int supply_same(const maia_ldo3_snapshot_t *a) {
    maia_ldo3_snapshot_t b;
    return maia_esp32p4_ldo3_snapshot(&b) == MAIA_LDO3_OK && a->control == b.control && a->analog == b.analog &&
        a->mac_sys2 == b.mac_sys2 && a->mac_sys3 == b.mac_sys3 && a->rs_error0 == b.rs_error0;
}
static int guard(uint32_t cpu,uint32_t sys,uint32_t phy,uint32_t reset,uint32_t isp,uint32_t norst,
                 const maia_ldo3_snapshot_t *s) {
    return dsi_read(DSI_CPU) == cpu && dsi_read(DSI_SYS) == sys && dsi_read(DSI_PHY) == phy &&
        dsi_read(DSI_RESET) == reset && dsi_read(DSI_ISP) == isp && dsi_read(DSI_NORST) == norst &&
        dma_quiet() && supply_same(s);
}
maia_dsi_access_result_t maia_esp32p4_dsi_open_config(maia_dsi_access_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    r->completed=MAIA_DSI_ACCESS_NONE;r->attempted_mask=0;
    r->sys_before=r->sys_after=r->phy_before=r->phy_after=r->reset_before=r->reset_after=0;
    r->control=r->analog=r->version=r->power=r->phy_reset=r->bridge=r->dpi=r->fifo=r->phy_status=0;
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (state == OWNED) return MAIA_DSI_ACCESS_BUSY;
    if (state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    uint32_t cpu=dsi_read(DSI_CPU),sys=dsi_read(DSI_SYS),phy=dsi_read(DSI_PHY);
    uint32_t reset=dsi_read(DSI_RESET),isp=dsi_read(DSI_ISP),norst=dsi_read(DSI_NORST);
    r->sys_before=r->sys_after=sys;r->phy_before=r->phy_after=phy;r->reset_before=r->reset_after=reset;
    if (!(cpu&(1U<<13U)) || !(sys&(1U<<5U)) || (sys&0x1C00U) || (phy&0x93U) ||
        (reset&0x3C000000U) != 0x3C000000U || (norst&0x7800U) || !dma_quiet()) return MAIA_DSI_ACCESS_BUSY;
    maia_ldo3_snapshot_t supply;maia_ldo3_plan_t plan;
    if (maia_esp32p4_ldo3_snapshot(&supply) != MAIA_LDO3_OK ||
        maia_esp32p4_ldo3_plan2500(&supply,&plan) != MAIA_LDO3_OK) return MAIA_DSI_ACCESS_BUSY;
    r->control=supply.control;r->analog=supply.analog;
    uint32_t desired_analog=((uint32_t)plan.dref<<28U)|((uint32_t)plan.mul<<23U)|(1U<<26U);
    if ((supply.control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (supply.analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) != desired_analog) return MAIA_DSI_ACCESS_BUSY;
    if (!guard(cpu,sys,phy,reset,isp,norst,&supply)) return MAIA_DSI_ACCESS_VERIFY;
    r->completed=MAIA_DSI_ACCESS_GUARDED;state=FAULTED;
    uint32_t desired=sys|(1U<<12U);
    r->attempted_mask|=1U;dsi_write(DSI_SYS,desired);r->sys_after=dsi_read(DSI_SYS);
    if (r->sys_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    sys=desired;r->completed=MAIA_DSI_ACCESS_BUS;
    if (!guard(cpu,sys,phy,reset,isp,norst,&supply)) return MAIA_DSI_ACCESS_VERIFY;
    desired=phy|1U;
    r->attempted_mask|=2U;dsi_write(DSI_PHY,desired);r->phy_after=dsi_read(DSI_PHY);
    if (r->phy_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    phy=desired;r->completed=MAIA_DSI_ACCESS_CONFIG;
    if (!guard(cpu,sys,phy,reset,isp,norst,&supply)) return MAIA_DSI_ACCESS_VERIFY;
    desired=reset&~(1U<<26U);
    r->attempted_mask|=4U;dsi_write(DSI_RESET,desired);r->reset_after=dsi_read(DSI_RESET);
    if (r->reset_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    reset=desired;r->completed=MAIA_DSI_ACCESS_RELEASED;
    if (!guard(cpu,sys,phy,reset,isp,norst,&supply)) return MAIA_DSI_ACCESS_VERIFY;
    r->version=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE);
    r->power=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+4U);
    r->phy_reset=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U);
    r->phy_status=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U);
    r->bridge=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U);
    r->dpi=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x40U);
    r->fifo=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x14U);
    if ((r->power&1U) || (r->phy_reset&15U) || (r->bridge&1U) || (r->dpi&1U) || (r->fifo&0x3FFFU) ||
        !guard(cpu,sys,phy,reset,isp,norst,&supply)) return MAIA_DSI_ACCESS_VERIFY;
    r->completed=MAIA_DSI_ACCESS_VERIFIED;state=OWNED;return MAIA_DSI_ACCESS_OK;
}
