#ifndef MAIA_ARCH_RISCV_TRAP_POLICY_H
#define MAIA_ARCH_RISCV_TRAP_POLICY_H
#include <stdint.h>
#include <stddef.h>

/* Private pure policy: only an interrupt-enabled M-mode ECALL is a yield.
 * Reject hardware-vector-fetch state, other causes and PC overflow/alignment.
 * On failure leave output untouched. Task-lifecycle checks belong to dispatch.
 */
static inline int maia_riscv_yield_resume_pc(uint32_t cause, uint32_t pc,
                                           uint32_t *resume_pc) {
    if (resume_pc == NULL || (cause & 0xC0000FFFU) != 11U ||
        (cause & 0x38000000U) != 0x38000000U ||
        (pc & 1U) != 0U || pc > 0xFFFFFFFBU) return 0;
    *resume_pc = pc + 4U; /* ECALL is always 32 bits, even with the C extension. */
    return 1;
}
#endif
