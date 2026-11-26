#ifndef MAIA_BOARDS_TAB5_RTC_H
#define MAIA_BOARDS_TAB5_RTC_H

#include <maia/services/rtc_rx8130.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Called during native Tab5 bring-up after adopting the shared System I2C bus. */
void maia_tab5_rtc_bind(maia_i2c_bus_t *bus);
maia_rtc_result_t maia_tab5_rtc_read(maia_rtc_time_t *time_out,
                                     maia_rtc_raw_t *raw_out,
                                     maia_i2c_result_t *bus_error);

#ifdef __cplusplus
}
#endif

#endif
