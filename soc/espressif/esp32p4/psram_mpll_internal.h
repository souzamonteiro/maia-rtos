/* Register facts from ESP-IDF f5c3654, analog-I2C patch/clk_tree_ll/reg_base.
 * Fixed divider read / powered400MHz calibration; no public generic writer. */
#ifndef MAIA_PSRAM_MPLL_INTERNAL_H
#define MAIA_PSRAM_MPLL_INTERNAL_H
#include "psram_internal.h"
#define MPLL_ANA 0x50124000U
#define MPLL_CONF1 (MPLL_ANA+0x1CU)
#define MPLL_CONF2 (MPLL_ANA+0x20U)
#define MPLL_LPCLK 0x50120000U
#define MPLL_LPRESET 0x50120008U
#define MPLL_POWER 0x5011515CU
#define MPLL_CLOCK 0x50111040U
#define MPLL_CAL (MAIA_ESP32P4_HP_SYS_CLKRST_BASE+0xBCU)
#define MPLL_BUSY 0x2000000U
#define MPLL_ANA_CLOCK (MPLL_ANA+0x34U)
#define MPLL_READ_DIV 0x263U
#endif
