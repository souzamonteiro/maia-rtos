/* RAM-only read-only touch identity probe. No expander, reset, display or DMA. */
#include <maia/boards/tab5_touch.h>
#include <maia/soc/esp32p4_boot_i2c.h>
#include <maia/console.h>
#include <maia/kernel.h>
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
void maia_boot_main(void) {
    maia_console_puts("MAIA_TOUCH_PROBE boot-only GPIO31/32 no power/reset/DMA writes\n");
    maia_i2c_result_t r=maia_esp32p4_boot_i2c_prepare(&bus,31U,32U);
    if (r != MAIA_I2C_OK) {
        maia_console_puts("MAIA_NATIVE_TOUCH_FAIL prepare\n");
        for (;;) __asm__ volatile("wfi");
    }
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
