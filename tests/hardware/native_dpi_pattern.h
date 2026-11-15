/* Private native host VPG fixture: no bridge latch/output, DMA or framebuffer.
 * Register confirmation is not physical pixel proof. */
#include "dpi_backlight_fixture.h"
static void pattern_log(const char *prefix,maia_dsi_access_result_t result,const maia_dsi_dpi_pattern_report_t *r) {
    field(prefix,(uint32_t)result);field(" stage=",r->completed);field(" attempted=",r->attempted_mask);
    field(" pattern=",r->pattern);field(" mode=",r->mode);field(" lp=",r->lp_clock);field(" shadow=",r->shadow);
    field(" polls=",r->observation.polls);field(" status=",r->observation.status);field(" fifo=",r->observation.fifo);
    field(" error0=",r->observation.error0);field(" error1=",r->observation.error1);maia_console_puts("\n");
}
static void dpi_pattern(void) {
    dpi_backlight_snapshot_t saved;
    if (!dpi_bl_on(rd,wr,&saved)) halt("dpi-pattern-light-on");
    field("DPI_PATTERN_LIGHT out_before=",saved.out);field(" enable_before=",saved.enable);field(" pad_before=",saved.pad);
    field(" mux_before=",saved.mux);field(" matrix_before=",saved.matrix);
    field(" out=",rd(MAIA_ESP32P4_GPIO_OUT_REG));field(" enable=",rd(MAIA_ESP32P4_GPIO_ENABLE_REG));
    field(" pad=",rd(MAIA_ESP32P4_GPIO_PIN_REG(22)));field(" mux=",rd(MAIA_ESP32P4_IO_MUX_GPIO_REG(22)));
    field(" matrix=",rd(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22)));maia_console_puts("\n");
    maia_dsi_dpi_pattern_report_t r,again;
    maia_dsi_access_result_t result=maia_esp32p4_dsi_dpi_pattern_start(&r);
    pattern_log("DPI_PATTERN_START result=",result,&r);
    if (result != MAIA_DSI_ACCESS_OK) halt("dpi-pattern-start");
    maia_dsi_read_report_t blocked;
    int exclusive=maia_esp32p4_dsi_read_power_mode(10000,&blocked) == MAIA_DSI_ACCESS_BUSY &&
        maia_esp32p4_dsi_dpi_pattern_start(&again) == MAIA_DSI_ACCESS_BUSY;
    if (!exclusive) halt("dpi-pattern-exclusive");
    for (unsigned i=0;i<5U;++i) {
        if (st7121_wait(NULL,1000)) halt("dpi-pattern-wait");
        uint32_t mode=rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34U),pattern=rd(MAIA_ESP32P4_DSI_HOST_BASE+0x38U),
            lp=rd(MAIA_ESP32P4_DSI_HOST_BASE+0x94U),status=rd(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U),
            fifo=rd(MAIA_ESP32P4_DSI_HOST_BASE+0x74U),e0=rd(MAIA_ESP32P4_DSI_HOST_BASE+0xBCU),e1=rd(MAIA_ESP32P4_DSI_HOST_BASE+0xC0U);
        int inactive=!(rd(MAIA_ESP32P4_DSI_BRG_BASE+4U)&1U) && !(rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U)&1U) &&
            !rd(MAIA_ESP32P4_DSI_BRG_BASE+0x44U) && vo3_dma_quiet(rd);
        field("DPI_PATTERN_HOLD index=",i);field(" mode=",mode);field(" pattern=",pattern);field(" lp=",lp);
        field(" status=",status);field(" fifo=",fifo);field(" error0=",e0);field(" error1=",e1);
        field(" inactive=",(uint32_t)inactive);field(" exclusive=",(uint32_t)exclusive);maia_console_puts("\n");
        if (mode != r.mode || pattern != r.pattern || lp != r.lp_clock || !(status&1U) ||
            (e0&0x1FFFFFU) || (e1&0x81FFFU) || !inactive) halt("dpi-pattern-hold");
    }
    result=maia_esp32p4_dsi_dpi_pattern_stop(10000,&r);
    pattern_log("DPI_PATTERN_STOP result=",result,&r);
    if (result != MAIA_DSI_ACCESS_OK || maia_esp32p4_dsi_dpi_pattern_stop(10000,&again) != MAIA_DSI_ACCESS_BUSY)
        halt("dpi-pattern-stop");
    if (!dpi_bl_restore(rd,wr,&saved)) halt("dpi-pattern-light-restore");
    maia_console_puts("DPI_PATTERN_LIGHT_RESTORE verified=00000001\n");
    for (unsigned i=0;i<4U;++i) {
        maia_dsi_read_report_t p;result=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DPI_PATTERN_REPLY index=",i);field(" result=",(uint32_t)result);field(" stage=",p.transfer.completed);
        field(" headers=",p.headers_attempted);field(" polls=",p.transfer.polls);field(" word=",p.word);field(" words=",p.words_received);
        field(" before=",p.transfer.command_before);field(" after=",p.transfer.command_after);field(" restored=",p.restore_attempted);
        field(" status=",p.transfer.status);field(" fifo=",p.transfer.fifo);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if (result != MAIA_DSI_ACCESS_OK || p.transfer.completed != 10U || p.words_received != 1U || (p.word&0x14U) != 0x14U) halt("dpi-pattern-reply");
    }
    maia_console_puts("MAIA_NATIVE_DPI_PATTERN_PASS\n");
}
