/* SPDX-FileCopyrightText: 2015-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Bounded, boot-only slot-0 command setup adapted from ESP-IDF f5c3654:
 * esp_driver_sdmmc/src/sdmmc_host.c, sdmmc_transaction.c and
 * hal/esp32p4/include/hal/sdmmc_ll.h. No ESP-IDF or FreeRTOS runtime.
 * License: LICENSES/Apache-2.0.txt. */
#include <maia/boards/tab5_sdmmc_boot.h>
#include <maia/drivers/sd_read.h>
#include <maia/soc/esp32p4_regs.h>
#include <stddef.h>

#define HOST_BASE UINT32_C(0x50083000)
#define HOST_CTRL (HOST_BASE + 0x00U)
#define HOST_CLKDIV (HOST_BASE + 0x08U)
#define HOST_CLKSRC (HOST_BASE + 0x0CU)
#define HOST_CLKENA (HOST_BASE + 0x10U)
#define HOST_TMOUT (HOST_BASE + 0x14U)
#define HOST_CTYPE (HOST_BASE + 0x18U)
#define HOST_INTMASK (HOST_BASE + 0x24U)
#define HOST_CMDARG (HOST_BASE + 0x28U)
#define HOST_CMD (HOST_BASE + 0x2CU)
#define HOST_RESP0 (HOST_BASE + 0x30U)
#define HOST_RESP1 (HOST_BASE + 0x34U)
#define HOST_RESP2 (HOST_BASE + 0x38U)
#define HOST_RESP3 (HOST_BASE + 0x3CU)
#define HOST_RINTSTS (HOST_BASE + 0x44U)
#define HOST_STATUS (HOST_BASE + 0x48U)
#define HOST_FIFOTH (HOST_BASE + 0x4CU)
#define HOST_BLKSIZ (HOST_BASE + 0x1CU)
#define HOST_BYTCNT (HOST_BASE + 0x20U)
#define HOST_CARDTHRCTL (HOST_BASE + 0x100U)
#define HOST_CDETECT (HOST_BASE + 0x50U)
#define HOST_TCBCNT (HOST_BASE + 0x5CU)
#define HOST_TBBCNT (HOST_BASE + 0x60U)
#define HOST_UHS (HOST_BASE + 0x74U)
#define HOST_BMOD (HOST_BASE + 0x80U)
#define HOST_PLDMND (HOST_BASE + 0x84U)
#define HOST_DBADDR (HOST_BASE + 0x88U)
#define HOST_IDSTS (HOST_BASE + 0x8CU)
#define HOST_IDINTEN (HOST_BASE + 0x90U)
#define CLOCK_SYS (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x18U)
#define CLOCK_SOURCE (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x34U)
#define CLOCK_DIVIDER (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x38U)
#define HOST_RESET UINT32_C(0x5011104C)
#define VO4_CONTROL (MAIA_ESP32P4_PMU_BASE + 0x1D8U)
#define CARD_DETECT_INPUT (MAIA_ESP32P4_GPIO_BASE + 0x350U)
#define CARD_INT_INPUT (MAIA_ESP32P4_GPIO_BASE + 0x358U)

#define CMD_START (UINT32_C(1) << 31)
#define CMD_HOLD (UINT32_C(1) << 29)
#define CMD_UPDATE_CLOCK (UINT32_C(1) << 21)
#define CMD_INITIALIZE (UINT32_C(1) << 15)
#define CMD_WAIT_PREVIOUS (UINT32_C(1) << 13)
#define CMD_RESPONSE_CRC (UINT32_C(1) << 8)
#define CMD_RESPONSE (UINT32_C(1) << 6)
#define CMD_RESPONSE_LONG (UINT32_C(1) << 7)
#define CMD_DATA_EXPECTED (UINT32_C(1) << 9)
#define CMD_WRITE (UINT32_C(1) << 10)
#define INT_END_BIT (UINT32_C(1) << 15)
#define INT_START_BIT (UINT32_C(1) << 13)
#define INT_HARDWARE_LOCK (UINT32_C(1) << 12)
#define INT_FIFO_ERROR (UINT32_C(1) << 11)
#define INT_HOST_TIMEOUT (UINT32_C(1) << 10)
#define INT_DATA_TIMEOUT (UINT32_C(1) << 9)
#define INT_RESPONSE_TIMEOUT (UINT32_C(1) << 8)
#define INT_DATA_CRC (UINT32_C(1) << 7)
#define INT_RESPONSE_CRC (UINT32_C(1) << 6)
#define INT_COMMAND_DONE (UINT32_C(1) << 2)
#define INT_RESPONSE_ERROR (UINT32_C(1) << 1)
#define INT_DATA_OVER (UINT32_C(1) << 3)
#define INT_DATA_ERRORS (INT_END_BIT | INT_START_BIT | INT_HARDWARE_LOCK | INT_FIFO_ERROR | \
                         INT_HOST_TIMEOUT | INT_DATA_TIMEOUT | INT_DATA_CRC)
#define INT_TIMEOUTS (INT_HOST_TIMEOUT | INT_DATA_TIMEOUT | INT_RESPONSE_TIMEOUT)
#define INT_COMMAND_ERRORS (INT_END_BIT | INT_START_BIT | INT_HARDWARE_LOCK | INT_FIFO_ERROR | \
                            INT_TIMEOUTS | INT_DATA_CRC | INT_RESPONSE_CRC | INT_RESPONSE_ERROR)
#define POLL_LIMIT 2000000U
#define STATUS_RESPONSE_INDEX_SHIFT 11U
#define STATUS_RESPONSE_INDEX_MASK (0x3FU << STATUS_RESPONSE_INDEX_SHIFT)
#define STATUS_FIFO_COUNT_SHIFT 17U
#define STATUS_FIFO_COUNT_MASK (0x1FFFU << STATUS_FIFO_COUNT_SHIFT)
#define CARDTHRCTL_CONFIG_MASK UINT32_C(0xFFFF0003)
#define CTRL_DMA_RESET (UINT32_C(1) << 2)
#define CTRL_DMA_ENABLE (UINT32_C(1) << 5)
#define CTRL_USE_INTERNAL_DMA (UINT32_C(1) << 25)
#define BMOD_SOFTWARE_RESET UINT32_C(1)
#define BMOD_FIXED_BURST (UINT32_C(1) << 1)
#define BMOD_ENABLE (UINT32_C(1) << 7)
#define IDSTS_TX (UINT32_C(1) << 0)
#define IDSTS_RX (UINT32_C(1) << 1)
#define IDSTS_FATAL_BUS (UINT32_C(1) << 2)
#define IDSTS_DESCRIPTOR_UNAVAILABLE (UINT32_C(1) << 4)
#define IDSTS_CARD_ERROR (UINT32_C(1) << 5)
#define IDSTS_NORMAL_SUMMARY (UINT32_C(1) << 8)
#define IDSTS_ABNORMAL_SUMMARY (UINT32_C(1) << 9)
#define IDSTS_CLEAR_MASK (IDSTS_TX | IDSTS_RX | IDSTS_FATAL_BUS | \
                          IDSTS_DESCRIPTOR_UNAVAILABLE | IDSTS_CARD_ERROR | \
                          IDSTS_NORMAL_SUMMARY | IDSTS_ABNORMAL_SUMMARY)
#define IDSTS_ERROR_MASK (IDSTS_FATAL_BUS | IDSTS_DESCRIPTOR_UNAVAILABLE | \
                          IDSTS_CARD_ERROR | IDSTS_ABNORMAL_SUMMARY)
#define DMA_CACHE_MAP_L1D UINT32_C(0x10)
#define DMA_UNCACHED_ALIAS_OFFSET UINT32_C(0x40000000)
#define DMA_STORAGE_ALIGNMENT 128U
#define DMA_DESCRIPTOR_STORAGE_WORDS 32U
#define DMA_DATA_OFFSET (DMA_DESCRIPTOR_STORAGE_WORDS * sizeof(uint32_t))
#define DMA_DATA_BYTES 512U
#define DMA_STORAGE_BYTES (DMA_DATA_OFFSET + DMA_DATA_BYTES)
#define STATUS_DATA_BUSY (UINT32_C(1) << 9)
#define STATUS_DATA_FSM_BUSY (UINT32_C(1) << 10)
#define OCR_VOLTAGE_MASK UINT32_C(0x00FF8000)
#define OCR_HCS UINT32_C(0x40000000)
#define OCR_READY UINT32_C(0x80000000)
#define R1_ERROR_MASK UINT32_C(0xFFF9A088)
#define R1_APP_CMD UINT32_C(0x00000020)
#define R1_READY_FOR_DATA UINT32_C(0x00000100)
#define R1_CURRENT_STATE_MASK UINT32_C(0x00001E00)
#define R1_CURRENT_STATE_TRAN UINT32_C(4)
#define R6_ERROR_MASK UINT32_C(0x0000E000)
#define SDMMC_ACMD41_RETRIES 100U

enum { UNCLAIMED, OWNED, FAULTED };
static unsigned state;

#if defined(__riscv)
#define DMA_STORAGE_ATTRIBUTES \
    __attribute__((section(".maia_sdmmc_dma"), aligned(DMA_STORAGE_ALIGNMENT), used))
#else
#define DMA_STORAGE_ATTRIBUTES __attribute__((aligned(DMA_STORAGE_ALIGNMENT)))
#endif

static uint32_t sdmmc_dma_storage[DMA_STORAGE_BYTES / sizeof(uint32_t)]
    DMA_STORAGE_ATTRIBUTES;

_Static_assert(DMA_STORAGE_BYTES == 640U, "unexpected SDMMC DMA storage size");

#ifdef MAIA_SDMMC_BOOT_TEST
int maia_tab5_sdmmc_test_context(void);
uint32_t maia_tab5_sdmmc_test_read(uintptr_t address);
void maia_tab5_sdmmc_test_write(uintptr_t address, uint32_t value);
void maia_tab5_sdmmc_test_reset(void);
uint32_t *maia_tab5_sdmmc_test_dma_storage(void) { return sdmmc_dma_storage; }
void maia_tab5_sdmmc_test_reset(void) { state = UNCLAIMED; }
#define boot_context maia_tab5_sdmmc_test_context
#define read32 maia_tab5_sdmmc_test_read
#define write32 maia_tab5_sdmmc_test_write
#else
extern int maia_probe_cache_writeback_invalidate(uint32_t map, uint32_t address, uint32_t size);
extern uint32_t maia_probe_cache_line_size(void);

static int boot_context(void) {
    uint32_t status, hart;
    __asm__ volatile("csrr %0, mstatus\n\tcsrr %1, mhartid" : "=r"(status), "=r"(hart));
    return !(status & 8U) && hart == 0U;
}
static uint32_t read32(uintptr_t address) { return MAIA_MMIO32(address); }
static void write32(uintptr_t address, uint32_t value) {
    MAIA_MMIO32(address) = value;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}
#endif

static uintptr_t dma_storage_address(void) {
    return (uintptr_t)sdmmc_dma_storage;
}

static int modify(uintptr_t address, uint32_t mask, uint32_t value);

static volatile uint32_t *dma_cpu_storage(void) {
#ifdef MAIA_SDMMC_BOOT_TEST
    return sdmmc_dma_storage;
#else
    return (volatile uint32_t *)(dma_storage_address() + DMA_UNCACHED_ALIAS_OFFSET);
#endif
}

static void dma_memory_fence(void) {
#ifdef MAIA_SDMMC_BOOT_TEST
    __atomic_thread_fence(__ATOMIC_SEQ_CST);
#else
    __asm__ volatile("fence iorw, iorw" ::: "memory");
#endif
}

static int dma_prepare_storage(maia_sdmmc_boot_report_t *report) {
#ifdef MAIA_SDMMC_BOOT_TEST
    report->block_dma_cache_line = DMA_STORAGE_ALIGNMENT;
    report->block_dma_cache_status = 0U;
    return 1;
#else
    uint32_t line = maia_probe_cache_line_size();
    if ((line != 16U && line != 32U && line != 64U && line != 128U) ||
        (DMA_STORAGE_ALIGNMENT % line) != 0U) return 0;
    int status = maia_probe_cache_writeback_invalidate(
        DMA_CACHE_MAP_L1D, (uint32_t)dma_storage_address(), DMA_STORAGE_BYTES);
    report->block_dma_cache_line = line;
    report->block_dma_cache_status = (uint32_t)status;
    if (status != 0) return 0;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
    return 1;
#endif
}

static int dma_wait_software_reset(void) {
    for (unsigned poll = 0U; poll < POLL_LIMIT; ++poll) {
        if (!(read32(HOST_BMOD) & BMOD_SOFTWARE_RESET)) return 1;
    }
    return 0;
}

static int dma_stop(void) {
    if (!modify(HOST_BMOD, BMOD_ENABLE | BMOD_FIXED_BURST, 0U) ||
        !modify(HOST_CTRL, CTRL_USE_INTERNAL_DMA, 0U)) return 0;
    write32(HOST_CTRL, read32(HOST_CTRL) | CTRL_DMA_RESET);
    unsigned poll;
    for (poll = 0U; poll < POLL_LIMIT; ++poll) {
        if (!(read32(HOST_CTRL) & CTRL_DMA_RESET)) break;
    }
    if (poll == POLL_LIMIT) return 0;
    if (!modify(HOST_CTRL, CTRL_DMA_ENABLE, 0U) ||
        !modify(HOST_IDINTEN, IDSTS_CLEAR_MASK, 0U)) return 0;
    write32(HOST_IDSTS, IDSTS_CLEAR_MASK);
    return !(read32(HOST_IDSTS) & IDSTS_CLEAR_MASK);
}

static maia_sdmmc_boot_result_t dma_abort(maia_sdmmc_boot_result_t result) {
    return dma_stop() ? result : MAIA_SDMMC_BOOT_IO;
}

static maia_sdmmc_boot_result_t dma_prepare_block(
    maia_sdmmc_boot_report_t *report, volatile uint32_t **descriptor,
    const uint8_t *write_source) {
    volatile uint32_t *storage = dma_cpu_storage();
    for (unsigned i = 0U; i < DMA_DESCRIPTOR_STORAGE_WORDS; ++i) storage[i] = 0U;
    if (write_source != NULL) {
        volatile uint8_t *data =
            (volatile uint8_t *)(storage + DMA_DESCRIPTOR_STORAGE_WORDS);
        for (size_t i = 0U; i < DMA_DATA_BYTES; ++i) data[i] = write_source[i];
    }
    *descriptor = storage;
    (*descriptor)[0] = (UINT32_C(1) << 31) | (UINT32_C(1) << 4) |
                       (UINT32_C(1) << 3) | (UINT32_C(1) << 2);
    (*descriptor)[1] = DMA_DATA_BYTES;
    (*descriptor)[2] = (uint32_t)(dma_storage_address() + DMA_DATA_OFFSET);
    (*descriptor)[3] = 0U;
    dma_memory_fence();

    if (!modify(HOST_CTRL, CTRL_DMA_ENABLE, CTRL_DMA_ENABLE)) return MAIA_SDMMC_BOOT_VERIFY;
    write32(HOST_BMOD, BMOD_SOFTWARE_RESET);
    if (!dma_wait_software_reset()) return MAIA_SDMMC_BOOT_TIMEOUT;
    write32(HOST_IDSTS, IDSTS_CLEAR_MASK);
    if (read32(HOST_IDSTS) & IDSTS_CLEAR_MASK) return MAIA_SDMMC_BOOT_VERIFY;
    uint32_t dma_completion = write_source != NULL ? IDSTS_TX : IDSTS_RX;
    if (!modify(HOST_IDINTEN, IDSTS_CLEAR_MASK, IDSTS_NORMAL_SUMMARY | dma_completion) ||
        !modify(HOST_FIFOTH, UINT32_C(0x07FF0000), 0U)) return MAIA_SDMMC_BOOT_VERIFY;
    report->block_fifoth = read32(HOST_FIFOTH);
    if (!modify(HOST_BLKSIZ, UINT32_MAX, DMA_DATA_BYTES) ||
        !modify(HOST_BYTCNT, UINT32_MAX, DMA_DATA_BYTES) ||
        !modify(HOST_DBADDR, UINT32_MAX, (uint32_t)dma_storage_address()) ||
        !modify(HOST_CTRL, CTRL_DMA_ENABLE | CTRL_USE_INTERNAL_DMA,
                CTRL_DMA_ENABLE | CTRL_USE_INTERNAL_DMA) ||
        !modify(HOST_BMOD, BMOD_ENABLE | BMOD_FIXED_BURST,
                BMOD_ENABLE | BMOD_FIXED_BURST)) return MAIA_SDMMC_BOOT_VERIFY;
    write32(HOST_PLDMND, 1U);
    return MAIA_SDMMC_BOOT_OK;
}

static void dma_report_state(maia_sdmmc_boot_report_t *report,
                             const volatile uint32_t *descriptor) {
    report->block_dma_status = read32(HOST_IDSTS);
    report->block_dma_ctrl = read32(HOST_CTRL);
    report->block_dma_bmod = read32(HOST_BMOD);
    report->block_dma_inten = read32(HOST_IDINTEN);
    report->block_dma_dbaddr = read32(HOST_DBADDR);
    report->block_dma_desc0 = descriptor[0];
    report->block_dma_desc1 = descriptor[1];
    report->block_dma_desc2 = descriptor[2];
    report->block_dma_desc3 = descriptor[3];
}

static int timer_ticks(uint32_t *ticks) {
    write32(MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG, MAIA_ESP32P4_SYSTIMER_UNIT0_UPDATE);
    for (unsigned i = 0; i < 256U; ++i) {
        if (read32(MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG) & MAIA_ESP32P4_SYSTIMER_UNIT0_VALID) {
            *ticks = read32(MAIA_ESP32P4_SYSTIMER_UNIT0_VAL_LO_REG);
            return 1;
        }
    }
    return 0;
}
static int timer_prepare(void) {
    write32(MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL2_REG,
            read32(MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL2_REG) | MAIA_ESP32P4_HP_CLKRST_SYSTIMER_APB_EN);
    write32(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG,
            (read32(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG) & ~MAIA_ESP32P4_HP_CLKRST_SYSTIMER_SRC_RC) |
            MAIA_ESP32P4_HP_CLKRST_SYSTIMER_CLK_EN);
    write32(MAIA_ESP32P4_SYSTIMER_CONF_REG, read32(MAIA_ESP32P4_SYSTIMER_CONF_REG) |
            MAIA_ESP32P4_SYSTIMER_CLK_EN | MAIA_ESP32P4_SYSTIMER_UNIT0_WORK_EN);
    uint32_t ticks;
    return timer_ticks(&ticks);
}
static int delay_us(uint32_t microseconds) {
    uint32_t first;
    if (!timer_ticks(&first)) return 0;
    for (unsigned i = 0; i < POLL_LIMIT; ++i) {
        uint32_t now;
        if (!timer_ticks(&now)) return 0;
        if ((uint32_t)(now - first) >= microseconds * MAIA_ESP32P4_SYSTIMER_TICKS_PER_US) return 1;
    }
    return 0;
}

static int modify(uintptr_t address, uint32_t mask, uint32_t value) {
    uint32_t desired = (read32(address) & ~mask) | (value & mask);
    write32(address, desired);
    return (read32(address) & mask) == (desired & mask);
}

static int pins_available(void) {
    const unsigned pins[] = {39U, 42U, 43U, 44U};
    uint32_t enabled = read32(MAIA_ESP32P4_GPIO_ENABLE1_REG);
    for (unsigned i = 0; i < 4U; ++i) {
        if ((enabled & (UINT32_C(1) << (pins[i] - 32U))) ||
            (read32(MAIA_ESP32P4_IO_MUX_GPIO_REG(pins[i])) & 0x7000U) != 0x1000U) return 0;
    }
    return 1;
}

static int configure_pins(void) {
    const uint32_t d3 = UINT32_C(1) << (42U - 32U);
    write32(MAIA_ESP32P4_GPIO_ENABLE1_W1TC_REG, d3);
    write32(MAIA_ESP32P4_GPIO_OUT1_W1TS_REG, d3);
    if (!modify(MAIA_ESP32P4_GPIO_PIN_REG(42U), 1U << 2, 0U) ||
        !modify(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(42U), 0xFFFU, 0x500U) ||
        !modify(MAIA_ESP32P4_IO_MUX_GPIO_REG(42U), 0x7E80U, 0x1E00U)) return 0;
    write32(MAIA_ESP32P4_GPIO_ENABLE1_W1TS_REG, d3);
    if (!(read32(MAIA_ESP32P4_GPIO_OUT1_REG) & d3) ||
        !(read32(MAIA_ESP32P4_GPIO_ENABLE1_REG) & d3)) return 0;
    const unsigned pins[] = {39U, 43U, 44U};
    for (unsigned i = 0; i < 3U; ++i) {
        if (!modify(MAIA_ESP32P4_IO_MUX_GPIO_REG(pins[i]), 0x7E80U, 0x0E00U)) return 0;
    }
    return modify(CARD_DETECT_INPUT, 0xFFU, 0xBEU) &&
           modify(CARD_INT_INPUT, 0xFFU, 0xBFU) &&
           !(read32(HOST_CDETECT) & 1U);
}

static maia_sdmmc_boot_result_t wait_command_taken(maia_sdmmc_boot_report_t *report) {
    for (unsigned i = 0; i < POLL_LIMIT; ++i) {
        uint32_t raw = read32(HOST_RINTSTS);
        if (raw & INT_HARDWARE_LOCK) {
            report->raw_status = raw;
            return MAIA_SDMMC_BOOT_IO;
        }
        if (!(read32(HOST_CMD) & CMD_START)) return MAIA_SDMMC_BOOT_OK;
    }
    report->raw_status = read32(HOST_RINTSTS);
    return MAIA_SDMMC_BOOT_TIMEOUT;
}

static maia_sdmmc_boot_result_t start_command(uint32_t command, uint32_t argument,
                                             maia_sdmmc_boot_report_t *report) {
    maia_sdmmc_boot_result_t result = wait_command_taken(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    write32(HOST_RINTSTS, 0x3FFFFU);
    uint32_t raw = read32(HOST_RINTSTS);
    if (raw & (INT_COMMAND_ERRORS | INT_COMMAND_DONE)) {
        report->raw_status = raw;
        return MAIA_SDMMC_BOOT_IO;
    }
    write32(HOST_CMDARG, argument);
    write32(HOST_CMD, command | CMD_HOLD | CMD_START);
    return wait_command_taken(report);
}

static maia_sdmmc_boot_result_t update_clock(maia_sdmmc_boot_report_t *report) {
    return start_command(CMD_UPDATE_CLOCK | CMD_WAIT_PREVIOUS, 0U, report);
}

static maia_sdmmc_boot_result_t send_command(uint32_t command, uint32_t argument,
                                            maia_sdmmc_boot_report_t *report) {
    report->last_command = command & 0x3FU;
    maia_sdmmc_boot_result_t result = start_command(command, argument, report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    for (unsigned i = 0; i < POLL_LIMIT; ++i) {
        uint32_t raw = read32(HOST_RINTSTS);
        if (raw & INT_COMMAND_ERRORS) {
            report->raw_status = raw;
            return (raw & INT_TIMEOUTS) ? MAIA_SDMMC_BOOT_TIMEOUT : MAIA_SDMMC_BOOT_IO;
        }
        if (raw & INT_COMMAND_DONE) {
            report->raw_status = raw;
            return MAIA_SDMMC_BOOT_OK;
        }
    }
    report->raw_status = read32(HOST_RINTSTS);
    return MAIA_SDMMC_BOOT_TIMEOUT;
}

static void report_clear(maia_sdmmc_boot_report_t *report) {
    unsigned char *bytes = (unsigned char *)report;
    for (size_t i = 0U; i < sizeof(*report); ++i) bytes[i] = 0U;
}

static maia_sdmmc_boot_result_t boot_initialize(maia_sdmmc_boot_report_t *report) {
    if (report == NULL) return MAIA_SDMMC_BOOT_INVALID;
    report_clear(report);
    if (!boot_context()) return MAIA_SDMMC_BOOT_CONTEXT;
    if (state == OWNED) return MAIA_SDMMC_BOOT_BUSY;
    if (state == FAULTED) return MAIA_SDMMC_BOOT_FAILED;
    if ((read32(VO4_CONTROL) & 0x4F80U) != 0x4180U) return MAIA_SDMMC_BOOT_POWER;
    if (!(read32(CLOCK_SYS) & (1U << 14)) ||
        (read32(HOST_RESET) & (1U << 28)) ||
        (read32(CLOCK_SOURCE) & (7U << 22)) != (1U << 24)) return MAIA_SDMMC_BOOT_HOST_STATE;
    if ((read32(HOST_CTRL) & ((1U << 25) | (1U << 5) | (1U << 4) | 7U)) ||
        (read32(HOST_BMOD) & 0x83U) || (read32(HOST_CLKENA) & 3U) ||
        (read32(HOST_CMD) & CMD_START) ||
        (read32(HOST_STATUS) & ((0x1FFFU << 17) | 0x6F0U)) ||
        !(read32(HOST_STATUS) & 4U) || !pins_available()) return MAIA_SDMMC_BOOT_BUSY;
    report->completed = MAIA_SDMMC_BOOT_GUARDED;
    state = FAULTED;
    if (!timer_prepare() || !delay_us(10U)) return MAIA_SDMMC_BOOT_TIMEOUT;
    report->completed = MAIA_SDMMC_BOOT_TIMER;
    if (!configure_pins()) return MAIA_SDMMC_BOOT_VERIFY;
    if (!delay_us(10U)) return MAIA_SDMMC_BOOT_TIMEOUT;
    if (!(read32(MAIA_ESP32P4_GPIO_IN1_REG) & (1U << (42U - 32U))))
        return MAIA_SDMMC_BOOT_VERIFY;
    report->completed = MAIA_SDMMC_BOOT_PINS;
    write32(HOST_CTRL, read32(HOST_CTRL) | 7U);
    unsigned polls;
    for (polls = 0; polls < POLL_LIMIT && (read32(HOST_CTRL) & 7U); ++polls) { }
    if (polls == POLL_LIMIT) return MAIA_SDMMC_BOOT_TIMEOUT;
    write32(HOST_INTMASK, 0U);
    if (read32(HOST_INTMASK) & 0x3FFFFU) return MAIA_SDMMC_BOOT_VERIFY;
    write32(HOST_RINTSTS, 0x3FFFFU);
    report->completed = MAIA_SDMMC_BOOT_RESET;

    maia_sdmmc_boot_result_t result = update_clock(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    const uint32_t divider_mask = (15U << 9) | (15U << 13) | (15U << 17) |
                                  (3U << 21) | (3U << 23) | (3U << 25) | (7U << 27);
    const uint32_t divider_value = (9U << 9) | (4U << 13) | (9U << 17) | (1U << 23) | (7U << 27);
    if (!modify(CLOCK_DIVIDER, divider_mask, divider_value)) return MAIA_SDMMC_BOOT_VERIFY;
    uint32_t divider = read32(CLOCK_DIVIDER);
    write32(CLOCK_DIVIDER, divider | (1U << 8));
    write32(CLOCK_DIVIDER, divider & ~(1U << 8));
    if (!delay_us(10U)) return MAIA_SDMMC_BOOT_TIMEOUT;
    if (!modify(HOST_CLKSRC, 3U, 0U) || !modify(HOST_CLKDIV, 0xFFU, 20U))
        return MAIA_SDMMC_BOOT_VERIFY;
    result = update_clock(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    if (!modify(HOST_CLKENA, 0x10001U, 1U)) return MAIA_SDMMC_BOOT_VERIFY;
    result = update_clock(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    if (!modify(HOST_CTYPE, 0x10001U, 0U) || !modify(HOST_UHS, 0x10001U, 0U) ||
        !modify(HOST_TMOUT, UINT32_MAX, UINT32_C(0x009C40FF))) return MAIA_SDMMC_BOOT_VERIFY;
    if (!delay_us(2000U)) return MAIA_SDMMC_BOOT_TIMEOUT;
    report->completed = MAIA_SDMMC_BOOT_CLOCK;
    result = send_command(CMD_INITIALIZE, 0U, report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    report->cmd0_raw_status = report->raw_status;
    report->completed = MAIA_SDMMC_BOOT_CMD0;
    if (!delay_us(20000U)) return MAIA_SDMMC_BOOT_TIMEOUT;
    result = send_command(8U | CMD_WAIT_PREVIOUS | CMD_RESPONSE | CMD_RESPONSE_CRC,
                          UINT32_C(0x1AA), report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    report->response = read32(HOST_RESP0);
    report->host_status = read32(HOST_STATUS);
    if ((report->response & 0xFFFU) != 0x1AAU ||
        ((report->host_status & STATUS_RESPONSE_INDEX_MASK) >> STATUS_RESPONSE_INDEX_SHIFT) != 8U)
        return MAIA_SDMMC_BOOT_RESPONSE;
    report->completed = MAIA_SDMMC_BOOT_CMD8;
    return MAIA_SDMMC_BOOT_OK;
}

maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_cmd8(maia_sdmmc_boot_report_t *report) {
    maia_sdmmc_boot_result_t result = boot_initialize(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    state = OWNED;
    return MAIA_SDMMC_BOOT_OK;
}

static maia_sdmmc_boot_result_t send_response(uint32_t opcode, uint32_t argument,
                                               uint32_t flags, unsigned response_index,
                                               int check_index, uint32_t response[4],
                                               maia_sdmmc_boot_report_t *report) {
    maia_sdmmc_boot_result_t result =
        send_command(opcode | CMD_WAIT_PREVIOUS | flags, argument, report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    response[0] = read32(HOST_RESP0);
    response[1] = (flags & CMD_RESPONSE_LONG) ? read32(HOST_RESP1) : 0U;
    response[2] = (flags & CMD_RESPONSE_LONG) ? read32(HOST_RESP2) : 0U;
    response[3] = (flags & CMD_RESPONSE_LONG) ? read32(HOST_RESP3) : 0U;
    report->response = response[0];
    report->host_status = read32(HOST_STATUS);
    if (check_index &&
        ((report->host_status & STATUS_RESPONSE_INDEX_MASK) >> STATUS_RESPONSE_INDEX_SHIFT) !=
            response_index) return MAIA_SDMMC_BOOT_RESPONSE;
    return MAIA_SDMMC_BOOT_OK;
}

static maia_sdmmc_boot_result_t send_r1(uint32_t opcode, uint32_t argument,
                                        uint32_t *r1,
                                        maia_sdmmc_boot_report_t *report) {
    uint32_t response[4];
    maia_sdmmc_boot_result_t result =
        send_response(opcode, argument, CMD_RESPONSE | CMD_RESPONSE_CRC,
                      opcode, 1, response, report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    *r1 = response[0];
    return (*r1 & R1_ERROR_MASK) ? MAIA_SDMMC_BOOT_RESPONSE : MAIA_SDMMC_BOOT_OK;
}

static maia_sdmmc_boot_result_t initialize_ocr(maia_sdmmc_boot_report_t *report) {
    const uint32_t host_ocr = OCR_VOLTAGE_MASK | OCR_HCS;
    uint32_t response[4];
    for (unsigned attempt = 0U; attempt < SDMMC_ACMD41_RETRIES; ++attempt) {
        uint32_t r1;
        maia_sdmmc_boot_result_t result = send_r1(55U, 0U, &r1, report);
        if (result != MAIA_SDMMC_BOOT_OK) return result;
        if (!(r1 & R1_APP_CMD)) return MAIA_SDMMC_BOOT_RESPONSE;
        result = send_response(41U, host_ocr, CMD_RESPONSE, 0U, 0, response, report);
        if (result != MAIA_SDMMC_BOOT_OK) return result;
        report->ocr = response[0];
        if (!(report->ocr & OCR_VOLTAGE_MASK & host_ocr)) return MAIA_SDMMC_BOOT_UNSUPPORTED;
        if (report->ocr & OCR_READY) {
            if (!(report->ocr & OCR_HCS)) return MAIA_SDMMC_BOOT_UNSUPPORTED;
            report->completed = MAIA_SDMMC_BOOT_OCR;
            return MAIA_SDMMC_BOOT_OK;
        }
        if (!delay_us(10000U)) return MAIA_SDMMC_BOOT_TIMEOUT;
    }
    return MAIA_SDMMC_BOOT_TIMEOUT;
}

static maia_sdmmc_boot_result_t read_card_identity(maia_sdmmc_boot_report_t *report) {
    uint32_t response[4];
    maia_sdmmc_boot_result_t result =
        send_response(2U, 0U, CMD_RESPONSE | CMD_RESPONSE_LONG | CMD_RESPONSE_CRC,
                      0U, 0, response, report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    for (unsigned i = 0U; i < 4U; ++i) report->cid[i] = response[i];
    report->completed = MAIA_SDMMC_BOOT_CID;

    result = send_response(3U, 0U, CMD_RESPONSE | CMD_RESPONSE_CRC, 3U, 1,
                           response, report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    if (response[0] & R6_ERROR_MASK) return MAIA_SDMMC_BOOT_RESPONSE;
    report->rca = response[0] >> 16U;
    if (report->rca == 0U) return MAIA_SDMMC_BOOT_RESPONSE;
    report->completed = MAIA_SDMMC_BOOT_RCA;

    result = send_response(9U, report->rca << 16U,
                           CMD_RESPONSE | CMD_RESPONSE_LONG | CMD_RESPONSE_CRC,
                           0U, 0, response, report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    for (unsigned i = 0U; i < 4U; ++i) report->csd[i] = response[i];
    maia_sd_geometry_t geometry;
    maia_sd_result_t geometry_result =
        maia_sd_sdhc_geometry(report->csd, report->ocr, &geometry);
    if (geometry_result != MAIA_SD_OK) return MAIA_SDMMC_BOOT_UNSUPPORTED;
    report->block_count = geometry.block_count;
    report->completed = MAIA_SDMMC_BOOT_CSD;
    return MAIA_SDMMC_BOOT_OK;
}

static maia_sdmmc_boot_result_t wait_transfer_ready(maia_sdmmc_boot_report_t *report) {
    for (unsigned attempt = 0U; attempt < 100U; ++attempt) {
        uint32_t status;
        maia_sdmmc_boot_result_t result = send_r1(13U, report->rca << 16U,
                                                  &status, report);
        if (result != MAIA_SDMMC_BOOT_OK) return result;
        if ((status & R1_READY_FOR_DATA) &&
            (status & R1_CURRENT_STATE_MASK) == (R1_CURRENT_STATE_TRAN << 9U)) return MAIA_SDMMC_BOOT_OK;
        if (!delay_us(1000U)) return MAIA_SDMMC_BOOT_TIMEOUT;
    }
    return MAIA_SDMMC_BOOT_TIMEOUT;
}

static maia_sdmmc_boot_result_t transfer_block(
    maia_sdmmc_boot_report_t *report, uint32_t lba,
    const uint8_t *write_source, uint8_t *read_destination) {
    int writing = write_source != NULL;
    if (writing == (read_destination != NULL)) return MAIA_SDMMC_BOOT_INVALID;
    uint32_t initial_status = read32(HOST_STATUS);
    if (((initial_status & STATUS_FIFO_COUNT_MASK) >> STATUS_FIFO_COUNT_SHIFT) != 0U ||
        (initial_status & (STATUS_DATA_BUSY | STATUS_DATA_FSM_BUSY))) {
        return MAIA_SDMMC_BOOT_BUSY;
    }
    report->block_fifoth_before = read32(HOST_FIFOTH);
    if (!modify(HOST_INTMASK, 0x3FFFFU, 0U)) return MAIA_SDMMC_BOOT_VERIFY;
    report->block_intmask = read32(HOST_INTMASK);
    report->block_cardthrctl_before = read32(HOST_CARDTHRCTL);
    if (!modify(HOST_CARDTHRCTL, CARDTHRCTL_CONFIG_MASK, 0U)) return MAIA_SDMMC_BOOT_VERIFY;
    report->block_cardthrctl = read32(HOST_CARDTHRCTL);
    if (!dma_prepare_storage(report)) return MAIA_SDMMC_BOOT_VERIFY;
    volatile uint32_t *descriptor;
    maia_sdmmc_boot_result_t result = dma_prepare_block(report, &descriptor, write_source);
    if (result != MAIA_SDMMC_BOOT_OK) return dma_abort(result);

    uint32_t response[4];
    uint32_t opcode = writing ? 24U : 17U;
    uint32_t flags = CMD_RESPONSE | CMD_RESPONSE_CRC | CMD_DATA_EXPECTED |
                     (writing ? CMD_WRITE : 0U);
    result = send_response(opcode, lba, flags, opcode, 1, response, report);
    if (result != MAIA_SDMMC_BOOT_OK) return dma_abort(result);
    if (response[0] & R1_ERROR_MASK) return dma_abort(MAIA_SDMMC_BOOT_RESPONSE);

    uint32_t dma_completion = writing ? IDSTS_TX : IDSTS_RX;
    uint32_t raw = 0U, dma_status = 0U;
    unsigned poll;
    for (poll = 0U; poll < POLL_LIMIT; ++poll) {
        raw = read32(HOST_RINTSTS);
        dma_status = read32(HOST_IDSTS);
        if (raw & INT_DATA_ERRORS) {
            report->block_raw_status = raw;
            dma_report_state(report, descriptor);
            result = (raw & (INT_HOST_TIMEOUT | INT_DATA_TIMEOUT)) ?
                MAIA_SDMMC_BOOT_TIMEOUT : MAIA_SDMMC_BOOT_IO;
            return dma_abort(result);
        }
        if (dma_status & IDSTS_ERROR_MASK) {
            report->block_raw_status = raw;
            dma_report_state(report, descriptor);
            return dma_abort(MAIA_SDMMC_BOOT_IO);
        }
        if ((raw & INT_DATA_OVER) && (dma_status & dma_completion) &&
            !(descriptor[0] & (UINT32_C(1) << 31))) break;
    }
    if (poll == POLL_LIMIT) {
        report->block_raw_status = read32(HOST_RINTSTS);
        report->block_host_status = read32(HOST_STATUS);
        dma_report_state(report, descriptor);
        return dma_abort(MAIA_SDMMC_BOOT_TIMEOUT);
    }

    report->block_raw_status = raw;
    dma_memory_fence();
    uint32_t status = read32(HOST_STATUS);
    report->block_host_status = status;
    dma_report_state(report, descriptor);
    if (((status & STATUS_FIFO_COUNT_MASK) >> STATUS_FIFO_COUNT_SHIFT) != 0U ||
        !(status & 4U) || (status & STATUS_DATA_FSM_BUSY) ||
        (!writing && (status & STATUS_DATA_BUSY))) {
        return dma_abort(MAIA_SDMMC_BOOT_IO);
    }
    if ((report->block_dma_desc0 & (UINT32_C(1) << 31)) ||
        read32(HOST_TCBCNT) != DMA_DATA_BYTES ||
        read32(HOST_TBBCNT) != DMA_DATA_BYTES) {
        return dma_abort(MAIA_SDMMC_BOOT_IO);
    }
    if (!dma_stop()) return MAIA_SDMMC_BOOT_IO;
    if (writing) {
        result = wait_transfer_ready(report);
        if (result != MAIA_SDMMC_BOOT_OK) return result;
        ++report->block_write_count;
    } else {
        volatile uint32_t *storage = dma_cpu_storage();
        volatile uint8_t *data =
            (volatile uint8_t *)(storage + DMA_DESCRIPTOR_STORAGE_WORDS);
        for (size_t i = 0U; i < DMA_DATA_BYTES; ++i) read_destination[i] = data[i];
        ++report->block_read_count;
    }
    report->completed = MAIA_SDMMC_BOOT_BLOCK;
    return MAIA_SDMMC_BOOT_OK;
}

static maia_sdmmc_boot_result_t initialize_card(maia_sdmmc_boot_report_t *report) {
    maia_sdmmc_boot_result_t result = boot_initialize(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;

    result = initialize_ocr(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    return read_card_identity(report);
}

static maia_sdmmc_boot_result_t select_card(maia_sdmmc_boot_report_t *report) {
    uint32_t r1;
    maia_sdmmc_boot_result_t result =
        send_r1(7U, report->rca << 16U, &r1, report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    result = wait_transfer_ready(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    report->completed = MAIA_SDMMC_BOOT_SELECTED;
    return MAIA_SDMMC_BOOT_OK;
}

static maia_sdmmc_boot_result_t read_blocks(
    maia_sdmmc_boot_report_t *report, uint64_t first_lba,
    uint64_t second_lba, int read_second) {
    if (report == NULL) return MAIA_SDMMC_BOOT_INVALID;
    report_clear(report);
    if (first_lba > UINT32_MAX ||
        (read_second && second_lba > UINT32_MAX)) return MAIA_SDMMC_BOOT_INVALID;
    maia_sdmmc_boot_result_t result = initialize_card(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    if (first_lba >= report->block_count ||
        (read_second && second_lba >= report->block_count))
        return MAIA_SDMMC_BOOT_INVALID;
    result = select_card(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;

    report->block_lba = (uint32_t)first_lba;
    result = transfer_block(report, (uint32_t)first_lba, NULL, report->block);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    if (read_second) {
        result = wait_transfer_ready(report);
        if (result != MAIA_SDMMC_BOOT_OK) return result;
        report->second_block_lba = (uint32_t)second_lba;
        result = transfer_block(report, (uint32_t)second_lba, NULL, report->second_block);
        if (result != MAIA_SDMMC_BOOT_OK) return result;
    }
    state = OWNED;
    return MAIA_SDMMC_BOOT_OK;
}

maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_read_block(
    maia_sdmmc_boot_report_t *report, uint64_t lba) {
    return read_blocks(report, lba, 0U, 0);
}

maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_read_two_blocks(
    maia_sdmmc_boot_report_t *report, uint64_t first_lba, uint64_t second_lba) {
    return read_blocks(report, first_lba, second_lba, 1);
}

maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_read_block0(
    maia_sdmmc_boot_report_t *report) {
    return maia_tab5_sdmmc_boot_read_block(report, 0U);
}

maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_write_block(
    maia_sdmmc_boot_report_t *report, uint64_t lba, const uint8_t block[512]) {
    if (report == NULL) return MAIA_SDMMC_BOOT_INVALID;
    if (block != NULL) {
        uintptr_t report_address = (uintptr_t)report;
        uintptr_t block_address = (uintptr_t)block;
        if ((block_address >= report_address &&
             block_address - report_address < sizeof(*report)) ||
            (block_address < report_address &&
             report_address - block_address < DMA_DATA_BYTES)) {
            return MAIA_SDMMC_BOOT_INVALID;
        }
    }
    report_clear(report);
    if (block == NULL || lba > UINT32_MAX) return MAIA_SDMMC_BOOT_INVALID;

    maia_sdmmc_boot_result_t result = initialize_card(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    if (lba >= report->block_count) return MAIA_SDMMC_BOOT_INVALID;
    result = select_card(report);
    if (result != MAIA_SDMMC_BOOT_OK) return result;

    report->block_lba = (uint32_t)lba;
    result = transfer_block(report, (uint32_t)lba, block, NULL);
    if (result != MAIA_SDMMC_BOOT_OK) return result;
    state = OWNED;
    return MAIA_SDMMC_BOOT_OK;
}
