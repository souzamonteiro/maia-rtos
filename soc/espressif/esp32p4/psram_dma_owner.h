/* Original MaiaRTOS MIT. Boot-only native SoC memory ownership. */
#ifndef MAIA_PSRAM_DMA_OWNER_H
#define MAIA_PSRAM_DMA_OWNER_H
#include <stdint.h>
/* Private PSRAM->DSI lifetime handoff, fixed4MiB boot reservation only.
 * acquire syncs cache, requires200MHz and idleDMA, freezes sync/stop; returns
 * the first frame source or0. live verifies owned roots/controller; release
 * only after actualDMA disabled/drained and table/guards match. Faults require
 * reboot. */
uint32_t maia_psram_window_dma_acquire(void);
int maia_psram_window_dma_live(void);
int maia_psram_window_dma_release(void);
/* Core0 IRQs-off begins an edit in the non-scanout frame while DMA reads the
 * other frame. Publish may run with IRQs enabled and writes back only the edited
 * range. The lease blocks window lifecycle changes; no other PSRAM/cache clients
 * are supported. Zero on refusal/fault; failed publish is sticky. */
uint32_t maia_psram_window_edit_begin(uint32_t source,uint32_t bytes);
int maia_psram_window_edit_publish(uint32_t source,uint32_t bytes);
#endif
