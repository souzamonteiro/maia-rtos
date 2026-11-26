#include <maia/boards/tab5_battery.h>
#include <maia/drivers/pi4ioe5v6408.h>
#include <maia/services/ina226.h>

#define TAB5_INA226_ADDRESS 0x41U
#define TAB5_EXPANDER2_ADDRESS 0x44U
#define TAB5_CHARGE_SIGNAL_PIN 6U

static maia_i2c_bus_t *system_bus;

void maia_tab5_battery_bind(maia_i2c_bus_t *bus) {
    system_bus = bus;
}

maia_tab5_battery_result_t maia_tab5_battery_read(maia_tab5_battery_sample_t *sample_out,
                                                  maia_i2c_result_t *bus_error) {
    if (bus_error) *bus_error = MAIA_I2C_INVALID;
    if (!system_bus || !sample_out) return MAIA_TAB5_BATTERY_INVALID;

    uint16_t bus_raw;
    maia_i2c_result_t result;
    maia_ina226_result_t monitor_result = maia_ina226_read_bus_voltage(
        system_bus, TAB5_INA226_ADDRESS, &bus_raw, &result);
    if (monitor_result == MAIA_INA226_BUS_ERROR) {
        if (bus_error) *bus_error = result;
        return MAIA_TAB5_BATTERY_BUS_ERROR;
    }
    if (monitor_result == MAIA_INA226_DEVICE_ID) {
        if (bus_error) *bus_error = MAIA_I2C_OK;
        return MAIA_TAB5_BATTERY_DEVICE_ID;
    }
    if (monitor_result != MAIA_INA226_OK) return MAIA_TAB5_BATTERY_INVALID;

    uint8_t input_status;
    result = maia_pi4io_read_input_status(system_bus, TAB5_EXPANDER2_ADDRESS, &input_status);
    if (result != MAIA_I2C_OK) {
        if (bus_error) *bus_error = result;
        return MAIA_TAB5_BATTERY_BUS_ERROR;
    }

    maia_tab5_battery_sample_t sample;
    sample.monitor_bus_millivolts = ((uint32_t)bus_raw * 5U) / 4U;
    sample.charge_signal_high = (input_status & (1U << TAB5_CHARGE_SIGNAL_PIN)) != 0U;
    *sample_out = sample;
    if (bus_error) *bus_error = MAIA_I2C_OK;
    return MAIA_TAB5_BATTERY_OK;
}
