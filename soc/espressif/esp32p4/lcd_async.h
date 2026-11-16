/* Original MaiaRTOS MIT. Native core0 asynchronous LCD engine; caller serializes
 * each short operation with local IRQ exclusion. prepare/stop require exclusive
 * source/cache clients quiescent; PSRAM source held through DMA/CPU edit leases.
 * No blocking frame wait inside engine; IRQ latches predicate and wakes owner.
 * finish BUSY retains source while FIFOs drain. cancel bounds cleanup to32 polls;
 * uncertain recovery cannot release memory. Clean stop permits warm prepare/reuse;
 * no full controller cold reinitialization or recovery after uncertain faults.
 * Task scheduling, timeout units and callback lifetime belong to native service.
 * No SMP/cache-client sharing, WCET/tear-free guarantee or foreign OS dependency. */
#ifndef MAIA_LCD_ASYNC_H
#define MAIA_LCD_ASYNC_H
#include "dsi_framebuffer_probe.h"
typedef void (*maia_lcd_wake_t)(void *);
typedef struct maia_lcd_async_report {
    maia_dsi_dma_frame_report_t last;
    uint32_t frames,irqs,spurious,notifications,drain_retries,released,init_stage,updates;
} maia_lcd_async_report_t;
maia_dsi_access_result_t maia_lcd_async_prepare(maia_lcd_wake_t,void *);
maia_dsi_access_result_t maia_lcd_async_start(void);
maia_dsi_access_result_t maia_lcd_async_finish(void);
maia_dsi_access_result_t maia_lcd_async_cancel(void);
maia_dsi_access_result_t maia_lcd_async_stop(void);
/* READY/idle only -> exclusive CPU edit lease. Draw outside engine with IRQs
 * enabled, then publish with IRQs excluded to write back cache and reclaim DMA.
 * No start/stop is legal during an edit; publication failure retains ownership. */
uint16_t *maia_lcd_async_edit_begin(void);
maia_dsi_access_result_t maia_lcd_async_edit_publish(void);
int maia_lcd_async_pending(void);
void maia_lcd_async_report(maia_lcd_async_report_t *);
#endif
