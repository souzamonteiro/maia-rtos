/* Power-off sequence ported from the M5Stack Tab5 BSP
 * bsp_generate_poweroff_signal() (m5stack_tab5.c), MIT, Copyright (c) M5Stack.
 * Uses only the native MaiaRTOS shared System I2C bus. */
#include <maia/boards/tab5_power.h>
#include <maia/drivers/pi4ioe5v6408.h>

#define TAB5_EXPANDER2_ADDRESS 0x44U
#define TAB5_POWEROFF_BIT (1U << 4U)
#define TAB5_POWEROFF_PULSES 3U
#define TAB5_POWEROFF_PULSE_MS 100U
#define TAB5_BUSY_ATTEMPTS 20U

static maia_i2c_bus_t *system_bus;

void maia_tab5_power_bind(maia_i2c_bus_t *bus) {
    system_bus = bus;
}

static maia_i2c_result_t read_output(maia_tab5_power_delay_fn delay_ms, uint8_t *value) {
    maia_i2c_result_t r = MAIA_I2C_BUSY;
    for (unsigned attempt = 0; attempt < TAB5_BUSY_ATTEMPTS && r == MAIA_I2C_BUSY; ++attempt) {
        if (attempt) delay_ms(1U);
        r = maia_i2c_bus_read8(system_bus, TAB5_EXPANDER2_ADDRESS, MAIA_PI4IO_OUTPUT, value, 1U);
    }
    return r;
}

static maia_i2c_result_t write_output(maia_tab5_power_delay_fn delay_ms, uint8_t value) {
    maia_i2c_result_t r = MAIA_I2C_BUSY;
    for (unsigned attempt = 0; attempt < TAB5_BUSY_ATTEMPTS && r == MAIA_I2C_BUSY; ++attempt) {
        if (attempt) delay_ms(1U);
        r = maia_i2c_bus_write8(system_bus, TAB5_EXPANDER2_ADDRESS, MAIA_PI4IO_OUTPUT, value);
    }
    return r;
}

maia_tab5_power_result_t maia_tab5_power_off(maia_tab5_power_delay_fn delay_ms,
                                             maia_i2c_result_t *bus_error) {
    if (bus_error) *bus_error = MAIA_I2C_INVALID;
    if (!system_bus || !delay_ms) return MAIA_TAB5_POWER_INVALID;

    uint8_t output;
    maia_i2c_result_t r = read_output(delay_ms, &output);
    if (r != MAIA_I2C_OK) {
        if (bus_error) *bus_error = r;
        return MAIA_TAB5_POWER_BUS_ERROR;
    }
    for (unsigned pulse = 0; pulse < TAB5_POWEROFF_PULSES; ++pulse) {
        r = write_output(delay_ms, (uint8_t)(output | TAB5_POWEROFF_BIT));
        if (r != MAIA_I2C_OK) break;
        delay_ms(TAB5_POWEROFF_PULSE_MS);
        r = write_output(delay_ms, (uint8_t)(output & (uint8_t)~TAB5_POWEROFF_BIT));
        if (r != MAIA_I2C_OK) break;
        delay_ms(TAB5_POWEROFF_PULSE_MS);
    }
    if (r != MAIA_I2C_OK) {
        /* Never leave the power-off line asserted after a partial sequence. */
        (void)write_output(delay_ms, (uint8_t)(output & (uint8_t)~TAB5_POWEROFF_BIT));
        if (bus_error) *bus_error = r;
        return MAIA_TAB5_POWER_BUS_ERROR;
    }
    if (bus_error) *bus_error = MAIA_I2C_OK;
    return MAIA_TAB5_POWER_OK;
}
