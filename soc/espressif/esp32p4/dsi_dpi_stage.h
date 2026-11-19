/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Host/bridge field layout and parameter translation adapted from pinned
 * ESP-IDF f5c3654 HAL dpi timing/color and esp_lcd_panel_dpi setup.
 * LICENSES/Apache-2.0.txt. No SDK headers/runtime. Native staging guards,
 * checked RMW, failure claim and clock-gated policy original MaiaRTOS MIT. */
#include <maia/soc/esp32p4_display.h>
/* Private staged snapshot is ownership evidence, never supplied by a caller. */
static reference_guard_t dpi_staged_guard;
static uint32_t dpi_staged_phy[5],dpi_staged_mode,dpi_staged_words[MAIA_DSI_DPI_WORDS];
static void dpi_save(const reference_guard_t *g,const uint32_t phy[5],uint32_t mode,const uint32_t words[MAIA_DSI_DPI_WORDS]) {
    dpi_staged_guard.cpu=g->cpu;dpi_staged_guard.sys=g->sys;dpi_staged_guard.phy=g->phy;
    dpi_staged_guard.reset=g->reset;dpi_staged_guard.isp=g->isp;dpi_staged_guard.norst=g->norst;
    dpi_staged_guard.divider=g->divider;dpi_staged_guard.lp=g->lp;
    dpi_staged_guard.provider=g->provider;dpi_staged_guard.source=g->source;
    dpi_staged_guard.supply.control=g->supply.control;dpi_staged_guard.supply.analog=g->supply.analog;
    dpi_staged_guard.supply.mac_sys2=g->supply.mac_sys2;dpi_staged_guard.supply.mac_sys3=g->supply.mac_sys3;
    dpi_staged_guard.supply.rs_error0=g->supply.rs_error0;
    for (unsigned i=0;i<5U;++i) dpi_staged_phy[i]=phy[i];
    for (unsigned i=0;i<MAIA_DSI_DPI_WORDS;++i) dpi_staged_words[i]=words[i];
    dpi_staged_mode=mode;
}
static const uint16_t dpi_offsets[MAIA_DSI_DPI_WORDS]={
    0x0C,0x10,0x14,0x38,0x3C,0x40,0x44,0x48,0x4C,0x50,0x54,0x58,0x5C,0x60,
    0x18,0x30,0x34,0x38,0x3C};
static const uint32_t dpi_masks[MAIA_DSI_DPI_WORDS]={
    3,0x10F,0x1F,0xBF03,0x3FFF,0x1FFF,0x1FFF,0xFFF,0xFFF,0x7FFF,0x3FF,0x3FF,0x3FF,0x3FFF,
    0x7F,0x0FFF0FFF,0x0FFF0FFF,0x0FFF0FFF,0x0FFF0FFF};
static uintptr_t dpi_address(unsigned i) {
    return (i<14U ? MAIA_ESP32P4_DSI_HOST_BASE : MAIA_ESP32P4_DSI_BRG_BASE)+dpi_offsets[i];
}
static int dpi_values(const maia_dsi_dpi_timing_t *t,uint32_t v[MAIA_DSI_DPI_WORDS]) {
    if (!t || !t->width || !t->height || !t->hsync || !t->vsync || !t->hfront || !t->vfront ||
        t->width>4095U || t->height>4095U || t->hsync>4095U || t->hback>4095U || t->hfront>4095U ||
        t->vsync>1023U || t->vback>1023U || t->vfront>1023U) return 0;
    /* All inputs now bounded, so sums/products below cannot overflow32 bits.
     * Match SDK's individual float->integer conversions with exact3/2 arithmetic. */
    uint32_t ht=t->width+t->hsync+t->hback+t->hfront,vt=t->height+t->vsync+t->vback+t->vfront;
    if (ht>4095U || vt>4095U || t->hsync*3U/2U>4095U || t->hback*3U/2U>4095U) return 0;
    v[0]=v[1]=v[2]=0;v[3]=MAIA_TAB5_DSI_VIDEO_MODE_CFG;v[4]=t->width;v[5]=v[6]=0;
    v[7]=t->hsync*3U/2U;v[8]=t->hback*3U/2U;
    v[9]=v[7]+v[8]+t->width*3U/2U+t->hfront*3U/2U;
    v[10]=t->vsync;v[11]=t->vback;v[12]=t->vfront;v[13]=t->height;
    v[14]=2;v[15]=(t->height<<16U)|vt;v[16]=(t->vsync<<16U)|t->vback;
    v[17]=(t->width<<16U)|ht;v[18]=(t->hsync<<16U)|t->hback;return 1;
}
static int dpi_guard(const reference_guard_t *g,const uint32_t phy[5],uint32_t mode,const uint32_t expected[MAIA_DSI_DPI_WORDS]) {
    if (!transfer_guard(g,phy,mode)) return 0;
    for (unsigned i=0;i<MAIA_DSI_DPI_WORDS;++i) if (dsi_read(dpi_address(i)) != expected[i]) return 0;
    return 1;
}
maia_dsi_access_result_t maia_esp32p4_dsi_dpi_stage(const maia_dsi_dpi_timing_t *t,maia_dsi_dpi_report_t *r) {
    if (!r) return MAIA_DSI_ACCESS_INVALID;
    r->completed=r->attempted_mask=r->clock_before=r->clock_after=r->nominal_hz=0;
    maia_dsi_short_report_t *io=&r->observation;
    io->completed=io->header=io->header_attempted=io->command_before=io->command_after=0;
    io->polls=io->status=io->fifo=io->error0=io->error1=io->saw_busy=0;
    for (unsigned i=0;i<MAIA_DSI_DPI_WORDS;++i) r->before[i]=r->after[i]=0;
    uint32_t values[MAIA_DSI_DPI_WORDS];
    if (!dpi_values(t,values)) return MAIA_DSI_ACCESS_INVALID;
    if (!dsi_context()) return MAIA_DSI_ACCESS_CONTEXT;
    if (dpi_state == FAULTED || transfer_state == FAULTED) return MAIA_DSI_ACCESS_FAILED;
    if (dpi_state == OWNED || transfer_state == OWNED || command_state != OWNED) return MAIA_DSI_ACCESS_BUSY;
    reference_guard_t g;
    g.cpu=dsi_read(DSI_CPU);g.sys=dsi_read(DSI_SYS);g.phy=dsi_read(DSI_PHY);
    g.reset=dsi_read(DSI_RESET);g.isp=dsi_read(DSI_ISP);g.norst=dsi_read(DSI_NORST);
    g.divider=dsi_read(DSI_REF_DIV);g.lp=dsi_read(DSI_LP_GATE);
    g.provider=dsi_read(DSI_REF_GATE);g.source=dsi_read(DSI_REF_SOURCE);
    if (!(g.cpu&0x2000U) || !(g.sys&0x20U) || (g.sys&0x1C00U) != 0x1000U ||
        (g.phy&0x93U) != 3U || (g.reset&0x3C000000U) != 0x38000000U || (g.norst&0x7800U) ||
        ((g.divider>>16U)&255U) != 23U || (g.lp&0x0A000000U) != 0x0A000000U ||
        !(g.provider&0x100U) || (g.source&0xC0000000U) || !dma_quiet()) return MAIA_DSI_ACCESS_BUSY;
    maia_ldo3_plan_t plan;
    if (maia_esp32p4_ldo3_snapshot(&g.supply) != MAIA_LDO3_OK ||
        maia_esp32p4_ldo3_plan2500(&g.supply,&plan) != MAIA_LDO3_OK ||
        (g.supply.control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (g.supply.analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) !=
        (((uint32_t)plan.dref<<28U)|((uint32_t)plan.mul<<23U)|(1U<<26U))) return MAIA_DSI_ACCESS_BUSY;
    uint32_t phy[5]={dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+4U),dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U),
        dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0xA4U),dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x94U),
        dsi_read(MAIA_ESP32P4_DSI_BRG_BASE+0x40U)};
    uint32_t mode=dsi_read(MAIA_ESP32P4_DSI_HOST_BASE+0x68U);
    if (!(phy[0]&1U) || (phy[1]&15U) != 15U || (phy[2]&3U) != 1U || (phy[3]&3U) || (phy[4]&1U) ||
        (mode&3U) || (saved_command_table[4]&4U) || !transfer_guard(&g,phy,mode)) return MAIA_DSI_ACCESS_BUSY;
    for (unsigned i=0;i<MAIA_DSI_DPI_WORDS;++i) r->before[i]=r->after[i]=dsi_read(dpi_address(i));
    if (r->before[3]&0x14000U) return MAIA_DSI_ACCESS_BUSY; /* No pattern/frame-BTA. */
    if (!dpi_guard(&g,phy,mode,r->after) || !transfer_bus_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    if (!transfer_observe(io)) { dpi_state=transfer_state=FAULTED;return MAIA_DSI_ACCESS_IO; }
    if (!read_idle(io)) return MAIA_DSI_ACCESS_BUSY;
    r->completed=1;dpi_state=transfer_state=FAULTED;
    r->clock_before=r->clock_after=g.phy;
    uint32_t desired=(g.phy&~0xFF60U)|0x220U; /* PLL240/source1, div3-minus1; gate7 stays0. */
    if (desired != g.phy) { r->attempted_mask|=1;dsi_write(DSI_PHY,desired); }
    r->clock_after=dsi_read(DSI_PHY);
    if (r->clock_after != desired) return MAIA_DSI_ACCESS_VERIFY;
    g.phy=desired;r->completed=2;
    for (unsigned i=0;i<MAIA_DSI_DPI_WORDS;++i) {
        if (!dpi_guard(&g,phy,mode,r->after) || !transfer_bus_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
        if (!transfer_observe(io)) return MAIA_DSI_ACCESS_IO;
        if (!read_idle(io)) return MAIA_DSI_ACCESS_VERIFY;
        desired=(r->after[i]&~dpi_masks[i])|values[i];
        if (desired != r->after[i]) { r->attempted_mask|=1U<<(i+1U);dsi_write(dpi_address(i),desired); }
        if (!transfer_bus_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
        r->after[i]=dsi_read(dpi_address(i));
        if (r->after[i] != desired) return MAIA_DSI_ACCESS_VERIFY;
        r->completed=i+3U;
    }
    if (!dpi_guard(&g,phy,mode,r->after) || !transfer_bus_guard(&g)) return MAIA_DSI_ACCESS_VERIFY;
    if (!transfer_observe(io)) return MAIA_DSI_ACCESS_IO;
    if (!read_idle(io)) return MAIA_DSI_ACCESS_VERIFY;
    dpi_save(&g,phy,mode,r->after);
    r->nominal_hz=80000000U;r->completed=22;dpi_state=OWNED;transfer_state=UNCLAIMED;
    return MAIA_DSI_ACCESS_OK;
}
