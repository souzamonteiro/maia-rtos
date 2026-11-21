#include <maia/arch/irq.h>
#include <maia/arch/riscv_stack.h>
#include <maia/runtime/newlib_locks.h>

#include <sys/lock.h>

#if !defined(_RETARGETABLE_LOCKING)
#error "Maia Newlib locks require retargetable locking support"
#endif

#define NEWLIB_LOCK_MAGIC 0x4D4C4F43U
#define NEWLIB_LOCK_PANIC 0x4C4F434BU
#define NEWLIB_BOOT_OWNER ((int32_t)MAIA_MAX_TASKS)

struct __lock {
    maia_mutex_t mutex;
    uint32_t magic;
    uint8_t recursive;
    uint8_t initialized;
    uint8_t dynamic;
};

struct __lock __lock___arc4random_mutex;
struct __lock __lock___dd_hash_mutex;
struct __lock __lock___tz_mutex;
struct __lock __lock___env_recursive_mutex;
struct __lock __lock___malloc_recursive_mutex;
struct __lock __lock___at_quick_exit_mutex;
struct __lock __lock___atexit_recursive_mutex;
struct __lock __lock___sfp_recursive_mutex;

static maia_mutex_t g_lock_allocator_mutex;
static uint32_t g_runtime_ready;

static __attribute__((noreturn)) void lock_fault(uint32_t reason) {
#if defined(__riscv)
    uintptr_t sp;
    __asm__ volatile("mv %0, sp" : "=r"(sp));
    maia_arch_stack_panic(NEWLIB_LOCK_PANIC, 0U, reason, sp, MAIA_STACK_BAD_CONTEXT);
#else
    (void)reason;
    __builtin_trap();
#endif
}

static int32_t lock_owner(void) {
    maia_task_id_t task_id = maia_task_current_id();
    return task_id >= 0 ? task_id : NEWLIB_BOOT_OWNER;
}

static void kernel_mutex_acquire(maia_mutex_t *mutex) {
    int32_t owner = lock_owner();
    maia_status_t status;
    do {
        status = maia_mutex_trylock(mutex, owner);
        if (status == MAIA_ERR_BUSY) maia_task_yield();
    } while (status == MAIA_ERR_BUSY);
    if (status != MAIA_OK) lock_fault((uint32_t)(-status));
}

static void kernel_mutex_release(maia_mutex_t *mutex) {
    if (maia_mutex_unlock(mutex, lock_owner()) != MAIA_OK) {
        lock_fault((uint32_t)(-MAIA_ERR_PERM));
    }
}

static int is_static_lock(_LOCK_T lock) {
    return lock == &__lock___arc4random_mutex ||
           lock == &__lock___dd_hash_mutex ||
           lock == &__lock___tz_mutex ||
           lock == &__lock___env_recursive_mutex ||
           lock == &__lock___malloc_recursive_mutex ||
           lock == &__lock___at_quick_exit_mutex ||
           lock == &__lock___atexit_recursive_mutex ||
           lock == &__lock___sfp_recursive_mutex;
}

static int valid_lock(_LOCK_T lock) {
    if (lock == NULL) return 0;
    if (is_static_lock(lock)) return lock->magic == 0U || lock->magic == NEWLIB_LOCK_MAGIC;
    return lock->magic == NEWLIB_LOCK_MAGIC && lock->dynamic != 0U;
}

static void reset_lock(_LOCK_T lock, int recursive) {
    uintptr_t irq = maia_arch_irq_save();
    if (lock->initialized && lock->mutex.locked) {
        maia_arch_irq_restore(irq);
        lock_fault(1U);
    }
    if (maia_mutex_init(&lock->mutex) != MAIA_OK) {
        maia_arch_irq_restore(irq);
        lock_fault(2U);
    }
    lock->magic = NEWLIB_LOCK_MAGIC;
    lock->recursive = (uint8_t)(recursive != 0);
    lock->initialized = 1U;
    maia_arch_irq_restore(irq);
}

static _LOCK_T allocate_lock(void) {
    if (!g_runtime_ready) lock_fault(3U);
    kernel_mutex_acquire(&g_lock_allocator_mutex);
    _LOCK_T lock = (_LOCK_T)maia_mem_alloc(sizeof *lock);
    if (lock != NULL) {
        lock->magic = NEWLIB_LOCK_MAGIC;
        lock->recursive = 0U;
        lock->initialized = 0U;
        lock->dynamic = 1U;
    }
    kernel_mutex_release(&g_lock_allocator_mutex);
    if (lock == NULL) lock_fault(4U);
    return lock;
}

static void initialize_lock(_LOCK_T *lock_ref, int recursive) {
    if (lock_ref == NULL) lock_fault(5U);
    /* __sfp initializes recycled FILE locks without clearing the freed pointer. */
    if (!is_static_lock(*lock_ref)) {
        *lock_ref = allocate_lock();
    } else if (!valid_lock(*lock_ref)) {
        lock_fault(6U);
    }
    reset_lock(*lock_ref, recursive);
}

static void ensure_lock_initialized(_LOCK_T lock, int recursive) {
    if (!valid_lock(lock)) lock_fault(7U);
    if (!lock->initialized) {
        if (!is_static_lock(lock)) lock_fault(8U);
        reset_lock(lock, recursive);
    } else if (lock->recursive != (uint8_t)(recursive != 0)) {
        lock_fault(9U);
    }
}

static int try_acquire(_LOCK_T lock, int recursive) {
    ensure_lock_initialized(lock, recursive);
    int32_t owner = lock_owner();
    uintptr_t irq = maia_arch_irq_save();
    if (!recursive && lock->mutex.locked && lock->mutex.owner_task_id == owner) {
        maia_arch_irq_restore(irq);
        return 0;
    }
    maia_status_t status = maia_mutex_trylock(&lock->mutex, owner);
    maia_arch_irq_restore(irq);
    if (status == MAIA_OK) return 1;
    if (status == MAIA_ERR_BUSY) return 0;
    lock_fault((uint32_t)(-status));
}

static void acquire(_LOCK_T lock, int recursive) {
    while (!try_acquire(lock, recursive)) maia_task_yield();
}

static void release(_LOCK_T lock, int recursive) {
    ensure_lock_initialized(lock, recursive);
    if (maia_mutex_unlock(&lock->mutex, lock_owner()) != MAIA_OK) {
        lock_fault(10U);
    }
}

static void close_lock(_LOCK_T lock, int recursive) {
    if (!valid_lock(lock)) lock_fault(11U);
    uintptr_t irq = maia_arch_irq_save();
    if (!lock->initialized ||
        lock->recursive != (uint8_t)(recursive != 0) ||
        lock->mutex.locked) {
        maia_arch_irq_restore(irq);
        lock_fault(12U);
    }
    int dynamic = lock->dynamic != 0U;
    lock->initialized = 0U;
    maia_arch_irq_restore(irq);
    if (!dynamic) return;
    if (!g_runtime_ready) lock_fault(13U);
    kernel_mutex_acquire(&g_lock_allocator_mutex);
    maia_status_t status = maia_mem_free(lock);
    kernel_mutex_release(&g_lock_allocator_mutex);
    if (status != MAIA_OK) lock_fault(14U);
}

maia_status_t maia_newlib_locks_init(void) {
    if (g_runtime_ready) return MAIA_ERR_BUSY;
    maia_mem_stats_t stats;
    if (maia_mem_get_stats(&stats) != MAIA_OK || stats.total_bytes == 0U) {
        return MAIA_ERR_INVALID;
    }
    maia_status_t status = maia_mutex_init(&g_lock_allocator_mutex);
    if (status != MAIA_OK) return status;
    g_runtime_ready = 1U;
    return MAIA_OK;
}

void __retarget_lock_init(_LOCK_T *lock) {
    initialize_lock(lock, 0);
}

void __retarget_lock_init_recursive(_LOCK_T *lock) {
    initialize_lock(lock, 1);
}

void __retarget_lock_close(_LOCK_T lock) {
    close_lock(lock, 0);
}

void __retarget_lock_close_recursive(_LOCK_T lock) {
    close_lock(lock, 1);
}

void __retarget_lock_acquire(_LOCK_T lock) {
    acquire(lock, 0);
}

void __retarget_lock_acquire_recursive(_LOCK_T lock) {
    acquire(lock, 1);
}

int __retarget_lock_try_acquire(_LOCK_T lock) {
    return try_acquire(lock, 0);
}

int __retarget_lock_try_acquire_recursive(_LOCK_T lock) {
    return try_acquire(lock, 1);
}

void __retarget_lock_release(_LOCK_T lock) {
    release(lock, 0);
}

void __retarget_lock_release_recursive(_LOCK_T lock) {
    release(lock, 1);
}
