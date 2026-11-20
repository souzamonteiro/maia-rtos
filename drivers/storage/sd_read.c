/*
 * Copyright (c) 2006 Uwe Stuehler <uwe@openbsd.org>
 * Adaptations to ESP-IDF Copyright (c) 2016-2024 Espressif Systems (Shanghai) PTE LTD
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 *
 * SDHC CSD fields and capacity calculation adapted from ESP-IDF revision
 * f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0, components/sdmmc/sdmmc_sd.c
 * and components/sdmmc/include/sd_protocol_defs.h. No ESP-IDF runtime.
 */
#include <maia/drivers/sd_read.h>

#include <stddef.h>

#define SD_OCR_READY UINT32_C(0x80000000)
#define SD_OCR_BLOCK_ADDRESSED UINT32_C(0x40000000)
#define SD_CSD_V2_MAX_BLOCKS (UINT64_C(1) << 32)

maia_sd_result_t maia_sd_sdhc_geometry(const uint32_t csd[4], uint32_t ocr,
                                       maia_sd_geometry_t *out) {
    if (csd == NULL || out == NULL) return MAIA_SD_INVALID;
    if ((ocr & SD_OCR_READY) == 0U) return MAIA_SD_NOT_READY;
    if ((ocr & SD_OCR_BLOCK_ADDRESSED) == 0U || (csd[3] >> 30U) != 1U) {
        return MAIA_SD_UNSUPPORTED;
    }

    uint32_t read_block_length = (csd[2] >> 16U) & 0xFU;
    uint32_t command_classes = (csd[2] >> 20U) & 0xFFFU;
    if (read_block_length != 9U || (command_classes & 4U) == 0U) {
        return MAIA_SD_UNSUPPORTED;
    }
    uint32_t size = ((csd[2] & 0x3FU) << 16U) | (csd[1] >> 16U);
    maia_sd_geometry_t geometry;
    geometry.block_count = ((uint64_t)size + 1U) * 1024U;
    *out = geometry;
    return MAIA_SD_OK;
}

maia_sd_result_t maia_sd_read_block(const maia_sd_geometry_t *geometry,
                                    maia_sd_read_block_fn read_block,
                                    void *context, uint64_t block,
                                    uint8_t *buffer) {
    if (geometry == NULL || read_block == NULL || buffer == NULL) {
        return MAIA_SD_INVALID;
    }
    if (geometry->block_count == 0U ||
        geometry->block_count > SD_CSD_V2_MAX_BLOCKS ||
        (geometry->block_count & 1023U) != 0U) {
        return MAIA_SD_INVALID;
    }
    if (block >= geometry->block_count || block > UINT32_MAX) {
        return MAIA_SD_OUT_OF_RANGE;
    }
    return read_block(context, (uint32_t)block, buffer);
}
