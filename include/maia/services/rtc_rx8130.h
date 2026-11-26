#ifndef MAIA_SERVICES_RTC_RX8130_H
#define MAIA_SERVICES_RTC_RX8130_H

#include <maia/services/i2c_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIA_RTC_OK = 0,
    MAIA_RTC_BUS_ERROR = -1,
    MAIA_RTC_INVALID_DATA = -2,
    MAIA_RTC_TIME_LOST = -3
} maia_rtc_result_t;

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint8_t weekday;
} maia_rtc_time_t;

typedef struct {
    uint8_t calendar[7];
    uint8_t flags;
} maia_rtc_raw_t;

/* Reads the RX8130CE calendar without changing device registers. Years map to
 * 2000-2099; weekday is 0 for Sunday through 6 for Saturday. On failure,
 * time_out is unchanged and bus_error receives the I2C result when applicable.
 * raw_out is optional and receives the registers after successful bus reads,
 * including when the data is invalid or VLF is set.
 */
maia_rtc_result_t maia_rtc_rx8130_read(maia_i2c_bus_t *bus,
                                       maia_rtc_time_t *time_out,
                                       maia_rtc_raw_t *raw_out,
                                       maia_i2c_result_t *bus_error);

#ifdef __cplusplus
}
#endif

#endif
