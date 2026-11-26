/* Original MaiaRTOS MIT. Native core0 asynchronous LCD engine; caller serializes
 * state transitions with local IRQ exclusion. edit_publish performs range cache
 * synchronization with IRQs enabled. prepare/stop require exclusive source/cache
 * clients quiescent; PSRAM source held through DMA/CPU edit leases.
 * The IRQ rearms the scanout buffer at each completed frame while video remains
 * active; edits target the other framebuffer and swap at a frame boundary.
 * request_pause stops at a frame boundary for shutdown. finish BUSY retains
 * source while paused FIFOs drain. cancel bounds cleanup to
 * 32 polls; uncertain recovery cannot release memory. Clean stop permits warm
 * prepare/reuse. No SMP/cache-client sharing, WCET/tear-free guarantee or foreign
 * OS dependency. */
#ifndef MAIA_LCD_ASYNC_H
#define MAIA_LCD_ASYNC_H
#include "dsi_framebuffer_probe.h"
typedef void (*maia_lcd_wake_t)(void *);
typedef struct maia_lcd_async_report {
    maia_dsi_dma_frame_report_t last;
    uint32_t frames,irqs,spurious,notifications,drain_retries,released,init_stage,updates,presented;
} maia_lcd_async_report_t;
maia_dsi_access_result_t maia_lcd_async_prepare(maia_lcd_wake_t,void *);
maia_dsi_access_result_t maia_lcd_async_start(void);
maia_dsi_access_result_t maia_lcd_async_finish(void);
maia_dsi_access_result_t maia_lcd_async_cancel(void);
maia_dsi_access_result_t maia_lcd_async_stop(void);
maia_dsi_access_result_t maia_lcd_async_request_pause(void);
int maia_lcd_async_is_paused(void);
/* READY or ACTIVE -> exclusive CPU edit lease on the non-scanout framebuffer.
 * Preserve copies the current scanout image to this lease while DMA reads the
 * front; the client may then make incremental edits. Draw and publish with IRQs
 * enabled; publication writes back only the edited frame. The next completed-
 * frame IRQ swaps the source. No stop is legal during edit; publication failure
 * retains ownership. No bandwidth/WCET bound is claimed for the full-frame copy. */
uint16_t *maia_lcd_async_edit_begin(void);
/* While an edit lease is active, preserve the current scanout image in the
 * leased backbuffer. DMA reads the front while the owner copies it. */
int maia_lcd_async_edit_preserve(void);
maia_dsi_access_result_t maia_lcd_async_edit_publish(void);
int maia_lcd_async_pending(void);
void maia_lcd_async_report(maia_lcd_async_report_t *);
#endif
