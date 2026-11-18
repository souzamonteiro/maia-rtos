#include <maia/kernel.h>
#include <maia/runtime/newlib_heap.h>
#include <maia/runtime/newlib_locks.h>

#include <stdint.h>

static uint8_t heap[8192] __attribute__((aligned(16)));
static maia_task_id_t current_task = 1;
static uintptr_t irq_state = 8U;

uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous = irq_state;
    irq_state = 0U;
    return previous;
}

void maia_arch_irq_restore(uintptr_t previous) {
    irq_state = previous & 8U;
}

maia_task_id_t maia_task_current_id(void) {
    return current_task;
}

void maia_task_yield(void) {
}

int main(void) {
    if (maia_mem_init(heap, sizeof heap) != MAIA_OK ||
        maia_newlib_locks_init() != MAIA_OK) return 1;
    if (maia_newlib_heap_alloc(0U) != NULL ||
        maia_newlib_heap_calloc(0U, 16U) != NULL) return 2;

    uint8_t *original = (uint8_t *)maia_newlib_heap_alloc(17U);
    if (original == NULL || maia_newlib_heap_usable_size(original) != 24U) return 3;
    for (size_t i = 0U; i < 17U; ++i) original[i] = (uint8_t)(i + 1U);

    uint8_t *zeroed = (uint8_t *)maia_newlib_heap_calloc(8U, 4U);
    if (zeroed == NULL || maia_newlib_heap_usable_size(zeroed) != 32U) return 4;
    for (size_t i = 0U; i < 32U; ++i) {
        if (zeroed[i] != 0U) return 5;
    }
    if (maia_newlib_heap_calloc(SIZE_MAX, 2U) != NULL) return 6;

    uint8_t *grown = (uint8_t *)maia_newlib_heap_realloc(original, 64U);
    if (grown == NULL || maia_newlib_heap_usable_size(grown) != 64U) return 7;
    for (size_t i = 0U; i < 17U; ++i) {
        if (grown[i] != (uint8_t)(i + 1U)) return 8;
    }

    if (maia_newlib_heap_realloc(grown, SIZE_MAX) != NULL) return 9;
    for (size_t i = 0U; i < 17U; ++i) {
        if (grown[i] != (uint8_t)(i + 1U)) return 10;
    }
    uint8_t *fresh = (uint8_t *)maia_newlib_heap_realloc(NULL, 12U);
    if (fresh == NULL) return 11;
    if (maia_newlib_heap_realloc(grown, 0U) != NULL) return 12;

    maia_newlib_heap_free(zeroed);
    maia_newlib_heap_free(fresh);
    maia_newlib_heap_free(NULL);
    maia_mem_stats_t stats;
    if (maia_mem_get_stats(&stats) != MAIA_OK || stats.used_bytes != 0U) return 13;
    return 0;
}
