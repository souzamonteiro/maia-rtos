#ifndef MAIA_DRIVERS_SD_READ_H
#define MAIA_DRIVERS_SD_READ_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAIA_SD_BLOCK_BYTES 512U

typedef enum maia_sd_result {
    MAIA_SD_OK = 0,
    MAIA_SD_INVALID = -1,
    MAIA_SD_UNSUPPORTED = -2,
    MAIA_SD_NOT_READY = -3,
    MAIA_SD_OUT_OF_RANGE = -4,
    MAIA_SD_TIMEOUT = -5,
    MAIA_SD_IO_ERROR = -6
} maia_sd_result_t;

typedef struct maia_sd_geometry {
    uint64_t block_count;
} maia_sd_geometry_t;

/* R2 CSD bits [127:0] are represented as four words, least significant
 * word first, as in the ESP32-P4 SDMMC response registers. The transport must
 * already have validated the response and negotiated OCR at the actual supply
 * voltage. Only ready, block-addressed SDHC/SDXC with CSD structure 1 and
 * 512-byte read blocks are accepted. Other CSD formats are unsupported.
 * The output is unchanged on failure. Pure, reentrant, no I/O or allocation. */
maia_sd_result_t maia_sd_sdhc_geometry(const uint32_t csd[4], uint32_t ocr,
                                       maia_sd_geometry_t *out);

/* The callback must issue CMD17 with this block address, wait in bounded time,
 * verify the R1 status, data length and CRC, and fill exactly 512 bytes on
 * success. It must return a specific non-OK result on failure. On error it may
 * have partially modified the buffer; callers must discard it. It must not
 * issue writes, erase, format, or change card configuration.
 *
 * Caller owns an initialized and selected SDHC/SDXC card, exclusively
 * serializes its transport, and provides a stable 512-byte buffer satisfying
 * the transport's DMA/alignment/cache requirements. Buffer and geometry must
 * not overlap. This operation is thread/boot context only, not ISR-safe.
 * Invalid inputs and out-of-range addresses never call the transport. */
typedef maia_sd_result_t (*maia_sd_read_block_fn)(void *context,
                                                   uint32_t block_address,
                                                   uint8_t *buffer);
maia_sd_result_t maia_sd_read_block(const maia_sd_geometry_t *geometry,
                                    maia_sd_read_block_fn read_block,
                                    void *context, uint64_t block,
                                    uint8_t *buffer);

#ifdef __cplusplus
}
#endif
#endif
