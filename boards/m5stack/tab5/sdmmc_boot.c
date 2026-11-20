/* SPDX-FileCopyrightText: 2015-2025 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Bounded, boot-only slot-0 command setup adapted from ESP-IDF f5c3654:
 * esp_driver_sdmmc/src/sdmmc_host.c, sdmmc_transaction.c and
 * hal/esp32p4/include/hal/sdmmc_ll.h. No ESP-IDF or FreeRTOS runtime.
 * License: LICENSES/Apache-2.0.txt. */
#include <maia/boards/tab5_sdmmc_boot.h>
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
#define HOST_RINTSTS (HOST_BASE + 0x44U)
#define HOST_STATUS (HOST_BASE + 0x48U)
#define HOST_CDETECT (HOST_BASE + 0x50U)
#define HOST_UHS (HOST_BASE + 0x74U)
#define HOST_BMOD (HOST_BASE + 0x80U)
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
#define INT_TIMEOUTS (INT_HOST_TIMEOUT | INT_DATA_TIMEOUT | INT_RESPONSE_TIMEOUT)
#define INT_COMMAND_ERRORS (INT_END_BIT | INT_START_BIT | INT_HARDWARE_LOCK | INT_FIFO_ERROR | \
                            INT_TIMEOUTS | INT_DATA_CRC | INT_RESPONSE_CRC | INT_RESPONSE_ERROR)
#define POLL_LIMIT 2000000U
#define STATUS_RESPONSE_INDEX_SHIFT 11U
#define STATUS_RESPONSE_INDEX_MASK (0x3FU << STATUS_RESPONSE_INDEX_SHIFT)

enum { UNCLAIMED, OWNED, FAULTED };
static unsigned state;

#ifdef MAIA_SDMMC_BOOT_TEST
int maia_tab5_sdmmc_test_context(void);
uint32_t maia_tab5_sdmmc_test_read(uintptr_t address);
void maia_tab5_sdmmc_test_write(uintptr_t address, uint32_t value);
void maia_tab5_sdmmc_test_reset(void);
void maia_tab5_sdmmc_test_reset(void) { state = UNCLAIMED; }
#define boot_context maia_tab5_sdmmc_test_context
#define read32 maia_tab5_sdmmc_test_read
#define write32 maia_tab5_sdmmc_test_write
#else
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

maia_sdmmc_boot_result_t maia_tab5_sdmmc_boot_cmd8(maia_sdmmc_boot_report_t *report) {
    if (report == NULL) return MAIA_SDMMC_BOOT_INVALID;
    maia_sdmmc_boot_report_t empty = {MAIA_SDMMC_BOOT_NONE, 0U, 0U, 0U, 0U, 0U};
    *report = empty;
    if (!boot_context()) return MAIA_SDMMC_BOOT_CONTEXT;
    if (state == OWNED) return MAIA_SDMMC_BOOT_BUSY;
    if (state == FAULTED) return MAIA_SDMMC_BOOT_FAILED;
    if ((read32(VO4_CONTROL) & 0x4F80U) != 0x4180U) return MAIA_SDMMC_BOOT_POWER;
    if (!(read32(CLOCK_SYS) & (1U << 14)) ||
        (read32(HOST_RESET) & (1U << 28)) ||
        (read32(CLOCK_SOURCE) & (7U << 22)) != (1U << 24)) return MAIA_SDMMC_BOOT_HOST_STATE;
    if ((read32(HOST_CTRL) & ((1U << 25) | (1U << 5) | (1U << 4) | 7U)) ||
        (read32(HOST_BMOD) & 0x81U) || (read32(HOST_CLKENA) & 3U) ||
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
    state = OWNED;
    return MAIA_SDMMC_BOOT_OK;
}
