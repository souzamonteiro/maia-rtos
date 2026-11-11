#include <maia/arch/riscv_stack.h>
#include <stddef.h>

_Static_assert(sizeof(uintptr_t) == 4, "RV32 stack port");
_Static_assert(sizeof(maia_trap_stack_metadata_t) == 48, "scratch layout");
#define OFFSET(field, value) _Static_assert(offsetof(maia_trap_stack_metadata_t, field) == value, "scratch offset")
OFFSET(saved_t0,MAIA_STACK_T0); OFFSET(saved_t1,MAIA_STACK_T1);
OFFSET(source_sp,MAIA_STACK_SOURCE_SP); OFFSET(low,MAIA_STACK_LOW); OFFSET(high,MAIA_STACK_HIGH);
OFFSET(active,MAIA_STACK_ACTIVE); OFFSET(emergency_top,MAIA_STACK_EMERGENCY_TOP);
OFFSET(irq_top,MAIA_STACK_IRQ_TOP); OFFSET(saved_t2,MAIA_STACK_T2);
OFFSET(cause,MAIA_STACK_CAUSE); OFFSET(pc,MAIA_STACK_PC); OFFSET(value,MAIA_STACK_VALUE);

volatile maia_trap_stack_metadata_t maia_trap_stack_metadata __attribute__((aligned(16)));
static uint8_t irq_stack[MAIA_IRQ_STACK_BYTES] __attribute__((aligned(16)));
static uint8_t emergency_stack[MAIA_EMERGENCY_STACK_BYTES] __attribute__((aligned(16)));
extern unsigned char __stack_bottom[], __stack_top[];
static int initialized;

int maia_arch_stack_reserved(uintptr_t base, size_t size) {
    return maia_stack_overlap(base,size,(uintptr_t)irq_stack,sizeof irq_stack) ||
           maia_stack_overlap(base,size,(uintptr_t)emergency_stack,sizeof emergency_stack) ||
           maia_stack_overlap(base,size,(uintptr_t)__stack_bottom,(size_t)((uintptr_t)__stack_top-(uintptr_t)__stack_bottom));
}
void maia_arch_stack_publish(uintptr_t low, uintptr_t high) {
    maia_trap_stack_metadata.low = (uint32_t)low;
    maia_trap_stack_metadata.high = (uint32_t)high;
    __asm__ volatile("fence rw, rw" ::: "memory");
}
void maia_arch_trap_stack_init(void) {
    if (initialized) return;
    maia_stack_guard_init((uintptr_t)__stack_bottom);
    maia_stack_guard_init((uintptr_t)irq_stack);
    maia_stack_guard_init((uintptr_t)emergency_stack);
    maia_trap_stack_metadata.low = (uint32_t)maia_stack_low((uintptr_t)__stack_bottom);
    maia_trap_stack_metadata.high = (uint32_t)(uintptr_t)__stack_top;
    maia_trap_stack_metadata.irq_top = (uint32_t)((uintptr_t)irq_stack+sizeof irq_stack);
    maia_trap_stack_metadata.emergency_top = (uint32_t)((uintptr_t)emergency_stack+sizeof emergency_stack);
    maia_trap_stack_metadata.active = 0U;
    __asm__ volatile("fence rw, rw" ::: "memory");
    __asm__ volatile("csrw mscratch, %0" :: "r"(&maia_trap_stack_metadata) : "memory");
    initialized = 1;
}
