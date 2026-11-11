#ifndef MAIA_ARCH_RISCV_STACK_H
#define MAIA_ARCH_RISCV_STACK_H
#include <maia/arch/riscv_context.h>
#define MAIA_STACK_T0 0
#define MAIA_STACK_T1 4
#define MAIA_STACK_SOURCE_SP 8
#define MAIA_STACK_LOW 12
#define MAIA_STACK_HIGH 16
#define MAIA_STACK_ACTIVE 20
#define MAIA_STACK_EMERGENCY_TOP 24
#define MAIA_STACK_IRQ_TOP 28
#define MAIA_STACK_T2 32
#define MAIA_STACK_CAUSE 36
#define MAIA_STACK_PC 40
#define MAIA_STACK_VALUE 44
#define MAIA_IRQ_STACK_BYTES 4096
#define MAIA_EMERGENCY_STACK_BYTES 2048
#define MAIA_STACK_BAD_SP 1
#define MAIA_STACK_BAD_GUARD 2
#define MAIA_STACK_BAD_CONTEXT 3
#define MAIA_STACK_NESTED 4
#define MAIA_STACK_BAD_IRQ_GUARD 5

#ifndef __ASSEMBLER__
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Private RV32 port metadata. mscratch always points to this once initialized. */
typedef struct maia_trap_stack_metadata {
    uint32_t saved_t0, saved_t1, source_sp, low, high, active;
    uint32_t emergency_top, irq_top, saved_t2, cause, pc, value;
} maia_trap_stack_metadata_t;
extern volatile maia_trap_stack_metadata_t maia_trap_stack_metadata;
void maia_arch_trap_stack_init(void); /* Boot-only, IRQs disabled. */
void maia_arch_stack_publish(uintptr_t low, uintptr_t high); /* IRQs disabled. */
int maia_arch_stack_reserved(uintptr_t base, size_t size);
__attribute__((noreturn)) void maia_arch_stack_panic(uint32_t cause, uint32_t pc,
    uint32_t value, uintptr_t source_sp, uint32_t reason);
__attribute__((noreturn)) void maia_arch_stack_fault(uint32_t cause, uint32_t pc,
    uint32_t value, uintptr_t source_sp, uint32_t reason);
#ifdef __cplusplus
}
#endif

/* Caller supplies a valid owned writable buffer; no MMU/security boundary. */
static inline uintptr_t maia_stack_low(uintptr_t base) {
    return ((base + 15U) & ~(uintptr_t)15U) + MAIA_RISCV_STACK_GUARD_BYTES;
}
static inline uintptr_t maia_stack_top(uintptr_t base, size_t size) {
    return (base + size) & ~(uintptr_t)15U;
}
static inline void maia_stack_guard_init(uintptr_t base) {
    volatile uint32_t *guard = (volatile uint32_t *)(maia_stack_low(base) - MAIA_RISCV_STACK_GUARD_BYTES);
    for (unsigned i=0U; i<4U; ++i) guard[i] = MAIA_RISCV_STACK_GUARD_WORD;
}
static inline int maia_stack_guard_valid(uintptr_t base) {
    const volatile uint32_t *guard = (const volatile uint32_t *)(maia_stack_low(base) - MAIA_RISCV_STACK_GUARD_BYTES);
    for (unsigned i=0U; i<4U; ++i) if (guard[i] != MAIA_RISCV_STACK_GUARD_WORD) return 0;
    return 1;
}
static inline int maia_stack_frame_valid(uintptr_t base, size_t size, uintptr_t sp) {
    if (!base || size < MAIA_RISCV_MIN_STACK_BYTES || size > UINTPTR_MAX-base || (sp & 15U)) return 0;
    uintptr_t low = maia_stack_low(base), top = maia_stack_top(base,size);
    return sp >= low && sp <= top && top-sp >= MAIA_RISCV_CONTEXT_BYTES && maia_stack_guard_valid(base);
}
static inline int maia_stack_overlap(uintptr_t a, size_t na, uintptr_t b, size_t nb) {
    if (na > UINTPTR_MAX-a || nb > UINTPTR_MAX-b) return 1;
    return a < b+nb && b < a+na;
}
#endif
#endif
