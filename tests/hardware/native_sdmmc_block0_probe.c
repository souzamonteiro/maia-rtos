/* Tab5 boot-only SDHC initialization and one-block read-only diagnostic. */
#include <maia/boards/tab5_sdmmc_boot.h>
#include <maia/console.h>
#include <maia/drivers/serial_rx.h>
#include <maia/soc/esp32p4_regs.h>

#ifndef MAIA_NATIVE_SDMMC_READ_LBA
#define MAIA_NATIVE_SDMMC_READ_LBA 0U
#endif
#ifndef MAIA_NATIVE_SDMMC_REPEAT_LBA
#define MAIA_NATIVE_SDMMC_REPEAT_LBA 0U
#endif

void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }

static void hex32(uint32_t value, char text[9]) {
    const char digits[] = "0123456789abcdef";
    for (unsigned i = 0U; i < 8U; ++i) {
        text[i] = digits[(value >> (28U - 4U * i)) & 15U];
    }
    text[8] = 0;
}

static void field(const char *name, uint32_t value) {
    char text[9];
    hex32(value, text);
    maia_console_puts(name);
    maia_console_puts(text);
}

static void words(const char *name, const uint32_t value[4]) {
    maia_console_puts(name);
    for (unsigned i = 0U; i < 4U; ++i) {
        char key[5] = {' ', 'w', (char)('0' + i), '=', 0};
        char text[9];
        hex32(value[i], text);
        maia_console_puts(key);
        maia_console_puts(text);
    }
}

static void block_data(const char *name, const uint8_t *data, unsigned bytes) {
    for (unsigned offset = 0U; offset < bytes; offset += 16U) {
        maia_console_puts(name);
        char address[9];
        hex32(offset, address);
        maia_console_puts(" offset=");
        maia_console_puts(address);
        maia_console_puts(" data=");
        unsigned count = bytes - offset;
        if (count > 16U) count = 16U;
        for (unsigned i = 0U; i < count; ++i) {
            char byte[3];
            hex32(data[offset + i], address);
            byte[0] = address[6];
            byte[1] = address[7];
            byte[2] = 0;
            maia_console_puts(byte);
        }
        maia_console_puts("\n");
    }
}

void maia_boot_main(void) {
    unsigned ready = 0U;
    for (unsigned i = 0U; i < 10000000U; ++i) {
        if (maia_serial_rx_read(NULL) == 'B') {
            ready = 1U;
            break;
        }
    }
    if (!ready) {
        maia_console_puts("MAIA_NATIVE_SDMMC_READ_FAIL host-timeout\n");
        for (;;) __asm__ volatile("wfi");
    }

    maia_sdmmc_boot_report_t report;
#if defined(MAIA_NATIVE_PROBE_SDMMC_REPEAT)
    maia_console_puts("MAIA_SDMMC_READ_PROBE schema=6 slot=0 read-only dma repeat\n");
    maia_sdmmc_boot_result_t result = maia_tab5_sdmmc_boot_read_two_blocks(
        &report, MAIA_NATIVE_SDMMC_READ_LBA, MAIA_NATIVE_SDMMC_REPEAT_LBA);
#else
    maia_console_puts("MAIA_SDMMC_READ_PROBE schema=5 slot=0 read-only dma\n");
    maia_sdmmc_boot_result_t result =
        maia_tab5_sdmmc_boot_read_block(&report, MAIA_NATIVE_SDMMC_READ_LBA);
#endif
    field("SDMMC_BLOCK result=", (uint32_t)result);
    field(" completed=", (uint32_t)report.completed);
    field(" command=", report.last_command);
    field(" ocr=", report.ocr);
    field(" rca=", report.rca);
    field(" blocks_hi=", (uint32_t)(report.block_count >> 32U));
    field(" blocks_lo=", (uint32_t)report.block_count);
#if defined(MAIA_NATIVE_PROBE_SDMMC_REPEAT)
    field(" reads=", report.block_read_count);
#endif
    field(" lba=", report.block_lba);
#if defined(MAIA_NATIVE_PROBE_SDMMC_REPEAT)
    field(" second_lba=", report.second_block_lba);
#endif
    field(" cmd17_raw=", report.block_raw_status);
    field(" cmd17_status=", report.block_host_status);
    field(" response=", report.response);
    field(" fifoth_before=", report.block_fifoth_before);
    field(" fifoth=", report.block_fifoth);
    field(" intmask=", report.block_intmask);
    field(" cardthrctl_before=", report.block_cardthrctl_before);
    field(" cardthrctl=", report.block_cardthrctl);
    field(" cache_line=", report.block_dma_cache_line);
    field(" cache_status=", report.block_dma_cache_status);
    field(" dma_status=", report.block_dma_status);
    field(" dma_ctrl=", report.block_dma_ctrl);
    field(" dma_bmod=", report.block_dma_bmod);
    field(" dma_inten=", report.block_dma_inten);
    field(" dma_dbaddr=", report.block_dma_dbaddr);
    field(" dma_desc0=", report.block_dma_desc0);
    field(" dma_desc1=", report.block_dma_desc1);
    field(" dma_desc2=", report.block_dma_desc2);
    field(" dma_desc3=", report.block_dma_desc3);
    maia_console_puts("\n");
    words("SDMMC_CID", report.cid);
    maia_console_puts("\n");
    words("SDMMC_CSD", report.csd);
    maia_console_puts("\n");

    {
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
        field(" ctrl=", MAIA_MMIO32(host));
        field(" bmod=", MAIA_MMIO32(host + 0x80U));
        field(" idsts=", MAIA_MMIO32(host + 0x8CU));
        field(" idinten=", MAIA_MMIO32(host + 0x90U));
        field(" tcbc=", MAIA_MMIO32(host + 0x5CU));
        field(" tbbc=", MAIA_MMIO32(host + 0x60U));
        maia_console_puts("\n");
    }

    if (result == MAIA_SDMMC_BOOT_OK) {
        block_data("SDMMC_BLOCK_DATA", report.block, sizeof(report.block));
#if defined(MAIA_NATIVE_PROBE_SDMMC_REPEAT)
        block_data("SDMMC_SECOND_BLOCK_DATA",
                   report.second_block, sizeof(report.second_block));
#endif
        maia_console_puts("MAIA_NATIVE_SDMMC_READ_PASS\n");
    } else {
        maia_console_puts("MAIA_NATIVE_SDMMC_READ_FAIL\n");
    }
    for (;;) __asm__ volatile("wfi");
}
