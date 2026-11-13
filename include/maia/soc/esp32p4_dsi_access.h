/* Original MaiaRTOS MIT; register references: docs/audits/native-dsi-access.md. */
#ifndef MAIA_ESP32P4_DSI_ACCESS_H
#define MAIA_ESP32P4_DSI_ACCESS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum maia_dsi_access_result {
    MAIA_DSI_ACCESS_OK=0, MAIA_DSI_ACCESS_INVALID=-1, MAIA_DSI_ACCESS_CONTEXT=-2,
    MAIA_DSI_ACCESS_BUSY=-3, MAIA_DSI_ACCESS_VERIFY=-4, MAIA_DSI_ACCESS_FAILED=-5
} maia_dsi_access_result_t;
typedef enum maia_dsi_access_stage {
    MAIA_DSI_ACCESS_NONE=0, MAIA_DSI_ACCESS_GUARDED, MAIA_DSI_ACCESS_BUS,
    MAIA_DSI_ACCESS_CONFIG, MAIA_DSI_ACCESS_RELEASED, MAIA_DSI_ACCESS_VERIFIED
} maia_dsi_access_stage_t;
typedef struct maia_dsi_access_report {
    maia_dsi_access_stage_t completed;
    uint32_t attempted_mask; /* Bits0/1/2: DSI bus, config clock, reset release. */
    uint32_t sys_before,sys_after,phy_before,phy_after,reset_before,reset_after;
    uint32_t control,analog; /* Full calibrated active VO3 words, never written. */
    uint32_t version,power,phy_reset,bridge,dpi,fifo,phy_status;
} maia_dsi_access_report_t;
/* Boot-only exclusive register-access preparation, reservation until reset.
 * ESP32-P4 chip1.3, core0/IRQs-off/pre-scheduler; no ISR/task/SMP/concurrent use.
 * Caller owns RCC/VDMA/DSI/CSI/ISP/PMU, all producers quiescent; native VO3
 * acquisition must have succeeded and supply be stable by board policy. Hart/MIE
 * and matching register state are guards, not proof of ownership/analog voltage.
 * Require ALL MIPI SYS/PHY clocks gated, shared resets26..29 held, reset overrides
 * clear, VDMA bus clocks on/global/channels/abort/reset/FIFOs0, supported eFuse
 * calibration and active VO3 exactly matching native2500mV model. No adoption of
 * a running DSI or resetting active hardware. BUSY/VERIFY before writes is retryable.
 * Enable only DSI SYS12, PHY config0, then release ONLY DSI bridge RCC reset26.
 * Full readbacks/fresh RCC, VDMA and supply guards; preserve other fields,
 * CSI/ISP resets and all reference/DPI clocks/source/dividers. Only then read DSI
 * host/bridge and require host power/PHY analog+digital controls/bridge/DPI off,
 * FIFO empty. Version/status are observations, not an analog-ready certificate.
 * No PLL/source/ref-clock/test-port/lane/PHY power/video/DMA/IRQ/panel/PSRAM writes.
 * Finite steps, no waits/heap/locks, no retry/rollback after first write attempt:
 * failed programming is sticky FAILED, may leave clocks/reset changed. Success
 * makes duplicates BUSY without MMIO. Required report is always initialized,
 * last completed stage and conservative attempts reported. No release API.
 * OK means safe configuration access with transmit controls off, not initialized
 * PHY/lanes, settling, functional display or a complete native desktop. */
maia_dsi_access_result_t maia_esp32p4_dsi_open_config(maia_dsi_access_report_t *report);
#ifdef __cplusplus
}
#endif
#endif
