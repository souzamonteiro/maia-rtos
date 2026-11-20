#include <maia/arch/irq.h>
#include <maia/arch/riscv_context.h>
#include <maia/kernel.h>
#include <maia/runtime/pthread.h>

#include <errno.h>
#include <limits.h>
#include <pthread.h>
#include <stdint.h>

#define MAIA_PTHREAD_DEFAULT_STACK_SIZE 4096U
#define MAIA_PTHREAD_HANDLE_MASK 0x7FFFFFFFU
#define MAIA_PTHREAD_EXTERNAL_ID 0x80000000U
#define MAIA_PTHREAD_NO_TASK ((maia_task_id_t)-1)

typedef enum maia_pthread_state {
    MAIA_PTHREAD_FREE = 0,
    MAIA_PTHREAD_STARTING,
    MAIA_PTHREAD_RUNNING,
    MAIA_PTHREAD_EXITED
} maia_pthread_state_t;

typedef struct maia_pthread_record {
    pthread_t handle;
    maia_task_id_t task_id;
    maia_pthread_state_t state;
    void *(*start_routine)(void *);
    void *arg;
    void *return_value;
    void *stack_base;
    size_t stack_size;
    maia_task_id_t joiner_task_id;
    uint8_t owns_stack;
    uint8_t detached;
} maia_pthread_record_t;

static maia_pthread_record_t g_threads[MAIA_MAX_TASKS];
static uint32_t g_next_handle = 1U;

static int attr_is_valid(const pthread_attr_t *attr) {
    return attr != NULL && attr->is_initialized == 1;
}

static maia_pthread_record_t *find_handle_locked(pthread_t handle) {
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_threads[i].state != MAIA_PTHREAD_FREE &&
            g_threads[i].handle == handle) {
            return &g_threads[i];
        }
    }
    return NULL;
}

static maia_pthread_record_t *find_task_locked(maia_task_id_t task_id) {
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if ((g_threads[i].state == MAIA_PTHREAD_STARTING ||
             g_threads[i].state == MAIA_PTHREAD_RUNNING) &&
            g_threads[i].task_id == task_id) {
            return &g_threads[i];
        }
    }
    return NULL;
}

static pthread_t allocate_handle_locked(void) {
    for (uint32_t attempt = 0U; attempt <= MAIA_MAX_TASKS; ++attempt) {
        uint32_t candidate = g_next_handle++ & MAIA_PTHREAD_HANDLE_MASK;
        if (candidate == 0U) {
            candidate = g_next_handle++ & MAIA_PTHREAD_HANDLE_MASK;
        }
        if (candidate != 0U &&
            find_handle_locked((pthread_t)candidate) == NULL) {
            return (pthread_t)candidate;
        }
    }
    return (pthread_t)0U;
}

static void clear_record_locked(maia_pthread_record_t *record) {
    record->handle = (pthread_t)0U;
    record->task_id = MAIA_PTHREAD_NO_TASK;
    record->state = MAIA_PTHREAD_FREE;
    record->start_routine = NULL;
    record->arg = NULL;
    record->return_value = NULL;
    record->stack_base = NULL;
    record->stack_size = 0U;
    record->joiner_task_id = MAIA_PTHREAD_NO_TASK;
    record->owns_stack = 0U;
    record->detached = 0U;
}

static int task_status_to_error(maia_status_t status) {
    if (status == MAIA_ERR_INVALID) return EINVAL;
    if (status == MAIA_ERR_PERM) return EPERM;
    return EAGAIN;
}

static void pthread_task_entry(void *arg) {
    maia_pthread_record_t *record = (maia_pthread_record_t *)arg;
    maia_task_id_t task_id = maia_task_current_id();
    uintptr_t irq = maia_arch_irq_save();
    if (record->state != MAIA_PTHREAD_STARTING || task_id < 0 ||
        (uint32_t)task_id >= MAIA_MAX_TASKS) {
        maia_arch_irq_restore(irq);
        maia_task_exit_current();
    }
    record->task_id = task_id;
    record->state = MAIA_PTHREAD_RUNNING;
    void *(*start_routine)(void *) = record->start_routine;
    void *start_arg = record->arg;
    maia_arch_irq_restore(irq);

    pthread_exit(start_routine(start_arg));
}

int pthread_attr_init(pthread_attr_t *attr) {
    if (attr == NULL) return EINVAL;
    pthread_attr_t value = {0};
    value.is_initialized = 1;
    value.stacksize = (int)MAIA_PTHREAD_DEFAULT_STACK_SIZE;
    value.detachstate = PTHREAD_CREATE_JOINABLE;
    *attr = value;
    return 0;
}

int pthread_attr_destroy(pthread_attr_t *attr) {
    if (!attr_is_valid(attr)) return EINVAL;
    attr->is_initialized = 0;
    return 0;
}

int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stack_size) {
    if (!attr_is_valid(attr) || stack_size < MAIA_RISCV_MIN_STACK_BYTES ||
        stack_size > INT_MAX) {
        return EINVAL;
    }
    attr->stacksize = (int)stack_size;
    return 0;
}

int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stack_size) {
    if (!attr_is_valid(attr) || stack_size == NULL || attr->stacksize <= 0) {
        return EINVAL;
    }
    *stack_size = (size_t)attr->stacksize;
    return 0;
}

int pthread_attr_setstack(pthread_attr_t *attr, void *stack, size_t stack_size) {
    if (!attr_is_valid(attr) || stack == NULL ||
        ((uintptr_t)stack & 15U) != 0U ||
        stack_size < MAIA_RISCV_MIN_STACK_BYTES || stack_size > INT_MAX ||
        stack_size > UINTPTR_MAX - (uintptr_t)stack) {
        return EINVAL;
    }
    attr->stackaddr = stack;
    attr->stacksize = (int)stack_size;
    return 0;
}

int pthread_attr_getstack(const pthread_attr_t *attr, void **stack,
                          size_t *stack_size) {
    if (!attr_is_valid(attr) || stack == NULL || stack_size == NULL ||
        attr->stacksize <= 0) {
        return EINVAL;
    }
    *stack = attr->stackaddr;
    *stack_size = (size_t)attr->stacksize;
    return 0;
}

int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stack) {
    if (!attr_is_valid(attr) || stack == NULL ||
        ((uintptr_t)stack & 15U) != 0U ||
        attr->stacksize < (int)MAIA_RISCV_MIN_STACK_BYTES ||
        (size_t)attr->stacksize > UINTPTR_MAX - (uintptr_t)stack) {
        return EINVAL;
    }
    attr->stackaddr = stack;
    return 0;
}

int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stack) {
    if (!attr_is_valid(attr) || stack == NULL) return EINVAL;
    *stack = attr->stackaddr;
    return 0;
}

int pthread_attr_setdetachstate(pthread_attr_t *attr, int detach_state) {
    if (!attr_is_valid(attr) ||
        (detach_state != PTHREAD_CREATE_JOINABLE &&
         detach_state != PTHREAD_CREATE_DETACHED)) {
        return EINVAL;
    }
    attr->detachstate = detach_state;
    return 0;
}

int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detach_state) {
    if (!attr_is_valid(attr) || detach_state == NULL) return EINVAL;
    *detach_state = attr->detachstate;
    return 0;
}

int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guard_size) {
    if (!attr_is_valid(attr)) return EINVAL;
    return guard_size == 0U ? 0 : ENOTSUP;
}

int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guard_size) {
    if (!attr_is_valid(attr) || guard_size == NULL) return EINVAL;
    *guard_size = 0U;
    return 0;
}

int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg) {
    if (thread == NULL || start_routine == NULL) return EINVAL;
    if (attr != NULL && (!attr_is_valid(attr) ||
        (attr->detachstate != PTHREAD_CREATE_JOINABLE &&
         attr->detachstate != PTHREAD_CREATE_DETACHED))) {
        return EINVAL;
    }

    size_t stack_size = attr == NULL
        ? MAIA_PTHREAD_DEFAULT_STACK_SIZE : (size_t)attr->stacksize;
    void *stack_base = attr == NULL ? NULL : attr->stackaddr;
    if (stack_size < MAIA_RISCV_MIN_STACK_BYTES || stack_size > INT_MAX ||
        (stack_base != NULL &&
         ((((uintptr_t)stack_base & 15U) != 0U) ||
          stack_size > UINTPTR_MAX - (uintptr_t)stack_base))) {
        return EINVAL;
    }

    uintptr_t irq = maia_arch_irq_save();
    maia_pthread_record_t *record = NULL;
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_threads[i].state == MAIA_PTHREAD_FREE) {
            record = &g_threads[i];
            break;
        }
    }
    pthread_t handle = record != NULL ? allocate_handle_locked() : (pthread_t)0U;
    if (record == NULL || handle == (pthread_t)0U) {
        maia_arch_irq_restore(irq);
        return EAGAIN;
    }
    if (stack_base == NULL) {
        stack_base = maia_mem_alloc_aligned(stack_size, 16U);
        if (stack_base == NULL) {
            maia_arch_irq_restore(irq);
            return EAGAIN;
        }
    }

    clear_record_locked(record);
    record->handle = handle;
    record->state = MAIA_PTHREAD_STARTING;
    record->start_routine = start_routine;
    record->arg = arg;
    record->stack_base = stack_base;
    record->stack_size = stack_size;
    record->joiner_task_id = MAIA_PTHREAD_NO_TASK;
    record->owns_stack = (attr == NULL || attr->stackaddr == NULL) ? 1U : 0U;
    record->detached = attr != NULL &&
        attr->detachstate == PTHREAD_CREATE_DETACHED;
    maia_arch_irq_restore(irq);

    maia_task_id_t task_id = MAIA_PTHREAD_NO_TASK;
    maia_status_t status = maia_task_create(
        "std_thread", pthread_task_entry, record, MAIA_PRIO_NORMAL,
        stack_base, stack_size, &task_id);
    if (status != MAIA_OK) {
        irq = maia_arch_irq_save();
        void *owned_stack = NULL;
        if (record->state == MAIA_PTHREAD_STARTING &&
            record->handle == handle) {
            if (record->owns_stack) owned_stack = record->stack_base;
            clear_record_locked(record);
            if (owned_stack != NULL && maia_mem_free(owned_stack) != MAIA_OK) {
                maia_arch_irq_restore(irq);
                __builtin_trap();
            }
        }
        maia_arch_irq_restore(irq);
        return task_status_to_error(status);
    }

    *thread = handle;
    return 0;
}

pthread_t pthread_self(void) {
    maia_task_id_t task_id = maia_task_current_id();
    if (task_id < 0) return (pthread_t)UINT32_MAX;
    uintptr_t irq = maia_arch_irq_save();
    maia_pthread_record_t *record = find_task_locked(task_id);
    pthread_t handle = record != NULL
        ? record->handle
        : (pthread_t)(MAIA_PTHREAD_EXTERNAL_ID | ((uint32_t)task_id + 1U));
    maia_arch_irq_restore(irq);
    return handle;
}

int pthread_equal(pthread_t first, pthread_t second) {
    return first == second;
}

int pthread_join(pthread_t thread, void **return_value) {
    maia_task_id_t self_task = maia_task_current_id();
    if (thread == pthread_self()) return EDEADLK;

    for (;;) {
        uintptr_t irq = maia_arch_irq_save();
        maia_pthread_record_t *record = find_handle_locked(thread);
        if (record == NULL) {
            maia_arch_irq_restore(irq);
            return ESRCH;
        }
        if (record->detached) {
            maia_arch_irq_restore(irq);
            return EINVAL;
        }
        if (record->state == MAIA_PTHREAD_EXITED) {
            if (record->joiner_task_id != MAIA_PTHREAD_NO_TASK &&
                record->joiner_task_id != self_task) {
                maia_arch_irq_restore(irq);
                return EINVAL;
            }
            void *result = record->return_value;
            clear_record_locked(record);
            maia_arch_irq_restore(irq);
            if (return_value != NULL) *return_value = result;
            return 0;
        }
        if (self_task < 0 || (uint32_t)self_task >= MAIA_MAX_TASKS) {
            maia_arch_irq_restore(irq);
            return EPERM;
        }
        if (record->joiner_task_id != MAIA_PTHREAD_NO_TASK &&
            record->joiner_task_id != self_task) {
            maia_arch_irq_restore(irq);
            return EINVAL;
        }
        record->joiner_task_id = self_task;
        maia_arch_irq_restore(irq);

        maia_status_t wait_status =
            maia_task_wait_notification(MAIA_TASK_WAIT_FOREVER);
        if (wait_status != MAIA_OK) {
            irq = maia_arch_irq_save();
            record = find_handle_locked(thread);
            if (record != NULL && record->joiner_task_id == self_task) {
                record->joiner_task_id = MAIA_PTHREAD_NO_TASK;
            }
            maia_arch_irq_restore(irq);
            return wait_status == MAIA_ERR_PERM ? EPERM : EINVAL;
        }
    }
}

int pthread_detach(pthread_t thread) {
    uintptr_t irq = maia_arch_irq_save();
    maia_pthread_record_t *record = find_handle_locked(thread);
    if (record == NULL) {
        maia_arch_irq_restore(irq);
        return ESRCH;
    }
    if (record->detached || record->joiner_task_id != MAIA_PTHREAD_NO_TASK) {
        maia_arch_irq_restore(irq);
        return EINVAL;
    }
    if (record->state == MAIA_PTHREAD_EXITED) {
        clear_record_locked(record);
    } else {
        record->detached = 1U;
    }
    maia_arch_irq_restore(irq);
    return 0;
}

void pthread_exit(void *return_value) {
    maia_task_id_t task_id = maia_task_current_id();
    if (task_id >= 0 && (uint32_t)task_id < MAIA_MAX_TASKS) {
        uintptr_t irq = maia_arch_irq_save();
        maia_pthread_record_t *record = find_task_locked(task_id);
        if (record != NULL && record->state == MAIA_PTHREAD_RUNNING) {
            record->return_value = return_value;
        }
        maia_arch_irq_restore(irq);
    }
    maia_task_exit_current();
}

void maia_pthread_task_exit_complete(maia_task_id_t task_id) {
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) return;
    uintptr_t irq = maia_arch_irq_save();
    for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
        if (g_threads[i].state != MAIA_PTHREAD_FREE &&
            g_threads[i].joiner_task_id == task_id) {
            g_threads[i].joiner_task_id = MAIA_PTHREAD_NO_TASK;
        }
    }
    maia_pthread_record_t *record = find_task_locked(task_id);
    if (record == NULL || record->state != MAIA_PTHREAD_RUNNING) {
        maia_arch_irq_restore(irq);
        return;
    }

    void *owned_stack = record->owns_stack ? record->stack_base : NULL;
    if (record->owns_stack && maia_mem_free(owned_stack) != MAIA_OK) {
        maia_arch_irq_restore(irq);
        __builtin_trap();
    }
    record->stack_base = NULL;
    record->stack_size = 0U;
    record->owns_stack = 0U;
    record->state = MAIA_PTHREAD_EXITED;

    maia_task_id_t joiner = record->joiner_task_id;
    if (record->detached) clear_record_locked(record);
    maia_arch_irq_restore(irq);

    if (joiner != MAIA_PTHREAD_NO_TASK &&
        maia_task_notify(joiner) != MAIA_OK) {
        irq = maia_arch_irq_save();
        record = find_task_locked(task_id);
        if (record != NULL && record->joiner_task_id == joiner) {
            record->joiner_task_id = MAIA_PTHREAD_NO_TASK;
        }
        maia_arch_irq_restore(irq);
    }
}
