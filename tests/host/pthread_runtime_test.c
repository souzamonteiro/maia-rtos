#include <maia/arch/irq.h>
#include <maia/arch/riscv_context.h>
#include <maia/kernel.h>
#include <maia/runtime/pthread.h>

#include <errno.h>
#include <pthread.h>
#include <setjmp.h>
#include <stdint.h>

#define TEST_STACK_CAPACITY 8192U

typedef struct test_task {
    maia_task_entry_fn entry;
    void *arg;
    void *stack;
    size_t stack_size;
    maia_task_id_t id;
    uint8_t executed;
} test_task_t;

static uintptr_t g_irq_state = 8U;
static maia_task_id_t g_current_task_id = 15;
static maia_task_id_t g_next_task_id = 2;
static test_task_t g_tasks[MAIA_MAX_TASKS];
static uint32_t g_task_count;
static maia_status_t g_task_create_status = MAIA_OK;
static void *g_stack_allocations[MAIA_MAX_TASKS];
static uint8_t g_stack_used[MAIA_MAX_TASKS];
static uint8_t g_stack_pool[MAIA_MAX_TASKS][TEST_STACK_CAPACITY]
    __attribute__((aligned(64)));
static uint32_t g_free_count;
static uint32_t g_notify_count;
static maia_task_id_t g_last_notified;
static jmp_buf g_exit_context;
static uint8_t g_exit_context_active;
static pthread_t g_entry_self;
static int g_self_join_result;

uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous = g_irq_state;
    g_irq_state = 0U;
    return previous;
}

void maia_arch_irq_restore(uintptr_t previous) {
    g_irq_state = previous & 8U;
}

void *maia_mem_alloc_aligned(size_t size, size_t alignment) {
    if (size == 0U || size > TEST_STACK_CAPACITY ||
        alignment == 0U || (alignment & (alignment - 1U)) != 0U) {
        return NULL;
    }
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_stack_used[i]) continue;
        if (((uintptr_t)g_stack_pool[i] & (alignment - 1U)) != 0U) {
            return NULL;
        }
        g_stack_used[i] = 1U;
        g_stack_allocations[i] = g_stack_pool[i];
        return g_stack_pool[i];
    }
    return NULL;
}

maia_status_t maia_mem_free(void *ptr) {
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_stack_used[i] && g_stack_allocations[i] == ptr) {
            g_stack_used[i] = 0U;
            ++g_free_count;
            return MAIA_OK;
        }
    }
    return MAIA_ERR_INVALID;
}

maia_status_t maia_task_create(const char *name, maia_task_entry_fn entry,
                               void *arg, uint8_t priority, void *stack_mem,
                               size_t stack_size, maia_task_id_t *out_id) {
    (void)name;
    (void)priority;
    if (g_task_create_status != MAIA_OK) return g_task_create_status;
    if (entry == NULL || stack_mem == NULL || out_id == NULL ||
        g_task_count >= MAIA_MAX_TASKS) {
        return MAIA_ERR_INVALID;
    }
    test_task_t *task = &g_tasks[g_task_count++];
    task->entry = entry;
    task->arg = arg;
    task->stack = stack_mem;
    task->stack_size = stack_size;
    task->id = g_next_task_id;
    task->executed = 0U;
    *out_id = task->id;
    return MAIA_OK;
}

maia_task_id_t maia_task_current_id(void) {
    return g_current_task_id;
}

maia_status_t maia_task_notify(maia_task_id_t task_id) {
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) {
        return MAIA_ERR_INVALID;
    }
    ++g_notify_count;
    g_last_notified = task_id;
    return MAIA_OK;
}

static int run_task(uint32_t index) {
    if (index >= g_task_count || g_tasks[index].executed ||
        g_exit_context_active) {
        return 0;
    }
    maia_task_id_t return_to = g_current_task_id;
    g_current_task_id = g_tasks[index].id;
    g_exit_context_active = 1U;
    if (setjmp(g_exit_context) == 0) {
        g_tasks[index].entry(g_tasks[index].arg);
        __builtin_trap();
    }
    g_exit_context_active = 0U;
    g_tasks[index].executed = 1U;
    g_current_task_id = return_to;
    return 1;
}

maia_status_t maia_task_wait_notification(uint32_t timeout_ticks) {
    if (timeout_ticks != MAIA_TASK_WAIT_FOREVER) return MAIA_ERR_INVALID;
    for (uint32_t i = 0U; i < g_task_count; ++i) {
        if (!g_tasks[i].executed) {
            return run_task(i) ? MAIA_OK : MAIA_ERR_INVALID;
        }
    }
    return MAIA_ERR_INVALID;
}

void maia_task_exit_current(void) {
    if (!g_exit_context_active) __builtin_trap();
    maia_pthread_task_exit_complete(g_current_task_id);
    longjmp(g_exit_context, 1);
}

static void *return_argument(void *arg) {
    g_entry_self = pthread_self();
    return (void *)((uintptr_t)arg + 0x10U);
}

static void *explicit_exit(void *arg) {
    g_entry_self = pthread_self();
    pthread_exit((void *)((uintptr_t)arg + 0x20U));
}

static void *self_join(void *arg) {
    g_self_join_result = pthread_join(pthread_self(), NULL);
    return arg;
}

static uint32_t used_stack_count(void) {
    uint32_t count = 0U;
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_stack_used[i]) ++count;
    }
    return count;
}

int main(void) {
    pthread_attr_t attr;
    size_t stack_size = 0U;
    int detach_state = -1;
    if (pthread_attr_init(&attr) != 0) return 101;
    if (pthread_attr_getstacksize(&attr, &stack_size) != 0) return 102;
    if (stack_size != 4096U) return 103;
    if (pthread_attr_getdetachstate(&attr, &detach_state) != 0) return 104;
    if (detach_state != PTHREAD_CREATE_JOINABLE) return 105;
    if (pthread_attr_setstacksize(&attr,
                                  MAIA_RISCV_MIN_STACK_BYTES - 1U) != EINVAL) {
        return 106;
    }
    if (pthread_attr_setguardsize(&attr, 1U) != ENOTSUP) return 107;

    pthread_t first = 0U;
    if (pthread_create(&first, NULL, return_argument, (void *)0x120U) != 0 ||
        first == 0U || pthread_equal(first, pthread_self()) ||
        g_task_count != 1U || g_tasks[0].stack_size != 4096U ||
        used_stack_count() != 1U) {
        return 2;
    }
    void *result = NULL;
    if (!run_task(0U) || used_stack_count() != 0U) {
        return 3;
    }

    pthread_t second = 0U;
    g_next_task_id = 2;
    if (pthread_create(&second, NULL, explicit_exit, (void *)0x200U) != 0 ||
        second == first || pthread_join(second, &result) != 0 ||
        result != (void *)0x220U || g_entry_self != second ||
        g_notify_count != 1U || g_last_notified != 15 ||
        used_stack_count() != 0U ||
        pthread_join(first, &result) != 0 || result != (void *)0x130U ||
        pthread_join(first, NULL) != ESRCH) {
        return 4;
    }

    pthread_t third = 0U;
    g_next_task_id = 3;
    if (pthread_create(&third, NULL, self_join, (void *)0x300U) != 0 ||
        pthread_join(third, &result) != 0 ||
        g_self_join_result != EDEADLK || result != (void *)0x300U) {
        return 5;
    }

    _Alignas(16) uint8_t external_stack[2048];
    if (pthread_attr_setstack(&attr, external_stack, sizeof(external_stack)) != 0 ||
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED) != 0) {
        return 6;
    }
    pthread_t detached = 0U;
    g_next_task_id = 4;
    if (pthread_create(&detached, &attr, return_argument, (void *)0x400U) != 0 ||
        g_tasks[3].stack != external_stack || used_stack_count() != 0U ||
        pthread_join(detached, NULL) != EINVAL || !run_task(3) ||
        pthread_join(detached, NULL) != ESRCH) {
        return 7;
    }

    if (pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE) != 0) {
        return 8;
    }
    pthread_t external_joinable = 0U;
    g_next_task_id = 5;
    if (pthread_create(&external_joinable, &attr, return_argument,
                       (void *)0x500U) != 0 ||
        pthread_detach(external_joinable) != 0 || !run_task(4) ||
        pthread_detach(external_joinable) != ESRCH || used_stack_count() != 0U) {
        return 9;
    }

    pthread_t failed = 0U;
    uint32_t frees_before_failure = g_free_count;
    g_task_create_status = MAIA_ERR_NOMEM;
    g_next_task_id = 6;
    if (pthread_create(&failed, NULL, return_argument, NULL) != EAGAIN ||
        used_stack_count() != 0U || g_free_count != frees_before_failure + 1U) {
        return 10;
    }
    g_task_create_status = MAIA_OK;
    if (pthread_attr_destroy(&attr) != 0 ||
        pthread_attr_getstacksize(&attr, &stack_size) != EINVAL ||
        g_irq_state != 8U) {
        return 11;
    }
    return 0;
}
