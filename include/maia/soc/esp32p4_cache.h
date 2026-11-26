#ifndef MAIA_SOC_ESP32P4_CACHE_H
#define MAIA_SOC_ESP32P4_CACHE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Read cache geometry without changing global cache configuration. */
uint32_t maia_esp32p4_cache_line_size(void);

/* Synchronize only the requested L1 data-cache range for a DMA owner change.
 * The range must be aligned to the reported cache-line size. */
int maia_esp32p4_cache_writeback_invalidate(uint32_t address, uint32_t size);

/* Make CPU-written code visible to instruction fetch, like ESP-IDF's
 * esp_cache_msync(C2M|DATA) followed by esp_cache_msync(M2C|INST): write back
 * L1 DCache and invalidate both L1 ICaches for the line-expanded range. */
int maia_esp32p4_cache_sync_code(uint32_t address, uint32_t size);

#ifdef __cplusplus
}
#endif
#endif
