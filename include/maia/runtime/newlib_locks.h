#ifndef MAIA_RUNTIME_NEWLIB_LOCKS_H
#define MAIA_RUNTIME_NEWLIB_LOCKS_H

#include <maia/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Call once after maia_mem_init and before tasks may allocate Newlib locks. */
maia_status_t maia_newlib_locks_init(void);

#ifdef __cplusplus
}
#endif

#endif
