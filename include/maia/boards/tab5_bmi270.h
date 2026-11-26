#ifndef MAIA_BOARDS_TAB5_BMI270_H
#define MAIA_BOARDS_TAB5_BMI270_H

#include <maia/services/i2c_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIA_TAB5_BMI270_OK = 0,
    MAIA_TAB5_BMI270_BUS_ERROR = -1,
    MAIA_TAB5_BMI270_ID_MISMATCH = -2,
    MAIA_TAB5_BMI270_INVALID = -3,
    MAIA_TAB5_BMI270_DRIVER_ERROR = -4,
    MAIA_TAB5_BMI270_NOT_INITIALIZED = -5
} maia_tab5_bmi270_result_t;

typedef struct {
    int16_t accel_x;
    int16_t accel_y;
    int16_t accel_z;
    int16_t gyro_x;
    int16_t gyro_y;
    int16_t gyro_z;
} maia_tab5_bmi270_sample_t;

void maia_tab5_bmi270_bind(maia_i2c_bus_t *bus);
void maia_tab5_bmi270_driver_bind(maia_i2c_bus_t *bus);
maia_tab5_bmi270_result_t maia_tab5_bmi270_read_chip_id(
    uint8_t *chip_id_out, maia_i2c_result_t *bus_error);
/* Task-context initialization writes the Bosch BMI270 configuration and enables
 * accel (200 Hz, +/-4 g) and gyro (200 Hz, +/-1000 dps). */
maia_tab5_bmi270_result_t maia_tab5_bmi270_init(
    int8_t *driver_error, maia_i2c_result_t *bus_error);
/* Returns unscaled sensor-axis raw counts; outputs are unchanged on failure. */
maia_tab5_bmi270_result_t maia_tab5_bmi270_read(
    maia_tab5_bmi270_sample_t *sample_out,
    int8_t *driver_error,
    maia_i2c_result_t *bus_error);

#ifdef __cplusplus
}
#endif

#endif
