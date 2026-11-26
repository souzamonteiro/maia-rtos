#include <maia/boards/tab5_bmi270.h>

#define TAB5_BMI270_ADDRESS 0x68U
#define BMI270_REG_CHIP_ID 0x00U
#define BMI270_CHIP_ID 0x24U

static maia_i2c_bus_t *system_bus;

void maia_tab5_bmi270_bind(maia_i2c_bus_t *bus) {
    system_bus = bus;
}

maia_tab5_bmi270_result_t maia_tab5_bmi270_read_chip_id(
    uint8_t *chip_id_out, maia_i2c_result_t *bus_error) {
    if (bus_error) *bus_error = MAIA_I2C_INVALID;
    if (!system_bus || !chip_id_out) return MAIA_TAB5_BMI270_INVALID;

    uint8_t chip_id;
    maia_i2c_result_t result = maia_i2c_bus_read8(
        system_bus, TAB5_BMI270_ADDRESS, BMI270_REG_CHIP_ID, &chip_id, 1U);
    if (result != MAIA_I2C_OK) {
        if (bus_error) *bus_error = result;
        return MAIA_TAB5_BMI270_BUS_ERROR;
    }

    *chip_id_out = chip_id;
    if (bus_error) *bus_error = MAIA_I2C_OK;
    return chip_id == BMI270_CHIP_ID ? MAIA_TAB5_BMI270_OK : MAIA_TAB5_BMI270_ID_MISMATCH;
}
