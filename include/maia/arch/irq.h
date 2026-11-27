#ifndef MAIA_ARCH_IRQ_H
#define MAIA_ARCH_IRQ_H
#include <stdint.h>
/* Per-core kernel stacks: the SMP product puts them in high SRAM. */
#if defined(MAIA_SMP_HIGH_BSS)
#define MAIA_SMP_BSS __attribute__((section(".maia_high_bss")))
#else
#define MAIA_SMP_BSS
#endif
/* Kernel critical section. Tokens nest; restore only MIE.
 * Native: masks local interrupts and holds the global recursive kernel lock
 * (one owner hart; per-hart nesting depth), so every existing critical section
 * also excludes the other core. Never yield/ecall or wait for the other core
 * while holding it. This is not a DMA/cache barrier.
 * The *_local variants mask only this hart (no lock): use them only for
 * hart-private state.
 */
#if defined(__riscv)
#define MAIA_ARCH_MAX_HARTS 2U
#ifdef __cplusplus
extern "C" {
#endif
uintptr_t maia_arch_irq_save(void);
void maia_arch_irq_restore(uintptr_t previous);
/* Current hart's nesting depth (diagnostics/assertions). */
uint32_t maia_arch_klock_depth(void);
#ifdef __cplusplus
}
#endif
static inline uint32_t maia_arch_core_id(void) {
    uint32_t hart;
    __asm__ volatile("csrr %0, mhartid" : "=r"(hart));
    return hart;
}
static inline uintptr_t maia_arch_irq_save_local(void) {
    uintptr_t previous;
    __asm__ volatile("csrrci %0, mstatus, 8" : "=r"(previous) :: "memory");
    return previous & 8U;
}
static inline void maia_arch_irq_restore_local(uintptr_t previous) {
    if ((previous & 8U) != 0U) {
        __asm__ volatile("csrsi mstatus, 8" ::: "memory");
    } else {
        __asm__ volatile("csrci mstatus, 8" ::: "memory");
    }
}
#else
/* Hosted algorithm tests only; not a thread synchronization implementation. */
#ifdef __cplusplus
extern "C" {
#endif
uintptr_t maia_arch_irq_save(void);
void maia_arch_irq_restore(uintptr_t previous);
uint32_t maia_arch_core_id(void);
#ifdef __cplusplus
}
#endif
#define maia_arch_irq_save_local maia_arch_irq_save
#define maia_arch_irq_restore_local maia_arch_irq_restore
#endif
#endif
