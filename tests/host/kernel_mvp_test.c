#include <maia/kernel.h>
#include <maia_shell/shell_mvp.h>
#include <stdio.h>
#include <string.h>

static void dummy_task(void *arg) {
    (void)arg;
}

typedef struct mock_console {
    char   out[2048];
    size_t len;
} mock_console_t;

static int32_t mock_read(void *ctx, msh_fd_t fd, void *buf, uint32_t len) {
    (void)ctx;
    (void)fd;
    (void)buf;
    (void)len;
    return MSH_ERR_AGAIN;
}

static int32_t mock_write(void *ctx, msh_fd_t fd, const void *buf, uint32_t len) {
    (void)fd;
    mock_console_t *mc = (mock_console_t *)ctx;
    const char *cbuf = (const char *)buf;
    for (size_t i = 0; i < (size_t)len && mc->len + 1U < sizeof(mc->out); ++i) {
        mc->out[mc->len++] = cbuf[i];
    }
    mc->out[mc->len] = '\0';
    return (int32_t)len;
}

int main(void) {
    /* 1. M4 Region Allocator positive & negative tests */
    uint8_t pool[4096];
    if (maia_mem_init(NULL, 1024) != MAIA_ERR_INVALID) return 1;
    if (maia_mem_init(pool, 16) != MAIA_ERR_INVALID) return 2;
    if (maia_mem_init(pool, sizeof(pool)) != MAIA_OK) return 3;

    void *p1 = maia_mem_alloc(128);
    void *p2 = maia_mem_alloc_aligned(256, 64);
    if (p1 == NULL || p2 == NULL) return 4;
    if (((uintptr_t)p2 & 63U) != 0U) return 5;
    if (maia_mem_alloc_aligned(64, 3) != NULL) return 6; /* non-power-of-two alignment */

    maia_mem_stats_t ms;
    if (maia_mem_get_stats(&ms) != MAIA_OK) return 7;
    if (ms.alloc_count != 2U || ms.used_bytes < 384U) return 8;

    if (maia_mem_free(p1) != MAIA_OK) return 9;
    if (maia_mem_free(p1) != MAIA_ERR_INVALID) return 10; /* double-free rejected */
    if (maia_mem_free(p2) != MAIA_OK) return 11;
    (void)maia_mem_get_stats(&ms);
    if (ms.used_bytes != 0U || ms.free_bytes != ms.total_bytes) return 12;

    /* 2. M5 Mutex, Semaphore & Queue tests */
    maia_mutex_t mtx;
    if (maia_mutex_init(&mtx) != MAIA_OK) return 13;
    if (maia_mutex_trylock(&mtx, 1) != MAIA_OK) return 14;
    if (maia_mutex_trylock(&mtx, 2) != MAIA_ERR_BUSY) return 15;
    if (maia_mutex_unlock(&mtx, 2) != MAIA_ERR_PERM) return 16;
    if (maia_mutex_unlock(&mtx, 1) != MAIA_OK) return 17;

    maia_sem_t sem;
    if (maia_sem_init(&sem, 1, 2) != MAIA_OK) return 18;
    if (maia_sem_post(&sem) != MAIA_OK) return 19;
    if (maia_sem_post(&sem) != MAIA_ERR_FULL) return 20;
    if (maia_sem_trywait(&sem) != MAIA_OK) return 21;
    if (maia_sem_trywait(&sem) != MAIA_OK) return 22;
    if (maia_sem_trywait(&sem) != MAIA_ERR_EMPTY) return 23;

    uint32_t qbuf[2];
    maia_queue_t q;
    if (maia_queue_init(&q, qbuf, sizeof(uint32_t), 2) != MAIA_OK) return 24;
    uint32_t v1 = 0x11223344UL, v2 = 0x55667788UL, out = 0;
    if (maia_queue_send(&q, &v1) != MAIA_OK) return 25;
    if (maia_queue_send(&q, &v2) != MAIA_OK) return 26;
    if (maia_queue_send(&q, &v1) != MAIA_ERR_FULL) return 27;
    if (maia_queue_recv(&q, &out) != MAIA_OK || out != v1) return 28;
    if (maia_queue_recv(&q, &out) != MAIA_OK || out != v2) return 29;
    if (maia_queue_recv(&q, &out) != MAIA_ERR_EMPTY) return 30;

    /* 3. M1/M2 Trap stats & Scheduler priority/round-robin/delay tests */
    if (maia_sched_init() != MAIA_OK) return 31;
    maia_trap_record_event(3UL, 0x4FF00100UL, 0UL);
    maia_trap_record_event(0x80000010UL, 0x4FF00200UL, 0UL);
    maia_trap_stats_t ts;
    maia_trap_get_stats(&ts);
    if (ts.total_traps != 2U || ts.sync_exceptions != 1U || ts.timer_irqs != 1U) return 32;

    uint8_t s0[1024], s1[1024], s2[1024];
    maia_task_id_t t0 = -1, t1 = -1, t2 = -1;
    if (maia_task_create("idle", dummy_task, NULL, MAIA_PRIO_IDLE, s0, sizeof(s0), &t0) != MAIA_OK) return 33;
    if (maia_task_create("workA", dummy_task, NULL, MAIA_PRIO_NORMAL, s1, sizeof(s1), &t1) != MAIA_OK) return 34;
    if (maia_task_create("workB", dummy_task, NULL, MAIA_PRIO_NORMAL, s2, sizeof(s2), &t2) != MAIA_OK) return 35;

    /* Round-robin between equal higher-priority tasks t1 and t2 */
    (void)maia_sched_select_next_sp(0U);
    if (maia_task_current_id() != t1) return 36;
    (void)maia_sched_select_next_sp(0U);
    if (maia_task_current_id() != t2) return 37;
    (void)maia_sched_select_next_sp(0U);
    if (maia_task_current_id() != t1) return 38;

    /* Delay t1 for 2 ticks -> t2 runs, then delay t2 for 2 ticks -> idle (t0) runs */
    maia_task_delay_ticks(2U);
    (void)maia_sched_select_next_sp(0U);
    if (maia_task_current_id() != t2) return 39;
    maia_task_delay_ticks(2U);
    (void)maia_sched_select_next_sp(0U);
    if (maia_task_current_id() != t0) return 40;

    /* Advance 2 ticks -> t1 and t2 wake up and preempt idle */
    (void)maia_sched_tick_isr();
    if (maia_sched_tick_isr() == 0) return 41;
    (void)maia_sched_select_next_sp(0U);
    if (maia_task_current_id() != t1) return 42;

    /* 4. N2 MaiaShell MVP tests */
    mock_console_t mc;
    memset(&mc, 0, sizeof(mc));
    msh_host_v1 host;
    msh_mvp_bind_console_host(&host, &mc, mock_read, mock_write);
    msh_ctx_t sh;
    if (msh_mvp_init(&sh, &host) != MSH_OK) return 43;
    if (msh_mvp_eval_line(&sh, "set GREET=Hello_Tab5; echo $GREET; uname; ps; mem; qtest") != 0) return 44;
    if (strstr(mc.out, "Hello_Tab5") == NULL) return 45;
    if (strstr(mc.out, "qtest PASS") == NULL) return 46;

    const char *interactive = "alloc 64\n";
    for (size_t i = 0; interactive[i] != '\0'; ++i) {
        (void)msh_mvp_feed_char(&sh, interactive[i]);
    }
    if (strstr(mc.out, "alloc OK") == NULL) return 47;

    puts("MaiaRTOS kernel (M1/M2/M4/M5) + MaiaShell MVP (N2) host test PASS");
    return 0;
}
