/* Read-only ROM-shadow/PMU inspection; facts: docs/audits/native-ldo3-calibration.md. */
#include <maia/soc/esp32p4_ldo3.h>
#include <maia/soc/esp32p4_regs.h>
maia_ldo3_result_t maia_esp32p4_ldo3_snapshot(maia_ldo3_snapshot_t *out) {
    if (!out) return MAIA_LDO3_INVALID;
    uint32_t status,hart;
    __asm__ volatile("csrr %0, mstatus\n\tcsrr %1, mhartid" : "=r"(status),"=r"(hart));
    if ((status&8U) || hart) return MAIA_LDO3_CONTEXT;
    maia_ldo3_snapshot_t s;
    s.mac_sys2=MAIA_MMIO32(MAIA_ESP32P4_EFUSE_MAC_SYS2_REG);
    s.mac_sys3=MAIA_MMIO32(MAIA_ESP32P4_EFUSE_MAC_SYS3_REG);
    s.rs_error0=MAIA_MMIO32(MAIA_ESP32P4_EFUSE_RS_ERR0_REG);
    s.control=MAIA_MMIO32(MAIA_ESP32P4_PMU_EXT_LDO_VO3_REG);
    s.analog=MAIA_MMIO32(MAIA_ESP32P4_PMU_EXT_LDO_VO3_ANA_REG);
    *out=s;
    return MAIA_LDO3_OK;
}
