/* MaiaRTOS Tab5 smoke-test adapter: C99, no RTOS/ESP-IDF dependencies.
 * Wire these six board hooks to your EXISTING Tab5 BSP/driver.
 * No success stubs: absent hardware hooks produce link errors.
 */
#include <stdint.h>
#include <stddef.h>

/* Implement these symbols in your existing board support sources.
 * Pixel coordinates are physical display coordinates; color is RGB565.
 * Board functions must handle DSI/cache/flush and clipping as appropriate.
 */
extern int maia_board_lcd_start(void);
extern void maia_board_lcd_rect(int x, int y, int w, int h, uint16_t rgb565);
extern void maia_board_lcd_text(int x, int y, const char *ascii,
                                uint16_t fg, uint16_t bg);
extern int maia_board_keyboard_start(void);
extern int maia_board_keyboard_read_ascii(void);
extern void maia_board_serial_write(const char *text);

int tab5_display_init(void) {
    return maia_board_lcd_start();
}

void tab5_fill_rect(int x, int y, int w, int h, uint16_t rgb565) {
    if (w <= 0 || h <= 0) return;
    maia_board_lcd_rect(x, y, w, h, rgb565);
}

void tab5_draw_text(int x, int y, const char *ascii,
                    uint16_t fg, uint16_t bg) {
    if (!ascii) return;
    maia_board_lcd_text(x, y, ascii, fg, bg);
}

int tab5_keyboard_init(void) {
    return maia_board_keyboard_start();
}

int tab5_keyboard_poll_ascii(void) {
    int c = maia_board_keyboard_read_ascii();
    return (c >= 0 && c <= 127) ? c : -1;
}

void tab5_log(const char *message) {
    if (message) maia_board_serial_write(message);
}
