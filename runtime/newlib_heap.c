#include <maia/arch/riscv_stack.h>
#include <maia/kernel.h>
#include <maia/runtime/newlib_heap.h>

#include <sys/lock.h>

#define NEWLIB_HEAP_PANIC 0x4D484541U

extern struct __lock __lock___malloc_recursive_mutex;

static __attribute__((noreturn)) void heap_fault(uint32_t reason) {
#if defined(__riscv)
    uintptr_t sp;
    __asm__ volatile("mv %0, sp" : "=r"(sp));
    maia_arch_stack_panic(NEWLIB_HEAP_PANIC, 0U, reason, sp, MAIA_STACK_BAD_CONTEXT);
#else
    (void)reason;
    __builtin_trap();
#endif
}

static void heap_lock(void) {
    __retarget_lock_acquire_recursive(&__lock___malloc_recursive_mutex);
}

static void heap_unlock(void) {
    __retarget_lock_release_recursive(&__lock___malloc_recursive_mutex);
}

void *maia_newlib_heap_alloc(size_t size) {
    if (size == 0U) return NULL;
    heap_lock();
    void *ptr = maia_mem_alloc(size);
    heap_unlock();
    return ptr;
}

void *maia_newlib_heap_calloc(size_t count, size_t size) {
    if (count != 0U && size > SIZE_MAX / count) return NULL;
    size_t total = count * size;
    if (total == 0U) return NULL;
    heap_lock();
    uint8_t *ptr = (uint8_t *)maia_mem_alloc(total);
    if (ptr != NULL) {
        for (size_t i = 0U; i < total; ++i) ptr[i] = 0U;
    }
    heap_unlock();
    return ptr;
}

void *maia_newlib_heap_realloc(void *ptr, size_t size) {
    if (ptr == NULL) return maia_newlib_heap_alloc(size);
    if (size == 0U) {
        maia_newlib_heap_free(ptr);
        return NULL;
    }

    heap_lock();
    size_t old_size = 0U;
    if (maia_mem_get_alloc_size(ptr, &old_size) != MAIA_OK) {
        heap_unlock();
        heap_fault(1U);
    }
    void *replacement = maia_mem_alloc(size);
    if (replacement == NULL) {
        heap_unlock();
        return NULL;
    }

    uint8_t *dst = (uint8_t *)replacement;
    const uint8_t *src = (const uint8_t *)ptr;
    size_t copy_size = old_size < size ? old_size : size;
    for (size_t i = 0U; i < copy_size; ++i) dst[i] = src[i];

    if (maia_mem_free(ptr) != MAIA_OK) {
        (void)maia_mem_free(replacement);
        heap_unlock();
        heap_fault(2U);
    }
    heap_unlock();
    return replacement;
}

void maia_newlib_heap_free(void *ptr) {
    if (ptr == NULL) return;
    heap_lock();
    maia_status_t status = maia_mem_free(ptr);
    heap_unlock();
    if (status != MAIA_OK) heap_fault(3U);
}

size_t maia_newlib_heap_usable_size(void *ptr) {
    if (ptr == NULL) return 0U;
    heap_lock();
    size_t size = 0U;
    maia_status_t status = maia_mem_get_alloc_size(ptr, &size);
    heap_unlock();
    if (status != MAIA_OK) heap_fault(4U);
    return size;
}

#ifndef MAIA_NEWLIB_HEAP_NO_CRT_EXPORTS
#include <errno.h>
#include <malloc.h>
#include <reent.h>
#include <stdlib.h>

static void set_reent_errno(struct _reent *reent, int error) {
    if (reent != NULL) _REENT_ERRNO(reent) = error;
}

void *_malloc_r(struct _reent *reent, size_t size) {
    void *ptr = maia_newlib_heap_alloc(size);
    if (ptr == NULL && size != 0U) set_reent_errno(reent, ENOMEM);
    return ptr;
}

void _free_r(struct _reent *reent, void *ptr) {
    (void)reent;
    maia_newlib_heap_free(ptr);
}

void *_calloc_r(struct _reent *reent, size_t count, size_t size) {
    void *ptr = maia_newlib_heap_calloc(count, size);
    if (ptr == NULL && count != 0U && size != 0U) {
        set_reent_errno(reent, ENOMEM);
    }
    return ptr;
}

void *_realloc_r(struct _reent *reent, void *ptr, size_t size) {
    void *replacement = maia_newlib_heap_realloc(ptr, size);
    if (replacement == NULL && size != 0U) set_reent_errno(reent, ENOMEM);
    return replacement;
}

size_t _malloc_usable_size_r(struct _reent *reent, void *ptr) {
    (void)reent;
    return maia_newlib_heap_usable_size(ptr);
}

void *malloc(size_t size) {
    return _malloc_r(_REENT, size);
}

void free(void *ptr) {
    _free_r(_REENT, ptr);
}

void *calloc(size_t count, size_t size) {
    return _calloc_r(_REENT, count, size);
}

void *realloc(void *ptr, size_t size) {
    return _realloc_r(_REENT, ptr, size);
}

size_t malloc_usable_size(void *ptr) {
    return _malloc_usable_size_r(_REENT, ptr);
}
#endif
