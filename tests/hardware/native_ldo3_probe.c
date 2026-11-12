/* RAM LDO3 diagnostics: read-only baseline, opt-in adoption/enable variants. */
#include <maia/soc/esp32p4_ldo3.h>
#include <maia/console.h>
#ifdef MAIA_NATIVE_PROBE_LDO3_ACQUIRE
#include <maia/soc/esp32p4_regs.h>
#endif
static unsigned errors;
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }
static void number(uint32_t n) {
    char d[10];unsigned count=0U;
    do { d[count++]=(char)('0'+n%10U);n/=10U; } while(n);
    while(count) maia_console_putc(d[--count]);
}
static void signed_number(int32_t n) { if (n<0) { maia_console_putc('-');n=-n; }number((uint32_t)n); }
static void field(const char *name,uint32_t n) { maia_console_puts(name);number(n); }
static int same(const maia_ldo3_snapshot_t *a,const maia_ldo3_snapshot_t *b) {
    return a->mac_sys2 == b->mac_sys2 && a->mac_sys3 == b->mac_sys3 && a->rs_error0 == b->rs_error0 &&
        a->control == b->control && a->analog == b->analog;
}
#ifdef MAIA_NATIVE_PROBE_LDO3_ACQUIRE
static int acquire(const maia_ldo3_snapshot_t *before,const maia_ldo3_plan_t *plan) {
    uint32_t sys=MAIA_MMIO32(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG),phy=MAIA_MMIO32(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG);
    field("LDO3_CLOCKS sys_active=",!!(sys&MAIA_ESP32P4_HP_MIPI_SYS_CLK_MASK));
    field(" phy_active=",!!(phy&MAIA_ESP32P4_HP_MIPI_PHY_CLK_MASK));maia_console_puts("\n");
#ifdef MAIA_NATIVE_PROBE_LDO3_CYCLE
    if ((sys&MAIA_ESP32P4_HP_MIPI_SYS_CLK_MASK) || (phy&MAIA_ESP32P4_HP_MIPI_PHY_CLK_MASK)) {
        field("LDO3_GUARD sys=",sys);field(" phy=",phy);maia_console_puts("\n");return 0;
    }
#endif
    uint32_t analog=((uint32_t)plan->dref<<28U)|((uint32_t)plan->mul<<23U)|(1UL<<26U);
    if ((before->control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (before->analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) != analog) return 0;
    uint32_t others[10];unsigned n=0U;
    for (unsigned slot=0U;slot<6U;++slot) if (slot != 1U) {
        others[n++]=MAIA_MMIO32(MAIA_ESP32P4_PMU_BASE+0x1B8U+8U*slot);
        others[n++]=MAIA_MMIO32(MAIA_ESP32P4_PMU_BASE+0x1BCU+8U*slot);
    }
#ifdef MAIA_NATIVE_PROBE_LDO3_CYCLE
    /* Diagnostic only, never part of acquire: request VO3 off with all DSI
     * functional clocks verified gated. No other fields or calibration change. */
    uint32_t disabled=before->control&~MAIA_ESP32P4_LDO3_XPD;
    MAIA_MMIO32(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG)=disabled;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    if (MAIA_MMIO32(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG) != disabled) return 0;
    maia_console_puts("LDO3_CYCLE off_confirmed=1 clocks_gated=1\n");
#endif
    maia_ldo3_acquire_report_t report;
    maia_ldo3_result_t r=maia_esp32p4_ldo3_acquire2500(&report);
    if (r != MAIA_LDO3_OK) {
        field("LDO3_ACQUIRE error_code=",(uint32_t)(-r));field(" stage=",(uint32_t)report.completed);
        maia_console_puts("\n");return 0;
    }
    maia_ldo3_acquire_report_t duplicate;
    unsigned busy=maia_esp32p4_ldo3_acquire2500(&duplicate) == MAIA_LDO3_BUSY;
    maia_ldo3_snapshot_t after;
    unsigned preserved=maia_esp32p4_ldo3_snapshot(&after) == MAIA_LDO3_OK && same(before,&after) &&
        MAIA_MMIO32(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG) == sys && MAIA_MMIO32(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG) == phy;
    n=0U;
    for (unsigned slot=0U;slot<6U;++slot) if (slot != 1U) {
        if (others[n++] != MAIA_MMIO32(MAIA_ESP32P4_PMU_BASE+0x1B8U+8U*slot)) preserved=0U;
        if (others[n++] != MAIA_MMIO32(MAIA_ESP32P4_PMU_BASE+0x1BCU+8U*slot)) preserved=0U;
    }
    field("LDO3_ACQUIRE stage=",(uint32_t)report.completed);field(" adopted=",report.adopted);
    field(" changed=",report.may_have_changed);field(" preserved=",preserved);field(" duplicate_busy=",busy);
    maia_console_puts("\n");
#ifdef MAIA_NATIVE_PROBE_LDO3_CYCLE
    return report.completed == MAIA_LDO3_ACQUIRE_VERIFIED && !report.adopted && report.may_have_changed && preserved && busy;
#else
    return report.completed == MAIA_LDO3_ACQUIRE_VERIFIED && report.adopted && !report.may_have_changed && preserved && busy;
#endif
}
#endif
void maia_boot_main(void) {
#ifdef MAIA_NATIVE_PROBE_LDO3_ACQUIRE
    maia_console_puts("MAIA_LDO3_ACQUIRE_PROBE boot-only, guarded programming\n");
#else
    maia_console_puts("MAIA_LDO3_PROBE read-only shadows and PMU; no regulator writes\n");
#endif
    maia_ldo3_snapshot_t before,after;maia_ldo3_plan_t p;
    if (maia_esp32p4_ldo3_snapshot(&before) != MAIA_LDO3_OK) {
        maia_console_puts("MAIA_NATIVE_LDO3_FAIL snapshot\n");for (;;) __asm__ volatile("wfi");
    }
    field("LDO3_VERSION chip_major=",(before.mac_sys2>>4U)&3U);field(" chip_minor=",before.mac_sys2&15U);
    field(" block_major=",(before.mac_sys2>>11U)&3U);field(" block_minor=",(before.mac_sys2>>8U)&7U);
    field(" disabled=",(before.mac_sys2>>6U)&3U);field(" rs_fail=",(before.rs_error0>>3U)&1U);
    field(" corrected=",before.rs_error0&7U);maia_console_puts("\n");
    field("LDO3_TRIM k=",(before.mac_sys3>>6U)&255U);field(" vos=",(before.mac_sys3>>14U)&63U);
    field(" c=",(before.mac_sys3>>20U)&63U);maia_console_puts("\n");
    maia_ldo3_result_t r=maia_esp32p4_ldo3_plan2500(&before,&p);
    if (r != MAIA_LDO3_OK) {
        field("LDO3_PLAN error_code=",(uint32_t)(-r));maia_console_puts("\nMAIA_NATIVE_LDO3_FAIL plan\n");
        for (;;) __asm__ volatile("wfi");
    }
    for (unsigned i=0U;i<16U;++i) {
        maia_ldo3_plan_t next;
        if (maia_esp32p4_ldo3_snapshot(&after) != MAIA_LDO3_OK || !same(&before,&after) ||
            maia_esp32p4_ldo3_plan2500(&after,&next) != MAIA_LDO3_OK || next.dref != p.dref || next.mul != p.mul) ++errors;
    }
    field("LDO3_PLAN target_mv=2500 dref=",p.dref);field(" mul=",p.mul);field(" default_mask=",p.default_mask);
    field(" estimated_uv=",p.estimated_uv);field(" error_uv=",p.error_uv);maia_console_puts("\n");
    maia_console_puts("LDO3_COEFF k1000=");signed_number(p.k1000);maia_console_puts(" vos1000=");signed_number(p.vos1000);
    maia_console_puts(" c1000=");signed_number(p.c1000);maia_console_puts("\n");
    field("LDO3_CURRENT enabled=",(before.control>>8U)&1U);field(" sw_owner=",(before.control>>7U)&1U);
    field(" bypass=",(before.control>>14U)&1U);field(" source=",(before.control>>9U)&7U);
    field(" dref=",before.analog>>28U);field(" mul=",(before.analog>>23U)&7U);
    field(" vdet=",(before.analog>>26U)&1U);maia_console_puts("\n");
    field("LDO3_READONLY samples=16 unchanged=",!errors);field(" errors=",errors);maia_console_puts("\n");
#ifdef MAIA_NATIVE_PROBE_LDO3_ACQUIRE
    if (!errors && !acquire(&before,&p)) ++errors;
#endif
    maia_console_puts(errors ? "MAIA_NATIVE_LDO3_FAIL\n" : "MAIA_NATIVE_LDO3_PASS\n");
    for (;;) __asm__ volatile("wfi");
}
