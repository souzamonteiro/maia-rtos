/* RAM-only read-only LDO3 calibration inspection: no PMU or eFuse writes. */
#include <maia/soc/esp32p4_ldo3.h>
#include <maia/console.h>
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
void maia_boot_main(void) {
    maia_console_puts("MAIA_LDO3_PROBE read-only shadows and PMU; no regulator writes\n");
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
    maia_console_puts(errors ? "MAIA_NATIVE_LDO3_FAIL\n" : "MAIA_NATIVE_LDO3_PASS\n");
    for (;;) __asm__ volatile("wfi");
}
