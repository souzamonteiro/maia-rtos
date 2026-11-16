/* Original MaiaRTOS MIT. Single-core native LCD owner service. */
#ifndef MAIA_LCD_OUTPUT_H
#define MAIA_LCD_OUTPUT_H
#include <maia/kernel.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct maia_lcd_output_stats {
    uint32_t frames,waits,timeouts,errors,stopped,stop_requested;
    maia_task_id_t owner;
} maia_lcd_output_stats_t;
/* Boot-only IRQs-off after native panel/PSRAM setup; owner must be READY with
 * static lifetime. Claims immutable PSRAM framebuffer until stop/recovery.
 * Single session, no SMP or arbitrary drawing/client cache sharing yet.
 * OK; BUSY if already initialized/backend refuses; PERM for wrong context;
 * INVALID for a missing/non-ready owner. Backend partial failure is reset-only. */
maia_status_t maia_lcd_output_init(maia_task_id_t owner);
/* Owner thread only, IRQs enabled: repeat frames until stop request or fault.
 * Waits on actual completion IRQ notification; finite100tick transfer timeout,
 *1tick sleeps only for residual FIFO drain. Other tasks remain schedulable.
 * Local IRQ exclusion covers short hardware transitions, never the frame wait.
 * ROM cache claim at boot and bounded32poll error cleanup have no measured WCET.
 * OK after graceful stop; PERM for wrong context/reentry; BUSY on hardware fault
 * or uncertain recovery. Failed stop retains source ownership. */
maia_status_t maia_lcd_output_run(void);
/* Nonblocking task/ISR request, native core0. Graceful current-frame completion;
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
