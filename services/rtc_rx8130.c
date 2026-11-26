#include <maia/services/rtc_rx8130.h>

#define RX8130_ADDRESS 0x32U
#define RX8130_REG_SECONDS 0x10U
#define RX8130_REG_FLAGS 0x1DU
#define RX8130_VLF_MASK 0x02U

static int bcd_to_u8(uint8_t raw, uint8_t mask, uint8_t *value) {
    if ((raw & (uint8_t)~mask) != 0U) return 0;
    raw &= mask;
    uint8_t tens = (uint8_t)(raw >> 4);
    uint8_t ones = (uint8_t)(raw & 0x0FU);
    if (tens > 9U || ones > 9U) return 0;
    *value = (uint8_t)(tens * 10U + ones);
    return 1;
}

static uint8_t days_in_month(uint16_t year, uint8_t month) {
    static const uint8_t days[] = {31U, 28U, 31U, 30U, 31U, 30U,
                                   31U, 31U, 30U, 31U, 30U, 31U};
    if (month == 2U && (year % 4U) == 0U) return 29U;
    return days[month - 1U];
}

maia_rtc_result_t maia_rtc_rx8130_read(maia_i2c_bus_t *bus,
                                       maia_rtc_time_t *time_out,
                                       maia_rtc_raw_t *raw_out,
                                       maia_i2c_result_t *bus_error) {
    uint8_t calendar[7];
    uint8_t flags;
    maia_rtc_time_t decoded;
    if (bus_error) *bus_error = MAIA_I2C_OK;
    if (!bus || !time_out) return MAIA_RTC_INVALID_DATA;

    maia_i2c_result_t io = maia_i2c_bus_read8(
        bus, RX8130_ADDRESS, RX8130_REG_SECONDS, calendar, sizeof(calendar));
    if (io != MAIA_I2C_OK) {
        if (bus_error) *bus_error = io;
        return MAIA_RTC_BUS_ERROR;
    }
    io = maia_i2c_bus_read8(bus, RX8130_ADDRESS, RX8130_REG_FLAGS, &flags, 1U);
    if (io != MAIA_I2C_OK) {
        if (bus_error) *bus_error = io;
        return MAIA_RTC_BUS_ERROR;
    }
    if (raw_out) {
        for (uint8_t i = 0U; i < sizeof(calendar); ++i) {
            raw_out->calendar[i] = calendar[i];
        }
        raw_out->flags = flags;
    }
    if ((flags & RX8130_VLF_MASK) != 0U) return MAIA_RTC_TIME_LOST;

    if (!bcd_to_u8(calendar[0], 0x7FU, &decoded.second) ||
        !bcd_to_u8(calendar[1], 0x7FU, &decoded.minute) ||
        !bcd_to_u8(calendar[2], 0x3FU, &decoded.hour) ||
        !bcd_to_u8(calendar[4], 0x3FU, &decoded.day) ||
        !bcd_to_u8(calendar[5], 0x1FU, &decoded.month)) {
        return MAIA_RTC_INVALID_DATA;
    }
    uint8_t year;
    if (!bcd_to_u8(calendar[6], 0xFFU, &year)) return MAIA_RTC_INVALID_DATA;

    uint8_t week = calendar[3];
    if ((week & 0x80U) != 0U || week == 0U || (week & (uint8_t)(week - 1U)) != 0U ||
        decoded.second > 59U || decoded.minute > 59U || decoded.hour > 23U ||
        decoded.month < 1U || decoded.month > 12U) {
        return MAIA_RTC_INVALID_DATA;
    }

    decoded.year = (uint16_t)(2000U + year);
    if (decoded.day < 1U || decoded.day > days_in_month(decoded.year, decoded.month)) {
        return MAIA_RTC_INVALID_DATA;
    }
    decoded.weekday = 0U;
    while ((week & 1U) == 0U) {
        week >>= 1;
        ++decoded.weekday;
    }
    *time_out = decoded;
    return MAIA_RTC_OK;
}
