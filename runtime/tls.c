#include <maia/arch/irq.h>
#include <maia/kernel.h>
#include <maia/runtime/tls.h>

#include <stddef.h>
#include <stdint.h>

#define MAIA_TLS_TEMPLATE_CAPACITY 1024U

static uint8_t g_tls_template[MAIA_TLS_TEMPLATE_CAPACITY];
static uint8_t g_boot_tls[MAIA_TLS_TEMPLATE_CAPACITY] __attribute__((aligned(64)));
static uint8_t g_idle_tls[MAIA_TLS_TEMPLATE_CAPACITY] __attribute__((aligned(64)));
static void *g_task_tls[MAIA_MAX_TASKS];
static size_t g_tdata_size;
static size_t g_tls_block_size;
static size_t g_tls_alignment;
static uintptr_t g_boot_tp;
static uintptr_t g_idle_tp;
static uint8_t g_tls_ready;

#if defined(MAIA_COMPILER_TLS_TEST)
static const uint8_t *g_test_tdata;
static size_t g_test_tdata_size;
static size_t g_test_block_size;
static size_t g_test_alignment;

void maia_tls_test_configure(const uint8_t *tdata, size_t tdata_size,
                             size_t block_size, size_t alignment) {
    g_test_tdata = tdata;
    g_test_tdata_size = tdata_size;
    g_test_block_size = block_size;
    g_test_alignment = alignment;
}

uintptr_t maia_tls_test_boot_thread_pointer(void) {
    return g_boot_tp;
}
#else
extern uint8_t __maia_tls_tdata_start[];
extern uint8_t __maia_tls_tdata_end[];
extern uint8_t __maia_tls_block_end[];
extern uint8_t __maia_tls_alignment;
#endif

int maia_tls_boot_init(void) {
    const uint8_t *tdata;
    size_t tdata_size;
    size_t block_size;
    size_t alignment;
#if defined(MAIA_COMPILER_TLS_TEST)
    tdata = g_test_tdata;
    tdata_size = g_test_tdata_size;
    block_size = g_test_block_size;
    alignment = g_test_alignment;
#else
    uintptr_t tdata_start = (uintptr_t)__maia_tls_tdata_start;
    uintptr_t tdata_end = (uintptr_t)__maia_tls_tdata_end;
    uintptr_t block_end = (uintptr_t)__maia_tls_block_end;
    if (tdata_end < tdata_start || block_end < tdata_start) return -1;
    tdata = __maia_tls_tdata_start;
    tdata_size = (size_t)(tdata_end - tdata_start);
    block_size = (size_t)(block_end - tdata_start);
    alignment = (uintptr_t)&__maia_tls_alignment;
#endif
    if ((tdata_size != 0U && tdata == NULL) ||
        tdata_size > MAIA_TLS_TEMPLATE_CAPACITY ||
        block_size < tdata_size || block_size > MAIA_TLS_TEMPLATE_CAPACITY ||
        alignment == 0U || (alignment & (alignment - 1U)) != 0U ||
        alignment > 64U) {
        return -1;
    }

    g_tdata_size = tdata_size;
    g_tls_block_size = block_size;
    g_tls_alignment = alignment;
    for (size_t i = 0U; i < block_size; ++i) {
        g_boot_tls[i] = 0U;
        g_idle_tls[i] = 0U;
    }
    for (size_t i = 0U; i < tdata_size; ++i) {
        g_tls_template[i] = tdata[i];
        g_boot_tls[i] = tdata[i];
        g_idle_tls[i] = tdata[i];
    }

    g_boot_tp = block_size != 0U ? (uintptr_t)g_boot_tls : 0U;
    g_idle_tp = block_size != 0U ? (uintptr_t)g_idle_tls : 0U;
#if defined(__riscv)
    __asm__ volatile("mv tp, %0" : : "r"(g_boot_tp) : "memory");
#endif
    g_tls_ready = 1U;
    return 0;
}

uintptr_t maia_tls_idle_thread_pointer(void) {
    return g_tls_ready ? g_idle_tp : 0U;
}

uintptr_t maia_tls_task_create(maia_task_id_t task_id) {
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) {
        return MAIA_TLS_TASK_CREATE_FAILED;
    }
    if (!g_tls_ready) return MAIA_TLS_TASK_CREATE_FAILED;
    if (g_tls_block_size == 0U) return 0U;

    uintptr_t irq = maia_arch_irq_save();
    if (g_task_tls[task_id] != NULL) {
        maia_arch_irq_restore(irq);
        return MAIA_TLS_TASK_CREATE_FAILED;
    }
    uint8_t *thread_tls = (uint8_t *)maia_mem_alloc_aligned(
        g_tls_block_size, g_tls_alignment);
    if (thread_tls == NULL) {
        maia_arch_irq_restore(irq);
        return MAIA_TLS_TASK_CREATE_FAILED;
    }
    for (size_t i = 0U; i < g_tls_block_size; ++i) thread_tls[i] = 0U;
    for (size_t i = 0U; i < g_tdata_size; ++i) {
        thread_tls[i] = g_tls_template[i];
    }
    g_task_tls[task_id] = thread_tls;
    maia_arch_irq_restore(irq);
    return (uintptr_t)thread_tls;
}

void maia_tls_task_reclaim(maia_task_id_t task_id) {
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) return;
    uintptr_t irq = maia_arch_irq_save();
    void *thread_tls = g_task_tls[task_id];
    g_task_tls[task_id] = NULL;
    maia_status_t status = MAIA_OK;
    if (thread_tls != NULL) {
        uint8_t *bytes = (uint8_t *)thread_tls;
        for (size_t i = 0U; i < g_tls_block_size; ++i) bytes[i] = 0U;
        status = maia_mem_free(thread_tls);
    }
    maia_arch_irq_restore(irq);
    if (status != MAIA_OK) __builtin_trap();
}
