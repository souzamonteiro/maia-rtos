#include <maia/services/serial_output.h>
#include <maia/console.h>
#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_regs.h>
#include <stdio.h>
#include <string.h>
static uintptr_t irq_state;
static maia_task_id_t current=-1;
static unsigned saves,restores,violations,usb_ready=1U,uart_count,reads,writes,commits,display;
static uint8_t usb[4096],uart[4096];static unsigned usb_n,uart_n;
uintptr_t maia_arch_irq_save(void) { uintptr_t old=irq_state;irq_state=0U;++saves;return old; }
void maia_arch_irq_restore(uintptr_t old) { irq_state=old&8U;++restores; }
maia_task_id_t maia_task_current_id(void) { return current; }
maia_status_t maia_task_get_info(uint32_t id, maia_task_info_t *out) {
    if (id != 0U) return MAIA_ERR_INVALID;
    out->state=MAIA_TASK_READY;return MAIA_OK;
}
void maia_display_putc(char c) { (void)c;++display; }
void maia_display_refresh_dma_if_needed(void) { ++violations; }
int maia_tab5_keyboard_getc_nonblocking(void) { ++violations;return -1; }
uint32_t maia_serial_tx_test_read(uintptr_t address) {
    ++reads;if (irq_state) ++violations;
    if (address == MAIA_ESP32P4_USB_EP1_CONF_REG) return usb_ready ? MAIA_ESP32P4_USB_SERIAL_IN_EP_DATA_FREE : 0U;
    if (address == MAIA_ESP32P4_UART0_STATUS_REG) return uart_count<<MAIA_ESP32P4_UART_TXFIFO_CNT_SHIFT;
    ++violations;return 0U;
}
void maia_serial_tx_test_write(uintptr_t address, uint32_t value) {
    ++writes;if (irq_state) ++violations;
    if (address == MAIA_ESP32P4_USB_EP1_REG) { if (!usb_ready) ++violations;usb[usb_n++]=(uint8_t)value; }
    else if (address == MAIA_ESP32P4_UART0_FIFO_REG) uart[uart_n++]=(uint8_t)value;
    else if (address == MAIA_ESP32P4_USB_EP1_CONF_REG && value == MAIA_ESP32P4_USB_WR_DONE) { ++commits;usb_ready=0U; }
    else ++violations;
}
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d\n",__LINE__);return 1; } } while(0)
int main(void) {
    maia_serial_output_stats_t stats;
    CHECK(maia_serial_output_poll() == MAIA_ERR_INVALID);
    CHECK(maia_serial_output_write("x",1U) == MAIA_ERR_INVALID);
    CHECK(maia_serial_output_get_stats(&stats) == MAIA_ERR_INVALID);
    maia_console_init();irq_state=8U;
    for (unsigned i=0U;i<32U;++i) CHECK(maia_serial_tx_try_put(NULL,MAIA_SERIAL_TX_USB,(uint8_t)i) == 0);
    CHECK(commits == 1U && !usb_ready && irq_state == 8U);
    unsigned w=writes;CHECK(maia_serial_tx_flush(NULL,MAIA_SERIAL_TX_USB) == 0 && writes == w);
    CHECK(maia_serial_tx_try_put(NULL,MAIA_SERIAL_TX_USB,99U) == -1 && writes == w);
    uart_count=120U;CHECK(maia_serial_tx_try_put(NULL,MAIA_SERIAL_TX_UART0,7U) == -1);
    uart_count=119U;CHECK(maia_serial_tx_try_put(NULL,MAIA_SERIAL_TX_UART0,7U) == 0);
    CHECK(maia_serial_tx_try_put(NULL,(maia_serial_tx_sink_t)9,0U) == -3);
    CHECK(maia_serial_tx_flush(NULL,(maia_serial_tx_sink_t)9) == -3);
    usb_ready=1U;uart_count=0U;usb_n=uart_n=0U;
    maia_console_puts("A\n");
    CHECK(usb_n == 3U && uart_n == 3U && memcmp(usb,"A\r\n",3U) == 0 && memcmp(uart,"A\r\n",3U) == 0 && display == 2U);
    usb_ready=1U;CHECK(maia_serial_tx_try_put(NULL,MAIA_SERIAL_TX_USB,'p') == 0);
    CHECK(maia_serial_output_init(0,MAIA_SERIAL_TX_USB) == MAIA_ERR_PERM);
    irq_state=0U;
    CHECK(maia_serial_output_init(-1,MAIA_SERIAL_TX_USB) == MAIA_ERR_INVALID);
    CHECK(maia_serial_output_init(8,MAIA_SERIAL_TX_USB) == MAIA_ERR_INVALID);
    CHECK(maia_serial_output_init(0,(maia_serial_tx_sink_t)9) == MAIA_ERR_INVALID);
    unsigned old_commits=commits;
    CHECK(maia_serial_output_init(0,MAIA_SERIAL_TX_USB) == MAIA_OK && commits == old_commits+1U && irq_state == 0U);
    CHECK(maia_serial_output_init(0,MAIA_SERIAL_TX_USB) == MAIA_ERR_BUSY);
    unsigned r=reads;w=writes;
    CHECK(!maia_serial_tx_may_write(NULL));
    CHECK(maia_serial_tx_try_put(NULL,MAIA_SERIAL_TX_USB,1U) == -2);
    CHECK(maia_serial_tx_flush(NULL,MAIA_SERIAL_TX_USB) == -2);
    maia_console_init();maia_console_puts("MUST_NOT_APPEAR\n");maia_console_flush();
    CHECK(reads == r && writes == w && display == 2U);
    CHECK(maia_serial_output_write(NULL,1U) == MAIA_ERR_INVALID);
    CHECK(maia_serial_output_write("x",0U) == MAIA_ERR_INVALID);
    uint8_t data[65];for (unsigned i=0U;i<sizeof data;++i) data[i]=(uint8_t)i;
    CHECK(maia_serial_output_write(data,65U) == MAIA_ERR_INVALID);
    CHECK(maia_serial_output_get_stats(NULL) == MAIA_ERR_INVALID);
    current=0;CHECK(maia_serial_output_poll() == MAIA_ERR_PERM);
    current=1;irq_state=8U;CHECK(maia_serial_output_poll() == MAIA_ERR_PERM && reads == r);
    CHECK(maia_serial_output_write(data,64U) == MAIA_OK);
    memset(data,0xcc,64U); /* Accepted data must already be privately copied. */
    for (unsigned i=0U;i<7U;++i) CHECK(maia_serial_output_write("z",1U) == MAIA_OK);
    CHECK(maia_serial_output_write("full",4U) == MAIA_ERR_FULL);
    current=0;usb_ready=0U;
    CHECK(maia_serial_output_poll() == MAIA_ERR_BUSY && usb_n == 4U);
    CHECK(maia_serial_output_get_stats(&stats) == MAIA_OK && stats.pending_bytes == 64U && stats.queued_records == 7U && stats.accepted_records == 8U);
    usb_ready=1U;unsigned begin=usb_n;
    CHECK(maia_serial_output_poll() == MAIA_OK && usb_n == begin+16U && irq_state == 8U);
    CHECK(maia_serial_output_get_stats(&stats) == MAIA_OK && stats.pending_bytes == 48U && stats.written_bytes == 16U);
    CHECK(maia_serial_output_poll() == MAIA_ERR_BUSY && usb_n == begin+16U);
    for (unsigned n=0U;n<20U;++n) {
        usb_ready=1U;
        maia_status_t result=maia_serial_output_poll();
        CHECK(result == MAIA_OK || result == MAIA_ERR_BUSY || result == MAIA_ERR_EMPTY);
    }
    CHECK(usb_n == begin+71U);
    for (unsigned i=0U;i<64U;++i) CHECK(usb[begin+i] == i);
    for (unsigned i=64U;i<71U;++i) CHECK(usb[begin+i] == 'z');
    CHECK(maia_serial_output_get_stats(&stats) == MAIA_OK && stats.accepted_records == 8U && stats.completed_records == 8U && stats.accepted_bytes == 71U && stats.written_bytes == 71U && !stats.pending_bytes && !stats.queued_records && stats.busy_events>=2U);
    irq_state=0U;CHECK(maia_serial_output_write("irq",3U) == MAIA_OK && irq_state == 0U);
    CHECK(maia_serial_output_get_stats(&stats) == MAIA_OK && irq_state == 0U);
    irq_state=8U;usb_ready=1U;CHECK(maia_serial_output_poll() == MAIA_OK && memcmp(usb+begin+71U,"irq",3U) == 0);
    CHECK(!violations && saves == restores && irq_state == 8U);
    puts("PASS: legacy CRLF/claim, sole TX owner, whole-record enqueue, FULL, partial BUSY/resume, FIFO order, batch budget and nested masks");
    return 0;
}
