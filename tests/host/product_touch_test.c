#include <assert.h>
#include <string.h>

#include "../../boards/m5stack/tab5/product_touch.c"

static maia_i2c_result_t identify_result;
static maia_status_t bus_result, task_result, init_result, poll_result;
static maia_st712x_info_t identity = {720, 1280, 1, 10};
static unsigned adopted, audio_started, created, initialized, polls, redraws, logs, reads;
static uint32_t sequence;
static maia_i2c_lines_t lines;
static unsigned bound;

static void record_bind(maia_i2c_bus_t *bus, unsigned bit) {
    assert(bus == &system_bus && adopted == 1 && !(bound & bit));
    bound |= bit;
}
void maia_tab5_ina226_bind(maia_i2c_bus_t *bus) { record_bind(bus, 1U << 0); }
void maia_tab5_bmi270_bind(maia_i2c_bus_t *bus) { record_bind(bus, 1U << 1); }
void maia_tab5_bmi270_driver_bind(maia_i2c_bus_t *bus) { record_bind(bus, 1U << 2); }
void maia_tab5_battery_bind(maia_i2c_bus_t *bus) { record_bind(bus, 1U << 3); }
void maia_tab5_power_bind(maia_i2c_bus_t *bus) { record_bind(bus, 1U << 4); }
void maia_tab5_rtc_bind(maia_i2c_bus_t *bus) { record_bind(bus, 1U << 5); }
maia_status_t maia_tab5_audio_start(maia_i2c_bus_t *bus) {
    assert(bus == &system_bus && adopted == 1);
    ++audio_started;
    return MAIA_OK;
}

void maia_console_puts(const char *s) { assert(s); ++logs; }
void maia_product_display_touch_changed(void) { ++redraws; }
void maia_tab5_system_i2c_adopt(maia_i2c_bus_t *bus) { assert(bus == &system_bus); ++adopted; }
maia_i2c_result_t maia_i2c_read_register16_block(const maia_i2c_lines_t *bus,
    uint8_t address, uint16_t reg, uint8_t *out, uint32_t count) {
    assert(bus == &lines && address == 0x55 && reg == 1 && count == 1);
    ++reads; *out = 0; return MAIA_I2C_OK;
}
maia_i2c_result_t maia_i2c_bus_read16(maia_i2c_bus_t *bus,
    uint8_t address, uint16_t reg, uint8_t *out, uint32_t count) {
    assert(bus == &system_bus && address == 0x55 && reg == 1 && count == 1);
    ++reads; *out = 0; return MAIA_I2C_OK;
}
maia_i2c_result_t maia_st712x_identify(maia_touch_read_fn reader, void *ctx, maia_st712x_info_t *out) {
    uint8_t value;
    assert(reader(ctx, 1, &value, 1) == MAIA_I2C_OK);
    if (identify_result == MAIA_I2C_OK) *out = identity;
    return identify_result;
}
maia_status_t maia_i2c_bus_init(maia_i2c_bus_t *bus, const maia_i2c_lines_t *actual) {
    assert(bus == &system_bus && actual == &lines);
    return bus_result;
}
maia_status_t maia_task_create(const char *name, maia_task_entry_fn entry, void *arg,
    uint8_t priority, void *stack, size_t size, maia_task_id_t *id) {
    assert(!strcmp(name, "touch") && entry == touch_task && !arg);
    assert(priority == MAIA_PRIO_NORMAL && stack == touch_stack && size == sizeof touch_stack);
    ++created; *id = 3; return task_result;
}
maia_status_t maia_touch_input_init(maia_task_id_t owner, maia_touch_read_fn reader, void *ctx,
    const maia_st712x_info_t *info, uint16_t width, uint16_t height, uint16_t rotation) {
    uint8_t value;
    assert(owner == 3 && info->firmware == 1 && width == 720 && height == 1280 && rotation == 90);
    assert(reader(ctx, 1, &value, 1) == MAIA_I2C_OK);
    ++initialized; return init_result;
}
maia_status_t maia_touch_input_poll(void) { ++polls; return poll_result; }
maia_status_t maia_touch_input_snapshot(maia_touch_snapshot_t *out) {
    memset(out, 0, sizeof *out); out->sequence = sequence; return MAIA_ERR_BUSY;
}
void maia_task_delay_ticks(uint32_t ticks) { (void)ticks; }

static void reset(void) {
    memset(&system_bus, 0, sizeof system_bus);
    touch_active = poll_failed = previous_sequence = 0;
    identify_result = MAIA_I2C_OK;
    bus_result = task_result = init_result = poll_result = MAIA_OK;
    identity = (maia_st712x_info_t){720, 1280, 1, 10};
    adopted = audio_started = created = initialized = polls = redraws = logs = reads = sequence = 0;
    bound = 0;
}

int main(void) {
    reset(); maia_tab5_product_touch_start(&lines);
    assert(bound == 0x3fU);
    assert(adopted == 1 && audio_started == 1 && created == 1 &&
           initialized == 1 && touch_active && reads == 2);
    touch_poll_once(); assert(polls == 1 && redraws == 0);
    sequence = 1; touch_poll_once(); assert(redraws == 1);
    poll_result = MAIA_ERR_BUSY; sequence = 2; touch_poll_once();
    assert(poll_failed && redraws == 2 && logs == 2);
    touch_poll_once(); assert(logs == 2 && redraws == 2);
    poll_result = MAIA_OK; touch_poll_once(); assert(!poll_failed && logs == 3);

    reset(); identify_result = MAIA_I2C_TIMEOUT; maia_tab5_product_touch_start(&lines);
    assert(bound == 0x3fU);
    assert(adopted == 1 && audio_started == 1 && !created && !touch_active && logs == 1);
    reset(); identity.firmware = 3; maia_tab5_product_touch_start(&lines);
    assert(adopted == 1 && audio_started == 1 && !created && !touch_active);
    reset(); identity.max_x = 719; maia_tab5_product_touch_start(&lines);
    assert(adopted == 1 && audio_started == 1 && !created && !touch_active);
    reset(); bus_result = MAIA_ERR_BUSY; maia_tab5_product_touch_start(&lines);
    assert(bound == 0);
    assert(!adopted && !audio_started && !created && !touch_active && logs == 1);
    reset(); task_result = MAIA_ERR_FULL; maia_tab5_product_touch_start(&lines);
    assert(adopted == 1 && audio_started == 1 && created == 1 &&
           !initialized && !touch_active);
    reset(); init_result = MAIA_ERR_INVALID; maia_tab5_product_touch_start(&lines);
    assert(adopted == 1 && audio_started == 1 && initialized == 1 && !touch_active);
    touch_poll_once(); assert(!polls);
}
