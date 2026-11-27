#ifndef MAIA_SOC_ESP32P4_SRAM_H
#define MAIA_SOC_ESP32P4_SRAM_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Zero-initialized data placed in high L2MEM (0x4FF40000..0x4FF80000).
 * Data only: never place code, DMA descriptors or buffers that are checked
 * against the low SRAM window there. */
#if defined(MAIA_PRODUCT_HIGH_SRAM)
#define MAIA_SRAM_HIGH_BSS __attribute__((section(".maia_high_bss")))
#else
#define MAIA_SRAM_HIGH_BSS
#endif

#define MAIA_SRAM_HIGH_OK 0
#define MAIA_SRAM_HIGH_CACHE_SIZE (-1)
#define MAIA_SRAM_HIGH_VERIFY (-2)

/* Boot only (core 0, IRQs off, from start.S): checks that the L2 cache is
 * configured as 128 or 256 KiB so it cannot overlap 0x4FF40000..0x4FF80000,
 * then zeroes and verifies .maia_high_bss. Returns MAIA_SRAM_HIGH_*. */
int maia_esp32p4_sram_high_init(void);

/* Former 2nd-stage bootloader area, usable once the app runs. */
void maia_esp32p4_sram_reclaim_region(void **base, size_t *bytes);

/* High L2MEM heap window. */
void maia_esp32p4_sram_high_heap_region(void **base, size_t *bytes);

/* Writes `li a0,0x5A; ret` at base, syncs the I-cache and calls it.
 * Returns 1 when the region executes code (a trap is fatal). */
int maia_esp32p4_sram_exec_probe(void *base);

#ifdef __cplusplus
}
#endif
#endif
