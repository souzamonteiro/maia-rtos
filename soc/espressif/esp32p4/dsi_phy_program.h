/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Adapted from ESP-IDF f5c3654 mipi_dsi_hal_configure_phy_pll /
 * mipi_dsi_hal_phy_write_register and PHY LL. License LICENSES/Apache-2.0.txt.
 * Modified: fixed exact960Mbps/reference20MHz, masked checked MMIO, bounded
 * original native failure/ownership policy in soc_dsi_access.c. No SDK headers. */
static int phy_test_write(uintptr_t address,uint32_t value,uint32_t mask) {
    uint32_t old=dsi_read(address),desired=(old&~mask)|(value&mask);
    dsi_write(address,desired);
    return (dsi_read(address)&mask) == (desired&mask);
}
static int phy_program_byte(uint8_t address,uint8_t value) {
    uintptr_t clock=MAIA_ESP32P4_DSI_HOST_BASE+0xB4U,data=MAIA_ESP32P4_DSI_HOST_BASE+0xB8U;
    return phy_test_write(clock,0U,3U) && phy_test_write(data,0x10000U|address,0x100FFU) &&
        phy_test_write(clock,2U,3U) && phy_test_write(clock,0U,3U) &&
        phy_test_write(data,value,0x100FFU) && phy_test_write(clock,2U,3U) && phy_test_write(clock,0U,3U);
}
/* SDK range950..999=>hsfreq0x1A, M48/N1 realizes960MHz from20MHz.
 * reg18 is programmed twice: low five M-1 bits, then high bits with latch bit7. */
static const uint8_t phy_pll_registers[5]={0x44,0x19,0x17,0x18,0x18};
static const uint8_t phy_pll_values[5]={0x34,0x30,0,0x0F,0x81};
