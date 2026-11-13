/* Native boot reset parking only. Host/analog shutdown must remain asserted. */
#include <maia/console.h>
#include <maia/drivers/serial_rx.h>
#include <maia/soc/esp32p4_mipi_park.h>
#include "native_mipi_snapshot.h"
static uint32_t rd(uintptr_t p) { return MAIA_MMIO32(p); }
static void wr(uintptr_t p,uint32_t v) { MAIA_MMIO32(p)=v;__asm__ volatile("fence iorw, iorw" ::: "memory"); }
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }
static void field(const char *key,uint32_t n) {
    const char digits[]="0123456789abcdef";char b[9];
    for (unsigned i=0;i<8U;++i) b[i]=digits[(n>>(28U-i*4U))&15U];
    b[8]=0;maia_console_puts(key);maia_console_puts(b);
}
static void halt(const char *reason) {
    maia_console_puts("MAIA_NATIVE_MIPI_PARK_FAIL ");maia_console_puts(reason);maia_console_puts("\n");
    for (;;) __asm__ volatile("wfi");
}
#ifdef MAIA_NATIVE_PROBE_DSI_ACCESS
#include <maia/soc/esp32p4_dsi_access.h>
#endif
#ifdef MAIA_NATIVE_PROBE_VO3_CYCLE
#include "native_vo3_cycle.h"
#endif
void maia_boot_main(void) {
    maia_console_puts("MAIA_MIPI_PARK_BOOT\n");
    uint32_t hart,status;__asm__ volatile("csrr %0, mhartid":"=r"(hart));__asm__ volatile("csrr %0, mstatus":"=r"(status));
    if (hart || status&8U) halt("context");
#ifdef MAIA_NATIVE_PROBE_VO3_CYCLE
    extern void maia_soc_trap_init(void);
    maia_soc_trap_init();
#endif
    unsigned host_ready=0;
    for (unsigned i=0;i<10000000U;++i) if (maia_serial_rx_read(NULL) == 'P') { host_ready=1;break; }
    if (!host_ready) halt("host-timeout");
#ifdef MAIA_NATIVE_PROBE_VO3_CYCLE
    uint32_t csi_sys=rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG),csi_phy=rd(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG);
    uint32_t csi_reset=rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U);
    if (!vo3_prepare_csi(rd,wr)) halt("vo3-csi-prepare");
    field("VO3_CSI_PREP sys_before=",csi_sys);field(" sys_after=",rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG));
    field(" phy_before=",csi_phy);field(" phy_after=",rd(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG));
    field(" reset_before=",csi_reset);field(" reset_after=",rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U));maia_console_puts("\n");
#endif
    uint32_t original_sys=rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG);
    if ((original_sys&0x1420U) != 0x1420U || !(rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)&(1U<<13U))) {
        field("MIPI_CLOCK_GUARD sys=",original_sys);field(" cpu=",rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U));
        field(" phy=",rd(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG));maia_console_puts("\n");halt("bus-clocks");
    }
    /* Only CSI host bus access enabled; no PHY/source/DPI clock or reset write. */
    if (!(original_sys&(1U<<11U))) wr(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG,original_sys|(1U<<11U));
    if (rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG) != (original_sys|(1U<<11U))) halt("host-bus-readback");
    mipi_snapshot_t before,armed,after;
    if (!mipi_snapshot(&before,rd) || before.valid != 15U || before.dma[0] || before.dma[1] || before.dma[2] ||
        before.dsi[0]&1U || before.dsi[1]&1U || before.dsi[2]&0x3FFFU || before.dsi[3]&1U || before.dsi[5]&15U ||
        before.csi[0]&1U || before.csi[1]&1U || before.csi[2]&1U || before.csi_bridge&1U) halt("reset-precondition");
    if (rd(MAIA_ESP32P4_GDMA_BASE+0x58U)&1U) halt("dma-reset");
    for (unsigned c=0;c<4U;++c) if (before.channel[c][3]&0x7FFFU) halt("dma-fifo");
    const uintptr_t regs[]={0x500A0804U,0x5009F804U,0x500A00A0U,0x5009F044U};
    const uint32_t values[]={before.dsi[0]|1U,before.csi_bridge|1U,before.dsi[5]|2U,before.csi[2]|1U};
    for (unsigned i=0;i<4U;++i) { wr(regs[i],values[i]);if (rd(regs[i]) != values[i]) halt("arm-readback"); }
    if (!mipi_snapshot(&armed,rd) || armed.dsi[3]&1U || armed.dsi[5]&1U || armed.dsi[1]&1U ||
        armed.csi[0]&1U || armed.csi[1]&1U || armed.dma[0] || armed.dma[1] || armed.dma[2]) halt("armed-live");
    field("MIPI_PARK_SETUP sys_before=",original_sys);field(" sys_ready=",before.sys);
    field(" dsi_phy=",armed.dsi[5]);field(" csi_phy=",armed.csi[2]);maia_console_puts(" armed=0000000f live=00000000\n");
    maia_mipi_park_report_t r,duplicate;
    maia_mipi_park_result_t result=maia_esp32p4_mipi_park_reset(&r);
    int preserved=mipi_snapshot(&after,rd) && before.cpu == after.cpu && before.sys == after.sys && before.phy == after.phy && before.valid == after.valid;
    /* All sampled stable configuration fields must return exactly to baseline.
     * PHY status under reset is not a valid live-lane stop handshake. */
    for (unsigned i=0;i<6U;++i) if (before.dsi[i] != after.dsi[i]) preserved=0;
    for (unsigned i=0;i<3U;++i) if (before.csi[i] != after.csi[i]) preserved=0;
    if (before.csi_bridge != after.csi_bridge) preserved=0;
    for (unsigned i=0;i<3U;++i) if (before.dma[i] != after.dma[i]) preserved=0;
    for (unsigned c=0;c<4U;++c) for (unsigned i=0;i<7U;++i) if (before.channel[c][i] != after.channel[c][i]) preserved=0;
    int busy=maia_esp32p4_mipi_park_reset(&duplicate) == MAIA_MIPI_PARK_BUSY;
    field("MIPI_PARK result=",(uint32_t)(-result));field(" stage=",r.completed);field(" attempted=",r.attempted_mask);
    field(" dsi_bridge=",r.dsi_bridge);field(" csi_bridge=",r.csi_bridge);field(" dsi_phy=",r.dsi_phy);field(" csi_phy=",r.csi_phy);
    field(" preserved=",(uint32_t)preserved);field(" duplicate_busy=",(uint32_t)busy);maia_console_puts("\n");
    if (result != MAIA_MIPI_PARK_OK || r.completed != MAIA_MIPI_PARK_VERIFIED || r.attempted_mask != 15U || !preserved || !busy) halt("verification");
    maia_console_puts("MAIA_NATIVE_MIPI_PARK_PASS\n");
#ifdef MAIA_NATIVE_PROBE_MIPI_GATE
    /* Parking API has committed its lifetime claim. Read RCC-only ISP state
     * before requesting gate; no ISP MMIO through its disabled clock. */
    field("MIPI_ISP clock=",rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xA8U));
    field(" reset=",rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U));
    field(" norst=",rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xCCU));maia_console_puts("\n");
    maia_mipi_gate_report_t g,again;
    result=maia_esp32p4_mipi_gate_clocks(&g);
    field("MIPI_ISP_STATE inspected=",g.isp_inspected);field(" control=",g.isp_control);field(" camera=",g.isp_camera);maia_console_puts("\n");
    int gate_busy=maia_esp32p4_mipi_gate_clocks(&again) == MAIA_MIPI_PARK_BUSY;
    /* Do not snapshot MIPI after clocks go off. Only RCC and VDMA remain legal. */
    int kept=rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U) == before.cpu &&
        rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xA8U) == g.isp_clock &&
        rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xCCU) == g.force_norst;
    const unsigned dma_regs[]={0x10,0x18,0x1C};
    for (unsigned i=0;i<3U;++i) if (rd(MAIA_ESP32P4_GDMA_BASE+dma_regs[i]) != before.dma[i]) kept=0;
    for (unsigned c=0;c<4U;++c) {
        const unsigned offsets[]={0x20,0x24,0x30,0x34,0,8,0x28};
        for (unsigned i=0;i<7U;++i) if (rd(MAIA_ESP32P4_GDMA_BASE+0x100U*(c+1U)+offsets[i]) != before.channel[c][i]) kept=0;
    }
    field("MIPI_GATE result=",(uint32_t)(-result));field(" stage=",g.completed);field(" attempted=",g.attempted_mask);
    field(" reset_before=",g.reset_before);field(" reset_after=",g.reset_after);
    field(" phy_before=",g.phy_before);field(" phy_after=",g.phy_after);
    field(" sys_before=",g.sys_before);field(" sys_after=",g.sys_after);
    field(" preserved=",(uint32_t)kept);field(" duplicate_busy=",(uint32_t)gate_busy);maia_console_puts("\n");
    if (result != MAIA_MIPI_PARK_OK || g.completed != MAIA_MIPI_GATE_VERIFIED || !kept || !gate_busy) halt("gate");
    maia_console_puts("MAIA_NATIVE_MIPI_GATE_PASS\n");
#ifdef MAIA_NATIVE_PROBE_VO3_CYCLE
    vo3_cycle(&g,before.cpu);
#ifdef MAIA_NATIVE_PROBE_DSI_ACCESS
    maia_dsi_access_report_t access,access_again;
    maia_dsi_access_result_t opened=maia_esp32p4_dsi_open_config(&access);
    int repeated=opened == MAIA_DSI_ACCESS_OK && maia_esp32p4_dsi_open_config(&access_again) == MAIA_DSI_ACCESS_BUSY;
    field("DSI_ACCESS result=",(uint32_t)opened);field(" stage=",access.completed);field(" attempted=",access.attempted_mask);
    field(" sys_before=",access.sys_before);field(" sys_after=",access.sys_after);
    field(" phy_before=",access.phy_before);field(" phy_after=",access.phy_after);
    field(" reset_before=",access.reset_before);field(" reset_after=",access.reset_after);
    field(" control=",access.control);field(" analog=",access.analog);field(" duplicate_busy=",(uint32_t)repeated);maia_console_puts("\n");
    field("DSI_CONFIG version=",access.version);field(" power=",access.power);field(" phy_reset=",access.phy_reset);
    field(" bridge=",access.bridge);field(" dpi=",access.dpi);field(" fifo=",access.fifo);field(" status=",access.phy_status);maia_console_puts("\n");
    if (opened != MAIA_DSI_ACCESS_OK || access.completed != MAIA_DSI_ACCESS_VERIFIED || !repeated) halt("dsi-access");
    maia_console_puts("MAIA_NATIVE_DSI_ACCESS_PASS\n");
#ifdef MAIA_NATIVE_PROBE_DSI_REFERENCE
    maia_ldo3_snapshot_t supplied,observed;
    if (maia_esp32p4_ldo3_snapshot(&supplied) != MAIA_LDO3_OK) halt("reference-snapshot");
    maia_dsi_reference_report_t reference,ref_again;
    maia_dsi_access_result_t routed=maia_esp32p4_dsi_reference20(&reference);
    if (routed != MAIA_DSI_ACCESS_OK) {
        field("DSI_REFERENCE_FAILURE result=",(uint32_t)routed);field(" stage=",reference.completed);
        field(" attempted=",reference.attempted_mask);maia_console_puts("\n");halt("reference");
    }
    int ref_busy=routed == MAIA_DSI_ACCESS_OK && maia_esp32p4_dsi_reference20(&ref_again) == MAIA_DSI_ACCESS_BUSY;
    int ref_kept=1;
    for (unsigned sample=0;sample<16U;++sample) {
        if (rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x28U) != reference.divider_word ||
            rd(0x50111040U) != reference.lp_gate || rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x2CU) != reference.provider_after ||
            rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x38U) != reference.source_after ||
            rd(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG) != reference.phy_after ||
            rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG) != access.sys_after ||
            rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xC0U) != access.reset_after ||
            maia_esp32p4_ldo3_snapshot(&observed) != MAIA_LDO3_OK || !vo3_same(&supplied,&observed)) ref_kept=0;
    }
    field("DSI_REFERENCE result=",(uint32_t)routed);field(" stage=",reference.completed);field(" attempted=",reference.attempted_mask);
    field(" divider=",reference.divider_word);field(" lp_gate=",reference.lp_gate);
    field(" provider_before=",reference.provider_before);field(" provider_after=",reference.provider_after);
    field(" source_before=",reference.source_before);field(" source_after=",reference.source_after);
    field(" phy_before=",reference.phy_before);field(" phy_after=",reference.phy_after);field(" nominal_hz=",reference.nominal_hz);
    field(" preserved=",(uint32_t)ref_kept);field(" duplicate_busy=",(uint32_t)ref_busy);maia_console_puts(" samples=00000010\n");
    field("DSI_REFERENCE_STATE power=",rd(MAIA_ESP32P4_DSI_HOST_BASE+4U));
    field(" phy_reset=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0xA0U));field(" bridge=",rd(MAIA_ESP32P4_DSI_BRG_BASE+4U));
    field(" dpi=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x40U));field(" fifo=",rd(MAIA_ESP32P4_DSI_BRG_BASE+0x14U));
    field(" status=",rd(MAIA_ESP32P4_DSI_HOST_BASE+0xB0U));maia_console_puts("\n");
    if (routed != MAIA_DSI_ACCESS_OK || reference.completed != MAIA_DSI_REFERENCE_VERIFIED || !ref_kept || !ref_busy) halt("reference");
    maia_console_puts("MAIA_NATIVE_DSI_REFERENCE_PASS\n");
#endif
#endif
#endif
#endif
    for (;;) __asm__ volatile("wfi");
}
