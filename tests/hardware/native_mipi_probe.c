/* Original MaiaRTOS MIT: read-only baseline; opt-in empty-controller stop. */
#include <maia/console.h>
#include "native_mipi_snapshot.h"
#ifdef MAIA_NATIVE_PROBE_VDMA_STOP
#include <maia/soc/esp32p4_vdma.h>
#ifdef MAIA_NATIVE_PROBE_VDMA_ARM_EMPTY
static void write32(uintptr_t address,uint32_t value) {
    MAIA_MMIO32(address)=value;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}
#endif
#endif
/* Diagnostic console only; do not touch inherited display/DMA. */
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }
static uint32_t read32(uintptr_t address) { return MAIA_MMIO32(address); }
static void field(const char *name,uint32_t value) {
    const char hex[]="0123456789abcdef";char text[9];
    for (unsigned i=0;i<8U;++i) text[i]=hex[(value>>(28U-4U*i))&15U];
    text[8]=0;maia_console_puts(name);maia_console_puts(text);
}
#ifdef MAIA_NATIVE_PROBE_VDMA_STOP
static int stop_dma(void) {
    mipi_snapshot_t before,after;
    if (!mipi_snapshot(&before,read32) || !(before.valid&MIPI_DMA) || before.dma[0] ||
        before.dma[1] || before.dma[2] || (read32(MAIA_ESP32P4_GDMA_BASE+0x58U)&1U)) return 0;
    for (unsigned c=0;c<4U;++c) {
        if (before.channel[c][2] || before.channel[c][3] || before.channel[c][4] ||
            before.channel[c][5] || before.channel[c][6]) return 0;
    }
    unsigned armed=0;
#ifdef MAIA_NATIVE_PROBE_VDMA_ARM_EMPTY
    /* Explicit empty-controller test only: channels remain disabled throughout.
     * No SAR/DAR/LLP/config writes and no DMA transfer is initiated. */
    write32(MAIA_ESP32P4_GDMA_BASE+0x10U,1U);
    if (read32(MAIA_ESP32P4_GDMA_BASE+0x10U) != 1U ||
        read32(MAIA_ESP32P4_GDMA_BASE+0x18U) != 0U) return 0;
    armed=1;
#endif
    maia_vdma_stop_report_t r,duplicate;
    maia_vdma_result_t result=maia_esp32p4_vdma_stop_all(1024U,&r);
    if (result != MAIA_VDMA_OK) {
        field("VDMA_STOP error=",(uint32_t)(-result));maia_console_puts("\n");return 0;
    }
    int busy=maia_esp32p4_vdma_stop_all(1024U,&duplicate) == MAIA_VDMA_BUSY;
    int preserved=mipi_snapshot(&after,read32) && before.cpu == after.cpu &&
        before.sys == after.sys && before.phy == after.phy && before.valid == after.valid &&
        before.csi_bridge == after.csi_bridge;
    for (unsigned i=0;i<7U;++i) if (before.dsi[i] != after.dsi[i]) preserved=0;
    for (unsigned i=0;i<4U;++i) if (before.csi[i] != after.csi[i]) preserved=0;
    for (unsigned i=0;i<3U;++i) if (before.dma[i] != after.dma[i]) preserved=0;
    for (unsigned c=0;c<4U;++c) for (unsigned i=0;i<7U;++i)
        if (before.channel[c][i] != after.channel[c][i]) preserved=0;
    field("VDMA_STOP armed=",armed);field(" stage=",r.completed);field(" adopted=",r.adopted);
    field(" changed=",r.may_have_changed);field(" requested=",r.requested_channels);field(" polls=",r.polls);
    field(" cfg=",r.cfg);field(" channels=",r.channels);field(" abort=",r.abort);field(" fifo=",r.fifo_nonempty);
    field(" preserved=",(uint32_t)preserved);field(" duplicate_busy=",(uint32_t)busy);maia_console_puts("\n");
    return r.completed == MAIA_VDMA_STOP_VERIFIED && r.adopted == !armed &&
        r.may_have_changed == armed && !r.requested_channels && r.polls == 1U &&
        !r.cfg && !r.channels && !r.abort && !r.fifo_nonempty && preserved && busy;
}
#endif
void maia_boot_main(void) {
    uint32_t hart,status;__asm__ volatile("csrr %0, mhartid":"=r"(hart));
    __asm__ volatile("csrr %0, mstatus":"=r"(status));
#ifdef MAIA_NATIVE_PROBE_VDMA_STOP
    maia_console_puts("MAIA_VDMA_STOP_PROBE optional empty-controller writes; no transfers\n");
#endif
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
#ifdef MAIA_NATIVE_PROBE_VDMA_STOP
    if (!errors && !stop_dma()) ++errors;
#endif
    maia_console_puts(errors ? "MAIA_NATIVE_MIPI_FAIL unstable-clocks\n" : "MAIA_NATIVE_MIPI_PASS\n");
    for (;;) __asm__ volatile("wfi");
}
