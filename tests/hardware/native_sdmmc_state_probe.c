/* Boot-only, read-only SDMMC/VO4 observation; does not transmit SD commands. */
#include <maia/console.h>
#include "native_sdmmc_snapshot.h"

void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }

static uint32_t read32(uintptr_t address) { return MAIA_MMIO32(address); }
static void field(const char *name, uint32_t value) {
    const char hex[] = "0123456789abcdef";
    char text[9];
    for (unsigned i = 0; i < 8U; ++i) text[i] = hex[(value >> (28U - 4U * i)) & 15U];
    text[8] = 0;
    maia_console_puts(name);
    maia_console_puts(text);
}

void maia_boot_main(void) {
    uint32_t hart, status;
    __asm__ volatile("csrr %0, mhartid" : "=r"(hart));
    __asm__ volatile("csrr %0, mstatus" : "=r"(status));
    maia_console_puts("MAIA_SDMMC_STATE_PROBE schema=1 read-only\n");
    if (hart || (status & 8U)) {
        maia_console_puts("MAIA_NATIVE_SDMMC_STATE_FAIL context\n");
        for (;;) __asm__ volatile("wfi");
    }
    unsigned errors = 0;
    for (unsigned n = 0; n < 8U; ++n) {
        sdmmc_snapshot_t s;
        int stable = sdmmc_snapshot(&s, read32);
        if (!stable) ++errors;
        field("SDMMC_SAMPLE i=", n);
        field(" sys=", s.sys); field(" clock=", s.clock); field(" div=", s.div);
        field(" reset=", s.reset); field(" ldo=", s.ldo); field(" analog=", s.analog);
        field(" sys_after=", s.sys_after); field(" clock_after=", s.clock_after);
        field(" reset_after=", s.reset_after); field(" ldo_after=", s.ldo_after);
        field(" analog_after=", s.analog_after);
        field(" valid=", s.valid); field(" stable=", (uint32_t)stable);
        maia_console_puts("\n");
        const char *pin_names[] = {" d0=", " d1=", " d2=", " d3=", " clk=", " cmd="};
        field("SDMMC_PINS i=", n);
        for (unsigned i = 0; i < 6U; ++i) field(pin_names[i], s.pins[i]);
        maia_console_puts("\n");
        const char *host_names[] = {
            " ctrl=", " clkdiv=", " clksrc=", " clkena=", " tmout=", " ctype=", " cmd=",
            " rint=", " status=", " cdetect=", " fifoth=", " bmod=", " verid=", " hcon="
        };
        field("SDMMC_HOST i=", n);
        for (unsigned i = 0; i < 14U; ++i) field(host_names[i], s.host[i]);
        maia_console_puts("\n");
    }
    maia_console_puts(errors ? "MAIA_NATIVE_SDMMC_STATE_FAIL changed\n" : "MAIA_NATIVE_SDMMC_STATE_PASS\n");
    for (;;) __asm__ volatile("wfi");
}
