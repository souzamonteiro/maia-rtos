#include <maia/drivers/serial_tx.h>
#include <maia/soc/esp32p4_regs.h>
#include <stddef.h>
#include <stdint.h>

#define TX_SPIN_LIMIT 2000000U

enum {
    PROBE_STATUS_TX_ERROR = 1U,
    PROBE_STATUS_FLUSH_ERROR = 2U,
    PROBE_STATUS_OK = 3U
};

volatile uint32_t maia_f0_flash_probe_status;

static int put_string(const char *text)
{
    while (*text != '\0') {
        int sent = 0;
        for (uint32_t spin = 0U; spin < TX_SPIN_LIMIT; ++spin) {
            int rc = maia_serial_tx_try_put(NULL, MAIA_SERIAL_TX_USB, (uint8_t)*text);
            if (rc == 0) {
                sent = 1;
                break;
            }
            if (rc != -1) {
                return rc;
            }
        }
        if (!sent) {
            return -1;
        }
        ++text;
    }
    return 0;
}

void maia_boot_main(void)
{
    /* A flashed probe must stay recoverable by the host's USB RTS/DTR reset. */
    MAIA_MMIO32(MAIA_ESP32P4_USB_CHIP_RST_REG) &= ~MAIA_ESP32P4_USB_UART_CHIP_RST_DIS;
    if (put_string("\r\nMAIA_F0_FLASH_PROBE\r\n") != 0) {
        maia_f0_flash_probe_status = PROBE_STATUS_TX_ERROR;
    } else if (maia_serial_tx_flush(NULL, MAIA_SERIAL_TX_USB) != 0) {
        maia_f0_flash_probe_status = PROBE_STATUS_FLUSH_ERROR;
    } else {
        maia_f0_flash_probe_status = PROBE_STATUS_OK;
    }
    for (;;) {
        __asm__ volatile("wfi");
    }
}
