#include <maia/boards/tab5_i2s.h>
#include <maia/soc/esp32p4_regs.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "line %d: %s\n", __LINE__, #expr); exit(1); \
} } while (0)

#define I2S_TX_CONF MAIA_ESP32P4_I2S1_TX_CONF_REG
#define I2S_TX_CONF1 MAIA_ESP32P4_I2S1_TX_CONF1_REG
#define I2S_TDM_CTRL MAIA_ESP32P4_I2S1_TX_TDM_CTRL_REG
#define I2S_RESET MAIA_ESP32P4_HP_CLKRST_HP_RST_EN2_REG
#define DMA_LINK MAIA_ESP32P4_AHB_DMA_OUT_LINK_CH0_REG
#define DMA_LINK_ADDR MAIA_ESP32P4_AHB_DMA_OUT_LINK_ADDR_CH0_REG
#define DMA_OWNER (UINT32_C(1) << 31U)
#define DMA_PARK MAIA_ESP32P4_AHB_DMA_OUTLINK_PARK_CH0

typedef struct {
    uintptr_t address;
    uint32_t value;
} reg_t;

static reg_t regs[128];
static unsigned reg_count;
static unsigned i2s_reset_assertions;

static reg_t *find_reg(uintptr_t address, int create) {
    for (unsigned i = 0U; i < reg_count; ++i) {
        if (regs[i].address == address) return &regs[i];
    }
    if (!create) return NULL;
    CHECK(reg_count < sizeof regs / sizeof regs[0]);
    regs[reg_count] = (reg_t){address, 0U};
    return &regs[reg_count++];
}

uint32_t maia_tab5_i2s_test_read(uintptr_t address) {
    reg_t *reg = find_reg(address, 0);
    return reg ? reg->value : 0U;
}

void maia_tab5_i2s_test_write(uintptr_t address, uint32_t value) {
    reg_t *reg = find_reg(address, 1);
    if (address == I2S_RESET &&
        (value & MAIA_ESP32P4_HP_CLKRST_I2S1_APB_RESET)) {
        ++i2s_reset_assertions;
    }
    if (address == I2S_TX_CONF && (value & MAIA_ESP32P4_I2S1_TX_UPDATE)) {
        value &= ~MAIA_ESP32P4_I2S1_TX_UPDATE;
    }
    if (address == DMA_LINK) {
        if (value & MAIA_ESP32P4_AHB_DMA_OUTLINK_START_CH0) value = 0U;
        if (value & MAIA_ESP32P4_AHB_DMA_OUTLINK_RESTART_CH0) value = 0U;
        if (value & MAIA_ESP32P4_AHB_DMA_OUTLINK_STOP_CH0) value = DMA_PARK;
    }
    reg->value = value;
}

uintptr_t maia_arch_irq_save(void) { return 0U; }
void maia_arch_irq_restore(uintptr_t state) { (void)state; }
void maia_tab5_i2s_test_reset(void);
uint32_t *maia_tab5_i2s_test_descriptor(unsigned index);

int main(void) {
    maia_tab5_i2s_test_reset();
    reg_count = 0U;
    i2s_reset_assertions = 0U;
    unsigned preinit_index = 0U;
    int16_t *preinit_samples = NULL;
    CHECK(maia_tab5_i2s_tx_acquire(&preinit_index, &preinit_samples) ==
          MAIA_TAB5_I2S_NOT_INITIALIZED);
    CHECK(maia_tab5_i2s_tx_init() == MAIA_TAB5_I2S_OK);
    CHECK(maia_tab5_i2s_tx_init() == MAIA_TAB5_I2S_OK);
    CHECK(i2s_reset_assertions == 1U);
    CHECK(!(maia_tab5_i2s_test_read(I2S_RESET) &
            MAIA_ESP32P4_HP_CLKRST_I2S1_APB_RESET));

    uint32_t clock15 = maia_tab5_i2s_test_read(MAIA_ESP32P4_HP_CLKRST_I2S1_CLK_CTRL15_REG);
    uint32_t clock16 = maia_tab5_i2s_test_read(MAIA_ESP32P4_HP_CLKRST_I2S1_CLK_CTRL16_REG);
    uint32_t clock17 = maia_tab5_i2s_test_read(MAIA_ESP32P4_HP_CLKRST_I2S1_CLK_CTRL17_REG);
    CHECK(maia_tab5_i2s_test_read(MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL2_REG) &
          MAIA_ESP32P4_HP_CLKRST_I2S1_APB_EN);
    CHECK(clock15 & MAIA_ESP32P4_HP_CLKRST_I2S1_TX_CLK_EN);
    CHECK((clock15 & MAIA_ESP32P4_HP_CLKRST_I2S1_TX_CLK_SRC_MASK) == 0U);
    CHECK((clock16 & MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_N_MASK) == 3U);
    CHECK(((clock16 & MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_X_MASK) >>
           MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_X_SHIFT) == 3U);
    CHECK(((clock16 & MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Y_MASK) >>
           MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Y_SHIFT) == 45U);
    CHECK((clock17 & MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Z_MASK) == 49U);
    CHECK(!(clock17 & MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_YN1));
    CHECK(clock17 & MAIA_ESP32P4_HP_CLKRST_I2S1_MST_CLK_SEL);
    CHECK(maia_tab5_i2s_test_read(MAIA_ESP32P4_LP_CLKRST_HP_CLK_CTRL_REG) &
          MAIA_ESP32P4_LP_CLKRST_I2S1_MCLK_EN);
    CHECK(!(maia_tab5_i2s_test_read(I2S_TX_CONF) & MAIA_ESP32P4_I2S1_TX_UPDATE));
    CHECK((maia_tab5_i2s_test_read(I2S_TX_CONF) &
           (MAIA_ESP32P4_I2S1_TX_TDM_ENABLE |
            MAIA_ESP32P4_I2S1_TX_MSB_SHIFT |
            MAIA_ESP32P4_I2S1_TX_STOP_ENABLE)) ==
          (MAIA_ESP32P4_I2S1_TX_TDM_ENABLE |
           MAIA_ESP32P4_I2S1_TX_MSB_SHIFT |
           MAIA_ESP32P4_I2S1_TX_STOP_ENABLE));
    CHECK(((maia_tab5_i2s_test_read(I2S_TX_CONF) &
            MAIA_ESP32P4_I2S1_TX_BCK_DIV_MASK) >>
           MAIA_ESP32P4_I2S1_TX_BCK_DIV_SHIFT) == 7U);
    CHECK((maia_tab5_i2s_test_read(I2S_TX_CONF1) &
           MAIA_ESP32P4_I2S1_TX_CONF1_BITS_MOD_MASK) ==
          (15U << MAIA_ESP32P4_I2S1_TX_CONF1_BITS_MOD_SHIFT));
    CHECK((maia_tab5_i2s_test_read(I2S_TDM_CTRL) & 0x000FFFFFUL) == 0x00010003UL);
    CHECK((maia_tab5_i2s_test_read(MAIA_ESP32P4_AHB_DMA_OUT_PERI_SEL_CH0_REG) & 0x3FU) == 4U);
    CHECK(maia_tab5_i2s_test_read(MAIA_ESP32P4_AHB_DMA_OUT_CONF0_CH0_REG) &
          MAIA_ESP32P4_AHB_DMA_OUT_AUTO_WRBACK_CH0);
    CHECK(maia_tab5_i2s_test_read(MAIA_ESP32P4_AHB_DMA_OUT_CONF1_CH0_REG) &
          MAIA_ESP32P4_AHB_DMA_OUT_CHECK_OWNER_CH0);
    CHECK(maia_tab5_i2s_test_read(MAIA_ESP32P4_AHB_DMA_INTR_MEM_START_ADDR_REG) == 0x4FF00000U);
    CHECK(maia_tab5_i2s_test_read(MAIA_ESP32P4_AHB_DMA_INTR_MEM_END_ADDR_REG) == 0x4FFC0000U);
    CHECK(maia_tab5_i2s_test_read(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(30U)) ==
          (MAIA_ESP32P4_I2S1_MCLK_PAD_OUT_IDX | MAIA_ESP32P4_GPIO_FUNC_OEN_SEL));
    CHECK(maia_tab5_i2s_test_read(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(26U)) ==
          (MAIA_ESP32P4_I2S1_O_SD_PAD_OUT_IDX | MAIA_ESP32P4_GPIO_FUNC_OEN_SEL));

    unsigned first_index = 99U;
    unsigned second_index = 99U;
    int16_t *first_samples = NULL;
    int16_t *second_samples = NULL;
    CHECK(maia_tab5_i2s_tx_acquire(&first_index, &first_samples) == MAIA_TAB5_I2S_OK);
    CHECK(first_index == 0U && first_samples != NULL);
    CHECK(maia_tab5_i2s_tx_submit(first_index, 0U) == MAIA_TAB5_I2S_INVALID);
    CHECK(maia_tab5_i2s_tx_submit(first_index, MAIA_TAB5_I2S_TX_BUFFER_FRAMES + 1U) ==
          MAIA_TAB5_I2S_INVALID);
    CHECK(maia_tab5_i2s_tx_submit(MAIA_TAB5_I2S_TX_BUFFER_COUNT, 1U) ==
          MAIA_TAB5_I2S_INVALID);
    CHECK(maia_tab5_i2s_tx_submit(first_index, MAIA_TAB5_I2S_TX_BUFFER_FRAMES) ==
          MAIA_TAB5_I2S_OK);

    uint32_t *first_desc = maia_tab5_i2s_test_descriptor(first_index);
    CHECK(first_desc != NULL);
    CHECK((first_desc[0] & 0xFFFU) == 1920U);
    CHECK(((first_desc[0] >> 12U) & 0xFFFU) == 1920U);
    CHECK(first_desc[0] & DMA_OWNER);
    CHECK(first_desc[1] == (uint32_t)(uintptr_t)first_samples);
    CHECK(first_desc[2] == (uint32_t)(uintptr_t)maia_tab5_i2s_test_descriptor(1U));
    CHECK(maia_tab5_i2s_test_read(DMA_LINK_ADDR) ==
          (uint32_t)(uintptr_t)maia_tab5_i2s_test_descriptor(0U));
    CHECK(maia_tab5_i2s_test_read(I2S_TX_CONF) & MAIA_ESP32P4_I2S1_TX_START);

    CHECK(maia_tab5_i2s_tx_acquire(&second_index, &second_samples) == MAIA_TAB5_I2S_OK);
    CHECK(second_index == 1U && second_samples != NULL);
    CHECK(maia_tab5_i2s_tx_submit(second_index, 240U) == MAIA_TAB5_I2S_OK);
    CHECK(maia_tab5_i2s_tx_acquire(&second_index, &second_samples) == MAIA_TAB5_I2S_BUSY);

    first_desc[0] &= ~DMA_OWNER;
    CHECK(maia_tab5_i2s_tx_acquire(&first_index, &first_samples) == MAIA_TAB5_I2S_OK);
    CHECK(first_index == 0U);
    find_reg(DMA_LINK, 1)->value = DMA_PARK;
    CHECK(maia_tab5_i2s_tx_submit(first_index, 100U) == MAIA_TAB5_I2S_OK);
    CHECK(maia_tab5_i2s_test_read(DMA_LINK) == 0U);
    CHECK(((first_desc[0] >> 12U) & 0xFFFU) == 400U);

    CHECK(maia_tab5_i2s_tx_stop() == MAIA_TAB5_I2S_OK);
    CHECK(!(maia_tab5_i2s_test_read(I2S_TX_CONF) & MAIA_ESP32P4_I2S1_TX_START));
    CHECK(maia_tab5_i2s_tx_submit(first_index, 100U) == MAIA_TAB5_I2S_INVALID);
    CHECK(maia_tab5_i2s_tx_acquire(&first_index, &first_samples) == MAIA_TAB5_I2S_OK);
    CHECK(maia_tab5_i2s_tx_stop() == MAIA_TAB5_I2S_OK);
    CHECK(maia_tab5_i2s_tx_acquire(&first_index, &first_samples) == MAIA_TAB5_I2S_OK);
    puts("tab5_i2s_tx_test: PASS");
    return 0;
}
