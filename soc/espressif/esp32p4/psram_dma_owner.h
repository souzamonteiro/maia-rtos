/* Original MaiaRTOS MIT. Boot-only native SoC memory ownership. */
#ifndef MAIA_PSRAM_DMA_OWNER_H
#define MAIA_PSRAM_DMA_OWNER_H
#include <stdint.h>
/* Private PSRAM->DSI lifetime handoff, fixed2MiB boot reservation only.
 * acquire syncs cache, requires200MHz and idleDMA, freezes sync/stop; returns
 * fixed source or0. live verifies owned roots/controller; release only after
 * actualDMA disabled/drained and table/guards match. Faults require reboot. */
uint32_t maia_psram_window_dma_acquire(void);
int maia_psram_window_dma_live(void);
int maia_psram_window_dma_release(void);
#endif
