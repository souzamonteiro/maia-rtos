#include <maia/console.h>
#include <maia/soc/esp32p4_core1.h>
#include <maia/soc/esp32p4_regs.h>

void maia_display_putc(char c) { (void)c; }

static void field(const char *name, uint32_t value) {
    static const char digits[] = "0123456789abcdef";
    maia_console_puts(name);
    for (unsigned i = 0U; i < 8U; ++i) {
        maia_console_putc(digits[(value >> (28U - 4U * i)) & 15U]);
    }
}

void maia_boot_main(void) {
    maia_core1_report_t report;
    maia_console_puts("\nMAIA_CORE1_PROBE schema=1 SRAM-only no-scheduler\n");
    maia_core1_result_t result = maia_esp32p4_core1_probe(&report);
    field("CORE1 result=", (uint32_t)result);
    field(" completed=", report.completed);
    field(" hart=", report.hart);
    field(" sp=", report.stack_pointer);
    field(" rounds=", report.rounds);
    field(" cause=", report.cause);
    field(" pc=", report.pc);
    maia_console_puts("\n");
    field("CORE1_INITIAL clock=", report.clock_before);
    field(" reset=", report.reset_before);
    field(" stall=", report.stall_before);
    field(" boot=", report.boot_before);
    maia_console_puts("\n");
    maia_console_puts(result == MAIA_CORE1_OK
        ? "MAIA_NATIVE_CORE1_PASS\n" : "MAIA_NATIVE_CORE1_FAIL\n");
    MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) = MAIA_ESP32P4_USB_WR_DONE;
    for (;;) __asm__ volatile("wfi");
}
