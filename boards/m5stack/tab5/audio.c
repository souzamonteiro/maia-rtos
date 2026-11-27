#include <maia/boards/tab5_audio.h>
#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/soc/esp32p4_sram.h>

#include <stdint.h>

#define ES8388_ADDRESS_7BIT 0x10U
#define ES8388_REG_CONTROL1 0x00U
#define ES8388_REG_CONTROL2 0x01U
#define ES8388_REG_CHIPPOWER 0x02U
#define ES8388_REG_DACPOWER 0x04U
#define ES8388_REG_MASTERMODE 0x08U
#define ES8388_REG_DACCONTROL1 0x17U
#define ES8388_REG_DACCONTROL2 0x18U
#define ES8388_REG_DACCONTROL3 0x19U
#define ES8388_REG_DACCONTROL4 0x1AU
#define ES8388_REG_DACCONTROL5 0x1BU
#define ES8388_REG_DACCONTROL16 0x26U
#define ES8388_REG_DACCONTROL17 0x27U
#define ES8388_REG_DACCONTROL20 0x2AU
#define ES8388_REG_DACCONTROL21 0x2BU
#define ES8388_REG_DACCONTROL23 0x2DU
#define ES8388_REG_DACCONTROL24 0x2EU
#define ES8388_REG_DACCONTROL25 0x2FU
#define ES8388_REG_DACCONTROL26 0x30U
#define ES8388_REG_DACCONTROL27 0x31U

#define PI4IOE1_ADDRESS 0x43U
#define PI4IOE_OUTPUT_REG 0x05U
#define PI4IOE_SPEAKER_ENABLE (1U << 1U)

#define AUDIO_TASK_STACK_BYTES 4096U
#define AUDIO_PROBE_CYCLES 30U
#define AUDIO_ACQUIRE_TIMEOUT_TICKS 1000U
#define AUDIO_TONE_PERIOD_FRAMES 48U
#define AUDIO_TONE_PEAK 8192

typedef struct codec_register {
    uint8_t address;
    uint8_t value;
} codec_register_t;

static const codec_register_t s_es8388_playback_config[] = {
    {ES8388_REG_DACCONTROL3, 0x04U},
    {ES8388_REG_CONTROL2, 0x50U},
    {ES8388_REG_CHIPPOWER, 0x00U},
    {ES8388_REG_MASTERMODE, 0x00U},
    {ES8388_REG_DACPOWER, 0xC0U},
    {ES8388_REG_CONTROL1, 0x12U},
    {ES8388_REG_DACCONTROL1, 0x18U},
    {ES8388_REG_DACCONTROL2, 0x02U},
    {ES8388_REG_DACCONTROL16, 0x00U},
    {ES8388_REG_DACCONTROL17, 0x90U},
    {ES8388_REG_DACCONTROL20, 0x90U},
    {ES8388_REG_DACCONTROL21, 0x80U},
    {ES8388_REG_DACCONTROL23, 0x00U},
    {ES8388_REG_DACCONTROL4, 0x24U},
    {ES8388_REG_DACCONTROL5, 0x24U},
    {ES8388_REG_DACCONTROL24, 0x1EU},
    {ES8388_REG_DACCONTROL25, 0x1EU},
    {ES8388_REG_DACCONTROL26, 0x00U},
    {ES8388_REG_DACCONTROL27, 0x00U},
    {ES8388_REG_DACPOWER, 0x3CU},
};

static uint8_t s_audio_task_stack[AUDIO_TASK_STACK_BYTES]
    MAIA_SRAM_HIGH_BSS __attribute__((aligned(16)));
static maia_i2c_bus_t *s_initialized_bus;

#ifdef MAIA_TAB5_AUDIO_TEST
void maia_tab5_audio_test_reset(void) {
    s_initialized_bus = NULL;
}
#endif

static maia_i2c_result_t bus_read_retry(maia_i2c_bus_t *bus, uint8_t address,
                                        uint8_t reg, uint8_t *value) {
    maia_i2c_result_t result;
    for (unsigned attempt = 0U;; ++attempt) {
        result = maia_i2c_bus_read8(bus, address, reg, value, 1U);
        if (result != MAIA_I2C_BUSY || attempt == 2U) return result;
        maia_task_yield();
    }
}

static maia_i2c_result_t bus_write_retry(maia_i2c_bus_t *bus, uint8_t address,
                                         uint8_t reg, uint8_t value) {
    maia_i2c_result_t result;
    for (unsigned attempt = 0U;; ++attempt) {
        result = maia_i2c_bus_write8(bus, address, reg, value);
        if (result != MAIA_I2C_BUSY || attempt == 2U) return result;
        maia_task_yield();
    }
}

static maia_i2c_result_t set_speaker_enabled(maia_i2c_bus_t *bus, unsigned enabled) {
    uint8_t output;
    maia_i2c_result_t result = bus_read_retry(
        bus, PI4IOE1_ADDRESS, PI4IOE_OUTPUT_REG, &output);
    if (result != MAIA_I2C_OK) return result;

    uint8_t desired = (uint8_t)(enabled ?
        output | PI4IOE_SPEAKER_ENABLE :
        output & (uint8_t)~PI4IOE_SPEAKER_ENABLE);
    if (desired != output) {
        result = bus_write_retry(bus, PI4IOE1_ADDRESS, PI4IOE_OUTPUT_REG, desired);
        if (result != MAIA_I2C_OK) return result;
    }

    result = bus_read_retry(bus, PI4IOE1_ADDRESS, PI4IOE_OUTPUT_REG, &output);
    if (result != MAIA_I2C_OK) return result;
    return (output & PI4IOE_SPEAKER_ENABLE) == (desired & PI4IOE_SPEAKER_ENABLE) ?
        MAIA_I2C_OK : MAIA_I2C_VERIFY;
}

static maia_i2c_result_t es8388_write(maia_i2c_bus_t *bus, uint8_t reg, uint8_t value) {
    return bus_write_retry(bus, ES8388_ADDRESS_7BIT, reg, value);
}

maia_tab5_audio_result_t maia_tab5_audio_init(
    maia_i2c_bus_t *bus,
    maia_i2c_result_t *bus_error,
    maia_tab5_i2s_result_t *i2s_error) {
    if (bus_error != NULL) *bus_error = MAIA_I2C_OK;
    if (i2s_error != NULL) *i2s_error = MAIA_TAB5_I2S_OK;
    if (bus == NULL) return MAIA_TAB5_AUDIO_INVALID;
    if (s_initialized_bus != NULL) {
        return s_initialized_bus == bus ? MAIA_TAB5_AUDIO_OK : MAIA_TAB5_AUDIO_INVALID;
    }

    maia_i2c_result_t result = set_speaker_enabled(bus, 0U);
    if (result != MAIA_I2C_OK) {
        if (bus_error != NULL) *bus_error = result;
        return MAIA_TAB5_AUDIO_I2C_ERROR;
    }

    for (unsigned i = 0U;
         i < sizeof(s_es8388_playback_config) / sizeof(s_es8388_playback_config[0]);
         ++i) {
        result = es8388_write(bus, s_es8388_playback_config[i].address,
                              s_es8388_playback_config[i].value);
        if (result != MAIA_I2C_OK) {
            if (bus_error != NULL) *bus_error = result;
            (void)set_speaker_enabled(bus, 0U);
            return MAIA_TAB5_AUDIO_I2C_ERROR;
        }
    }

    maia_tab5_i2s_result_t tx_result = maia_tab5_i2s_tx_init();
    if (tx_result != MAIA_TAB5_I2S_OK) {
        if (i2s_error != NULL) *i2s_error = tx_result;
        (void)set_speaker_enabled(bus, 0U);
        return MAIA_TAB5_AUDIO_I2S_ERROR;
    }

    result = set_speaker_enabled(bus, 1U);
    if (result != MAIA_I2C_OK) {
        if (bus_error != NULL) *bus_error = result;
        (void)set_speaker_enabled(bus, 0U);
        return MAIA_TAB5_AUDIO_I2C_ERROR;
    }

    result = es8388_write(bus, ES8388_REG_DACCONTROL3, 0x00U);
    if (result != MAIA_I2C_OK) {
        if (bus_error != NULL) *bus_error = result;
        (void)set_speaker_enabled(bus, 0U);
        return MAIA_TAB5_AUDIO_I2C_ERROR;
    }

    s_initialized_bus = bus;
    return MAIA_TAB5_AUDIO_OK;
}

static maia_tab5_audio_result_t acquire_buffer(unsigned *index, int16_t **samples,
                                                maia_tab5_i2s_result_t *i2s_error) {
    for (unsigned tick = 0U; tick < AUDIO_ACQUIRE_TIMEOUT_TICKS; ++tick) {
        maia_tab5_i2s_result_t result = maia_tab5_i2s_tx_acquire(index, samples);
        if (result == MAIA_TAB5_I2S_OK) return MAIA_TAB5_AUDIO_OK;
        if (result != MAIA_TAB5_I2S_BUSY) {
            if (i2s_error != NULL) *i2s_error = result;
            return MAIA_TAB5_AUDIO_I2S_ERROR;
        }
        maia_task_delay_ticks(1U);
    }
    if (i2s_error != NULL) *i2s_error = MAIA_TAB5_I2S_TIMEOUT;
    return MAIA_TAB5_AUDIO_TIMEOUT;
}

static int16_t tone_sample(uint32_t frame) {
    unsigned phase = frame % AUDIO_TONE_PERIOD_FRAMES;
    int32_t sample;
    if (phase < 12U) {
        sample = (int32_t)phase * AUDIO_TONE_PEAK / 12;
    } else if (phase < 24U) {
        sample = (int32_t)(24U - phase) * AUDIO_TONE_PEAK / 12;
    } else if (phase < 36U) {
        sample = -(int32_t)(phase - 24U) * AUDIO_TONE_PEAK / 12;
    } else {
        sample = -(int32_t)(48U - phase) * AUDIO_TONE_PEAK / 12;
    }

    int32_t envelope = AUDIO_TONE_PEAK;
    uint32_t total_frames = AUDIO_PROBE_CYCLES * MAIA_TAB5_I2S_TX_BUFFER_FRAMES;
    if (frame < AUDIO_TONE_PERIOD_FRAMES) {
        envelope = envelope * (int32_t)frame / AUDIO_TONE_PERIOD_FRAMES;
    } else if (frame >= total_frames - AUDIO_TONE_PERIOD_FRAMES) {
        envelope = envelope * (int32_t)(total_frames - frame) / AUDIO_TONE_PERIOD_FRAMES;
    }
    return (int16_t)(sample * envelope / AUDIO_TONE_PEAK);
}

maia_tab5_audio_result_t maia_tab5_audio_probe_tone(
    maia_tab5_i2s_result_t *i2s_error) {
    if (i2s_error != NULL) *i2s_error = MAIA_TAB5_I2S_OK;
    if (s_initialized_bus == NULL) return MAIA_TAB5_AUDIO_NOT_INITIALIZED;

    uint32_t total_frames = AUDIO_PROBE_CYCLES * MAIA_TAB5_I2S_TX_BUFFER_FRAMES;
    for (uint32_t offset = 0U; offset < total_frames;
         offset += MAIA_TAB5_I2S_TX_BUFFER_FRAMES) {
        unsigned index;
        int16_t *samples;
        maia_tab5_audio_result_t result = acquire_buffer(&index, &samples, i2s_error);
        if (result != MAIA_TAB5_AUDIO_OK) {
            (void)maia_tab5_i2s_tx_stop();
            return result;
        }
        for (uint32_t frame = 0U; frame < MAIA_TAB5_I2S_TX_BUFFER_FRAMES; ++frame) {
            int16_t sample = tone_sample(offset + frame);
            samples[frame * 2U] = sample;
            samples[frame * 2U + 1U] = sample;
        }
        maia_tab5_i2s_result_t tx_result =
            maia_tab5_i2s_tx_submit(index, MAIA_TAB5_I2S_TX_BUFFER_FRAMES);
        if (tx_result != MAIA_TAB5_I2S_OK) {
            if (i2s_error != NULL) *i2s_error = tx_result;
            (void)maia_tab5_i2s_tx_stop();
            return MAIA_TAB5_AUDIO_I2S_ERROR;
        }
    }

    unsigned completed_mask = 0U;
    while (completed_mask != ((1U << MAIA_TAB5_I2S_TX_BUFFER_COUNT) - 1U)) {
        unsigned index;
        int16_t *samples;
        maia_tab5_audio_result_t result = acquire_buffer(&index, &samples, i2s_error);
        if (result != MAIA_TAB5_AUDIO_OK) {
            (void)maia_tab5_i2s_tx_stop();
            return result;
        }
        completed_mask |= 1U << index;
    }

    maia_tab5_i2s_result_t tx_result = maia_tab5_i2s_tx_stop();
    if (tx_result != MAIA_TAB5_I2S_OK) {
        if (i2s_error != NULL) *i2s_error = tx_result;
        return MAIA_TAB5_AUDIO_I2S_ERROR;
    }
    return MAIA_TAB5_AUDIO_OK;
}

static void print_unsigned(uint32_t value) {
    char digits[10];
    unsigned count = 0U;
    do {
        digits[count++] = (char)('0' + value % 10U);
        value /= 10U;
    } while (value != 0U && count < sizeof(digits));
    while (count != 0U) {
        char text[2] = {digits[--count], '\0'};
        maia_console_puts(text);
    }
}

static void print_error_code(const char *prefix, int32_t code) {
    maia_console_puts(prefix);
    if (code < 0) {
        maia_console_puts("-");
        print_unsigned(0U - (uint32_t)code);
    } else {
        print_unsigned((uint32_t)code);
    }
    maia_console_puts("\n");
}

static void audio_init_task(void *arg) {
    maia_i2c_result_t bus_error = MAIA_I2C_OK;
    maia_tab5_i2s_result_t i2s_error = MAIA_TAB5_I2S_OK;
    maia_tab5_audio_result_t result =
        maia_tab5_audio_init((maia_i2c_bus_t *)arg, &bus_error, &i2s_error);
    if (result != MAIA_TAB5_AUDIO_OK) {
        print_error_code("[AUDIO] Init failed, stage=", result);
        if (bus_error != MAIA_I2C_OK) {
            print_error_code("[AUDIO] I2C error=", bus_error);
        }
        if (i2s_error != MAIA_TAB5_I2S_OK) {
            print_error_code("[AUDIO] I2S error=", i2s_error);
        }
        return;
    }

    maia_console_puts("[AUDIO] ES8388 playback and speaker amp ready\n");
#ifdef MAIA_DIAGNOSTIC_AUDIO_PROBE
    result = maia_tab5_audio_probe_tone(&i2s_error);
    if (result == MAIA_TAB5_AUDIO_OK) {
        maia_console_puts("[AUDIO] 1kHz probe tone completed\n");
    } else {
        print_error_code("[AUDIO] Probe failed, stage=", result);
        if (i2s_error != MAIA_TAB5_I2S_OK) {
            print_error_code("[AUDIO] I2S error=", i2s_error);
        }
    }
#endif
}

maia_status_t maia_tab5_audio_start(maia_i2c_bus_t *bus) {
    if (bus == NULL) return MAIA_ERR_INVALID;
    maia_task_id_t task_id = -1;
    return maia_task_create("tab5_audio", audio_init_task, bus, MAIA_PRIO_NORMAL,
                            s_audio_task_stack, sizeof(s_audio_task_stack), &task_id);
}
