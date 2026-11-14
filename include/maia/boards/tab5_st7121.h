#ifndef MAIA_TAB5_ST7121_H
#define MAIA_TAB5_ST7121_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct maia_st7121_io {
    void *context;
    /* Immutable packet includes command;1..61 bytes. Return0 only after checked
     * bounded transport completion. Failure may have changed panel state. */
    int (*write)(void *,const uint8_t *,uint32_t);
    /* Wait at least requested milliseconds, bounded or fail; never silently
     * shorten. Native backend must detect a stalled timebase. */
    int (*wait_ms)(void *,uint32_t);
} maia_st7121_io_t;
typedef struct maia_st7121_report {
    uint32_t attempted,completed,waited_ms,last_command;
    int callback_result;
} maia_st7121_report_t;
typedef enum maia_st7121_result {
    MAIA_ST7121_OK=0,MAIA_ST7121_INVALID=-1,MAIA_ST7121_UNSUPPORTED=-2,
    MAIA_ST7121_BUSY=-3,MAIA_ST7121_FAILED=-4,MAIA_ST7121_WRITE=-5,MAIA_ST7121_WAIT=-6
} maia_st7121_result_t;
/* One Tab5/exclusive boot, CPU0/IRQs-off; no ISR/tasks/SMP. Caller must prove
 * fresh native hardware reset, stable supply/DSI and identified touch FW1
 * (working BSP's ST7121 selection). Does not prove silicon model itself.
 * Sends software reset120ms then pinned board table with each delay+5ms.
 * All callbacks/storage immutable for entire call; no reentry/concurrency.
 * Private lifetime claim until SoC reboot: duplicate success BUSY; after any attempted
 * write, failure sticks FAILED. No retry/rollback/software reset on failure.
 * Report initialized; attempts set BEFORE callback, completed AFTER its wait.
 * OK means callback-verified command program, NOT ACK/readback/pixels, native
 * video/DPI/framebuffer/PSRAM or general cold power bring-up. No heap/MMIO here. */
maia_st7121_result_t maia_tab5_st7121_program(uint8_t firmware,const maia_st7121_io_t *,maia_st7121_report_t *);
#ifdef __cplusplus
}
#endif
#endif
