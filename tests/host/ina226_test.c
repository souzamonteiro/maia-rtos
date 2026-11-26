#include <assert.h>
#include <string.h>

#include <maia/boards/tab5_ina226.h>
#include <maia/services/ina226.h>

static maia_i2c_bus_t bus;
static uint8_t registers[256][2];
static uint8_t reads[16];
static unsigned read_count;
static unsigned write_count;
static int fail_reg = -1;
static maia_i2c_result_t fail_status = MAIA_I2C_TIMEOUT;

maia_i2c_result_t maia_i2c_bus_read8(maia_i2c_bus_t *read_bus,
                                     uint8_t address,
                                     uint8_t reg,
                                     uint8_t *out,
                                     uint32_t bytes) {
    assert(read_bus == &bus);
    assert(address == 0x41U);
    assert(bytes == 2U);
    assert(read_count < sizeof(reads));
    reads[read_count++] = reg;
    if (reg == fail_reg) return fail_status;
    out[0] = registers[reg][0];
    out[1] = registers[reg][1];
    return MAIA_I2C_OK;
}

maia_i2c_result_t maia_i2c_bus_write8(maia_i2c_bus_t *write_bus,
                                      uint8_t address,
                                      uint8_t reg,
                                      uint8_t value) {
    (void)write_bus;
    (void)address;
    (void)reg;
    (void)value;
    ++write_count;
    return MAIA_I2C_OK;
}

static void set_register(uint8_t reg, uint16_t value) {
    registers[reg][0] = (uint8_t)(value >> 8U);
    registers[reg][1] = (uint8_t)value;
}

static void reset_reads(void) {
    memset(reads, 0, sizeof(reads));
    read_count = 0;
}

static void test_successful_read(void) {
    set_register(0xFEU, 0x5449U);
    set_register(0xFFU, 0x2260U);
    set_register(0x00U, 0x0527U);
    set_register(0x01U, 0xFF38U);
    set_register(0x02U, 10000U);
    set_register(0x03U, 250U);
    set_register(0x04U, 1000U);
    set_register(0x05U, 3413U);
    reset_reads();

    maia_ina226_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    maia_i2c_result_t bus_error = MAIA_I2C_INVALID;
    assert(maia_ina226_read(&bus, 0x41U, &sample, &bus_error) == MAIA_INA226_OK);
    assert(bus_error == MAIA_I2C_OK);
    assert(sample.manufacturer_id == 0x5449U);
    assert(sample.die_id == 0x2260U);
    assert(sample.config == 0x0527U);
    assert(sample.shunt_raw == -200);
    assert(sample.bus_raw == 10000U);
    assert(sample.power_raw == 250U);
    assert(sample.current_raw == 1000);
    assert(sample.calibration == 3413U);
    const uint8_t expected[] = {0xFEU, 0xFFU, 0x00U, 0x01U, 0x02U, 0x03U, 0x04U, 0x05U};
    assert(read_count == sizeof(expected));
    assert(memcmp(reads, expected, sizeof(expected)) == 0);
    assert(write_count == 0U);
}

static void test_bus_failure_preserves_output(void) {
    maia_ina226_sample_t sample;
    memset(&sample, 0xA5, sizeof(sample));
    maia_ina226_sample_t original = sample;
    maia_i2c_result_t bus_error = MAIA_I2C_OK;
    fail_reg = 0x02;
    fail_status = MAIA_I2C_BUSY;
    reset_reads();

    assert(maia_ina226_read(&bus, 0x41U, &sample, &bus_error) == MAIA_INA226_BUS_ERROR);
    assert(bus_error == MAIA_I2C_BUSY);
    assert(memcmp(&sample, &original, sizeof(sample)) == 0);
    assert(read_count == 5U);
    fail_reg = -1;
}

static void test_identity_mismatch_stops_before_measurements(void) {
    maia_ina226_sample_t sample;
    memset(&sample, 0xA5, sizeof(sample));
    set_register(0xFEU, 0x1234U);
    reset_reads();

    assert(maia_ina226_read(&bus, 0x41U, &sample, NULL) == MAIA_INA226_DEVICE_ID);
    assert(sample.manufacturer_id == 0x1234U);
    assert(sample.die_id == 0x2260U);
    assert(read_count == 2U);
    assert(reads[0] == 0xFEU && reads[1] == 0xFFU);
    assert(write_count == 0U);
    set_register(0xFEU, 0x5449U);
}

static void test_invalid_arguments(void) {
    maia_ina226_sample_t sample;
    maia_i2c_result_t bus_error = MAIA_I2C_OK;
    reset_reads();
    assert(maia_ina226_read(NULL, 0x41U, &sample, &bus_error) == MAIA_INA226_INVALID);
    assert(bus_error == MAIA_I2C_INVALID);
    assert(maia_ina226_read(&bus, 0x41U, NULL, NULL) == MAIA_INA226_INVALID);
    assert(read_count == 0U);
}

static void test_bus_voltage_only_read(void) {
    set_register(0xFEU, 0x5449U);
    set_register(0xFFU, 0x2260U);
    set_register(0x02U, 10000U);
    reset_reads();

    uint16_t bus_raw = 0xA55AU;
    maia_i2c_result_t bus_error = MAIA_I2C_INVALID;
    assert(maia_ina226_read_bus_voltage(&bus, 0x41U, &bus_raw, &bus_error) == MAIA_INA226_OK);
    assert(bus_error == MAIA_I2C_OK && bus_raw == 10000U);
    assert(read_count == 3U);
    assert(reads[0] == 0xFEU && reads[1] == 0xFFU && reads[2] == 0x02U);

    bus_raw = 0xA55AU;
    fail_reg = 0x02;
    assert(maia_ina226_read_bus_voltage(&bus, 0x41U, &bus_raw, &bus_error) == MAIA_INA226_BUS_ERROR);
    assert(bus_error == fail_status && bus_raw == 0xA55AU);
    fail_reg = -1;
}

static void test_tab5_shunt_derived_measurements(void) {
    maia_ina226_sample_t sample = {
        .config = 0x0527U,
        .shunt_raw = 2,
        .bus_raw = 10000U,
        .calibration = 0U
    };
    maia_tab5_ina226_measurements_t measurements = {0};
    assert(maia_tab5_ina226_derive(&sample, &measurements));
    assert(measurements.bus_millivolts == 12500U);
    assert(measurements.current_microamps == 1000);
    assert(measurements.power_microwatts == 12500);

    sample.shunt_raw = -2;
    assert(maia_tab5_ina226_derive(&sample, &measurements));
    assert(measurements.current_microamps == -1000);
    assert(measurements.power_microwatts == -12500);

    maia_tab5_ina226_measurements_t original = measurements;
    sample.config = 0x0526U;
    assert(!maia_tab5_ina226_derive(&sample, &measurements));
    assert(memcmp(&measurements, &original, sizeof(measurements)) == 0);
}

int main(void) {
    test_successful_read();
    test_bus_failure_preserves_output();
    test_identity_mismatch_stops_before_measurements();
    test_invalid_arguments();
    test_bus_voltage_only_read();
    test_tab5_shunt_derived_measurements();
    assert(write_count == 0U);
    return 0;
}
