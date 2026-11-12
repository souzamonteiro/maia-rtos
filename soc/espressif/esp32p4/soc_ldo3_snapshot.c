/* Read-only ROM-shadow/PMU inspection; facts: docs/audits/native-ldo3-calibration.md. */
#include <maia/soc/esp32p4_ldo3.h>
#include "ldo3_internal.h"
maia_ldo3_result_t maia_esp32p4_ldo3_snapshot(maia_ldo3_snapshot_t *out) {
    if (!out) return MAIA_LDO3_INVALID;
    if (!maia_ldo3_boot_context()) return MAIA_LDO3_CONTEXT;
    maia_ldo3_snapshot_t s;
    s.mac_sys2=maia_ldo3_read(MAIA_ESP32P4_EFUSE_MAC_SYS2_REG);
    s.mac_sys3=maia_ldo3_read(MAIA_ESP32P4_EFUSE_MAC_SYS3_REG);
    s.rs_error0=maia_ldo3_read(MAIA_ESP32P4_EFUSE_RS_ERR0_REG);
    s.control=maia_ldo3_read(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG);
    s.analog=maia_ldo3_read(MAIA_ESP32P4_PMU_EXT_LDO_VO3_ANA_REG);
    *out=s;
    return MAIA_LDO3_OK;
}
