#ifndef MAIA_RUNTIME_NEWLIB_HEAP_H
#define MAIA_RUNTIME_NEWLIB_HEAP_H

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

void *maia_newlib_heap_alloc(size_t size);
void *maia_newlib_heap_calloc(size_t count, size_t size);
void *maia_newlib_heap_realloc(void *ptr, size_t size);
void maia_newlib_heap_free(void *ptr);
size_t maia_newlib_heap_usable_size(void *ptr);

#ifdef __cplusplus
}
#endif

#endif
