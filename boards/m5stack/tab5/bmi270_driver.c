#include <string.h>

#include <maia/boards/tab5_bmi270.h>
#include <maia/console.h>
#include <maia/kernel.h>
#include <bmi270.h>

#define TAB5_BMI270_ADDRESS 0x68U
#define BMI270_REG_CHIP_ID 0x00U
#define BMI270_EXPECTED_CHIP_ID 0x24U

static maia_i2c_bus_t *system_bus;
static struct bmi2_dev device;
static maia_i2c_result_t last_bus_error;
static unsigned initialized;

void maia_tab5_bmi270_driver_bind(maia_i2c_bus_t *bus) {
    system_bus = bus;
}

static maia_i2c_result_t read_retry(maia_i2c_bus_t *bus,
                                    uint8_t reg, uint8_t *data, uint32_t count) {
    maia_i2c_result_t result;
    for (unsigned attempt = 0;; ++attempt) {
        result = maia_i2c_bus_read8(bus, TAB5_BMI270_ADDRESS, reg, data, count);
        if (result != MAIA_I2C_BUSY || attempt == 2U) break;
        maia_task_yield();
    }
    return result;
}

static maia_i2c_result_t write_retry(maia_i2c_bus_t *bus,
                                     uint8_t reg, const uint8_t *data, uint32_t count) {
    maia_i2c_result_t result;
    for (unsigned attempt = 0;; ++attempt) {
        result = maia_i2c_bus_write8_block(bus, TAB5_BMI270_ADDRESS, reg, data, count);
        if (result != MAIA_I2C_BUSY || attempt == 2U) break;
        maia_task_yield();
    }
    return result;
}

static int8_t bmi_read(uint8_t reg, uint8_t *data, uint32_t count, void *context) {
    maia_i2c_bus_t *bus = context;
    if (!bus || data == NULL || count == 0U || count > 70U) return -1;
    maia_i2c_result_t result = read_retry(bus, reg, data, count);
    if (result != MAIA_I2C_OK) {
        last_bus_error = result;
        return -1;
    }
    return BMI2_INTF_RET_SUCCESS;
}

static int8_t bmi_write(uint8_t reg, const uint8_t *data, uint32_t count, void *context) {
    maia_i2c_bus_t *bus = context;
    if (!bus || data == NULL || count == 0U || count > 70U) return -1;
    maia_i2c_result_t result = write_retry(bus, reg, data, count);
    if (result != MAIA_I2C_OK) {
        last_bus_error = result;
        return -1;
    }
    return BMI2_INTF_RET_SUCCESS;
}

static void bmi_delay_us(uint32_t period, void *context) {
    (void)context;
    uint32_t start;
    __asm__ volatile("rdcycle %0" : "=r"(start));
    uint32_t cycles = period > UINT32_MAX / 400U ? UINT32_MAX : period * 400U;
    uint32_t current;
    do {
        __asm__ volatile("rdcycle %0" : "=r"(current));
    } while ((uint32_t)(current - start) < cycles);
}

static maia_tab5_bmi270_result_t driver_result(int8_t result,
                                                int8_t *driver_error,
                                                maia_i2c_result_t *bus_error) {
    if (driver_error) *driver_error = result;
    if (last_bus_error != MAIA_I2C_OK) {
        if (bus_error) *bus_error = last_bus_error;
        return MAIA_TAB5_BMI270_BUS_ERROR;
    }
    if (bus_error) *bus_error = MAIA_I2C_OK;
    return result == BMI2_OK ? MAIA_TAB5_BMI270_OK : MAIA_TAB5_BMI270_DRIVER_ERROR;
}

maia_tab5_bmi270_result_t maia_tab5_bmi270_init(
    int8_t *driver_error, maia_i2c_result_t *bus_error) {
    if (driver_error) *driver_error = BMI2_OK;
    if (bus_error) *bus_error = MAIA_I2C_INVALID;
    if (!system_bus) return MAIA_TAB5_BMI270_INVALID;
    if (initialized) {
        if (bus_error) *bus_error = MAIA_I2C_OK;
        return MAIA_TAB5_BMI270_OK;
    }

    maia_console_puts("[IMU] Checking BMI270 before configuration\n");
    uint8_t chip_id;
    last_bus_error = MAIA_I2C_OK;
    maia_i2c_result_t bus_status =
        read_retry(system_bus, BMI270_REG_CHIP_ID, &chip_id, 1U);
    if (bus_status != MAIA_I2C_OK) {
        if (bus_error) *bus_error = bus_status;
        return MAIA_TAB5_BMI270_BUS_ERROR;
    }
    if (chip_id != BMI270_EXPECTED_CHIP_ID) {
        if (bus_error) *bus_error = MAIA_I2C_OK;
        return MAIA_TAB5_BMI270_ID_MISMATCH;
    }
    if (bus_error) *bus_error = MAIA_I2C_OK;

    maia_console_puts("[IMU] Starting Bosch BMI270 initialization\n");
    memset(&device, 0, sizeof(device));
    device.intf = BMI2_I2C_INTF;
    device.read = bmi_read;
    device.write = bmi_write;
    device.delay_us = bmi_delay_us;
    device.intf_ptr = system_bus;
    device.read_write_len = 30U;
    device.config_file_ptr = NULL;
    last_bus_error = MAIA_I2C_OK;

    int8_t driver_status = bmi270_init(&device);
    if (driver_status == BMI2_OK) maia_console_puts("[IMU] Bosch configuration loaded\n");
    if (driver_status != BMI2_OK) return driver_result(driver_status, driver_error, bus_error);

    struct bmi2_sens_config config[2] = {0};
    config[0].type = BMI2_ACCEL;
    config[1].type = BMI2_GYRO;
    driver_status = bmi2_get_sensor_config(config, 2U, &device);
    if (driver_status == BMI2_OK) {
        config[0].cfg.acc.odr = BMI2_ACC_ODR_200HZ;
        config[0].cfg.acc.range = BMI2_ACC_RANGE_4G;
        config[1].cfg.gyr.odr = BMI2_GYR_ODR_200HZ;
        config[1].cfg.gyr.range = BMI2_GYR_RANGE_1000;
        driver_status = bmi270_set_sensor_config(config, 2U, &device);
    }
    if (driver_status == BMI2_OK) {
        const uint8_t sensors[] = {BMI2_ACCEL, BMI2_GYRO};
        driver_status = bmi270_sensor_enable(sensors, 2U, &device);
    }

    maia_tab5_bmi270_result_t result =
        driver_result(driver_status, driver_error, bus_error);
    if (result == MAIA_TAB5_BMI270_OK) {
        initialized = 1U;
        maia_console_puts("[IMU] Accelerometer and gyroscope enabled\n");
    }
    return result;
}

maia_tab5_bmi270_result_t maia_tab5_bmi270_read(
    maia_tab5_bmi270_sample_t *sample_out,
    int8_t *driver_error,
    maia_i2c_result_t *bus_error) {
    if (driver_error) *driver_error = BMI2_OK;
    if (bus_error) *bus_error = MAIA_I2C_INVALID;
    if (!system_bus || !sample_out) return MAIA_TAB5_BMI270_INVALID;
    if (!initialized) return MAIA_TAB5_BMI270_NOT_INITIALIZED;

    struct bmi2_sens_data data = {0};
    last_bus_error = MAIA_I2C_OK;
    int8_t driver_status = bmi2_get_sensor_data(&data, &device);
    maia_tab5_bmi270_result_t result = driver_result(driver_status, driver_error, bus_error);
    if (result != MAIA_TAB5_BMI270_OK) return result;

    maia_tab5_bmi270_sample_t sample = {
        .accel_x = data.acc.x,
        .accel_y = data.acc.y,
        .accel_z = data.acc.z,
        .gyro_x = data.gyr.x,
        .gyro_y = data.gyr.y,
        .gyro_z = data.gyr.z
    };
    *sample_out = sample;
    return MAIA_TAB5_BMI270_OK;
}
