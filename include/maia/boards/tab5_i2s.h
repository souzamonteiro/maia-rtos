#ifndef MAIA_BOARDS_TAB5_I2S_H
#define MAIA_BOARDS_TAB5_I2S_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* TX buffers contain interleaved signed 16-bit stereo frames. */
#define MAIA_TAB5_I2S_TX_SAMPLE_RATE_HZ 48000U
#define MAIA_TAB5_I2S_TX_BUFFER_COUNT 2U
#define MAIA_TAB5_I2S_TX_BUFFER_FRAMES 480U

typedef enum {
    MAIA_TAB5_I2S_OK = 0,
    MAIA_TAB5_I2S_INVALID,
    MAIA_TAB5_I2S_BUSY,
    MAIA_TAB5_I2S_NOT_INITIALIZED,
    MAIA_TAB5_I2S_TIMEOUT,
    MAIA_TAB5_I2S_DMA_MEMORY_ERROR
} maia_tab5_i2s_result_t;

maia_tab5_i2s_result_t maia_tab5_i2s_tx_init(void);
maia_tab5_i2s_result_t maia_tab5_i2s_tx_acquire(unsigned *buffer_index, int16_t **samples);
maia_tab5_i2s_result_t maia_tab5_i2s_tx_submit(unsigned buffer_index, uint32_t frames);
maia_tab5_i2s_result_t maia_tab5_i2s_tx_stop(void);

#ifdef __cplusplus
}
#endif

#endif
