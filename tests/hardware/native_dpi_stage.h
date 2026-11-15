/* BSP timing in a private Tab5/ST7121 diagnostic; no geometry in generic kernel. */
static void dpi_stage(void) {
    static const maia_dsi_dpi_timing_t timing={720,1280,2,40,40,20,24,200};
    uint32_t mode=rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34),bridge=rd(MAIA_ESP32P4_DSI_BRG_BASE+4),
        dpi=rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40),update=rd(MAIA_ESP32P4_DSI_BRG_BASE+0x44),
        sys=rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG),reset=rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0);
    maia_ldo3_snapshot_t supply,now;
    if (maia_esp32p4_ldo3_snapshot(&supply) != MAIA_LDO3_OK) halt("dpi-supply");
    maia_dsi_dpi_report_t r,again;
    maia_dsi_access_result_t result=maia_esp32p4_dsi_dpi_stage(&timing,&r);
    field("DSI_DPI_STAGE result=",(uint32_t)result);field(" stage=",r.completed);field(" attempted=",r.attempted_mask);
    field(" clock_before=",r.clock_before);field(" clock_after=",r.clock_after);field(" nominal_hz=",r.nominal_hz);
    field(" status=",r.observation.status);field(" fifo=",r.observation.fifo);
    field(" error0=",r.observation.error0);field(" error1=",r.observation.error1);maia_console_puts("\n");
    for (unsigned i=0;i<MAIA_DSI_DPI_WORDS;++i) {
        field("DSI_DPI_WORD index=",i);field(" before=",r.before[i]);field(" after=",r.after[i]);maia_console_puts("\n");
    }
    if (result != MAIA_DSI_ACCESS_OK || r.completed != 22) halt("dpi-stage");
    int duplicate=maia_esp32p4_dsi_dpi_stage(&timing,&again) == MAIA_DSI_ACCESS_BUSY;
    int kept=mode == 1 && !(bridge&1U) && !(dpi&1U) && mode == rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34) &&
        bridge == rd(MAIA_ESP32P4_DSI_BRG_BASE+4) && dpi == rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40) &&
        update == rd(MAIA_ESP32P4_DSI_BRG_BASE+0x44) && sys == rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG) &&
        reset == rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0) &&
        maia_esp32p4_ldo3_snapshot(&now) == MAIA_LDO3_OK && vo3_same(&supply,&now);
    field("DSI_DPI_IDLE mode=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34));field(" bridge=",rd(MAIA_ESP32P4_DSI_BRG_BASE+4));
    field(" dpi=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40));field(" clock=",rd(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG));
    field(" update=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x44));field(" dma_quiet=",(uint32_t)vo3_dma_quiet(rd));
    field(" preserved=",(uint32_t)kept);field(" duplicate_busy=",(uint32_t)duplicate);maia_console_puts("\n");
    if (!kept || !duplicate || !vo3_dma_quiet(rd)) halt("dpi-idle");
    uint32_t first=0;
    for (unsigned i=0;i<4U;++i) {
        maia_dsi_read_report_t p;
        maia_dsi_access_result_t rr=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DSI_DPI_REPLY index=",i);field(" result=",(uint32_t)rr);field(" stage=",p.transfer.completed);
        field(" headers=",p.headers_attempted);field(" polls=",p.transfer.polls);field(" word=",p.word);field(" words=",p.words_received);
        field(" before=",p.transfer.command_before);field(" after=",p.transfer.command_after);field(" restored=",p.restore_attempted);
        field(" status=",p.transfer.status);field(" fifo=",p.transfer.fifo);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if (rr != MAIA_DSI_ACCESS_OK || p.transfer.completed != 10U || p.words_received != 1U ||
            (p.word&0x14U) != 0x14U || (i && (p.word&255U) != first)) halt("dpi-response");
        first=p.word&255U;
    }
    maia_console_puts("MAIA_NATIVE_DPI_STAGE_PASS\n");
}
