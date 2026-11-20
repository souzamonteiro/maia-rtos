#include <maia/boards/tab5_st7121.h>
#include <maia/boards/tab5_panel_reset.h>
#include <maia/boards/tab5_touch.h>
#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/soc/esp32p4_boot_i2c.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_dsi_access.h>
#include <maia/soc/esp32p4_ldo3.h>
#include <maia/soc/esp32p4_mipi_park.h>
#include <maia/soc/esp32p4_regs.h>

#include "soc_display_native.h"

#define DISPLAY_DSI_BRG_BASE MAIA_ESP32P4_DSI_BRG_BASE
#define DISPLAY_DSI_HOST_BASE MAIA_ESP32P4_DSI_HOST_BASE
#define DISPLAY_GDMA_BASE MAIA_ESP32P4_GDMA_BASE
#define DISPLAY_HP_CLKRST_BASE MAIA_ESP32P4_HP_SYS_CLKRST_BASE

static maia_esp32p4_boot_i2c_t s_display_reset_bus;

const maia_i2c_lines_t *maia_esp32p4_display_reset_i2c_lines(void) {
    return &s_display_reset_bus.lines;
}

static void display_native_print_hex32(uint32_t value) {
    static const char digits[] = "0123456789ABCDEF";
    for (int shift = 28; shift >= 0; shift -= 4) {
        maia_console_putc(digits[(value >> (uint32_t)shift) & 0x0FU]);
    }
}

static int display_native_fail(const char *stage, int result) {
    maia_console_puts("[DISPLAY] Native DSI ");
    maia_console_puts(stage);
    maia_console_puts(" failed result=");
    display_native_print_hex32((uint32_t)result);
    maia_console_puts("\n");
    return -1;
}

static void display_native_write32(uintptr_t address, uint32_t value) {
    MAIA_MMIO32(address) = value;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}

static int display_native_dma_quiet(void) {
    if ((MAIA_MMIO32(DISPLAY_GDMA_BASE + 0x10U) & 1U) ||
        (MAIA_MMIO32(DISPLAY_GDMA_BASE + 0x18U) & 0x0FU) ||
        (MAIA_MMIO32(DISPLAY_GDMA_BASE + 0x1CU) & 0x0FU) ||
        (MAIA_MMIO32(DISPLAY_GDMA_BASE + 0x58U) & 1U)) {
        return 0;
    }
    for (uint32_t channel = 0; channel < 4U; ++channel) {
        uintptr_t status1 = DISPLAY_GDMA_BASE + 0x134U + 0x100U * channel;
        if (MAIA_MMIO32(status1) & 0x7FFFU) {
            return 0;
        }
    }
    return 1;
}

static int display_native_prepare_csi(void) {
    const uintptr_t sysreg = MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG;
    const uintptr_t phyreg = MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG;
    const uintptr_t resetreg = DISPLAY_HP_CLKRST_BASE + 0xC0U;
    uint32_t sys = MAIA_MMIO32(sysreg);
    uint32_t phy = MAIA_MMIO32(phyreg);
    uint32_t cpu = MAIA_MMIO32(DISPLAY_HP_CLKRST_BASE + 0x14U);
    if (!(cpu & (1U << 13U)) || (sys & 0x1020U) != 0x1020U ||
        !(phy & 1U) || !display_native_dma_quiet()) {
        return 0;
    }

    uint32_t isp = MAIA_MMIO32(DISPLAY_HP_CLKRST_BASE + 0xA8U);
    uint32_t reset = MAIA_MMIO32(resetreg);
    if (MAIA_MMIO32(DISPLAY_HP_CLKRST_BASE + 0xCCU) & 0x7800U) {
        return 0;
    }
    if ((isp & (1U << 25U)) && !(reset & (1U << 29U)) &&
        ((MAIA_MMIO32(0x500A1008U) & 0x18000001U) ||
         (MAIA_MMIO32(0x500A1114U) & 7U) != 4U)) {
        return 0;
    }

    const uint32_t csi_reset_mask = 0x18000000U;
    uint32_t held = reset | csi_reset_mask;
    uint32_t ready = sys | 0xC00U;
    if (MAIA_MMIO32(sysreg) != sys || MAIA_MMIO32(phyreg) != phy ||
        MAIA_MMIO32(resetreg) != reset) {
        return 0;
    }
    if (held != reset) {
        display_native_write32(resetreg, held);
    }
    if (MAIA_MMIO32(resetreg) != held) {
        return 0;
    }
    if (ready != sys) {
        display_native_write32(sysreg, ready);
    }
    if (MAIA_MMIO32(sysreg) != ready) {
        return 0;
    }
    if (!(phy & (1U << 4U))) {
        display_native_write32(phyreg, phy | (1U << 4U));
    }
    if (MAIA_MMIO32(phyreg) != (phy | (1U << 4U)) ||
        !display_native_dma_quiet()) {
        return 0;
    }

    uint32_t released = reset & ~csi_reset_mask;
    display_native_write32(resetreg, released);
    if (MAIA_MMIO32(resetreg) != released || MAIA_MMIO32(sysreg) != ready ||
        MAIA_MMIO32(phyreg) != (phy | (1U << 4U))) {
        return 0;
    }
    return !(MAIA_MMIO32(0x5009F008U) & 1U) &&
           !(MAIA_MMIO32(0x5009F040U) & 1U) &&
           !(MAIA_MMIO32(0x5009F044U) & 1U) &&
           !(MAIA_MMIO32(0x5009F804U) & 1U) &&
           display_native_dma_quiet();
}

static int display_native_prepare_mipi(void) {
    if (!display_native_prepare_csi()) {
        return display_native_fail("CSI preparation", MAIA_MIPI_PARK_BUSY);
    }

    const uintptr_t sysreg = MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG;
    uint32_t cpu = MAIA_MMIO32(DISPLAY_HP_CLKRST_BASE + 0x14U);
    uint32_t sys = MAIA_MMIO32(sysreg);
    if ((sys & 0x1420U) != 0x1420U || !(cpu & (1U << 13U))) {
        return display_native_fail("MIPI clock guard", MAIA_MIPI_PARK_BUSY);
    }
    if (!(sys & (1U << 11U))) {
        display_native_write32(sysreg, sys | (1U << 11U));
    }
    if (MAIA_MMIO32(sysreg) != (sys | (1U << 11U)) ||
        !display_native_dma_quiet()) {
        return display_native_fail("CSI host clock", MAIA_MIPI_PARK_VERIFY);
    }

    uint32_t bridge = MAIA_MMIO32(DISPLAY_DSI_BRG_BASE + 0x04U);
    uint32_t dpi = MAIA_MMIO32(DISPLAY_DSI_BRG_BASE + 0x40U);
    uint32_t fifo = MAIA_MMIO32(DISPLAY_DSI_BRG_BASE + 0x14U);
    uint32_t host_power = MAIA_MMIO32(DISPLAY_DSI_HOST_BASE + 0x04U);
    uint32_t dsi_phy_reset = MAIA_MMIO32(DISPLAY_DSI_HOST_BASE + 0xA0U);
    uint32_t csi_reset = MAIA_MMIO32(0x5009F008U);
    uint32_t csi_shutdown = MAIA_MMIO32(0x5009F040U);
    uint32_t csi_phy_reset = MAIA_MMIO32(0x5009F044U);
    uint32_t csi_bridge = MAIA_MMIO32(0x5009F804U);
    if ((bridge & 1U) || (dpi & 1U) || (fifo & 0x3FFFU) ||
        (host_power & 1U) || (dsi_phy_reset & 0x0FU) ||
        (csi_reset & 1U) || (csi_shutdown & 1U) ||
        (csi_phy_reset & 1U) || (csi_bridge & 1U) ||
        (MAIA_MMIO32(DISPLAY_GDMA_BASE + 0x58U) & 1U) ||
        !display_native_dma_quiet()) {
        return display_native_fail("MIPI park precondition", MAIA_MIPI_PARK_BUSY);
    }

    const uintptr_t registers[] = {
        DISPLAY_DSI_BRG_BASE + 0x04U,
        0x5009F804U,
        DISPLAY_DSI_HOST_BASE + 0xA0U,
        0x5009F044U
    };
    const uint32_t values[] = {
        bridge | 1U,
        csi_bridge | 1U,
        dsi_phy_reset | 2U,
        csi_phy_reset | 1U
    };
    for (uint32_t i = 0; i < 4U; ++i) {
        display_native_write32(registers[i], values[i]);
        if (MAIA_MMIO32(registers[i]) != values[i]) {
            return display_native_fail("MIPI reset arming", MAIA_MIPI_PARK_VERIFY);
        }
    }

    maia_mipi_park_report_t park;
    maia_mipi_park_result_t park_result = maia_esp32p4_mipi_park_reset(&park);
    if (park_result != MAIA_MIPI_PARK_OK ||
        park.completed != MAIA_MIPI_PARK_VERIFIED ||
        park.attempted_mask != 0x0FU) {
        return display_native_fail("MIPI reset parking", park_result);
    }

    maia_mipi_gate_report_t gate;
    maia_mipi_park_result_t gate_result = maia_esp32p4_mipi_gate_clocks(&gate);
    if (gate_result != MAIA_MIPI_PARK_OK ||
        gate.completed != MAIA_MIPI_GATE_VERIFIED) {
        return display_native_fail("MIPI clock gating", gate_result);
    }
    if (MAIA_MMIO32(DISPLAY_HP_CLKRST_BASE + 0x14U) != cpu ||
        !display_native_dma_quiet()) {
        return display_native_fail("MIPI post-gate verification", MAIA_MIPI_PARK_VERIFY);
    }
    return 0;
}

static int display_native_write_dcs(void *context, const uint8_t *bytes, uint32_t length) {
    (void)context;
    if (bytes == NULL || length == 0U || length > 64U) {
        return -1;
    }
    if (length <= 2U) {
        maia_dsi_short_report_t report;
        maia_dsi_access_result_t result = maia_esp32p4_dsi_dcs_short(
            0U, bytes[0], length - 1U, length == 2U ? bytes[1] : 0U, 10000U, &report);
        return result == MAIA_DSI_ACCESS_OK ? 0 : (int)result;
    }

    maia_dsi_long_report_t report;
    maia_dsi_access_result_t result =
        maia_esp32p4_dsi_dcs_long(0U, bytes, length, 10000U, &report);
    return result == MAIA_DSI_ACCESS_OK ? 0 : (int)result;
}

static int display_native_wait_ms(void *context, uint32_t milliseconds) {
    (void)context;
    if (milliseconds > UINT32_MAX / 1000U) {
        return -1;
    }
    maia_udelay(milliseconds * 1000U);
    return 0;
}

static int display_native_gpio23_is_input(void) {
    return !(MAIA_MMIO32(MAIA_ESP32P4_GPIO_ENABLE_REG) & (1U << 23U)) &&
        MAIA_MMIO32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(23U)) ==
            (MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX | MAIA_ESP32P4_GPIO_FUNC_OEN_SEL) &&
        MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(23U)) ==
            (MAIA_ESP32P4_IO_MUX_MCU_SEL_GPIO | MAIA_ESP32P4_IO_MUX_FUN_IE);
}

static int display_native_same_supply(const maia_ldo3_snapshot_t *left,
                                     const maia_ldo3_snapshot_t *right) {
    return left->control == right->control && left->analog == right->analog &&
        left->mac_sys2 == right->mac_sys2 && left->mac_sys3 == right->mac_sys3 &&
        left->rs_error0 == right->rs_error0;
}

static int display_native_reset_panel(uint8_t firmware) {
    uint32_t cpu = MAIA_MMIO32(DISPLAY_HP_CLKRST_BASE + 0x14U);
    uint32_t sys = MAIA_MMIO32(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG);
    uint32_t phy = MAIA_MMIO32(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG);
    uint32_t reset = MAIA_MMIO32(DISPLAY_HP_CLKRST_BASE + 0xC0U);
    if (!(cpu & (1U << 13U)) || (sys & 0x1C00U) ||
        (phy & 0x93U) || (reset & 0x3C000000U) != 0x3C000000U ||
        !display_native_dma_quiet()) {
        return display_native_fail("panel reset quiescence", MAIA_MIPI_PARK_BUSY);
    }

    maia_ldo3_snapshot_t supply;
    maia_ldo3_plan_t plan;
    maia_ldo3_result_t supply_result = maia_esp32p4_ldo3_snapshot(&supply);
    if (supply_result != MAIA_LDO3_OK) {
        return display_native_fail("panel reset supply snapshot", supply_result);
    }
    maia_ldo3_result_t plan_result = maia_esp32p4_ldo3_plan2500(&supply, &plan);
    if (plan_result != MAIA_LDO3_OK) {
        return display_native_fail("panel reset supply plan", plan_result);
    }
    if ((supply.control & MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (supply.analog & MAIA_ESP32P4_LDO3_ANALOG_MASK) !=
            (((uint32_t)plan.dref << 28U) | ((uint32_t)plan.mul << 23U) | (1U << 26U))) {
        return display_native_fail("panel reset supply guard", MAIA_LDO3_VERIFY);
    }

    display_native_write32(MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG, 1U << 23U);
    display_native_write32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(23U),
        MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX | MAIA_ESP32P4_GPIO_FUNC_OEN_SEL);
    display_native_write32(MAIA_ESP32P4_IO_MUX_GPIO_REG(23U),
        MAIA_ESP32P4_IO_MUX_MCU_SEL_GPIO | MAIA_ESP32P4_IO_MUX_FUN_IE);
    if (!display_native_gpio23_is_input()) {
        return display_native_fail("GPIO23 reset guard", MAIA_MIPI_PARK_VERIFY);
    }

    maia_i2c_result_t i2c_result =
        maia_esp32p4_boot_i2c_prepare(&s_display_reset_bus, 31U, 32U);
    if (i2c_result != MAIA_I2C_OK) {
        return display_native_fail("panel reset I2C setup", i2c_result);
    }
    uint8_t detected_firmware = 0U;
    if (maia_tab5_touch_identify(&s_display_reset_bus.lines, &detected_firmware) != MAIA_I2C_OK ||
        detected_firmware != firmware) {
        return display_native_fail("panel reset firmware check", MAIA_I2C_BUSY);
    }

    maia_tab5_reset_report_t panel_reset;
    i2c_result = maia_tab5_panel_reset(&s_display_reset_bus.lines, &panel_reset);
    if (i2c_result != MAIA_I2C_OK) {
        return display_native_fail("physical panel reset", i2c_result);
    }
    if (panel_reset.completed != MAIA_TAB5_RESET_VERIFIED ||
        !panel_reset.may_have_changed) {
        return display_native_fail("physical panel reset report", MAIA_I2C_VERIFY);
    }

    uint8_t status = 0xFFU;
    uint8_t firmware_after = 0U;
    maia_ldo3_snapshot_t supply_after;
    if (maia_i2c_read_register16(&s_display_reset_bus.lines, 0x55U, 1U, &status) != MAIA_I2C_OK ||
        status != 0U ||
        maia_tab5_touch_identify(&s_display_reset_bus.lines, &firmware_after) != MAIA_I2C_OK ||
        firmware_after != firmware ||
        s_display_reset_bus.lines.sample(s_display_reset_bus.lines.context) != MAIA_I2C_BOTH ||
        !display_native_gpio23_is_input() ||
        !display_native_dma_quiet() ||
        MAIA_MMIO32(MAIA_ESP32P4_HP_MIPI_SYS_CLK_REG) != sys ||
        MAIA_MMIO32(MAIA_ESP32P4_HP_MIPI_PHY_CLK_REG) != phy ||
        MAIA_MMIO32(DISPLAY_HP_CLKRST_BASE + 0xC0U) != reset ||
        maia_esp32p4_ldo3_snapshot(&supply_after) != MAIA_LDO3_OK ||
        !display_native_same_supply(&supply, &supply_after)) {
        return display_native_fail("panel reset recovery checks", MAIA_I2C_VERIFY);
    }
    maia_console_puts("[DISPLAY] ST7121 physical reset verified\n");
    return 0;
}

static int display_native_read_id(uint32_t *id) {
    uint32_t first = 0U;
    for (uint32_t i = 0; i < 4U; ++i) {
        maia_dsi_read_report_t report;
        maia_dsi_access_result_t result = maia_esp32p4_dsi_read_id(10000U, &report);
        uint32_t current = report.word & 0x00FFFFFFU;
        if (result != MAIA_DSI_ACCESS_OK || report.transfer.completed != 10U ||
            report.words_received != 1U || (i != 0U && current != first)) {
            return -1;
        }
        first = current;
    }
    *id = first;
    return 0;
}

static int display_native_read_power_mode(uint8_t *power_mode) {
    uint8_t first = 0U;
    for (uint32_t i = 0; i < 4U; ++i) {
        maia_dsi_read_report_t report;
        maia_dsi_access_result_t result =
            maia_esp32p4_dsi_read_power_mode(10000U, &report);
        uint8_t current = (uint8_t)(report.word & 0xFFU);
        if (result != MAIA_DSI_ACCESS_OK || report.transfer.completed != 10U ||
            report.words_received != 1U || (i != 0U && current != first)) {
            return -1;
        }
        first = current;
    }
    *power_mode = first;
    return 0;
}

static int display_native_read_color_mode(uint8_t *color_mode) {
    uint8_t first = 0U;
    for (uint32_t i = 0; i < 4U; ++i) {
        maia_dsi_read_report_t report;
        maia_dsi_access_result_t result =
            maia_esp32p4_dsi_read_color_mode(10000U, &report);
        uint8_t current = (uint8_t)(report.word & 0xFFU);
        if (result != MAIA_DSI_ACCESS_OK || report.transfer.completed != 10U ||
            report.words_received != 1U || (i != 0U && current != first)) {
            return -1;
        }
        first = current;
    }
    *color_mode = first;
    return 0;
}

static int display_native_dsi_setup(uint8_t firmware) {
    maia_ldo3_acquire_report_t supply;
    maia_ldo3_result_t supply_result = maia_esp32p4_ldo3_acquire2500(&supply);
    if (supply_result != MAIA_LDO3_OK ||
        supply.completed != MAIA_LDO3_ACQUIRE_VERIFIED) {
        return display_native_fail("VO3 acquisition", supply_result);
    }
    maia_udelay(10000U);
    if (display_native_reset_panel(firmware) != 0) {
        return -1;
    }

    maia_dsi_access_report_t access;
    maia_dsi_access_result_t result = maia_esp32p4_dsi_open_config(&access);
    if (result != MAIA_DSI_ACCESS_OK ||
        access.completed != MAIA_DSI_ACCESS_VERIFIED) {
        return display_native_fail("DSI access", result);
    }

    maia_dsi_reference_report_t reference;
    result = maia_esp32p4_dsi_reference20(&reference);
    if (result != MAIA_DSI_ACCESS_OK ||
        reference.completed != MAIA_DSI_REFERENCE_VERIFIED) {
        return display_native_fail("DSI reference", result);
    }

    maia_dsi_phy_report_t phy;
    result = maia_esp32p4_dsi_phy960(100000U, &phy);
    if (result != MAIA_DSI_ACCESS_OK || phy.completed != 11U) {
        return display_native_fail("DSI PHY", result);
    }

    maia_dsi_command_report_t command;
    result = maia_esp32p4_dsi_command_config(&command);
    if (result != MAIA_DSI_ACCESS_OK || command.completed != 16U) {
        return display_native_fail("DSI command mode", result);
    }

    uint32_t id_before;
    uint8_t power_before;
    if (display_native_read_id(&id_before) != 0 ||
        display_native_read_power_mode(&power_before) != 0) {
        return display_native_fail("initial panel readback", MAIA_DSI_ACCESS_IO);
    }
    maia_console_puts("[DISPLAY] DSI panel id=");
    display_native_print_hex32(id_before);
    maia_console_puts(" power=");
    display_native_print_hex32(power_before);
    maia_console_puts("\n");

    maia_st7121_io_t io = {NULL, display_native_write_dcs, display_native_wait_ms};
    maia_st7121_report_t program;
    maia_st7121_result_t program_result = maia_tab5_st7121_program(firmware, &io, &program);
    if (program_result != MAIA_ST7121_OK || program.completed != 31U) {
        return display_native_fail("ST7121 program", (int)program_result);
    }

    uint32_t id_after;
    uint8_t power_after;
    if (display_native_read_id(&id_after) != 0 || id_after != id_before ||
        display_native_read_power_mode(&power_after) != 0 ||
        (power_after & 0x14U) != 0x14U) {
        return display_native_fail("panel wakeup readback", MAIA_DSI_ACCESS_IO);
    }

    maia_dsi_dpi_timing_t timing = {720U, 1280U, 2U, 40U, 40U, 20U, 24U, 200U};
    maia_dsi_dpi_report_t dpi;
    result = maia_esp32p4_dsi_dpi_stage(&timing, &dpi);
    if (result != MAIA_DSI_ACCESS_OK || dpi.completed != 22U) {
        return display_native_fail("DPI timing stage", result);
    }

    uint8_t color_before;
    if (display_native_read_color_mode(&color_before) != 0 ||
        ((color_before & 7U) != 0U && (color_before & 7U) != 5U)) {
        return display_native_fail("COLMOD readback", MAIA_DSI_ACCESS_IO);
    }
    if ((color_before & 7U) != 5U) {
        maia_dsi_short_report_t write;
        result = maia_esp32p4_dsi_dcs_short(0U, 0x3AU, 1U, 0x55U, 10000U, &write);
        if (result != MAIA_DSI_ACCESS_OK) {
            return display_native_fail("COLMOD write", result);
        }
        maia_udelay(5000U);
    }
    uint8_t color_after;
    if (display_native_read_color_mode(&color_after) != 0 ||
        (color_after & 7U) != 5U) {
        return display_native_fail("COLMOD verification", MAIA_DSI_ACCESS_IO);
    }

    maia_dsi_dpi_clock_report_t clock;
    result = maia_esp32p4_dsi_dpi_clock_enable(&clock);
    if (result != MAIA_DSI_ACCESS_OK || clock.completed != 4U) {
        return display_native_fail("DPI clock enable", result);
    }

    maia_dsi_dpi_bridge_report_t bridge;
    result = maia_esp32p4_dsi_dpi_bridge_prepare(&bridge);
    if (result != MAIA_DSI_ACCESS_OK || bridge.completed != 4U) {
        return display_native_fail("DPI bridge preparation", result);
    }
    return 0;
}

int maia_esp32p4_display_st7121_prepare(uint8_t firmware) {
    uint32_t hart;
    uint32_t status;
    __asm__ volatile("csrr %0, mhartid" : "=r"(hart));
    __asm__ volatile("csrr %0, mstatus" : "=r"(status));
    if (hart != 0U || (status & 8U) != 0U) {
        return display_native_fail("execution context", MAIA_MIPI_PARK_CONTEXT);
    }
    if (firmware != 1U) {
        return display_native_fail("unsupported panel firmware", MAIA_ST7121_UNSUPPORTED);
    }
    if (display_native_prepare_mipi() != 0) {
        return -1;
    }
    return display_native_dsi_setup(firmware);
}
