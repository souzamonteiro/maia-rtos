#ifndef MAIA_BOARDS_TAB5_BATTERY_H
#define MAIA_BOARDS_TAB5_BATTERY_H

#include <stdbool.h>
#include <maia/services/i2c_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIA_TAB5_BATTERY_OK = 0,
    MAIA_TAB5_BATTERY_BUS_ERROR = -1,
    MAIA_TAB5_BATTERY_DEVICE_ID = -2,
    MAIA_TAB5_BATTERY_INVALID = -3
} maia_tab5_battery_result_t;

typedef struct {
    uint32_t monitor_bus_millivolts;
    bool charge_signal_high;
} maia_tab5_battery_sample_t;

/* Binds the shared System I2C bus adopted during native Tab5 bring-up. */
void maia_tab5_battery_bind(maia_i2c_bus_t *bus);

/* Reads INA226 bus voltage and the raw expander P6 charge signal. The signal
 * is not translated to charge state or state-of-charge percentage.
 */
maia_tab5_battery_result_t maia_tab5_battery_read(maia_tab5_battery_sample_t *sample_out,
                                                  maia_i2c_result_t *bus_error);

#ifdef __cplusplus
}
#endif

#endif
