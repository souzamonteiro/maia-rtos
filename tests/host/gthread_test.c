#include <maia/arch/irq.h>
#include <maia/kernel.h>
#include <maia/runtime/gthread.h>

#include <errno.h>
#include <stdint.h>

static maia_task_id_t current_task = 1;
static uintptr_t irq_state = 8U;
static pthread_mutex_t *release_on_yield;
static unsigned destructor_calls;
static void *destructor_value;
static unsigned once_calls;
static pthread_cond_t *condition_to_signal;
static pthread_mutex_t *mutex_to_release;
static int signal_during_wait;

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
    if (release_on_yield != NULL) {
        maia_task_id_t waiting = current_task;
        current_task = 1;
        (void)maia_gthread_mutex_unlock(release_on_yield);
        current_task = waiting;
        release_on_yield = NULL;
    }
}

maia_status_t maia_task_notify(maia_task_id_t task_id) {
    return task_id >= 0 && (uint32_t)task_id < MAIA_MAX_TASKS ? MAIA_OK : MAIA_ERR_INVALID;
}

maia_status_t maia_task_wait_notification(uint32_t timeout_ticks) {
    if (timeout_ticks != MAIA_TASK_WAIT_FOREVER || !signal_during_wait) return MAIA_ERR_TIMEOUT;
    maia_task_id_t waiting = current_task;
    current_task = 2;
    if (maia_gthread_mutex_lock(mutex_to_release) != 0 ||
        maia_gthread_cond_signal(condition_to_signal) != 0 ||
        maia_gthread_mutex_unlock(mutex_to_release) != 0) return MAIA_ERR_INVALID;
    current_task = waiting;
    signal_during_wait = 0;
    return MAIA_OK;
}

static void test_destructor(void *value) {
    ++destructor_calls;
    destructor_value = value;
}

static void test_once_initializer(void) {
    ++once_calls;
}

int main(void) {
    pthread_mutex_t uninitialized_mutex = 0U;
    if (maia_gthread_mutex_unlock(&uninitialized_mutex) != EINVAL) return 1;

    pthread_mutex_t static_mutex = PTHREAD_MUTEX_INITIALIZER;
    if (maia_gthread_mutex_trylock(&static_mutex) != 0) return 2;
    if (maia_gthread_mutex_lock(&static_mutex) != EDEADLK) return 3;
    if (maia_gthread_mutex_destroy(&static_mutex) != EBUSY) return 4;
    current_task = 2;
    if (maia_gthread_mutex_trylock(&static_mutex) != EBUSY) return 5;
    release_on_yield = &static_mutex;
    if (maia_gthread_mutex_lock(&static_mutex) != 0) return 6;
    if (maia_gthread_mutex_unlock(&static_mutex) != 0) return 7;

    current_task = 1;
    if (maia_gthread_mutex_destroy(&static_mutex) != 0) return 8;

    pthread_mutex_t dynamic_mutex = 0U;
    if (maia_gthread_mutex_init(&dynamic_mutex, NULL) != 0) return 9;
    if (maia_gthread_mutex_init(&dynamic_mutex, NULL) != EBUSY) return 10;
    if (maia_gthread_mutex_lock(&dynamic_mutex) != 0) return 11;
    if (maia_gthread_mutex_init(&dynamic_mutex, NULL) != EBUSY) return 12;
    if (maia_gthread_mutex_unlock(&dynamic_mutex) != 0) return 13;
    if (maia_gthread_mutex_destroy(&dynamic_mutex) != 0) return 14;

    pthread_mutexattr_t recursive_attr;
    pthread_mutex_t recursive_mutex = 0U;
    if (maia_gthread_mutexattr_init(&recursive_attr) != 0 ||
        maia_gthread_mutexattr_settype(&recursive_attr, PTHREAD_MUTEX_NORMAL) != ENOTSUP ||
        maia_gthread_mutexattr_settype(&recursive_attr, PTHREAD_MUTEX_RECURSIVE) != 0 ||
        maia_gthread_mutex_init(&recursive_mutex, &recursive_attr) != 0) return 15;
    if (maia_gthread_mutex_lock(&recursive_mutex) != 0 ||
        maia_gthread_mutex_lock(&recursive_mutex) != 0 ||
        maia_gthread_mutex_unlock(&recursive_mutex) != 0 ||
        maia_gthread_mutex_unlock(&recursive_mutex) != 0) return 16;
    if (maia_gthread_mutexattr_destroy(&recursive_attr) != 0 ||
        maia_gthread_mutex_destroy(&recursive_mutex) != 0) return 17;

    pthread_key_t key = 0U;
    int task_one_value = 1;
    int task_two_value = 2;
    if (maia_gthread_key_create(&key, test_destructor) != 0) return 18;
    current_task = 1;
    if (maia_gthread_setspecific(key, &task_one_value) != 0 ||
        maia_gthread_getspecific(key) != &task_one_value) return 19;
    current_task = 2;
    if (maia_gthread_getspecific(key) != NULL ||
        maia_gthread_setspecific(key, &task_two_value) != 0 ||
        maia_gthread_getspecific(key) != &task_two_value) return 20;
    maia_gthread_task_reclaim(1);
    if (destructor_calls != 1U || destructor_value != &task_one_value) return 21;
    current_task = 1;
    if (maia_gthread_getspecific(key) != NULL) return 22;
    current_task = 2;
    if (maia_gthread_key_delete(key) != 0 ||
        maia_gthread_setspecific(key, NULL) != EINVAL ||
        maia_gthread_getspecific(key) != NULL) return 23;

    pthread_once_t once = PTHREAD_ONCE_INIT;
    if (maia_gthread_once(&once, test_once_initializer) != 0 ||
        maia_gthread_once(&once, test_once_initializer) != 0 || once_calls != 1U) return 24;
    if (maia_gthread_cancel(1U) != ENOTSUP) return 25;

    pthread_cond_t condition = PTHREAD_COND_INITIALIZER;
    pthread_mutex_t condition_mutex = 0U;
    if (maia_gthread_mutex_init(&condition_mutex, NULL) != 0) return 26;
    current_task = 1;
    if (maia_gthread_mutex_lock(&condition_mutex) != 0) return 27;
    condition_to_signal = &condition;
    mutex_to_release = &condition_mutex;
    signal_during_wait = 1;
    if (maia_gthread_cond_wait(&condition, &condition_mutex) != 0 ||
        signal_during_wait != 0) return 28;
    current_task = 2;
    if (maia_gthread_mutex_trylock(&condition_mutex) != EBUSY) return 29;
    current_task = 1;
    if (maia_gthread_mutex_unlock(&condition_mutex) != 0 ||
        maia_gthread_cond_destroy(&condition) != 0 ||
        maia_gthread_mutex_destroy(&condition_mutex) != 0) return 30;

    if (irq_state != 8U) return 31;
    return 0;
}
