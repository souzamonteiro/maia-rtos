#include <maia/services/serial_input.h>
#include <maia/console.h>
#include <maia/drivers/serial_rx.h>
#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_regs.h>
#include <stdio.h>
static uintptr_t irq_state;
static maia_task_id_t current=-1;
static uint8_t usb[256], uart[2];
static unsigned head,tail,uart_head,uart_tail,reads,saves,restores,violations,refreshes,keyboard_calls;
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { ++refreshes; }
int maia_tab5_keyboard_getc_nonblocking(void) { ++keyboard_calls;return 'k'; }
uintptr_t maia_arch_irq_save(void) { uintptr_t old=irq_state;irq_state=0U;++saves;return old; }
void maia_arch_irq_restore(uintptr_t previous) { irq_state=previous&8U;++restores; }
maia_task_id_t maia_task_current_id(void) { return current; }
maia_status_t maia_task_get_info(uint32_t id, maia_task_info_t *out) {
    if (id != 0U) return MAIA_ERR_INVALID;
    out->state=MAIA_TASK_READY;return MAIA_OK;
}
uint32_t maia_serial_rx_test_register(uintptr_t address) {
    ++reads;if (irq_state) ++violations;
    if (address == MAIA_ESP32P4_USB_EP1_CONF_REG) return head<tail ? MAIA_ESP32P4_USB_SERIAL_OUT_EP_AVAIL : 0U;
    if (address == MAIA_ESP32P4_USB_EP1_REG) return usb[head++];
    if (address == MAIA_ESP32P4_UART0_STATUS_REG) return uart_tail-uart_head;
    if (address == MAIA_ESP32P4_UART0_FIFO_REG) return uart[uart_head++];
    ++violations;return 0U;
}
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d\n",__LINE__);return 1; } } while(0)
int main(void) {
    uint8_t byte=0xaaU;maia_serial_input_stats_t stats;
    CHECK(maia_serial_input_read(&byte) == MAIA_ERR_INVALID && byte == 0xaaU);
    CHECK(maia_serial_input_poll() == MAIA_ERR_INVALID);
    CHECK(maia_serial_input_get_stats(&stats) == MAIA_ERR_INVALID);
    CHECK(maia_serial_input_init(-1) == MAIA_ERR_INVALID);
    CHECK(maia_serial_input_init(8) == MAIA_ERR_INVALID);
    irq_state=8U; CHECK(maia_serial_input_init(0) == MAIA_ERR_PERM && irq_state == 8U);
    usb[tail++]=0U;uart[uart_tail++]=255U;
    CHECK(maia_serial_rx_read(NULL) == 0 && maia_serial_rx_read(NULL) == 255);
    CHECK(maia_serial_rx_read(NULL) == -1 && irq_state == 8U);
    usb[tail++]=0x81U;
    CHECK(maia_console_getc_nonblocking() == 0x81 && refreshes == 1U);
    CHECK(maia_console_getc_nonblocking() == 'k' && refreshes == 2U && keyboard_calls == 1U);
    CHECK(maia_serial_rx_claim(NULL) == -2);
    irq_state=0U;CHECK(maia_serial_input_init(0) == MAIA_OK && irq_state == 0U);
    CHECK(maia_serial_input_init(0) == MAIA_ERR_BUSY);
    CHECK(maia_serial_rx_claim(&stats) == -3);
    unsigned before=reads;
    CHECK(maia_serial_rx_read(NULL) == -2 && reads == before);
    CHECK(maia_console_getc_nonblocking() == -1 && reads == before && refreshes == 2U && keyboard_calls == 1U);
    CHECK(maia_serial_rx_read(&stats) == -2 && reads == before);
    for (unsigned i=0U;i<80U;++i) usb[tail++]=(uint8_t)i;
    current=0;CHECK(maia_serial_input_poll() == MAIA_ERR_PERM && reads == before);
    irq_state=8U;current=1;
    CHECK(maia_serial_input_poll() == MAIA_ERR_PERM && reads == before);
    current=0;
    for (unsigned i=0U;i<4U;++i) {
        CHECK(maia_serial_input_poll() == MAIA_OK && irq_state == 8U);
        CHECK(maia_serial_input_get_stats(&stats) == MAIA_OK && stats.received == (i+1U)*16U);
    }
    before=reads;
    CHECK(maia_serial_input_poll() == MAIA_ERR_FULL && reads == before);
    CHECK(maia_serial_input_get_stats(&stats) == MAIA_OK && stats.queued == 64U && stats.full_events == 1U);
    CHECK(maia_serial_input_read(NULL) == MAIA_ERR_INVALID);
    CHECK(maia_serial_input_get_stats(NULL) == MAIA_ERR_INVALID);
    current=1;irq_state=0U;
    for (unsigned i=0U;i<64U;++i) CHECK(maia_serial_input_read(&byte) == MAIA_OK && byte == i && irq_state == 0U);
    byte=0xccU;CHECK(maia_serial_input_read(&byte) == MAIA_ERR_EMPTY && byte == 0xccU);
    current=0;irq_state=8U;
    CHECK(maia_serial_input_poll() == MAIA_OK);
    CHECK(maia_serial_input_poll() == MAIA_ERR_EMPTY && irq_state == 8U);
    for (unsigned i=64U;i<80U;++i) CHECK(maia_serial_input_read(&byte) == MAIA_OK && byte == i);
    CHECK(maia_serial_input_get_stats(&stats) == MAIA_OK && stats.received == 80U && !stats.queued && stats.owner == 0);
    CHECK(!violations && saves == restores && irq_state == 8U);
    puts("PASS: RX ownership, legacy denial, USB/UART ordering, bounded batches, full queue preserves FIFO, nested masks and service lifetime");
    return 0;
}
