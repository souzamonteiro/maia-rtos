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
    MAIA_LDO3_CONTEXT=-4
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
#ifdef __cplusplus
}
#endif
#endif
