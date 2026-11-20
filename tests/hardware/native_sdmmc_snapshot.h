/* Read-only boot diagnostic, not a driver or proof of media readiness.
 * Register sources: ESP-IDF f5c3654, documented in docs/hardware/EVIDENCE.md.
 * Original MaiaRTOS code, MIT. */
#ifndef MAIA_NATIVE_SDMMC_SNAPSHOT_H
#define MAIA_NATIVE_SDMMC_SNAPSHOT_H

#include <stdint.h>
#include <maia/soc/esp32p4_regs.h>

#define SDMMC_SNAPSHOT_SYS (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x18U)
#define SDMMC_SNAPSHOT_CLOCK (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x34U)
#define SDMMC_SNAPSHOT_DIV (MAIA_ESP32P4_HP_SYS_CLKRST_BASE + 0x38U)
#define SDMMC_SNAPSHOT_RESET 0x5011104CU
#define SDMMC_SNAPSHOT_LDO (MAIA_ESP32P4_PMU_BASE + 0x1D8U)
#define SDMMC_SNAPSHOT_LDO_ANA (MAIA_ESP32P4_PMU_BASE + 0x1DCU)
#define SDMMC_SNAPSHOT_HOST 0x50083000U
#define SDMMC_SNAPSHOT_SYS_ENABLE (1U << 14U)
#define SDMMC_SNAPSHOT_RESET_ASSERTED (1U << 28U)

typedef uint32_t (*sdmmc_snapshot_read_fn)(uintptr_t);
typedef struct {
    uint32_t sys, clock, div, reset, ldo, analog;
    uint32_t sys_after, clock_after, reset_after, ldo_after, analog_after;
    uint32_t valid, pins[6], host[14];
} sdmmc_snapshot_t;

/* Access is serialized in boot context. This is a sequential observation;
 * stable gates do not establish controller quiescence or a stable supply. */
static inline int sdmmc_snapshot(sdmmc_snapshot_t *s, sdmmc_snapshot_read_fn read32) {
    static const unsigned offsets[14] = {
        0x00U, 0x08U, 0x0CU, 0x10U, 0x14U, 0x18U, 0x2CU,
        0x44U, 0x48U, 0x50U, 0x4CU, 0x80U, 0x6CU, 0x70U
    };
    volatile unsigned char *bytes = (volatile unsigned char *)s;
    for (unsigned i = 0; i < sizeof *s; ++i) bytes[i] = 0;
    s->sys = read32(SDMMC_SNAPSHOT_SYS);
    s->clock = read32(SDMMC_SNAPSHOT_CLOCK);
    s->div = read32(SDMMC_SNAPSHOT_DIV);
    s->reset = read32(SDMMC_SNAPSHOT_RESET);
    s->ldo = read32(SDMMC_SNAPSHOT_LDO);
    s->analog = read32(SDMMC_SNAPSHOT_LDO_ANA);
    for (unsigned i = 0; i < 6U; ++i) {
        s->pins[i] = read32(MAIA_ESP32P4_IO_MUX_BASE + 0xA0U + 4U * i);
    }
    if ((s->sys & SDMMC_SNAPSHOT_SYS_ENABLE) && !(s->reset & SDMMC_SNAPSHOT_RESET_ASSERTED)) {
        s->valid = 1U;
        for (unsigned i = 0; i < 14U; ++i) s->host[i] = read32(SDMMC_SNAPSHOT_HOST + offsets[i]);
    }
    s->sys_after = read32(SDMMC_SNAPSHOT_SYS);
    s->clock_after = read32(SDMMC_SNAPSHOT_CLOCK);
    s->reset_after = read32(SDMMC_SNAPSHOT_RESET);
    s->ldo_after = read32(SDMMC_SNAPSHOT_LDO);
    s->analog_after = read32(SDMMC_SNAPSHOT_LDO_ANA);
    return s->sys == s->sys_after && s->clock == s->clock_after &&
           s->reset == s->reset_after && s->ldo == s->ldo_after &&
           s->analog == s->analog_after;
}
#endif
