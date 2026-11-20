#include <maia/arch/irq.h>
#include <maia/kernel.h>
#include <maia/runtime/tls.h>

#include <stdint.h>

static uintptr_t irq_state = 8U;
static uint8_t allocations[MAIA_MAX_TASKS][1024U] __attribute__((aligned(64)));
static uint8_t allocation_used[MAIA_MAX_TASKS];

uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous = irq_state;
    irq_state = 0U;
    return previous;
}

void maia_arch_irq_restore(uintptr_t previous) {
    irq_state = previous & 8U;
}

void *maia_mem_alloc_aligned(size_t size, size_t alignment) {
    if (size == 0U || size > sizeof(allocations[0]) ||
        alignment == 0U || (alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (allocation_used[i]) continue;
        if (((uintptr_t)allocations[i] & (alignment - 1U)) != 0U) return NULL;
        allocation_used[i] = 1U;
        return allocations[i];
    }
    return NULL;
}

maia_status_t maia_mem_free(void *ptr) {
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (ptr == allocations[i] && allocation_used[i]) {
            allocation_used[i] = 0U;
            return MAIA_OK;
        }
    }
    return MAIA_ERR_INVALID;
}

static int bytes_are(const uint8_t *bytes, uint8_t value, size_t size) {
    for (size_t i = 0U; i < size; ++i) {
        if (bytes[i] != value) return 0;
    }
    return 1;
}

int main(void) {
    static const uint8_t initial_data[] = {0x78U, 0x56U, 0x34U, 0x12U};
    uint8_t template_data[sizeof(initial_data)];
    for (size_t i = 0U; i < sizeof(initial_data); ++i) {
        template_data[i] = initial_data[i];
    }
    maia_tls_test_configure(template_data, sizeof(template_data), 80U, 64U);
    if (maia_tls_boot_init() != 0) return 1;

    uint8_t *boot_tp = (uint8_t *)maia_tls_test_boot_thread_pointer();
    uint8_t *idle_tp = (uint8_t *)maia_tls_idle_thread_pointer();
    if (boot_tp == NULL || idle_tp == NULL || boot_tp == idle_tp ||
        ((uintptr_t)boot_tp & 63U) != 0U || ((uintptr_t)idle_tp & 63U) != 0U ||
        boot_tp[0] != initial_data[0] || boot_tp[3] != initial_data[3] ||
        idle_tp[0] != initial_data[0] || idle_tp[3] != initial_data[3] ||
        !bytes_are(boot_tp + sizeof(initial_data), 0U,
                   80U - sizeof(initial_data)) ||
        !bytes_are(idle_tp + sizeof(initial_data), 0U,
                   80U - sizeof(initial_data))) return 2;

    template_data[0] = 0U;
    uint8_t *first = (uint8_t *)maia_tls_task_create(2);
    uint8_t *second = (uint8_t *)maia_tls_task_create(3);
    if (first == NULL || second == NULL || first == second ||
        ((uintptr_t)first & 63U) != 0U || ((uintptr_t)second & 63U) != 0U ||
        first[0] != initial_data[0] || second[0] != initial_data[0] ||
        !bytes_are(first + sizeof(initial_data), 0U, 76U) ||
        !bytes_are(second + sizeof(initial_data), 0U, 76U)) return 3;

    first[0] = 0xA5U;
    if (second[0] != initial_data[0]) return 4;
    maia_tls_task_reclaim(2);
    uint8_t *reused = (uint8_t *)maia_tls_task_create(2);
    if (reused != first || reused[0] != initial_data[0] ||
        !bytes_are(reused + sizeof(initial_data), 0U, 76U)) return 5;

    maia_tls_task_reclaim(2);
    maia_tls_task_reclaim(3);
    if (maia_tls_task_create(-1) != MAIA_TLS_TASK_CREATE_FAILED ||
        irq_state != 8U) return 6;
    return 0;
}
