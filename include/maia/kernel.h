#ifndef MAIA_KERNEL_H
#define MAIA_KERNEL_H

/*
 * MaiaRTOS Core Kernel Public API (M1..M5)
 *
 * Clean-room bare-metal RTOS kernel interface.
 * Strictly zero FreeRTOS, ESP-IDF, Arduino, or MaiaWASMVM dependencies.
 * Compatible with C11 and C++98 (-Wall -Wextra -Wpedantic -Werror).
 */

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ==========================================================================
 * Status Codes
 * ========================================================================== */

typedef enum maia_status {
    MAIA_OK          =  0,
    MAIA_ERR_INVALID = -1,
    MAIA_ERR_NOMEM   = -2,
    MAIA_ERR_TIMEOUT = -3,
    MAIA_ERR_BUSY    = -4,
    MAIA_ERR_FULL    = -5,
    MAIA_ERR_EMPTY   = -6,
    MAIA_ERR_PERM    = -7
} maia_status_t;

/* ==========================================================================
 * Trap, IRQ & Time API (M1)
 * ========================================================================== */

typedef struct maia_trap_stats {
    uint32_t total_traps;
    uint32_t timer_irqs;
    uint32_t external_irqs;
    uint32_t sync_exceptions;
    uint32_t ecall_count;
    uint32_t last_mcause;
    uint32_t last_mepc;
    uint32_t last_mtval;
} maia_trap_stats_t;

uint64_t maia_tick_get(void);
uint64_t maia_time_us(void);
void     maia_trap_get_stats(maia_trap_stats_t *out_stats);
void     maia_trap_record_event(uint32_t mcause, uint32_t mepc, uint32_t mtval);

/* ==========================================================================
 * Deterministic Region Memory Allocator (M4)
 * ========================================================================== */

typedef struct maia_mem_stats {
    size_t   total_bytes;
    size_t   used_bytes;
    size_t   free_bytes;
    size_t   peak_used_bytes;
    uint32_t alloc_count;
    uint32_t free_count;
} maia_mem_stats_t;

maia_status_t maia_mem_init(void *pool_base, size_t pool_bytes);
void         *maia_mem_alloc(size_t size);
void         *maia_mem_alloc_aligned(size_t size, size_t alignment);
maia_status_t maia_mem_free(void *ptr);
maia_status_t maia_mem_get_stats(maia_mem_stats_t *out_stats);

/* ==========================================================================
 * Synchronization Primitives (M5)
 * ========================================================================== */

typedef struct maia_mutex {
    uint32_t locked;
    int32_t  owner_task_id;
    uint32_t lock_count;
} maia_mutex_t;

typedef struct maia_sem {
    uint32_t count;
    uint32_t max_count;
} maia_sem_t;

typedef struct maia_queue {
    uint8_t *buffer;
    size_t   item_size;
    size_t   capacity;
    size_t   head;
    size_t   tail;
    size_t   count;
} maia_queue_t;

maia_status_t maia_mutex_init(maia_mutex_t *mtx);
maia_status_t maia_mutex_trylock(maia_mutex_t *mtx, int32_t task_id);
maia_status_t maia_mutex_unlock(maia_mutex_t *mtx, int32_t task_id);

maia_status_t maia_sem_init(maia_sem_t *sem, uint32_t initial_count, uint32_t max_count);
maia_status_t maia_sem_trywait(maia_sem_t *sem);
maia_status_t maia_sem_post(maia_sem_t *sem);
uint32_t      maia_sem_getvalue(const maia_sem_t *sem);

maia_status_t maia_queue_init(maia_queue_t *q, void *storage, size_t item_size, size_t capacity);
maia_status_t maia_queue_send(maia_queue_t *q, const void *item);
maia_status_t maia_queue_recv(maia_queue_t *q, void *out_item);
size_t        maia_queue_count(const maia_queue_t *q);

/* ==========================================================================
 * Task & Preemptive/Cooperative Scheduler API (M2)
 * ========================================================================== */

#define MAIA_MAX_TASKS      16U
#define MAIA_TASK_NAME_MAX  16U
#define MAIA_PRIO_IDLE      0U
#define MAIA_PRIO_LOW       1U
#define MAIA_PRIO_NORMAL    2U
#define MAIA_PRIO_HIGH      3U
#define MAIA_PRIO_REALTIME  7U

typedef int32_t maia_task_id_t;
typedef void (*maia_task_entry_fn)(void *arg);

typedef enum maia_task_state {
    MAIA_TASK_UNUSED   = 0,
    MAIA_TASK_READY    = 1,
    MAIA_TASK_RUNNING  = 2,
    MAIA_TASK_SLEEPING = 3,
    MAIA_TASK_BLOCKED  = 4,
    MAIA_TASK_EXITED   = 5
} maia_task_state_t;

typedef struct maia_task_info {
    maia_task_id_t    id;
    char              name[MAIA_TASK_NAME_MAX];
    uint8_t           priority;
    maia_task_state_t state;
    size_t            stack_size;
    uint32_t          run_ticks;
    uint32_t          switches;
    uint64_t          wake_tick;
} maia_task_info_t;

maia_status_t  maia_sched_init(void);
/* Thread/boot context only; not ISR-safe. Caller owns stack_mem for task lifetime.
 * Returns INVALID for null entry/stack, a stack smaller than the architecture
 * minimum (256 integer, 400 RV32F), or pointer+length overflow; NOMEM when full.
 * Minimum size is not a workload stack bound; allow for actual C/interrupt depth.
 */
maia_status_t  maia_task_create(const char *name,
                                maia_task_entry_fn entry,
                                void *arg,
                                uint8_t priority,
                                void *stack_mem,
                                size_t stack_size,
                                maia_task_id_t *out_id);
void           maia_task_yield(void);
void           maia_task_delay_ticks(uint32_t ticks);
maia_task_id_t maia_task_current_id(void);
uint32_t       maia_task_count(void);
uint32_t       maia_sched_switch_count(void);
maia_status_t  maia_task_get_info(uint32_t index, maia_task_info_t *out_info);

/* Called by timer ISR (or host test harness) on each system tick */
int            maia_sched_tick_isr(void);

/* Selects next runnable task and returns its saved stack pointer (for trap/context switch) */
uintptr_t      maia_sched_select_next_sp(uintptr_t current_sp);

/* Starts scheduling the highest-priority ready task */
void           maia_sched_start(void);

#ifdef __cplusplus
}
#endif

#endif /* MAIA_KERNEL_H */

