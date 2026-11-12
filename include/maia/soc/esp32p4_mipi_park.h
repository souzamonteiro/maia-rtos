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
typedef enum maia_mipi_gate_stage {
    MAIA_MIPI_GATE_NONE=0, MAIA_MIPI_GATE_GUARDED, MAIA_MIPI_GATE_RESET,
    MAIA_MIPI_GATE_PHY, MAIA_MIPI_GATE_SYS, MAIA_MIPI_GATE_VERIFIED
} maia_mipi_gate_stage_t;
typedef struct maia_mipi_gate_report {
    maia_mipi_gate_stage_t completed;
    uint32_t attempted_mask; /* Bits0/1/2: reset, PHY clock, SYS clock writes. */
    uint32_t isp_clock,force_norst;
    uint32_t isp_inspected,isp_control,isp_camera; /* Before reset; zero when not readable. */
    uint32_t reset_before,reset_after,phy_before,phy_after,sys_before,sys_after;
} maia_mipi_gate_report_t;
/* Same exclusive chip1.3 boot contract, only AFTER successful native park in this
 * boot. No fabricated caller token/report can substitute for the internal claim.
 * Require relevant reset overrides clear, parked MIPI and VDMA still inactive.
 * If ISP clock is on and reset not held, inspect its input controls: CSI source
 * selected but MIPI input disabled; DVP disabled/reset, no pending update. Other
 * modes/capture inputs are BUSY before writes. Clock-off/reset-held ISP is not
 * read. Global ISP_EN alone is not a capture-input signal. No live teardown.
 * Hold ISP/CSI host/CSI bridge/DSI bridge RCC resets asserted (never pulse/release),
 * then mask off MIPI PHY/config/reference/DPI clocks, then all three MIPI bus
 * clocks. Preserve unrelated clock/reset bits, DMA clocks and ISP clock config.
 * After reset assertion, no more MIPI peripheral access; after gating, only RCC
 * and still-clocked VDMA checks/readback. Finite checked steps, no retry/rollback.
 * Report last completed phase/conservative attempts; failure after guards is
 * sticky FAILED until reset; successful duplicate BUSY, without later MMIO.
 * Reset may clear peripheral IRQ flags; caller handles routing before IRQ enable.
 * No explicit IRQ acknowledgement, LDO, voltage,
 * pin reset or memory writes; OK is checked boot clock/reset state, not a live
 * PHY/ISP shutdown, electrical settling or automatic permission to power cycle.
 * Reactivation/release/teardown runtime API is not implemented. */
maia_mipi_park_result_t maia_esp32p4_mipi_gate_clocks(maia_mipi_gate_report_t *report);
#ifdef __cplusplus
}
#endif
#endif
