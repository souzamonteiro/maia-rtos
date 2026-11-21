/* Isolated one-operation-per-boot SDMMC write/compare diagnostic. */
#include <maia/boards/tab5_sdmmc_boot.h>
#include <maia/console.h>
#include <maia/drivers/serial_rx.h>
#include <stdint.h>

#ifndef MAIA_NATIVE_SDMMC_WRITE_LBA
#define MAIA_NATIVE_SDMMC_WRITE_LBA 0U
#endif

#define HOST_INPUT_TIMEOUT_CYCLES UINT64_C(4000000000)

void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }

static uint64_t cycle_count(void) {
    uint32_t high_before, low, high_after;
    do {
        __asm__ volatile("csrr %0, mcycleh" : "=r"(high_before));
        __asm__ volatile("csrr %0, mcycle" : "=r"(low));
        __asm__ volatile("csrr %0, mcycleh" : "=r"(high_after));
    } while (high_before != high_after);
    return ((uint64_t)high_before << 32U) | low;
}

static int receive_byte(uint8_t *value, uint64_t deadline) {
    while (cycle_count() < deadline) {
        int input = maia_serial_rx_read(NULL);
        if (input >= 0) {
            *value = (uint8_t)input;
            return 1;
        }
    }
    return 0;
}

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

static void fail(const char *reason) {
    maia_console_puts("MAIA_NATIVE_SDMMC_WRITE_FAIL reason=");
    maia_console_puts(reason);
    maia_console_puts("\n");
    for (;;) __asm__ volatile("wfi");
}

void maia_boot_main(void) {
    maia_console_puts("MAIA_SDMMC_WRITE_BOOTED schema=1\n");

    uint64_t deadline = cycle_count() + HOST_INPUT_TIMEOUT_CYCLES;
    uint8_t input;
    while (cycle_count() < deadline) {
        int value = maia_serial_rx_read(NULL);
        if (value == 'B') break;
    }
    if (cycle_count() >= deadline) fail("host-timeout");

    maia_console_puts("MAIA_SDMMC_WRITE_READY schema=1\n");
    deadline = cycle_count() + HOST_INPUT_TIMEOUT_CYCLES;
    if (!receive_byte(&input, deadline)) fail("operation-timeout");
    if (input != 'C' && input != 'W') fail("invalid-operation");

    uint8_t data[512];
    for (unsigned i = 0U; i < sizeof(data); ++i) {
        if (!receive_byte(&data[i], deadline)) fail("payload-timeout");
    }

    maia_sdmmc_boot_report_t report;
    maia_sdmmc_boot_result_t result;
    uint32_t matched = 0U;
    uint32_t mismatch = sizeof(data);
    if (input == 'C') {
        result = maia_tab5_sdmmc_boot_read_block(
            &report, MAIA_NATIVE_SDMMC_WRITE_LBA);
        if (result == MAIA_SDMMC_BOOT_OK) {
            for (unsigned i = 0U; i < sizeof(data); ++i) {
                if (report.block[i] != data[i]) {
                    mismatch = i;
                    break;
                }
            }
            matched = mismatch == sizeof(data);
        }
        maia_console_puts("SDMMC_WRITE_OP mode=compare");
        field(" result=", (uint32_t)result);
        field(" completed=", (uint32_t)report.completed);
        field(" lba=", report.block_lba);
        field(" reads=", report.block_read_count);
        field(" writes=", report.block_write_count);
        field(" matches=", matched);
        field(" mismatch=", mismatch);
        maia_console_puts("\n");
        maia_console_puts(result == MAIA_SDMMC_BOOT_OK && matched
                              ? "MAIA_NATIVE_SDMMC_WRITE_PASS\n"
                              : "MAIA_NATIVE_SDMMC_WRITE_FAIL reason=compare\n");
    } else {
        result = maia_tab5_sdmmc_boot_write_block(
            &report, MAIA_NATIVE_SDMMC_WRITE_LBA, data);
        maia_console_puts("SDMMC_WRITE_OP mode=write");
        field(" result=", (uint32_t)result);
        field(" completed=", (uint32_t)report.completed);
        field(" lba=", report.block_lba);
        field(" writes=", report.block_write_count);
        maia_console_puts("\n");
        maia_console_puts(result == MAIA_SDMMC_BOOT_OK &&
                                  report.block_write_count == 1U
                              ? "MAIA_NATIVE_SDMMC_WRITE_PASS\n"
                              : "MAIA_NATIVE_SDMMC_WRITE_FAIL reason=write\n");
    }
    for (;;) __asm__ volatile("wfi");
}
