#include <maia/boards/tab5_rtc.h>

static maia_i2c_bus_t *system_bus;

void maia_tab5_rtc_bind(maia_i2c_bus_t *bus) {
    system_bus = bus;
}

maia_rtc_result_t maia_tab5_rtc_read(maia_rtc_time_t *time_out,
                                     maia_rtc_raw_t *raw_out,
                                     maia_i2c_result_t *bus_error) {
    if (!system_bus) {
        if (bus_error) *bus_error = MAIA_I2C_INVALID;
        return MAIA_RTC_BUS_ERROR;
    }
    return maia_rtc_rx8130_read(system_bus, time_out, raw_out, bus_error);
}
