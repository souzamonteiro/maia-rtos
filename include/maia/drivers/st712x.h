/* Native port of Espressif ST7123 report decoding; Apache-2.0.
 * No foreign runtime. Caller owns controller/bus, boot or thread only, no ISR.
 * Sources/revision and ST7121 qualification: docs/audits/native-touch-input.md. */
#ifndef MAIA_ST712X_H
#define MAIA_ST712X_H
#include <maia/drivers/i2c_bitbang.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAIA_TOUCH_MAX_POINTS 10U
typedef maia_i2c_result_t (*maia_touch_read_fn)(void *,uint16_t,uint8_t *,uint32_t);
typedef struct maia_st712x_info {uint16_t max_x,max_y;uint8_t firmware,slots;} maia_st712x_info_t;
typedef struct maia_touch_point {uint16_t x,y;uint8_t id,area;} maia_touch_point_t;
typedef struct maia_touch_frame {
    uint8_t fresh,count;
    maia_touch_point_t points[MAIA_TOUCH_MAX_POINTS];
} maia_touch_frame_t;
/* Outputs committed only after complete success. OK/fresh0 means no new report,
 * never finger release. OK/fresh1/count0 is an actual empty report. Read through
 * the final supported slot acknowledges a report. Errors must invalidate client
 * state, never fabricate release. No proximity/gesture/checksum mode support. */
maia_i2c_result_t maia_st712x_identify(maia_touch_read_fn,void *,maia_st712x_info_t *);
maia_i2c_result_t maia_st712x_read(maia_touch_read_fn,void *,const maia_st712x_info_t *,maia_touch_frame_t *);
#ifdef __cplusplus
}
#endif
#endif
