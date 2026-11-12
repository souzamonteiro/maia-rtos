#ifndef MAIA_SOC_ESP32P4_VDMA_H
#define MAIA_SOC_ESP32P4_VDMA_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum maia_vdma_result {
    MAIA_VDMA_OK=0, MAIA_VDMA_INVALID=-1, MAIA_VDMA_CONTEXT=-2,
    MAIA_VDMA_BUSY=-3, MAIA_VDMA_VERIFY=-4, MAIA_VDMA_TIMEOUT=-5,
    MAIA_VDMA_FAILED=-6
} maia_vdma_result_t;
typedef enum maia_vdma_stop_stage {
    MAIA_VDMA_STOP_NONE=0, MAIA_VDMA_STOP_INSPECTED,
    MAIA_VDMA_STOP_REQUESTED, MAIA_VDMA_STOP_VERIFIED
} maia_vdma_stop_stage_t;
typedef struct maia_vdma_stop_report {
    maia_vdma_stop_stage_t completed;
    uint32_t initial_cfg,initial_channels,requested_channels;
    uint32_t cfg,channels,abort,fifo_nonempty,polls;
    unsigned adopted,may_have_changed;
} maia_vdma_stop_report_t;
/* Stop/reserve the entire four-channel VDMA until reset; no release/restart.
 * ESP32-P4 chip1.3 only. Exclusive pre-scheduler/core0/IRQs-off only: all other cores, LP/external
 * writers, callbacks and producers must be quiescent. Caller owns ALL channels.
 * Keep source/destination buses, clocks, supplies and handshakes available until
 * OK. No ISR/task/SMP/concurrent use. CSR checks enforce hart/MIE only.
 * Bus clocks must already be enabled (otherwise BUSY, no peripheral reads).
 * Existing abort/reset or inconsistent global-off/channel-on state rejects.
 * Disable enabled channels using hardware write-enable strobes; clear global
 * EN and poll its completion handshake. No abort/reset/IRQ-clear command,
 * descriptor/memory access or clock/PHY/power writes. Preserve global IRQ/other bits.
 * poll_limit=1..4096 is a TOTAL poll budget, not microseconds or a hard-time
 * guarantee; MMIO itself requires a responding bus. No retries or rollback.
 * Failure once stop phase starts is sticky FAILED until reset, no later I/O.
 * Successful call makes later calls BUSY without I/O. Required report always
 * initialized; completed is the last successful phase, not current HW status.
 * OK verifies global activity terminated per TRM CFG0 semantics, NOT successful
 * delivery/lossless drain, FIFO empty, other DMA engines or MIPI/PHY shutdown.
 * Pending interrupt flags are retained; caller must handle them before IRQ enable.
 * Early channel disable can discard partial data. fifo_nonempty reports final
 * nonempty channel FIFOs; controller remains disabled, registers/data retained.
 * Idle controller is adopted read-only; this is not active-transfer stop proof. */
maia_vdma_result_t maia_esp32p4_vdma_stop_all(uint32_t poll_limit,
                                            maia_vdma_stop_report_t *report);
#ifdef __cplusplus
}
#endif
#endif
