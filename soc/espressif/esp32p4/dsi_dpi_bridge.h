/* Original MaiaRTOS MIT guarded bridge ownership/publication policy.
 * Register/parameter facts: pinned SDK f5c3654 mipi_dsi_brg_ll_enable,
 * update_dpi_config and mipi_dsi_bridge_reg.h; no SDK headers/runtime/control
 * flow imported. Provenance/WT limits: docs/audits/native-dpi-bridge.md. */
static int bridge_guard(uint32_t expected) {
    return dpi_guard(&dpi_staged_guard,dpi_staged_phy,dpi_staged_mode,dpi_staged_words) &&
        transfer_bus_guard(&dpi_staged_guard) &&
        dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U) == expected &&
        dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x50U) == 0U &&
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x100U) == 0U;
}
static maia_dsi_access_result_t bridge_check(uint32_t expected,maia_dsi_dpi_bridge_report_t *r) {
    if (!bridge_guard(expected)) return MAIA_DSI_ACCESS_VERIFY;
    r->irq_raw|=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x58U);
    if (!transfer_observe(&r->observation) || (r->irq_raw&1U)) return MAIA_DSI_ACCESS_IO;
    if (!read_idle(&r->observation)) return MAIA_DSI_ACCESS_BUSY;
    return MAIA_DSI_ACCESS_OK;
}
maia_dsi_access_result_t maia_esp32p4_dsi_dpi_bridge_prepare(maia_dsi_dpi_bridge_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    r->completed=r->attempted_mask=r->bridge_before=r->bridge_after=r->update_value=r->irq_raw=0;
    maia_dsi_short_report_t *io=&r->observation;
    io->completed=io->header=io->header_attempted=io->command_before=io->command_after=0;
    io->polls=io->status=io->fifo=io->error0=io->error1=io->saw_busy=0;
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (dpi_bridge_state == FAULTED || transfer_state == FAULTED || dpi_clock_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (dpi_bridge_state != UNCLAIMED || dpi_clock_state != OWNED || dpi_state != OWNED ||
        transfer_state != UNCLAIMED || dpi_pattern_state == OWNED) return MAIA_DSI_ACCESS_BUSY;
    /* Root/full stage guards precede any new peripheral register observation. */
    if (!dpi_guard(&dpi_staged_guard,dpi_staged_phy,dpi_staged_mode,dpi_staged_words) ||
        (dpi_staged_words[3]&0x14000U) || dpi_bridge_expected) return MAIA_DSI_ACCESS_VERIFY;
    r->bridge_before=r->bridge_after=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U);
    maia_dsi_access_result_t result=bridge_check(r->bridge_before,r);
    if (result != MAIA_DSI_ACCESS_OK) {
        if (result == MAIA_DSI_ACCESS_IO) dpi_bridge_state=transfer_state=FAULTED;
        return result;
    }
    r->completed=1;dpi_bridge_state=transfer_state=FAULTED;
    uint32_t desired=r->bridge_before|1U;
    r->attempted_mask|=1U;dsi_write(MAIA_ESP32P4_DSI_BRG_BASE+4U,desired);
    if (!transfer_bus_guard(&dpi_staged_guard)) return MAIA_DSI_ACCESS_VERIFY;
    r->bridge_after=dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+4U);
    if (r->bridge_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    dpi_bridge_expected=1U;r->completed=2;
    result=bridge_check(desired,r);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    /* WT publication: never RMW/read/poll/retry the trigger as an ACK. */
    r->attempted_mask|=2U;r->update_value=1U;
    dsi_write(MAIA_ESP32P4_DSI_BRG_BASE+0x44U,1U);r->completed=3;
    result=bridge_check(desired,r);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    r->completed=4;dpi_bridge_state=OWNED;transfer_state=UNCLAIMED;
    return MAIA_DSI_ACCESS_OK;
}
