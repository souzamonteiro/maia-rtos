#include <maia/drivers/serial_rx.h>
#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_regs.h>
static const void *rx_owner;
static uint32_t read_register(uintptr_t address) {
#ifdef MAIA_SERIAL_RX_TEST
    extern uint32_t maia_serial_rx_test_register(uintptr_t address);
    return maia_serial_rx_test_register(address);
#else
    return MAIA_MMIO32(address);
#endif
}
int maia_serial_rx_claim(const void *owner) {
    if (!owner) return -2;
    uintptr_t irq=maia_arch_irq_save();
    int status=rx_owner ? -3 : 0;
    if (!status) rx_owner=owner;
    maia_arch_irq_restore(irq);
    return status;
}
int maia_serial_rx_read(const void *owner) {
    uintptr_t irq=maia_arch_irq_save();
    int result=-1;
    if (owner != rx_owner) result=-2;
    else if (read_register(MAIA_ESP32P4_USB_EP1_CONF_REG) & MAIA_ESP32P4_USB_SERIAL_OUT_EP_AVAIL)
        result=(int)(read_register(MAIA_ESP32P4_USB_EP1_REG)&255U);
    else if (read_register(MAIA_ESP32P4_UART0_STATUS_REG) & MAIA_ESP32P4_UART_RXFIFO_CNT_MASK)
        result=(int)(read_register(MAIA_ESP32P4_UART0_FIFO_REG)&255U);
    maia_arch_irq_restore(irq);
    return result;
}
