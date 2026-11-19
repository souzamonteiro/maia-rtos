#ifndef MAIA_RUNTIME_NEWLIB_REENT_H
#define MAIA_RUNTIME_NEWLIB_REENT_H

#include <maia/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

struct _reent *__getreent(void);
void maia_newlib_task_reclaim(maia_task_id_t task_id);

#ifdef __cplusplus
}
#endif

#endif
