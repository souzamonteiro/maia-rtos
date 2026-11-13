/* Original MaiaRTOS MIT. Sequencing policy from pinned Espressif register/LL
 * definitions, no imported runtime. docs/audits/native-dsi-access.md. */
#include <maia/soc/esp32p4_dsi_access.h>
#include <maia/soc/esp32p4_ldo3.h>
#include "dsi_access_internal.h"
enum { UNCLAIMED,OWNED,FAULTED };
static unsigned state,reference_state,phy_state,command_state,short_state;
#ifdef MAIA_DSI_ACCESS_TEST
void maia_dsi_access_test_reset(void);
void maia_dsi_access_test_reset(void) { state=UNCLAIMED;reference_state=UNCLAIMED;phy_state=UNCLAIMED;command_state=UNCLAIMED;short_state=UNCLAIMED; }
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

/* Original native reference-routing policy, primary sources native-dsi-reference.md. */
typedef struct reference_guard {
    uint32_t cpu,sys,phy,reset,isp,norst,divider,lp,provider,source;
    maia_ldo3_snapshot_t supply;
} reference_guard_t;
static int reference_guard(const reference_guard_t *g) {
    return guard(g->cpu,g->sys,g->phy,g->reset,g->isp,g->norst,&g->supply) &&
        dsi_read(DSI_REF_DIV) == g->divider && dsi_read(DSI_LP_GATE) == g->lp &&
        dsi_read(DSI_REF_GATE) == g->provider && dsi_read(DSI_REF_SOURCE) == g->source &&
        (!(g->phy&1U) || (!(dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+4U)&1U) &&
        !(dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U)&15U) &&
        !(dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U)&1U) &&
        !(dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x40U)&1U) &&
        !(dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x14U)&0x3FFFU)));
}
maia_dsi_access_result_t maia_esp32p4_dsi_reference20(maia_dsi_reference_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    r->completed=MAIA_DSI_REFERENCE_NONE;r->attempted_mask=0;
    r->divider_word=r->lp_gate=r->nominal_hz=0;r->provider_before=r->provider_after=0;
    r->source_before=r->source_after=r->phy_before=r->phy_after=0;
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (reference_state == OWNED) return MAIA_DSI_ACCESS_BUSY;
    if (reference_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (state != OWNED) return MAIA_DSI_ACCESS_BUSY;
    reference_guard_t g;
    g.cpu=dsi_read(DSI_CPU);g.sys=dsi_read(DSI_SYS);g.phy=dsi_read(DSI_PHY);
    g.reset=dsi_read(DSI_RESET);g.isp=dsi_read(DSI_ISP);g.norst=dsi_read(DSI_NORST);
    g.divider=dsi_read(DSI_REF_DIV);g.lp=dsi_read(DSI_LP_GATE);
    g.provider=dsi_read(DSI_REF_GATE);g.source=dsi_read(DSI_REF_SOURCE);
    r->divider_word=g.divider;r->lp_gate=g.lp;r->provider_before=r->provider_after=g.provider;
    r->source_before=r->source_after=g.source;r->phy_before=r->phy_after=g.phy;
    if (!(g.cpu&(1U<<13U)) || !(g.sys&(1U<<5U)) || (g.sys&0x1C00U) != 0x1000U ||
        (g.phy&0x93U) != 1U || (g.reset&0x3C000000U) != 0x38000000U || (g.norst&0x7800U) ||
        ((g.divider>>16U)&255U) != 23U || (g.lp&0x0A000000U) != 0x0A000000U || !dma_quiet()) return MAIA_DSI_ACCESS_BUSY;
    maia_ldo3_plan_t plan;
    if (maia_esp32p4_ldo3_snapshot(&g.supply) != MAIA_LDO3_OK ||
        maia_esp32p4_ldo3_plan2500(&g.supply,&plan) != MAIA_LDO3_OK ||
        (g.supply.control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (g.supply.analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) !=
            (((uint32_t)plan.dref<<28U)|((uint32_t)plan.mul<<23U)|(1U<<26U))) return MAIA_DSI_ACCESS_BUSY;
    if (!reference_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    r->completed=MAIA_DSI_REFERENCE_GUARDED;reference_state=FAULTED;
    uint32_t desired=g.provider|0x100U;
    if (desired != g.provider) { r->attempted_mask|=1U;dsi_write(DSI_REF_GATE,desired); }
    r->provider_after=dsi_read(DSI_REF_GATE);
    if (r->provider_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    g.provider=desired;r->completed=MAIA_DSI_REFERENCE_PROVIDER;
    if (!reference_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    /* Source drives both config and reference: gate config for a mux change. */
    unsigned switch_source=(g.source&0xC0000000U) != 0U;
    desired=switch_source ? g.phy&~1U : g.phy;
    if (desired != g.phy) { r->attempted_mask|=2U;dsi_write(DSI_PHY,desired); }
    r->phy_after=dsi_read(DSI_PHY);
    if (r->phy_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    g.phy=desired;r->completed=MAIA_DSI_REFERENCE_CONFIG_OFF;
    if (!reference_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    desired=g.source&~0xC0000000U;
    if (desired != g.source) { r->attempted_mask|=4U;dsi_write(DSI_REF_SOURCE,desired); }
    r->source_after=dsi_read(DSI_REF_SOURCE);
    if (r->source_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    g.source=desired;r->completed=MAIA_DSI_REFERENCE_SOURCE;
    if (!reference_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    desired=g.phy|1U;
    if (desired != g.phy) { r->attempted_mask|=8U;dsi_write(DSI_PHY,desired); }
    r->phy_after=dsi_read(DSI_PHY);
    if (r->phy_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    g.phy=desired;r->completed=MAIA_DSI_REFERENCE_CONFIG;
    if (!reference_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    desired=g.phy|2U;r->attempted_mask|=16U;dsi_write(DSI_PHY,desired);
    r->phy_after=dsi_read(DSI_PHY);
    if (r->phy_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    g.phy=desired;r->completed=MAIA_DSI_REFERENCE_CLOCK;
    if (!reference_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    r->nominal_hz=20000000U;r->completed=MAIA_DSI_REFERENCE_VERIFIED;reference_state=OWNED;
    return MAIA_DSI_ACCESS_OK;
}

/* Original native boot guard/poll policy; Apache-2.0 program sequence below. */
#include "dsi_phy_program.h"
static int phy_guard(const reference_guard_t *g,const uint32_t words[5]) {
    return guard(g->cpu,g->sys,g->phy,g->reset,g->isp,g->norst,&g->supply) &&
        dsi_read(DSI_REF_DIV) == g->divider && dsi_read(DSI_LP_GATE) == g->lp &&
        dsi_read(DSI_REF_GATE) == g->provider && dsi_read(DSI_REF_SOURCE) == g->source &&
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+4U) == words[0] &&
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U) == words[1] &&
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA4U) == words[2] &&
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x94U) == words[3] &&
        dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x40U) == words[4] &&
        !(dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U)&1U) && !(dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x14U)&0x3FFFU);
}
maia_dsi_access_result_t maia_esp32p4_dsi_phy960(uint32_t budget,maia_dsi_phy_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    r->completed=r->writes=r->programmed=r->polls=r->status=r->power=r->phy_reset=r->lanes=r->nominal_mbps=0;
    if (!budget || budget>100000U) return MAIA_DSI_ACCESS_INVALID;
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (phy_state == OWNED) return MAIA_DSI_ACCESS_BUSY;
    if (phy_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (reference_state != OWNED) return MAIA_DSI_ACCESS_BUSY;
    reference_guard_t g;
    g.cpu=dsi_read(DSI_CPU);g.sys=dsi_read(DSI_SYS);g.phy=dsi_read(DSI_PHY);
    g.reset=dsi_read(DSI_RESET);g.isp=dsi_read(DSI_ISP);g.norst=dsi_read(DSI_NORST);
    g.divider=dsi_read(DSI_REF_DIV);g.lp=dsi_read(DSI_LP_GATE);
    g.provider=dsi_read(DSI_REF_GATE);g.source=dsi_read(DSI_REF_SOURCE);
    if (!(g.cpu&0x2000U) || !(g.sys&0x20U) || (g.sys&0x1C00U) != 0x1000U ||
        (g.phy&0x93U) != 3U || (g.reset&0x3C000000U) != 0x38000000U || (g.norst&0x7800U) ||
        ((g.divider>>16U)&255U) != 23U || (g.lp&0x0A000000U) != 0x0A000000U ||
        !(g.provider&0x100U) || (g.source&0xC0000000U) || !dma_quiet()) return MAIA_DSI_ACCESS_BUSY;
    maia_ldo3_plan_t plan;
    if (maia_esp32p4_ldo3_snapshot(&g.supply) != MAIA_LDO3_OK ||
        maia_esp32p4_ldo3_plan2500(&g.supply,&plan) != MAIA_LDO3_OK ||
        (g.supply.control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (g.supply.analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) !=
          (((uint32_t)plan.dref<<28U)|((uint32_t)plan.mul<<23U)|(1U<<26U))) return MAIA_DSI_ACCESS_BUSY;
    uint32_t words[5]={dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+4U),dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U),
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA4U),dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x94U),
        dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x40U)};
    r->status=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U);
    if ((words[0]&1U) || (words[1]&15U) || (words[3]&3U) || (words[4]&1U) || (r->status&1U)) return MAIA_DSI_ACCESS_BUSY;
    if (!phy_guard(&g,words)) return MAIA_DSI_ACCESS_VERIFY;
    r->completed=1;phy_state=FAULTED;
    const unsigned indices[6]={2,0,1,1,1,1};
    const uint32_t masks[6]={3,1,1,2,4,8},values[6]={1,1,1,2,4,8};
    const uintptr_t addresses[3]={MAIA_ESP32P4_DSI_HOST_BASE+4U,MAIA_ESP32P4_DSI_HOST_BASE+0xA0U,MAIA_ESP32P4_DSI_HOST_BASE+0xA4U};
    for (unsigned step=0;step<6U;++step) {
        if (!phy_guard(&g,words)) return MAIA_DSI_ACCESS_VERIFY;
        unsigned i=indices[step];uint32_t desired=(words[i]&~masks[step])|values[step];
        if (desired != words[i]) { ++r->writes;dsi_write(addresses[i],desired); }
        if (dsi_read(addresses[i]) != desired) return MAIA_DSI_ACCESS_VERIFY;
        words[i]=desired;r->completed=2U+step;
    }
    for (unsigned i=0;i<5U;++i) {
        if (!phy_guard(&g,words)) return MAIA_DSI_ACCESS_VERIFY;
        r->writes+=7U; /* Conservative: byte helper may fail before all seven stores. */
        if (!phy_program_byte(phy_pll_registers[i],phy_pll_values[i])) return MAIA_DSI_ACCESS_VERIFY;
        ++r->programmed;
    }
    r->completed=8;
    for (unsigned i=0;i<budget;++i) {
        if (!phy_guard(&g,words)) return MAIA_DSI_ACCESS_VERIFY;
        r->status=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U);++r->polls;
        if (r->status&1U) {
            r->completed=9;
            if ((r->status&0x94U) == 0x94U) { r->completed=10;break; }
        }
    }
    if (r->completed != 10U) return MAIA_DSI_ACCESS_TIMEOUT;
    if (!phy_guard(&g,words)) return MAIA_DSI_ACCESS_VERIFY;
    r->power=words[0];r->phy_reset=words[1];r->lanes=words[2];r->nominal_mbps=960;
    r->completed=11;phy_state=OWNED;return MAIA_DSI_ACCESS_OK;
}

/* Original checked native policy; attributed SDK parameters in separate table. */
#include "dsi_command_program.h"
static uint32_t saved_command_table[MAIA_DSI_COMMAND_WORDS];
static int command_guard(const reference_guard_t *g,const uint32_t words[5],const uint32_t table[MAIA_DSI_COMMAND_WORDS]) {
    if (!phy_guard(g,words) || (dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U)&0x95U) != 0x95U ||
        (dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x74U)&0x7FU) != 0x15U) return 0;
    for (unsigned i=0;i<MAIA_DSI_COMMAND_WORDS;++i)
        if (dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+command_offsets[i]) != table[i]) return 0;
    return 1;
}
maia_dsi_access_result_t maia_esp32p4_dsi_command_config(maia_dsi_command_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    r->completed=r->attempted_mask=r->status=r->fifo=r->escape_hz=r->timeout_hz=0;
    for (unsigned i=0;i<MAIA_DSI_COMMAND_WORDS;++i) { r->before[i]=0;r->after[i]=0; }
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (command_state == OWNED) return MAIA_DSI_ACCESS_BUSY;
    if (command_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (phy_state != OWNED) return MAIA_DSI_ACCESS_BUSY;
    reference_guard_t g;
    g.cpu=dsi_read(DSI_CPU);g.sys=dsi_read(DSI_SYS);g.phy=dsi_read(DSI_PHY);
    g.reset=dsi_read(DSI_RESET);g.isp=dsi_read(DSI_ISP);g.norst=dsi_read(DSI_NORST);
    g.divider=dsi_read(DSI_REF_DIV);g.lp=dsi_read(DSI_LP_GATE);
    g.provider=dsi_read(DSI_REF_GATE);g.source=dsi_read(DSI_REF_SOURCE);
    if (!(g.cpu&0x2000U) || !(g.sys&0x20U) || (g.sys&0x1C00U) != 0x1000U ||
        (g.phy&0x93U) != 3U || (g.reset&0x3C000000U) != 0x38000000U || (g.norst&0x7800U) ||
        ((g.divider>>16U)&255U) != 23U || (g.lp&0x0A000000U) != 0x0A000000U ||
        !(g.provider&0x100U) || (g.source&0xC0000000U) || !dma_quiet()) return MAIA_DSI_ACCESS_BUSY;
    maia_ldo3_plan_t plan;
    if (maia_esp32p4_ldo3_snapshot(&g.supply) != MAIA_LDO3_OK ||
        maia_esp32p4_ldo3_plan2500(&g.supply,&plan) != MAIA_LDO3_OK ||
        (g.supply.control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (g.supply.analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) !=
          (((uint32_t)plan.dref<<28U)|((uint32_t)plan.mul<<23U)|(1U<<26U))) return MAIA_DSI_ACCESS_BUSY;
    uint32_t words[5]={dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+4U),dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U),
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA4U),dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x94U),
        dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x40U)};
    r->status=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U);r->fifo=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x74U);
    if (!(words[0]&1U) || (words[1]&15U) != 15U || (words[2]&3U) != 1U || (words[3]&3U) ||
        (words[4]&1U) || (r->status&0x95U) != 0x95U || (r->fifo&0x7FU) != 0x15U) return MAIA_DSI_ACCESS_BUSY;
    for (unsigned i=0;i<MAIA_DSI_COMMAND_WORDS;++i) r->before[i]=r->after[i]=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+command_offsets[i]);
    if (!command_guard(&g,words,r->after)) return MAIA_DSI_ACCESS_VERIFY;
    r->completed=1;command_state=FAULTED;
    for (unsigned i=0;i<MAIA_DSI_COMMAND_WORDS;++i) {
        if (!command_guard(&g,words,r->after)) return MAIA_DSI_ACCESS_VERIFY;
        uint32_t desired=(r->after[i]&~command_masks[i])|command_values[i];
        uintptr_t address=MAIA_ESP32P4_DSI_HOST_BASE+command_offsets[i];
        if (desired != r->after[i]) { r->attempted_mask|=1U<<i;dsi_write(address,desired); }
        r->after[i]=dsi_read(address);
        if (r->after[i] != desired) return MAIA_DSI_ACCESS_VERIFY;
        if (i == 12U) words[2]=desired;
        if (i == 13U) words[3]=desired;
        r->completed=2U+i;
    }
    if (!command_guard(&g,words,r->after)) return MAIA_DSI_ACCESS_VERIFY;
    r->status=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U);r->fifo=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x74U);
    if ((r->status&0x95U) != 0x95U || (r->fifo&0x7FU) != 0x15U) return MAIA_DSI_ACCESS_VERIFY;
    r->escape_hz=20000000U;r->timeout_hz=10000000U;
    for (unsigned i=0;i<MAIA_DSI_COMMAND_WORDS;++i) saved_command_table[i]=r->after[i];
    r->completed=16;command_state=OWNED;return MAIA_DSI_ACCESS_OK;
}

/* Original bounded native transfer policy. Header/LP fields follow pinned SDK
 * host HAL/LL; docs/audits/native-dsi-short.md. No SDK busy-wait loop imported. */
#include "dsi_short_packet.h"
static int short_bus_guard(const reference_guard_t *g) {
    return dsi_read(DSI_CPU) == g->cpu && dsi_read(DSI_SYS) == g->sys && dsi_read(DSI_PHY) == g->phy &&
        dsi_read(DSI_RESET) == g->reset && dsi_read(DSI_ISP) == g->isp && dsi_read(DSI_NORST) == g->norst &&
        dsi_read(DSI_REF_DIV) == g->divider && dsi_read(DSI_LP_GATE) == g->lp &&
        dsi_read(DSI_REF_GATE) == g->provider && dsi_read(DSI_REF_SOURCE) == g->source;
}
static int short_guard(const reference_guard_t *g,const uint32_t words[5],uint32_t mode) {
    if (!phy_guard(g,words) || !short_bus_guard(g) || dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x68U) != mode) return 0;
    for (unsigned i=0;i<MAIA_DSI_COMMAND_WORDS;++i)
        if (dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+command_offsets[i]) != saved_command_table[i]) return 0;
    return 1;
}
static int short_observe(maia_dsi_short_report_t *r) {
    r->fifo=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x74U);r->status=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U);
    r->error0|=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xBCU);
    r->error1|=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xC0U);
    return !(r->error0&0x1FFFFFU) && !(r->error1&0x81FFFU);
}
maia_dsi_access_result_t maia_esp32p4_dsi_dcs_short(uint32_t vc,uint32_t command,
    uint32_t count,uint32_t parameter,uint32_t budget,maia_dsi_short_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    r->completed=r->header=r->header_attempted=r->command_before=r->command_after=0;
    r->polls=r->status=r->fifo=r->error0=r->error1=r->saw_busy=0;
    if (vc>3U || command>255U || count>1U || parameter>255U || (!count && parameter) || !budget || budget>100000U)
        return MAIA_DSI_ACCESS_INVALID;
    r->header=short_packet_header(vc,command,count,parameter);
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (short_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (short_state == OWNED || command_state != OWNED) return MAIA_DSI_ACCESS_BUSY;
    reference_guard_t g;
    g.cpu=dsi_read(DSI_CPU);g.sys=dsi_read(DSI_SYS);g.phy=dsi_read(DSI_PHY);
    g.reset=dsi_read(DSI_RESET);g.isp=dsi_read(DSI_ISP);g.norst=dsi_read(DSI_NORST);
    g.divider=dsi_read(DSI_REF_DIV);g.lp=dsi_read(DSI_LP_GATE);
    g.provider=dsi_read(DSI_REF_GATE);g.source=dsi_read(DSI_REF_SOURCE);
    if (!(g.cpu&0x2000U) || !(g.sys&0x20U) || (g.sys&0x1C00U) != 0x1000U ||
        (g.phy&0x93U) != 3U || (g.reset&0x3C000000U) != 0x38000000U || (g.norst&0x7800U) ||
        ((g.divider>>16U)&255U) != 23U || (g.lp&0x0A000000U) != 0x0A000000U ||
        !(g.provider&0x100U) || (g.source&0xC0000000U) || !dma_quiet()) return MAIA_DSI_ACCESS_BUSY;
    maia_ldo3_plan_t plan;
    if (maia_esp32p4_ldo3_snapshot(&g.supply) != MAIA_LDO3_OK ||
        maia_esp32p4_ldo3_plan2500(&g.supply,&plan) != MAIA_LDO3_OK ||
        (g.supply.control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (g.supply.analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) !=
          (((uint32_t)plan.dref<<28U)|((uint32_t)plan.mul<<23U)|(1U<<26U))) return MAIA_DSI_ACCESS_BUSY;
    uint32_t words[5]={dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+4U),dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U),
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA4U),dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x94U),
        dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x40U)};
    r->command_before=r->command_after=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x68U);
    r->fifo=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x74U);r->status=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U);
    if (!(words[0]&1U) || (words[1]&15U) != 15U || (words[2]&3U) != 1U || (words[3]&3U) || (words[4]&1U) ||
        (r->status&0x95U) != 0x95U || (r->fifo&0x7FU) != 0x15U || (r->command_before&3U) || (saved_command_table[4]&4U)) return MAIA_DSI_ACCESS_BUSY;
    if (!short_guard(&g,words,r->command_after)) return MAIA_DSI_ACCESS_VERIFY;
    if (!short_observe(r)) { short_state=FAULTED;return MAIA_DSI_ACCESS_IO; }
    if ((r->status&0x95U) != 0x95U || (r->fifo&0x7FU) != 0x15U) return MAIA_DSI_ACCESS_BUSY;
    r->completed=1;short_state=FAULTED;
    uint32_t desired=short_packet_lp(r->command_before,count);
    if (desired != r->command_before) dsi_write(MAIA_ESP32P4_DSI_HOST_BASE+0x68U,desired);
    if (!short_bus_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    r->command_after=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x68U);
    if (r->command_after != desired || !short_guard(&g,words,desired)) return MAIA_DSI_ACCESS_VERIFY;
    r->completed=2;
    if (!short_observe(r)) return MAIA_DSI_ACCESS_IO;
    if ((r->status&0x95U) != 0x95U || (r->fifo&0x7FU) != 0x15U) return MAIA_DSI_ACCESS_VERIFY;
    r->header_attempted=1;dsi_write(MAIA_ESP32P4_DSI_HOST_BASE+0x6CU,r->header);r->completed=3;
    for (unsigned i=0;i<budget;++i) {
        if (!short_bus_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
        ++r->polls;
        if (!short_observe(r)) return MAIA_DSI_ACCESS_IO;
        if (!(r->status&1U) || (r->fifo&0x7CU) != 0x14U) return MAIA_DSI_ACCESS_IO;
        if (!short_guard(&g,words,desired)) return MAIA_DSI_ACCESS_VERIFY;
        if ((r->fifo&0x7FU) != 0x15U || (r->status&0x94U) != 0x94U) r->saw_busy=1;
        else { r->completed=4;break; }
    }
    if (r->completed != 4U) return MAIA_DSI_ACCESS_TIMEOUT;
    if (!short_guard(&g,words,desired) || !short_bus_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    if (!short_observe(r)) return MAIA_DSI_ACCESS_IO;
    if ((r->status&0x95U) != 0x95U || (r->fifo&0x7FU) != 0x15U) return MAIA_DSI_ACCESS_VERIFY;
    r->completed=5;short_state=UNCLAIMED;return MAIA_DSI_ACCESS_OK;
}
