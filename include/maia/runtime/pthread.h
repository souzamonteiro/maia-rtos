#ifndef MAIA_RUNTIME_PTHREAD_H
#define MAIA_RUNTIME_PTHREAD_H

#include <maia/kernel.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Called on the dedicated IRQ stack after the scheduler marks a task EXITED. */
void maia_pthread_task_exit_complete(maia_task_id_t task_id);

#ifdef __cplusplus
}
#endif

#endif
