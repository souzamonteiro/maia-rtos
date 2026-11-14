/* Opt-in boot adapter; native transport and bounded native GPIO timebase only. */
#include <maia/boards/tab5_st7121.h>
static unsigned st7121_index;
static int st7121_write(void *context,const uint8_t *bytes,uint32_t length) {
    (void)context;maia_dsi_short_report_t observation;maia_dsi_access_result_t result;
    if (length<=2U) result=maia_esp32p4_dsi_dcs_short(0,bytes[0],length-1U,length == 2U ? bytes[1] : 0U,10000,&observation);
    else {
        maia_dsi_long_report_t r;
        result=maia_esp32p4_dsi_dcs_long(0,bytes,length,10000,&r);observation=r.transfer;
        if (result != MAIA_DSI_ACCESS_OK) {
            field("ST7121_TRANSFER_FAULT fifo=",r.transfer.fifo);field(" status=",r.transfer.status);
            field(" payload_words=",r.payload_words_attempted);field(" payload_bytes=",r.payload_bytes_attempted);
            field(" pre_fifo=",r.pre_header_fifo);maia_console_puts("\n");
        }
    }
    field("ST7121_PACKET index=",st7121_index++);field(" command=",bytes[0]);field(" bytes=",length);
    field(" result=",(uint32_t)result);field(" stage=",observation.completed);field(" polls=",observation.polls);
    field(" error0=",observation.error0);field(" error1=",observation.error1);maia_console_puts("\n");
    return (int)result;
}
static int st7121_wait(void *context,uint32_t ms) {
    (void)context;
    for (uint32_t i=0;i<ms;++i) for (unsigned p=0;p<200U;++p)
        if (reset_bus.lines.pause(reset_bus.lines.context) != MAIA_I2C_OK) return -1;
    return 0;
}
static void st7121_program(void) {
    uint8_t firmware=0;
    if (maia_tab5_touch_identify(&reset_bus.lines,&firmware) != MAIA_I2C_OK || firmware != 1U) halt("st7121-variant");
    maia_st7121_io_t io={NULL,st7121_write,st7121_wait};maia_st7121_report_t r,duplicate;
    uint64_t start=maia_soc_time_us();
    maia_st7121_result_t result=maia_tab5_st7121_program(firmware,&io,&r);
    uint64_t elapsed=maia_soc_time_us()-start;
    field("ST7121_PROGRAM result=",(uint32_t)result);field(" attempted=",r.attempted);field(" completed=",r.completed);
    field(" waited_ms=",r.waited_ms);field(" last=",r.last_command);field(" callback=",(uint32_t)r.callback_result);
    field(" elapsed_us=",(uint32_t)elapsed);maia_console_puts("\n");
    if (result != MAIA_ST7121_OK || elapsed<1150000U || elapsed>0xFFFFFFFFU ||
        maia_tab5_st7121_program(firmware,&io,&duplicate) != MAIA_ST7121_BUSY) halt("st7121-program");
    uint32_t first=0;
    for (unsigned i=0;i<4U;++i) {
        maia_dsi_read_report_t reply;maia_dsi_access_result_t rr=maia_esp32p4_dsi_read_id(10000,&reply);
        field("ST7121_REPLY index=",i);field(" result=",(uint32_t)rr);field(" stage=",reply.transfer.completed);
        field(" headers=",reply.headers_attempted);field(" polls=",reply.transfer.polls);field(" word=",reply.word);field(" words=",reply.words_received);field(" before=",reply.transfer.command_before);
        field(" after=",reply.transfer.command_after);field(" restored=",reply.restore_attempted);
        field(" status=",reply.transfer.status);field(" fifo=",reply.transfer.fifo);
        field(" error0=",reply.transfer.error0);field(" error1=",reply.transfer.error1);maia_console_puts("\n");
        if (rr != MAIA_DSI_ACCESS_OK || reply.words_received != 1U || reply.transfer.completed != 10U ||
            (i && (reply.word&0xFFFFFFU) != first)) halt("st7121-response");
        first=reply.word&0xFFFFFFU;
    }
    maia_console_puts("MAIA_NATIVE_ST7121_PASS\n");
}
