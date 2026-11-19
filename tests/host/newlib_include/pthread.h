#ifndef MAIA_TEST_PTHREAD_H
#define MAIA_TEST_PTHREAD_H

#include <stdint.h>

typedef uint32_t pthread_mutex_t;
typedef uint32_t pthread_cond_t;
typedef uint32_t pthread_key_t;
typedef uint32_t pthread_t;
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
#define PTHREAD_ONCE_INIT {0, 0}
#define PTHREAD_MUTEX_NORMAL 0
#define PTHREAD_MUTEX_RECURSIVE 1
#define PTHREAD_MUTEX_ERRORCHECK 2
#define PTHREAD_MUTEX_DEFAULT 3

#endif
