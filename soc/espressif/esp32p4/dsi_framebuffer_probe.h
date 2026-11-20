/* Original MIT. Private boot-only diagnostic; no production task/ISR API.
 * Native200MHz PSRAM window reserves two aligned framebuffers, the first at
 * +128 and the second immediately after the first frame; each is 1843200 bytes.
 * The diagnostic uses the first immutable framebuffer[+128,+1843328),
 * all external clients stopped. Claim syncs cache and freezes window lifecycle
 * until bounded transfer/drain. Source fixed by private memory capability, not
 * caller-provided address/token.4 or1000 frames selected by probe; API1..1200,
 * poll budget1..100000 per frame. Failure sticky, no unsafe source release on
 * uncertain recovery. No visual/streaming/deadline/underrun-free guarantee. */
#ifndef MAIA_DSI_FRAMEBUFFER_PROBE_H
#define MAIA_DSI_FRAMEBUFFER_PROBE_H
#include <maia/soc/esp32p4_dsi_access.h>
#define DSI_FB_SOURCE 0x4B800080U
#define DSI_FB_BYTES 1843200U
#define DSI_FB_BACK_SOURCE (DSI_FB_SOURCE+DSI_FB_BYTES)
typedef struct maia_dsi_framebuffer_report {
    maia_dsi_dma_stage_report_t stage;
    maia_dsi_dma_refresh_report_t refresh;
    uint32_t source_released;
} maia_dsi_framebuffer_report_t;
maia_dsi_access_result_t maia_dsi_framebuffer_probe(uint32_t frames,uint32_t budget,maia_dsi_framebuffer_report_t *);
#endif
