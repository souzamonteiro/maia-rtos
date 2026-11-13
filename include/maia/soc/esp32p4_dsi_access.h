/* Original MaiaRTOS MIT; register references: docs/audits/native-dsi-access.md. */
#ifndef MAIA_ESP32P4_DSI_ACCESS_H
#define MAIA_ESP32P4_DSI_ACCESS_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum maia_dsi_access_result {
    MAIA_DSI_ACCESS_OK=0, MAIA_DSI_ACCESS_INVALID=-1, MAIA_DSI_ACCESS_CONTEXT=-2,
    MAIA_DSI_ACCESS_BUSY=-3, MAIA_DSI_ACCESS_VERIFY=-4, MAIA_DSI_ACCESS_FAILED=-5, MAIA_DSI_ACCESS_TIMEOUT=-6
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
typedef enum maia_dsi_reference_stage {
    MAIA_DSI_REFERENCE_NONE=0, MAIA_DSI_REFERENCE_GUARDED,
    MAIA_DSI_REFERENCE_PROVIDER, MAIA_DSI_REFERENCE_CONFIG_OFF, MAIA_DSI_REFERENCE_SOURCE,
    MAIA_DSI_REFERENCE_CONFIG,
    MAIA_DSI_REFERENCE_CLOCK, MAIA_DSI_REFERENCE_VERIFIED
} maia_dsi_reference_stage_t;
typedef struct maia_dsi_reference_report {
    maia_dsi_reference_stage_t completed;
    uint32_t attempted_mask; /* Bits0..4: provider, config off, mux, config restore, ref on. */
    uint32_t divider_word,lp_gate,nominal_hz; /* Model480MHz/div24, NOT measured. */
    uint32_t provider_before,provider_after,source_before,source_after,phy_before,phy_after;
} maia_dsi_reference_report_t;
/* Same exclusive boot/supply contract, only after successful native open_config
 * in this boot (internal claim). Requires still-shutdown DSI, same access/reset
 * pattern, inactive VDMA, reference/DPI/CSI clocks off. Caller guarantees stable
 * inherited SPLL480MHz/40MHz XTAL; this is NOT cold PLL startup or frequency
 * measurement. Verify LP XTAL/SPLL gates on and shared REF20 divider exactly24;
 * never reprogram that divider, root PLL/power or LP gates. Refuse other dividers.
 * Enable shared REF20 provider only if off, select DSI source0 only if needed,
 * gate config clock around any mux change, restore config, then enable reference
 * bit1. No DSI MMIO while config clock is off. Preserve other fields; shared gate stays
 * enabled until reset; no general clock manager/refcount/runtime teardown.
 * Fresh RCC/provider/LP/divider/VDMA/eFuse/PMU/DSI-off guards and full readbacks.
 * Finite steps/no waits/rollback; failure after guards sticky FAILED until reset,
 * duplicate successful call BUSY without MMIO. Required report always initialized.
 * No PHY test-port/PLL/analog/lane/panel/video writes; OK is reference routing and
 * nominal software frequency only, not PLL lock, clock measurement or display. */
maia_dsi_access_result_t maia_esp32p4_dsi_reference20(maia_dsi_reference_report_t *report);
typedef struct maia_dsi_phy_report {
    uint32_t completed; /*0:none 1:guarded 2:lanes 3:host 4:analog 5:reset release
                         6:clock 7:force PLL 8:programmed 9:lock 10:stop 11:verified. */
    uint32_t writes,programmed,polls,status;
    uint32_t power,phy_reset,lanes,nominal_mbps;
} maia_dsi_phy_report_t;
/* Fixed two-lane960Mbps PHY, nominal20MHz reference, exclusive boot contract.
 * Only after this module's successful reference20 claim. Require unchanged
 * reference routing/divider/provider, quiet VDMA, calibrated VO3, bridge/DPI off,
 * host/PHY shutdown/reset and no HS clock request; initial lock must be low.
 * Adapted SDK initialization and test-port sequence, attributed Apache-2.0.
 * Mask/check lanes, host power, analog shutdown release, digital reset release,
 * clock lane enable and forcePLL; program five PHY bytes (M48/N1/hsfreq1A).
 * Test-interface readback verifies writable controls only, not hidden PHY bytes.
 * Poll lock bit0 then all lane-stop bits94, finite poll_budget1..100000; no
 * real-time/settling guarantee. TIMEOUT=-6 leaves state as-is/sticky FAILED.
 * Fresh RCC/VDMA/supply/DSI guards between steps/polls; no rollback/retry on any
 * programming failure. Duplicate success BUSY without MMIO, lifetime until reset.
 * No packet/HS request, panel command, video/DMA/PSRAM setup or complete display.
 * Powered PHY may drive LP idle levels; caller exclusively owns attached lanes.
 * Model960Mbps not measured. Report required/always initialized except NULL;
 * writes includes conservative attempted stores, programmed counts complete bytes. */
maia_dsi_access_result_t maia_esp32p4_dsi_phy960(uint32_t poll_budget,maia_dsi_phy_report_t *report);
#ifdef __cplusplus
}
#endif
#endif
