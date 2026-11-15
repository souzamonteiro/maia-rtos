/* Original native MIT ownership/checked transition policy. Register/parameter
 * facts: ESP-IDF f5c3654 mipi_dsi_host_ll_dpi_set_pattern_type,
 * enable_video_mode, set_clock_lane_state and mipi_dsi_host_reg.h.
 * No SDK headers/runtime or vendor control flow imported.
 * Provenance and limits: docs/audits/native-dpi-pattern.md. */
#define DPI_PATTERN_FINISHED 3U
static uint32_t dpi_pattern_word,dpi_pattern_mode,dpi_pattern_lp;
static void pattern_report(maia_dsi_dpi_pattern_report_t *r) {
    r->completed=r->attempted_mask=r->pattern=r->mode=r->lp_clock=r->shadow=0;
    maia_dsi_short_report_t *io=&r->observation;
    io->completed=io->header=io->header_attempted=io->command_before=io->command_after=0;
    io->polls=io->status=io->fifo=io->error0=io->error1=io->saw_busy=0;
}
static int pattern_guard(void) {
    uint32_t phy[5];
    for (unsigned i=0;i<5U;++i) phy[i]=dpi_staged_phy[i];
    phy[3]=dpi_pattern_lp;
    if (!phy_guard(&dpi_staged_guard,phy) || !transfer_bus_guard(&dpi_staged_guard) ||
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x68U) != dpi_staged_mode ||
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x100U) != 0U || !dma_quiet()) return 0;
    for (unsigned i=0;i<MAIA_DSI_COMMAND_WORDS;++i) {
        uint32_t expected=command_offsets[i] == 0x34U ? dpi_pattern_mode :
            command_offsets[i] == 0x94U ? dpi_pattern_lp : saved_command_table[i];
        if (dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+command_offsets[i]) != expected) return 0;
    }
    for (unsigned i=0;i<MAIA_DSI_DPI_WORDS;++i)
        if (dsi_read(dpi_address(i)) != (i == 3U ? dpi_pattern_word : dpi_staged_words[i])) return 0;
    return 1;
}
static maia_dsi_access_result_t pattern_check(maia_dsi_dpi_pattern_report_t *r) {
    if (!pattern_guard()) return MAIA_DSI_ACCESS_VERIFY;
    if (!transfer_observe(&r->observation)) return MAIA_DSI_ACCESS_IO;
    if (!(r->observation.status&1U)) return MAIA_DSI_ACCESS_VERIFY;
    return MAIA_DSI_ACCESS_OK;
}
static maia_dsi_access_result_t pattern_store(maia_dsi_dpi_pattern_report_t *r,unsigned bit,
        uintptr_t address,uint32_t desired,uint32_t *expected) {
    maia_dsi_access_result_t result=pattern_check(r);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    r->attempted_mask|=1U<<bit;dsi_write(address,desired);
    if (!transfer_bus_guard(&dpi_staged_guard)) return MAIA_DSI_ACCESS_VERIFY;
    if (dsi_read(address) != desired) return MAIA_DSI_ACCESS_VERIFY;
    *expected=desired;return pattern_check(r);
}
static void pattern_values(maia_dsi_dpi_pattern_report_t *r) {
    r->pattern=dpi_pattern_word;r->mode=dpi_pattern_mode;r->lp_clock=dpi_pattern_lp;
}
maia_dsi_access_result_t maia_esp32p4_dsi_dpi_pattern_start(maia_dsi_dpi_pattern_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    pattern_report(r);
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (dpi_pattern_state == FAULTED || transfer_state == FAULTED || dpi_clock_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (dpi_pattern_state != UNCLAIMED || transfer_state != UNCLAIMED || dpi_clock_state != OWNED || dpi_state != OWNED) return MAIA_DSI_ACCESS_BUSY;
    dpi_pattern_word=dpi_staged_words[3];dpi_pattern_mode=saved_command_table[0];dpi_pattern_lp=dpi_staged_phy[3];
    if ((dpi_pattern_mode&1U) != 1U || (dpi_pattern_lp&3U) || (dpi_pattern_word&0x14000U)) return MAIA_DSI_ACCESS_VERIFY;
    maia_dsi_access_result_t result=pattern_check(r);
    if (result != MAIA_DSI_ACCESS_OK) {
        if (result == MAIA_DSI_ACCESS_IO) dpi_pattern_state=transfer_state=FAULTED;
        return result;
    }
    if (!read_idle(&r->observation)) return MAIA_DSI_ACCESS_BUSY;
    r->completed=1;dpi_pattern_state=transfer_state=FAULTED;
    result=pattern_store(r,0,MAIA_ESP32P4_DSI_HOST_BASE+0x38U,
        (dpi_pattern_word&~0x1110000U)|0x10000U,&dpi_pattern_word);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    r->completed=2;
    result=pattern_store(r,1,MAIA_ESP32P4_DSI_HOST_BASE+0x94U,dpi_pattern_lp|3U,&dpi_pattern_lp);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    r->completed=3;
    result=pattern_store(r,2,MAIA_ESP32P4_DSI_HOST_BASE+0x34U,dpi_pattern_mode&~1U,&dpi_pattern_mode);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    r->completed=4;pattern_values(r);dpi_pattern_state=transfer_state=OWNED;
    return MAIA_DSI_ACCESS_OK;
}
maia_dsi_access_result_t maia_esp32p4_dsi_dpi_pattern_stop(uint32_t budget,maia_dsi_dpi_pattern_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    pattern_report(r);
    if (!budget || budget>100000U) return MAIA_DSI_ACCESS_INVALID;
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (dpi_pattern_state == FAULTED || transfer_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (dpi_pattern_state != OWNED || transfer_state != OWNED) return MAIA_DSI_ACCESS_BUSY;
    /* A failed stop is never retried, even if a guard prevents the first store. */
    dpi_pattern_state=transfer_state=FAULTED;
    maia_dsi_access_result_t result=pattern_check(r);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    r->completed=1;
    result=pattern_store(r,0,MAIA_ESP32P4_DSI_HOST_BASE+0x34U,saved_command_table[0],&dpi_pattern_mode);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    r->completed=2;
    result=pattern_store(r,1,MAIA_ESP32P4_DSI_HOST_BASE+0x94U,dpi_staged_phy[3],&dpi_pattern_lp);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    r->completed=3;
    for (unsigned i=0;i<budget;++i) {
        result=pattern_check(r);++r->observation.polls;
        if (result != MAIA_DSI_ACCESS_OK) return result;
        if (read_idle(&r->observation)) { r->completed=4;break; }
    }
    if (r->completed != 4U) return MAIA_DSI_ACCESS_TIMEOUT;
    result=pattern_store(r,2,MAIA_ESP32P4_DSI_HOST_BASE+0x38U,dpi_staged_words[3],&dpi_pattern_word);
    if (result != MAIA_DSI_ACCESS_OK) return result;
    if (!read_idle(&r->observation)) return MAIA_DSI_ACCESS_VERIFY;
    r->completed=5;pattern_values(r);dpi_pattern_state=DPI_PATTERN_FINISHED;transfer_state=UNCLAIMED;
    return MAIA_DSI_ACCESS_OK;
}
