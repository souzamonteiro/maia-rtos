#ifndef MAIA_SOC_ESP32P4_LDO3_H
#define MAIA_SOC_ESP32P4_LDO3_H
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef enum maia_ldo3_result {
    MAIA_LDO3_OK=0,
    MAIA_LDO3_INVALID=-1,
    MAIA_LDO3_UNSUPPORTED=-2,
    MAIA_LDO3_EFUSE_ERROR=-3,
    MAIA_LDO3_CONTEXT=-4,
    MAIA_LDO3_BUSY=-5,
    MAIA_LDO3_VERIFY=-6,
    MAIA_LDO3_FAILED=-7
} maia_ldo3_result_t;
typedef struct maia_ldo3_snapshot {
    uint32_t mac_sys2,mac_sys3,rs_error0;
    uint32_t control,analog;
} maia_ldo3_snapshot_t;
typedef struct maia_ldo3_plan {
    uint8_t dref,mul,default_mask; /* Defaults used for zero K/Vos/C: bits0/1/2. */
    int32_t k1000,vos1000,c1000; /* SDK fixed-point coefficients. */
    uint32_t estimated_uv,error_uv; /* Model only, NOT measured rail voltage. */
} maia_ldo3_plan_t;
/* Read ROM-populated eFuse shadows and current VO3 PMU registers only.
 * Exclusive pre-scheduler/core0/IRQs-off use; no ISR/task/concurrent use.
 * No clocks, eFuse commands/programming or PMU writes. Requires ROM-initialized
 * readable shadows; does not initiate an eFuse read. out commits on success. */
maia_ldo3_result_t maia_esp32p4_ldo3_snapshot(maia_ldo3_snapshot_t *out);
/* Pure reentrant/thread/ISR-safe arithmetic with immutable caller inputs.
 * VO3 at2500mV only, chip1.3 and block schema major0/minor>=1, flags enabled.
 * Rejects uncorrectable block1 errors; corrected count is not a failure.
 * SDK signed-magnitude decoding/zero defaults and first-minimum tie order.
 * No heap/FPU/register writes. out commits only on success. No ownership,
 * enable/disable, stabilization, calibration burn or actual voltage guarantee. */
maia_ldo3_result_t maia_esp32p4_ldo3_plan2500(const maia_ldo3_snapshot_t *snapshot,
                                           maia_ldo3_plan_t *out);
typedef enum maia_ldo3_acquire_stage {
    MAIA_LDO3_ACQUIRE_NONE=0,
    MAIA_LDO3_ACQUIRE_PLANNED,
    MAIA_LDO3_ACQUIRE_ANALOG,
    MAIA_LDO3_ACQUIRE_ROUTED,
    MAIA_LDO3_ACQUIRE_ENABLED,
    MAIA_LDO3_ACQUIRE_VERIFIED
} maia_ldo3_acquire_stage_t;
typedef struct maia_ldo3_acquire_report {
    maia_ldo3_acquire_stage_t completed;
    unsigned adopted,may_have_changed;
    uint8_t dref,mul;
    uint32_t control,analog; /* Snapshot/programming-readback words, zero until captured. */
} maia_ldo3_acquire_report_t;
/* Dedicated fixed2500mV VO3 reservation until reset. No release/retune/refcount.
 * Boot/core0/IRQs-off only, exclusive PMU access, stable board supplies and no
 * other native/LP/external writer. DSI/CSI clocks must be gated before off-rail programming (checked).
 * Matching active rail is adopted without writes even if consumers run; active mismatch returns BUSY.
 * Off rail: masked configure/verify analog, SW source and enable; preserve other
 * fields. Any failure after entering programming phase is sticky until reset;
 * later calls return FAILED. No rollback/retry; hardware may already be changed.
 * Successful claim makes later acquisitions BUSY, without I/O. Required report
 * always initialized; completed describes last verified phase, not a failed step.
 * OK proves register state/reservation, NOT analog voltage or stabilization.
 * No eFuse writes, heap, FPU, locks, clocks setup, DSI or DMA initialization. */
maia_ldo3_result_t maia_esp32p4_ldo3_acquire2500(maia_ldo3_acquire_report_t *report);
#ifdef __cplusplus
}
#endif
#endif
