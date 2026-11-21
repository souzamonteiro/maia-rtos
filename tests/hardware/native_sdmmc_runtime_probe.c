/* Runtime SDMMC block-service test from a MaiaRTOS task. */
#include <maia/arch/irq.h>
#include <maia/boards/tab5_sdmmc_service.h>
#include <maia/console.h>
#include <maia/drivers/serial_rx.h>
#include <maia/kernel.h>
#include <stdint.h>

#ifndef MAIA_NATIVE_SDMMC_RUNTIME_LBA
#define MAIA_NATIVE_SDMMC_RUNTIME_LBA 0U
#endif

#define PROBE_BLOCKS 2U
#define PROBE_BYTES (PROBE_BLOCKS * MAIA_SD_BLOCK_BYTES)
#define HOST_INPUT_TIMEOUT_CYCLES UINT64_C(4000000000)

static uint32_t s_dma_storage[MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES / sizeof(uint32_t)]
    __attribute__((aligned(MAIA_TAB5_SDMMC_DMA_ALIGNMENT)));
static uint8_t s_data[PROBE_BYTES] __attribute__((aligned(16)));
static uint8_t s_payload[PROBE_BYTES] __attribute__((aligned(16)));
static uint8_t s_task_stack[4096] __attribute__((aligned(16)));
static maia_sd_block_device_t *s_device;
static maia_sdmmc_boot_result_t s_init_result;

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

static void print_data(void) {
    for (unsigned block = 0U; block < PROBE_BLOCKS; ++block) {
        for (unsigned offset = 0U; offset < MAIA_SD_BLOCK_BYTES; offset += 16U) {
            maia_console_puts("SDMMC_RUNTIME_DATA block=");
            char text[9];
            hex32(block, text);
            maia_console_puts(text);
            maia_console_puts(" offset=");
            hex32(offset, text);
            maia_console_puts(text);
            maia_console_puts(" data=");
            for (unsigned i = 0U; i < 16U; ++i) {
                hex32(s_data[block * MAIA_SD_BLOCK_BYTES + offset + i], text);
                char byte[3] = {text[6], text[7], 0};
                maia_console_puts(byte);
            }
            maia_console_puts("\n");
        }
    }
}

static void runtime_task(void *arg) {
    (void)arg;
    uint8_t input;
    uint64_t deadline = cycle_count() + HOST_INPUT_TIMEOUT_CYCLES;
    while (cycle_count() < deadline) {
        if (maia_serial_rx_read(NULL) == 'B') break;
    }
    if (cycle_count() >= deadline) {
        maia_console_puts("MAIA_NATIVE_SDMMC_RUNTIME_FAIL reason=host-timeout\n");
        maia_console_flush();
        for (;;) maia_task_delay_ticks(100U);
    }

    maia_console_puts("MAIA_SDMMC_RUNTIME_READY schema=1\n");
    maia_console_flush();
    deadline = cycle_count() + HOST_INPUT_TIMEOUT_CYCLES;
    if (!receive_byte(&input, deadline) ||
        (input != 'R' && input != 'W' && input != 'C')) {
        maia_console_puts("MAIA_NATIVE_SDMMC_RUNTIME_FAIL reason=invalid-operation\n");
        maia_console_flush();
        for (;;) maia_task_delay_ticks(100U);
    }

    if (input == 'W' || input == 'C') {
        for (unsigned i = 0U; i < PROBE_BYTES; ++i) {
            if (!receive_byte(&s_payload[i], deadline)) {
                maia_console_puts("MAIA_NATIVE_SDMMC_RUNTIME_FAIL reason=payload-timeout\n");
                maia_console_flush();
                for (;;) maia_task_delay_ticks(100U);
            }
        }
    }

    uint32_t completed = 0U;
    uint32_t mismatch = PROBE_BYTES;
    int matches = 0;
    maia_sd_result_t result = s_device == NULL
        ? MAIA_SD_NOT_READY
        : input == 'W'
            ? maia_sd_block_device_write(
                  s_device, MAIA_NATIVE_SDMMC_RUNTIME_LBA, PROBE_BLOCKS,
                  s_payload, &completed)
            : maia_sd_block_device_read(
                  s_device, MAIA_NATIVE_SDMMC_RUNTIME_LBA, PROBE_BLOCKS,
                  s_data, &completed);

    if (result == MAIA_SD_OK && completed == PROBE_BLOCKS) {
        matches = 1;
        if (input == 'C') {
            for (unsigned i = 0U; i < PROBE_BYTES; ++i) {
                if (s_data[i] != s_payload[i]) {
                    mismatch = i;
                    matches = 0;
                    break;
                }
            }
        }
        if (input == 'R') print_data();
    }

    const char *mode = input == 'R' ? "read" : input == 'W' ? "write" : "compare";
    maia_console_puts("SDMMC_RUNTIME_OP mode=");
    maia_console_puts(mode);
    field(" result=", (uint32_t)(int32_t)result);
    field(" completed=", completed);
    field(" lba=", MAIA_NATIVE_SDMMC_RUNTIME_LBA);
    field(" matches=", (uint32_t)matches);
    field(" mismatch=", mismatch);
    maia_console_puts("\n");
    int passed = result == MAIA_SD_OK && completed == PROBE_BLOCKS &&
                 (input != 'C' || matches);
    maia_console_puts(passed ? "MAIA_NATIVE_SDMMC_RUNTIME_PASS\n"
                             : "MAIA_NATIVE_SDMMC_RUNTIME_FAIL reason=operation\n");
    maia_console_flush();
    for (;;) maia_task_delay_ticks(100U);
}

void maia_soc_systimer_init(uint32_t tick_hz);

void maia_boot_main(void) {
    maia_console_puts("MAIA_SDMMC_RUNTIME_PROBE schema=1 lba=");
    char lba[9];
    hex32(MAIA_NATIVE_SDMMC_RUNTIME_LBA, lba);
    maia_console_puts(lba);
    maia_console_puts(" count=00000002\n");

    if (maia_sched_init() != MAIA_OK) {
        maia_console_puts("MAIA_NATIVE_SDMMC_RUNTIME_FAIL reason=scheduler-init\n");
        for (;;) __asm__ volatile("wfi");
    }

    maia_sdmmc_boot_report_t report;
    uintptr_t irq_state = maia_arch_irq_save();
    s_init_result = maia_tab5_sdmmc_service_init(
        s_dma_storage, sizeof(s_dma_storage), &report);
    maia_arch_irq_restore(irq_state);
    s_device = maia_tab5_sdmmc_service_device();

    maia_console_puts("SDMMC_RUNTIME_INIT result=");
    field("", (uint32_t)s_init_result);
    field(" stage=", (uint32_t)report.completed);
    field(" blocks_hi=", (uint32_t)(report.block_count >> 32U));
    field(" blocks_lo=", (uint32_t)report.block_count);
    maia_console_puts("\n");

    maia_console_puts("MAIA_SDMMC_RUNTIME_BOOTED schema=1\n");
    maia_console_flush();
    if (maia_task_create("sdmmc-runtime", runtime_task, NULL, MAIA_PRIO_NORMAL,
                         s_task_stack, sizeof(s_task_stack), NULL) != MAIA_OK) {
        maia_console_puts("MAIA_NATIVE_SDMMC_RUNTIME_FAIL reason=task-create\n");
        for (;;) __asm__ volatile("wfi");
    }
    maia_soc_systimer_init(1000U);
    maia_sched_start();
    maia_console_puts("MAIA_NATIVE_SDMMC_RUNTIME_FAIL reason=scheduler-returned\n");
    for (;;) __asm__ volatile("wfi");
}
