#include <maia/boards/tab5_ina226.h>

#define TAB5_INA226_ADDRESS 0x41U
#define TAB5_SHUNT_MICRO_OHMS 5000LL

static maia_i2c_bus_t *system_bus;

void maia_tab5_ina226_bind(maia_i2c_bus_t *bus) {
    system_bus = bus;
}

maia_ina226_result_t maia_tab5_ina226_read(maia_ina226_sample_t *sample_out,
                                           maia_i2c_result_t *bus_error) {
    if (!system_bus) {
        if (bus_error) *bus_error = MAIA_I2C_INVALID;
        return MAIA_INA226_BUS_ERROR;
    }
    return maia_ina226_read(system_bus, TAB5_INA226_ADDRESS, sample_out, bus_error);
}

bool maia_tab5_ina226_derive(const maia_ina226_sample_t *sample,
                            maia_tab5_ina226_measurements_t *measurements_out) {
    if (!sample || !measurements_out || (sample->config & 0x0007U) != 0x0007U) return false;

    maia_tab5_ina226_measurements_t measurements;
    measurements.bus_millivolts = ((uint32_t)sample->bus_raw * 5U) / 4U;
    measurements.current_microamps = (int32_t)(
        (int64_t)sample->shunt_raw * 2500000LL / TAB5_SHUNT_MICRO_OHMS);
    int64_t power_microwatts = (int64_t)measurements.current_microamps *
                               sample->bus_raw * 1250LL / 1000000LL;
    measurements.power_microwatts = (int32_t)power_microwatts;
    *measurements_out = measurements;
    return true;
}
