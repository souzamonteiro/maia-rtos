#ifndef MAIA_RUNTIME_GTHREAD_H
#define MAIA_RUNTIME_GTHREAD_H

#include <pthread.h>
#include <maia/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

int maia_gthread_mutex_init(pthread_mutex_t *mutex, const pthread_mutexattr_t *attr);
int maia_gthread_mutex_destroy(pthread_mutex_t *mutex);
int maia_gthread_mutex_lock(pthread_mutex_t *mutex);
int maia_gthread_mutex_trylock(pthread_mutex_t *mutex);
int maia_gthread_mutex_unlock(pthread_mutex_t *mutex);
int maia_gthread_mutexattr_init(pthread_mutexattr_t *attr);
int maia_gthread_mutexattr_destroy(pthread_mutexattr_t *attr);
int maia_gthread_mutexattr_settype(pthread_mutexattr_t *attr, int type);
int maia_gthread_mutexattr_gettype(const pthread_mutexattr_t *attr, int *type);
int maia_gthread_key_create(pthread_key_t *key, void (*destructor)(void *));
int maia_gthread_key_delete(pthread_key_t key);
int maia_gthread_setspecific(pthread_key_t key, const void *value);
void *maia_gthread_getspecific(pthread_key_t key);
int maia_gthread_once(pthread_once_t *once, void (*initializer)(void));
int maia_gthread_cancel(pthread_t thread);
int maia_gthread_cond_init(pthread_cond_t *condition, const pthread_condattr_t *attr);
int maia_gthread_cond_destroy(pthread_cond_t *condition);
int maia_gthread_cond_signal(pthread_cond_t *condition);
int maia_gthread_cond_broadcast(pthread_cond_t *condition);
int maia_gthread_cond_wait(pthread_cond_t *condition, pthread_mutex_t *mutex);
void maia_gthread_task_reclaim(maia_task_id_t task_id);

#ifdef __cplusplus
}
#endif

#endif
