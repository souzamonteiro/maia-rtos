#include <maia/console.h>
#include <maia/drivers/serial_rx.h>
#include <maia/drivers/serial_tx.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_display.h>

/* Legacy best-effort output only; bounded attempts, no time guarantee. */
#define MAIA_CONSOLE_SPIN_LIMIT 2000000U
void maia_console_init(void) { maia_serial_tx_legacy_init(); }
static void legacy_putc(maia_serial_tx_sink_t sink, uint8_t byte) {
    for (uint32_t spin=0U;spin<MAIA_CONSOLE_SPIN_LIMIT;++spin) {
        if (maia_serial_tx_try_put(NULL,sink,byte) != -1) return;
    }
}
void maia_console_flush(void) { (void)maia_serial_tx_flush(NULL,MAIA_SERIAL_TX_USB); }

void maia_console_putc(char c) {
    if (!maia_serial_tx_may_write(NULL)) return;
    if (c == '\n') {
        legacy_putc(MAIA_SERIAL_TX_USB,(uint8_t)'\r');
        legacy_putc(MAIA_SERIAL_TX_UART0,(uint8_t)'\r');
    }
    legacy_putc(MAIA_SERIAL_TX_USB,(uint8_t)c);
    legacy_putc(MAIA_SERIAL_TX_UART0,(uint8_t)c);
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
