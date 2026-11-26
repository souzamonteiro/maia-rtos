#include <assert.h>
#include <string.h>

#include <maia/boards/tab5_battery.h>
#include <maia/drivers/pi4ioe5v6408.h>

static maia_i2c_bus_t bus;
static uint8_t ina_registers[256][2];
static uint8_t expander_input;
static unsigned read_count;
static unsigned write_count;
static unsigned raw_io_count;
static int fail_address = -1;
static int fail_reg = -1;
static maia_i2c_result_t fail_status = MAIA_I2C_TIMEOUT;

maia_i2c_result_t maia_i2c_bus_read8(maia_i2c_bus_t *read_bus,
                                     uint8_t address,
                                     uint8_t reg,
                                     uint8_t *out,
                                     uint32_t bytes) {
    assert(read_bus == &bus);
    ++read_count;
    if ((int)address == fail_address && (int)reg == fail_reg) return fail_status;
    if (address == 0x41U) {
        assert(bytes == 2U);
        out[0] = ina_registers[reg][0];
        out[1] = ina_registers[reg][1];
        return MAIA_I2C_OK;
    }
    assert(address == 0x44U && reg == MAIA_PI4IO_INPUT_STATUS && bytes == 1U);
    out[0] = expander_input;
    return MAIA_I2C_OK;
}

maia_i2c_result_t maia_i2c_read_register8(const maia_i2c_lines_t *read_bus,
                                          uint8_t address,
                                          uint8_t reg,
                                          uint8_t *out) {
    (void)read_bus;
    (void)address;
    (void)reg;
    (void)out;
    ++raw_io_count;
    return MAIA_I2C_INVALID;
}

maia_i2c_result_t maia_i2c_write_register8(const maia_i2c_lines_t *write_bus,
                                           uint8_t address,
                                           uint8_t reg,
                                           uint8_t value) {
    (void)write_bus;
    (void)address;
    (void)reg;
    (void)value;
    ++raw_io_count;
    return MAIA_I2C_INVALID;
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

static void set_ina_register(uint8_t reg, uint16_t value) {
    ina_registers[reg][0] = (uint8_t)(value >> 8U);
    ina_registers[reg][1] = (uint8_t)value;
}

static void prepare_device(void) {
    memset(ina_registers, 0, sizeof(ina_registers));
    set_ina_register(0xFEU, 0x5449U);
    set_ina_register(0xFFU, 0x2260U);
    set_ina_register(0x00U, 0x0527U);
    set_ina_register(0x02U, 10000U);
    read_count = 0;
    write_count = 0;
    raw_io_count = 0;
    fail_address = -1;
    fail_reg = -1;
    expander_input = 0x40U;
    maia_tab5_battery_bind(&bus);
}

static void test_successful_read(void) {
    prepare_device();
    maia_tab5_battery_sample_t sample = {0};
    maia_i2c_result_t bus_error = MAIA_I2C_INVALID;
    assert(maia_tab5_battery_read(&sample, &bus_error) == MAIA_TAB5_BATTERY_OK);
    assert(bus_error == MAIA_I2C_OK);
    assert(sample.monitor_bus_millivolts == 12500U);
    assert(sample.charge_signal_high);
    assert(read_count == 4U);
    assert(write_count == 0U);
}

static void test_expander_busy_preserves_output(void) {
    prepare_device();
    fail_address = 0x44;
    fail_reg = MAIA_PI4IO_INPUT_STATUS;
    fail_status = MAIA_I2C_BUSY;
    maia_tab5_battery_sample_t sample;
    memset(&sample, 0xA5, sizeof(sample));
    maia_tab5_battery_sample_t original = sample;
    maia_i2c_result_t bus_error = MAIA_I2C_OK;
    assert(maia_tab5_battery_read(&sample, &bus_error) == MAIA_TAB5_BATTERY_BUS_ERROR);
    assert(bus_error == MAIA_I2C_BUSY);
    assert(memcmp(&sample, &original, sizeof(sample)) == 0);
    assert(read_count == 4U);
    assert(write_count == 0U);
}

static void test_wrong_monitor_id_stops_before_expander(void) {
    prepare_device();
    set_ina_register(0xFEU, 0x1234U);
    maia_tab5_battery_sample_t sample;
    memset(&sample, 0xA5, sizeof(sample));
    maia_tab5_battery_sample_t original = sample;
    assert(maia_tab5_battery_read(&sample, NULL) == MAIA_TAB5_BATTERY_DEVICE_ID);
    assert(memcmp(&sample, &original, sizeof(sample)) == 0);
    assert(read_count == 2U);
    assert(write_count == 0U);
}

static void test_invalid_state(void) {
    prepare_device();
    maia_tab5_battery_sample_t sample;
    maia_i2c_result_t bus_error = MAIA_I2C_OK;
    maia_tab5_battery_bind(NULL);
    assert(maia_tab5_battery_read(&sample, &bus_error) == MAIA_TAB5_BATTERY_INVALID);
    assert(bus_error == MAIA_I2C_INVALID);
    assert(read_count == 0U);
}

int main(void) {
    test_successful_read();
    test_expander_busy_preserves_output();
    test_wrong_monitor_id_stops_before_expander();
    test_invalid_state();
    assert(raw_io_count == 0U);
    return 0;
}
