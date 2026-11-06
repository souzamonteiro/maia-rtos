#include <stddef.h>
#include <stdint.h>
#include <maia/console.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_display.h>
#include <maia/soc/esp32p4_regs.h>
#include "../../soc/espressif/esp32p4/font_8x16.h"

#define DISP_W 720
#define DISP_H 1280

static inline void put_pixel(int x, int y, uint16_t color) {
    if (x < 0 || x >= DISP_W || y < 0 || y >= DISP_H) {
        return;
    }
    volatile uint16_t *band = NULL;
    uint32_t line = 0U;
    if (y < 128) {
        band = (volatile uint16_t *)0x8FF20000UL;
        line = (uint32_t)y;
    } else if (y < 256) {
        band = (volatile uint16_t *)0x8FF4D000UL;
        line = (uint32_t)(y - 128);
    } else {
        band = (volatile uint16_t *)0x8FF7A000UL;
        line = (uint32_t)((y - 256) % 128);
    }
    band[line * (uint32_t)DISP_W + (uint32_t)x] = color;
}

int maia_board_lcd_start(void) {
    maia_console_init();
    maia_tab5_board_io_init();
    maia_display_init(maia_tab5_get_detected_panel());
    return 0;
}

void maia_board_lcd_rect(int x, int y, int w, int h, uint16_t rgb565) {
    if (w <= 0 || h <= 0) {
        return;
    }
    int max_x = x + w;
    int max_y = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (max_x > DISP_W) max_x = DISP_W;
    if (max_y > DISP_H) max_y = DISP_H;
    for (int cy = y; cy < max_y; ++cy) {
        for (int cx = x; cx < max_x; ++cx) {
            put_pixel(cx, cy, rgb565);
        }
    }
}

void maia_board_lcd_text(int x, int y, const char *ascii, uint16_t fg, uint16_t bg) {
    if (ascii == NULL) {
        return;
    }
    int cur_x = x;
    while (*ascii != '\0') {
        char c = *ascii++;
        const uint8_t *glyph = NULL;
        if ((uint8_t)c >= 32 && (uint8_t)c <= 126) {
            glyph = &g_font_8x16[((uint8_t)c - 32U) * 16U];
        }
        for (int py = 0; py < 16; ++py) {
            uint8_t bits = glyph ? glyph[py] : 0U;
            for (int px = 0; px < 8; ++px) {
                uint16_t col = ((bits >> (7 - px)) & 1U) ? fg : bg;
                put_pixel(cur_x + px, y + py, col);
            }
        }
        cur_x += 8;
    }
}

int maia_board_keyboard_start(void) {
    return 0;
}

int maia_board_keyboard_read_ascii(void) {
    maia_display_refresh_dma_if_needed();
    int ch = maia_tab5_keyboard_getc_nonblocking();
    if (ch < 0) {
        ch = maia_console_getc_nonblocking();
    }
    if (ch >= 0) {
        maia_console_putc((char)ch);
        maia_console_flush();
    }
    return ch;
}

void maia_board_serial_write(const char *text) {
    if (text != NULL) {
        maia_console_puts(text);
        maia_console_flush();
    }
}

extern int main(void);

void maia_boot_main(void) {
    (void)main();
    for (;;) {
        __asm__ volatile("wfi");
    }
}

