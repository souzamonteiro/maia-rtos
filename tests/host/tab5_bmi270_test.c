#include <assert.h>
#include <maia/boards/tab5_bmi270.h>

static maia_i2c_bus_t bus;
static maia_i2c_result_t read_result;
static uint8_t register_value;
static unsigned reads;
static unsigned writes;

maia_i2c_result_t maia_i2c_bus_read8(maia_i2c_bus_t *read_bus,
                                     uint8_t address,
                                     uint8_t reg,
                                     uint8_t *out,
                                     uint32_t count) {
    assert(read_bus == &bus);
    assert(address == 0x68U && reg == 0x00U && count == 1U);
    ++reads;
    if (read_result == MAIA_I2C_OK) *out = register_value;
    return read_result;
}

maia_i2c_result_t maia_i2c_bus_write8(maia_i2c_bus_t *write_bus,
                                      uint8_t address,
                                      uint8_t reg,
                                      uint8_t value) {
    (void)write_bus;
    (void)address;
    (void)reg;
    (void)value;
    ++writes;
    return MAIA_I2C_OK;
}

int main(void) {
    maia_tab5_bmi270_bind(&bus);
    uint8_t chip_id = 0xA5U;
    maia_i2c_result_t bus_error;

    register_value = 0x24U;
    read_result = MAIA_I2C_OK;
    assert(maia_tab5_bmi270_read_chip_id(&chip_id, &bus_error) == MAIA_TAB5_BMI270_OK);
    assert(chip_id == 0x24U && bus_error == MAIA_I2C_OK && reads == 1U);

    register_value = 0x12U;
    assert(maia_tab5_bmi270_read_chip_id(&chip_id, &bus_error) == MAIA_TAB5_BMI270_ID_MISMATCH);
    assert(chip_id == 0x12U && bus_error == MAIA_I2C_OK && reads == 2U);

    chip_id = 0xA5U;
    read_result = MAIA_I2C_BUSY;
    assert(maia_tab5_bmi270_read_chip_id(&chip_id, &bus_error) == MAIA_TAB5_BMI270_BUS_ERROR);
    assert(chip_id == 0xA5U && bus_error == MAIA_I2C_BUSY && reads == 3U);
    assert(writes == 0U);

    assert(maia_tab5_bmi270_read_chip_id(NULL, &bus_error) == MAIA_TAB5_BMI270_INVALID);
    assert(bus_error == MAIA_I2C_INVALID && reads == 3U);
    return 0;
}
