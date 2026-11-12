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
void maia_boot_main(void) {
    maia_console_puts("MAIA_MIPI_PARK_BOOT\n");
    uint32_t hart,status;__asm__ volatile("csrr %0, mhartid":"=r"(hart));__asm__ volatile("csrr %0, mstatus":"=r"(status));
    if (hart || status&8U) halt("context");
    unsigned host_ready=0;
    for (unsigned i=0;i<10000000U;++i) if (maia_serial_rx_read(NULL) == 'P') { host_ready=1;break; }
    if (!host_ready) halt("host-timeout");
    uint32_t original_sys=rd(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG);
    if ((original_sys&0x1420U) != 0x1420U || !(rd(MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0x14U)&(1U<<13U))) halt("bus-clocks");
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
    maia_console_puts("MAIA_NATIVE_MIPI_PARK_PASS\n");for (;;) __asm__ volatile("wfi");
}
