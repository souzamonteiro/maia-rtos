#ifndef MAIA_SOC_ESP32P4_BOOT_I2C_H
#define MAIA_SOC_ESP32P4_BOOT_I2C_H
#include <maia/drivers/i2c_bitbang.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Temporary pre-scheduler, core0-only exclusive ownership of two HP GPIOs
 * (16..56), SYSTIMER UNIT0 capture and clock selection. IRQs must already be
 * off; never call from an ISR, idle or task. No peripheral may own these pins.
 * Storage must stay at its original address until reset; do not copy/move it.
 * Prepare configures pins and enables the timer, not any board power/reset.
 * Success leaves both lines released; failures after pin setup release them.
 * No teardown, concurrent use, runtime scheduling or frequency guarantee. */
typedef struct maia_esp32p4_boot_i2c {
    maia_i2c_lines_t lines;
    uint32_t sda_pin;
    uint32_t scl_pin;
} maia_esp32p4_boot_i2c_t;
maia_i2c_result_t maia_esp32p4_boot_i2c_prepare(maia_esp32p4_boot_i2c_t *bus,
                                             uint32_t sda_pin, uint32_t scl_pin);
#ifdef __cplusplus
}
#endif
#endif
