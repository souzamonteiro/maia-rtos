#include <maia/drivers/serial_tx.h>
#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_regs.h>
static const void *tx_owner;
static unsigned usb_count;
static uint32_t read_register(uintptr_t address) {
#ifdef MAIA_SERIAL_TX_TEST
    extern uint32_t maia_serial_tx_test_read(uintptr_t);
    return maia_serial_tx_test_read(address);
#else
    return MAIA_MMIO32(address);
#endif
}
static void write_register(uintptr_t address, uint32_t value) {
#ifdef MAIA_SERIAL_TX_TEST
    extern void maia_serial_tx_test_write(uintptr_t,uint32_t);
    maia_serial_tx_test_write(address,value);
#else
    MAIA_MMIO32(address)=value;
#endif
}
static void commit_usb(void) {
    if (usb_count) { write_register(MAIA_ESP32P4_USB_EP1_CONF_REG,MAIA_ESP32P4_USB_WR_DONE);usb_count=0U; }
}
void maia_serial_tx_legacy_init(void) {
    uintptr_t irq=maia_arch_irq_save();if (!tx_owner) usb_count=0U;maia_arch_irq_restore(irq);
}
int maia_serial_tx_claim(const void *owner) {
    if (!owner) return -2;
    uintptr_t irq=maia_arch_irq_save();
    int status=tx_owner ? -3 : 0;
    if (!status) { commit_usb();tx_owner=owner; }
    maia_arch_irq_restore(irq);return status;
}
int maia_serial_tx_may_write(const void *owner) {
    uintptr_t irq=maia_arch_irq_save();int allowed=owner == tx_owner;maia_arch_irq_restore(irq);return allowed;
}
int maia_serial_tx_try_put(const void *owner, maia_serial_tx_sink_t sink, uint8_t byte) {
    uintptr_t irq=maia_arch_irq_save();
    int status=-1;
    if (owner != tx_owner) status=-2;
    else if (sink == MAIA_SERIAL_TX_USB) {
        if (read_register(MAIA_ESP32P4_USB_EP1_CONF_REG)&MAIA_ESP32P4_USB_SERIAL_IN_EP_DATA_FREE) {
            write_register(MAIA_ESP32P4_USB_EP1_REG,byte);
            if (++usb_count == 32U) commit_usb();
            status=0;
        }
    } else if (sink == MAIA_SERIAL_TX_UART0) {
        uint32_t count=(read_register(MAIA_ESP32P4_UART0_STATUS_REG)>>MAIA_ESP32P4_UART_TXFIFO_CNT_SHIFT)&MAIA_ESP32P4_UART_TXFIFO_CNT_MASK;
        if (count<120U) { write_register(MAIA_ESP32P4_UART0_FIFO_REG,byte);status=0; }
    } else status=-3;
    maia_arch_irq_restore(irq);return status;
}
int maia_serial_tx_flush(const void *owner, maia_serial_tx_sink_t sink) {
    uintptr_t irq=maia_arch_irq_save();int status=0;
    if (owner != tx_owner) status=-2;
    else if (sink == MAIA_SERIAL_TX_USB) commit_usb();
    else if (sink != MAIA_SERIAL_TX_UART0) status=-3;
    maia_arch_irq_restore(irq);return status;
}
