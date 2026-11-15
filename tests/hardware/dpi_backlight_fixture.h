/* Private boot diagnostic only. Tab5 PDF update2026-11-05 p12 G22/LEDA;
 * working BSP dc-high100% policy. GPIO fields: SDK f5c3654 gpio_reg/io_mux_reg.
 * Original checked save/restore, no LEDC/vendor runtime implementation copied. */
#include <maia/soc/esp32p4_regs.h>
typedef struct dpi_backlight_snapshot { uint32_t out,enable,pad,mux,matrix; } dpi_backlight_snapshot_t;
#define DPI_BL_MASK (1U<<22U)
static void dpi_bl_snapshot(uint32_t (*read)(uintptr_t),dpi_backlight_snapshot_t *s) {
    s->out=read(MAIA_ESP32P4_GPIO_OUT_REG);s->enable=read(MAIA_ESP32P4_GPIO_ENABLE_REG);
    s->pad=read(MAIA_ESP32P4_GPIO_PIN_REG(22));s->mux=read(MAIA_ESP32P4_IO_MUX_GPIO_REG(22));
    s->matrix=read(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22));
}
static int dpi_bl_set(uint32_t (*read)(uintptr_t),void (*write)(uintptr_t,uint32_t),uintptr_t address,uint32_t value,uintptr_t mirror,uint32_t expected) {
    write(address,value);return read(mirror) == expected;
}
static int dpi_bl_on(uint32_t (*read)(uintptr_t),void (*write)(uintptr_t,uint32_t),dpi_backlight_snapshot_t *s) {
    dpi_bl_snapshot(read,s);
    if (!dpi_bl_set(read,write,MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG,DPI_BL_MASK,MAIA_ESP32P4_GPIO_ENABLE_REG,s->enable&~DPI_BL_MASK) ||
        !dpi_bl_set(read,write,MAIA_ESP32P4_GPIO_OUT_W1TS_REG,DPI_BL_MASK,MAIA_ESP32P4_GPIO_OUT_REG,s->out|DPI_BL_MASK) ||
        !dpi_bl_set(read,write,MAIA_ESP32P4_GPIO_PIN_REG(22),s->pad&~4U,MAIA_ESP32P4_GPIO_PIN_REG(22),s->pad&~4U) ||
        !dpi_bl_set(read,write,MAIA_ESP32P4_IO_MUX_GPIO_REG(22),(s->mux&~0x7000U)|0x1000U,MAIA_ESP32P4_IO_MUX_GPIO_REG(22),(s->mux&~0x7000U)|0x1000U) ||
        !dpi_bl_set(read,write,MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22),(s->matrix&~0xFFFU)|0x500U,MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22),(s->matrix&~0xFFFU)|0x500U) ||
        !dpi_bl_set(read,write,MAIA_ESP32P4_GPIO_ENABLE_W1TS_REG,DPI_BL_MASK,MAIA_ESP32P4_GPIO_ENABLE_REG,s->enable|DPI_BL_MASK)) return 0;
    return 1;
}
static int dpi_bl_restore(uint32_t (*read)(uintptr_t),void (*write)(uintptr_t,uint32_t),const dpi_backlight_snapshot_t *s) {
    uint32_t enable=read(MAIA_ESP32P4_GPIO_ENABLE_REG)&~DPI_BL_MASK;
    if (!dpi_bl_set(read,write,MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG,DPI_BL_MASK,MAIA_ESP32P4_GPIO_ENABLE_REG,enable) ||
        !dpi_bl_set(read,write,(s->out&DPI_BL_MASK) ? MAIA_ESP32P4_GPIO_OUT_W1TS_REG : MAIA_ESP32P4_GPIO_OUT_W1TC_REG,DPI_BL_MASK,MAIA_ESP32P4_GPIO_OUT_REG,s->out) ||
        !dpi_bl_set(read,write,MAIA_ESP32P4_GPIO_PIN_REG(22),s->pad,MAIA_ESP32P4_GPIO_PIN_REG(22),s->pad) ||
        !dpi_bl_set(read,write,MAIA_ESP32P4_IO_MUX_GPIO_REG(22),s->mux,MAIA_ESP32P4_IO_MUX_GPIO_REG(22),s->mux) ||
        !dpi_bl_set(read,write,MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22),s->matrix,MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(22),s->matrix) ||
        !dpi_bl_set(read,write,(s->enable&DPI_BL_MASK) ? MAIA_ESP32P4_GPIO_ENABLE_W1TS_REG : MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG,DPI_BL_MASK,MAIA_ESP32P4_GPIO_ENABLE_REG,s->enable)) return 0;
    return 1;
}
