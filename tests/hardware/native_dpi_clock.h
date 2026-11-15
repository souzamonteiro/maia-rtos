/* Private ST7121/BSP fixture. COLMOD55 selection fact cross-checked against
 * M5GFX28d47949 Panel_DSI init_panel/setColorDepth. Original native control flow;
 * no vendor runtime copied. References/limits: docs/audits/native-dpi-clock.md. */
static uint32_t dpi_color(unsigned phase) {
    uint32_t first=0;
    for (unsigned i=0;i<4U;++i) {
        maia_dsi_read_report_t r;
        maia_dsi_access_result_t result=maia_esp32p4_dsi_read_color_mode(10000,&r);
        uint32_t value=r.word&255U;
        field("DPI_COLOR phase=",phase);field(" index=",i);field(" result=",(uint32_t)result);field(" stage=",r.transfer.completed);
        field(" request=",r.transfer.header);field(" headers=",r.headers_attempted);field(" polls=",r.transfer.polls);
        field(" word=",r.word);field(" value=",value);field(" words=",r.words_received);
        field(" before=",r.transfer.command_before);field(" after=",r.transfer.command_after);field(" restored=",r.restore_attempted);
        field(" status=",r.transfer.status);field(" fifo=",r.transfer.fifo);field(" error0=",r.transfer.error0);field(" error1=",r.transfer.error1);maia_console_puts("\n");
        if (result != MAIA_DSI_ACCESS_OK || r.transfer.completed != 10 || r.words_received != 1 || (i && value != first)) halt("dpi-color-read");
        first=value;
    }
    return first;
}
static void dpi_clock(void) {
    uint32_t before=dpi_color(0),attempted=0,header=0;
    if ((before&7U) != 0U && (before&7U)<5U) halt("dpi-color-unsupported");
    if ((before&7U) != 5U) {
        maia_dsi_short_report_t w;attempted=1;
        maia_dsi_access_result_t result=maia_esp32p4_dsi_dcs_short(0,0x3A,1,0x55,10000,&w);header=w.header;
        if (result != MAIA_DSI_ACCESS_OK || st7121_wait(NULL,5)) halt("dpi-color-select");
    }
    uint32_t after=dpi_color(1);
    field("DPI_COLOR_SELECT before=",before);field(" after=",after);field(" parameter=",0x55);
    field(" attempted=",attempted);field(" header=",header);maia_console_puts("\n");
    if ((after&7U) != 5U) halt("dpi-color-verify");
    uint32_t mode=rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34),bridge=rd(MAIA_ESP32P4_DSI_BRG_BASE+4),
        dpi=rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40),update=rd(MAIA_ESP32P4_DSI_BRG_BASE+0x44);
    maia_dsi_dpi_clock_report_t r,again;
    maia_dsi_access_result_t result=maia_esp32p4_dsi_dpi_clock_enable(&r);
    field("DPI_CLOCK result=",(uint32_t)result);field(" stage=",r.completed);field(" attempted=",r.attempted_mask);
    field(" provider_before=",r.provider_before);field(" provider_after=",r.provider_after);
    field(" clock_before=",r.clock_before);field(" clock_after=",r.clock_after);field(" nominal_hz=",r.nominal_hz);
    field(" status=",r.observation.status);field(" fifo=",r.observation.fifo);field(" error0=",r.observation.error0);field(" error1=",r.observation.error1);maia_console_puts("\n");
    if (result != MAIA_DSI_ACCESS_OK || r.completed != 4) halt("dpi-clock-enable");
    int duplicate=maia_esp32p4_dsi_dpi_clock_enable(&again) == MAIA_DSI_ACCESS_BUSY;
    int kept=mode == 1 && !(bridge&1U) && !(dpi&1U) && mode == rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34) &&
        bridge == rd(MAIA_ESP32P4_DSI_BRG_BASE+4) && dpi == rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40) &&
        update == rd(MAIA_ESP32P4_DSI_BRG_BASE+0x44) && vo3_dma_quiet(rd);
    field("DPI_CLOCK_IDLE mode=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34));field(" bridge=",rd(MAIA_ESP32P4_DSI_BRG_BASE+4));
    field(" dpi=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40));field(" update=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x44));
    field(" dma_quiet=",(uint32_t)vo3_dma_quiet(rd));field(" preserved=",(uint32_t)kept);field(" duplicate_busy=",(uint32_t)duplicate);maia_console_puts("\n");
    if (!kept || !duplicate) halt("dpi-clock-idle");
    if (dpi_color(2) != after) halt("dpi-clock-color");
    for (unsigned i=0;i<4U;++i) {
        maia_dsi_read_report_t p;maia_dsi_access_result_t rr=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DPI_CLOCK_REPLY index=",i);field(" result=",(uint32_t)rr);field(" stage=",p.transfer.completed);
        field(" headers=",p.headers_attempted);field(" polls=",p.transfer.polls);field(" word=",p.word);field(" words=",p.words_received);
        field(" before=",p.transfer.command_before);field(" after=",p.transfer.command_after);field(" restored=",p.restore_attempted);
        field(" status=",p.transfer.status);field(" fifo=",p.transfer.fifo);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if (rr != MAIA_DSI_ACCESS_OK || p.transfer.completed != 10 || p.words_received != 1 || (p.word&0x14U) != 0x14U) halt("dpi-clock-power");
    }
    maia_console_puts("MAIA_NATIVE_DPI_CLOCK_PASS\n");
}
