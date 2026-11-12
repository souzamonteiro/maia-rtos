#ifndef MAIA_BOARDS_TAB5_TOUCH_H
#define MAIA_BOARDS_TAB5_TOUCH_H
#include <maia/drivers/i2c_bitbang.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Exclusive boot-time bus on Tab5 system SDA31/SCL32. No power/reset writes.
 * Reads status 0x0001 first; Normal(0) or Idle(4) permit this read-only ID.
 * Init, error, power-down and unrecognized states return BUSY.
 * Then reads firmware 0x0000. out changes only on success. Raw firmware is not
 * a physical LCD ID: legacy BSP maps 1->ST7121, 3->ST7123. Unknown values must
 * stay unknown, never authorize panel initialization. Same bus/context rules as
 * bitbang.h, no ISR, concurrent callers, implicit setup, retries or fallback. */
maia_i2c_result_t maia_tab5_touch_identify(const maia_i2c_lines_t *bus, uint8_t *firmware);
#ifdef __cplusplus
}
#endif
#endif
