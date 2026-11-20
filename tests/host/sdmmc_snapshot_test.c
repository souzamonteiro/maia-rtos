#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include "native_sdmmc_snapshot.h"

static const unsigned offsets[14] = {
    0x00U, 0x08U, 0x0CU, 0x10U, 0x14U, 0x18U, 0x2CU,
    0x44U, 0x48U, 0x50U, 0x4CU, 0x80U, 0x6CU, 0x70U
};
static const uintptr_t guards[] = {
    SDMMC_SNAPSHOT_SYS, SDMMC_SNAPSHOT_CLOCK, SDMMC_SNAPSHOT_RESET,
    SDMMC_SNAPSHOT_LDO, SDMMC_SNAPSHOT_LDO_ANA
};
static unsigned seen_guards[5], seen_div, seen_pins[6], seen_host[14];
static unsigned changing, enabled, reset_asserted;

static uint32_t read32(uintptr_t address) {
    for (unsigned i = 0; i < 5U; ++i) {
        if (address != guards[i]) continue;
        assert(++seen_guards[i] <= 2U);
        uint32_t value = i == 0U ? (enabled ? SDMMC_SNAPSHOT_SYS_ENABLE : 0U) :
                         i == 2U ? (reset_asserted ? SDMMC_SNAPSHOT_RESET_ASSERTED : 0U) :
                         (uint32_t)address;
        return value ^ ((changing == i + 1U && seen_guards[i] == 2U) ? 1U : 0U);
    }
    if (address == SDMMC_SNAPSHOT_DIV) {
        assert(++seen_div == 1U);
        return (uint32_t)address;
    }
    for (unsigned i = 0; i < 6U; ++i) {
        if (address == MAIA_ESP32P4_IO_MUX_BASE + 0xA0U + 4U * i) {
            assert(++seen_pins[i] == 1U);
            return (uint32_t)address;
        }
    }
    for (unsigned i = 0; i < 14U; ++i) {
        if (address == SDMMC_SNAPSHOT_HOST + offsets[i]) {
            assert(enabled && !reset_asserted);
            assert(++seen_host[i] == 1U);
            return (uint32_t)address;
        }
    }
    assert(!"register not on the read-only whitelist");
    return 0U;
}

static void run(unsigned bus_on, unsigned reset_on, unsigned changed) {
    enabled = bus_on; reset_asserted = reset_on; changing = changed;
    seen_div = 0;
    for (unsigned i = 0; i < 5U; ++i) seen_guards[i] = 0;
    for (unsigned i = 0; i < 6U; ++i) seen_pins[i] = 0;
    for (unsigned i = 0; i < 14U; ++i) seen_host[i] = 0;
    sdmmc_snapshot_t s;
    assert(sdmmc_snapshot(&s, read32) == (changed == 0U));
    assert(s.valid == (bus_on && !reset_on));
    assert(seen_div == 1U);
    for (unsigned i = 0; i < 5U; ++i) assert(seen_guards[i] == 2U);
    for (unsigned i = 0; i < 6U; ++i) {
        assert(seen_pins[i] == 1U);
        assert(s.pins[i] == MAIA_ESP32P4_IO_MUX_BASE + 0xA0U + 4U * i);
    }
    for (unsigned i = 0; i < 14U; ++i) {
        assert(seen_host[i] == s.valid);
        assert(s.host[i] == (s.valid ? SDMMC_SNAPSHOT_HOST + offsets[i] : 0U));
    }
}

int main(void) {
    for (unsigned on = 0; on < 2U; ++on) {
        for (unsigned reset = 0; reset < 2U; ++reset) run(on, reset, 0U);
    }
    for (unsigned changed = 1; changed <= 5U; ++changed) run(1U, 0U, changed);
    puts("PASS: SDMMC whitelist, clock/reset guards, skipped fields and unstable-gate rejection");
    return 0;
}
