/* Boot-only GPIO transport. Register facts: docs/audits/native-touch-identify.md. */
#include <maia/soc/esp32p4_boot_i2c.h>
#include <maia/soc/esp32p4_regs.h>

static void output(uint32_t pin, unsigned enable) {
    uintptr_t reg = pin < 32U ?
        (enable ? MAIA_ESP32P4_GPIO_ENABLE_W1TS_REG : MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG) :
        (enable ? MAIA_ESP32P4_GPIO_ENABLE1_W1TS_REG : MAIA_ESP32P4_GPIO_ENABLE1_W1TC_REG);
    MAIA_MMIO32(reg) = 1UL << (pin % 32U);
}
static void drive(void *context, unsigned mask, unsigned release) {
    maia_esp32p4_boot_i2c_t *bus = context;
    if (mask & MAIA_I2C_SDA) output(bus->sda_pin, !release);
    if (mask & MAIA_I2C_SCL) output(bus->scl_pin, !release);
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}
static unsigned sample(void *context) {
    const maia_esp32p4_boot_i2c_t *bus = context;
    unsigned result = 0U;
    uint32_t low = MAIA_MMIO32(MAIA_ESP32P4_GPIO_IN_REG);
    uint32_t high = MAIA_MMIO32(MAIA_ESP32P4_GPIO_IN1_REG);
    if (((bus->sda_pin < 32U ? low : high) >> (bus->sda_pin % 32U)) & 1U) result |= MAIA_I2C_SDA;
    if (((bus->scl_pin < 32U ? low : high) >> (bus->scl_pin % 32U)) & 1U) result |= MAIA_I2C_SCL;
    return result;
}
static maia_i2c_result_t capture(uint32_t *ticks) {
    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG) = MAIA_ESP32P4_SYSTIMER_UNIT0_UPDATE;
    for (unsigned i = 0U; i < 256U; ++i) {
        if (MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_UNIT0_OP_REG) & MAIA_ESP32P4_SYSTIMER_UNIT0_VALID) {
            *ticks = MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_UNIT0_VAL_LO_REG);
            return MAIA_I2C_OK;
        }
    }
    return MAIA_I2C_TIMEOUT;
}
static maia_i2c_result_t pause5us(void *context) {
    (void)context;
    uint32_t first;
    maia_i2c_result_t r = capture(&first);
    if (r != MAIA_I2C_OK) return r;
    for (unsigned i = 0U; i < 1024U; ++i) {
        uint32_t now;
        r = capture(&now);
        if (r != MAIA_I2C_OK) return r;
        if ((uint32_t)(now - first) >= 5U * MAIA_ESP32P4_SYSTIMER_TICKS_PER_US) return MAIA_I2C_OK;
    }
    return MAIA_I2C_TIMEOUT; /* Also bound a frozen but valid timer. */
}
static void pin_prepare(uint32_t pin) {
    output(pin, 0U);
    MAIA_MMIO32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(pin)) =
        MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX | MAIA_ESP32P4_GPIO_FUNC_OEN_SEL;
    MAIA_MMIO32(MAIA_ESP32P4_GPIO_PIN_REG(pin)) = MAIA_ESP32P4_GPIO_PIN_PAD_DRIVER_OD;
    MAIA_MMIO32(pin < 32U ? MAIA_ESP32P4_GPIO_OUT_W1TC_REG : MAIA_ESP32P4_GPIO_OUT1_W1TC_REG) =
        1UL << (pin % 32U); /* Latch always zero; HIGH is output disabled. */
    MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(pin)) =
        MAIA_ESP32P4_IO_MUX_MCU_SEL_GPIO | MAIA_ESP32P4_IO_MUX_FUN_IE | MAIA_ESP32P4_IO_MUX_FUN_WPU;
}
maia_i2c_result_t maia_esp32p4_boot_i2c_prepare(maia_esp32p4_boot_i2c_t *bus,
                                             uint32_t sda_pin, uint32_t scl_pin) {
    uint32_t status, hart;
    __asm__ volatile("csrr %0, mstatus\n\tcsrr %1, mhartid" : "=r"(status), "=r"(hart));
    if (!bus || sda_pin < 16U || sda_pin > 56U || scl_pin < 16U || scl_pin > 56U || sda_pin == scl_pin)
        return MAIA_I2C_INVALID;
    if ((status & 8U) || hart != 0U) return MAIA_I2C_BUSY;
    MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL2_REG) |= MAIA_ESP32P4_HP_CLKRST_SYSTIMER_APB_EN;
    MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG) =
        (MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL21_REG) & ~MAIA_ESP32P4_HP_CLKRST_SYSTIMER_SRC_RC) |
        MAIA_ESP32P4_HP_CLKRST_SYSTIMER_CLK_EN;
    MAIA_MMIO32(MAIA_ESP32P4_SYSTIMER_CONF_REG) |=
        MAIA_ESP32P4_SYSTIMER_CLK_EN | MAIA_ESP32P4_SYSTIMER_UNIT0_WORK_EN;
    maia_i2c_result_t r = pause5us(0);
    if (r != MAIA_I2C_OK) return r;
    bus->sda_pin = sda_pin;
    bus->scl_pin = scl_pin;
    bus->lines.context = bus;
    bus->lines.drive = drive;
    bus->lines.sample = sample;
    bus->lines.pause = pause5us;
    pin_prepare(sda_pin);
    pin_prepare(scl_pin);
    drive(bus, MAIA_I2C_BOTH, 1U);
    return MAIA_I2C_OK;
}
