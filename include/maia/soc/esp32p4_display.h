#ifndef MAIA_SOC_ESP32P4_DISPLAY_H
#define MAIA_SOC_ESP32P4_DISPLAY_H

#include <stddef.h>
#include <stdint.h>

#define MAIA_TAB5_DSI_VIDEO_MODE_CFG 0xBF02UL /* DSI video mode with burst/no frame-BTA */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * M5Stack Tab5 native MIPI-DSI display service, backed by the native PSRAM
 * framebuffer and DMA path. No FreeRTOS or ESP-IDF runtime.
 */

void maia_display_init(uint32_t panel_type);
void maia_display_putc(char c);
void maia_display_puts(const char *str);
void maia_display_write(const char *buf, size_t len);
void maia_display_clear(void);
void maia_display_refresh_dma_if_needed(void);
void maia_display_set_exclusive(int exclusive);

#ifdef __cplusplus
}
#endif

#endif /* MAIA_SOC_ESP32P4_DISPLAY_H */
