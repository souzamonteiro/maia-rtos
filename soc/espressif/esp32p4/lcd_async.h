/* Original MaiaRTOS MIT. Native core0 asynchronous LCD engine; caller serializes
 * each short operation with local IRQ exclusion. prepare/stop require exclusive
 * source/cache clients quiescent; immutable PSRAM source held for whole session.
 * No blocking frame wait inside engine; IRQ latches predicate and wakes owner.
 * finish BUSY retains source while FIFOs drain. cancel bounds cleanup to32 polls;
 * uncertain recovery cannot release memory. Single prepared session until reboot;
 * repeated frame starts are supported, not full controller cold reinitialization.
 * Task scheduling, timeout units and callback lifetime belong to native service.
 * No SMP/cache-client sharing, WCET/tear-free guarantee or foreign OS dependency. */
#ifndef MAIA_LCD_ASYNC_H
#define MAIA_LCD_ASYNC_H
#include "dsi_framebuffer_probe.h"
typedef void (*maia_lcd_wake_t)(void *);
typedef struct maia_lcd_async_report {
    maia_dsi_dma_frame_report_t last;
    uint32_t frames,irqs,spurious,notifications,drain_retries,released,init_stage;
} maia_lcd_async_report_t;
maia_dsi_access_result_t maia_lcd_async_prepare(maia_lcd_wake_t,void *);
maia_dsi_access_result_t maia_lcd_async_start(void);
maia_dsi_access_result_t maia_lcd_async_finish(void);
maia_dsi_access_result_t maia_lcd_async_cancel(void);
maia_dsi_access_result_t maia_lcd_async_stop(void);
int maia_lcd_async_pending(void);
void maia_lcd_async_report(maia_lcd_async_report_t *);
#endif
