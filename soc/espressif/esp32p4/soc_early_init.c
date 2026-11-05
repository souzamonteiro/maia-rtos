#include <maia/console.h>
#include <maia/soc/esp32p4_regs.h>

static void disable_timg_wdt(uint32_t base) {
    MAIA_MMIO32(MAIA_ESP32P4_TIMG_WDTWPROTECT_REG(base)) = MAIA_ESP32P4_WDT_WKEY_VALUE;
    MAIA_MMIO32(MAIA_ESP32P4_TIMG_WDTCONFIG0_REG(base))  = MAIA_ESP32P4_TIMG_WDT_CONF_UPDATE_EN;
    MAIA_MMIO32(MAIA_ESP32P4_TIMG_WDTWPROTECT_REG(base)) = 0UL;
}

static void disable_lp_wdt_and_swd(void) {
    /* Disable LP_WDT (RWDT) and its flashboot mode */
    MAIA_MMIO32(MAIA_ESP32P4_LP_WDT_WPROTECT_REG) = MAIA_ESP32P4_WDT_WKEY_VALUE;
    MAIA_MMIO32(MAIA_ESP32P4_LP_WDT_CONFIG0_REG)  = 0UL;
    MAIA_MMIO32(MAIA_ESP32P4_LP_WDT_WPROTECT_REG) = 0UL;

    /* Auto-feed and disable Super Watchdog (SWD) */
    MAIA_MMIO32(MAIA_ESP32P4_LP_WDT_SWD_WPROTECT_REG) = MAIA_ESP32P4_WDT_WKEY_VALUE;
    MAIA_MMIO32(MAIA_ESP32P4_LP_WDT_SWD_CONFIG_REG) |=
        (MAIA_ESP32P4_LP_WDT_SWD_AUTO_FEED_EN | MAIA_ESP32P4_LP_WDT_SWD_DISABLE);
    MAIA_MMIO32(MAIA_ESP32P4_LP_WDT_SWD_WPROTECT_REG) = 0UL;
}

void maia_soc_early_init(void) {
    disable_timg_wdt(MAIA_ESP32P4_TIMG0_BASE);
    disable_timg_wdt(MAIA_ESP32P4_TIMG1_BASE);
    disable_lp_wdt_and_swd();
    /* Disable USB-Serial-JTAG automatic chip reset on host DTR/RTS port close */
    MAIA_MMIO32(MAIA_ESP32P4_USB_CHIP_RST_REG) |= MAIA_ESP32P4_USB_UART_CHIP_RST_DIS;
    maia_console_init();
}

