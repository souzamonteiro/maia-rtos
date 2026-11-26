#ifndef MAIA_SERVICES_INA226_H
#define MAIA_SERVICES_INA226_H

#include <maia/services/i2c_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIA_INA226_OK = 0,
    MAIA_INA226_BUS_ERROR = -1,
    MAIA_INA226_DEVICE_ID = -2,
    MAIA_INA226_INVALID = -3
} maia_ina226_result_t;

typedef struct {
    uint16_t manufacturer_id;
    uint16_t die_id;
    uint16_t config;
    uint16_t calibration;
    int16_t shunt_raw;
    uint16_t bus_raw;
    uint16_t power_raw;
    int16_t current_raw;
} maia_ina226_sample_t;

/* Reads the INA226 register set without writing device state or reading the
 * Mask/Enable register. On bus failure sample_out is unchanged. If identity
 * does not match, sample_out receives just the two IDs and no other registers
 * are read. bus_error receives the I2C result when applicable.
 */
maia_ina226_result_t maia_ina226_read(maia_i2c_bus_t *bus,
                                      uint8_t address,
                                      maia_ina226_sample_t *sample_out,
                                      maia_i2c_result_t *bus_error);

/* Validates the INA226 identity and reads only the bus-voltage register. */
maia_ina226_result_t maia_ina226_read_bus_voltage(maia_i2c_bus_t *bus,
                                                 uint8_t address,
                                                 uint16_t *bus_raw_out,
                                                 maia_i2c_result_t *bus_error);

#ifdef __cplusplus
}
#endif

#endif
