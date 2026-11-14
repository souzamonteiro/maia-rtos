/* Private boot diagnostic integration. Existing native reset/transport retain
 * their original attribution; references: docs/audits/native-dsi-reset.md. */
#include <maia/boards/tab5_panel_reset.h>
#include <maia/boards/tab5_touch.h>
#include <maia/soc/esp32p4_boot_i2c.h>
extern uint64_t maia_soc_time_us(void);
static maia_esp32p4_boot_i2c_t reset_bus;
static int dsi_reset_input(void) {
    return !(rd(MAIA_ESP32P4_GPIO_ENABLE_REG)&(1U<<23U)) &&
        rd(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(23U)) == (MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX|MAIA_ESP32P4_GPIO_FUNC_OEN_SEL) &&
        rd(MAIA_ESP32P4_IO_MUX_GPIO_REG(23U)) == (MAIA_ESP32P4_IO_MUX_MCU_SEL_GPIO|MAIA_ESP32P4_IO_MUX_FUN_IE);
}
static void dsi_panel_reset(const maia_mipi_gate_report_t *g,uint32_t cpu) {
    maia_ldo3_snapshot_t supply,now;maia_ldo3_plan_t plan;
    if (!vo3_cycle_guard(g,cpu,rd) || maia_esp32p4_ldo3_snapshot(&supply) != MAIA_LDO3_OK ||
        maia_esp32p4_ldo3_plan2500(&supply,&plan) != MAIA_LDO3_OK ||
        (supply.control&MAIA_ESP32P4_LDO3_CONTROL_MASK) != 0x180U ||
        (supply.analog&MAIA_ESP32P4_LDO3_ANALOG_MASK) !=
          (((uint32_t)plan.dref<<28U)|((uint32_t)plan.mul<<23U)|(1U<<26U))) halt("panel-reset-quiet");
    if (maia_esp32p4_boot_i2c_prepare(&reset_bus,31,32) != MAIA_I2C_OK) halt("panel-reset-bus");
    uint8_t firmware=0,after_fw=0,status=0xFF;
    if (maia_tab5_touch_identify(&reset_bus.lines,&firmware) != MAIA_I2C_OK ||
        (firmware != 1U && firmware != 3U)) halt("panel-reset-variant");
    /* ST712x only; never apply a GT911 address strap. Disable output first. */
    wr(MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG,1U<<23U);
    wr(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(23U),MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX|MAIA_ESP32P4_GPIO_FUNC_OEN_SEL);
    wr(MAIA_ESP32P4_IO_MUX_GPIO_REG(23U),MAIA_ESP32P4_IO_MUX_MCU_SEL_GPIO|MAIA_ESP32P4_IO_MUX_FUN_IE);
    if (!dsi_reset_input()) halt("panel-reset-gpio23");
    const uint8_t regs[]={3,5,7,0x0B,0x0D},masks[]={0x30,0x30,0x30,0x10,0x10},bits[]={0x20,0x20,0,0x10,0x10};
    uint8_t banks[2][5];
    for (unsigned b=0;b<2U;++b) for (unsigned i=0;i<5U;++i)
        if (maia_pi4io_read_config(&reset_bus.lines,(uint8_t)(0x43U+b),regs[i],&banks[b][i]) != MAIA_I2C_OK) halt("panel-reset-snapshot");
    if (!dsi_reset_input() || !vo3_cycle_guard(g,cpu,rd) || maia_esp32p4_ldo3_snapshot(&now) != MAIA_LDO3_OK || !vo3_same(&supply,&now)) halt("panel-reset-precondition");
    field("DSI_RESET_QUIET sys=",g->sys_after);field(" phy=",g->phy_after);field(" reset=",g->reset_after);
    field(" firmware=",firmware);maia_console_puts(" dma_quiet=00000001 supply_ready=00000001\n");
    maia_tab5_reset_report_t report;
    uint64_t start=maia_soc_time_us();
    maia_i2c_result_t result=maia_tab5_panel_reset(&reset_bus.lines,&report);
    uint64_t elapsed=maia_soc_time_us()-start;
    field("DSI_PANEL_RESET result=",(uint32_t)result);field(" stage=",report.completed);
    field(" changed=",report.may_have_changed);field(" elapsed_us=",(uint32_t)elapsed);maia_console_puts("\n");
    if (result != MAIA_I2C_OK || report.completed != MAIA_TAB5_RESET_VERIFIED || !report.may_have_changed || elapsed<200000U || elapsed>0xFFFFFFFFU) halt("panel-reset-sequence");
    for (unsigned b=0;b<2U;++b) for (unsigned i=0;i<5U;++i) {
        uint8_t value,expected=b ? banks[b][i] : (uint8_t)((banks[b][i]&~masks[i])|bits[i]);
        if (maia_pi4io_read_config(&reset_bus.lines,(uint8_t)(0x43U+b),regs[i],&value) != MAIA_I2C_OK || value != expected) halt("panel-reset-banks");
    }
    if (maia_i2c_read_register16(&reset_bus.lines,0x55,1,&status) != MAIA_I2C_OK || status != 0U ||
        maia_tab5_touch_identify(&reset_bus.lines,&after_fw) != MAIA_I2C_OK || after_fw != firmware ||
        reset_bus.lines.sample(reset_bus.lines.context) != MAIA_I2C_BOTH || !dsi_reset_input() || !vo3_cycle_guard(g,cpu,rd) ||
        maia_esp32p4_ldo3_snapshot(&now) != MAIA_LDO3_OK || !vo3_same(&supply,&now)) halt("panel-reset-recovery");
    field("DSI_RESET_RECOVERY firmware=",after_fw);field(" status=",status);
    maia_console_puts(" banks_preserved=00000001 clocks_preserved=00000001 supply_preserved=00000001 gpio23_input=00000001\nMAIA_NATIVE_DSI_RESET_PASS\n");
}
