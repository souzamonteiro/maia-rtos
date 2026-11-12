/* Original MaiaRTOS MIT diagnostic. Only channel0, owned SRAM, no peripherals. */
#include <maia/console.h>
#include <maia/soc/esp32p4_vdma.h>
#include "vdma_transfer_fixture.h"
#include "native_mipi_snapshot.h"
static unsigned char reserved[DMA_FIXTURE_TOTAL] __attribute__((section(".maia_dma_probe"),aligned(128),used));
extern unsigned char __maia_dma_start[],__maia_dma_end[],__bss_end[],__stack_bottom[];
extern int maia_probe_cache_writeback_invalidate(uint32_t,uint32_t,uint32_t);
extern uint32_t maia_probe_cache_line_size(void);
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }
static uintptr_t last_reg;static uint32_t last_word,setup_stage;
static uint32_t rd(uintptr_t p) { last_reg=p;last_word=MAIA_MMIO32(p);return last_word; }
static void wr(uintptr_t p,uint32_t v) { MAIA_MMIO32(p)=v;__asm__ volatile("fence iorw, iorw" ::: "memory"); }
static void fence(void) { __asm__ volatile("fence iorw, iorw" ::: "memory"); }
static void field(const char *key,uint32_t n) {
    const char digits[]="0123456789abcdef";char b[9];
    for (unsigned i=0;i<8U;++i) b[i]=digits[(n>>(28U-i*4U))&15U];
    b[8]=0;maia_console_puts(key);maia_console_puts(b);
}
static void halt(const char *reason) {
    maia_console_puts("MAIA_NATIVE_VDMA_TRANSFER_FAIL ");maia_console_puts(reason);maia_console_puts("\n");
    for (;;) __asm__ volatile("wfi");
}
static void fill(volatile unsigned char *mem,unsigned seed) {
    for (unsigned i=0;i<DMA_FIXTURE_TOTAL;++i) mem[i]=0xA5;
    for (unsigned i=0;i<DMA_FIXTURE_BYTES;++i) {
        mem[DMA_FIXTURE_GUARD+i]=(unsigned char)dma_fixture_pattern(i,seed);
        mem[DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD+i]=0;
    }
    fence();
}
static int guards(volatile unsigned char *mem,unsigned seed) {
    for (unsigned i=0;i<DMA_FIXTURE_BYTES;++i)
        if (mem[DMA_FIXTURE_GUARD+i] != dma_fixture_pattern(i,seed)) return 0;
    for (unsigned block=0;block<2U;++block) for (unsigned i=0;i<DMA_FIXTURE_GUARD;++i)
        if (mem[block*DMA_FIXTURE_SPAN+i] != 0xA5 ||
            mem[block*DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD+DMA_FIXTURE_BYTES+i] != 0xA5) return 0;
    return 1;
}
static int start(uint32_t base) {
    setup_stage=0;
    if (rd(DMA_CFG) != 0U || rd(DMA_CHEN) != 0U || rd(DMA_ABORT) || rd(DMA_RESET)&1U) return 0;
    setup_stage=1;
    if (!dma_fixture_configure(base,rd,wr)) return 0;
    setup_stage=2;
    /* Clear only documented completion/disabled flags for owned channel0. */
    wr(DMA_CH0+0x98U,0x4000001BU);
    if (rd(DMA_CH0+0x88U) || rd(DMA_COMMON_STATUS)) return 0;
    setup_stage=3;
    wr(DMA_CFG,1U);if (rd(DMA_CFG) != 1U) return 0;
    wr(DMA_CHEN,0x101U);return 1;
}
void maia_boot_main(void) {
    maia_console_puts("MAIA_VDMA_TRANSFER_BOOT\n");
    uint32_t hart,status;__asm__ volatile("csrr %0, mhartid":"=r"(hart));
    __asm__ volatile("csrr %0, mstatus":"=r"(status));
    if (hart || status&8U) halt("context");
    uint32_t begin=(uint32_t)(uintptr_t)__maia_dma_start,end=(uint32_t)(uintptr_t)__maia_dma_end;
    if (!dma_fixture_layout(begin,end,(uint32_t)(uintptr_t)__bss_end,(uint32_t)(uintptr_t)__stack_bottom) ||
        (uint32_t)(uintptr_t)reserved != begin) halt("layout");
    mipi_snapshot_t before,after;
    if (!mipi_snapshot(&before,rd) || !(before.valid&MIPI_DMA) || before.dma[0] || before.dma[1] ||
        before.dma[2] || rd(DMA_RESET)&1U || rd(DMA_COMMON_STATUS)) halt("initial-state");
    /* Purge old cache aliases before taking ownership, then access only NC.
     * Never touch the cached reservation, including through .bss startup. */
    uint32_t line=maia_probe_cache_line_size();
    if (!(line == 16U || line == 32U || line == 64U || line == 128U) ||
        maia_probe_cache_writeback_invalidate(0x10U,begin,DMA_FIXTURE_TOTAL)) halt("cache-prepare");
    fence();
    volatile unsigned char *mem=(volatile unsigned char *)(uintptr_t)(begin+DMA_FIXTURE_ALIAS);
    volatile unsigned char *dst=mem+DMA_FIXTURE_SPAN+DMA_FIXTURE_GUARD;
    field("DMA_MEMORY start=",begin);field(" end=",end);field(" alias=",begin+DMA_FIXTURE_ALIAS);
    field(" bytes=",DMA_FIXTURE_BYTES);field(" cache_line=",line);maia_console_puts(" prepared=00000001\n");
    fill(mem,0);
    if (!start(begin)) {
        field("DMA_SETUP stage=",setup_stage);field(" address=",(uint32_t)last_reg);field(" observed=",last_word);maia_console_puts("\n");
        halt("copy-start");
    }
    unsigned done=0;
    for (unsigned i=0;i<1000000U;++i) if (!(rd(DMA_CHEN)&1U)) { done=1;break; }
    if (!done) halt("copy-timeout");
    wr(DMA_CFG,0U);
    done=0;for (unsigned i=0;i<4096U;++i) if (rd(DMA_CFG) == 0U) { done=1;break; }
    if (!done) halt("copy-global-timeout");
    fence();
    unsigned matched=0;
    for (unsigned i=0;i<DMA_FIXTURE_BYTES;++i) if (dst[i] == dma_fixture_pattern(i,0)) ++matched;
    field("DMA_COPY matched=",matched);field(" status=",rd(DMA_CH0+0x88U));field(" common=",rd(DMA_COMMON_STATUS));maia_console_puts("\n");
    if (matched != DMA_FIXTURE_BYTES || !guards(mem,0) || (rd(DMA_CH0+0x88U)&DMA_FIXTURE_ERRORS) || rd(DMA_COMMON_STATUS)) halt("copy-data");
    maia_console_puts("MAIA_VDMA_TRANSFER_PROBE schema=1 owned-sram\n");
    fill(mem,73U);
    if (!start(begin)) halt("active-start");
    unsigned active=0;
    for (unsigned i=0;i<100000U;++i) if (dst[1023U] == dma_fixture_pattern(1023U,73U) && (rd(DMA_CHEN)&1U)) { active=1;break; }
    /* No console output between activity observation and native stop call. */
    maia_vdma_stop_report_t report,duplicate;
    maia_vdma_result_t result=maia_esp32p4_vdma_stop_all(4096U,&report);
    field("DMA_STOP result=",(uint32_t)(-result));field(" active=",active);field(" initial_cfg=",report.initial_cfg);
    field(" initial_channels=",report.initial_channels);field(" requested=",report.requested_channels);
    field(" stage=",report.completed);field(" polls=",report.polls);field(" cfg=",report.cfg);
    field(" channels=",report.channels);field(" abort=",report.abort);field(" fifo=",report.fifo_nonempty);maia_console_puts("\n");
    if (!active || result != MAIA_VDMA_OK || report.initial_cfg != 1U || report.requested_channels != 1U ||
        report.completed != MAIA_VDMA_STOP_VERIFIED || report.adopted || !report.may_have_changed) halt("active-stop");
    fence();
    unsigned copied=0,bad=0,tail=0;
    for (unsigned i=0;i<DMA_FIXTURE_BYTES;++i) {
        unsigned v=dst[i];
        if (!v) tail=1;
        else if (v != dma_fixture_pattern(i,73U) || tail) ++bad;
        else ++copied;
    }
    unsigned intact=guards(mem,73U);
    /* Repeated full NC scans after completion: no further destination writes. */
    unsigned stable=1;
    for (unsigned pass=0;pass<8U;++pass) for (unsigned i=0;i<DMA_FIXTURE_BYTES;++i)
        if (dst[i] != (i<copied ? dma_fixture_pattern(i,73U) : 0U)) stable=0;
    int busy=maia_esp32p4_vdma_stop_all(1U,&duplicate) == MAIA_VDMA_BUSY;
    int preserved=mipi_snapshot(&after,rd) && before.cpu == after.cpu && before.sys == after.sys &&
        before.phy == after.phy && before.valid == after.valid && before.csi_bridge == after.csi_bridge;
    for (unsigned i=0;i<7U;++i) if (before.dsi[i] != after.dsi[i]) preserved=0;
    for (unsigned i=0;i<4U;++i) if (before.csi[i] != after.csi[i]) preserved=0;
    for (unsigned c=1;c<4U;++c) for (unsigned i=0;i<7U;++i) if (before.channel[c][i] != after.channel[c][i]) preserved=0;
    unsigned ch_status=rd(DMA_CH0+0x88U),common=rd(DMA_COMMON_STATUS);
    field("DMA_VERIFY copied=",copied);field(" bad=",bad);field(" guards=",intact);field(" stable=",stable);
    field(" preserved=",(uint32_t)preserved);field(" duplicate_busy=",(uint32_t)busy);
    field(" status=",ch_status);field(" common=",common);maia_console_puts("\n");
    if (!copied || copied>=DMA_FIXTURE_BYTES || bad || !intact || !stable || !preserved || !busy ||
        (ch_status&DMA_FIXTURE_ERRORS) || common) halt("verification");
    maia_console_puts("MAIA_NATIVE_VDMA_TRANSFER_PASS\n");for (;;) __asm__ volatile("wfi");
}
