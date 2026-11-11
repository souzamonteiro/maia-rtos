#ifndef MAIA_SCHEDULER_INTERNAL_H
#define MAIA_SCHEDULER_INTERNAL_H
#include <maia/kernel.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Private port entry points. Native dispatch calls with interrupts disabled.
 * Host models call these instead of executing RISC-V traps.
 */
maia_status_t maia_sched_delay_current_isr(uint32_t ticks);
maia_status_t maia_sched_exit_current_isr(void);
void maia_arch_delay_trap(uint32_t ticks);
__attribute__((noreturn)) void maia_arch_exit_trap(void);
#if defined(__riscv)
extern const unsigned char maia_arch_delay_ecall[];
extern const unsigned char maia_arch_exit_ecall[];
#endif
#ifdef MAIA_SCHED_TEST
void maia_sched_test_set_tick(uint64_t tick);
#endif
#ifdef __cplusplus
}
#endif
#endif
