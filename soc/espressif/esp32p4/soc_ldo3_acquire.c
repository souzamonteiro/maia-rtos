/* SPDX-FileCopyrightText: 2023-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Native fixed-VO3 adaptation of ESP-IDF f5c3654 esp_ldo_acquire_channel/ldo_ll:
 * boot-only lifetime, no reference counts/adjustable voltage/FreeRTOS locks;
 * fail-closed adoption, preserved fields and checked writes. License: LICENSES/Apache-2.0.txt. */
#include <maia/soc/esp32p4_ldo3.h>
#include "ldo3_internal.h"
enum { UNCLAIMED,OWNED,FAULTED };
static unsigned state;
#ifdef MAIA_LDO3_TEST
void maia_ldo3_test_reset(void);
void maia_ldo3_test_reset(void) { state=UNCLAIMED; }
#endif
static int same(const maia_ldo3_snapshot_t *a,const maia_ldo3_snapshot_t *b) {
    return a->mac_sys2 == b->mac_sys2 && a->mac_sys3 == b->mac_sys3 && a->rs_error0 == b->rs_error0;
}
static maia_ldo3_result_t checked_write(uintptr_t address,uint32_t before,uint32_t desired,
                                      maia_ldo3_acquire_report_t *report) {
    if (maia_ldo3_read(address) != before) return MAIA_LDO3_VERIFY;
    if (before != desired) {
        report->may_have_changed=1U;
        maia_ldo3_write(address,desired);
    }
    uint32_t observed=maia_ldo3_read(address);
    if (address == MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG) report->control=observed;
    else report->analog=observed;
    return observed == desired ? MAIA_LDO3_OK : MAIA_LDO3_VERIFY;
}
maia_ldo3_result_t maia_esp32p4_ldo3_acquire2500(maia_ldo3_acquire_report_t *report) {
    if (!report) return MAIA_LDO3_INVALID;
    maia_ldo3_acquire_report_t empty={0};*report=empty;
    if (!maia_ldo3_boot_context()) return MAIA_LDO3_CONTEXT;
    if (state == OWNED) return MAIA_LDO3_BUSY;
    if (state == FAULTED) return MAIA_LDO3_FAILED;
    maia_ldo3_snapshot_t before,after;maia_ldo3_plan_t plan;
    maia_ldo3_result_t r=maia_esp32p4_ldo3_snapshot(&before);
    if (r != MAIA_LDO3_OK) return r;
    report->control=before.control;report->analog=before.analog;
    r=maia_esp32p4_ldo3_plan2500(&before,&plan);
    if (r != MAIA_LDO3_OK) return r;
    report->dref=plan.dref;report->mul=plan.mul;report->completed=MAIA_LDO3_ACQUIRE_PLANNED;
    uint32_t analog=(before.analog&~MAIA_ESP32P4_LDO3_ANALOG_MASK) |
        ((uint32_t)plan.dref<<28U) | ((uint32_t)plan.mul<<23U) | (1UL<<26U);
    uint32_t routed=(before.control&~MAIA_ESP32P4_LDO3_CONTROL_MASK) | MAIA_ESP32P4_LDO3_SW_SELECT;
    uint32_t enabled=routed | MAIA_ESP32P4_LDO3_XPD;
    int active=(before.control&MAIA_ESP32P4_LDO3_XPD) != 0U;
    if (active && (before.control != enabled || before.analog != analog)) return MAIA_LDO3_BUSY;
    r=maia_esp32p4_ldo3_snapshot(&after);
    if (r != MAIA_LDO3_OK) return r;
    if (!same(&before,&after) || after.control != before.control || after.analog != before.analog) return MAIA_LDO3_VERIFY;
    if (active) {
        report->adopted=1U;
    } else {
        if ((maia_ldo3_read(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG)&MAIA_ESP32P4_HP_MIPI_SYS_CLK_MASK) ||
            (maia_ldo3_read(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG)&MAIA_ESP32P4_HP_MIPI_PHY_CLK_MASK)) return MAIA_LDO3_BUSY;
        state=FAULTED; /* Programming phase: fail closed until reset on any error. */
        r=checked_write(MAIA_ESP32P4_PMU_EXT_LDO_VO3_ANA_REG,before.analog,analog,report);
        if (r != MAIA_LDO3_OK) return r;
        report->completed=MAIA_LDO3_ACQUIRE_ANALOG;
        r=checked_write(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG,before.control,routed,report);
        if (r != MAIA_LDO3_OK) return r;
        report->completed=MAIA_LDO3_ACQUIRE_ROUTED;
        r=checked_write(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG,routed,enabled,report);
        if (r != MAIA_LDO3_OK) return r;
        report->completed=MAIA_LDO3_ACQUIRE_ENABLED;
        r=maia_esp32p4_ldo3_snapshot(&after);
        if (r != MAIA_LDO3_OK) return r;
        if (!same(&before,&after) || after.control != enabled || after.analog != analog) return MAIA_LDO3_VERIFY;
    }
    report->control=after.control;report->analog=after.analog;
    report->completed=MAIA_LDO3_ACQUIRE_VERIFIED;state=OWNED;
    return MAIA_LDO3_OK;
}
