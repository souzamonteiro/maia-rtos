/* SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 * SPDX-License-Identifier: Apache-2.0
 * Adapted from ESP-IDF f5c3654 HAL gen_read_short_packet/host LL.
 * Modified: fixed VC0/RDDID04/MRPS3 or RDDPM0A/MRPS1, bounded checked settings.
 * No SDK headers, runtime or unbounded loops. LICENSES/Apache-2.0.txt. */
#define READ_ID_MRPS_HEADER 0x00000337U
#define READ_ID_DCS_HEADER 0x00000406U
#define READ_ID_LP_BITS 0x01040000U
#define READ_POWER_MRPS_HEADER 0x00000137U
#define READ_POWER_DCS_HEADER 0x00000A06U
