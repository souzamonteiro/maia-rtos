#ifndef MAIA_BOARDS_TAB5_AUDIO_H
#define MAIA_BOARDS_TAB5_AUDIO_H

#include <maia/boards/tab5_i2s.h>
#include <maia/services/i2c_bus.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MAIA_TAB5_AUDIO_OK = 0,
    MAIA_TAB5_AUDIO_INVALID,
    MAIA_TAB5_AUDIO_I2C_ERROR,
    MAIA_TAB5_AUDIO_I2S_ERROR,
    MAIA_TAB5_AUDIO_NOT_INITIALIZED,
    MAIA_TAB5_AUDIO_TIMEOUT
} maia_tab5_audio_result_t;

/* Core-0 task context only. Error outputs may be NULL. */
maia_tab5_audio_result_t maia_tab5_audio_init(
    maia_i2c_bus_t *bus,
    maia_i2c_result_t *bus_error,
    maia_tab5_i2s_result_t *i2s_error);

/* Plays a bounded 300ms, low-level 1kHz tone and stops I2S TX. */
maia_tab5_audio_result_t maia_tab5_audio_probe_tone(
    maia_tab5_i2s_result_t *i2s_error);

/* Starts the product audio initialization task; it never blocks the caller. */
maia_status_t maia_tab5_audio_start(maia_i2c_bus_t *bus);

#ifdef __cplusplus
}
#endif

#endif
