#ifndef MAIA_TAB5_PRODUCT_TOUCH_H
#define MAIA_TAB5_PRODUCT_TOUCH_H

#include <maia/drivers/i2c_bitbang.h>

/* Boot, IRQs off, after the display has finished using the reset bus. */
void maia_tab5_product_touch_start(const maia_i2c_lines_t *lines);
/* Mark the terminal for a redraw when the touch snapshot changes. */
void maia_product_display_touch_changed(void);

#endif
