/* Read-only diagnostic; standard DCS RDDPM bit4=SleepOut, bit2=DisplayOn.
 * Sources and ST7121/P qualification: docs/audits/native-panel-power.md. */
static uint32_t panel_power_sample(unsigned phase) {
    uint32_t first=0;
    for (unsigned i=0;i<4U;++i) {
        maia_dsi_read_report_t r;
        maia_dsi_access_result_t result=maia_esp32p4_dsi_read_power_mode(10000,&r);
        uint32_t value=r.word&255U;
        field("PANEL_POWER phase=",phase);field(" index=",i);field(" result=",(uint32_t)result);
        field(" stage=",r.transfer.completed);field(" request=",r.transfer.header);
        field(" headers=",r.headers_attempted);field(" polls=",r.transfer.polls);
        field(" word=",r.word);field(" value=",value);field(" words=",r.words_received);
        field(" before=",r.transfer.command_before);field(" after=",r.transfer.command_after);
        field(" restored=",r.restore_attempted);field(" status=",r.transfer.status);field(" fifo=",r.transfer.fifo);
        field(" error0=",r.transfer.error0);field(" error1=",r.transfer.error1);maia_console_puts("\n");
        if (result != MAIA_DSI_ACCESS_OK || r.transfer.completed != 10U || r.words_received != 1U ||
            (i && value != first)) halt("panel-power-response");
        first=value;
    }
    return first;
}
