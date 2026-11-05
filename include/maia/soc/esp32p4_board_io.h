#ifndef MAIA_SOC_ESP32P4_BOARD_IO_H
#define MAIA_SOC_ESP32P4_BOARD_IO_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize M5Stack Tab5 IO expanders (PI4IOE5V6408 @ 0x43, 0x44),
 * enable EXT5V (for keyboard), release LCD_RST & TP_RST, turn on LCD backlight (GPIO22),
 * and initialize keyboard controller (TCA8418 @ 0x34 or STM32 @ 0x6D on GPIO0/1).
 */
void maia_tab5_board_io_init(void);

/* Poll physical keyboard on Ext.Port1 I2C (SDA=GPIO0, SCL=GPIO1, INT=GPIO50).
 * Returns ASCII character (0..127) if a key was pressed, or -1 if none.
 */
int maia_tab5_keyboard_getc_nonblocking(void);

/* Microsecond busy-wait delay using SYSTIMER. */
void maia_udelay(uint32_t us);

/* System I2C bus (SDA=GPIO31, SCL=GPIO32) & Ext I2C bus (SDA=GPIO0, SCL=GPIO1) helpers */
int maia_i2c_write_bytes(uint32_t sda_pin, uint32_t scl_pin, uint8_t addr7, const uint8_t *data, size_t len);
int maia_i2c_write_read(uint32_t sda_pin, uint32_t scl_pin, uint8_t addr7,
                        const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len);

/* Detected Tab5 display controller type */
#define MAIA_TAB5_PANEL_UNKNOWN  0U
#define MAIA_TAB5_PANEL_ILI9881C 1U
#define MAIA_TAB5_PANEL_ST7123   2U
#define MAIA_TAB5_PANEL_ST7121   3U

uint32_t maia_tab5_get_detected_panel(void);

/* Set display backlight PWM duty cycle (0..4095) */
void maia_tab5_backlight_set_duty(uint32_t duty_4095);

#ifdef __cplusplus
}
#endif

#endif /* MAIA_SOC_ESP32P4_BOARD_IO_H */

