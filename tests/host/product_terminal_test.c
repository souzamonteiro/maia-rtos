#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "../../soc/espressif/esp32p4/soc_display_product.c"

static uintptr_t irq_state = 8U;
static maia_touch_snapshot_t touch_snapshot;
static maia_status_t touch_status = MAIA_ERR_BUSY;

maia_status_t maia_touch_input_snapshot(maia_touch_snapshot_t *out) {
    *out = touch_snapshot;
    return touch_status;
}

uintptr_t maia_arch_irq_save(void) {
    uintptr_t saved = irq_state;
    irq_state = 0U;
    return saved;
}

void maia_arch_irq_restore(uintptr_t saved) {
    irq_state = saved;
}

int main(void) {
    static uint16_t frames[2][TAB5_PHYS_W * TAB5_PHYS_H + 2];
    const uint16_t poison = 0xA55AU;
    for (unsigned frame = 0; frame < 2U; ++frame) {
        for (size_t i = 0; i < sizeof frames[frame] / sizeof frames[frame][0]; ++i) {
            frames[frame][i] = poison;
        }
    }
    for (int row = 0; row < (int)TERM_ROWS; ++row) {
        for (int col = 0; col < (int)TERM_COLS; ++col) {
            s_term_lines[row][col] = ' ';
        }
    }
    for (unsigned update = 0; update < 4U; ++update) {
        unsigned frame = update % 2U;
        term_putc('A');
        lcd_draw(&frames[frame][1], TAB5_PHYS_W, TAB5_PHYS_H, NULL);
        assert(frames[frame][0] == poison);
        assert(frames[frame][TAB5_PHYS_W * TAB5_PHYS_H + 1U] == poison);
        for (int y = 0; y < TAB5_LOG_H; ++y) {
            for (int x = 0; x < TAB5_LOG_W; ++x) {
                uint16_t pixel = frames[frame][1 + (TAB5_LOG_W - 1 - x) * TAB5_PHYS_W + y];
                assert(pixel != poison);
                if (y >= (int)TERM_ROWS * CHAR_H) {
                    assert(pixel == COLOR_BLACK);
                }
            }
        }
    }
    touch_status = MAIA_OK;
    touch_snapshot.frame.count = 2;
    touch_snapshot.frame.points[0].x = 0;
    touch_snapshot.frame.points[0].y = 0;
    touch_snapshot.frame.points[1].x = 1279;
    touch_snapshot.frame.points[1].y = 719;
    lcd_draw(&frames[0][1], TAB5_PHYS_W, TAB5_PHYS_H, NULL);
    assert(frames[0][1 + 1279 * TAB5_PHYS_W] == COLOR_WHITE);
    assert(frames[0][1 + 719] == COLOR_WHITE);
    touch_status = MAIA_ERR_BUSY;
    lcd_draw(&frames[1][1], TAB5_PHYS_W, TAB5_PHYS_H, NULL);
    assert(frames[1][1 + 1279 * TAB5_PHYS_W] != COLOR_WHITE);
    assert(frames[1][1 + 719] == COLOR_BLACK);
    assert(frames[0][0] == poison && frames[0][TAB5_PHYS_W * TAB5_PHYS_H + 1U] == poison);
    assert(frames[1][0] == poison && frames[1][TAB5_PHYS_W * TAB5_PHYS_H + 1U] == poison);
    s_dirty_rows = 0;
    maia_product_display_touch_changed();
    assert(s_dirty_rows != 0);
    assert(irq_state == 8U);
}
