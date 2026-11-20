/* Native Tab5 product touch owner and system-I2C handoff. */
#include "product_touch.h"
#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/services/i2c_bus.h>
#include <maia/services/touch_input.h>
#include <maia/soc/esp32p4_board_io.h>

#define TOUCH_ADDRESS 0x55U

static maia_i2c_bus_t system_bus;
static uint8_t touch_stack[4096] __attribute__((aligned(16)));
static unsigned touch_active;
static uint32_t previous_sequence;
static unsigned poll_failed;

static maia_i2c_result_t read_boot(void *context, uint16_t reg, uint8_t *out, uint32_t count) {
    const maia_i2c_lines_t *const *lines = context;
    return maia_i2c_read_register16_block(*lines, TOUCH_ADDRESS, reg, out, count);
}

static maia_i2c_result_t read_task(void *context, uint16_t reg, uint8_t *out, uint32_t count) {
    return maia_i2c_bus_read16(context, TOUCH_ADDRESS, reg, out, count);
}

static void touch_poll_once(void) {
    if (!touch_active) return;
    maia_status_t result = maia_touch_input_poll();
    if (result != MAIA_OK && !poll_failed) {
        maia_console_puts("[TOUCH] Poll failed; input invalidated\n");
        poll_failed = 1U;
    } else if (result == MAIA_OK && poll_failed) {
        maia_console_puts("[TOUCH] Poll recovered; awaiting fresh report\n");
        poll_failed = 0U;
    }
    maia_touch_snapshot_t snapshot;
    (void)maia_touch_input_snapshot(&snapshot);
    if (snapshot.sequence != previous_sequence) {
        previous_sequence = snapshot.sequence;
        maia_product_display_touch_changed();
    }
}

static void touch_task(void *arg) {
    (void)arg;
    for (;;) {
        touch_poll_once();
        maia_task_delay_ticks(5U);
    }
}

void maia_tab5_product_touch_start(const maia_i2c_lines_t *lines) {
    maia_st712x_info_t info;
    maia_i2c_result_t identified = maia_st712x_identify(read_boot, &lines, &info);
    maia_status_t result = maia_i2c_bus_init(&system_bus, lines);
    if (result != MAIA_OK) {
        maia_console_puts("[TOUCH] System I2C handoff failed; touch disabled\n");
        return;
    }
    maia_tab5_system_i2c_adopt(&system_bus);
    if (identified != MAIA_I2C_OK || info.firmware != 1U ||
        info.max_x != 720U || info.max_y != 1280U) {
        maia_console_puts("[TOUCH] ST7121 identification failed; touch disabled\n");
        return;
    }
    maia_task_id_t owner = -1;
    result = maia_task_create("touch", touch_task, NULL, MAIA_PRIO_NORMAL,
                              touch_stack, sizeof(touch_stack), &owner);
    if (result != MAIA_OK) {
        maia_console_puts("[TOUCH] Task creation failed; touch disabled\n");
        return;
    }
    result = maia_touch_input_init(owner, read_task, &system_bus, &info, 720U, 1280U, 90U);
    if (result != MAIA_OK) {
        maia_console_puts("[TOUCH] Service initialization failed; touch disabled\n");
        return;
    }
    touch_active = 1U;
    maia_console_puts("[TOUCH] Native ST7121 service ready\n");
}
