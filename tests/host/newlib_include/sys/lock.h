#ifndef MAIA_TEST_SYS_LOCK_H
#define MAIA_TEST_SYS_LOCK_H

#define _RETARGETABLE_LOCKING 1

struct __lock;
typedef struct __lock *_LOCK_T;
typedef _LOCK_T _LOCK_RECURSIVE_T;

void __retarget_lock_init(_LOCK_T *lock);
void __retarget_lock_init_recursive(_LOCK_T *lock);
void __retarget_lock_close(_LOCK_T lock);
void __retarget_lock_close_recursive(_LOCK_T lock);
void __retarget_lock_acquire(_LOCK_T lock);
void __retarget_lock_acquire_recursive(_LOCK_T lock);
int __retarget_lock_try_acquire(_LOCK_T lock);
int __retarget_lock_try_acquire_recursive(_LOCK_T lock);
void __retarget_lock_release(_LOCK_T lock);
void __retarget_lock_release_recursive(_LOCK_T lock);

#endif
