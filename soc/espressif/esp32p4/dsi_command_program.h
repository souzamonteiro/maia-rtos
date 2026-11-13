/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Parameters adapted from ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
 * esp_lcd_new_dsi_bus and mipi_dsi_host/phy_ll. LICENSES/Apache-2.0.txt.
 * Modified: fixed960Mbps model, declarative masked steps, native checked guards.
 * No SDK includes/runtime. docs/audits/native-dsi-command.md. */
static const uint16_t command_offsets[MAIA_DSI_COMMAND_WORDS]={
    0x34,0x08,0x9C,0x98,0x2C,0x78,0x7C,0x80,0x84,0x88,0x8C,0xF4,0xA4,0x94};
static const uint32_t command_masks[MAIA_DSI_COMMAND_WORDS]={
    1,0xFFFF,0x03FF03FF,0x03FF03FF,0x39,0xFFFFFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0xFFFF,0x7FFF,0xFF00,3};
static const uint32_t command_values[MAIA_DSI_COMMAND_WORDS]={
    1,0x0C06,0x00320068,0x002E0080,0x19,0,0,0,0,0,0,6000,0x3F00,0};
