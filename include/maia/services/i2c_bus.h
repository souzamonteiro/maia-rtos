/* Original MaiaRTOS MIT. Native core0 task bus, no foreign runtime. */
#ifndef MAIA_I2C_BUS_H
#define MAIA_I2C_BUS_H
#include <maia/kernel.h>
#include <maia/drivers/i2c_bitbang.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct maia_i2c_bus {
    const maia_i2c_lines_t *lines;
    uint32_t initialized,busy,transactions,errors,contentions;
} maia_i2c_bus_t;
/* Zero-initialized stationary storage, core0 boot IRQs-off/pre-scheduler only.
 * Adopts already prepared lines; all later clients use this object exclusively.
 * Raw callbacks/configuration must no longer be accessed by other clients.
 * OK, INVALID missing callbacks/storage, PERM context, BUSY already initialized. */
maia_status_t maia_i2c_bus_init(maia_i2c_bus_t *,const maia_i2c_lines_t *);
/* Core0 task/IRQs-on only, clients stopped before close. Refuses active wire
 * call or non-idle input lines. Closed bus rejects all reads without I/O/output
 * changes; reopen retains the same stationary lines/config and cumulative stats.
 * This quiesces software ownership, not power/pins/clocks or physical recovery.
 * OK; INVALID wrong state/null; PERM context; BUSY active/non-idle. No ISR/SMP.
 * Storage/callbacks/pin reservation stay alive even while closed. */
maia_status_t maia_i2c_bus_close(maia_i2c_bus_t *);
maia_status_t maia_i2c_bus_reopen(maia_i2c_bus_t *);
/* Core0 task/IRQs-on only. Try ownership, never blocks on another client; BUSY
 * on contention/context. Wire operation runs with IRQs enabled, finite waits,
 * releases ownership on every exit. No ISR/SMP/retry/recovery pulses. Same block
 * read atomic output/error contract as bitbang; other callers may retry later. */
maia_i2c_result_t maia_i2c_bus_read16(maia_i2c_bus_t *,uint8_t address,uint16_t reg,uint8_t *,uint32_t);
#ifdef __cplusplus
}
#endif
#endif
