/* Native boot bridge preparation only, no pixels/DMA or WT ACK inference. */
static void dpi_bridge(void) {
    uint32_t mode=rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34U),lp=rd(MAIA_ESP32P4_DSI_HOST_BASE+0x94U),
        dpi=rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U);
    maia_dsi_dpi_bridge_report_t r,again;
    maia_dsi_access_result_t result=maia_esp32p4_dsi_dpi_bridge_prepare(&r);
    field("DPI_BRIDGE_REQUEST result=",(uint32_t)result);field(" stage=",r.completed);field(" attempted=",r.attempted_mask);
    field(" before=",r.bridge_before);field(" after=",r.bridge_after);field(" update_value=",r.update_value);field(" irq_raw=",r.irq_raw);
    field(" status=",r.observation.status);field(" fifo=",r.observation.fifo);field(" error0=",r.observation.error0);field(" error1=",r.observation.error1);maia_console_puts("\n");
    if (result != MAIA_DSI_ACCESS_OK || r.completed != 4U) halt("dpi-bridge-request");
    int duplicate=maia_esp32p4_dsi_dpi_bridge_prepare(&again) == MAIA_DSI_ACCESS_BUSY;
    maia_dsi_dpi_pattern_report_t pattern;
    int vpg=maia_esp32p4_dsi_dpi_pattern_start(&pattern) == MAIA_DSI_ACCESS_BUSY;
    int kept=mode == 1U && !(lp&3U) && !(dpi&1U) && mode == rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34U) &&
        lp == rd(MAIA_ESP32P4_DSI_HOST_BASE+0x94U) && dpi == rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U) &&
        rd(MAIA_ESP32P4_DSI_BRG_BASE+4U) == r.bridge_after && vo3_dma_quiet(rd);
    field("DPI_BRIDGE_IDLE bridge=",rd(MAIA_ESP32P4_DSI_BRG_BASE+4U));field(" dpi=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U));
    field(" mode=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x34U));field(" lp=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0x94U));
    field(" dma_quiet=",(uint32_t)vo3_dma_quiet(rd));field(" preserved=",(uint32_t)kept);
    field(" duplicate_busy=",(uint32_t)duplicate);field(" vpg_busy=",(uint32_t)vpg);field(" latch_verified=",0);maia_console_puts("\n");
    if (!kept || !duplicate || !vpg) halt("dpi-bridge-idle");
    for (unsigned i=0;i<4U;++i) {
        maia_dsi_read_report_t p;result=maia_esp32p4_dsi_read_power_mode(10000,&p);
        field("DPI_BRIDGE_REPLY index=",i);field(" result=",(uint32_t)result);field(" stage=",p.transfer.completed);
        field(" headers=",p.headers_attempted);field(" polls=",p.transfer.polls);field(" word=",p.word);field(" words=",p.words_received);
        field(" before=",p.transfer.command_before);field(" after=",p.transfer.command_after);field(" restored=",p.restore_attempted);
        field(" status=",p.transfer.status);field(" fifo=",p.transfer.fifo);field(" error0=",p.transfer.error0);field(" error1=",p.transfer.error1);maia_console_puts("\n");
        if (result != MAIA_DSI_ACCESS_OK || p.transfer.completed != 10U || p.words_received != 1U || (p.word&0x14U) != 0x14U)
            halt("dpi-bridge-reply");
    }
    maia_console_puts("MAIA_NATIVE_DPI_BRIDGE_PASS\n");
}
