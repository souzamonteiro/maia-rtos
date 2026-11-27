/* Original MaiaRTOS MIT. Single-core native LCD owner service. */
#ifndef MAIA_LCD_OUTPUT_H
#define MAIA_LCD_OUTPUT_H
#include <maia/kernel.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct maia_lcd_output_stats {
    uint32_t frames,waits,timeouts,errors,stopped,stop_requested;
    uint32_t updates,presented;
    maia_task_id_t owner;
} maia_lcd_output_stats_t;
/* Borrowed packed native-orientation RGB565 backbuffer; pitch=width pixels.
 * The callback must produce a complete frame and runs on the owner thread with
 * IRQs enabled while DMA scans the other buffer. Do not retain the pointer, touch
 * outside width*height, throw, or call other PSRAM/cache/lifecycle APIs.
 * The callback must return; scheduling remains enabled but its duration is not
 * covered by the DMA timeout. Callback and arg must live until publication/stop.
 * No SMP or unrelated PSRAM/cache clients are supported in this warm service. */
typedef void (*maia_lcd_draw_fn)(uint16_t *,uint32_t width,uint32_t height,void *);
/* Core0 task/ISR, one pending request. OK queued (not yet visible), BUSY if a
 * request/edit/presentation exists; INVALID null callback or stopped/stopping/
 * uninitialized. Stop cancels requests not yet taken; a callback already started
 * publishes and presents one frame before stopping. Updates swap buffers at a
 * frame boundary without pausing DMA or reinitializing DPI/DSI video. updates
 * counts cache publication; presented counts completed frames after each swap. */
maia_status_t maia_lcd_output_request_update(maia_lcd_draw_fn,void *);
/* Task-only incremental edit lease for native modules. The single LCD owner
 * leases and seeds the backbuffer from the current front before returning it.
 * The caller may edit only the borrowed buffer and must publish it before
 * requesting another lease. Publish waits until the new frame is presented.
 * Only one client lease may exist; stop is rejected while a lease is active.
 * The client must be a different task from the LCD owner and keep the lease
 * pointer only until publish returns. OK; BUSY for another client/stop; PERM
 * for IRQ-off/non-task/owner callers; INVALID if the service is not running. */
maia_status_t maia_lcd_output_edit_begin(uint16_t **pixels);
maia_status_t maia_lcd_output_edit_publish(void);
/* Same as edit_publish, declaring that only the native-orientation half-open box
 * [x0,x1)x[y0,y1) (720x1280) changed since the lease began. The next lease then
 * seeds only that box instead of copying the whole frame. An understated box
 * leaves stale pixels in the other buffer; an empty box is legal. INVALID for a
 * box outside the panel. */
maia_status_t maia_lcd_output_edit_publish_region(uint32_t x0,uint32_t y0,uint32_t x1,uint32_t y1);
/* Task-only direct scanout lease (any core, non-owner): returns the buffer DMA
 * is scanning, as the ESP-IDF reference's single framebuffer. Draw in place and
 * flush the changed native-orientation box [x0,x1)x[y0,y1) with a cache
 * writeback; no frame wait, tearing is possible. While held, owner updates and
 * backbuffer edits wait. Begin waits up to 200 ticks for an in-flight owner
 * update/presentation: OK; BUSY on timeout; PERM for IRQ-off/owner callers;
 * INVALID if not running or already held by the caller. flush/end: INVALID if
 * the caller does not hold the lease; flush BUSY on a cache failure. */
maia_status_t maia_lcd_output_direct_begin(uint16_t **pixels);
maia_status_t maia_lcd_output_direct_flush(uint32_t x0,uint32_t y0,uint32_t x1,uint32_t y1);
maia_status_t maia_lcd_output_direct_end(void);
/* Owner thread, IRQs enabled, only after run returned a clean graceful stop.
 * Reclaims the same warm panel/window/config; clears per-session counters.
 * OK; PERM wrong context; BUSY active/faulted/backend refusal. Not cold init. */
maia_status_t maia_lcd_output_restart(void);
/* Boot-only IRQs-off after native panel/PSRAM setup; owner must be READY with
 * static lifetime. Claims PSRAM framebuffer until stop/recovery.
 * One active session, CPU drawing only via queued callback; no SMP/cache sharing.
 * OK; BUSY if already initialized/backend refuses; PERM for wrong context;
 * INVALID for a missing/non-ready owner. Backend partial failure is reset-only. */
maia_status_t maia_lcd_output_init(maia_task_id_t owner);
/* Owner thread only, IRQs enabled: continuously refresh the framebuffer until
 * stop request or fault. The completion IRQ rearms DMA without disabling video;
 * CPU updates swap buffers at a frame boundary, shutdown pauses there. Waits on actual completion
 * IRQ notification; finite100tick frame timeout,1tick sleeps only for residual
 * FIFO drain. Other tasks remain schedulable.
 * Local IRQ exclusion covers short hardware transitions, never the frame wait.
 * ROM cache claim at boot and bounded32poll error cleanup have no measured WCET.
 * OK after graceful stop; PERM for wrong context/reentry; BUSY on hardware fault
 * or uncertain recovery. Failed stop retains source ownership. */
maia_status_t maia_lcd_output_run(void);
/* Nonblocking task/ISR request, native core0. Graceful next-frame-boundary stop;
 * timeout invokes guarded cancel. Does not revoke source ownership by itself.
 * OK when queued; INVALID before init/after stop. */
maia_status_t maia_lcd_output_request_stop(void);
/* Native core0 task/ISR snapshot under local IRQ exclusion. INVALID for null
 * output or before init (a nonnull output still receives current counters). */
maia_status_t maia_lcd_output_stats(maia_lcd_output_stats_t *);
#ifdef __cplusplus
}
#endif
#endif
