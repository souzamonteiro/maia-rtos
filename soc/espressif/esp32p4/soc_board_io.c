#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_regs.h>

#define TAB5_SYS_SDA_PIN 31U
#define TAB5_SYS_SCL_PIN 32U
#define TAB5_EXT_SDA_PIN 0U
#define TAB5_EXT_SCL_PIN 1U
#define TAB5_LCD_BL_PIN  22U
#define TAB5_TP_INT_PIN  23U
#define TAB5_KBD_INT_PIN 50U

#define PI4IOE1_ADDR     0x43U
#define PI4IOE2_ADDR     0x44U
#define TCA8418_ADDR     0x34U
#define STM32_KBD_ADDR   0x6DU

static uint32_t g_detected_panel = MAIA_TAB5_PANEL_UNKNOWN;
static uint8_t  g_kbd_type       = 0U; /* 0=none, 1=TCA8418(0x34), 2=STM32(0x6D) */
static uint8_t  g_kbd_bus_sda    = TAB5_EXT_SDA_PIN;
static uint8_t  g_kbd_bus_scl    = TAB5_EXT_SCL_PIN;
static uint8_t  g_shift_down     = 0U;
static uint8_t  g_caps_lock      = 0U;
static uint8_t  g_ctrl_down      = 0U;

static uint8_t  s_stm32_kbuf[32];
static uint8_t  s_stm32_khead    = 0U;
static uint8_t  s_stm32_ktail    = 0U;

void maia_udelay(uint32_t us) {
    uint32_t start = 0U;
    __asm__ volatile("csrr %0, mcycle" : "=r"(start));
    uint32_t cycles = us * 400U;
    for (;;) {
        uint32_t now = 0U;
        __asm__ volatile("csrr %0, mcycle" : "=r"(now));
        if ((now - start) >= cycles) {
            break;
        }
    }
}

static void gpio_route_to_hp_mux(uint32_t pin, int pullup, int input_en) {
    if (pin < 16U) {
        uint32_t lp = MAIA_MMIO32(MAIA_ESP32P4_LP_IOMUX_PAD_REG(pin));
        lp &= ~(MAIA_ESP32P4_LP_IOMUX_PAD_MUX_SEL_LP | MAIA_ESP32P4_LP_IOMUX_PAD_RDE);
        if (pullup) {
            lp |= MAIA_ESP32P4_LP_IOMUX_PAD_RUE;
        } else {
            lp &= ~MAIA_ESP32P4_LP_IOMUX_PAD_RUE;
        }
        if (input_en) {
            lp |= MAIA_ESP32P4_LP_IOMUX_PAD_FUN_IE;
        }
        MAIA_MMIO32(MAIA_ESP32P4_LP_IOMUX_PAD_REG(pin)) = lp;
    }
    uint32_t iomux = MAIA_ESP32P4_IO_MUX_MCU_SEL_GPIO | (2UL << MAIA_ESP32P4_IO_MUX_FUN_DRV_SHIFT);
    if (pullup) {
        iomux |= MAIA_ESP32P4_IO_MUX_FUN_WPU;
    }
    if (input_en) {
        iomux |= MAIA_ESP32P4_IO_MUX_FUN_IE;
    }
    MAIA_MMIO32(MAIA_ESP32P4_IO_MUX_GPIO_REG(pin)) = iomux;
    MAIA_MMIO32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(pin)) = MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX;
}

static void gpio_set_level(uint32_t pin, uint32_t high) {
    if (pin < 32U) {
        if (high) {
            MAIA_MMIO32(MAIA_ESP32P4_GPIO_OUT_W1TS_REG) = (1UL << pin);
        } else {
            MAIA_MMIO32(MAIA_ESP32P4_GPIO_OUT_W1TC_REG) = (1UL << pin);
        }
    } else {
        uint32_t bit = 1UL << (pin - 32U);
        if (high) {
            MAIA_MMIO32(MAIA_ESP32P4_GPIO_OUT1_W1TS_REG) = bit;
        } else {
            MAIA_MMIO32(MAIA_ESP32P4_GPIO_OUT1_W1TC_REG) = bit;
        }
    }
}

static void gpio_set_output_enable(uint32_t pin, uint32_t enable) {
    if (pin < 32U) {
        if (enable) {
            MAIA_MMIO32(MAIA_ESP32P4_GPIO_ENABLE_W1TS_REG) = (1UL << pin);
        } else {
            MAIA_MMIO32(MAIA_ESP32P4_GPIO_ENABLE_W1TC_REG) = (1UL << pin);
        }
    } else {
        uint32_t bit = 1UL << (pin - 32U);
        if (enable) {
            MAIA_MMIO32(MAIA_ESP32P4_GPIO_ENABLE1_W1TS_REG) = bit;
        } else {
            MAIA_MMIO32(MAIA_ESP32P4_GPIO_ENABLE1_W1TC_REG) = bit;
        }
    }
}

static uint32_t gpio_get_level(uint32_t pin) {
    if (pin < 32U) {
        return (MAIA_MMIO32(MAIA_ESP32P4_GPIO_IN_REG) >> pin) & 1UL;
    }
    return (MAIA_MMIO32(MAIA_ESP32P4_GPIO_IN1_REG) >> (pin - 32U)) & 1UL;
}

static void gpio_config_push_pull_output(uint32_t pin, uint32_t initial_level) __attribute__((unused));
static void gpio_config_push_pull_output(uint32_t pin, uint32_t initial_level) {
    gpio_route_to_hp_mux(pin, 0, 0);
    MAIA_MMIO32(MAIA_ESP32P4_GPIO_PIN_REG(pin)) = 0UL;
    gpio_set_level(pin, initial_level);
    gpio_set_output_enable(pin, 1U);
}

static void gpio_config_open_drain_io(uint32_t pin) {
    gpio_route_to_hp_mux(pin, 1, 1);
    MAIA_MMIO32(MAIA_ESP32P4_GPIO_PIN_REG(pin)) = MAIA_ESP32P4_GPIO_PIN_PAD_DRIVER_OD;
    gpio_set_level(pin, 1U);
    gpio_set_output_enable(pin, 1U);
}

static void i2c_delay(void) {
    maia_udelay(5U);
}

static void i2c_sda_hi(uint32_t sda) {
    gpio_set_level(sda, 1U);
}

static void i2c_sda_lo(uint32_t sda) {
    gpio_set_level(sda, 0U);
}

static void i2c_scl_hi(uint32_t scl) {
    gpio_set_level(scl, 1U);
    for (uint32_t t = 0; t < 200U; ++t) {
        if (gpio_get_level(scl) != 0U) {
            break;
        }
        maia_udelay(1U);
    }
}

static void i2c_scl_lo(uint32_t scl) {
    gpio_set_level(scl, 0U);
}

static void i2c_start(uint32_t sda, uint32_t scl) {
    i2c_sda_hi(sda);
    i2c_scl_hi(scl);
    i2c_delay();
    i2c_sda_lo(sda);
    i2c_delay();
    i2c_scl_lo(scl);
    i2c_delay();
}

static void i2c_stop(uint32_t sda, uint32_t scl) {
    i2c_sda_lo(sda);
    i2c_delay();
    i2c_scl_hi(scl);
    i2c_delay();
    i2c_sda_hi(sda);
    i2c_delay();
}

static int i2c_write_byte(uint32_t sda, uint32_t scl, uint8_t b) {
    for (int i = 7; i >= 0; --i) {
        if ((b >> i) & 1U) {
            i2c_sda_hi(sda);
        } else {
            i2c_sda_lo(sda);
        }
        i2c_delay();
        i2c_scl_hi(scl);
        i2c_delay();
        i2c_scl_lo(scl);
    }
    i2c_sda_hi(sda);
    i2c_delay();
    i2c_scl_hi(scl);
    i2c_delay();
    int ack = (gpio_get_level(sda) == 0U) ? 1 : 0;
    i2c_scl_lo(scl);
    i2c_delay();
    return ack;
}

static uint8_t i2c_read_byte(uint32_t sda, uint32_t scl, int send_ack) {
    uint8_t val = 0U;
    i2c_sda_hi(sda);
    for (int i = 7; i >= 0; --i) {
        i2c_delay();
        i2c_scl_hi(scl);
        i2c_delay();
        if (gpio_get_level(sda) != 0U) {
            val |= (uint8_t)(1U << i);
        }
        i2c_scl_lo(scl);
    }
    if (send_ack) {
        i2c_sda_lo(sda);
    } else {
        i2c_sda_hi(sda);
    }
    i2c_delay();
    i2c_scl_hi(scl);
    i2c_delay();
    i2c_scl_lo(scl);
    i2c_sda_hi(sda);
    i2c_delay();
    return val;
}

int maia_i2c_write_bytes(uint32_t sda_pin, uint32_t scl_pin, uint8_t addr7, const uint8_t *data, size_t len) {
    i2c_start(sda_pin, scl_pin);
    if (!i2c_write_byte(sda_pin, scl_pin, (uint8_t)(addr7 << 1U))) {
        i2c_stop(sda_pin, scl_pin);
        return -1;
    }
    for (size_t i = 0; i < len; ++i) {
        if (!i2c_write_byte(sda_pin, scl_pin, data[i])) {
            i2c_stop(sda_pin, scl_pin);
            return -1;
        }
    }
    i2c_stop(sda_pin, scl_pin);
    return 0;
}

int maia_i2c_write_read(uint32_t sda_pin, uint32_t scl_pin, uint8_t addr7,
                        const uint8_t *tx, size_t tx_len, uint8_t *rx, size_t rx_len) {
    if (tx_len > 0U) {
        i2c_start(sda_pin, scl_pin);
        if (!i2c_write_byte(sda_pin, scl_pin, (uint8_t)(addr7 << 1U))) {
            i2c_stop(sda_pin, scl_pin);
            return -1;
        }
        for (size_t i = 0; i < tx_len; ++i) {
            if (!i2c_write_byte(sda_pin, scl_pin, tx[i])) {
                i2c_stop(sda_pin, scl_pin);
                return -1;
            }
        }
    }
    if (rx_len > 0U) {
        i2c_start(sda_pin, scl_pin);
        if (!i2c_write_byte(sda_pin, scl_pin, (uint8_t)((addr7 << 1U) | 1U))) {
            i2c_stop(sda_pin, scl_pin);
            return -1;
        }
        for (size_t i = 0; i < rx_len; ++i) {
            rx[i] = i2c_read_byte(sda_pin, scl_pin, (i + 1U < rx_len) ? 1 : 0);
        }
    }
    i2c_stop(sda_pin, scl_pin);
    return 0;
}

static int i2c_probe(uint32_t sda_pin, uint32_t scl_pin, uint8_t addr7) {
    i2c_start(sda_pin, scl_pin);
    int ack = i2c_write_byte(sda_pin, scl_pin, (uint8_t)(addr7 << 1U));
    i2c_stop(sda_pin, scl_pin);
    return ack;
}

static void print_hex8(uint8_t v) {
    static const char kHex[] = "0123456789ABCDEF";
    char s[5];
    s[0] = '0';
    s[1] = 'x';
    s[2] = kHex[(v >> 4U) & 0xFU];
    s[3] = kHex[v & 0xFU];
    s[4] = '\0';
    maia_console_puts(s);
}

static void pi4ioe_write_reg(uint8_t addr, uint8_t reg, uint8_t val) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    (void)maia_i2c_write_bytes(TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN, addr, buf, 2U);
}

static uint8_t pi4ioe_read_reg(uint8_t addr, uint8_t reg) {
    uint8_t val = 0U;
    (void)maia_i2c_write_read(TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN, addr, &reg, 1U, &val, 1U);
    return val;
}

static void tca8418_write_reg(uint8_t reg, uint8_t val) {
    uint8_t buf[2];
    buf[0] = reg;
    buf[1] = val;
    (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, TCA8418_ADDR, buf, 2U);
}

static uint8_t tca8418_read_reg(uint8_t reg) {
    uint8_t val = 0U;
    (void)maia_i2c_write_read(g_kbd_bus_sda, g_kbd_bus_scl, TCA8418_ADDR, &reg, 1U, &val, 1U);
    return val;
}

static void tca8418_init_scanner(void) {
    /* Configure 8x9 matrix + GPI as in M5Tab5-UserDemo keypad_scanner_tca8418.c */
    tca8418_write_reg(0x23U, 0x00U); /* GPIO_DIR_1 */
    tca8418_write_reg(0x24U, 0x00U); /* GPIO_DIR_2 */
    tca8418_write_reg(0x25U, 0x00U); /* GPIO_DIR_3 */
    tca8418_write_reg(0x20U, 0xFFU); /* GPI_EM_1 */
    tca8418_write_reg(0x21U, 0xFFU); /* GPI_EM_2 */
    tca8418_write_reg(0x22U, 0xFFU); /* GPI_EM_3 */
    tca8418_write_reg(0x26U, 0x00U); /* GPIO_INT_LVL_1 */
    tca8418_write_reg(0x27U, 0x00U); /* GPIO_INT_LVL_2 */
    tca8418_write_reg(0x28U, 0x00U); /* GPIO_INT_LVL_3 */
    tca8418_write_reg(0x1AU, 0xFFU); /* GPIO_INT_EN_1 */
    tca8418_write_reg(0x1BU, 0xFFU); /* GPIO_INT_EN_2 */
    tca8418_write_reg(0x1CU, 0xFFU); /* GPIO_INT_EN_3 */

    /* 8 rows, 9 cols */
    tca8418_write_reg(0x1DU, 0xFFU); /* KP_GPIO_1: 8 rows */
    tca8418_write_reg(0x1EU, 0xFFU); /* KP_GPIO_2: cols 0..7 */
    tca8418_write_reg(0x1FU, 0x01U); /* KP_GPIO_3: col 8 */

    /* Flush FIFO */
    for (int i = 0; i < 16; ++i) {
        if (tca8418_read_reg(0x04U) == 0U) {
            break;
        }
    }
    (void)tca8418_read_reg(0x11U);
    (void)tca8418_read_reg(0x12U);
    (void)tca8418_read_reg(0x13U);
    tca8418_write_reg(0x02U, 0x03U); /* Clear INT_STAT */
    tca8418_write_reg(0x01U, 0x03U); /* Enable KE_IEN | GPI_IEN */
}

/* Keycode 1..81 mapping from M5Tab5-UserDemo keypad_scanner_tca8418.c */
static const char kTab5TcaKeyMap[81] = {
    '0', '8',  ':', 'c', 'k', 's',  0,   0,   ' ', 0,
    '1', '9',  ']', 'd', 'l', 't',  0,   0,   0,   0,
    '2', '-',  ',', 'e', 'm', 'u',  0,   27,  0,   0,
    '3', '^',  '.', 'f', 'n', 'v',  0,   '\t',0,   0,
    '4', '$',  '/', 'g', 'o', 'w',  0,   0,   0,   0,
    '5', '@',  '_', 'h', 'p', 'x',  0,   '\b',0,   0,
    '6', '[',  'a', 'i', 'q', 'y',  0,   0,   0,   0,
    '7', ';',  'b', 'j', 'r', 'z',  0,   '\n',0,   0,
    0
};

static int str_is_key(const char *str, size_t slen, const char *key) {
    size_t klen = 0;
    while (key[klen] != '\0') {
        klen++;
    }
    if (slen < klen) {
        return 0;
    }
    for (size_t i = 0; i < klen; ++i) {
        char a = str[i];
        char b = key[i];
        if (a >= 'A' && a <= 'Z') {
            a = (char)(a - 'A' + 'a');
        }
        if (b >= 'A' && b <= 'Z') {
            b = (char)(b - 'A' + 'a');
        }
        if (a != b) {
            return 0;
        }
    }
    if (slen == klen || str[klen] == '\0' || str[klen] == '\r' || str[klen] == '\n' || str[klen] == ' ') {
        return 1;
    }
    return 0;
}

int maia_tab5_keyboard_getc_nonblocking(void) {
    if (g_kbd_type == 1U) {
        uint8_t int_stat = tca8418_read_reg(0x02U);
        if ((int_stat & 0x02U) != 0U) {
            (void)tca8418_read_reg(0x11U);
            (void)tca8418_read_reg(0x12U);
            (void)tca8418_read_reg(0x13U);
            tca8418_write_reg(0x02U, 0x02U);
        }
        uint8_t ec = tca8418_read_reg(0x03U) & 0x0FU;
        if (ec == 0U && (int_stat & 0x01U) == 0U) {
            return -1;
        }
        uint8_t ev = tca8418_read_reg(0x04U);
        tca8418_write_reg(0x02U, 0x01U);
        if (ev == 0U) {
            return -1;
        }
        int pressed = (ev & 0x80U) ? 1 : 0;
        uint8_t code = (uint8_t)(ev & 0x7FU);
        if (code == 7U) { /* SHIFT */
            g_shift_down = (uint8_t)pressed;
            return -1;
        }
        if (code == 17U) { /* CTRL */
            g_ctrl_down = (uint8_t)pressed;
            return -1;
        }
        if (code == 37U && pressed) { /* CAPS */
            g_caps_lock ^= 1U;
            return -1;
        }
        if (!pressed || code == 0U || code > 81U) {
            return -1;
        }
        char ch = kTab5TcaKeyMap[code - 1U];
        if (ch == 0) {
            return -1;
        }
        if (ch >= 'a' && ch <= 'z') {
            if (g_ctrl_down && ch == 'c') {
                return 3;
            }
            if (g_shift_down ^ g_caps_lock) {
                ch = (char)(ch - 'a' + 'A');
            }
        } else if (g_shift_down) {
            if (ch == '1') ch = '!';
            else if (ch == '2') ch = '"';
            else if (ch == '3') ch = '#';
            else if (ch == '4') ch = '$';
            else if (ch == '5') ch = '%';
            else if (ch == '6') ch = '&';
            else if (ch == '7') ch = '\'';
            else if (ch == '8') ch = '(';
            else if (ch == '9') ch = ')';
            else if (ch == '0') ch = '=';
            else if (ch == ';') ch = '+';
            else if (ch == ':') ch = '*';
            else if (ch == '-') ch = '=';
            else if (ch == ',') ch = '<';
            else if (ch == '.') ch = '>';
            else if (ch == '/') ch = '?';
        }
        return (int)(uint8_t)ch;
    } else if (g_kbd_type == 2U) {
        /* If we have buffered characters from a previous chunk, return next char */
        if (s_stm32_khead != s_stm32_ktail) {
            uint8_t c = s_stm32_kbuf[s_stm32_ktail];
            s_stm32_ktail = (uint8_t)((s_stm32_ktail + 1U) % sizeof(s_stm32_kbuf));
            return (int)c;
        }

        /* Check character queue in A164 STM32 Keyboard (String Mode) */
        uint8_t reg_len = 0x40U;
        uint8_t qlen = 0U;

        /* Reg 0x40 returns string length L (1..9). In Character Mode, 0x50 has L+1 bytes (mod + chars) */
        if (maia_i2c_write_read(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, &reg_len, 1U, &qlen, 1U) == 0 &&
            qlen >= 1U && qlen <= 16U) {
            uint8_t reg_fifo = 0x50U;
            uint8_t buf[20] = {0};
            /* Read exactly qlen + 1 bytes (byte 0: modifier, bytes 1..qlen: characters) */
            if (maia_i2c_write_read(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, &reg_fifo, 1U, buf, (size_t)(qlen + 1U)) == 0) {
                /* Clear STM32 INT_STA (Reg 0x01) to acknowledge the event */
                uint8_t clr_cmd[2] = {0x01U, 0x00U};
                (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, clr_cmd, 2U);

                uint8_t mod = buf[0];
                char str[17];
                size_t slen = (size_t)qlen;
                for (size_t i = 0; i < slen; ++i) {
                    str[i] = (char)buf[1U + i];
                }
                str[slen] = '\0';

                /* Translate named keys emitted by Tab5 Keyboard MCU in String Mode */
                if (str_is_key(str, slen, "enter") || str_is_key(str, slen, "return")) {
                    str[0] = '\n';
                    slen = 1U;
                } else if (str_is_key(str, slen, "backspace")) {
                    str[0] = '\b';
                    slen = 1U;
                } else if (str_is_key(str, slen, "space")) {
                    str[0] = ' ';
                    slen = 1U;
                } else if (str_is_key(str, slen, "tab")) {
                    str[0] = '\t';
                    slen = 1U;
                } else if (str_is_key(str, slen, "del") || str_is_key(str, slen, "delete")) {
                    str[0] = 0x7FU; /* DEL */
                    slen = 1U;
                } else if (str_is_key(str, slen, "esc") || str_is_key(str, slen, "escape")) {
                    str[0] = 27; /* ESC */
                    slen = 1U;
                }

                for (size_t i = 0; i < slen; ++i) {
                    uint8_t raw = (uint8_t)str[i];
                    if (raw == 0U) {
                        continue;
                    }
                    uint8_t next_head = (uint8_t)((s_stm32_khead + 1U) % sizeof(s_stm32_kbuf));
                    if (next_head == s_stm32_ktail) {
                        break; /* Buffer full */
                    }
                    if ((mod & 1U) != 0U && (raw == 'c' || raw == 'C')) {
                        raw = 3U; /* Ctrl+C */
                    } else if (raw == '\r') {
                        raw = (uint8_t)'\n';
                    }
                    s_stm32_kbuf[s_stm32_khead] = raw;
                    s_stm32_khead = next_head;
                }

                if (s_stm32_khead != s_stm32_ktail) {
                    uint8_t c = s_stm32_kbuf[s_stm32_ktail];
                    s_stm32_ktail = (uint8_t)((s_stm32_ktail + 1U) % sizeof(s_stm32_kbuf));
                    return (int)c;
                }
            }
        }
        return -1;
    }
    return -1;
}

uint32_t maia_tab5_get_detected_panel(void) {
    return g_detected_panel;
}

static void scan_and_report_i2c_bus(const char *name, uint32_t sda_pin, uint32_t scl_pin) {
    maia_console_puts("[I2C] Scanning ");
    maia_console_puts(name);
    maia_console_puts(": ");
    int found = 0;
    for (uint8_t addr = 0x08U; addr < 0x78U; ++addr) {
        if (i2c_probe(sda_pin, scl_pin, addr)) {
            if (found > 0) {
                maia_console_putc(' ');
            }
            print_hex8(addr);
            found++;
        }
    }
    if (found == 0) {
        maia_console_puts("(none)");
    }
    maia_console_puts("\n");
}

static void ledc_init_backlight_pwm(void) {
    /* 1. Enable LEDC APB bus clock in HP_SYS_CLKRST */
    MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_SOC_CLK_CTRL3_REG) |= (1UL << 0U);
    /* 2. De-assert LEDC reset in HP_SYS_CLKRST */
    MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_HP_RST_EN1_REG) &= ~(1UL << 29U);
    /* 3. Configure LEDC functional clock: source = XTAL (bits 1:0 = 0), enable = 1 (bit 2) */
    MAIA_MMIO32(MAIA_ESP32P4_HP_CLKRST_PERI_CLK_CTRL22_REG) = (1UL << 2U);
    /* 4. Global LEDC configuration: force clock gate open (bit 31) and select XTAL */
    MAIA_MMIO32(MAIA_ESP32P4_LEDC_CONF_REG) = (1UL << 31U) | 2UL;
    /* 5. Timer 0: 5000 Hz PWM with 10-bit resolution (precision 1024)
     * Divisor for 40 MHz XTAL to 5 kHz at 10-bit: (40000000 << 8) / (5000 * 1024) = 2000 (0x07D0)
     * duty_resolution = 10, clk_div = 0x07D0 << 5, para_up = 1 << 26 */
    MAIA_MMIO32(MAIA_ESP32P4_LEDC_TIMER0_CONF_REG) = 10UL | (0x07D0UL << 5U) | (1UL << 26U);

    /* 6. Channel 1: 100% duty (1023 << 4), output enable, duty start */
    MAIA_MMIO32(MAIA_ESP32P4_LEDC_CH1_CONF0_REG) = (1UL << 2U) | (1UL << 4U);
    MAIA_MMIO32(MAIA_ESP32P4_LEDC_CH1_HPOINT_REG) = 0U;
    MAIA_MMIO32(MAIA_ESP32P4_LEDC_CH1_DUTY_REG) = (1023UL << 4U);
    MAIA_MMIO32(MAIA_ESP32P4_LEDC_CH1_CONF1_REG) = (1UL << 31U);

    /* Start at 100% brightness (DC HIGH) */
    gpio_config_push_pull_output(TAB5_LCD_BL_PIN, 1U);
    gpio_set_level(TAB5_LCD_BL_PIN, 1U);
    MAIA_MMIO32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(TAB5_LCD_BL_PIN)) = MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX;
}

void maia_tab5_backlight_set_duty(uint32_t duty_4095) {
    if (duty_4095 > 4095U) {
        duty_4095 = 4095U;
    }
    if (duty_4095 >= 4095U) {
        /* Direct pure DC HIGH for 100% brightness */
        gpio_config_push_pull_output(TAB5_LCD_BL_PIN, 1U);
        gpio_set_level(TAB5_LCD_BL_PIN, 1U);
        MAIA_MMIO32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(TAB5_LCD_BL_PIN)) = MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX;
    } else if (duty_4095 == 0U) {
        /* Direct pure DC LOW for 0% brightness */
        gpio_config_push_pull_output(TAB5_LCD_BL_PIN, 0U);
        gpio_set_level(TAB5_LCD_BL_PIN, 0U);
        MAIA_MMIO32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(TAB5_LCD_BL_PIN)) = MAIA_ESP32P4_GPIO_SIG_GPIO_OUT_IDX;
    } else {
        /* Convert 0..4095 scale to 10-bit duty (0..1023) */
        uint32_t duty_1023 = (duty_4095 * 1023U) / 4095U;
        gpio_config_push_pull_output(TAB5_LCD_BL_PIN, 1U);
        MAIA_MMIO32(MAIA_ESP32P4_GPIO_FUNC_OUT_SEL_REG(TAB5_LCD_BL_PIN)) = 127U; /* LEDC_LS_SIG_OUT_PAD_OUT1_IDX */

        MAIA_MMIO32(MAIA_ESP32P4_LEDC_CH1_DUTY_REG) = (duty_1023 << 4U);
        MAIA_MMIO32(MAIA_ESP32P4_LEDC_CH1_CONF1_REG) = (1UL << 31U);
        MAIA_MMIO32(MAIA_ESP32P4_LEDC_CH1_CONF0_REG) = (1UL << 2U) | (1UL << 4U);
    }
}

void maia_tab5_board_io_init(void) {
    /* 1. Configure System I2C (SDA=GPIO31, SCL=GPIO32) and Ext.Port1 I2C (SDA=GPIO0, SCL=GPIO1) */
    gpio_config_open_drain_io(TAB5_SYS_SDA_PIN);
    gpio_config_open_drain_io(TAB5_SYS_SCL_PIN);
    gpio_config_open_drain_io(TAB5_EXT_SDA_PIN);
    gpio_config_open_drain_io(TAB5_EXT_SCL_PIN);
    gpio_route_to_hp_mux(TAB5_KBD_INT_PIN, 1, 1);
    gpio_route_to_hp_mux(TAB5_TP_INT_PIN, 1, 1);

    /* 2. Turn on LCD Backlight PWM (5000 Hz, 12-bit) on GPIO22 via LEDC Channel 1 */
    ledc_init_backlight_pwm();

    /* 3. Initialize PI4IOE1 (0x43) and PI4IOE2 (0x44) per official M5Stack Tab5 BSP */
    pi4ioe_write_reg(PI4IOE1_ADDR, 0x01U, 0xFFU); /* CHIP_RESET */
    (void)pi4ioe_read_reg(PI4IOE1_ADDR, 0x01U);
    pi4ioe_write_reg(PI4IOE1_ADDR, 0x0DU, 0x7FU); /* PULL_SEL: pull-up on P0..P6 */
    pi4ioe_write_reg(PI4IOE1_ADDR, 0x0BU, 0x7FU); /* PULL_EN: enable pull-up on P0..P6 */
    pi4ioe_write_reg(PI4IOE1_ADDR, 0x05U, 0x66U); /* OUT_SET: P1(SPK)=1, P2(EXT5V)=1, P5(TP_RST)=1, P6(CAM_RST)=1, P4(LCD_RST)=0 */
    pi4ioe_write_reg(PI4IOE1_ADDR, 0x07U, 0x00U); /* OUT_H_IM: active push-pull outputs */
    pi4ioe_write_reg(PI4IOE1_ADDR, 0x03U, 0x7FU); /* IO_DIR: P4 output low (assert LCD_RST at 0V) */
    maia_udelay(100000U);                        /* 100ms hold in reset (per Tab5 BSP) */
    /* LCD_RST (P4) released via input pull-up (1.8V safe, NEVER driven push-pull to 3.3V) */
    pi4ioe_write_reg(PI4IOE1_ADDR, 0x03U, 0x6FU); /* IO_DIR: P4 input with pull-up */
    maia_udelay(120000U);                        /* 120ms post-reset stabilization (per Tab5 BSP) */

    /* Initialize PI4IOE2 (0x44): enable WLAN_PWR (P0), USB5V_EN (P3) */
    pi4ioe_write_reg(PI4IOE2_ADDR, 0x01U, 0xFFU); /* CHIP_RESET */
    (void)pi4ioe_read_reg(PI4IOE2_ADDR, 0x01U);
    pi4ioe_write_reg(PI4IOE2_ADDR, 0x03U, 0xB9U); /* IO_DIR */
    pi4ioe_write_reg(PI4IOE2_ADDR, 0x07U, 0x06U); /* OUT_H_IM */
    pi4ioe_write_reg(PI4IOE2_ADDR, 0x0DU, 0xB9U); /* PULL_SEL */
    pi4ioe_write_reg(PI4IOE2_ADDR, 0x0BU, 0xF9U); /* PULL_EN */
    pi4ioe_write_reg(PI4IOE2_ADDR, 0x09U, 0x40U); /* IN_DEF_STA */
    pi4ioe_write_reg(PI4IOE2_ADDR, 0x11U, 0xBFU); /* INT_MASK */
    pi4ioe_write_reg(PI4IOE2_ADDR, 0x05U, 0x09U); /* OUT_SET: P0(WLAN)=1, P3(USB5V_EN)=1 */

    /* Wait 100ms for LCD, Touch, and 5V Keyboard MCU to fully power up and boot */
    maia_udelay(100000U);

    scan_and_report_i2c_bus("SysI2C(GPIO31/32)", TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN);
    scan_and_report_i2c_bus("ExtI2C(GPIO0/1)", TAB5_EXT_SDA_PIN, TAB5_EXT_SCL_PIN);

    /* 4. Detect Display Controller via Touch I2C address on System I2C */
    if (i2c_probe(TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN, 0x55U)) {
        uint8_t reg16[2] = {0x00U, 0x00U};
        uint8_t fw_ver = 0U;
        (void)maia_i2c_write_read(TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN, 0x55U, reg16, 2U, &fw_ver, 1U);
        if (fw_ver == 3U) {
            g_detected_panel = MAIA_TAB5_PANEL_ST7123;
            maia_console_puts("[PANEL] Detected ST7123 (touch 0x55 fw=3)\n");
        } else {
            g_detected_panel = MAIA_TAB5_PANEL_ST7121;
            maia_console_puts("[PANEL] Detected ST7121 (touch 0x55 fw=");
            print_hex8(fw_ver);
            maia_console_puts(")\n");
        }
    } else if (i2c_probe(TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN, 0x14U) ||
               i2c_probe(TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN, 0x5DU)) {
        g_detected_panel = MAIA_TAB5_PANEL_ILI9881C;
        maia_console_puts("[PANEL] Detected ILI9881C (GT911 touch @ 0x14/0x5D)\n");
    } else {
        g_detected_panel = MAIA_TAB5_PANEL_ST7121;
        maia_console_puts("[PANEL] Defaulting to ST7121 (Tab5 v1.3)\n");
    }

    /* 5. Detect Physical Keyboard on ExtI2C (GPIO0/1) or SysI2C (GPIO31/32) */
    if (i2c_probe(TAB5_EXT_SDA_PIN, TAB5_EXT_SCL_PIN, TCA8418_ADDR)) {
        g_kbd_type = 1U;
        g_kbd_bus_sda = TAB5_EXT_SDA_PIN;
        g_kbd_bus_scl = TAB5_EXT_SCL_PIN;
        tca8418_init_scanner();
        maia_console_puts("[KBD] Initialized M5Stack Tab5 Keyboard (TCA8418 @ 0x34 on GPIO0/1)\n");
    } else if (i2c_probe(TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN, TCA8418_ADDR)) {
        g_kbd_type = 1U;
        g_kbd_bus_sda = TAB5_SYS_SDA_PIN;
        g_kbd_bus_scl = TAB5_SYS_SCL_PIN;
        tca8418_init_scanner();
        maia_console_puts("[KBD] Initialized M5Stack Tab5 Keyboard (TCA8418 @ 0x34 on GPIO31/32)\n");
    } else if (i2c_probe(TAB5_EXT_SDA_PIN, TAB5_EXT_SCL_PIN, STM32_KBD_ADDR)) {
        g_kbd_type = 2U;
        g_kbd_bus_sda = TAB5_EXT_SDA_PIN;
        g_kbd_bus_scl = TAB5_EXT_SCL_PIN;
        /* Configure A164 STM32 Keyboard: Reg 0x10 = 0x02 (String Mode), INT_CFG=7, clr queue & sta, RGB on */
        uint8_t cfg_mode[2] = {0x10U, 0x02U}; /* Reg 0x10 = String/Character Mode */
        uint8_t cfg_irq[2]  = {0x00U, 0x07U}; /* Reg 0x00 = INT_CFG enabled */
        uint8_t clr_evq[2]  = {0x02U, 0x00U}; /* Reg 0x02 = Clear EVENT_NUM queue */
        uint8_t clr_sta[2]  = {0x01U, 0x00U}; /* Reg 0x01 = Clear INT_STA */
        uint8_t cfg_rgb[2]  = {0x03U, 0x32U}; /* Reg 0x03 = RGB_BRIGHTNESS = 50 */
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, cfg_mode, 2U);
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, cfg_irq, 2U);
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, clr_evq, 2U);
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, clr_sta, 2U);
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, cfg_rgb, 2U);
        maia_console_puts("[KBD] Initialized M5Stack Tab5 Keyboard (A164 @ 0x6D on GPIO0/1)\n");
    } else if (i2c_probe(TAB5_SYS_SDA_PIN, TAB5_SYS_SCL_PIN, STM32_KBD_ADDR)) {
        g_kbd_type = 2U;
        g_kbd_bus_sda = TAB5_SYS_SDA_PIN;
        g_kbd_bus_scl = TAB5_SYS_SCL_PIN;
        uint8_t cfg_mode[2] = {0x10U, 0x02U};
        uint8_t cfg_irq[2]  = {0x00U, 0x07U};
        uint8_t clr_evq[2]  = {0x02U, 0x00U};
        uint8_t clr_sta[2]  = {0x01U, 0x00U};
        uint8_t cfg_rgb[2]  = {0x03U, 0x32U};
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, cfg_mode, 2U);
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, cfg_irq, 2U);
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, clr_evq, 2U);
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, clr_sta, 2U);
        (void)maia_i2c_write_bytes(g_kbd_bus_sda, g_kbd_bus_scl, STM32_KBD_ADDR, cfg_rgb, 2U);
        maia_console_puts("[KBD] Initialized M5Stack Tab5 Keyboard (A164 @ 0x6D on GPIO31/32)\n");
    } else {
        maia_console_puts("[KBD] No I2C keyboard detected on GPIO0/1 or GPIO31/32\n");
    }
}
