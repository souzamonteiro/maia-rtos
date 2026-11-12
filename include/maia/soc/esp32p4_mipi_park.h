#ifndef MAIA_ESP32P4_MIPI_PARK_H
#define MAIA_ESP32P4_MIPI_PARK_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum maia_mipi_park_result {
    MAIA_MIPI_PARK_OK=0, MAIA_MIPI_PARK_INVALID=-1, MAIA_MIPI_PARK_CONTEXT=-2,
    MAIA_MIPI_PARK_BUSY=-3, MAIA_MIPI_PARK_VERIFY=-4, MAIA_MIPI_PARK_FAILED=-5
} maia_mipi_park_result_t;
typedef enum maia_mipi_park_stage {
    MAIA_MIPI_PARK_NONE=0, MAIA_MIPI_PARK_INSPECTED,
    MAIA_MIPI_PARK_BRIDGES, MAIA_MIPI_PARK_PHYS, MAIA_MIPI_PARK_VERIFIED
} maia_mipi_park_stage_t;
typedef struct maia_mipi_park_report {
    maia_mipi_park_stage_t completed;
    uint32_t attempted_mask; /* Bits0..3: DSI bridge, CSI bridge, DSI PHY, CSI PHY. */
    uint32_t dsi_bridge,csi_bridge,dsi_phy,csi_phy;
} maia_mipi_park_report_t;
/* ESP32-P4 chip1.3 boot-only reset-state normalization, reservation until reset.
 * Caller exclusively owns DSI/CSI/shared bridge/VDMA/clock access; other cores,
 * external sources and all producers quiescent; core0/IRQs-off, pre-scheduler.
 * No ISR/task/SMP/concurrent use. Hart/MIE check is not an ownership mechanism.
 * Require all three MIPI bus clocks, DSI/CSI PHY config clocks and both VDMA bus
 * clocks already on. No clocks are changed. VDMA global EN/channels/abort/reset
 * and all channel FIFOs must be zero. Stop VDMA separately before this call.
 * Require DSI host/analog PHY and CSI host/analog PHY already in shutdown/reset,
 * DPI output disabled, DSI FIFO empty. Live transmit/receive returns BUSY before
 * writes; this is not a general active-stream shutdown or lane-stop protocol.
 * Mask/verify bridges disabled, then DSI digital reset/clock/forcePLL released
 * bits cleared and CSI digital reset asserted. Preserve other fields and every
 * clock/power/VDMA register. No IRQ-status, test-port, DMA or framebuffer access.
 * Four finite steps, fresh preconditions and full readbacks. No retry/rollback;
 * failed step may have applied. Failure after inspection is sticky FAILED until
 * reset; success makes duplicate BUSY, both without later MMIO. Required report
 * always initialized; attempted_mask is conservative, stages are completed work.
 * OK proves register parked state, not analog settling, live-link stop, clock-
 * gating permission or a power-cycle guarantee. No native display completion. */
maia_mipi_park_result_t maia_esp32p4_mipi_park_reset(maia_mipi_park_report_t *report);
#ifdef __cplusplus
}
#endif
#endif
