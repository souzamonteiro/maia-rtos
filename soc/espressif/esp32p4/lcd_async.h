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
/* Native-orientation half-open rectangle [x0,x1)x[y0,y1) of 720x1280 pixels. */
typedef struct maia_lcd_async_box {uint32_t x0,y0,x1,y1;} maia_lcd_async_box_t;
/* Publish declaring the only pixels changed since preserve. The two buffers then
 * differ only there, so the next preserve copies just that box instead of the
 * whole frame. NULL (or plain edit_publish) means unknown: next copy is full. */
maia_dsi_access_result_t maia_lcd_async_edit_publish_box(const maia_lcd_async_box_t *);
/* Direct scanout lease (single-buffer model, like the ESP-IDF reference): the
 * client draws into the framebuffer DMA is scanning and flushes changed rows
 * with a cache writeback; tearing is possible, no frame wait. Any hart, caller
 * holds the kernel lock (maia_arch_irq_save) for begin/end. Begin requires
 * ACTIVE video with no edit, swap or pause in flight and returns the front
 * buffer or 0. While held, edit_begin and request_pause refuse. flush takes the
 * kernel lock itself only for the ROM cache operation; the box joins the stale
 * region so the next backbuffer edit re-seeds it. */
uint16_t *maia_lcd_async_direct_begin(void);
maia_dsi_access_result_t maia_lcd_async_direct_flush(const maia_lcd_async_box_t *);
void maia_lcd_async_direct_end(void);
int maia_lcd_async_pending(void);
void maia_lcd_async_report(maia_lcd_async_report_t *);
#endif
