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

#ifdef __cplusplus
}
#endif
#endif
