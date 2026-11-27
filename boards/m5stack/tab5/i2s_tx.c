#include <maia/arch/irq.h>
#include <maia/boards/tab5_i2s.h>
#include <maia/soc/esp32p4_regs.h>
#include <stddef.h>
#include <stdint.h>

#define DMA_UNCACHED_ALIAS_OFFSET UINT32_C(0x40000000)
#define DMA_DESC_SIZE_MASK UINT32_C(0x00000FFF)
#define DMA_DESC_LENGTH_SHIFT 12U
#define DMA_DESC_OWNER (UINT32_C(1) << 31U)
#define DMA_STORAGE_ALIGNMENT 64U
#define DMA_BUFFER_BYTES (MAIA_TAB5_I2S_TX_BUFFER_FRAMES * 2U * sizeof(int16_t))
#define DMA_MEMORY_START UINT32_C(0x4FF00000)
#define DMA_MEMORY_END UINT32_C(0x4FFC0000)
#define I2S_UPDATE_SPIN_LIMIT 10000U
#define DMA_STOP_SPIN_LIMIT 100000U

#define I2S_TX_CONF_FIELDS (MAIA_ESP32P4_I2S1_TX_START | MAIA_ESP32P4_I2S1_TX_SLAVE_MODE | \
                            MAIA_ESP32P4_I2S1_TX_STOP_ENABLE | MAIA_ESP32P4_I2S1_TX_PCM_BYPASS | \
                            MAIA_ESP32P4_I2S1_TX_MSB_SHIFT | MAIA_ESP32P4_I2S1_TX_TDM_ENABLE | \
                            MAIA_ESP32P4_I2S1_TX_PDM_ENABLE | MAIA_ESP32P4_I2S1_TX_BCK_DIV_MASK)
#define I2S_TX_CONF1_FIELDS (MAIA_ESP32P4_I2S1_TX_CONF1_WS_WIDTH_MASK | \
                             MAIA_ESP32P4_I2S1_TX_CONF1_BITS_MOD_MASK | \
                             MAIA_ESP32P4_I2S1_TX_CONF1_HALF_BITS_MASK | \
                             MAIA_ESP32P4_I2S1_TX_CONF1_TDM_BITS_MASK)

typedef struct {
    volatile uint32_t control;
    uint32_t buffer;
    uint32_t next;
} dma_descriptor_t;

typedef struct {
    dma_descriptor_t descriptors[MAIA_TAB5_I2S_TX_BUFFER_COUNT];
    int16_t samples[MAIA_TAB5_I2S_TX_BUFFER_COUNT][MAIA_TAB5_I2S_TX_BUFFER_FRAMES * 2U]
        __attribute__((aligned(DMA_STORAGE_ALIGNMENT)));
} dma_storage_t;

_Static_assert(sizeof(dma_descriptor_t) == 12U, "AHB-DMA descriptor size must be 12 bytes");
_Static_assert((DMA_BUFFER_BYTES & 3U) == 0U, "audio DMA buffers must be word-aligned");

static dma_storage_t s_dma_storage __attribute__((aligned(DMA_STORAGE_ALIGNMENT)));
static uint8_t s_buffer_state[MAIA_TAB5_I2S_TX_BUFFER_COUNT];
static unsigned s_initialized;
static unsigned s_dma_started;
static unsigned s_tx_started;
static unsigned s_stopping;

enum {
    BUFFER_FREE,
    BUFFER_LEASED,
    BUFFER_QUEUED
};

#ifdef MAIA_TAB5_I2S_TEST
uint32_t maia_tab5_i2s_test_read(uintptr_t address);
void maia_tab5_i2s_test_write(uintptr_t address, uint32_t value);
static uint32_t read32(uintptr_t address) { return maia_tab5_i2s_test_read(address); }
static void write32(uintptr_t address, uint32_t value) { maia_tab5_i2s_test_write(address, value); }
static void dma_memory_fence(void) { __atomic_thread_fence(__ATOMIC_SEQ_CST); }
static volatile dma_storage_t *dma_storage_cpu(void) { return &s_dma_storage; }
#else
static uint32_t read32(uintptr_t address) { return MAIA_MMIO32(address); }
static void write32(uintptr_t address, uint32_t value) {
    MAIA_MMIO32(address) = value;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}
static void dma_memory_fence(void) { __asm__ volatile("fence iorw, iorw" ::: "memory"); }
static volatile dma_storage_t *dma_storage_cpu(void) {
    return (volatile dma_storage_t *)((uintptr_t)&s_dma_storage + DMA_UNCACHED_ALIAS_OFFSET);
}
#endif

static void update_bits(uintptr_t address, uint32_t mask, uint32_t value) {
    uint32_t old_value = read32(address);
    write32(address, (old_value & ~mask) | (value & mask));
}

static uint32_t physical_address(const void *pointer) {
    return (uint32_t)(uintptr_t)pointer;
}

static volatile dma_descriptor_t *dma_descriptor_cpu(unsigned index) {
    return &dma_storage_cpu()->descriptors[index];
}

static void enable_bus_clock(void) {
    update_bits(MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL2_REG,
                MAIA_ESP32P4_HP_CLKRST_I2S1_APB_EN,
                MAIA_ESP32P4_HP_CLKRST_I2S1_APB_EN);
    update_bits(MAIA_ESP32P4_LP_CLKRST_HP_CLK_CTRL_REG,
                MAIA_ESP32P4_LP_CLKRST_I2S1_MCLK_EN,
                MAIA_ESP32P4_LP_CLKRST_I2S1_MCLK_EN);
}

static void reset_i2s_registers(void) {
    update_bits(MAIA_ESP32P4_HP_CLKRST_HP_RST_EN2_REG,
                MAIA_ESP32P4_HP_CLKRST_I2S1_APB_RESET,
                MAIA_ESP32P4_HP_CLKRST_I2S1_APB_RESET);
    update_bits(MAIA_ESP32P4_HP_CLKRST_HP_RST_EN2_REG,
                MAIA_ESP32P4_HP_CLKRST_I2S1_APB_RESET, 0U);
}

static void configure_clock(void) {
    const uintptr_t ctrl15 = MAIA_ESP32P4_HP_CLKRST_I2S1_CLK_CTRL15_REG;
    const uintptr_t ctrl16 = MAIA_ESP32P4_HP_CLKRST_I2S1_CLK_CTRL16_REG;
    const uintptr_t ctrl17 = MAIA_ESP32P4_HP_CLKRST_I2S1_CLK_CTRL17_REG;

    update_bits(ctrl15,
                MAIA_ESP32P4_HP_CLKRST_I2S1_TX_CLK_EN |
                    MAIA_ESP32P4_HP_CLKRST_I2S1_TX_CLK_SRC_MASK,
                MAIA_ESP32P4_HP_CLKRST_I2S1_TX_CLK_EN);
    update_bits(ctrl17, MAIA_ESP32P4_HP_CLKRST_I2S1_MST_CLK_SEL,
                MAIA_ESP32P4_HP_CLKRST_I2S1_MST_CLK_SEL);

    update_bits(ctrl16, MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_N_MASK, 2U);
    update_bits(ctrl17, MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_YN1, 0U);
    update_bits(ctrl16, MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Y_MASK,
                1UL << MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Y_SHIFT);
    update_bits(ctrl17, MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Z_MASK, 0U);
    update_bits(ctrl16, MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_X_MASK, 0U);
    update_bits(ctrl17,
                MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_YN1 |
                    MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Z_MASK,
                49U);
    update_bits(ctrl16,
                MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_N_MASK |
                    MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_X_MASK |
                    MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Y_MASK,
                3U |
                    (3UL << MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_X_SHIFT) |
                    (45UL << MAIA_ESP32P4_HP_CLKRST_I2S1_DIV_Y_SHIFT));
}

static void configure_i2s_pins(void) {
    static const uint32_t pins[] = {30U, 27U, 29U, 26U};
    static const uint32_t signals[] = {
        MAIA_ESP32P4_I2S1_MCLK_PAD_OUT_IDX,
        MAIA_ESP32P4_I2S1_O_BCK_PAD_OUT_IDX,
        MAIA_ESP32P4_I2S1_O_WS_PAD_OUT_IDX,
        MAIA_ESP32P4_I2S1_O_SD_PAD_OUT_IDX
    };

    for (unsigned i = 0U; i < sizeof pins / sizeof pins[0]; ++i) {
        uint32_t pin = pins[i];
        uint32_t bit = 1UL << (pin % 32U);
        uintptr_t enable_reg = pin < 32U ?
            MAIA_ESP32P4_GPIO_ENABLE_W1TS_REG : MAIA_ESP32P4_GPIO_ENABLE1_W1TS_REG;
        write32(MAIA_ESP32P4_IO_MUX_GPIO_REG(pin),
                MAIA_ESP32P4_IO_MUX_MCU_SEL_GPIO |
                    (2UL << MAIA_ESP32P4_IO_MUX_FUN_DRV_SHIFT));
        write32(MAIA_ESP32P4_GPIO_PIN_REG(pin), 0U);
        write32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(pin),
                signals[i] | MAIA_ESP32P4_GPIO_FUNC_OEN_SEL);
        write32(enable_reg, bit);
    }
    dma_memory_fence();
}

static int update_i2s_tx(void) {
    uint32_t conf = read32(MAIA_ESP32P4_I2S1_TX_CONF_REG);
    write32(MAIA_ESP32P4_I2S1_TX_CONF_REG, conf | MAIA_ESP32P4_I2S1_TX_UPDATE);
    for (unsigned i = 0U; i < I2S_UPDATE_SPIN_LIMIT; ++i) {
        if (!(read32(MAIA_ESP32P4_I2S1_TX_CONF_REG) & MAIA_ESP32P4_I2S1_TX_UPDATE)) return 1;
    }
    return 0;
}

static void configure_i2s_tx(void) {
    uintptr_t conf_reg = MAIA_ESP32P4_I2S1_TX_CONF_REG;
    uint32_t conf = read32(conf_reg);
    write32(conf_reg, conf | MAIA_ESP32P4_I2S1_TX_RESET);
    write32(conf_reg, read32(conf_reg) & ~MAIA_ESP32P4_I2S1_TX_RESET);
    write32(conf_reg, read32(conf_reg) | MAIA_ESP32P4_I2S1_TX_FIFO_RESET);
    write32(conf_reg, read32(conf_reg) & ~MAIA_ESP32P4_I2S1_TX_FIFO_RESET);

    uint32_t conf1 =
        15U |
        (15UL << MAIA_ESP32P4_I2S1_TX_CONF1_BITS_MOD_SHIFT) |
        (15UL << MAIA_ESP32P4_I2S1_TX_CONF1_HALF_BITS_SHIFT) |
        (15UL << MAIA_ESP32P4_I2S1_TX_CONF1_TDM_BITS_SHIFT);
    update_bits(MAIA_ESP32P4_I2S1_TX_CONF1_REG, I2S_TX_CONF1_FIELDS, conf1);
    update_bits(MAIA_ESP32P4_I2S1_TX_TDM_CTRL_REG,
                MAIA_ESP32P4_I2S1_TX_TDM_ACTIVE_MASK |
                    MAIA_ESP32P4_I2S1_TX_TDM_TOTAL_MASK |
                    MAIA_ESP32P4_I2S1_TX_TDM_SKIP_MASK_EN,
                3U | (1UL << MAIA_ESP32P4_I2S1_TX_TDM_TOTAL_SHIFT));
    update_bits(conf_reg, I2S_TX_CONF_FIELDS,
                MAIA_ESP32P4_I2S1_TX_STOP_ENABLE |
                    MAIA_ESP32P4_I2S1_TX_PCM_BYPASS |
                    MAIA_ESP32P4_I2S1_TX_MSB_SHIFT |
                    MAIA_ESP32P4_I2S1_TX_TDM_ENABLE |
                    (7UL << MAIA_ESP32P4_I2S1_TX_BCK_DIV_SHIFT));
}

static int dma_storage_is_valid(void) {
#ifdef MAIA_TAB5_I2S_TEST
    return 1;
#else
    uintptr_t start = (uintptr_t)&s_dma_storage;
    uintptr_t end = start + sizeof s_dma_storage;
    return !(start & (DMA_STORAGE_ALIGNMENT - 1U)) &&
           start >= DMA_MEMORY_START && end <= DMA_MEMORY_END;
#endif
}

static void configure_dma(void) {
    uintptr_t misc = MAIA_ESP32P4_AHB_DMA_MISC_CONF_REG;
    uint32_t misc_value = read32(misc) | MAIA_ESP32P4_AHB_DMA_MISC_CLK_EN;
    write32(misc, misc_value | MAIA_ESP32P4_AHB_DMA_MISC_AHBM_RST_INTER);
    write32(misc, misc_value & ~MAIA_ESP32P4_AHB_DMA_MISC_AHBM_RST_INTER);
    write32(MAIA_ESP32P4_AHB_DMA_INTR_MEM_START_ADDR_REG, DMA_MEMORY_START);
    write32(MAIA_ESP32P4_AHB_DMA_INTR_MEM_END_ADDR_REG, DMA_MEMORY_END);

    uintptr_t conf0 = MAIA_ESP32P4_AHB_DMA_OUT_CONF0_CH0_REG;
    write32(conf0, read32(conf0) | MAIA_ESP32P4_AHB_DMA_OUT_RST_CH0);
    write32(conf0, read32(conf0) & ~MAIA_ESP32P4_AHB_DMA_OUT_RST_CH0);
    update_bits(conf0, MAIA_ESP32P4_AHB_DMA_OUT_AUTO_WRBACK_CH0,
                MAIA_ESP32P4_AHB_DMA_OUT_AUTO_WRBACK_CH0);
    update_bits(MAIA_ESP32P4_AHB_DMA_OUT_CONF1_CH0_REG,
                MAIA_ESP32P4_AHB_DMA_OUT_CHECK_OWNER_CH0,
                MAIA_ESP32P4_AHB_DMA_OUT_CHECK_OWNER_CH0);
    update_bits(MAIA_ESP32P4_AHB_DMA_OUT_PERI_SEL_CH0_REG,
                MAIA_ESP32P4_AHB_DMA_PERI_OUT_SEL_CH0_MASK,
                MAIA_ESP32P4_AHB_DMA_PERI_OUT_SEL_I2S1);

    volatile dma_storage_t *storage = dma_storage_cpu();
    for (unsigned i = 0U; i < MAIA_TAB5_I2S_TX_BUFFER_COUNT; ++i) {
        unsigned next = (i + 1U) % MAIA_TAB5_I2S_TX_BUFFER_COUNT;
        volatile dma_descriptor_t *descriptor = &storage->descriptors[i];
        descriptor->control = DMA_BUFFER_BYTES;
        descriptor->buffer = physical_address(&s_dma_storage.samples[i][0]);
        descriptor->next = physical_address(&s_dma_storage.descriptors[next]);
        s_buffer_state[i] = BUFFER_FREE;
    }
    dma_memory_fence();
}

maia_tab5_i2s_result_t maia_tab5_i2s_tx_init(void) {
    uintptr_t irq = maia_arch_irq_save();
    if (s_initialized) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_OK;
    }
    if (!dma_storage_is_valid()) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_DMA_MEMORY_ERROR;
    }

    enable_bus_clock();
    reset_i2s_registers();
    configure_clock();
    configure_i2s_tx();
    if (!update_i2s_tx()) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_TIMEOUT;
    }
    configure_dma();
    configure_i2s_pins();
    s_dma_started = 0U;
    s_tx_started = 0U;
    s_stopping = 0U;
    s_initialized = 1U;
    maia_arch_irq_restore(irq);
    return MAIA_TAB5_I2S_OK;
}

maia_tab5_i2s_result_t maia_tab5_i2s_tx_acquire(unsigned *buffer_index, int16_t **samples) {
    if (!buffer_index || !samples) return MAIA_TAB5_I2S_INVALID;
    uintptr_t irq = maia_arch_irq_save();
    if (!s_initialized) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_NOT_INITIALIZED;
    }
    if (s_stopping) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_BUSY;
    }

    for (unsigned i = 0U; i < MAIA_TAB5_I2S_TX_BUFFER_COUNT; ++i) {
        volatile dma_descriptor_t *descriptor = dma_descriptor_cpu(i);
        if (s_buffer_state[i] == BUFFER_QUEUED && !(descriptor->control & DMA_DESC_OWNER)) {
            s_buffer_state[i] = BUFFER_FREE;
        }
        if (s_buffer_state[i] == BUFFER_FREE) {
            s_buffer_state[i] = BUFFER_LEASED;
            *buffer_index = i;
            *samples = (int16_t *)(uintptr_t)&dma_storage_cpu()->samples[i][0];
            maia_arch_irq_restore(irq);
            return MAIA_TAB5_I2S_OK;
        }
    }
    maia_arch_irq_restore(irq);
    return MAIA_TAB5_I2S_BUSY;
}

maia_tab5_i2s_result_t maia_tab5_i2s_tx_submit(unsigned buffer_index, uint32_t frames) {
    if (buffer_index >= MAIA_TAB5_I2S_TX_BUFFER_COUNT ||
        frames == 0U || frames > MAIA_TAB5_I2S_TX_BUFFER_FRAMES) {
        return MAIA_TAB5_I2S_INVALID;
    }
    uintptr_t irq = maia_arch_irq_save();
    if (!s_initialized) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_NOT_INITIALIZED;
    }
    if (s_stopping) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_BUSY;
    }
    if (s_buffer_state[buffer_index] != BUFFER_LEASED) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_INVALID;
    }

    if (!s_tx_started && !update_i2s_tx()) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_TIMEOUT;
    }
    uint32_t bytes = frames * 2U * sizeof(int16_t);
    volatile dma_descriptor_t *descriptor = dma_descriptor_cpu(buffer_index);
    uint32_t control = DMA_BUFFER_BYTES |
        ((bytes & DMA_DESC_SIZE_MASK) << DMA_DESC_LENGTH_SHIFT) |
        DMA_DESC_OWNER;
    descriptor->control = control & ~DMA_DESC_OWNER;
    dma_memory_fence();
    descriptor->control = control;
    s_buffer_state[buffer_index] = BUFFER_QUEUED;

    if (!s_dma_started) {
        write32(MAIA_ESP32P4_AHB_DMA_OUT_LINK_ADDR_CH0_REG,
                physical_address(&s_dma_storage.descriptors[buffer_index]));
        write32(MAIA_ESP32P4_AHB_DMA_OUT_LINK_CH0_REG,
                MAIA_ESP32P4_AHB_DMA_OUTLINK_START_CH0);
        s_dma_started = 1U;
    } else if (read32(MAIA_ESP32P4_AHB_DMA_OUT_LINK_CH0_REG) &
               MAIA_ESP32P4_AHB_DMA_OUTLINK_PARK_CH0) {
        write32(MAIA_ESP32P4_AHB_DMA_OUT_LINK_CH0_REG,
                MAIA_ESP32P4_AHB_DMA_OUTLINK_RESTART_CH0);
    }
    if (!s_tx_started) {
        write32(MAIA_ESP32P4_I2S1_TX_CONF_REG,
                read32(MAIA_ESP32P4_I2S1_TX_CONF_REG) |
                    MAIA_ESP32P4_I2S1_TX_START);
        s_tx_started = 1U;
    }
    maia_arch_irq_restore(irq);
    return MAIA_TAB5_I2S_OK;
}

maia_tab5_i2s_result_t maia_tab5_i2s_tx_stop(void) {
    uintptr_t irq = maia_arch_irq_save();
    if (!s_initialized) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_NOT_INITIALIZED;
    }
    if (!s_dma_started) {
        for (unsigned i = 0U; i < MAIA_TAB5_I2S_TX_BUFFER_COUNT; ++i) {
            dma_descriptor_cpu(i)->control = DMA_BUFFER_BYTES;
            s_buffer_state[i] = BUFFER_FREE;
        }
        dma_memory_fence();
        s_tx_started = 0U;
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_OK;
    }
    if (s_stopping) {
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_BUSY;
    }

    s_stopping = 1U;
    write32(MAIA_ESP32P4_AHB_DMA_OUT_LINK_CH0_REG,
            MAIA_ESP32P4_AHB_DMA_OUTLINK_STOP_CH0);
    maia_arch_irq_restore(irq);

    unsigned stopped = 0U;
    for (unsigned i = 0U; i < DMA_STOP_SPIN_LIMIT; ++i) {
        if (read32(MAIA_ESP32P4_AHB_DMA_OUT_LINK_CH0_REG) &
            MAIA_ESP32P4_AHB_DMA_OUTLINK_PARK_CH0) {
            stopped = 1U;
            break;
        }
    }
    if (!stopped) {
        irq = maia_arch_irq_save();
        s_stopping = 0U;
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_TIMEOUT;
    }

    uint32_t conf = read32(MAIA_ESP32P4_I2S1_TX_CONF_REG);
    write32(MAIA_ESP32P4_I2S1_TX_CONF_REG,
            conf & ~MAIA_ESP32P4_I2S1_TX_START);
    if (!update_i2s_tx()) {
        irq = maia_arch_irq_save();
        s_stopping = 0U;
        maia_arch_irq_restore(irq);
        return MAIA_TAB5_I2S_TIMEOUT;
    }

    irq = maia_arch_irq_save();
    for (unsigned i = 0U; i < MAIA_TAB5_I2S_TX_BUFFER_COUNT; ++i) {
        dma_descriptor_cpu(i)->control = DMA_BUFFER_BYTES;
        s_buffer_state[i] = BUFFER_FREE;
    }
    dma_memory_fence();
    s_dma_started = 0U;
    s_tx_started = 0U;
    s_stopping = 0U;
    maia_arch_irq_restore(irq);
    return MAIA_TAB5_I2S_OK;
}

#ifdef MAIA_TAB5_I2S_TEST
void maia_tab5_i2s_test_reset(void) {
    s_initialized = 0U;
    s_dma_started = 0U;
    s_tx_started = 0U;
    s_stopping = 0U;
    for (unsigned i = 0U; i < MAIA_TAB5_I2S_TX_BUFFER_COUNT; ++i) {
        s_buffer_state[i] = BUFFER_FREE;
        s_dma_storage.descriptors[i].control = 0U;
        s_dma_storage.descriptors[i].buffer = 0U;
        s_dma_storage.descriptors[i].next = 0U;
    }
}

uint32_t *maia_tab5_i2s_test_descriptor(unsigned index) {
    return index < MAIA_TAB5_I2S_TX_BUFFER_COUNT ?
        (uint32_t *)&s_dma_storage.descriptors[index] : NULL;
}
#endif
