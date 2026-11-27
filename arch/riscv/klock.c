/* Global recursive kernel lock behind maia_arch_irq_save/restore (SMP). */
#include <maia/arch/irq.h>

static volatile uint32_t klock_word;
static uint32_t klock_depth[MAIA_ARCH_MAX_HARTS];

uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous = maia_arch_irq_save_local();
    uint32_t hart = maia_arch_core_id() & (MAIA_ARCH_MAX_HARTS - 1U);
    if (klock_depth[hart]++ == 0U) {
        while (__atomic_exchange_n(&klock_word, 1U, __ATOMIC_ACQUIRE) != 0U) {
            while (__atomic_load_n(&klock_word, __ATOMIC_RELAXED) != 0U) {
            }
        }
    }
    return previous;
}

void maia_arch_irq_restore(uintptr_t previous) {
    uint32_t hart = maia_arch_core_id() & (MAIA_ARCH_MAX_HARTS - 1U);
    if (klock_depth[hart] != 0U && --klock_depth[hart] == 0U) {
        __atomic_store_n(&klock_word, 0U, __ATOMIC_RELEASE);
    }
    maia_arch_irq_restore_local(previous);
}

uint32_t maia_arch_klock_depth(void) {
    return klock_depth[maia_arch_core_id() & (MAIA_ARCH_MAX_HARTS - 1U)];
}
