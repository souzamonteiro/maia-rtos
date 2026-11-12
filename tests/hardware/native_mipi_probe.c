/* Original MaiaRTOS MIT diagnostic. Only reads MIPI/DW-GDMA state. */
#include <maia/console.h>
#include "native_mipi_snapshot.h"
/* Diagnostic console only; do not touch inherited display/DMA. */
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }
static uint32_t read32(uintptr_t address) { return MAIA_MMIO32(address); }
static void field(const char *name,uint32_t value) {
    const char hex[]="0123456789abcdef";char text[9];
    for (unsigned i=0;i<8U;++i) text[i]=hex[(value>>(28U-4U*i))&15U];
    text[8]=0;maia_console_puts(name);maia_console_puts(text);
}
void maia_boot_main(void) {
    uint32_t hart,status;__asm__ volatile("csrr %0, mhartid":"=r"(hart));
    __asm__ volatile("csrr %0, mstatus":"=r"(status));
    maia_console_puts("MAIA_MIPI_STATE_PROBE schema=1 read-only\n");
    if (hart || (status&8U)) { maia_console_puts("MAIA_NATIVE_MIPI_FAIL context\n");for (;;) __asm__ volatile("wfi"); }
    unsigned errors=0;
    for (unsigned n=0;n<8U;++n) {
        mipi_snapshot_t s;int stable=mipi_snapshot(&s,read32);if (!stable) ++errors;
        field("MIPI_SAMPLE i=",n);field(" cpu=",s.cpu);field(" sys=",s.sys);field(" phy=",s.phy);
        field(" cpu_after=",s.cpu_after);field(" sys_after=",s.sys_after);field(" phy_after=",s.phy_after);
        field(" valid=",s.valid);field(" stable=",(uint32_t)stable);maia_console_puts("\n");
        const char *dsi_names[]={" bridge="," dpi="," fifo="," power="," mode="," reset="," status="};
        field("MIPI_DSI i=",n);for (unsigned i=0;i<7U;++i) field(dsi_names[i],s.dsi[i]);maia_console_puts("\n");
        field("MIPI_CSI i=",n);field(" host=",s.csi[0]);field(" shutdown=",s.csi[1]);field(" reset=",s.csi[2]);
        field(" stop=",s.csi[3]);field(" bridge=",s.csi_bridge);maia_console_puts("\n");
        field("MIPI_DMA i=",n);field(" cfg=",s.dma[0]);field(" enable=",s.dma[1]);field(" abort=",s.dma[2]);maia_console_puts("\n");
        const char *ch_names[]={" cfg0="," cfg1="," transferred="," fifo="," sar="," dar="," llp="};
        for (unsigned c=0;c<4U;++c) {
            field("MIPI_CH i=",n);field(" ch=",c);for (unsigned i=0;i<7U;++i) field(ch_names[i],s.channel[c][i]);maia_console_puts("\n");
        }
    }
    maia_console_puts(errors ? "MAIA_NATIVE_MIPI_FAIL unstable-clocks\n" : "MAIA_NATIVE_MIPI_PASS\n");
    for (;;) __asm__ volatile("wfi");
}
