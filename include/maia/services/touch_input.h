/* Original MaiaRTOS MIT. Native core0 latest-state touch input, no event queue. */
#ifndef MAIA_TOUCH_INPUT_H
#define MAIA_TOUCH_INPUT_H
#include <maia/kernel.h>
#include <maia/drivers/st712x.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct maia_touch_snapshot {
    maia_touch_frame_t frame; /* logical coordinates, stable slot IDs */
    uint32_t valid,sequence,polls,reports,errors,downs,moves,ups;
    uint32_t stopped,stops,restarts;
    uint64_t last_poll;
    int32_t last_error;
} maia_touch_snapshot_t;
/* Pure checked transform. Rotation clockwise0/90/180/270, native dimensions.
 *90 maps native(x,y)->(height-1-y,x), matching the display. INVALID leaves out.
 * Max advertised coordinates do not authorize out-of-raster values or clipping. */
maia_status_t maia_touch_transform(uint16_t x,uint16_t y,uint16_t width,uint16_t height,uint16_t rotation,uint16_t *out_x,uint16_t *out_y);
/* Boot IRQs-off/pre-scheduler, stationary reader/context, READY static owner.
 * Caller identifies controller first; accepts at most10 slots. Dimensions denote
 * actual raster, never inferred from a missing controller. No other reader of
 * this touch controller may consume reports. OK; PERM context; INVALID args or
 * owner; BUSY already initialized. No runtime callbacks during init. */
maia_status_t maia_touch_input_init(maia_task_id_t,maia_touch_read_fn,void *,const maia_st712x_info_t *,uint16_t width,uint16_t height,uint16_t rotation);
/* Owner task/IRQs-on, polls once, never masks IRQs for the wire read. OK includes
 * no fresh report (holds state). Error/context -> BUSY/PERM; wire errors recorded
 * invalidate snapshot, never synthesize UP. A later complete report recovers.
 * Caller schedules polling. No ISR/SMP; reader must have bounded completion. */
maia_status_t maia_touch_input_poll(void);
/* Owner task/IRQs-on only; stop invalidates held contacts (no synthetic UP) and
 * stops wire access. Stop refuses an active poll/reentrant callback. Restart
 * rereads identity/capabilities with IRQs enabled and accepts only the original
 * firmware/maxima/slot count. Reopen adopted bus before restarting. A changed or
 * failed reader leaves input stopped/invalid; a later explicit retry is allowed.
 * Counters/sequence remain cumulative; fresh report required after restart.
 * OK; PERM context/uninitialized; BUSY active/wrong state/identity/read failure.
 * No hardware reset/power/clock changes; caller still schedules poll calls. */
maia_status_t maia_touch_input_stop(void);
maia_status_t maia_touch_input_restart(void);
/* Core0 task/ISR short snapshot. Always fills nonnull out. OK only for valid
 * latest state; BUSY until report/after error; TIMEOUT when last poll >100ticks
 * ago; INVALID null/before init. Not a lossless event stream: slow consumers can
 * miss intermediate reports; sequence detects progress. On non-OK cancel any
 * gesture locally; do not interpret an I2C fault as a physical finger release. */
maia_status_t maia_touch_input_snapshot(maia_touch_snapshot_t *);
#ifdef __cplusplus
}
#endif
#endif
