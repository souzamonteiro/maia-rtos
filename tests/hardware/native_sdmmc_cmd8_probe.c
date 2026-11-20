/* Tab5 boot-only CMD0/CMD8 diagnostic; no sector or data transfer. */
#include <maia/boards/tab5_sdmmc_boot.h>
#include <maia/console.h>
#include <maia/drivers/serial_rx.h>
#include <maia/soc/esp32p4_regs.h>

void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }

static void field(const char *name, uint32_t value) {
    const char hex[] = "0123456789abcdef";
    char text[9];
    for (unsigned i = 0; i < 8U; ++i) text[i] = hex[(value >> (28U - 4U * i)) & 15U];
    text[8] = 0;
    maia_console_puts(name);
    maia_console_puts(text);
}

void maia_boot_main(void) {
    unsigned ready = 0U;
    for (unsigned i = 0; i < 10000000U; ++i) {
        if (maia_serial_rx_read(NULL) == 'S') {
            ready = 1U;
            break;
        }
    }
    if (!ready) {
        maia_console_puts("MAIA_NATIVE_SDMMC_CMD8_FAIL host-timeout\n");
        for (;;) __asm__ volatile("wfi");
    }
    maia_sdmmc_boot_report_t report;
    maia_console_puts("MAIA_SDMMC_CMD8_PROBE schema=1 slot=0 no-data\n");
    maia_sdmmc_boot_result_t result = maia_tab5_sdmmc_boot_cmd8(&report);
    field("SDMMC_CMD8 result=", (uint32_t)result);
    field(" completed=", (uint32_t)report.completed);
    field(" command=", report.last_command);
    field(" cmd0=", report.cmd0_raw_status);
    field(" raw=", report.raw_status);
    field(" response=", report.response);
    field(" status=", report.host_status);
    maia_console_puts("\n");
    if (result == MAIA_SDMMC_BOOT_OK) {
        const uintptr_t host = UINT32_C(0x50083000);
        field("SDMMC_CONFIG sys=", MAIA_MMIO32(MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x18U));
        field(" source=", MAIA_MMIO32(MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x34U));
        field(" divider=", MAIA_MMIO32(MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x38U));
        field(" ldo=", MAIA_MMIO32(MAIA_ESP32P4_PMU_BASE + 0x1D8U));
        field(" clkdiv=", MAIA_MMIO32(host + 0x08U));
        field(" clksrc=", MAIA_MMIO32(host + 0x0CU));
        field(" clkena=", MAIA_MMIO32(host + 0x10U));
        field(" tmout=", MAIA_MMIO32(host + 0x14U));
        field(" ctype=", MAIA_MMIO32(host + 0x18U));
        field(" cdetect=", MAIA_MMIO32(host + 0x50U));
        field(" d0=", MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(39U)));
        field(" d3=", MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(42U)));
        field(" clk=", MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(43U)));
        field(" cmd=", MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(44U)));
        field(" out=", MAIA_MMIO32(MAIA_ESP32P4_GPIO_OUT1_REG));
        field(" enable=", MAIA_MMIO32(MAIA_ESP32P4_GPIO_ENABLE1_REG));
        field(" input=", MAIA_MMIO32(MAIA_ESP32P4_GPIO_IN1_REG));
        maia_console_puts("\nMAIA_NATIVE_SDMMC_CMD8_PASS\n");
    } else {
        maia_console_puts("MAIA_NATIVE_SDMMC_CMD8_FAIL\n");
    }
    for (;;) __asm__ volatile("wfi");
}
