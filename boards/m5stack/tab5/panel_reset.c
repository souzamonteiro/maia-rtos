/* SPDX-FileCopyrightText: 2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * License: LICENSES/Apache-2.0.txt. Modified for MaiaRTOS checked native I2C,
 * masked updates, bounded boot delays and explicit partial-failure reporting.
 * Native port of Tab5 reset behavior from the working bf044d3 BSP
 * bsp_reset_tp(), cross-checked with M5Unified Power_Class.inl reset release.
 * M5Stack configuration model MIT: LICENSES/M5Stack-MIT.txt.
 * Masks/delays/provenance: docs/audits/native-panel-reset.md. */
#include <maia/boards/tab5_panel_reset.h>
#define LCD 0x10U
#define TOUCH 0x20U
#define BOTH (LCD | TOUCH)
static const uint8_t regs[] = {MAIA_PI4IO_DIRECTION,MAIA_PI4IO_OUTPUT,
    MAIA_PI4IO_HIGH_Z,MAIA_PI4IO_PULL_ENABLE,MAIA_PI4IO_PULL_SELECT};
static const uint8_t masks[] = {BOTH,BOTH,BOTH,LCD,LCD};
static const uint8_t final_bits[] = {TOUCH,TOUCH,0U,LCD,LCD};
static maia_i2c_result_t update(const maia_i2c_lines_t *bus,uint8_t reg,uint8_t mask,uint8_t value) {
    return maia_pi4io_update_config(bus,0x43U,reg,mask,value);
}
static maia_i2c_result_t wait100(const maia_i2c_lines_t *bus) {
    for (unsigned i=0U;i<20000U;++i) {
        maia_i2c_result_t r=bus->pause(bus->context);
        if (r != MAIA_I2C_OK) return r;
    }
    return MAIA_I2C_OK;
}
maia_i2c_result_t maia_tab5_panel_reset(const maia_i2c_lines_t *bus,
                                      maia_tab5_reset_report_t *report) {
    if (!report) return MAIA_I2C_INVALID;
    report->completed=MAIA_TAB5_RESET_NONE;report->may_have_changed=0U;
    if (!bus || !bus->drive || !bus->sample || !bus->pause) return MAIA_I2C_INVALID;
    uint8_t before[5];
    for (unsigned i=0U;i<5U;++i) {
        maia_i2c_result_t r=maia_pi4io_read_config(bus,0x43U,regs[i],&before[i]);
        if (r != MAIA_I2C_OK) return r;
    }
    if ((before[0] & LCD) && !(before[2] & LCD) && (before[1] & LCD)) return MAIA_I2C_BUSY;
    report->completed=MAIA_TAB5_RESET_CHECKED;
    report->may_have_changed=1U;
    maia_i2c_result_t r=update(bus,MAIA_PI4IO_PULL_SELECT,LCD,LCD);
    if (r != MAIA_I2C_OK) return r;
    r=update(bus,MAIA_PI4IO_PULL_ENABLE,LCD,LCD);
    if (r != MAIA_I2C_OK) return r;
    report->completed=MAIA_TAB5_RESET_PULLUP;
    r=update(bus,MAIA_PI4IO_OUTPUT,BOTH,0U);
    if (r != MAIA_I2C_OK) return r;
    report->completed=MAIA_TAB5_RESET_LATCH_LOW;
    r=update(bus,MAIA_PI4IO_HIGH_Z,BOTH,0U);
    if (r != MAIA_I2C_OK) return r;
    r=update(bus,MAIA_PI4IO_DIRECTION,BOTH,BOTH);
    if (r != MAIA_I2C_OK) return r;
    report->completed=MAIA_TAB5_RESET_DRIVE_ENABLED;
    r=wait100(bus);
    if (r != MAIA_I2C_OK) return r;
    report->completed=MAIA_TAB5_RESET_HELD_LOW;
    r=update(bus,MAIA_PI4IO_OUTPUT,BOTH,TOUCH);
    if (r != MAIA_I2C_OK) return r;
    report->completed=MAIA_TAB5_RESET_TOUCH_RELEASED;
    r=update(bus,MAIA_PI4IO_DIRECTION,LCD,0U);
    if (r != MAIA_I2C_OK) return r;
    report->completed=MAIA_TAB5_RESET_LCD_RELEASED;
    r=wait100(bus);
    if (r != MAIA_I2C_OK) return r;
    report->completed=MAIA_TAB5_RESET_SETTLED;
    for (unsigned i=0U;i<5U;++i) {
        uint8_t after;
        r=maia_pi4io_read_config(bus,0x43U,regs[i],&after);
        if (r != MAIA_I2C_OK) return r;
        uint8_t expected=(uint8_t)((before[i] & (uint8_t)~masks[i]) | final_bits[i]);
        if (after != expected) return MAIA_I2C_VERIFY;
    }
    report->completed=MAIA_TAB5_RESET_VERIFIED;
    return MAIA_I2C_OK;
}
