/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Adapted header packing/LP selector from ESP-IDF f5c3654 HAL/host LL.
 * Modified: validated fixed DCS0/1 short cases, no SDK dependencies or busy loops.
 * LICENSES/Apache-2.0.txt; docs/audits/native-dsi-short.md. */
static uint32_t short_packet_header(uint32_t vc,uint32_t command,uint32_t count,uint32_t parameter) {
    return (parameter<<16U)|(command<<8U)|(vc<<6U)|(count ? 0x15U : 0x05U);
}
static uint32_t short_packet_lp(uint32_t old,uint32_t count) { return old|(1U<<(16U+count)); }
