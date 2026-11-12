/* Native C port of the configuration/bit-update model in M5Unified:
 * src/utility/PI4IOE5V6408_Class.inl and I2C_Class.inl,
 * revision44952cb8e4ac88bf04ad23b9dfd6f4b05c788348.
 * Original PI4IO component authors: Forairaaaaa, lovyan03; Copyright (c) 2024.
 * Copyright (c) 2021 M5Stack. MIT license: LICENSES/M5Stack-MIT.txt.
 * Maia additions: restricted register set, typed errors, no-op and readback.
 * No M5Unified, ESP-IDF, Arduino or FreeRTOS runtime is linked. */
#include <maia/drivers/pi4ioe5v6408.h>
static int config(uint8_t address, uint8_t reg) {
    return (address == 0x43U || address == 0x44U) &&
        (reg == MAIA_PI4IO_DIRECTION || reg == MAIA_PI4IO_OUTPUT ||
         reg == MAIA_PI4IO_HIGH_Z || reg == MAIA_PI4IO_PULL_ENABLE ||
         reg == MAIA_PI4IO_PULL_SELECT);
}
maia_i2c_result_t maia_pi4io_read_config(const maia_i2c_lines_t *bus,
                                       uint8_t address, uint8_t reg, uint8_t *out) {
    if (!config(address, reg)) return MAIA_I2C_INVALID;
    return maia_i2c_read_register8(bus, address, reg, out);
}
maia_i2c_result_t maia_pi4io_update_config(const maia_i2c_lines_t *bus,
                                         uint8_t address, uint8_t reg,
                                         uint8_t mask, uint8_t value) {
    if (!config(address, reg) || (value & (uint8_t)~mask)) return MAIA_I2C_INVALID;
    uint8_t before;
    maia_i2c_result_t r = maia_i2c_read_register8(bus, address, reg, &before);
    if (r != MAIA_I2C_OK) return r;
    uint8_t desired = (uint8_t)((before & (uint8_t)~mask) | value);
    if (desired == before) return MAIA_I2C_OK;
    r = maia_i2c_write_register8(bus, address, reg, desired);
    if (r != MAIA_I2C_OK) return r;
    uint8_t after;
    r = maia_i2c_read_register8(bus, address, reg, &after);
    if (r != MAIA_I2C_OK) return r;
    return after == desired ? MAIA_I2C_OK : MAIA_I2C_VERIFY;
}
