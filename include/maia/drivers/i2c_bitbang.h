#ifndef MAIA_DRIVERS_I2C_BITBANG_H
#define MAIA_DRIVERS_I2C_BITBANG_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif

typedef enum maia_i2c_result {
    MAIA_I2C_OK = 0,
    MAIA_I2C_INVALID = -1,
    MAIA_I2C_NACK = -2,
    MAIA_I2C_TIMEOUT = -3,
    MAIA_I2C_BUSY = -4
} maia_i2c_result_t;

/* Electrical line masks; release=1 means high impedance, never drive high. */
#define MAIA_I2C_SDA 1U
#define MAIA_I2C_SCL 2U
#define MAIA_I2C_BOTH 3U
/* Caller exclusively owns bus AND callbacks, single master, thread/boot only.
 * No lock, heap, IRQ masking, arbitration or bus recovery is provided.
 * Callbacks must return in bounded time; pause waits at least 5us or returns
 * TIMEOUT if its timebase fails. drive changes the requested mask only;
 * sample returns actual input levels. Context may be NULL if callbacks allow it.
 * No concurrent use or ISR calls. This bus description is immutable in use. */
typedef struct maia_i2c_lines {
    void *context;
    void (*drive)(void *context, unsigned mask, unsigned release);
    unsigned (*sample)(void *context);
    maia_i2c_result_t (*pause)(void *context);
} maia_i2c_lines_t;

/* Address range 0x08..0x77; reserved addresses rejected before touching lines.
 * Up to 40 five-us polls per released-SCL wait, then TIMEOUT. All failures
 * release both lines; NACK attempts STOP, whose failure overrides NACK.
 * BUSY means SDA low before START (no recovery pulses are sent).
 * A valid call may change device's register pointer, never register contents.
 * This read uses STOP between address-pointer write and read, per touch protocol.
 * out changes only after successful read AND STOP. */
maia_i2c_result_t maia_i2c_probe_address(const maia_i2c_lines_t *bus, uint8_t address);
maia_i2c_result_t maia_i2c_read_register16(const maia_i2c_lines_t *bus,
                                        uint8_t address, uint16_t reg, uint8_t *out);
#ifdef __cplusplus
}
#endif
#endif
