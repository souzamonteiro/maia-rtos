/* MaiaRTOS Tab5 LCD/keyboard smoke test. C99, no FreeRTOS/Arduino/LVGL.
 * Supply the five board adapters in tab5_port.c using EXISTING drivers.
 */
#include <stdint.h>
#include <stddef.h>

/* Return 0 on success, negative on failure. */
extern int tab5_display_init(void);
extern int tab5_keyboard_init(void);
/* Draw a solid rectangle, clipped to the display. RGB565 colors. */
extern void tab5_fill_rect(int x, int y, int w, int h, uint16_t rgb565);
/* Render ASCII text at pixel coordinates. Must update/flush display as needed. */
extern void tab5_draw_text(int x, int y, const char *ascii, uint16_t fg, uint16_t bg);
/* Return -1 if no key, otherwise ASCII code 0..127. Non-blocking. */
extern int tab5_keyboard_poll_ascii(void);
/* Optional serial logging: can be implemented as a no-op. */
extern void tab5_log(const char *message);

#define LCD_W 720
#define LCD_H 1280
#define FG 0xFFFFu
#define BG 0x0000u
#define LINE_H 24
#define MAX_COLS 78
#define FIRST_Y 72
#define MAX_ROWS ((LCD_H - FIRST_Y) / LINE_H)

static char line[MAX_COLS + 1];
static int col = 0;
static int row = 0;

static void show_line(void) {
    int y = FIRST_Y + row * LINE_H;
    tab5_fill_rect(12, y, LCD_W - 24, LINE_H, BG);
    tab5_draw_text(16, y + 2, line, FG, BG);
}

static void new_line(void) {
    col = 0;
    line[0] = '\0';
    row++;
    if (row >= MAX_ROWS) {
        row = 0; /* Minimal smoke test: wrap, no scrolling yet. */
        tab5_fill_rect(0, FIRST_Y, LCD_W, LCD_H - FIRST_Y, BG);
    }
    show_line();
}

static void put_key(int key) {
    if (key == '\r' || key == '\n') {
        new_line();
    } else if (key == 8 || key == 127) {
        if (col > 0) {
            line[--col] = '\0';
            show_line();
        }
    } else if (key >= 32 && key <= 126) {
        if (col >= MAX_COLS) new_line();
        line[col++] = (char)key;
        line[col] = '\0';
        show_line();
    }
}

int main(void) {
    tab5_log("[lcd-test] starting\n");
    if (tab5_display_init() != 0) {
        tab5_log("[lcd-test] LCD initialization FAILED\n");
        return 1;
    }
    tab5_fill_rect(0, 0, LCD_W, LCD_H, BG);
    tab5_draw_text(16, 16, "MaiaRTOS LCD Test", 0x07E0u, BG);
    tab5_draw_text(16, 44, "Type on keyboard (Enter / Backspace supported)", FG, BG);
    tab5_log("[lcd-test] LCD initialized\n");
    if (tab5_keyboard_init() != 0) {
        tab5_draw_text(16, 100, "KEYBOARD INIT FAILED", 0xF800u, BG);
        tab5_log("[lcd-test] keyboard initialization FAILED\n");
        return 2;
    }
    tab5_log("[lcd-test] keyboard initialized; polling\n");
    show_line();
    for (;;) {
        int key = tab5_keyboard_poll_ascii();
        if (key >= 0 && key <= 127) put_key(key);
    }
}
