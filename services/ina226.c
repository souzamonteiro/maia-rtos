#include <maia/services/ina226.h>

#define INA226_REG_CONFIG 0x00U
#define INA226_REG_SHUNT 0x01U
#define INA226_REG_BUS 0x02U
#define INA226_REG_POWER 0x03U
#define INA226_REG_CURRENT 0x04U
#define INA226_REG_CALIBRATION 0x05U
#define INA226_REG_MANUFACTURER_ID 0xFEU
#define INA226_REG_DIE_ID 0xFFU

#define INA226_MANUFACTURER_ID 0x5449U
#define INA226_DIE_ID 0x2260U

static maia_i2c_result_t read_word(maia_i2c_bus_t *bus,
                                   uint8_t address,
                                   uint8_t reg,
                                   uint16_t *value) {
    uint8_t bytes[2];
    maia_i2c_result_t result = maia_i2c_bus_read8(bus, address, reg, bytes, sizeof(bytes));
    if (result != MAIA_I2C_OK) return result;
    *value = (uint16_t)(((uint16_t)bytes[0] << 8U) | bytes[1]);
    return MAIA_I2C_OK;
}

static int16_t signed_word(uint16_t value) {
    int32_t signed_value = (value & 0x8000U) ? (int32_t)value - 0x10000 : (int32_t)value;
    return (int16_t)signed_value;
}

maia_ina226_result_t maia_ina226_read(maia_i2c_bus_t *bus,
                                      uint8_t address,
                                      maia_ina226_sample_t *sample_out,
                                      maia_i2c_result_t *bus_error) {
    if (bus_error) *bus_error = MAIA_I2C_INVALID;
    if (!bus || !sample_out) return MAIA_INA226_INVALID;

    maia_ina226_sample_t sample = {0};
    uint16_t value;
    maia_i2c_result_t result = read_word(bus, address, INA226_REG_MANUFACTURER_ID,
                                         &sample.manufacturer_id);
    if (result == MAIA_I2C_OK) {
        result = read_word(bus, address, INA226_REG_DIE_ID, &sample.die_id);
    }
    if (result != MAIA_I2C_OK) {
        if (bus_error) *bus_error = result;
        return MAIA_INA226_BUS_ERROR;
    }

    if (sample.manufacturer_id != INA226_MANUFACTURER_ID || sample.die_id != INA226_DIE_ID) {
        *sample_out = sample;
        if (bus_error) *bus_error = MAIA_I2C_OK;
        return MAIA_INA226_DEVICE_ID;
    }

    result = read_word(bus, address, INA226_REG_CONFIG, &sample.config);
    if (result == MAIA_I2C_OK) result = read_word(bus, address, INA226_REG_SHUNT, &value);
    if (result == MAIA_I2C_OK) sample.shunt_raw = signed_word(value);
    if (result == MAIA_I2C_OK) result = read_word(bus, address, INA226_REG_BUS, &sample.bus_raw);
    if (result == MAIA_I2C_OK) result = read_word(bus, address, INA226_REG_POWER, &sample.power_raw);
    if (result == MAIA_I2C_OK) result = read_word(bus, address, INA226_REG_CURRENT, &value);
    if (result == MAIA_I2C_OK) sample.current_raw = signed_word(value);
    if (result == MAIA_I2C_OK) result = read_word(bus, address, INA226_REG_CALIBRATION,
                                                  &sample.calibration);
    if (result != MAIA_I2C_OK) {
        if (bus_error) *bus_error = result;
        return MAIA_INA226_BUS_ERROR;
    }

    /* Reading Mask/Enable (06h) can clear conversion-ready and alert flags. */
    *sample_out = sample;
    if (bus_error) *bus_error = MAIA_I2C_OK;
    return MAIA_INA226_OK;
}

maia_ina226_result_t maia_ina226_read_bus_voltage(maia_i2c_bus_t *bus,
                                                 uint8_t address,
                                                 uint16_t *bus_raw_out,
                                                 maia_i2c_result_t *bus_error) {
    if (bus_error) *bus_error = MAIA_I2C_INVALID;
    if (!bus || !bus_raw_out) return MAIA_INA226_INVALID;

    uint16_t manufacturer_id;
    uint16_t die_id;
    maia_i2c_result_t result = read_word(bus, address, INA226_REG_MANUFACTURER_ID,
                                         &manufacturer_id);
    if (result == MAIA_I2C_OK) result = read_word(bus, address, INA226_REG_DIE_ID, &die_id);
    if (result != MAIA_I2C_OK) {
        if (bus_error) *bus_error = result;
        return MAIA_INA226_BUS_ERROR;
    }
    if (manufacturer_id != INA226_MANUFACTURER_ID || die_id != INA226_DIE_ID) {
        if (bus_error) *bus_error = MAIA_I2C_OK;
        return MAIA_INA226_DEVICE_ID;
    }

    uint16_t bus_raw;
    result = read_word(bus, address, INA226_REG_BUS, &bus_raw);
    if (result != MAIA_I2C_OK) {
        if (bus_error) *bus_error = result;
        return MAIA_INA226_BUS_ERROR;
    }
    *bus_raw_out = bus_raw;
    if (bus_error) *bus_error = MAIA_I2C_OK;
    return MAIA_INA226_OK;
}
