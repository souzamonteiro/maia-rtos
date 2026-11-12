/* Independently authored bounded single-master register-read transport. */
#include <maia/drivers/i2c_bitbang.h>

static int valid(const maia_i2c_lines_t *b, uint8_t address) {
    return b && b->drive && b->sample && b->pause && address >= 8U && address <= 0x77U;
}
static void drive(const maia_i2c_lines_t *b, unsigned mask, unsigned release) {
    b->drive(b->context, mask, release);
}
static maia_i2c_result_t high(const maia_i2c_lines_t *b) {
    drive(b, MAIA_I2C_SCL, 1U);
    for (unsigned i = 0U; i < 40U; ++i) {
        maia_i2c_result_t r = b->pause(b->context);
        if (r != MAIA_I2C_OK) return r;
        /* Stretch may end just before sampling; hold a full high phase
         * after observing HIGH instead of counting the preceding wait. */
        if (b->sample(b->context) & MAIA_I2C_SCL) return b->pause(b->context);
    }
    return MAIA_I2C_TIMEOUT;
}
static maia_i2c_result_t start(const maia_i2c_lines_t *b) {
    drive(b, MAIA_I2C_BOTH, 1U);
    maia_i2c_result_t r = high(b);
    if (r != MAIA_I2C_OK) return r;
    if (!(b->sample(b->context) & MAIA_I2C_SDA)) return MAIA_I2C_BUSY;
    drive(b, MAIA_I2C_SDA, 0U);
    r = b->pause(b->context);
    drive(b, MAIA_I2C_SCL, 0U);
    return r;
}
static maia_i2c_result_t stop(const maia_i2c_lines_t *b) {
    drive(b, MAIA_I2C_SCL, 0U);
    drive(b, MAIA_I2C_SDA, 0U);
    maia_i2c_result_t r = b->pause(b->context);
    if (r != MAIA_I2C_OK) return r;
    r = high(b);
    if (r != MAIA_I2C_OK) return r;
    drive(b, MAIA_I2C_SDA, 1U);
    r = b->pause(b->context);
    if (r != MAIA_I2C_OK) return r;
    return (b->sample(b->context) & MAIA_I2C_BOTH) == MAIA_I2C_BOTH ? MAIA_I2C_OK : MAIA_I2C_BUSY;
}
static maia_i2c_result_t send(const maia_i2c_lines_t *b, uint8_t byte) {
    for (unsigned i = 0U; i < 8U; ++i) {
        drive(b, MAIA_I2C_SDA, (byte & 0x80U) != 0U);
        maia_i2c_result_t r = b->pause(b->context);
        if (r != MAIA_I2C_OK) return r;
        r = high(b);
        if (r != MAIA_I2C_OK) return r;
        drive(b, MAIA_I2C_SCL, 0U);
        byte = (uint8_t)(byte << 1U);
    }
    drive(b, MAIA_I2C_SDA, 1U);
    maia_i2c_result_t r = b->pause(b->context);
    if (r != MAIA_I2C_OK) return r;
    r = high(b);
    if (r != MAIA_I2C_OK) return r;
    unsigned ack = !(b->sample(b->context) & MAIA_I2C_SDA);
    drive(b, MAIA_I2C_SCL, 0U);
    return ack ? MAIA_I2C_OK : MAIA_I2C_NACK;
}
static maia_i2c_result_t receive(const maia_i2c_lines_t *b, uint8_t *out) {
    uint8_t byte = 0U;
    drive(b, MAIA_I2C_SDA, 1U);
    for (unsigned i = 0U; i < 8U; ++i) {
        maia_i2c_result_t r = b->pause(b->context);
        if (r != MAIA_I2C_OK) return r;
        r = high(b);
        if (r != MAIA_I2C_OK) return r;
        byte = (uint8_t)((byte << 1U) | ((b->sample(b->context) & MAIA_I2C_SDA) ? 1U : 0U));
        drive(b, MAIA_I2C_SCL, 0U);
    }
    /* Single-byte read ends with master NACK, not ACK. */
    maia_i2c_result_t r = b->pause(b->context);
    if (r == MAIA_I2C_OK) r = high(b);
    drive(b, MAIA_I2C_SCL, 0U);
    if (r == MAIA_I2C_OK) *out = byte;
    return r;
}
static maia_i2c_result_t finish(const maia_i2c_lines_t *b, maia_i2c_result_t r) {
    if (r == MAIA_I2C_OK || r == MAIA_I2C_NACK) {
        maia_i2c_result_t s = stop(b);
        if (s != MAIA_I2C_OK) r = s;
    }
    drive(b, MAIA_I2C_BOTH, 1U);
    return r;
}
maia_i2c_result_t maia_i2c_probe_address(const maia_i2c_lines_t *b, uint8_t address) {
    if (!valid(b, address)) return MAIA_I2C_INVALID;
    maia_i2c_result_t r = start(b);
    if (r == MAIA_I2C_OK) r = send(b, (uint8_t)(address << 1U));
    return finish(b, r);
}
maia_i2c_result_t maia_i2c_read_register16(const maia_i2c_lines_t *b,
                                        uint8_t address, uint16_t reg, uint8_t *out) {
    if (!valid(b, address) || !out) return MAIA_I2C_INVALID;
    maia_i2c_result_t r = start(b);
    if (r == MAIA_I2C_OK) r = send(b, (uint8_t)(address << 1U));
    if (r == MAIA_I2C_OK) r = send(b, (uint8_t)(reg >> 8U));
    if (r == MAIA_I2C_OK) r = send(b, (uint8_t)reg);
    r = finish(b, r);
    if (r != MAIA_I2C_OK) return r;
    r = start(b);
    if (r == MAIA_I2C_OK) r = send(b, (uint8_t)((address << 1U) | 1U));
    uint8_t value = 0U;
    if (r == MAIA_I2C_OK) r = receive(b, &value);
    r = finish(b, r);
    if (r == MAIA_I2C_OK) *out = value;
    return r;
}

/* Unlike the touch protocol, an 8-bit-pointer register read retains bus
 * ownership through a repeated START. SDA is released while SCL is low. */
static maia_i2c_result_t restart(const maia_i2c_lines_t *b) {
    drive(b, MAIA_I2C_SDA, 1U);
    maia_i2c_result_t r = b->pause(b->context);
    if (r == MAIA_I2C_OK) r = high(b);
    if (r != MAIA_I2C_OK) return r;
    if (!(b->sample(b->context) & MAIA_I2C_SDA)) return MAIA_I2C_BUSY;
    drive(b, MAIA_I2C_SDA, 0U);
    r = b->pause(b->context);
    drive(b, MAIA_I2C_SCL, 0U);
    return r;
}
maia_i2c_result_t maia_i2c_read_register8(const maia_i2c_lines_t *b,
                                       uint8_t address, uint8_t reg, uint8_t *out) {
    if (!valid(b, address) || !out) return MAIA_I2C_INVALID;
    maia_i2c_result_t r = start(b);
    if (r == MAIA_I2C_OK) r = send(b, (uint8_t)(address << 1U));
    if (r == MAIA_I2C_OK) r = send(b, reg);
    if (r == MAIA_I2C_OK) r = restart(b);
    if (r == MAIA_I2C_OK) r = send(b, (uint8_t)((address << 1U) | 1U));
    uint8_t value = 0U;
    if (r == MAIA_I2C_OK) r = receive(b, &value);
    r = finish(b, r);
    if (r == MAIA_I2C_OK) *out = value;
    return r;
}
maia_i2c_result_t maia_i2c_write_register8(const maia_i2c_lines_t *b,
                                        uint8_t address, uint8_t reg, uint8_t value) {
    if (!valid(b, address)) return MAIA_I2C_INVALID;
    maia_i2c_result_t r = start(b);
    if (r == MAIA_I2C_OK) r = send(b, (uint8_t)(address << 1U));
    if (r == MAIA_I2C_OK) r = send(b, reg);
    if (r == MAIA_I2C_OK) r = send(b, value);
    return finish(b, r);
}
