#include <assert.h>
#include <string.h>

#include <maia/boards/tab5_power.h>
#include <maia/drivers/pi4ioe5v6408.h>

static maia_i2c_bus_t bus;
static uint8_t output_reg;
static uint8_t writes[16];
static unsigned write_count;
static unsigned delays[64];
static unsigned delay_count;
static unsigned read_busy;
static unsigned write_busy;
static int fail_write_index = -1;

maia_i2c_result_t maia_i2c_bus_read8(maia_i2c_bus_t *b, uint8_t address, uint8_t reg,
                                     uint8_t *out, uint32_t bytes) {
    assert(b == &bus && address == 0x44U && reg == MAIA_PI4IO_OUTPUT && bytes == 1U);
    if (read_busy) {
        --read_busy;
        return MAIA_I2C_BUSY;
    }
    *out = output_reg;
    return MAIA_I2C_OK;
}

maia_i2c_result_t maia_i2c_bus_write8(maia_i2c_bus_t *b, uint8_t address, uint8_t reg,
                                      uint8_t value) {
    assert(b == &bus && address == 0x44U && reg == MAIA_PI4IO_OUTPUT);
    if (write_busy) {
        --write_busy;
        return MAIA_I2C_BUSY;
    }
    if ((int)write_count == fail_write_index) {
        fail_write_index = -1;
        return MAIA_I2C_NACK;
    }
    assert(write_count < sizeof(writes));
    writes[write_count++] = value;
    output_reg = value;
    return MAIA_I2C_OK;
}

static void delay_ms(uint32_t ms) {
    assert(delay_count < sizeof(delays) / sizeof(delays[0]));
    delays[delay_count++] = ms;
}

static void reset(uint8_t initial) {
    output_reg = initial;
    memset(writes, 0, sizeof(writes));
    write_count = 0;
    delay_count = 0;
    read_busy = 0;
    write_busy = 0;
    fail_write_index = -1;
}

int main(void) {
    maia_i2c_result_t err;

    maia_tab5_power_bind(NULL);
    assert(maia_tab5_power_off(delay_ms, &err) == MAIA_TAB5_POWER_INVALID);
    maia_tab5_power_bind(&bus);
    assert(maia_tab5_power_off(NULL, &err) == MAIA_TAB5_POWER_INVALID);

    reset(0x09U);
    assert(maia_tab5_power_off(delay_ms, &err) == MAIA_TAB5_POWER_OK && err == MAIA_I2C_OK);
    assert(write_count == 6U && delay_count == 6U);
    for (unsigned i = 0; i < 6U; ++i) {
        assert(writes[i] == ((i % 2U) ? 0x09U : 0x19U));
        assert(delays[i] == 100U);
    }

    reset(0x19U);
    read_busy = 2U;
    write_busy = 1U;
    assert(maia_tab5_power_off(delay_ms, &err) == MAIA_TAB5_POWER_OK);
    assert(write_count == 6U && writes[5] == 0x09U && output_reg == 0x09U);
    assert(delays[0] == 1U && delays[1] == 1U && delays[2] == 1U && delays[3] == 100U);

    reset(0x09U);
    read_busy = 100U;
    assert(maia_tab5_power_off(delay_ms, &err) == MAIA_TAB5_POWER_BUS_ERROR && err == MAIA_I2C_BUSY);
    assert(write_count == 0U);

    reset(0x09U);
    fail_write_index = 1;
    assert(maia_tab5_power_off(delay_ms, &err) == MAIA_TAB5_POWER_BUS_ERROR && err == MAIA_I2C_NACK);
    assert(write_count == 2U && writes[0] == 0x19U && writes[1] == 0x09U && output_reg == 0x09U);
    return 0;
}
