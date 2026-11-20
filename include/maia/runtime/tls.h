#ifndef MAIA_RUNTIME_TLS_H
#define MAIA_RUNTIME_TLS_H

#include <maia/kernel.h>

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAIA_TLS_TASK_CREATE_FAILED UINTPTR_MAX

int maia_tls_boot_init(void);
uintptr_t maia_tls_idle_thread_pointer(void);
uintptr_t maia_tls_task_create(maia_task_id_t task_id);
void maia_tls_task_reclaim(maia_task_id_t task_id);

#ifdef MAIA_COMPILER_TLS_TEST
void maia_tls_test_configure(const uint8_t *tdata, size_t tdata_size,
                             size_t block_size, size_t alignment);
uintptr_t maia_tls_test_boot_thread_pointer(void);
#endif

#ifdef __cplusplus
}
#endif

#endif
