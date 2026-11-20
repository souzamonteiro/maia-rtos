#include <maia/boards/tab5_sdmmc_boot.h>
#include <maia/soc/esp32p4_regs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #expr); exit(1); \
} } while (0)

#define HOST UINT32_C(0x50083000)
#define CTRL (HOST + 0x00U)
#define CLKDIV (HOST + 0x08U)
#define CLKSRC (HOST + 0x0CU)
#define CLKENA (HOST + 0x10U)
#define TMOUT (HOST + 0x14U)
#define CTYPE (HOST + 0x18U)
#define INTMASK (HOST + 0x24U)
#define CMDARG (HOST + 0x28U)
#define CMD (HOST + 0x2CU)
#define RESP0 (HOST + 0x30U)
#define RINTSTS (HOST + 0x44U)
#define STATUS (HOST + 0x48U)
#define CDETECT (HOST + 0x50U)
#define UHS (HOST + 0x74U)
#define BMOD (HOST + 0x80U)
#define SYS (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x18U)
#define SOURCE (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x34U)
#define DIVIDER (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x38U)
#define RESET UINT32_C(0x5011104C)
#define VO4 (MAIA_ESP32P4_PMU_BASE + 0x1D8U)
#define CD_INPUT (MAIA_ESP32P4_GPIO_BASE + 0x350U)
#define INT_INPUT (MAIA_ESP32P4_GPIO_BASE + 0x358U)
#define CMD_START (UINT32_C(1) << 31)
#define CMD_HOLD (UINT32_C(1) << 29)
#define CMD_UPDATE (UINT32_C(1) << 21)
#define CMD_INIT (UINT32_C(1) << 15)
#define CMD_WAIT (UINT32_C(1) << 13)
#define CMD_CRC (UINT32_C(1) << 8)
#define CMD_RESPONSE (UINT32_C(1) << 6)
#define INT_DONE (UINT32_C(1) << 2)
#define INT_LOCK (UINT32_C(1) << 12)
#define D3 (UINT32_C(1) << (42U - 32U))

void maia_tab5_sdmmc_test_reset(void);
int maia_tab5_sdmmc_test_context(void);
uint32_t maia_tab5_sdmmc_test_read(uintptr_t address);
void maia_tab5_sdmmc_test_write(uintptr_t address, uint32_t value);

typedef struct {
    uintptr_t address;
    uint32_t value;
} reg_t;

static reg_t regs[40];
static unsigned nregs, reads, writes, host_reads, command_writes, clear_writes;
static unsigned updates, cmd0_count, cmd8_count, context_ok, timer_updates;
static unsigned invalid_timer_at, freeze_timer, freeze_after_cmd0, reset_stuck, d3_low;
static unsigned stall_command, lock_command, silent_command, clear_dropped;
static uintptr_t dropped_write;
static uint32_t response_value, response_index, cmd0_event, cmd8_event;
static uint32_t ticks, clock_started, cmd0_time, cmd8_time;
static maia_sdmmc_boot_report_t report;

static reg_t *reg(uintptr_t address) {
    for (unsigned i = 0; i < nregs; ++i) {
        if (regs[i].address == address) return &regs[i];
    }
    fprintf(stderr, "unexpected MMIO address 0x%lx\n", (unsigned long)address);
    exit(1);
}

static void add(uintptr_t address, uint32_t value) {
    CHECK(nregs < sizeof regs / sizeof regs[0]);
    regs[nregs++] = (reg_t){address, value};
}

static void reset_mock(void) {
    maia_tab5_sdmmc_test_reset();
    nregs = reads = writes = host_reads = command_writes = clear_writes = 0U;
    updates = cmd0_count = cmd8_count = timer_updates = 0U;
    context_ok = 1U;
    invalid_timer_at = freeze_timer = freeze_after_cmd0 = reset_stuck = d3_low = 0U;
    stall_command = lock_command = silent_command = clear_dropped = 0U;
    dropped_write = 0U;
    response_value = 0x1AAU;
    response_index = 8U;
    cmd0_event = cmd8_event = INT_DONE;
    ticks = UINT32_C(0xFFFFF000);
    clock_started = cmd0_time = cmd8_time = 0U;
    memset(&report, 0xA5, sizeof report);

    add(SYS, 0x0C0D5078U);
    add(SOURCE, 0x01000401U);
    add(DIVIDER, 0x38862600U);
    add(RESET, 0U);
    add(VO4, 0x40204180U);
    add(CTRL, 0U);
    add(CLKDIV, 0U);
    add(CLKSRC, 0U);
    add(CLKENA, 0U);
    add(TMOUT, 0xFFFFFF40U);
    add(CTYPE, 0U);
    add(INTMASK, 0x3FFFFU);
    add(CMDARG, 0U);
    add(CMD, CMD_HOLD);
    add(RESP0, 0U);
    add(RINTSTS, 0U);
    add(STATUS, 0x106U);
    add(CDETECT, 0U);
    add(UHS, 0U);
    add(BMOD, 0U);
    add(MAIA_ESP32P4_GPIO_ENABLE1_REG, 0U);
    add(MAIA_ESP32P4_GPIO_OUT1_REG, 0U);
    add(MAIA_ESP32P4_GPIO_IN1_REG, 0U);
    add(MAIA_ESP32P4_GPIO_PIN_REG(42U), 0U);
    add(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(42U), 0U);
    add(MAIA_ESP32P4_IO_MUX_GPIO_REG(39U), 0x1800U);
    add(MAIA_ESP32P4_IO_MUX_GPIO_REG(42U), 0x1800U);
    add(MAIA_ESP32P4_IO_MUX_GPIO_REG(43U), 0x1800U);
    add(MAIA_ESP32P4_IO_MUX_GPIO_REG(44U), 0x1800U);
    add(CD_INPUT, 0x3EU);
    add(INT_INPUT, 0x3FU);
    add(MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL2_REG, 0U);
    add(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG, 0U);
    add(MAIA_ESP32P4_SYSTIMER_CONF_REG, 0U);
    add(MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG, 0U);
    add(MAIA_ESP32P4_SYSTIMER_UNIT0_VAL_LO_REG, 0U);
}

int maia_tab5_sdmmc_test_context(void) { return (int)context_ok; }

uint32_t maia_tab5_sdmmc_test_read(uintptr_t address) {
    ++reads;
    if (address >= HOST && address < HOST + 0x100U) ++host_reads;
    if (address == MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG) {
        return (invalid_timer_at && timer_updates >= invalid_timer_at) ?
            0U : MAIA_ESP32P4_SYSTIMER_UNIT0_VALID;
    }
    if (address == MAIA_ESP32P4_SYSTIMER_UNIT0_VAL_LO_REG) return ticks;
    return reg(address)->value;
}

static void check_pins_and_clock(void) {
    CHECK(reg(MAIA_ESP32P4_GPIO_OUT1_REG)->value & D3);
    CHECK(reg(MAIA_ESP32P4_GPIO_ENABLE1_REG)->value & D3);
    CHECK((reg(MAIA_ESP32P4_IO_MUX_GPIO_REG(42U))->value & 0x7E80U) == 0x1E00U);
    const unsigned pins[] = {39U, 43U, 44U};
    for (unsigned i = 0; i < 3U; ++i)
        CHECK((reg(MAIA_ESP32P4_IO_MUX_GPIO_REG(pins[i]))->value & 0x7E80U) == 0x0E00U);
    CHECK((reg(CD_INPUT)->value & 0xFFU) == 0xBEU);
    CHECK((reg(INT_INPUT)->value & 0xFFU) == 0xBFU);
    CHECK((reg(CLKDIV)->value & 0xFFU) == 20U);
    CHECK((reg(CLKSRC)->value & 3U) == 0U);
    CHECK((reg(CLKENA)->value & 0x10001U) == 1U);
    CHECK((reg(CTYPE)->value & 0x10001U) == 0U);
    CHECK((reg(UHS)->value & 0x10001U) == 0U);
    CHECK(reg(TMOUT)->value == 0x009C40FFU);
}

void maia_tab5_sdmmc_test_write(uintptr_t address, uint32_t value) {
    ++writes;
    if (address == MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG) {
        CHECK(value == MAIA_ESP32P4_SYSTIMER_UNIT0_UPDATE);
        ++timer_updates;
        if (!freeze_timer) ticks += 1600U;
        return;
    }
    if (address == MAIA_ESP32P4_GPIO_ENABLE1_W1TC_REG) {
        CHECK(value == D3);
        reg(MAIA_ESP32P4_GPIO_ENABLE1_REG)->value &= ~value;
        return;
    }
    if (address == MAIA_ESP32P4_GPIO_ENABLE1_W1TS_REG) {
        CHECK(value == D3 && (reg(MAIA_ESP32P4_GPIO_OUT1_REG)->value & D3));
        reg(MAIA_ESP32P4_GPIO_ENABLE1_REG)->value |= value;
        if (!d3_low) reg(MAIA_ESP32P4_GPIO_IN1_REG)->value |= value;
        return;
    }
    if (address == MAIA_ESP32P4_GPIO_OUT1_W1TS_REG) {
        CHECK(value == D3 && !(reg(MAIA_ESP32P4_GPIO_ENABLE1_REG)->value & D3));
        reg(MAIA_ESP32P4_GPIO_OUT1_REG)->value |= value;
        return;
    }
    if (address == RINTSTS) {
        CHECK(value == 0x3FFFFU);
        ++clear_writes;
        if (clear_writes != clear_dropped) reg(address)->value &= ~value;
        return;
    }
    if (address == CTRL) {
        CHECK((value & 7U) == 7U);
        reg(address)->value = reset_stuck ? value : value & ~7U;
        return;
    }
    if (address == CMD) {
        ++command_writes;
        CHECK(value & CMD_START);
        CHECK(value & CMD_HOLD);
        CHECK(command_writes <= 5U);
        reg(address)->value = (command_writes == stall_command) ? value : value & ~CMD_START;
        if (command_writes == lock_command) reg(RINTSTS)->value |= INT_LOCK;
        if (value & CMD_UPDATE) {
            CHECK(value == (CMD_START | CMD_HOLD | CMD_UPDATE | CMD_WAIT));
            CHECK(reg(CMDARG)->value == 0U);
            ++updates;
            if (updates == 3U) clock_started = ticks;
            return;
        }
        check_pins_and_clock();
        if ((value & 0x3FU) == 0U) {
            CHECK(value == (CMD_START | CMD_HOLD | CMD_INIT));
            CHECK(reg(CMDARG)->value == 0U && updates == 3U);
            CHECK((uint32_t)(ticks - clock_started) >= 2000U * 16U);
            ++cmd0_count;
            cmd0_time = ticks;
            if (command_writes != silent_command) reg(RINTSTS)->value |= cmd0_event;
            if (freeze_after_cmd0) freeze_timer = 1U;
        } else {
            CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC | CMD_RESPONSE | 8U));
            CHECK(reg(CMDARG)->value == 0x1AAU && cmd0_count == 1U);
            CHECK((uint32_t)(ticks - cmd0_time) >= 20000U * 16U);
            ++cmd8_count;
            cmd8_time = ticks;
            reg(RESP0)->value = response_value;
            reg(STATUS)->value = (reg(STATUS)->value & ~(0x3FU << 11)) | (response_index << 11);
            if (command_writes != silent_command) reg(RINTSTS)->value |= cmd8_event;
        }
        return;
    }
    if (address == DIVIDER) {
        const uint32_t mask = (15U << 9) | (15U << 13) | (15U << 17) |
                              (3U << 21) | (3U << 23) | (3U << 25) | (7U << 27);
        const uint32_t expected = (9U << 9) | (4U << 13) | (9U << 17) | (1U << 23) | (7U << 27);
        CHECK((value & mask) == expected);
        CHECK((value & 0xC00000FFU) == (0x38862600U & 0xC00000FFU));
    }
    if (address == CLKENA && (value & 1U)) CHECK(updates == 2U);
    CHECK(address == DIVIDER || address == CLKDIV || address == CLKSRC || address == CLKENA ||
          address == TMOUT || address == CTYPE || address == UHS || address == INTMASK ||
          address == CMDARG || address == CD_INPUT || address == INT_INPUT ||
          address == MAIA_ESP32P4_GPIO_PIN_REG(42U) ||
          address == MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(42U) ||
          address == MAIA_ESP32P4_IO_MUX_GPIO_REG(39U) ||
          address == MAIA_ESP32P4_IO_MUX_GPIO_REG(42U) ||
          address == MAIA_ESP32P4_IO_MUX_GPIO_REG(43U) ||
          address == MAIA_ESP32P4_IO_MUX_GPIO_REG(44U) ||
          address == MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL2_REG ||
          address == MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG ||
          address == MAIA_ESP32P4_SYSTIMER_CONF_REG);
    if (address != dropped_write) reg(address)->value = value;
}

static void sticky_failure(maia_sdmmc_boot_result_t expected, maia_sdmmc_boot_stage_t completed) {
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == expected);
    CHECK(report.completed == completed);
    unsigned old_writes = writes, old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_FAILED);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE && writes == old_writes && reads == old_reads);
}

int main(void) {
    reset_mock();
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_OK);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CMD8 && report.last_command == 8U);
    CHECK(report.cmd0_raw_status == INT_DONE && report.raw_status == INT_DONE);
    CHECK(report.response == 0x1AAU && ((report.host_status >> 11) & 0x3FU) == 8U);
    CHECK(updates == 3U && cmd0_count == 1U && cmd8_count == 1U && command_writes == 5U);
    CHECK((uint32_t)(cmd8_time - cmd0_time) >= 320000U);
    CHECK((reg(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG)->value &
           (MAIA_ESP32P4_HP_CLKRST_SYSTIMER_CLK_EN | MAIA_ESP32P4_HP_CLKRST_SYSTIMER_SRC_RC)) ==
          MAIA_ESP32P4_HP_CLKRST_SYSTIMER_CLK_EN);
    unsigned old_writes = writes, old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_BUSY);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE && writes == old_writes && reads == old_reads);

    reset_mock();
    CHECK(maia_tab5_sdmmc_boot_cmd8(NULL) == MAIA_SDMMC_BOOT_INVALID && !reads && !writes);
    context_ok = 0U;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_CONTEXT && !reads && !writes);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE);

    const uint32_t power_bits[] = {1U << 7, 1U << 8, 1U << 9, 1U << 10, 1U << 11, 1U << 14};
    for (unsigned i = 0; i < sizeof power_bits / sizeof power_bits[0]; ++i) {
        reset_mock();
        reg(VO4)->value ^= power_bits[i];
        CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_POWER && !writes && !host_reads);
    }
    reset_mock(); reg(SYS)->value &= ~(1U << 14);
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_HOST_STATE && !writes && !host_reads);
    reset_mock(); reg(RESET)->value |= 1U << 28;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_HOST_STATE && !writes && !host_reads);
    for (unsigned bit = 22U; bit <= 24U; ++bit) {
        reset_mock(); reg(SOURCE)->value ^= 1U << bit;
        CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_HOST_STATE && !writes && !host_reads);
    }
    const struct { uintptr_t address; uint32_t value; } busy[] = {
        {CTRL, 1U << 4}, {CTRL, 1U << 5}, {CTRL, 1U << 25}, {CTRL, 1U},
        {BMOD, 0x80U}, {BMOD, 1U}, {CLKENA, 1U}, {CLKENA, 2U},
        {CMD, CMD_START}, {STATUS, 0x200U}, {STATUS, 0x400U},
        {STATUS, 0x10U}, {STATUS, 1U << 17}, {MAIA_ESP32P4_GPIO_ENABLE1_REG, D3},
        {MAIA_ESP32P4_IO_MUX_GPIO_REG(44U), 0x2000U}
    };
    for (unsigned i = 0; i < sizeof busy / sizeof busy[0]; ++i) {
        reset_mock();
        if (busy[i].address == MAIA_ESP32P4_IO_MUX_GPIO_REG(44U))
            reg(busy[i].address)->value = busy[i].value;
        else
            reg(busy[i].address)->value |= busy[i].value;
        CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_BUSY && !writes);
    }
    reset_mock(); reg(STATUS)->value &= ~4U;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_BUSY && !writes);
    reg(STATUS)->value |= 4U;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_OK);

    reset_mock(); invalid_timer_at = 1U;
    sticky_failure(MAIA_SDMMC_BOOT_TIMEOUT, MAIA_SDMMC_BOOT_GUARDED);
    CHECK(!command_writes && !(reg(MAIA_ESP32P4_GPIO_ENABLE1_REG)->value & D3));
    reset_mock(); freeze_timer = 1U;
    sticky_failure(MAIA_SDMMC_BOOT_TIMEOUT, MAIA_SDMMC_BOOT_GUARDED);
    CHECK(!command_writes && !(reg(MAIA_ESP32P4_GPIO_ENABLE1_REG)->value & D3));
    reset_mock(); dropped_write = MAIA_ESP32P4_IO_MUX_GPIO_REG(44U);
    sticky_failure(MAIA_SDMMC_BOOT_VERIFY, MAIA_SDMMC_BOOT_TIMER);
    CHECK(!command_writes);
    reset_mock(); reg(CDETECT)->value = 1U;
    sticky_failure(MAIA_SDMMC_BOOT_VERIFY, MAIA_SDMMC_BOOT_TIMER);
    CHECK(!command_writes);
    reset_mock(); d3_low = 1U;
    sticky_failure(MAIA_SDMMC_BOOT_VERIFY, MAIA_SDMMC_BOOT_TIMER);
    CHECK(!command_writes);
    reset_mock(); reset_stuck = 1U;
    sticky_failure(MAIA_SDMMC_BOOT_TIMEOUT, MAIA_SDMMC_BOOT_PINS);
    CHECK(!command_writes);
    reset_mock(); dropped_write = INTMASK;
    sticky_failure(MAIA_SDMMC_BOOT_VERIFY, MAIA_SDMMC_BOOT_PINS);
    CHECK(!command_writes);
    reset_mock(); stall_command = 1U;
    sticky_failure(MAIA_SDMMC_BOOT_TIMEOUT, MAIA_SDMMC_BOOT_RESET);
    CHECK(!cmd0_count && !cmd8_count);
    reset_mock(); lock_command = 2U;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_IO);
    CHECK(report.completed == MAIA_SDMMC_BOOT_RESET && (report.raw_status & INT_LOCK) && !cmd0_count);
    reset_mock(); dropped_write = DIVIDER;
    sticky_failure(MAIA_SDMMC_BOOT_VERIFY, MAIA_SDMMC_BOOT_RESET);
    CHECK(updates == 1U && !cmd0_count);
    reset_mock(); silent_command = 4U;
    sticky_failure(MAIA_SDMMC_BOOT_TIMEOUT, MAIA_SDMMC_BOOT_CLOCK);
    CHECK(cmd0_count == 1U && !cmd8_count);
    reset_mock(); cmd0_event = INT_DONE | (1U << 1);
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_IO);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CLOCK && report.last_command == 0U &&
          (report.raw_status & (1U << 1)) && !cmd8_count);
    reset_mock(); freeze_after_cmd0 = 1U;
    sticky_failure(MAIA_SDMMC_BOOT_TIMEOUT, MAIA_SDMMC_BOOT_CMD0);
    CHECK(cmd0_count == 1U && !cmd8_count);
    reset_mock(); clear_dropped = 6U;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_IO);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CMD0 && report.raw_status & INT_DONE && !cmd8_count);

    const uint32_t errors[] = {
        1U << 8, 1U << 9, 1U << 10, 1U << 6, 1U << 1,
        1U << 12, 1U << 7, 1U << 11, 1U << 13, 1U << 15
    };
    for (unsigned i = 0; i < sizeof errors / sizeof errors[0]; ++i) {
        reset_mock(); cmd8_event = INT_DONE | errors[i];
        maia_sdmmc_boot_result_t expected = i < 3U ? MAIA_SDMMC_BOOT_TIMEOUT : MAIA_SDMMC_BOOT_IO;
        CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == expected);
        CHECK(report.completed == MAIA_SDMMC_BOOT_CMD0 && report.last_command == 8U);
        CHECK(report.cmd0_raw_status == INT_DONE && (report.raw_status & errors[i]));
        old_writes = writes;
        CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_FAILED && writes == old_writes);
    }
    reset_mock(); response_value = 0x1ABU;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_RESPONSE);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CMD0 && report.response == 0x1ABU);
    reset_mock(); response_value = 0x2AAU;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_RESPONSE);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CMD0);
    reset_mock(); response_index = 7U;
    CHECK(maia_tab5_sdmmc_boot_cmd8(&report) == MAIA_SDMMC_BOOT_RESPONSE);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CMD0 && ((report.host_status >> 11) & 0x3FU) == 7U);
    puts("PASS: guarded Tab5 SDMMC CMD0/CMD8, timing, response, MMIO whitelist and sticky failures");
    return 0;
}
