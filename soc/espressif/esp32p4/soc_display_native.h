#ifndef MAIA_ESP32P4_DISPLAY_NATIVE_H
#define MAIA_ESP32P4_DISPLAY_NATIVE_H

#include <stdint.h>
#include <maia/drivers/i2c_bitbang.h>

int maia_esp32p4_display_st7121_prepare(uint8_t firmware);
/* Only after successful display preparation, before scheduler start. */
const maia_i2c_lines_t *maia_esp32p4_display_reset_i2c_lines(void);

#endif
