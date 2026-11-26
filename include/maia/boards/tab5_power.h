#ifndef MAIA_BOARDS_TAB5_POWER_H
#define MAIA_BOARDS_TAB5_POWER_H

#include <maia/services/i2c_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIA_TAB5_POWER_OK = 0,
    MAIA_TAB5_POWER_BUS_ERROR = -1,
    MAIA_TAB5_POWER_INVALID = -2
} maia_tab5_power_result_t;

typedef void (*maia_tab5_power_delay_fn)(uint32_t milliseconds);

/* Binds the shared System I2C bus adopted during native Tab5 bring-up. */
void maia_tab5_power_bind(maia_i2c_bus_t *bus);

/* Native port of the M5Stack Tab5 BSP power-off signal: pulses expander 0x44
 * P4 high/low three times (100 ms each), preserving the other output bits.
 * Core0 task context only. Each bus access retries briefly on BUSY. Returning
 * OK means the pulses were written; if the board is still alive, the power
 * path ignored them (for example, while USB power is present).
 */
maia_tab5_power_result_t maia_tab5_power_off(maia_tab5_power_delay_fn delay_ms,
                                             maia_i2c_result_t *bus_error);

#ifdef __cplusplus
}
#endif

#endif
