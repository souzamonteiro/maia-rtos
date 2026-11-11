#ifndef MAIA_ARCH_IRQ_H
#define MAIA_ARCH_IRQ_H
#include <stdint.h>
/* Private single-core interrupt exclusion. Tokens nest; restore only MIE.
 * Compiler memory barriers order scheduler memory around the local CSR mask.
 * This is not an SMP lock, DMA/cache barrier or inter-hart memory ordering.
 */
#if defined(__riscv)
static inline uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous;
    __asm__ volatile("csrrci %0, mstatus, 8" : "=r"(previous) :: "memory");
    return previous & 8U;
}
static inline void maia_arch_irq_restore(uintptr_t previous) {
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
#ifdef __cplusplus
}
#endif
#endif
#endif
