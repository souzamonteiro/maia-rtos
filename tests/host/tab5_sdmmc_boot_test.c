#include <maia/boards/tab5_sdmmc_boot.h>
#include <maia/boards/tab5_sdmmc_service.h>
#include <maia/arch/irq.h>
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
#define RESP1 (HOST + 0x34U)
#define RESP2 (HOST + 0x38U)
#define RESP3 (HOST + 0x3CU)
#define RINTSTS (HOST + 0x44U)
#define STATUS (HOST + 0x48U)
#define FIFOTH (HOST + 0x4CU)
#define BLKSIZ (HOST + 0x1CU)
#define BYTCNT (HOST + 0x20U)
#define CARDTHRCTL (HOST + 0x100U)
#define CDETECT (HOST + 0x50U)
#define TCBCNT (HOST + 0x5CU)
#define TBBCNT (HOST + 0x60U)
#define UHS (HOST + 0x74U)
#define BMOD (HOST + 0x80U)
#define PLDMND (HOST + 0x84U)
#define DBADDR (HOST + 0x88U)
#define IDSTS (HOST + 0x8CU)
#define IDINTEN (HOST + 0x90U)
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
#define CMD_RESPONSE_LONG (UINT32_C(1) << 7)
#define CMD_DATA_EXPECTED (UINT32_C(1) << 9)
#define CMD_WRITE (UINT32_C(1) << 10)
#define INT_DONE (UINT32_C(1) << 2)
#define INT_DATA_OVER (UINT32_C(1) << 3)
#define INT_DCRC (UINT32_C(1) << 7)
#define DMA_IDSTS_TX (UINT32_C(1) << 0)
#define DMA_IDSTS_RX (UINT32_C(1) << 1)
#define DMA_IDSTS_FBE (UINT32_C(1) << 2)
#define DMA_IDSTS_NIS (UINT32_C(1) << 8)
#define DMA_DESC_OWN (UINT32_C(1) << 31)
#define DMA_DESC_SINGLE (UINT32_C(1) << 4 | UINT32_C(1) << 3 | UINT32_C(1) << 2)
#define INT_LOCK (UINT32_C(1) << 12)
#define OCR_READY UINT32_C(0x80000000)
#define OCR_HCS UINT32_C(0x40000000)
#define OCR_VOLTAGE UINT32_C(0x00FF8000)
#define R1_APP_CMD UINT32_C(0x20)
#define R1_READY_TRAN UINT32_C(0x900)
#define R1_ERROR UINT32_C(0x00080000)
#define STATUS_FIFO_EMPTY (UINT32_C(1) << 2)
#define D3 (UINT32_C(1) << (42U - 32U))

void maia_tab5_sdmmc_test_reset(void);
int maia_tab5_sdmmc_test_context(void);
uint32_t maia_tab5_sdmmc_test_read(uintptr_t address);
void maia_tab5_sdmmc_test_write(uintptr_t address, uint32_t value);
uint32_t *maia_tab5_sdmmc_test_dma_storage(void);

typedef struct {
    uintptr_t address;
    uint32_t value;
} reg_t;

static reg_t regs[64];
static unsigned nregs, reads, writes, host_reads, command_writes, clear_writes;
static unsigned updates, cmd0_count, cmd8_count, context_ok, timer_updates;
static unsigned invalid_timer_at, freeze_timer, freeze_after_cmd0, reset_stuck, d3_low;
static unsigned stall_command, lock_command, silent_command, clear_dropped;
static unsigned full_mode, acmd41_count, acmd41_ready_after, cmd2_count, cmd3_count;
static unsigned cmd7_count, cmd9_count, cmd13_count, cmd17_count, cmd24_count;
static unsigned bad_app_cmd, bad_rca, bad_ocr_voltage, bad_ocr_ccs;
static unsigned bad_csd, bad_select, bad_status, bad_status_after_write;
static unsigned cmd17_error, cmd24_response_error;
static unsigned stall_data_over, stall_second_data_over;
static unsigned dma_error, dma_owner_stuck;
static unsigned cmd17_rint_reads, cmd24_rint_reads;
static uintptr_t dropped_write;
static uint32_t response_value, response_index, cmd0_event, cmd8_event;
static uint32_t ticks, clock_started, cmd0_time, cmd8_time;
static maia_sdmmc_boot_report_t report;
static uint32_t expected_lba;
static uint32_t expected_second_lba;
static uint32_t card_bytes, dma_bytes;
static uint32_t test_cycles;
static uint8_t sector0[512], sector1[512];
static uint8_t expected_write_block[512], written_sector[512];
static unsigned runtime_reenter, runtime_reentry_attempted;
static maia_sd_block_device_t *runtime_reentry_device;
static maia_sd_result_t runtime_reentry_result;
static uint32_t runtime_reentry_completed;
static uint8_t runtime_reentry_buffer[MAIA_SD_BLOCK_BYTES];

static reg_t *reg(uintptr_t address) {
    for (unsigned i = 0; i < nregs; ++i) {
        if (regs[i].address == address) return &regs[i];
    }
    fprintf(stderr, "unexpected MMIO address 0x%lx\n", (unsigned long)address);
    exit(1);
}

static void set_short_response(uint32_t value, unsigned index) {
    reg(RESP0)->value = value;
    reg(STATUS)->value = (reg(STATUS)->value & ~(0x3FU << 11U)) | (index << 11U);
}

static void add(uintptr_t address, uint32_t value) {
    CHECK(nregs < sizeof regs / sizeof regs[0]);
    regs[nregs++] = (reg_t){address, value};
}

static void reset_mock(void) {
    maia_tab5_sdmmc_test_reset();
    nregs = reads = writes = host_reads = command_writes = clear_writes = 0U;
    updates = cmd0_count = cmd8_count = timer_updates = 0U;
    full_mode = acmd41_count = cmd2_count = cmd3_count = 0U;
    cmd7_count = cmd9_count = cmd13_count = cmd17_count = cmd24_count = 0U;
    acmd41_ready_after = 1U;
    context_ok = 1U;
    invalid_timer_at = freeze_timer = freeze_after_cmd0 = reset_stuck = d3_low = 0U;
    stall_command = lock_command = silent_command = clear_dropped = 0U;
    bad_app_cmd = bad_rca = bad_ocr_voltage = bad_ocr_ccs = 0U;
    bad_csd = bad_select = bad_status = bad_status_after_write = 0U;
    cmd17_error = cmd24_response_error = 0U;
    stall_data_over = stall_second_data_over = 0U;
    dma_error = dma_owner_stuck = 0U;
    cmd17_rint_reads = cmd24_rint_reads = 0U;
    runtime_reenter = runtime_reentry_attempted = 0U;
    runtime_reentry_device = NULL;
    runtime_reentry_result = MAIA_SD_INVALID;
    runtime_reentry_completed = UINT32_MAX;
    expected_lba = 0U;
    expected_second_lba = 0U;
    card_bytes = dma_bytes = 512U;
    test_cycles = 0U;
    dropped_write = 0U;
    response_value = 0x1AAU;
    response_index = 8U;
    cmd0_event = cmd8_event = INT_DONE;
    ticks = UINT32_C(0xFFFFF000);
    clock_started = cmd0_time = cmd8_time = 0U;
    memset(&report, 0xA5, sizeof report);
    memset(sector0, 0, sizeof sector0);
    memset(sector1, 0x5A, sizeof sector1);
    memset(expected_write_block, 0, sizeof expected_write_block);
    memset(written_sector, 0, sizeof written_sector);
    sector0[0] = 0xEBU;
    sector0[1] = 0x58U;
    sector0[2] = 0x90U;
    sector0[510] = 0x55U;
    sector0[511] = 0xAAU;

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
    add(RESP1, 0U);
    add(RESP2, 0U);
    add(RESP3, 0U);
    add(RINTSTS, 0U);
    add(STATUS, 0x106U);
    add(FIFOTH, 0x01FF0000U);
    add(CDETECT, 0U);
    add(TCBCNT, 0U);
    add(TBBCNT, 0U);
    add(UHS, 0U);
    add(BMOD, 0U);
    add(PLDMND, 0U);
    add(DBADDR, 0U);
    add(IDSTS, 0U);
    add(IDINTEN, 0U);
    add(BLKSIZ, 0U);
    add(BYTCNT, 0U);
    add(CARDTHRCTL, 0x00010003U);
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
    if (address == RINTSTS && runtime_reenter &&
        !runtime_reentry_attempted) {
        CHECK(runtime_reentry_device != NULL);
        runtime_reentry_attempted = 1U;
        unsigned reads_before = reads;
        unsigned writes_before = writes;
        runtime_reentry_result = maia_sd_block_device_read(
            runtime_reentry_device, expected_lba + 1U, 1U,
            runtime_reentry_buffer, &runtime_reentry_completed);
        CHECK(reads == reads_before && writes == writes_before);
    }
    if (address == RINTSTS && cmd24_count && cmd24_rint_reads < 3U) {
        if (++cmd24_rint_reads == 3U && !stall_data_over && !cmd24_response_error) {
            uint32_t *storage = maia_tab5_sdmmc_test_dma_storage();
            uint32_t *descriptor = storage;
            CHECK(reg(DBADDR)->value == (uint32_t)(uintptr_t)storage);
            CHECK(reg(IDINTEN)->value == (DMA_IDSTS_NIS | DMA_IDSTS_TX));
            CHECK((reg(CTRL)->value & ((UINT32_C(1) << 25) | (UINT32_C(1) << 5))) ==
                  ((UINT32_C(1) << 25) | (UINT32_C(1) << 5)));
            CHECK((reg(BMOD)->value & 0x82U) == 0x82U);
            CHECK(descriptor[0] == (DMA_DESC_OWN | DMA_DESC_SINGLE));
            CHECK(descriptor[1] == sizeof expected_write_block);
            CHECK(descriptor[2] == (uint32_t)(uintptr_t)(storage + 32U));
            CHECK(descriptor[3] == 0U);
            CHECK(reg(CMDARG)->value == expected_lba + cmd24_count - 1U);
            CHECK(memcmp(storage + 32U, expected_write_block,
                         sizeof expected_write_block) == 0);
            memcpy(written_sector, storage + 32U, sizeof written_sector);
            reg(TCBCNT)->value = card_bytes;
            reg(TBBCNT)->value = dma_bytes;
            if (!dma_owner_stuck) descriptor[0] &= ~DMA_DESC_OWN;
            reg(IDSTS)->value |= dma_error ? DMA_IDSTS_FBE :
                                 (DMA_IDSTS_TX | DMA_IDSTS_NIS);
            reg(STATUS)->value =
                (reg(STATUS)->value & ~((UINT32_C(0x1FFF) << 17U) |
                                        STATUS_FIFO_EMPTY)) |
                STATUS_FIFO_EMPTY;
            reg(RINTSTS)->value |= INT_DATA_OVER;
        }
    }
    if (address == RINTSTS && cmd17_count && cmd17_rint_reads < 3U) {
        if (++cmd17_rint_reads == 3U && !stall_data_over &&
            !(stall_second_data_over && cmd17_count == 2U)) {
            uint32_t *storage = maia_tab5_sdmmc_test_dma_storage();
            uint32_t *descriptor = storage;
            CHECK(reg(DBADDR)->value == (uint32_t)(uintptr_t)storage);
            CHECK(reg(IDINTEN)->value == (DMA_IDSTS_NIS | DMA_IDSTS_RX));
            CHECK((reg(CTRL)->value & ((UINT32_C(1) << 25) | (UINT32_C(1) << 5))) ==
                  ((UINT32_C(1) << 25) | (UINT32_C(1) << 5)));
            CHECK((reg(BMOD)->value & 0x82U) == 0x82U);
            CHECK((reg(FIFOTH)->value & 0x07FF0000U) == 0U);
            CHECK(descriptor[0] == (DMA_DESC_OWN | DMA_DESC_SINGLE));
            CHECK(descriptor[1] == sizeof sector0);
            CHECK(descriptor[2] == (uint32_t)(uintptr_t)(storage + 32U));
            CHECK(descriptor[3] == 0U);
            memcpy(storage + 32U,
                   reg(CMDARG)->value == 0U ? sector0 : sector1, sizeof sector0);
            reg(TCBCNT)->value = card_bytes;
            reg(TBBCNT)->value = dma_bytes;
            if (!dma_owner_stuck) descriptor[0] &= ~DMA_DESC_OWN;
            reg(IDSTS)->value |= dma_error ? DMA_IDSTS_FBE :
                                 (DMA_IDSTS_RX | DMA_IDSTS_NIS);
            reg(STATUS)->value =
                (reg(STATUS)->value & ~((UINT32_C(0x1FFF) << 17U) |
                                        STATUS_FIFO_EMPTY)) |
                STATUS_FIFO_EMPTY;
            reg(RINTSTS)->value |= INT_DATA_OVER;
        }
    }
    if (address == MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG) {
        return (invalid_timer_at && timer_updates >= invalid_timer_at) ?
            0U : MAIA_ESP32P4_SYSTIMER_UNIT0_VALID;
    }
    if (address == MAIA_ESP32P4_SYSTIMER_UNIT0_VAL_LO_REG) return ticks;
    return reg(address)->value;
}

uintptr_t maia_arch_irq_save(void) { return 0U; }
void maia_arch_irq_restore(uintptr_t previous) { (void)previous; }
uint32_t maia_tab5_sdmmc_test_cycle_count(void) {
    test_cycles += 400U;
    return test_cycles;
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
        if (value == 0x3FFFFU) {
            ++clear_writes;
            if (clear_writes != clear_dropped) reg(address)->value &= ~value;
        }
        return;
    }
    if (address == CTRL) {
        if ((value & 7U) == 7U) {
            reg(address)->value = reset_stuck ? value : value & ~7U;
        } else if (value & (UINT32_C(1) << 2U)) {
            reg(address)->value = value & ~(UINT32_C(1) << 2U);
        } else {
            reg(address)->value = value;
        }
        return;
    }
    if (address == BMOD && value == 1U) {
        reg(address)->value = 0U;
        return;
    }
    if (address == IDSTS) {
        reg(address)->value &= ~value;
        return;
    }
    if (address == CMD) {
        ++command_writes;
        CHECK(value & CMD_START);
        CHECK(value & CMD_HOLD);
        if (!full_mode) CHECK(command_writes <= 5U);
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
        uint32_t opcode = value & 0x3FU;
        if (opcode == 0U) {
            CHECK(value == (CMD_START | CMD_HOLD | CMD_INIT));
            CHECK(reg(CMDARG)->value == 0U && updates == 3U);
            CHECK((uint32_t)(ticks - clock_started) >= 2000U * 16U);
            ++cmd0_count;
            cmd0_time = ticks;
            if (command_writes != silent_command) reg(RINTSTS)->value |= cmd0_event;
            if (freeze_after_cmd0) freeze_timer = 1U;
        } else if (opcode == 8U) {
            CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC | CMD_RESPONSE | 8U));
            CHECK(reg(CMDARG)->value == 0x1AAU && cmd0_count == 1U);
            CHECK((uint32_t)(ticks - cmd0_time) >= 20000U * 16U);
            ++cmd8_count;
            cmd8_time = ticks;
            reg(RESP0)->value = response_value;
            reg(STATUS)->value = (reg(STATUS)->value & ~(0x3FU << 11)) | (response_index << 11);
            if (command_writes != silent_command) reg(RINTSTS)->value |= cmd8_event;
        } else {
            CHECK(full_mode);
            uint32_t response = 0U;
            unsigned index = (unsigned)opcode;
            uint32_t event = INT_DONE;
            switch (opcode) {
            case 55U:
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC | CMD_RESPONSE | 55U));
                CHECK(reg(CMDARG)->value == 0U);
                response = bad_app_cmd ? 0U : R1_APP_CMD;
                break;
            case 41U:
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_RESPONSE | 41U));
                CHECK(reg(CMDARG)->value == (OCR_VOLTAGE | OCR_HCS));
                ++acmd41_count;
                response = (acmd41_count >= acmd41_ready_after ? OCR_READY : 0U) |
                           (bad_ocr_ccs ? 0U : OCR_HCS) |
                           (bad_ocr_voltage ? 0U : OCR_VOLTAGE);
                index = 0U;
                break;
            case 2U:
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC |
                               CMD_RESPONSE | CMD_RESPONSE_LONG | 2U));
                ++cmd2_count;
                reg(RESP0)->value = 0x11223344U;
                reg(RESP1)->value = 0x55667788U;
                reg(RESP2)->value = 0x99AABBCCU;
                reg(RESP3)->value = 0xDDEEFF00U;
                break;
            case 3U:
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC | CMD_RESPONSE | 3U));
                ++cmd3_count;
                response = bad_rca ? 0U : 0x12340000U;
                break;
            case 9U: {
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC |
                               CMD_RESPONSE | CMD_RESPONSE_LONG | 9U));
                CHECK(reg(CMDARG)->value == 0x12340000U);
                ++cmd9_count;
                uint32_t size = 121811U;
                reg(RESP0)->value = 0U;
                reg(RESP1)->value = ((size & 0xFFFFU) << 16U);
                reg(RESP2)->value = (UINT32_C(1) << 22U) | (9U << 16U) |
                                    ((size >> 16U) & 0x3FU);
                reg(RESP3)->value = bad_csd ? 0U : UINT32_C(0x40000000);
                index = 0U;
                break;
            }
            case 7U:
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC | CMD_RESPONSE | 7U));
                ++cmd7_count;
                response = bad_select ? R1_ERROR : R1_READY_TRAN;
                break;
            case 13U:
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC | CMD_RESPONSE | 13U));
                CHECK(reg(CMDARG)->value == 0x12340000U);
                ++cmd13_count;
                response = (bad_status ||
                            (bad_status_after_write && cmd24_count)) ? 0U : R1_READY_TRAN;
                break;
            case 17U:
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC |
                               CMD_RESPONSE | CMD_DATA_EXPECTED | 17U));
                CHECK(reg(CMDARG)->value ==
                      (cmd17_count == 0U ? expected_lba : expected_second_lba));
                CHECK(reg(CARDTHRCTL)->value == 0U);
                CHECK(reg(BLKSIZ)->value == sizeof sector0 && reg(BYTCNT)->value == sizeof sector0);
                ++cmd17_count;
                cmd17_rint_reads = 0U;
                reg(STATUS)->value = (reg(STATUS)->value & ~(0x3FU << 11U)) |
                                     (17U << 11U) | STATUS_FIFO_EMPTY;
                response = R1_READY_TRAN;
                if (cmd17_error) event |= INT_DCRC;
                break;
            case 24U:
                CHECK(value == (CMD_START | CMD_HOLD | CMD_WAIT | CMD_CRC |
                               CMD_RESPONSE | CMD_DATA_EXPECTED | CMD_WRITE | 24U));
                CHECK(reg(CMDARG)->value == expected_lba + cmd24_count);
                CHECK(reg(CARDTHRCTL)->value == 0U);
                CHECK(reg(BLKSIZ)->value == sizeof sector0 && reg(BYTCNT)->value == sizeof sector0);
                ++cmd24_count;
                cmd24_rint_reads = 0U;
                reg(STATUS)->value = (reg(STATUS)->value & ~(0x3FU << 11U)) |
                                     (24U << 11U) | STATUS_FIFO_EMPTY;
                response = cmd24_response_error ? R1_ERROR : R1_READY_TRAN;
                break;
            default:
                CHECK(0 && "unexpected SD command");
            }
            if (opcode != 2U && opcode != 9U) set_short_response(response, index);
            if (command_writes != silent_command) reg(RINTSTS)->value |= event;
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
          address == CMDARG || address == BLKSIZ || address == BYTCNT ||
          address == BMOD || address == IDINTEN || address == DBADDR || address == PLDMND ||
          address == CARDTHRCTL ||
          address == FIFOTH ||
          address == CD_INPUT || address == INT_INPUT ||
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

static void sticky_block0_failure(maia_sdmmc_boot_result_t expected,
                                  maia_sdmmc_boot_stage_t completed) {
    full_mode = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == expected);
    CHECK(report.completed == completed);
    unsigned old_writes = writes, old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_FAILED);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE &&
          writes == old_writes && reads == old_reads);
}

int main(void) {
#ifdef MAIA_SDMMC_DIAGNOSTIC_CMD8_TIMEOUT
    reset_mock();
    full_mode = 1U;
    CHECK(maia_tab5_sdmmc_service_init(
              maia_tab5_sdmmc_test_dma_storage(),
              MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &report) ==
          MAIA_SDMMC_BOOT_TIMEOUT);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CMD0 &&
          report.last_command == 8U && cmd0_count == 1U && cmd8_count == 1U);
    CHECK(maia_tab5_sdmmc_service_device() == NULL);
    unsigned writes_after_fault = writes;
    CHECK(maia_tab5_sdmmc_service_init(
              maia_tab5_sdmmc_test_dma_storage(),
              MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &report) ==
          MAIA_SDMMC_BOOT_FAILED);
    CHECK(writes == writes_after_fault && maia_tab5_sdmmc_service_device() == NULL);
    puts("PASS: injected CMD8 timeout leaves SDMMC service unavailable until reset");
    return 0;
#endif
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
        {BMOD, 0x80U}, {BMOD, 0x02U}, {BMOD, 1U}, {CLKENA, 1U}, {CLKENA, 2U},
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

    reset_mock();
    full_mode = 1U;
    acmd41_ready_after = 3U;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_OK);
    CHECK(report.completed == MAIA_SDMMC_BOOT_BLOCK && report.last_command == 17U);
    CHECK(report.ocr == (OCR_READY | OCR_HCS | OCR_VOLTAGE));
    CHECK(report.rca == 0x1234U && report.block_count == UINT64_C(124735488));
    CHECK(report.cid[0] == 0x11223344U && report.cid[3] == 0xDDEEFF00U);
    CHECK(report.csd[3] == 0x40000000U);
    CHECK(report.response == R1_READY_TRAN &&
          ((report.host_status >> 11U) & 0x3FU) == 17U);
    CHECK(report.block_lba == 0U);
    CHECK(report.block_read_count == 1U);
    CHECK(report.block_raw_status == (INT_DONE | INT_DATA_OVER));
    CHECK(report.block_host_status & (17U << 11U));
    CHECK(report.block_fifoth_before == 0x01FF0000U &&
          report.block_fifoth == 0U && report.block_intmask == 0U);
    CHECK(report.block_cardthrctl_before == 0x00010003U &&
          report.block_cardthrctl == 0U);
    uint32_t *dma_storage = maia_tab5_sdmmc_test_dma_storage();
    CHECK(report.block_dma_cache_line == 128U && report.block_dma_cache_status == 0U);
    CHECK(report.block_dma_status == (DMA_IDSTS_RX | DMA_IDSTS_NIS));
    CHECK(report.block_dma_ctrl == ((UINT32_C(1) << 25U) | (UINT32_C(1) << 5U)));
    CHECK(report.block_dma_bmod == 0x82U && report.block_dma_inten == 0x102U);
    CHECK(report.block_dma_dbaddr == (uint32_t)(uintptr_t)dma_storage);
    CHECK(report.block_dma_desc0 == DMA_DESC_SINGLE && report.block_dma_desc1 == 512U);
    CHECK(report.block_dma_desc2 == (uint32_t)(uintptr_t)(dma_storage + 32U) &&
          report.block_dma_desc3 == 0U);
    CHECK(reg(CTRL)->value == 0U && reg(BMOD)->value == 0U &&
          reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);
    CHECK(cmd17_rint_reads == 3U);
    CHECK(memcmp(report.block, sector0, sizeof sector0) == 0);
    CHECK(report.block[510] == 0x55U && report.block[511] == 0xAAU);
    CHECK(acmd41_count == 3U && cmd2_count == 1U && cmd3_count == 1U &&
          cmd7_count == 1U && cmd9_count == 1U && cmd13_count == 1U && cmd17_count == 1U);
    old_writes = writes; old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_BUSY);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE && writes == old_writes && reads == old_reads);

    reset_mock();
    CHECK(maia_tab5_sdmmc_boot_read_block(&report, UINT64_C(1) << 32U) ==
          MAIA_SDMMC_BOOT_INVALID);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE && !reads && !writes);

    reset_mock();
    full_mode = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_block(&report, UINT64_C(124735488)) ==
          MAIA_SDMMC_BOOT_INVALID);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CSD && !cmd7_count && !cmd17_count);
    old_writes = writes;
    old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_read_block(&report, UINT64_C(124735488)) ==
          MAIA_SDMMC_BOOT_FAILED);
    CHECK(writes == old_writes && reads == old_reads);

    reset_mock();
    full_mode = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_two_blocks(
              &report, 1U, UINT64_C(124735488)) == MAIA_SDMMC_BOOT_INVALID);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CSD && !cmd7_count && !cmd17_count);

    reset_mock();
    full_mode = 1U;
    expected_lba = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_block(&report, 1U) == MAIA_SDMMC_BOOT_OK);
    CHECK(report.block_lba == 1U && cmd17_count == 1U);
    CHECK(memcmp(report.block, sector1, sizeof sector1) == 0);
    CHECK(report.block[0] == 0x5AU && report.block[511] == 0x5AU);

    reset_mock();
    full_mode = 1U;
    expected_lba = UINT32_C(124735487);
    CHECK(maia_tab5_sdmmc_boot_read_block(&report, expected_lba) == MAIA_SDMMC_BOOT_OK);
    CHECK(report.block_lba == expected_lba && cmd17_count == 1U);

    reset_mock();
    full_mode = 1U;
    expected_lba = UINT32_C(32768);
    expected_second_lba = UINT32_C(32774);
    CHECK(maia_tab5_sdmmc_boot_read_two_blocks(
              &report, expected_lba, expected_second_lba) == MAIA_SDMMC_BOOT_OK);
    CHECK(report.block_read_count == 2U && report.block_lba == expected_lba &&
          report.second_block_lba == expected_second_lba);
    CHECK(cmd17_count == 2U && cmd13_count == 2U);
    CHECK(memcmp(report.block, sector1, sizeof sector1) == 0);
    CHECK(memcmp(report.second_block, sector1, sizeof sector1) == 0);
    CHECK(memcmp(report.block, report.second_block, sizeof report.block) == 0);
    CHECK(reg(CTRL)->value == 0U && reg(BMOD)->value == 0U &&
          reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);

    reset_mock();
    full_mode = 1U;
    expected_lba = 42U;
    for (unsigned i = 0U; i < sizeof expected_write_block; ++i)
        expected_write_block[i] = (uint8_t)(i * 37U + 11U);
    CHECK(maia_tab5_sdmmc_boot_write_block(
              &report, expected_lba, expected_write_block) == MAIA_SDMMC_BOOT_OK);
    CHECK(report.completed == MAIA_SDMMC_BOOT_BLOCK && report.last_command == 13U);
    CHECK(report.block_lba == expected_lba && report.block_write_count == 1U &&
          report.block_read_count == 0U);
    CHECK(report.block_dma_status == (DMA_IDSTS_TX | DMA_IDSTS_NIS));
    CHECK(report.block_dma_inten == (DMA_IDSTS_TX | DMA_IDSTS_NIS));
    CHECK(report.block_dma_desc0 == DMA_DESC_SINGLE && report.block_dma_desc1 == 512U);
    CHECK(report.block_raw_status == (INT_DONE | INT_DATA_OVER));
    CHECK(cmd24_count == 1U && cmd24_rint_reads == 3U && cmd13_count == 2U);
    CHECK(memcmp(written_sector, expected_write_block, sizeof written_sector) == 0);
    CHECK(reg(CTRL)->value == 0U && reg(BMOD)->value == 0U &&
          reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);

    reset_mock();
    CHECK(maia_tab5_sdmmc_service_device() == NULL);
    CHECK(maia_tab5_sdmmc_service_init(NULL, MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES,
                                       &report) == MAIA_SDMMC_BOOT_INVALID);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE && !reads && !writes);

    reset_mock();
    full_mode = 1U;
    bad_app_cmd = 1U;
    CHECK(maia_tab5_sdmmc_service_init(
              maia_tab5_sdmmc_test_dma_storage(),
              MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &report) ==
          MAIA_SDMMC_BOOT_RESPONSE);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CMD8 &&
          maia_tab5_sdmmc_service_device() == NULL);
    old_writes = writes;
    CHECK(maia_tab5_sdmmc_service_init(
              maia_tab5_sdmmc_test_dma_storage(),
              MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &report) ==
          MAIA_SDMMC_BOOT_FAILED);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE && writes == old_writes);

    reset_mock();
    full_mode = 1U;
    expected_lba = 42U;
    expected_second_lba = 43U;
    CHECK(maia_tab5_sdmmc_service_init(
              maia_tab5_sdmmc_test_dma_storage(),
              MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &report) == MAIA_SDMMC_BOOT_OK);
    maia_sd_block_device_t *device = maia_tab5_sdmmc_service_device();
    CHECK(device != NULL && device->geometry.block_count == UINT64_C(124735488));

    uint8_t read_buffer[2U * MAIA_SD_BLOCK_BYTES];
    uint32_t completed_blocks = 0U;
    CHECK(maia_sd_block_device_read(device, expected_lba, 2U, read_buffer,
                                   &completed_blocks) == MAIA_SD_OK);
    CHECK(completed_blocks == 2U && cmd17_count == 2U && cmd13_count == 3U);
    CHECK(memcmp(read_buffer, sector1, sizeof sector1) == 0);
    CHECK(memcmp(read_buffer + MAIA_SD_BLOCK_BYTES, sector1, sizeof sector1) == 0);
    old_writes = writes;
    CHECK(maia_sd_block_device_read(device, device->geometry.block_count - 1U,
                                   2U, read_buffer, &completed_blocks) ==
          MAIA_SD_OUT_OF_RANGE);
    CHECK(completed_blocks == 0U && writes == old_writes);
    reset_mock();

    full_mode = 1U;
    expected_lba = 60U;
    for (unsigned i = 0U; i < sizeof expected_write_block; ++i)
        expected_write_block[i] = (uint8_t)(i * 13U + 7U);
    uint8_t write_buffer[2U * MAIA_SD_BLOCK_BYTES];
    memcpy(write_buffer, expected_write_block, sizeof expected_write_block);
    memcpy(write_buffer + MAIA_SD_BLOCK_BYTES, expected_write_block,
           sizeof expected_write_block);
    CHECK(maia_tab5_sdmmc_service_init(
              maia_tab5_sdmmc_test_dma_storage(),
              MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &report) == MAIA_SDMMC_BOOT_OK);
    device = maia_tab5_sdmmc_service_device();
    completed_blocks = 0U;
    CHECK(maia_sd_block_device_write(device, expected_lba, 2U, write_buffer,
                                    &completed_blocks) == MAIA_SD_OK);
    CHECK(completed_blocks == 2U && cmd24_count == 2U && cmd13_count == 5U);
    CHECK(memcmp(written_sector, expected_write_block, sizeof written_sector) == 0);
    reset_mock();

    full_mode = 1U;
    expected_lba = 100U;
    expected_second_lba = 101U;
    stall_second_data_over = 1U;
    CHECK(maia_tab5_sdmmc_service_init(
              maia_tab5_sdmmc_test_dma_storage(),
              MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &report) == MAIA_SDMMC_BOOT_OK);
    device = maia_tab5_sdmmc_service_device();
    completed_blocks = 0U;
    CHECK(maia_sd_block_device_read(device, expected_lba, 2U, read_buffer,
                                   &completed_blocks) == MAIA_SD_TIMEOUT);
    CHECK(completed_blocks == 1U && cmd17_count == 2U);
    old_writes = writes;
    CHECK(maia_sd_block_device_read(device, expected_lba, 1U, read_buffer,
                                   &completed_blocks) == MAIA_SD_NOT_READY);
    CHECK(completed_blocks == 0U && writes == old_writes);

    reset_mock();
    full_mode = 1U;
    expected_lba = 52U;
    CHECK(maia_tab5_sdmmc_service_init(
              maia_tab5_sdmmc_test_dma_storage(),
              MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &report) == MAIA_SDMMC_BOOT_OK);
    device = maia_tab5_sdmmc_service_device();
    runtime_reentry_device = device;
    runtime_reenter = 1U;
    completed_blocks = 0U;
    CHECK(maia_sd_block_device_read(device, expected_lba, 1U, read_buffer,
                                   &completed_blocks) == MAIA_SD_OK);
    CHECK(completed_blocks == 1U && cmd17_count == 1U);
    CHECK(runtime_reentry_attempted &&
          runtime_reentry_result == MAIA_SD_BUSY &&
          runtime_reentry_completed == 0U);

    reset_mock();
    CHECK(maia_tab5_sdmmc_boot_write_block(&report, 0U, NULL) ==
          MAIA_SDMMC_BOOT_INVALID);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE && !reads && !writes);

    reset_mock();
    uint8_t report_input_byte = report.block[0];
    CHECK(maia_tab5_sdmmc_boot_write_block(&report, 0U, report.block) ==
          MAIA_SDMMC_BOOT_INVALID);
    CHECK(report.block[0] == report_input_byte && !reads && !writes);

    reset_mock();
    CHECK(maia_tab5_sdmmc_boot_write_block(
              &report, UINT64_C(1) << 32U, expected_write_block) ==
          MAIA_SDMMC_BOOT_INVALID);
    CHECK(report.completed == MAIA_SDMMC_BOOT_NONE && !reads && !writes);

    reset_mock();
    full_mode = 1U;
    CHECK(maia_tab5_sdmmc_boot_write_block(
              &report, UINT64_C(124735488), expected_write_block) ==
          MAIA_SDMMC_BOOT_INVALID);
    CHECK(report.completed == MAIA_SDMMC_BOOT_CSD && !cmd7_count && !cmd24_count);

    reset_mock();
    full_mode = 1U;
    expected_lba = 42U;
    cmd24_response_error = 1U;
    CHECK(maia_tab5_sdmmc_boot_write_block(
              &report, expected_lba, expected_write_block) == MAIA_SDMMC_BOOT_RESPONSE);
    CHECK(report.completed == MAIA_SDMMC_BOOT_SELECTED && cmd24_count == 1U &&
          report.block_write_count == 0U);
    CHECK(reg(CTRL)->value == 0U && reg(BMOD)->value == 0U &&
          reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);

    reset_mock();
    full_mode = 1U;
    expected_lba = 42U;
    dma_error = 1U;
    CHECK(maia_tab5_sdmmc_boot_write_block(
              &report, expected_lba, expected_write_block) == MAIA_SDMMC_BOOT_IO);
    CHECK(report.block_dma_status & DMA_IDSTS_FBE);
    CHECK(report.block_write_count == 0U && reg(CTRL)->value == 0U &&
          reg(BMOD)->value == 0U && reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);

    const uint32_t write_invalid_counts[] = {0U, 511U, 513U};
    for (unsigned i = 0U;
         i < sizeof write_invalid_counts / sizeof write_invalid_counts[0]; ++i) {
        reset_mock();
        full_mode = 1U;
        expected_lba = 42U;
        card_bytes = write_invalid_counts[i];
        CHECK(maia_tab5_sdmmc_boot_write_block(
                  &report, expected_lba, expected_write_block) == MAIA_SDMMC_BOOT_IO);
        CHECK(report.block_write_count == 0U && reg(CTRL)->value == 0U &&
              reg(BMOD)->value == 0U && reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);

        reset_mock();
        full_mode = 1U;
        expected_lba = 42U;
        dma_bytes = write_invalid_counts[i];
        CHECK(maia_tab5_sdmmc_boot_write_block(
                  &report, expected_lba, expected_write_block) == MAIA_SDMMC_BOOT_IO);
        CHECK(report.block_write_count == 0U && reg(CTRL)->value == 0U &&
              reg(BMOD)->value == 0U && reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);
    }

    reset_mock();
    full_mode = 1U;
    expected_lba = 42U;
    bad_status_after_write = 1U;
    CHECK(maia_tab5_sdmmc_boot_write_block(
              &report, expected_lba, expected_write_block) == MAIA_SDMMC_BOOT_TIMEOUT);
    CHECK(cmd24_count == 1U && cmd13_count == 101U && report.block_write_count == 0U);
    CHECK(reg(CTRL)->value == 0U && reg(BMOD)->value == 0U &&
          reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);

    reset_mock(); bad_app_cmd = 1U;
    sticky_block0_failure(MAIA_SDMMC_BOOT_RESPONSE, MAIA_SDMMC_BOOT_CMD8);
    CHECK(acmd41_count == 0U && !cmd2_count);
    reset_mock(); bad_ocr_voltage = 1U;
    sticky_block0_failure(MAIA_SDMMC_BOOT_UNSUPPORTED, MAIA_SDMMC_BOOT_CMD8);
    CHECK(acmd41_count == 1U && !cmd2_count);
    reset_mock(); bad_ocr_ccs = 1U;
    sticky_block0_failure(MAIA_SDMMC_BOOT_UNSUPPORTED, MAIA_SDMMC_BOOT_CMD8);
    CHECK(acmd41_count == 1U && !cmd2_count);
    reset_mock(); acmd41_ready_after = UINT32_MAX;
    sticky_block0_failure(MAIA_SDMMC_BOOT_TIMEOUT, MAIA_SDMMC_BOOT_CMD8);
    CHECK(acmd41_count == 100U && !cmd2_count);
    reset_mock(); bad_rca = 1U;
    sticky_block0_failure(MAIA_SDMMC_BOOT_RESPONSE, MAIA_SDMMC_BOOT_CID);
    CHECK(cmd2_count == 1U && cmd3_count == 1U && !cmd9_count);
    reset_mock(); bad_csd = 1U;
    sticky_block0_failure(MAIA_SDMMC_BOOT_UNSUPPORTED, MAIA_SDMMC_BOOT_RCA);
    CHECK(cmd9_count == 1U && !cmd7_count);
    reset_mock(); bad_select = 1U;
    sticky_block0_failure(MAIA_SDMMC_BOOT_RESPONSE, MAIA_SDMMC_BOOT_CSD);
    CHECK(cmd7_count == 1U && !cmd13_count);
    reset_mock(); bad_status = 1U;
    sticky_block0_failure(MAIA_SDMMC_BOOT_TIMEOUT, MAIA_SDMMC_BOOT_CSD);
    CHECK(cmd13_count == 100U && !cmd17_count);
    reset_mock(); cmd17_error = 1U; full_mode = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_IO);
    CHECK(report.completed == MAIA_SDMMC_BOOT_SELECTED &&
          cmd17_count == 1U && (report.raw_status & INT_DCRC));
    old_writes = writes; old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_FAILED);
    CHECK(writes == old_writes && reads == old_reads);
    reset_mock(); full_mode = 1U; stall_data_over = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_TIMEOUT);
    CHECK(report.completed == MAIA_SDMMC_BOOT_SELECTED);
    old_writes = writes; old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_FAILED);
    CHECK(writes == old_writes && reads == old_reads);
    reset_mock(); full_mode = 1U; stall_second_data_over = 1U;
    expected_lba = UINT32_C(32768);
    expected_second_lba = UINT32_C(32774);
    CHECK(maia_tab5_sdmmc_boot_read_two_blocks(
              &report, expected_lba, expected_second_lba) == MAIA_SDMMC_BOOT_TIMEOUT);
    CHECK(report.block_read_count == 1U && cmd17_count == 2U);
    CHECK(reg(CTRL)->value == 0U && reg(BMOD)->value == 0U &&
          reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);
    old_writes = writes; old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_read_two_blocks(
              &report, expected_lba, expected_second_lba) == MAIA_SDMMC_BOOT_FAILED);
    CHECK(writes == old_writes && reads == old_reads);
    reset_mock(); full_mode = 1U; dma_owner_stuck = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_TIMEOUT);
    CHECK(report.completed == MAIA_SDMMC_BOOT_SELECTED &&
          (report.block_dma_desc0 & DMA_DESC_OWN));
    old_writes = writes; old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_FAILED);
    CHECK(writes == old_writes && reads == old_reads);
    reset_mock(); full_mode = 1U; dma_error = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_IO);
    CHECK(report.completed == MAIA_SDMMC_BOOT_SELECTED &&
          (report.block_dma_status & DMA_IDSTS_FBE));
    old_writes = writes; old_reads = reads;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_FAILED);
    CHECK(writes == old_writes && reads == old_reads);

    const uint32_t invalid_counts[] = {0U, 511U, 513U};
    for (unsigned i = 0U; i < sizeof invalid_counts / sizeof invalid_counts[0]; ++i) {
        for (unsigned counter = 0U; counter < 2U; ++counter) {
            reset_mock();
            full_mode = 1U;
            if (counter == 0U) card_bytes = invalid_counts[i];
            else dma_bytes = invalid_counts[i];
            CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_IO);
            CHECK(report.completed == MAIA_SDMMC_BOOT_SELECTED);
            CHECK(reg(CTRL)->value == 0U && reg(BMOD)->value == 0U &&
                  reg(IDSTS)->value == 0U && reg(IDINTEN)->value == 0U);
            for (unsigned j = 0U; j < sizeof report.block; ++j)
                CHECK(report.block[j] == 0U);
            old_writes = writes; old_reads = reads;
            CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_FAILED);
            CHECK(writes == old_writes && reads == old_reads);
        }
    }
    reset_mock();
    full_mode = 1U;
    CHECK(maia_tab5_sdmmc_boot_read_block0(&report) == MAIA_SDMMC_BOOT_OK);

    puts("PASS: guarded Tab5 SDMMC init, capacity-bounded IDMAC LBA reads and sticky failures");
    return 0;
}
