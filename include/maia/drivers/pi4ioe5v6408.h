#ifndef MAIA_DRIVERS_PI4IOE5V6408_H
#define MAIA_DRIVERS_PI4IOE5V6408_H
#include <maia/drivers/i2c_bitbang.h>
#ifdef __cplusplus
extern "C" {
#endif
#define MAIA_PI4IO_DIRECTION 0x03U
#define MAIA_PI4IO_OUTPUT 0x05U
#define MAIA_PI4IO_HIGH_Z 0x07U
#define MAIA_PI4IO_PULL_ENABLE 0x0BU
#define MAIA_PI4IO_PULL_SELECT 0x0DU
/* Native subset of M5Unified's expander configuration model. Address43/44 only.
 * Caller selects a known expander; this subset does not identify the silicon.
 * Excludes ID/reset, status/IRQ and reserved registers (read side effects).
 * Caller owns the whole read-modify-write-readback exclusively; no concurrency,
 * ISR calls, heap, hidden locks or board power policy. With boot GPIO backend,
 * all calls are pre-scheduler/core0/IRQs-off only. out changes only on success. */
maia_i2c_result_t maia_pi4io_read_config(const maia_i2c_lines_t *bus,
                                       uint8_t address, uint8_t reg, uint8_t *out);
/* value contains only mask bits. Unmasked bits preserved from fresh read.
 * Unchanged desired byte skips write. Changed byte is written then read back;
 * full-byte mismatch returns VERIFY, bus errors propagate unchanged.
 * Any failure may leave hardware changed. No automatic rollback/retry. */
maia_i2c_result_t maia_pi4io_update_config(const maia_i2c_lines_t *bus,
                                         uint8_t address, uint8_t reg,
                                         uint8_t mask, uint8_t value);
#ifdef __cplusplus
}
#endif
#endif
