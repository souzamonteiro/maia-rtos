#ifndef MAIA_BOARDS_TAB5_INA226_H
#define MAIA_BOARDS_TAB5_INA226_H

#include <stdbool.h>
#include <maia/services/ina226.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Called after the native Tab5 bring-up adopts the shared System I2C bus. */
void maia_tab5_ina226_bind(maia_i2c_bus_t *bus);
maia_ina226_result_t maia_tab5_ina226_read(maia_ina226_sample_t *sample_out,
                                          maia_i2c_result_t *bus_error);

typedef struct {
    uint32_t bus_millivolts;
    int32_t current_microamps;
    int32_t power_microwatts;
} maia_tab5_ina226_measurements_t;

/* Derives current and power from the shunt and bus voltage registers using
 * the Tab5's nominal 5 mOhm shunt. Requires continuous shunt+bus mode.
 */
bool maia_tab5_ina226_derive(const maia_ina226_sample_t *sample,
                            maia_tab5_ina226_measurements_t *measurements_out);

#ifdef __cplusplus
}
#endif

#endif
