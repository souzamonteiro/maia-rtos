#include <assert.h>
#include <string.h>
#include <maia/services/rtc_rx8130.h>

static uint8_t calendar[7];
static uint8_t flags;
static maia_i2c_result_t calendar_result;
static maia_i2c_result_t flags_result;
static unsigned calendar_reads;
static unsigned flags_reads;

maia_i2c_result_t maia_i2c_bus_read8(maia_i2c_bus_t *bus, uint8_t address,
                                     uint8_t reg, uint8_t *out, uint32_t count) {
    assert(bus != NULL && address == 0x32U);
    if (reg == 0x10U) {
        assert(count == sizeof(calendar));
        ++calendar_reads;
        if (calendar_result == MAIA_I2C_OK) memcpy(out, calendar, sizeof(calendar));
        return calendar_result;
    }
    assert(reg == 0x1DU && count == 1U);
    ++flags_reads;
    if (flags_result == MAIA_I2C_OK) *out = flags;
    return flags_result;
}

static void reset_mocks(void) {
    const uint8_t valid[7] = {0x58U, 0x59U, 0x23U, 0x10U, 0x29U, 0x02U, 0x24U};
    memcpy(calendar, valid, sizeof(calendar));
    flags = 0U;
    calendar_result = MAIA_I2C_OK;
    flags_result = MAIA_I2C_OK;
    calendar_reads = 0U;
    flags_reads = 0U;
}

static maia_rtc_time_t sentinel_time(void) {
    maia_rtc_time_t value = {1999U, 12U, 31U, 1U, 2U, 3U, 4U};
    return value;
}

static void assert_unchanged(const maia_rtc_time_t *actual) {
    const maia_rtc_time_t expected = {1999U, 12U, 31U, 1U, 2U, 3U, 4U};
    assert(memcmp(actual, &expected, sizeof(expected)) == 0);
}

int main(void) {
    maia_i2c_bus_t bus = {0};
    maia_i2c_result_t io;
    maia_rtc_time_t time = sentinel_time();
    maia_rtc_raw_t raw;
    reset_mocks();
    assert(maia_rtc_rx8130_read(&bus, &time, &raw, &io) == MAIA_RTC_OK);
    assert(io == MAIA_I2C_OK);
    assert(time.year == 2024U && time.month == 2U && time.day == 29U);
    assert(time.hour == 23U && time.minute == 59U && time.second == 58U);
    assert(time.weekday == 4U);
    assert(memcmp(raw.calendar, calendar, sizeof(calendar)) == 0 && raw.flags == 0U);
    assert(calendar_reads == 1U && flags_reads == 1U);

    reset_mocks();
    flags = 0x02U;
    time = sentinel_time();
    assert(maia_rtc_rx8130_read(&bus, &time, &raw, &io) == MAIA_RTC_TIME_LOST);
    assert_unchanged(&time);
    assert(raw.flags == 0x02U);

    reset_mocks();
    calendar[0] = 0x6AU;
    time = sentinel_time();
    assert(maia_rtc_rx8130_read(&bus, &time, &raw, &io) == MAIA_RTC_INVALID_DATA);
    assert_unchanged(&time);
    assert(raw.calendar[0] == 0x6AU);

    reset_mocks();
    calendar[4] = 0x29U;
    calendar[5] = 0x02U;
    calendar[6] = 0x23U;
    time = sentinel_time();
    assert(maia_rtc_rx8130_read(&bus, &time, &raw, &io) == MAIA_RTC_INVALID_DATA);
    assert_unchanged(&time);

    reset_mocks();
    calendar[5] = 0x00U;
    time = sentinel_time();
    assert(maia_rtc_rx8130_read(&bus, &time, &raw, &io) == MAIA_RTC_INVALID_DATA);
    assert_unchanged(&time);
    assert(raw.calendar[5] == 0x00U);

    reset_mocks();
    calendar[3] = 0x03U;
    time = sentinel_time();
    assert(maia_rtc_rx8130_read(&bus, &time, &raw, &io) == MAIA_RTC_INVALID_DATA);
    assert_unchanged(&time);

    reset_mocks();
    calendar_result = MAIA_I2C_NACK;
    time = sentinel_time();
    assert(maia_rtc_rx8130_read(&bus, &time, &raw, &io) == MAIA_RTC_BUS_ERROR);
    assert(io == MAIA_I2C_NACK);
    assert_unchanged(&time);
    assert(calendar_reads == 1U && flags_reads == 0U);

    reset_mocks();
    flags_result = MAIA_I2C_TIMEOUT;
    time = sentinel_time();
    assert(maia_rtc_rx8130_read(&bus, &time, &raw, &io) == MAIA_RTC_BUS_ERROR);
    assert(io == MAIA_I2C_TIMEOUT);
    assert_unchanged(&time);
    assert(calendar_reads == 1U && flags_reads == 1U);
}
