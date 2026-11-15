/* Original native MIT guards/ownership; gate fields follow pinned ESP-IDF
 * f5c3654 clk_gate_ll_ref_240m_clk_en/mipi_dsi_ll_enable_dpi_clock.
 * Source facts/limits: docs/audits/native-dpi-clock.md. No SDK headers/runtime. */
maia_dsi_access_result_t maia_esp32p4_dsi_dpi_clock_enable(maia_dsi_dpi_clock_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    r->completed=r->attempted_mask=r->provider_before=r->provider_after=r->clock_before=r->clock_after=r->nominal_hz=0;
    maia_dsi_short_report_t *io=&r->observation;
    io->completed=io->header=io->header_attempted=io->command_before=io->command_after=0;
    io->polls=io->status=io->fifo=io->error0=io->error1=io->saw_busy=0;
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (dpi_clock_state == FAULTED || dpi_state == FAULTED || transfer_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (dpi_clock_state == OWNED || dpi_state != OWNED || transfer_state == OWNED) return MAIA_DSI_ACCESS_BUSY;
    reference_guard_t *g=&dpi_staged_guard;
    if ((g->phy&0xFFF3U) != 0x223U || (dpi_staged_words[3]&0x14000U) ||
        !dpi_guard(g,dpi_staged_phy,dpi_staged_mode,dpi_staged_words) || !transfer_bus_guard(g)) return MAIA_DSI_ACCESS_VERIFY;
    if (!transfer_observe(io)) { dpi_clock_state=transfer_state=FAULTED;return MAIA_DSI_ACCESS_IO; }
    if (!read_idle(io)) return MAIA_DSI_ACCESS_BUSY;
    r->completed=1;dpi_clock_state=transfer_state=FAULTED;
    r->provider_before=r->provider_after=g->divider;r->clock_before=r->clock_after=g->phy;
    uint32_t desired=g->divider|0x40000000U;
    if (desired != g->divider) { r->attempted_mask|=1;dsi_write(DSI_REF_DIV,desired); }
    r->provider_after=dsi_read(DSI_REF_DIV);
    if (r->provider_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    g->divider=desired;r->completed=2;
    if (!dpi_guard(g,dpi_staged_phy,dpi_staged_mode,dpi_staged_words) || !transfer_bus_guard(g)) return MAIA_DSI_ACCESS_VERIFY;
    if (!transfer_observe(io)) return MAIA_DSI_ACCESS_IO;
    if (!read_idle(io)) return MAIA_DSI_ACCESS_VERIFY;
    desired=g->phy|0x80U;r->attempted_mask|=2;dsi_write(DSI_PHY,desired);
    r->clock_after=dsi_read(DSI_PHY);
    if (r->clock_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    g->phy=desired;r->completed=3;
    if (!dpi_guard(g,dpi_staged_phy,dpi_staged_mode,dpi_staged_words) || !transfer_bus_guard(g)) return MAIA_DSI_ACCESS_VERIFY;
    if (!transfer_observe(io)) return MAIA_DSI_ACCESS_IO;
    if (!read_idle(io)) return MAIA_DSI_ACCESS_VERIFY;
    r->nominal_hz=80000000U;r->completed=4;dpi_clock_state=OWNED;transfer_state=UNCLAIMED;
    return MAIA_DSI_ACCESS_OK;
}
