#include <maia/console.h>
#include <maia/drivers/serial_rx.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_display.h>
#include <maia/soc/esp32p4_regs.h>

/* Bounded spin count (~5ms at 400 MHz) so console never hangs indefinitely if disconnected. */
#define MAIA_CONSOLE_SPIN_LIMIT 2000000U

static uint8_t s_ep1_fifo_cnt = 0U;

void maia_console_init(void) {
    /* ROM already configures USB-Serial/JTAG (40 MHz XTAL) and UART0 on reset. */
    s_ep1_fifo_cnt = 0U;
}

static void usb_jtag_putc(uint8_t byte) {
    for (uint32_t spin = 0; spin < MAIA_CONSOLE_SPIN_LIMIT; ++spin) {
        if ((MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) & MAIA_ESP32P4_USB_SERIAL_IN_EP_DATA_FREE) != 0UL) {
            MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_REG) = (uint32_t)byte;
            s_ep1_fifo_cnt++;
            if (s_ep1_fifo_cnt >= 63U) {
                MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) = MAIA_ESP32P4_USB_WR_DONE;
                s_ep1_fifo_cnt = 0U;
            }
            return;
        }
    }
}

static void uart0_putc(uint8_t byte) {
    for (uint32_t spin = 0; spin < MAIA_CONSOLE_SPIN_LIMIT; ++spin) {
        uint32_t status = MAIA_MMIO32(MAIA_ESP32P4_UART0_STATUS_REG);
        uint32_t txfifo_cnt = (status >> MAIA_ESP32P4_UART_TXFIFO_CNT_SHIFT) & MAIA_ESP32P4_UART_TXFIFO_CNT_MASK;
        if (txfifo_cnt < 120U) {
            MAIA_MMIO32(MAIA_ESP32P4_UART0_FIFO_REG) = (uint32_t)byte;
            return;
        }
    }
}

void maia_console_flush(void) {
    if (s_ep1_fifo_cnt > 0U) {
        MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) = MAIA_ESP32P4_USB_WR_DONE;
        s_ep1_fifo_cnt = 0U;
    }
}

void maia_console_putc(char c) {
    if (c == '\n') {
        usb_jtag_putc((uint8_t)'\r');
        uart0_putc((uint8_t)'\r');
    }
    usb_jtag_putc((uint8_t)c);
    uart0_putc((uint8_t)c);
    maia_display_putc(c);
    if (c == '\n') {
        maia_console_flush();
    }
}

void maia_console_write(const char *buf, size_t len) {
    if (buf == NULL) {
        return;
    }
    for (size_t i = 0; i < len; ++i) {
        maia_console_putc(buf[i]);
    }
    if (len > 0U && buf[len - 1U] == ' ') {
        maia_console_flush();
    }
}

void maia_console_puts(const char *str) {
    if (str == NULL) {
        return;
    }
    char last = '\0';
    while (*str != '\0') {
        last = *str++;
        maia_console_putc(last);
    }
    if (last == ' ') {
        maia_console_flush();
    }
}

int maia_console_getc_nonblocking(void) {
    int byte=maia_serial_rx_read(NULL);
    /* Managed input has a sole reader. Do not steal bytes or poll board/display. */
    if (byte == -2) return -1;
    maia_display_refresh_dma_if_needed();
    if (byte >= 0) return byte;
    return maia_tab5_keyboard_getc_nonblocking();
}
