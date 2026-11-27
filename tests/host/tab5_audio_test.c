#include <assert.h>
#include <string.h>

#include <maia/boards/tab5_audio.h>

#define ES8388_ADDRESS_7BIT 0x10U
#define PI4IOE1_ADDRESS 0x43U
#define PI4IOE_OUTPUT_REG 0x05U
#define SPEAKER_ENABLE (1U << 1U)
#define MAX_WRITES 64U

void maia_tab5_audio_test_reset(void);

static maia_i2c_bus_t s_bus;
static maia_i2c_bus_t s_other_bus;
static uint8_t s_expander_output;
static uint8_t s_codec_registers[0x40];
static uint8_t s_write_address[MAX_WRITES];
static uint8_t s_write_register[MAX_WRITES];
static uint8_t s_write_value[MAX_WRITES];
static unsigned s_write_count;
static unsigned s_busy_writes;
static unsigned s_i2s_init_count;
static unsigned s_acquire_count;
static unsigned s_submit_count;
static unsigned s_stop_count;
static unsigned s_delay_count;
static unsigned s_console_count;
static unsigned s_task_count;
static unsigned s_task_priority;
static void *s_task_stack;
static size_t s_task_stack_size;
static maia_task_entry_fn s_task_entry;
static maia_i2c_result_t s_fail_codec_write;
static uint8_t s_fail_codec_reg;
static maia_tab5_i2s_result_t s_i2s_init_result;
static maia_tab5_i2s_result_t s_i2s_stop_result;
static unsigned s_always_busy_acquire;
static int16_t s_samples[MAIA_TAB5_I2S_TX_BUFFER_COUNT]
                         [MAIA_TAB5_I2S_TX_BUFFER_FRAMES * 2U];
static int16_t s_first_buffer_samples[MAIA_TAB5_I2S_TX_BUFFER_FRAMES * 2U];

maia_i2c_result_t maia_i2c_bus_read8(maia_i2c_bus_t *bus, uint8_t address,
                                     uint8_t reg, uint8_t *out, uint32_t count) {
    assert(bus == &s_bus && out != NULL && count == 1U);
    if (address == PI4IOE1_ADDRESS && reg == PI4IOE_OUTPUT_REG) {
        *out = s_expander_output;
        return MAIA_I2C_OK;
    }
    assert(0);
    return MAIA_I2C_INVALID;
}

maia_i2c_result_t maia_i2c_bus_write8(maia_i2c_bus_t *bus, uint8_t address,
                                      uint8_t reg, uint8_t value) {
    assert(bus == &s_bus);
    if (s_busy_writes != 0U) {
        --s_busy_writes;
        return MAIA_I2C_BUSY;
    }
    if (address == ES8388_ADDRESS_7BIT && reg == s_fail_codec_reg &&
        s_fail_codec_write != MAIA_I2C_OK) {
        maia_i2c_result_t result = s_fail_codec_write;
        s_fail_codec_write = MAIA_I2C_OK;
        return result;
    }
    assert(s_write_count < MAX_WRITES);
    s_write_address[s_write_count] = address;
    s_write_register[s_write_count] = reg;
    s_write_value[s_write_count] = value;
    ++s_write_count;
    if (address == PI4IOE1_ADDRESS && reg == PI4IOE_OUTPUT_REG) {
        s_expander_output = value;
    } else if (address == ES8388_ADDRESS_7BIT && reg < sizeof(s_codec_registers)) {
        s_codec_registers[reg] = value;
    } else {
        assert(0);
    }
    return MAIA_I2C_OK;
}

maia_tab5_i2s_result_t maia_tab5_i2s_tx_init(void) {
    ++s_i2s_init_count;
    return s_i2s_init_result;
}

maia_tab5_i2s_result_t maia_tab5_i2s_tx_acquire(unsigned *index, int16_t **samples) {
    if (s_always_busy_acquire != 0U) return MAIA_TAB5_I2S_BUSY;
    *index = s_acquire_count++ % MAIA_TAB5_I2S_TX_BUFFER_COUNT;
    *samples = s_samples[*index];
    return MAIA_TAB5_I2S_OK;
}

maia_tab5_i2s_result_t maia_tab5_i2s_tx_submit(unsigned index, uint32_t frames) {
    assert(index < MAIA_TAB5_I2S_TX_BUFFER_COUNT);
    assert(frames == MAIA_TAB5_I2S_TX_BUFFER_FRAMES);
    if (s_submit_count == 0U) {
        memcpy(s_first_buffer_samples, s_samples[index], sizeof(s_first_buffer_samples));
    }
    ++s_submit_count;
    return MAIA_TAB5_I2S_OK;
}

maia_tab5_i2s_result_t maia_tab5_i2s_tx_stop(void) {
    ++s_stop_count;
    return s_i2s_stop_result;
}

maia_status_t maia_task_create(const char *name, maia_task_entry_fn entry, void *arg,
                               uint8_t priority, void *stack, size_t stack_size,
                               maia_task_id_t *id) {
    assert(strcmp(name, "tab5_audio") == 0 && entry != NULL && arg == &s_bus);
    assert(id != NULL);
    s_task_entry = entry;
    s_task_priority = priority;
    s_task_stack = stack;
    s_task_stack_size = stack_size;
    ++s_task_count;
    *id = 7;
    return MAIA_OK;
}

void maia_task_yield(void) {}
void maia_task_delay_ticks(uint32_t ticks) {
    assert(ticks == 1U);
    ++s_delay_count;
}
void maia_console_puts(const char *text) {
    assert(text != NULL);
    ++s_console_count;
}

static void reset_fakes(void) {
    maia_tab5_audio_test_reset();
    memset(&s_bus, 0, sizeof(s_bus));
    memset(&s_other_bus, 0, sizeof(s_other_bus));
    memset(s_codec_registers, 0, sizeof(s_codec_registers));
    memset(s_write_address, 0, sizeof(s_write_address));
    memset(s_write_register, 0, sizeof(s_write_register));
    memset(s_write_value, 0, sizeof(s_write_value));
    memset(s_samples, 0, sizeof(s_samples));
    memset(s_first_buffer_samples, 0, sizeof(s_first_buffer_samples));
    s_expander_output = 0x66U;
    s_write_count = s_busy_writes = s_i2s_init_count = 0U;
    s_acquire_count = s_submit_count = s_stop_count = s_delay_count = 0U;
    s_console_count = s_task_count = s_task_priority = 0U;
    s_task_stack = NULL;
    s_task_stack_size = 0U;
    s_task_entry = NULL;
    s_fail_codec_write = MAIA_I2C_OK;
    s_fail_codec_reg = 0U;
    s_i2s_init_result = MAIA_TAB5_I2S_OK;
    s_i2s_stop_result = MAIA_TAB5_I2S_OK;
    s_always_busy_acquire = 0U;
}

static void test_codec_amp_initialization(void) {
    reset_fakes();
    maia_i2c_result_t bus_error = MAIA_I2C_INVALID;
    maia_tab5_i2s_result_t i2s_error = MAIA_TAB5_I2S_INVALID;
    s_busy_writes = 1U;
    assert(maia_tab5_audio_init(&s_bus, &bus_error, &i2s_error) == MAIA_TAB5_AUDIO_OK);
    assert(bus_error == MAIA_I2C_OK && i2s_error == MAIA_TAB5_I2S_OK);
    assert(s_expander_output == 0x66U);
    assert(s_i2s_init_count == 1U);
    assert(s_codec_registers[0x19U] == 0x00U);
    assert(s_codec_registers[0x17U] == 0x18U);
    assert(s_codec_registers[0x18U] == 0x02U);
    assert(s_codec_registers[0x1AU] == 0x24U);
    assert(s_codec_registers[0x1BU] == 0x24U);
    assert(s_write_count == 23U);
    assert(s_write_address[0] == PI4IOE1_ADDRESS &&
           s_write_register[0] == PI4IOE_OUTPUT_REG &&
           (s_write_value[0] & SPEAKER_ENABLE) == 0U);
    assert(s_write_address[s_write_count - 1U] == ES8388_ADDRESS_7BIT &&
           s_write_register[s_write_count - 1U] == 0x19U &&
           s_write_value[s_write_count - 1U] == 0x00U);
    for (unsigned i = 0U; i < s_write_count; ++i) {
        assert(s_write_address[i] == PI4IOE1_ADDRESS ||
               s_write_address[i] == ES8388_ADDRESS_7BIT);
    }
    assert(maia_tab5_audio_init(&s_bus, &bus_error, &i2s_error) == MAIA_TAB5_AUDIO_OK);
    assert(maia_tab5_audio_init(&s_other_bus, &bus_error, &i2s_error) ==
           MAIA_TAB5_AUDIO_INVALID);
    assert(s_i2s_init_count == 1U && s_write_count == 23U);
}

static void test_initialization_errors_leave_amp_off(void) {
    maia_i2c_result_t bus_error;
    maia_tab5_i2s_result_t i2s_error;
    s_fail_codec_reg = 0x17U;
    s_fail_codec_write = MAIA_I2C_NACK;
    assert(maia_tab5_audio_init(&s_bus, &bus_error, &i2s_error) ==
           MAIA_TAB5_AUDIO_I2C_ERROR);
    assert(bus_error == MAIA_I2C_NACK && i2s_error == MAIA_TAB5_I2S_OK);
    assert((s_expander_output & SPEAKER_ENABLE) == 0U);
    assert(s_i2s_init_count == 0U);

    reset_fakes();
    s_i2s_init_result = MAIA_TAB5_I2S_DMA_MEMORY_ERROR;
    assert(maia_tab5_audio_init(&s_bus, &bus_error, &i2s_error) ==
           MAIA_TAB5_AUDIO_I2S_ERROR);
    assert(bus_error == MAIA_I2C_OK);
    assert(i2s_error == MAIA_TAB5_I2S_DMA_MEMORY_ERROR);
    assert((s_expander_output & SPEAKER_ENABLE) == 0U);
}

static void test_bounded_probe_tone(void) {
    maia_i2c_result_t bus_error;
    maia_tab5_i2s_result_t i2s_error;
    reset_fakes();
    assert(maia_tab5_audio_init(&s_bus, &bus_error, &i2s_error) == MAIA_TAB5_AUDIO_OK);
    assert(maia_tab5_audio_probe_tone(&i2s_error) == MAIA_TAB5_AUDIO_OK);
    assert(i2s_error == MAIA_TAB5_I2S_OK);
    assert(s_submit_count == 30U && s_stop_count == 1U);
    assert(s_first_buffer_samples[0] == 0);
    assert(s_first_buffer_samples[12U * 2U] > 0);
    assert(s_first_buffer_samples[24U * 2U] == 0);
    assert(s_first_buffer_samples[36U * 2U] < 0);
    for (unsigned frame = 0U; frame < MAIA_TAB5_I2S_TX_BUFFER_FRAMES; ++frame) {
        assert(s_first_buffer_samples[frame * 2U] ==
               s_first_buffer_samples[frame * 2U + 1U]);
    }

    reset_fakes();
    assert(maia_tab5_audio_init(&s_bus, &bus_error, &i2s_error) == MAIA_TAB5_AUDIO_OK);
    s_always_busy_acquire = 1U;
    assert(maia_tab5_audio_probe_tone(&i2s_error) == MAIA_TAB5_AUDIO_TIMEOUT);
    assert(i2s_error == MAIA_TAB5_I2S_TIMEOUT);
    assert(s_delay_count == 1000U && s_submit_count == 0U && s_stop_count == 1U);
}

static void test_async_start(void) {
    reset_fakes();
    assert(maia_tab5_audio_start(&s_bus) == MAIA_OK);
    assert(s_task_count == 1U && s_task_entry != NULL);
    assert(s_task_priority == MAIA_PRIO_NORMAL);
    assert(s_task_stack != NULL && s_task_stack_size == 4096U);
    s_task_entry(&s_bus);
    assert(s_console_count == 1U);
    assert(maia_tab5_audio_start(NULL) == MAIA_ERR_INVALID);
}

int main(void) {
    assert(maia_tab5_audio_probe_tone(NULL) == MAIA_TAB5_AUDIO_NOT_INITIALIZED);
    assert(maia_tab5_audio_init(NULL, NULL, NULL) == MAIA_TAB5_AUDIO_INVALID);
    reset_fakes();
    test_initialization_errors_leave_amp_off();
    test_codec_amp_initialization();
    test_bounded_probe_tone();
    test_async_start();
    return 0;
}
