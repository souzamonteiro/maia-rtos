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

static int sd_result_valid(maia_sd_result_t result) {
    return result == MAIA_SD_OK || result == MAIA_SD_INVALID ||
           result == MAIA_SD_UNSUPPORTED || result == MAIA_SD_NOT_READY ||
           result == MAIA_SD_OUT_OF_RANGE || result == MAIA_SD_TIMEOUT ||
           result == MAIA_SD_IO_ERROR || result == MAIA_SD_BUSY ||
           result == MAIA_SD_NO_MEDIA;
}

static maia_sd_result_t validate_request(
    const maia_sd_block_device_t *device, uint64_t first_block, uint32_t count,
    const void *buffer, uint32_t *completed_blocks) {
    if (completed_blocks != NULL) *completed_blocks = 0U;
    if (device == NULL || buffer == NULL || completed_blocks == NULL ||
        count == 0U) return MAIA_SD_INVALID;
    if (device->initialized != 1U) return MAIA_SD_NOT_READY;
    if (device->geometry.block_count == 0U ||
        device->geometry.block_count > SD_CSD_V2_MAX_BLOCKS) {
        return MAIA_SD_NOT_READY;
    }
    if (first_block >= device->geometry.block_count ||
        (uint64_t)count > device->geometry.block_count - first_block ||
        first_block > UINT32_MAX ||
        (uint64_t)(count - 1U) > UINT32_MAX - first_block) {
        return MAIA_SD_OUT_OF_RANGE;
    }

    uint64_t bytes = (uint64_t)count * MAIA_SD_BLOCK_BYTES;
    if (bytes > SIZE_MAX || bytes > UINTPTR_MAX ||
        (uintptr_t)buffer > UINTPTR_MAX - (uintptr_t)bytes) {
        return MAIA_SD_INVALID;
    }
    return MAIA_SD_OK;
}

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

maia_sd_result_t maia_sd_block_device_init(
    maia_sd_block_device_t *device, const maia_sd_geometry_t *geometry,
    maia_sd_block_read_fn read, maia_sd_block_write_fn write, void *context) {
    if (device == NULL || geometry == NULL || read == NULL || write == NULL) {
        return MAIA_SD_INVALID;
    }
    if (geometry->block_count == 0U ||
        geometry->block_count > SD_CSD_V2_MAX_BLOCKS) {
        return MAIA_SD_INVALID;
    }

    maia_sd_block_device_t initialized = {
        *geometry, read, write, context, 1U
    };
    *device = initialized;
    return MAIA_SD_OK;
}

static maia_sd_result_t transfer_blocks(
    maia_sd_block_device_t *device, uint64_t first_block, uint32_t count,
    uint8_t *read_buffer, const uint8_t *write_buffer,
    uint32_t *completed_blocks) {
    const void *buffer = read_buffer != NULL ? (const void *)read_buffer
                                              : (const void *)write_buffer;
    maia_sd_result_t result = validate_request(
        device, first_block, count, buffer, completed_blocks);
    if (result != MAIA_SD_OK) return result;

    maia_sd_block_read_fn read = device->read;
    maia_sd_block_write_fn write = device->write;
    void *context = device->context;
    if (read == NULL || write == NULL) return MAIA_SD_NOT_READY;

    uint32_t transferred = 0U;
    result = read_buffer != NULL
        ? read(context, (uint32_t)first_block, count, read_buffer, &transferred)
        : write(context, (uint32_t)first_block, count, write_buffer, &transferred);
    if (!sd_result_valid(result) || transferred > count) {
        return MAIA_SD_IO_ERROR;
    }
    *completed_blocks = transferred;
    if (result == MAIA_SD_OK && transferred != count) return MAIA_SD_IO_ERROR;
    return result;
}

maia_sd_result_t maia_sd_block_device_read(
    maia_sd_block_device_t *device, uint64_t first_block, uint32_t count,
    uint8_t *buffer, uint32_t *completed_blocks) {
    return transfer_blocks(device, first_block, count, buffer, NULL,
                           completed_blocks);
}

maia_sd_result_t maia_sd_block_device_write(
    maia_sd_block_device_t *device, uint64_t first_block, uint32_t count,
    const uint8_t *buffer, uint32_t *completed_blocks) {
    return transfer_blocks(device, first_block, count, NULL, buffer,
                           completed_blocks);
}
