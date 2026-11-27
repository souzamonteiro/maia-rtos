#ifndef MAIA_SOC_ESP32P4_SMP_H
#define MAIA_SOC_ESP32P4_SMP_H

#include <maia/kernel.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint32_t online;       /* Core 1 runs the scheduler. */
    uint32_t ipi[2];       /* IPIs taken per core. */
    uint32_t freezes;      /* Core 1 flash-freeze rounds. */
    uint32_t migrations;   /* Core-1 callers moved to core 0. */
} maia_esp32p4_smp_stats_t;

/* Boot, core 0, IRQs off, right after the systimer: claims CLIC ID 17 for the
 * core-1 -> core-0 IPI (before any dynamic IRQ allocation). */
void maia_esp32p4_smp_init_core0(void);
/* Boot, core 0, after maia_esp32p4_core1_start succeeded and right before
 * maia_sched_start: hands core 1 to the scheduler. Returns MAIA_CORE1_*. */
int maia_esp32p4_smp_launch_core1(void);
int maia_esp32p4_smp_core1_online(void);
void maia_esp32p4_smp_get_stats(maia_esp32p4_smp_stats_t *out);

/* Trap handler: services IPI line 17 on the current core; returns nonzero
 * when it was an IPI (a reschedule point). Called before taking the lock. */
int maia_esp32p4_smp_ipi_isr(void);

/* Thread context, IRQs enabled: runs the caller on core 0 until _end.
 * Returns 0 and fills *saved on success (no-op when already on core 0). */
int maia_esp32p4_smp_core0_begin(uint8_t *saved);
void maia_esp32p4_smp_core0_end(uint8_t saved);

/* Core 0, local IRQs off, kernel lock NOT held: parks core 1 in SRAM so the
 * L2 cache can be suspended (flash). Returns 0 on success (or no core 1). */
int maia_esp32p4_smp_freeze_core1(void);
void maia_esp32p4_smp_thaw_core1(void);

#ifdef __cplusplus
}
#endif
#endif
