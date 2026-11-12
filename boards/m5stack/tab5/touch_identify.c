#include <maia/boards/tab5_touch.h>
maia_i2c_result_t maia_tab5_touch_identify(const maia_i2c_lines_t *bus, uint8_t *firmware) {
    if (!firmware) return MAIA_I2C_INVALID;
    uint8_t status;
    maia_i2c_result_t r = maia_i2c_read_register16(bus, 0x55U, 0x0001U, &status);
    if (r != MAIA_I2C_OK) return r;
    if (status != 0U && status != 4U) return MAIA_I2C_BUSY;
    return maia_i2c_read_register16(bus, 0x55U, 0x0000U, firmware);
}
