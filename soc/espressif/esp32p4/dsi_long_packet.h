/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Adapted from ESP-IDF f5c3654 HAL long packet/header path. LICENSES/Apache-2.0.txt.
 * Modified: explicit DCS39/LP19, bytewise bounded packing (no unaligned uint32_t
 * cast or tail overread), native aggregate budget and partial-attempt accounting.
 * docs/audits/native-dsi-long.md. No SDK headers or unbounded FIFO loops. */
static uint32_t long_packet_header(uint32_t vc,uint32_t length) { return (length<<8U)|(vc<<6U)|0x39U; }
static uint32_t long_packet_word(const uint8_t *bytes,uint32_t count) {
    uint32_t word=0;
    for (uint32_t i=0;i<count;++i) word|=(uint32_t)bytes[i]<<(8U*i);
    return word;
}
