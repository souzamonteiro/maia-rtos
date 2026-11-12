#ifndef MAIA_BOARDS_TAB5_PANEL_RESET_H
#define MAIA_BOARDS_TAB5_PANEL_RESET_H
#include <maia/drivers/pi4ioe5v6408.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum maia_tab5_reset_stage {
    MAIA_TAB5_RESET_NONE = 0,
    MAIA_TAB5_RESET_CHECKED,
    MAIA_TAB5_RESET_PULLUP,
    MAIA_TAB5_RESET_LATCH_LOW,
    MAIA_TAB5_RESET_DRIVE_ENABLED,
    MAIA_TAB5_RESET_HELD_LOW,
    MAIA_TAB5_RESET_TOUCH_RELEASED,
    MAIA_TAB5_RESET_LCD_RELEASED,
    MAIA_TAB5_RESET_SETTLED,
    MAIA_TAB5_RESET_VERIFIED
} maia_tab5_reset_stage_t;
typedef struct maia_tab5_reset_report {
    maia_tab5_reset_stage_t completed; /* Last fully checked stage. */
    unsigned may_have_changed; /* Conservative; set BEFORE first update attempt. */
} maia_tab5_reset_report_t;
/* ST7121/ST7123 Tab5 only: caller must have identified the variant, stabilized
 * supplies, left GPIO23/TP_INT as input, and stopped display/DMA activity.
 * Boot/core0 only, exclusive ownership of bus + reset pins for entire operation;
 * with GPIO backend IRQs must already be off. No ISR/task/SMP/concurrent use.
 * Does not configure supplies, GPIO23, LDO, backlight, DSI, cache or DMA.
 * Holds P4/P5 low >=100ms then releases P5 high and P4 via input/pull-up, waits
 * >=100ms. Uses pause callback's >=5us contract, 20000 bounded calls per wait.
 * Only bank43 reset bits are changed; unrelated config preserved and checked.
 * Rejects an initially driven-high LCD_RST before writes. Required report is
 * initialized on entry; after any update failure hardware state is uncertain,
 * possibly still held in reset. No retry/rollback/release on error: caller must
 * stop bring-up and reset/reinitialize the board through a known recovery path.
 * OK means register sequence/delays verified, NOT working pixels or touch ID. */
maia_i2c_result_t maia_tab5_panel_reset(const maia_i2c_lines_t *bus,
                                      maia_tab5_reset_report_t *report);
#ifdef __cplusplus
}
#endif
#endif
