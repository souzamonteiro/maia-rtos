#include <maia/arch/irq.h>
#include <maia/kernel.h>
#include <maia/runtime/gthread.h>

#include <errno.h>

#define GTHREAD_MUTEX_SLOTS 32U
#define GTHREAD_KEY_SLOTS 16U
#define GTHREAD_COND_SLOTS 16U
#define GTHREAD_TASK_SLOTS (MAIA_MAX_TASKS + 1U)
#define GTHREAD_DESTRUCTOR_ITERATIONS 4U
#define GTHREAD_BOOT_OWNER ((int32_t)MAIA_MAX_TASKS)

typedef struct gthread_mutex_slot {
    pthread_mutex_t *key;
    maia_mutex_t mutex;
    uint8_t recursive;
} gthread_mutex_slot_t;

typedef struct gthread_cond_slot {
    pthread_cond_t *key;
    uint32_t sequence;
    uint32_t waiters;
} gthread_cond_slot_t;

typedef struct gthread_key_slot {
    pthread_key_t token;
    void (*destructor)(void *);
    uint8_t allocated;
} gthread_key_slot_t;

static gthread_mutex_slot_t g_mutex_slots[GTHREAD_MUTEX_SLOTS];
static gthread_cond_slot_t g_cond_slots[GTHREAD_COND_SLOTS];
static gthread_key_slot_t g_key_slots[GTHREAD_KEY_SLOTS];
static void *g_key_values[GTHREAD_TASK_SLOTS][GTHREAD_KEY_SLOTS];
static maia_mutex_t g_registry_mutex;
static uint8_t g_registry_initialized;
static pthread_key_t g_next_key_token = 1U;

static int32_t mutex_owner(void) {
    maia_task_id_t task_id = maia_task_current_id();
    return task_id >= 0 ? task_id : GTHREAD_BOOT_OWNER;
}

static void registry_lock(void) {
    uintptr_t irq = maia_arch_irq_save();
    if (!g_registry_initialized) {
        (void)maia_mutex_init(&g_registry_mutex);
        g_registry_initialized = 1U;
    }
    maia_arch_irq_restore(irq);

    int32_t owner = mutex_owner();
    maia_status_t status;
    do {
        status = maia_mutex_trylock(&g_registry_mutex, owner);
        if (status == MAIA_ERR_BUSY) maia_task_yield();
    } while (status == MAIA_ERR_BUSY);
    if (status != MAIA_OK) __builtin_trap();
}

static void registry_unlock(void) {
    if (maia_mutex_unlock(&g_registry_mutex, mutex_owner()) != MAIA_OK) {
        __builtin_trap();
    }
}

static gthread_mutex_slot_t *find_slot_locked(pthread_mutex_t *key) {
    for (uint32_t i = 0U; i < GTHREAD_MUTEX_SLOTS; ++i) {
        if (g_mutex_slots[i].key == key) return &g_mutex_slots[i];
    }
    return NULL;
}

static gthread_mutex_slot_t *allocate_slot_locked(pthread_mutex_t *key, int recursive) {
    for (uint32_t i = 0U; i < GTHREAD_MUTEX_SLOTS; ++i) {
        gthread_mutex_slot_t *slot = &g_mutex_slots[i];
        if (slot->key == NULL) {
            (void)maia_mutex_init(&slot->mutex);
            slot->key = key;
            slot->recursive = recursive != 0;
            return slot;
        }
    }
    return NULL;
}

static int find_or_create_slot(pthread_mutex_t *key, maia_mutex_t **out_mutex) {
    if (key == NULL || out_mutex == NULL) return EINVAL;
    registry_lock();
    gthread_mutex_slot_t *slot = find_slot_locked(key);
    if (slot == NULL) slot = allocate_slot_locked(key, 0);
    if (slot != NULL) *out_mutex = &slot->mutex;
    registry_unlock();
    return slot != NULL ? 0 : ENOMEM;
}

static int find_existing_slot(pthread_mutex_t *key, maia_mutex_t **out_mutex) {
    if (key == NULL || out_mutex == NULL) return EINVAL;
    registry_lock();
    gthread_mutex_slot_t *slot = find_slot_locked(key);
    if (slot != NULL) *out_mutex = &slot->mutex;
    registry_unlock();
    return slot != NULL ? 0 : EINVAL;
}

static int current_tls_slot(void) {
    maia_task_id_t task_id = maia_task_current_id();
    if (task_id == -1) return (int)MAIA_MAX_TASKS;
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) return -1;
    return (int)task_id;
}

static int find_key_slot_locked(pthread_key_t key) {
    for (uint32_t i = 0U; i < GTHREAD_KEY_SLOTS; ++i) {
        if (g_key_slots[i].allocated && g_key_slots[i].token == key) return (int)i;
    }
    return -1;
}

static int find_cond_slot_locked(pthread_cond_t *key, int create) {
    for (uint32_t i = 0U; i < GTHREAD_COND_SLOTS; ++i) {
        if (g_cond_slots[i].key == key) return (int)i;
    }
    if (!create) return -1;
    for (uint32_t i = 0U; i < GTHREAD_COND_SLOTS; ++i) {
        if (g_cond_slots[i].key == NULL) {
            g_cond_slots[i].key = key;
            g_cond_slots[i].sequence = 0U;
            g_cond_slots[i].waiters = 0U;
            return (int)i;
        }
    }
    return -1;
}

static void clear_cond_waiter(uint32_t task_bit, pthread_cond_t *condition) {
    uintptr_t irq = maia_arch_irq_save();
    int slot = find_cond_slot_locked(condition, 0);
    if (slot >= 0) g_cond_slots[slot].waiters &= ~task_bit;
    maia_arch_irq_restore(irq);
}

int maia_gthread_mutexattr_init(pthread_mutexattr_t *attr) {
    if (attr == NULL) return EINVAL;
    attr->is_initialized = 1;
    attr->recursive = 0;
    return 0;
}

int maia_gthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
    if (attr == NULL || !attr->is_initialized) return EINVAL;
    attr->is_initialized = 0;
    return 0;
}

int maia_gthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    if (attr == NULL || !attr->is_initialized) return EINVAL;
    if (type != PTHREAD_MUTEX_DEFAULT && type != PTHREAD_MUTEX_RECURSIVE) {
        return type == PTHREAD_MUTEX_NORMAL || type == PTHREAD_MUTEX_ERRORCHECK
            ? ENOTSUP : EINVAL;
    }
    attr->recursive = type == PTHREAD_MUTEX_RECURSIVE;
    return 0;
}

int maia_gthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type) {
    if (attr == NULL || !attr->is_initialized || type == NULL) return EINVAL;
    *type = attr->recursive ? PTHREAD_MUTEX_RECURSIVE : PTHREAD_MUTEX_DEFAULT;
    return 0;
}

int maia_gthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    if (key == NULL) return EINVAL;
    uintptr_t irq = maia_arch_irq_save();
    for (uint32_t i = 0U; i < GTHREAD_KEY_SLOTS; ++i) {
        if (g_key_slots[i].allocated) continue;
        pthread_key_t token = g_next_key_token++;
        if (token == 0U) token = g_next_key_token++;
        while (find_key_slot_locked(token) >= 0) {
            token = g_next_key_token++;
            if (token == 0U) token = g_next_key_token++;
        }
        g_key_slots[i].token = token;
        g_key_slots[i].destructor = destructor;
        g_key_slots[i].allocated = 1U;
        *key = token;
        maia_arch_irq_restore(irq);
        return 0;
    }
    maia_arch_irq_restore(irq);
    return EAGAIN;
}

int maia_gthread_key_delete(pthread_key_t key) {
    uintptr_t irq = maia_arch_irq_save();
    int slot_index = find_key_slot_locked(key);
    if (slot_index < 0) {
        maia_arch_irq_restore(irq);
        return EINVAL;
    }
    g_key_slots[slot_index].allocated = 0U;
    g_key_slots[slot_index].destructor = NULL;
    for (uint32_t task = 0U; task < GTHREAD_TASK_SLOTS; ++task) {
        g_key_values[task][slot_index] = NULL;
    }
    maia_arch_irq_restore(irq);
    return 0;
}

int maia_gthread_setspecific(pthread_key_t key, const void *value) {
    int task_slot = current_tls_slot();
    if (task_slot < 0) return EINVAL;
    uintptr_t irq = maia_arch_irq_save();
    int key_slot = find_key_slot_locked(key);
    if (key_slot < 0) {
        maia_arch_irq_restore(irq);
        return EINVAL;
    }
    g_key_values[task_slot][key_slot] = (void *)value;
    maia_arch_irq_restore(irq);
    return 0;
}

void *maia_gthread_getspecific(pthread_key_t key) {
    int task_slot = current_tls_slot();
    if (task_slot < 0) return NULL;
    uintptr_t irq = maia_arch_irq_save();
    int key_slot = find_key_slot_locked(key);
    void *value = key_slot < 0 ? NULL : g_key_values[task_slot][key_slot];
    maia_arch_irq_restore(irq);
    return value;
}

int maia_gthread_once(pthread_once_t *once, void (*initializer)(void)) {
    if (once == NULL || initializer == NULL) return EINVAL;
    for (;;) {
        uintptr_t irq = maia_arch_irq_save();
        if (once->is_initialized == 1 && once->init_executed == 1) {
            maia_arch_irq_restore(irq);
            return 0;
        }
        if (once->init_executed != 0) {
            maia_arch_irq_restore(irq);
            return EINVAL;
        }
        if (once->is_initialized == 0 || once->is_initialized == 1) {
            once->is_initialized = 2;
            maia_arch_irq_restore(irq);
            initializer();
            irq = maia_arch_irq_save();
            once->is_initialized = 1;
            once->init_executed = 1;
            maia_arch_irq_restore(irq);
            return 0;
        }
        if (once->is_initialized != 2) {
            maia_arch_irq_restore(irq);
            return EINVAL;
        }
        maia_arch_irq_restore(irq);
        maia_task_yield();
    }
}

int maia_gthread_cancel(pthread_t thread) {
    (void)thread;
    return ENOTSUP;
}

int maia_gthread_cond_init(pthread_cond_t *condition, const pthread_condattr_t *attr) {
    if (condition == NULL) return EINVAL;
    if (attr != NULL && !attr->is_initialized) return EINVAL;
    if (attr != NULL) return ENOTSUP;
    uintptr_t irq = maia_arch_irq_save();
    if (find_cond_slot_locked(condition, 0) >= 0) {
        maia_arch_irq_restore(irq);
        return EBUSY;
    }
    int slot = find_cond_slot_locked(condition, 1);
    maia_arch_irq_restore(irq);
    return slot >= 0 ? 0 : EAGAIN;
}

int maia_gthread_cond_destroy(pthread_cond_t *condition) {
    if (condition == NULL) return EINVAL;
    uintptr_t irq = maia_arch_irq_save();
    int slot = find_cond_slot_locked(condition, 0);
    if (slot < 0) {
        maia_arch_irq_restore(irq);
        return EINVAL;
    }
    if (g_cond_slots[slot].waiters != 0U) {
        maia_arch_irq_restore(irq);
        return EBUSY;
    }
    g_cond_slots[slot].key = NULL;
    g_cond_slots[slot].sequence = 0U;
    maia_arch_irq_restore(irq);
    return 0;
}

static int cond_notify(pthread_cond_t *condition, int broadcast) {
    if (condition == NULL) return EINVAL;
    uintptr_t irq = maia_arch_irq_save();
    int slot = find_cond_slot_locked(condition, 1);
    if (slot < 0) {
        maia_arch_irq_restore(irq);
        return EAGAIN;
    }
    ++g_cond_slots[slot].sequence;
    uint32_t waiters = g_cond_slots[slot].waiters;
    if (!broadcast && waiters != 0U) {
        uint32_t first = 0U;
        while ((waiters & (1U << first)) == 0U) ++first;
        waiters = 1U << first;
    }
    maia_arch_irq_restore(irq);

    for (uint32_t task = 0U; task < MAIA_MAX_TASKS; ++task) {
        if ((waiters & (1U << task)) == 0U) continue;
        maia_status_t status = maia_task_notify((maia_task_id_t)task);
        if (status != MAIA_OK && status != MAIA_ERR_INVALID) return EINVAL;
    }
    return 0;
}

int maia_gthread_cond_signal(pthread_cond_t *condition) {
    return cond_notify(condition, 0);
}

int maia_gthread_cond_broadcast(pthread_cond_t *condition) {
    return cond_notify(condition, 1);
}

int maia_gthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex) {
    if (condition == NULL || mutex == NULL) return EINVAL;
    maia_task_id_t task_id = maia_task_current_id();
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) return EPERM;
    maia_mutex_t *native = NULL;
    if (find_existing_slot(mutex, &native) != 0) return EINVAL;
    uintptr_t irq = maia_arch_irq_save();
    if (!native->locked || native->owner_task_id != task_id || native->lock_count != 1U) {
        maia_arch_irq_restore(irq);
        return EPERM;
    }
    int slot = find_cond_slot_locked(condition, 1);
    if (slot < 0) {
        maia_arch_irq_restore(irq);
        return EAGAIN;
    }
    uint32_t sequence = g_cond_slots[slot].sequence;
    uint32_t task_bit = 1U << (uint32_t)task_id;
    g_cond_slots[slot].waiters |= task_bit;
    maia_arch_irq_restore(irq);

    int result = maia_gthread_mutex_unlock(mutex);
    if (result != 0) {
        clear_cond_waiter(task_bit, condition);
        return result;
    }
    for (;;) {
        irq = maia_arch_irq_save();
        slot = find_cond_slot_locked(condition, 0);
        if (slot < 0 || g_cond_slots[slot].sequence != sequence) {
            if (slot >= 0) g_cond_slots[slot].waiters &= ~task_bit;
            maia_arch_irq_restore(irq);
            break;
        }
        maia_arch_irq_restore(irq);
        maia_status_t status = maia_task_wait_notification(MAIA_TASK_WAIT_FOREVER);
        if (status != MAIA_OK) {
            clear_cond_waiter(task_bit, condition);
            (void)maia_gthread_mutex_lock(mutex);
            return status == MAIA_ERR_PERM ? EPERM : EINVAL;
        }
    }
    return maia_gthread_mutex_lock(mutex);
}

void maia_gthread_task_reclaim(maia_task_id_t task_id) {
    if (task_id < 0 || (uint32_t)task_id >= MAIA_MAX_TASKS) return;
    uint32_t task_slot = (uint32_t)task_id;
    uint32_t task_bit = 1U << (uint32_t)task_id;
    for (uint32_t iteration = 0U; iteration < GTHREAD_DESTRUCTOR_ITERATIONS; ++iteration) {
        int called_destructor = 0;
        for (uint32_t key_slot = 0U; key_slot < GTHREAD_KEY_SLOTS; ++key_slot) {
            uintptr_t irq = maia_arch_irq_save();
            void *value = g_key_values[task_slot][key_slot];
            void (*destructor)(void *) = g_key_slots[key_slot].allocated
                ? g_key_slots[key_slot].destructor : NULL;
            if (destructor != NULL && value != NULL) {
                g_key_values[task_slot][key_slot] = NULL;
            }
            maia_arch_irq_restore(irq);
            if (destructor != NULL && value != NULL) {
                called_destructor = 1;
                destructor(value);
            }
        }
        if (!called_destructor) break;
    }
    uintptr_t irq = maia_arch_irq_save();
    for (uint32_t key_slot = 0U; key_slot < GTHREAD_KEY_SLOTS; ++key_slot) {
        g_key_values[task_slot][key_slot] = NULL;
    }
    for (uint32_t cond_slot = 0U; cond_slot < GTHREAD_COND_SLOTS; ++cond_slot) {
        g_cond_slots[cond_slot].waiters &= ~task_bit;
    }
    maia_arch_irq_restore(irq);
}

int maia_gthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    if (mutex == NULL) return EINVAL;
    int recursive = 0;
    if (attr != NULL) {
        if (!attr->is_initialized) return EINVAL;
        recursive = attr->recursive;
    }
    registry_lock();
    if (find_slot_locked(mutex) != NULL) {
        registry_unlock();
        return EBUSY;
    }
    gthread_mutex_slot_t *slot = allocate_slot_locked(mutex, recursive);
    registry_unlock();
    return slot != NULL ? 0 : ENOMEM;
}

int maia_gthread_mutex_destroy(pthread_mutex_t *mutex) {
    if (mutex == NULL) return EINVAL;
    registry_lock();
    for (uint32_t i = 0U; i < GTHREAD_MUTEX_SLOTS; ++i) {
        gthread_mutex_slot_t *slot = &g_mutex_slots[i];
        if (slot->key != mutex) continue;
        uintptr_t irq = maia_arch_irq_save();
        int locked = slot->mutex.locked != 0U;
        maia_arch_irq_restore(irq);
        if (locked) {
            registry_unlock();
            return EBUSY;
        }
        slot->key = NULL;
        registry_unlock();
        return 0;
    }
    registry_unlock();
    return EINVAL;
}

int maia_gthread_mutex_trylock(pthread_mutex_t *mutex) {
    maia_mutex_t *native = NULL;
    int result = find_or_create_slot(mutex, &native);
    if (result != 0) return result;
    int32_t owner = mutex_owner();
    uintptr_t irq = maia_arch_irq_save();
    if (native->locked && native->owner_task_id == owner) {
        gthread_mutex_slot_t *slot = find_slot_locked(mutex);
        if (slot != NULL && slot->recursive) {
            maia_status_t status = maia_mutex_trylock(native, owner);
            maia_arch_irq_restore(irq);
            return status == MAIA_OK ? 0 : EINVAL;
        }
        maia_arch_irq_restore(irq);
        return EBUSY;
    }
    maia_status_t status = maia_mutex_trylock(native, owner);
    maia_arch_irq_restore(irq);
    if (status == MAIA_OK) return 0;
    if (status == MAIA_ERR_BUSY) return EBUSY;
    return EINVAL;
}

int maia_gthread_mutex_lock(pthread_mutex_t *mutex) {
    maia_mutex_t *native = NULL;
    int result = find_or_create_slot(mutex, &native);
    if (result != 0) return result;
    int32_t owner = mutex_owner();
    for (;;) {
        uintptr_t irq = maia_arch_irq_save();
        if (native->locked && native->owner_task_id == owner) {
            gthread_mutex_slot_t *slot = find_slot_locked(mutex);
            if (slot != NULL && slot->recursive) {
                maia_status_t status = maia_mutex_trylock(native, owner);
                maia_arch_irq_restore(irq);
                return status == MAIA_OK ? 0 : EINVAL;
            }
            maia_arch_irq_restore(irq);
            return EDEADLK;
        }
        maia_status_t status = maia_mutex_trylock(native, owner);
        maia_arch_irq_restore(irq);
        if (status == MAIA_OK) return 0;
        if (status != MAIA_ERR_BUSY) return EINVAL;
        maia_task_yield();
    }
}

int maia_gthread_mutex_unlock(pthread_mutex_t *mutex) {
    maia_mutex_t *native = NULL;
    int result = find_existing_slot(mutex, &native);
    if (result != 0) return result;
    maia_status_t status = maia_mutex_unlock(native, mutex_owner());
    if (status == MAIA_OK) return 0;
    if (status == MAIA_ERR_PERM) return EPERM;
    return EINVAL;
}

#ifndef MAIA_GTHREAD_TEST
int pthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr) {
    return maia_gthread_mutex_init(mutex, attr);
}

int pthread_mutex_destroy(pthread_mutex_t *mutex) {
    return maia_gthread_mutex_destroy(mutex);
}

int pthread_mutex_lock(pthread_mutex_t *mutex) {
    return maia_gthread_mutex_lock(mutex);
}

int pthread_mutex_trylock(pthread_mutex_t *mutex) {
    return maia_gthread_mutex_trylock(mutex);
}

int pthread_mutex_unlock(pthread_mutex_t *mutex) {
    return maia_gthread_mutex_unlock(mutex);
}

int pthread_mutexattr_init(pthread_mutexattr_t *attr) {
    return maia_gthread_mutexattr_init(attr);
}

int pthread_mutexattr_destroy(pthread_mutexattr_t *attr) {
    return maia_gthread_mutexattr_destroy(attr);
}

int pthread_mutexattr_settype(pthread_mutexattr_t *attr, int type) {
    return maia_gthread_mutexattr_settype(attr, type);
}

int pthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type) {
    return maia_gthread_mutexattr_gettype(attr, type);
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void *)) {
    return maia_gthread_key_create(key, destructor);
}

int pthread_key_delete(pthread_key_t key) {
    return maia_gthread_key_delete(key);
}

int pthread_setspecific(pthread_key_t key, const void *value) {
    return maia_gthread_setspecific(key, value);
}

void *pthread_getspecific(pthread_key_t key) {
    return maia_gthread_getspecific(key);
}

int pthread_once(pthread_once_t *once, void (*initializer)(void)) {
    return maia_gthread_once(once, initializer);
}

int pthread_cancel(pthread_t thread) {
    /* libstdc++ uses this weak symbol to detect an active gthread backend. */
    return maia_gthread_cancel(thread);
}

int pthread_cond_init(pthread_cond_t *condition, const pthread_condattr_t *attr) {
    return maia_gthread_cond_init(condition, attr);
}

int pthread_cond_destroy(pthread_cond_t *condition) {
    return maia_gthread_cond_destroy(condition);
}

int pthread_cond_signal(pthread_cond_t *condition) {
    return maia_gthread_cond_signal(condition);
}

int pthread_cond_broadcast(pthread_cond_t *condition) {
    return maia_gthread_cond_broadcast(condition);
}

int pthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex) {
    return maia_gthread_cond_wait(condition, mutex);
}
#endif
