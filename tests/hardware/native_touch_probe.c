/* RAM touch probe; optional expander OUT05 same-value diagnostic. No panel/DMA. */
#include <maia/boards/tab5_touch.h>
#include <maia/soc/esp32p4_boot_i2c.h>
#include <maia/console.h>
#include <maia/kernel.h>
#ifdef MAIA_NATIVE_PROBE_EXPANDER
#include <maia/drivers/pi4ioe5v6408.h>
#endif
#ifdef MAIA_NATIVE_PROBE_PANEL_RESET
#include <maia/boards/tab5_panel_reset.h>
#include <maia/soc/esp32p4_regs.h>
extern uint64_t maia_soc_time_us(void);
#endif
static maia_esp32p4_boot_i2c_t bus;
static unsigned errors;
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }
static void number(uint32_t n) {
    char digits[10];unsigned count=0U;
    do { digits[count++]=(char)('0'+n%10U);n/=10U; } while(n);
    while(count) maia_console_putc(digits[--count]);
}
static void check(int ok) { if (!ok) ++errors; }
static unsigned forced_low(void *context) {
    return bus.lines.sample(context) & ~MAIA_I2C_SCL;
}
#ifdef MAIA_NATIVE_PROBE_EXPANDER
static void expander(uint8_t address) {
    const uint8_t regs[]={MAIA_PI4IO_DIRECTION,MAIA_PI4IO_OUTPUT,MAIA_PI4IO_HIGH_Z,
        MAIA_PI4IO_PULL_ENABLE,MAIA_PI4IO_PULL_SELECT};
    uint8_t before[5];unsigned rounds=0U,mirror=0U,unchanged=1U;
    for (unsigned i=0U;i<5U;++i) {
        if (maia_pi4io_read_config(&bus.lines,address,regs[i],&before[i]) != MAIA_I2C_OK) {
            ++errors;return; /* Never write when snapshot is incomplete. */
        }
    }
    /* OUT05 is a plain output latch, not reset/IRQ control. Mirror its exact
     * byte to exercise an ACKed data write without requesting output edges. */
    if (maia_i2c_write_register8(&bus.lines,address,MAIA_PI4IO_OUTPUT,before[1]) != MAIA_I2C_OK) {
        ++errors;return; /* No retry/rollback after uncertain write completion. */
    }
    mirror=1U;
    check(maia_pi4io_update_config(&bus.lines,address,MAIA_PI4IO_OUTPUT,0x10U,
                                  before[1] & 0x10U) == MAIA_I2C_OK);
    for (unsigned round=0U;round<16U;++round) {
        for (unsigned i=0U;i<5U;++i) {
            uint8_t value=0U;
            if (maia_pi4io_read_config(&bus.lines,address,regs[i],&value) != MAIA_I2C_OK || value != before[i]) {
                unchanged=0U;++errors;return;
            }
        }
        ++rounds;
    }
    maia_console_puts("EXPANDER bank=");number(address);maia_console_puts(" rounds=");number(rounds);
    maia_console_puts(" mirror=");number(mirror);maia_console_puts(" unchanged=");number(unchanged);
    maia_console_puts(" dir=");number(before[0]);maia_console_puts(" out=");number(before[1]);maia_console_puts("\n");
}
#endif
#ifdef MAIA_NATIVE_PROBE_PANEL_RESET
static int reset_panel(void) {
    const uint8_t regs[]={3U,5U,7U,0x0BU,0x0DU};
    const uint8_t masks[]={0x30U,0x30U,0x30U,0x10U,0x10U};
    const uint8_t bits[]={0x20U,0x20U,0U,0x10U,0x10U};
    uint8_t before[2][5],fw;
    if (maia_tab5_touch_identify(&bus.lines,&fw) != MAIA_I2C_OK || (fw != 1U && fw != 3U)) return 0;
    /* ST712x test precondition: TP_INT input, no GT911 address strap. */
    MAIA_MMIO32(MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG)=1UL<<23U;
    MAIA_MMIO32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(23U))=
        MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX | MAIA_ESP32P4_GPIO_FUNC_OEN_SEL;
    MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(23U))=
        MAIA_ESP32P4_IO_MUX_MCU_SEL_GPIO | MAIA_ESP32P4_IO_MUX_FUN_IE;
    for (unsigned b=0U;b<2U;++b) for (unsigned i=0U;i<5U;++i)
        if (maia_pi4io_read_config(&bus.lines,(uint8_t)(0x43U+b),regs[i],&before[b][i]) != MAIA_I2C_OK) return 0;
    maia_tab5_reset_report_t report;
    uint64_t start=maia_soc_time_us();
    maia_i2c_result_t result=maia_tab5_panel_reset(&bus.lines,&report);
    uint64_t elapsed=maia_soc_time_us()-start;
    if (result != MAIA_I2C_OK) {
        maia_console_puts("PANEL_RESET error_code=");number((uint32_t)(-result));
        maia_console_puts(" stage=");number((uint32_t)report.completed);maia_console_puts("\n");return 0;
    }
    unsigned preserved=1U;
    for (unsigned b=0U;b<2U;++b) for (unsigned i=0U;i<5U;++i) {
        uint8_t value;
        uint8_t expected=b ? before[b][i] : (uint8_t)((before[b][i] & (uint8_t)~masks[i]) | bits[i]);
        if (maia_pi4io_read_config(&bus.lines,(uint8_t)(0x43U+b),regs[i],&value) != MAIA_I2C_OK || value != expected) preserved=0U;
    }
    unsigned input=!(MAIA_MMIO32(MAIA_ESP32P4_GPIO_ENABLE_REG)&(1UL<<23U)) &&
        (MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(23U))&MAIA_ESP32P4_IO_MUX_FUN_IE);
    maia_console_puts("PANEL_RESET stage=");number((uint32_t)report.completed);
    maia_console_puts(" changed=");number(report.may_have_changed);maia_console_puts(" preserved=");number(preserved);
    maia_console_puts(" elapsed_us=");number((uint32_t)elapsed);maia_console_puts(" gpio23_input=");number(input);maia_console_puts("\n");
    return report.completed == MAIA_TAB5_RESET_VERIFIED && report.may_have_changed && preserved && input && elapsed>=200000U;
}
#endif
void maia_boot_main(void) {
    maia_console_puts("MAIA_TOUCH_PROBE boot-only GPIO31/32 no panel/DMA initialization\n");
    maia_i2c_result_t r=maia_esp32p4_boot_i2c_prepare(&bus,31U,32U);
    if (r != MAIA_I2C_OK) {
        maia_console_puts("MAIA_NATIVE_TOUCH_FAIL prepare\n");
        for (;;) __asm__ volatile("wfi");
    }
#ifdef MAIA_NATIVE_PROBE_EXPANDER
    maia_console_puts("EXPANDER_PROBE same-value OUT05 writes; no reset/power transitions requested\n");
    expander(0x43U);expander(0x44U);
    maia_i2c_result_t absent_write=maia_i2c_write_register8(&bus.lines,0x56U,5U,0U);
    check(absent_write == MAIA_I2C_NACK);
    maia_console_puts(absent_write == MAIA_I2C_NACK ? "EXPANDER_ABSENT_WRITE NACK\n" : "EXPANDER_ABSENT_WRITE FAIL\n");
#endif
#ifdef MAIA_NATIVE_PROBE_PANEL_RESET
    if (!reset_panel()) {
        maia_console_puts("MAIA_NATIVE_TOUCH_FAIL panel reset\n");
        for (;;) __asm__ volatile("wfi");
    }
#endif
    uint8_t raw_status=0xCCU;
    maia_i2c_result_t status_result=maia_i2c_read_register16(&bus.lines,0x55U,1U,&raw_status);
    maia_console_puts("TOUCH_STATUS result_code=");number((uint32_t)(-status_result));
    maia_console_puts(" raw=");number(raw_status);maia_console_puts("\n");
    uint32_t first=0U,last=0U,completed=0U;
    for (unsigned i=0U;i<16U;++i) {
        uint8_t fw=0xCCU;
        r=maia_tab5_touch_identify(&bus.lines,&fw);
        if (r != MAIA_I2C_OK) { maia_console_puts("TOUCH_READ error_code=");number((uint32_t)(-r));maia_console_puts("\n");++errors;break; }
        if (!i) first=fw;
        last=fw;check(first == last);++completed;
        check(bus.lines.sample(bus.lines.context) == MAIA_I2C_BOTH);
    }
    uint8_t sentinel=0xCCU;
    maia_i2c_result_t absent=maia_i2c_read_register16(&bus.lines,0x56U,0,&sentinel);
    check(absent == MAIA_I2C_NACK && sentinel == 0xCCU);
    check(bus.lines.sample(bus.lines.context) == MAIA_I2C_BOTH);
    /* Inject only the sampled SCL level, not an electrical short or GPIO low. */
    maia_i2c_lines_t injected=bus.lines;injected.sample=forced_low;
    maia_i2c_result_t timeout=maia_i2c_read_register16(&injected,0x55U,0,&sentinel);
    check(timeout == MAIA_I2C_TIMEOUT && sentinel == 0xCCU);
    check(bus.lines.sample(bus.lines.context) == MAIA_I2C_BOTH);
    uint8_t final=0xCCU;
    check(maia_tab5_touch_identify(&bus.lines,&final) == MAIA_I2C_OK && final == first);
    maia_console_puts("TOUCH_ID reads=");number(completed);maia_console_puts(" firmware=");number(last);
    maia_console_puts(" final=");number(final);maia_console_puts(" errors=");number(errors);maia_console_puts("\n");
    maia_console_puts(absent == MAIA_I2C_NACK ? "TOUCH_ABSENT address=0x56 NACK\n" : "TOUCH_ABSENT FAIL\n");
    maia_console_puts(timeout == MAIA_I2C_TIMEOUT ? "TOUCH_INJECT sampled_SCL_low TIMEOUT\n" : "TOUCH_INJECT FAIL\n");
    check(completed == 16U && (first == 1U || first == 3U));
    maia_console_puts(errors ? "MAIA_NATIVE_TOUCH_FAIL\n" : "MAIA_NATIVE_TOUCH_PASS\n");
    for (;;) __asm__ volatile("wfi");
}
