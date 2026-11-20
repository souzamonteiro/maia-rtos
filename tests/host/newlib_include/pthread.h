#ifndef MAIA_TEST_PTHREAD_H
#define MAIA_TEST_PTHREAD_H

#include <stddef.h>
#include <stdint.h>

#ifdef MAIA_PTHREAD_TEST
#define pthread_attr_init maia_test_pthread_attr_init
#define pthread_attr_destroy maia_test_pthread_attr_destroy
#define pthread_attr_setstacksize maia_test_pthread_attr_setstacksize
#define pthread_attr_getstacksize maia_test_pthread_attr_getstacksize
#define pthread_attr_setstack maia_test_pthread_attr_setstack
#define pthread_attr_getstack maia_test_pthread_attr_getstack
#define pthread_attr_setstackaddr maia_test_pthread_attr_setstackaddr
#define pthread_attr_getstackaddr maia_test_pthread_attr_getstackaddr
#define pthread_attr_setdetachstate maia_test_pthread_attr_setdetachstate
#define pthread_attr_getdetachstate maia_test_pthread_attr_getdetachstate
#define pthread_attr_setguardsize maia_test_pthread_attr_setguardsize
#define pthread_attr_getguardsize maia_test_pthread_attr_getguardsize
#define pthread_create maia_test_pthread_create
#define pthread_self maia_test_pthread_self
#define pthread_equal maia_test_pthread_equal
#define pthread_join maia_test_pthread_join
#define pthread_detach maia_test_pthread_detach
#define pthread_exit maia_test_pthread_exit
#endif

typedef uint32_t pthread_mutex_t;
typedef uint32_t pthread_cond_t;
typedef uint32_t pthread_key_t;
typedef uint32_t pthread_t;
typedef struct {
    int is_initialized;
    void *stackaddr;
    int stacksize;
    int detachstate;
} pthread_attr_t;
typedef struct {
    int is_initialized;
    int recursive;
} pthread_mutexattr_t;
typedef struct {
    int is_initialized;
    int clock;
} pthread_condattr_t;
typedef struct {
    int is_initialized;
    int init_executed;
} pthread_once_t;

#define PTHREAD_MUTEX_INITIALIZER UINT32_MAX
#define PTHREAD_COND_INITIALIZER UINT32_MAX
#define PTHREAD_ONCE_INIT {1, 0}
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_DEFAULT 3
#define PTHREAD_CREATE_DETACHED 0
#define PTHREAD_CREATE_JOINABLE 1

int pthread_attr_init(pthread_attr_t *attr);
int pthread_attr_destroy(pthread_attr_t *attr);
int pthread_attr_setstacksize(pthread_attr_t *attr, size_t stack_size);
int pthread_attr_getstacksize(const pthread_attr_t *attr, size_t *stack_size);
int pthread_attr_setstack(pthread_attr_t *attr, void *stack, size_t stack_size);
int pthread_attr_getstack(const pthread_attr_t *attr, void **stack, size_t *stack_size);
int pthread_attr_setstackaddr(pthread_attr_t *attr, void *stack);
int pthread_attr_getstackaddr(const pthread_attr_t *attr, void **stack);
int pthread_attr_setdetachstate(pthread_attr_t *attr, int detach_state);
int pthread_attr_getdetachstate(const pthread_attr_t *attr, int *detach_state);
int pthread_attr_setguardsize(pthread_attr_t *attr, size_t guard_size);
int pthread_attr_getguardsize(const pthread_attr_t *attr, size_t *guard_size);
int pthread_create(pthread_t *thread, const pthread_attr_t *attr,
                   void *(*start_routine)(void *), void *arg);
pthread_t pthread_self(void);
int pthread_equal(pthread_t first, pthread_t second);
int pthread_join(pthread_t thread, void **return_value);
int pthread_detach(pthread_t thread);
void pthread_exit(void *return_value) __attribute__((noreturn));

#endif
