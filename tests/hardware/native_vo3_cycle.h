/* Included only by opt-in native MIPI/VO3 diagnostic, not a driver service. */
#include "vo3_cycle_fixture.h"
static int vo3_same(const maia_ldo3_snapshot_t *a,const maia_ldo3_snapshot_t *b) {
    return a->control == b->control && a->analog == b->analog && a->mac_sys2 == b->mac_sys2 &&
        a->mac_sys3 == b->mac_sys3 && a->rs_error0 == b->rs_error0;
}
static void vo3_wait(int expected) {
    for (unsigned i=0;i<10000000U;++i) if (maia_serial_rx_read(NULL) == expected) return;
    halt("vo3-host-timeout");
}
static void vo3_cycle(const maia_mipi_gate_report_t *g,uint32_t cpu) {
    maia_ldo3_snapshot_t before,off,now;maia_ldo3_plan_t plan;
    if (!vo3_cycle_guard(g,cpu,rd) || maia_esp32p4_ldo3_snapshot(&before) != MAIA_LDO3_OK ||
        maia_esp32p4_ldo3_plan2500(&before,&plan) != MAIA_LDO3_OK) halt("vo3-plan");
    uint32_t other[10];unsigned n=0;
    for (unsigned slot=0;slot<6U;++slot) if (slot != 1U) {
        other[n++]=rd(MAIA_ESP32P4_PMU_BASE+0x1B8U+8U*slot);other[n++]=rd(MAIA_ESP32P4_PMU_BASE+0x1BCU+8U*slot);
    }
    field("VO3_PLAN dref=",plan.dref);field(" mul=",plan.mul);field(" defaults=",plan.default_mask);
    field(" control=",before.control);field(" analog=",before.analog);maia_console_puts("\n");
    for (unsigned i=0;i<16U;++i) if (maia_esp32p4_ldo3_snapshot(&now) != MAIA_LDO3_OK || !vo3_same(&before,&now)) halt("vo3-initial-unstable");
    if (!vo3_cycle_disable(g,cpu,&before,&plan,rd,wr)) halt("vo3-disable");
    off=before;off.control&=~MAIA_ESP32P4_LDO3_XPD;
    if (maia_esp32p4_ldo3_snapshot(&now) != MAIA_LDO3_OK || !vo3_same(&off,&now)) halt("vo3-off-readback");
    field("VO3_OFF control=",now.control);field(" analog=",now.analog);maia_console_puts("\nVO3_CYCLE_OFF_READY\n");
    vo3_wait('O'); /* Host observes >=20ms after receiving READY, not an analog spec. */
    for (unsigned i=0;i<16U;++i) if (maia_esp32p4_ldo3_snapshot(&now) != MAIA_LDO3_OK || !vo3_same(&off,&now)) halt("vo3-off-unstable");
    if (!vo3_cycle_guard(g,cpu,rd)) halt("vo3-off-guard");
    maia_ldo3_acquire_report_t a,duplicate;
    maia_ldo3_result_t result=maia_esp32p4_ldo3_acquire2500(&a);
    if (result != MAIA_LDO3_OK || a.completed != MAIA_LDO3_ACQUIRE_VERIFIED || a.adopted || !a.may_have_changed)
        halt("vo3-acquire");
    if (maia_esp32p4_ldo3_snapshot(&now) != MAIA_LDO3_OK || !vo3_same(&before,&now)) halt("vo3-on-readback");
    field("VO3_ON control=",now.control);field(" analog=",now.analog);maia_console_puts("\nVO3_CYCLE_ON_READY\n");
    vo3_wait('N');
    for (unsigned i=0;i<16U;++i) if (maia_esp32p4_ldo3_snapshot(&now) != MAIA_LDO3_OK || !vo3_same(&before,&now)) halt("vo3-on-unstable");
    int busy=maia_esp32p4_ldo3_acquire2500(&duplicate) == MAIA_LDO3_BUSY;
    int kept=vo3_cycle_guard(g,cpu,rd);n=0;
    for (unsigned slot=0;slot<6U;++slot) if (slot != 1U) {
        if (other[n++] != rd(MAIA_ESP32P4_PMU_BASE+0x1B8U+8U*slot)) kept=0;
        if (other[n++] != rd(MAIA_ESP32P4_PMU_BASE+0x1BCU+8U*slot)) kept=0;
    }
    field("VO3_ACQUIRE stage=",a.completed);field(" adopted=",a.adopted);field(" changed=",a.may_have_changed);
    field(" preserved=",(uint32_t)kept);field(" duplicate_busy=",(uint32_t)busy);maia_console_puts(" samples=00000010\n");
    if (!kept || !busy) halt("vo3-preservation");
    maia_console_puts("MAIA_NATIVE_VO3_CYCLE_PASS\n");
}
