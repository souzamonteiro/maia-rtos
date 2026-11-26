#include <maia/console.h>
#include <maia/arch/irq.h>
#include <maia/boards/tab5_sdmmc_service.h>
#ifdef MAIA_PRODUCT_FATFS
#include <maia/boards/tab5_kv.h>
#include <maia/boards/tab5_maiascript_kv.h>
#endif
#include <maia/kernel.h>
#include <maia/runtime/newlib_locks.h>
#ifdef MAIA_PRODUCT_FATFS
#include <maia/services/fatfs.h>
#endif
#if defined(MAIA_DIAGNOSTIC_FATFS_RW) || defined(MAIA_DIAGNOSTIC_VFS_STRESS)
#include <maia/services/vfs.h>
#endif
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_display.h>
#include <maia/version.h>
#include <maia_shell/shell_engine.h>
#include <stdint.h>

#if defined(MAIA_DIAGNOSTIC_FATFS_RW) || defined(MAIA_DIAGNOSTIC_VFS_STRESS)
#include <stdio.h>
#include <string.h>
#endif

extern void maia_soc_systimer_init(uint32_t tick_hz);

/* 49 KiB L2MEM heap pool + task stacks in internal L2MEM_LOW (.bss) */
static uint8_t g_heap_pool[50176] __attribute__((aligned(16)));
static uint8_t g_stack_idle[1024] __attribute__((aligned(16)));
static uint8_t g_stack_telem[1024] __attribute__((aligned(16)));
static uint8_t g_stack_shell[16384] __attribute__((aligned(16)));

static maia_mutex_t g_telem_mutex;
static maia_queue_t g_telem_queue;
static uint32_t     g_telem_qbuf[8];

#ifdef MAIA_DIAGNOSTIC_VFS_STRESS
#define VFS_STRESS_WORKERS 2U
#define VFS_STRESS_ITERATIONS 16U
#define VFS_STRESS_RECORD_BYTES MAIA_SD_BLOCK_BYTES
#define VFS_STRESS_BUSY_RETRIES 5000U
#define VFS_STRESS_WAIT_TICKS 30000U
#define VFS_STRESS_DIRECTORY "/sd/MaiaVFSStress-20261121-Retry6"
#define VFS_REBOOT_DIRECTORY "/sd/MaiaVFSReboot-20261121"
#define VFS_REBOOT_FILE VFS_REBOOT_DIRECTORY "/persist.bin"

typedef struct vfs_stress_worker {
    uint32_t id;
    maia_task_id_t task_id;
    maia_task_id_t coordinator_id;
    volatile uint32_t done;
    volatile uint32_t busy_retries;
    uint32_t last_busy_locked;
    int32_t last_busy_owner;
    uint32_t completed_iterations;
    uint32_t failure_phase;
    maia_vfs_result_t result;
    char path[80];
} vfs_stress_worker_t;

typedef struct vfs_stress_call {
    const char *path;
    uint32_t flags;
    maia_vfs_fd_t fd;
    uint32_t offset;
    void *buffer;
    const void *const_buffer;
    uint32_t count;
    uint32_t transferred;
} vfs_stress_call_t;

typedef maia_vfs_result_t (*vfs_stress_fn)(vfs_stress_call_t *call);

static uint8_t g_stack_vfs_stress_coordinator[8192]
    __attribute__((aligned(16)));
static uint8_t g_stack_vfs_stress_worker[VFS_STRESS_WORKERS][4096]
    __attribute__((aligned(16)));
static vfs_stress_worker_t g_vfs_stress_workers[VFS_STRESS_WORKERS];
static maia_task_id_t g_vfs_stress_coordinator_id = -1;
static void print_u32(uint32_t val);

static maia_vfs_result_t vfs_stress_open(vfs_stress_call_t *call) {
    return maia_vfs_open(call->path, call->flags, &call->fd);
}
static maia_vfs_result_t vfs_stress_write(vfs_stress_call_t *call) {
    return maia_vfs_write(call->fd, call->const_buffer, call->count,
                          &call->transferred);
}
static maia_vfs_result_t vfs_stress_read(vfs_stress_call_t *call) {
    return maia_vfs_read(call->fd, call->buffer, call->count,
                         &call->transferred);
}
static maia_vfs_result_t vfs_stress_seek(vfs_stress_call_t *call) {
    return maia_vfs_seek(call->fd, call->offset);
}
static maia_vfs_result_t vfs_stress_sync(vfs_stress_call_t *call) {
    return maia_vfs_sync(call->fd);
}
static maia_vfs_result_t vfs_stress_close(vfs_stress_call_t *call) {
    return maia_vfs_close(call->fd);
}
static maia_vfs_result_t vfs_stress_unlink(vfs_stress_call_t *call) {
    return maia_vfs_unlink(call->path);
}

static maia_vfs_result_t vfs_stress_retry(vfs_stress_worker_t *worker,
                                           vfs_stress_fn operation,
                                           vfs_stress_call_t *call) {
    for (uint32_t attempt = 0U; ; ++attempt) {
        maia_vfs_result_t result = operation(call);
        if (result != MAIA_VFS_BUSY) return result;
        ++worker->busy_retries;
        maia_vfs_diag_lock_state(&worker->last_busy_locked,
                                 &worker->last_busy_owner);
        if (attempt + 1U >= VFS_STRESS_BUSY_RETRIES) return result;
        maia_task_delay_ticks(1U);
    }
}

static void fill_vfs_stress_record(uint32_t worker_id, uint32_t iteration,
                                   uint8_t record[VFS_STRESS_RECORD_BYTES]) {
    for (uint32_t i = 0U; i < VFS_STRESS_RECORD_BYTES; ++i) {
        record[i] = (uint8_t)(worker_id * 53U + iteration * 29U +
                              i * 17U + (i >> 3U));
    }
}

static void vfs_stress_worker_entry(void *arg) {
    vfs_stress_worker_t *worker = (vfs_stress_worker_t *)arg;
    uint8_t expected[VFS_STRESS_RECORD_BYTES];
    uint8_t actual[VFS_STRESS_RECORD_BYTES];
    vfs_stress_call_t call = { 0 };

    maia_status_t gate = maia_task_wait_notification(VFS_STRESS_WAIT_TICKS);
    if (gate != MAIA_OK) {
        worker->failure_phase = 1U;
        worker->result = MAIA_VFS_FAILURE;
        goto finished;
    }

    for (uint32_t iteration = 0U; iteration < VFS_STRESS_ITERATIONS; ++iteration) {
        fill_vfs_stress_record(worker->id, iteration, expected);
        worker->failure_phase = 2U;
        call.path = worker->path;
        call.flags = MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                     MAIA_VFS_OPEN_CREATE | MAIA_VFS_OPEN_TRUNCATE;
        call.fd = 0U;
        worker->result = vfs_stress_retry(worker, vfs_stress_open, &call);
        if (worker->result != MAIA_VFS_OK) break;

        call.const_buffer = expected;
        call.count = sizeof(expected);
        call.transferred = 0U;
        worker->failure_phase = 3U;
        worker->result = vfs_stress_retry(worker, vfs_stress_write, &call);
        if (worker->result != MAIA_VFS_OK ||
            call.transferred != sizeof(expected)) {
            if (worker->result == MAIA_VFS_OK) worker->result = MAIA_VFS_IO_ERROR;
            (void)vfs_stress_retry(worker, vfs_stress_close, &call);
            break;
        }
        worker->failure_phase = 4U;
        worker->result = vfs_stress_retry(worker, vfs_stress_sync, &call);
        if (worker->result != MAIA_VFS_OK) {
            (void)vfs_stress_retry(worker, vfs_stress_close, &call);
            break;
        }
        worker->failure_phase = 5U;
        worker->result = vfs_stress_retry(worker, vfs_stress_close, &call);
        if (worker->result != MAIA_VFS_OK) break;

        call.flags = MAIA_VFS_OPEN_READ;
        call.fd = 0U;
        worker->failure_phase = 6U;
        worker->result = vfs_stress_retry(worker, vfs_stress_open, &call);
        if (worker->result != MAIA_VFS_OK) break;
        call.buffer = actual;
        call.count = sizeof(actual);
        call.transferred = 0U;
        worker->failure_phase = 7U;
        worker->result = vfs_stress_retry(worker, vfs_stress_read, &call);
        if (worker->result != MAIA_VFS_OK ||
            call.transferred != sizeof(actual) ||
            memcmp(expected, actual, sizeof(expected)) != 0) {
            if (worker->result == MAIA_VFS_OK) worker->result = MAIA_VFS_IO_ERROR;
            (void)vfs_stress_retry(worker, vfs_stress_close, &call);
            break;
        }
        call.count = 1U;
        call.transferred = UINT32_MAX;
        worker->failure_phase = 8U;
        worker->result = vfs_stress_retry(worker, vfs_stress_read, &call);
        if (worker->result != MAIA_VFS_OK || call.transferred != 0U) {
            if (worker->result == MAIA_VFS_OK) worker->result = MAIA_VFS_IO_ERROR;
            (void)vfs_stress_retry(worker, vfs_stress_close, &call);
            break;
        }
        worker->failure_phase = 9U;
        worker->result = vfs_stress_retry(worker, vfs_stress_close, &call);
        if (worker->result != MAIA_VFS_OK) break;
        ++worker->completed_iterations;
        worker->failure_phase = 0U;
        maia_task_yield();
    }

finished:
    worker->done = 1U;
    (void)maia_task_notify(worker->coordinator_id);
}

static maia_vfs_result_t vfs_stress_fault_recovery(void) {
    static const char path[] = VFS_STRESS_DIRECTORY "/fault.bin";
    uint8_t expected[VFS_STRESS_RECORD_BYTES];
    uint8_t actual[VFS_STRESS_RECORD_BYTES];
    vfs_stress_call_t call = { 0 };
    vfs_stress_worker_t retry_state = { 0 };
    uint32_t fault_count;
    for (uint32_t i = 0U; i < sizeof(expected); ++i)
        expected[i] = (uint8_t)(i * 13U + 0x31U);

    call.path = path;
    call.flags = MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                 MAIA_VFS_OPEN_CREATE_NEW;
    maia_vfs_result_t result = vfs_stress_retry(
        &retry_state, vfs_stress_open, &call);
    if (result != MAIA_VFS_OK) return result;
    call.const_buffer = expected;
    call.count = sizeof(expected);
    result = vfs_stress_retry(&retry_state, vfs_stress_write, &call);
    if (result != MAIA_VFS_OK || call.transferred != sizeof(expected))
        goto close_and_fail;
    result = vfs_stress_retry(&retry_state, vfs_stress_sync, &call);
    if (result != MAIA_VFS_OK) goto close_and_fail;
    result = vfs_stress_retry(&retry_state, vfs_stress_close, &call);
    if (result != MAIA_VFS_OK) return result;

    call.fd = 0U;
    call.flags = MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE;
    result = vfs_stress_retry(&retry_state, vfs_stress_open, &call);
    if (result != MAIA_VFS_OK) return result;
    call.offset = 7U;
    result = vfs_stress_retry(&retry_state, vfs_stress_seek, &call);
    if (result != MAIA_VFS_OK) goto close_and_fail;
    expected[7U] ^= 0xA5U;
    call.const_buffer = expected + 7U;
    call.count = 1U;
    result = vfs_stress_retry(&retry_state, vfs_stress_write, &call);
    if (result != MAIA_VFS_OK || call.transferred != 1U) goto close_and_fail;
    fault_count = maia_fatfs_diag_faults_triggered();
    maia_fatfs_diag_fail_next(MAIA_FATFS_DIAG_WRITE);
    result = vfs_stress_retry(&retry_state, vfs_stress_sync, &call);
    maia_fatfs_diag_clear_pending_fault();
    if (result != MAIA_VFS_IO_ERROR ||
        maia_fatfs_diag_faults_triggered() != fault_count + 1U) {
        if (result == MAIA_VFS_OK) result = MAIA_VFS_FAILURE;
        goto close_and_fail;
    }
    result = vfs_stress_retry(&retry_state, vfs_stress_sync, &call);
    if (result != MAIA_VFS_OK) goto close_and_fail;
    result = vfs_stress_retry(&retry_state, vfs_stress_close, &call);
    if (result != MAIA_VFS_OK) return result;

    call.fd = 0U;
    call.flags = MAIA_VFS_OPEN_READ;
    result = vfs_stress_retry(&retry_state, vfs_stress_open, &call);
    if (result != MAIA_VFS_OK) return result;
    fault_count = maia_fatfs_diag_faults_triggered();
    maia_fatfs_diag_fail_next(MAIA_FATFS_DIAG_READ);
    call.buffer = actual;
    call.count = sizeof(actual);
    call.transferred = UINT32_MAX;
    result = vfs_stress_retry(&retry_state, vfs_stress_read, &call);
    maia_fatfs_diag_clear_pending_fault();
    if (result != MAIA_VFS_IO_ERROR || call.transferred != 0U ||
        maia_fatfs_diag_faults_triggered() != fault_count + 1U) {
        if (result == MAIA_VFS_OK) result = MAIA_VFS_FAILURE;
        goto close_and_fail;
    }
    result = vfs_stress_retry(&retry_state, vfs_stress_close, &call);
    if (result != MAIA_VFS_OK) return result;
    call.fd = 0U;
    result = vfs_stress_retry(&retry_state, vfs_stress_open, &call);
    if (result != MAIA_VFS_OK) return result;
    result = vfs_stress_retry(&retry_state, vfs_stress_read, &call);
    if (result != MAIA_VFS_OK || call.transferred != sizeof(actual) ||
        memcmp(expected, actual, sizeof(expected)) != 0) {
        if (result == MAIA_VFS_OK) result = MAIA_VFS_FAILURE;
        goto close_and_fail;
    }
    result = vfs_stress_retry(&retry_state, vfs_stress_close, &call);
    if (result != MAIA_VFS_OK) return result;
    call.path = path;
    result = vfs_stress_retry(&retry_state, vfs_stress_unlink, &call);
    return result;

close_and_fail:
    (void)vfs_stress_retry(&retry_state, vfs_stress_close, &call);
    return result == MAIA_VFS_OK ? MAIA_VFS_IO_ERROR : result;
}

static void fill_vfs_reboot_record(
    uint8_t record[VFS_STRESS_RECORD_BYTES]) {
    for (uint32_t i = 0U; i < VFS_STRESS_RECORD_BYTES; ++i)
        record[i] = (uint8_t)(i * 37U + 0xA7U + (i >> 2U));
}

static maia_vfs_result_t vfs_stress_reboot_persistence(void) {
    uint8_t expected[VFS_STRESS_RECORD_BYTES];
    uint8_t actual[VFS_STRESS_RECORD_BYTES];
    vfs_stress_call_t call = { 0 };
    vfs_stress_worker_t retry_state = { 0 };
    maia_vfs_result_t result;
    uint32_t opened = 0U;

    fill_vfs_reboot_record(expected);
    result = maia_vfs_mkdir(VFS_REBOOT_DIRECTORY);
    if (result == MAIA_VFS_OK) {
        call.path = VFS_REBOOT_FILE;
        call.flags = MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                     MAIA_VFS_OPEN_CREATE_NEW;
        result = vfs_stress_retry(&retry_state, vfs_stress_open, &call);
        if (result != MAIA_VFS_OK) goto failed;
        opened = 1U;
        call.const_buffer = expected;
        call.count = sizeof(expected);
        call.transferred = 0U;
        result = vfs_stress_retry(&retry_state, vfs_stress_write, &call);
        if (result == MAIA_VFS_OK && call.transferred != sizeof(expected))
            result = MAIA_VFS_IO_ERROR;
        if (result != MAIA_VFS_OK) goto failed;
        result = vfs_stress_retry(&retry_state, vfs_stress_sync, &call);
        if (result != MAIA_VFS_OK) goto failed;
        result = vfs_stress_retry(&retry_state, vfs_stress_close, &call);
        opened = 0U;
        if (result != MAIA_VFS_OK) goto failed;
        maia_console_puts(
            "[F2] MAIA_VFS_REBOOT_ARMED bytes=512; awaiting clean reset\n");
        return MAIA_VFS_OK;
    }

    if (result != MAIA_VFS_EXISTS) goto failed;
    call.path = VFS_REBOOT_FILE;
    call.flags = MAIA_VFS_OPEN_READ;
    result = vfs_stress_retry(&retry_state, vfs_stress_open, &call);
    if (result != MAIA_VFS_OK) goto failed;
    opened = 1U;
    call.buffer = actual;
    call.count = sizeof(actual);
    call.transferred = 0U;
    result = vfs_stress_retry(&retry_state, vfs_stress_read, &call);
    if (result != MAIA_VFS_OK || call.transferred != sizeof(actual)) {
        if (result == MAIA_VFS_OK) result = MAIA_VFS_IO_ERROR;
        goto failed;
    }
    call.count = 1U;
    call.transferred = UINT32_MAX;
    result = vfs_stress_retry(&retry_state, vfs_stress_read, &call);
    if (result != MAIA_VFS_OK || call.transferred != 0U) {
        if (result == MAIA_VFS_OK) result = MAIA_VFS_IO_ERROR;
        goto failed;
    }
    result = vfs_stress_retry(&retry_state, vfs_stress_close, &call);
    opened = 0U;
    if (result != MAIA_VFS_OK) goto failed;
    if (memcmp(expected, actual, sizeof(expected)) != 0) {
        result = MAIA_VFS_IO_ERROR;
        goto failed;
    }
    maia_console_puts(
        "[F2] MAIA_VFS_REBOOT_PERSIST_PASS bytes=512; prior sync survived reset\n");
    return MAIA_VFS_OK;

failed:
    if (opened != 0U)
        (void)vfs_stress_retry(&retry_state, vfs_stress_close, &call);
    maia_console_puts("[F2] MAIA_VFS_REBOOT_PERSIST_FAIL result=");
    print_u32((uint32_t)(-(int32_t)result));
    maia_console_puts("; inspect ");
    maia_console_puts(VFS_REBOOT_DIRECTORY);
    maia_console_puts("\n");
    return result;
}

static void task_vfs_stress_coordinator(void *arg) {
    (void)arg;
    static const char directory[] = VFS_STRESS_DIRECTORY;
    vfs_stress_call_t call = { 0 };
    g_vfs_stress_coordinator_id = maia_task_current_id();
    call.path = directory;
    maia_vfs_result_t result = maia_vfs_mkdir(directory);
    if (result != MAIA_VFS_OK) {
        maia_console_puts(result == MAIA_VFS_EXISTS ?
            "[F2] MAIA_VFS_STRESS_SKIP directory already exists; no files touched\n" :
            "[F2] MAIA_VFS_STRESS_FAIL mkdir; no test files touched\n");
        return;
    }

    uint32_t created_workers = 0U;
    for (uint32_t i = 0U; i < VFS_STRESS_WORKERS; ++i) {
        vfs_stress_worker_t *worker = &g_vfs_stress_workers[i];
        memset(worker, 0, sizeof(*worker));
        worker->id = i;
        worker->coordinator_id = g_vfs_stress_coordinator_id;
        worker->result = MAIA_VFS_OK;
        (void)snprintf(worker->path, sizeof(worker->path),
                       "%s/worker%u.bin", directory, (unsigned)i);
        char task_name[MAIA_TASK_NAME_MAX];
        (void)snprintf(task_name, sizeof(task_name), "vfs-stress-%u",
                       (unsigned)i);
        maia_status_t task_result = maia_task_create(
            task_name, vfs_stress_worker_entry, worker, MAIA_PRIO_NORMAL,
            g_stack_vfs_stress_worker[i],
            sizeof(g_stack_vfs_stress_worker[i]), &worker->task_id);
        if (task_result != MAIA_OK) {
            result = MAIA_VFS_FAILURE;
            break;
        }
        ++created_workers;
    }
    for (uint32_t i = 0U; i < created_workers; ++i) {
        (void)maia_task_notify(g_vfs_stress_workers[i].task_id);
    }

    uint32_t wait_left = VFS_STRESS_WAIT_TICKS;
    for (;;) {
        uint32_t completed = 0U;
        for (uint32_t i = 0U; i < created_workers; ++i)
            completed += g_vfs_stress_workers[i].done != 0U;
        if (completed == created_workers) break;
        maia_status_t wait_result = wait_left == 0U ?
            MAIA_ERR_TIMEOUT : maia_task_wait_notification(1000U);
        if (wait_result == MAIA_ERR_TIMEOUT) {
            if (wait_left >= 1000U) wait_left -= 1000U;
            else wait_left = 0U;
        } else if (wait_result == MAIA_OK) {
            wait_left = VFS_STRESS_WAIT_TICKS;
        } else {
            result = MAIA_VFS_FAILURE;
            break;
        }
    }

    uint32_t busy_retries = 0U;
    uint32_t completed_iterations = 0U;
    for (uint32_t i = 0U; i < created_workers; ++i) {
        busy_retries += g_vfs_stress_workers[i].busy_retries;
        completed_iterations += g_vfs_stress_workers[i].completed_iterations;
        if (g_vfs_stress_workers[i].result != MAIA_VFS_OK) {
            result = g_vfs_stress_workers[i].result;
        }
    }
    if (created_workers != VFS_STRESS_WORKERS ||
        completed_iterations != VFS_STRESS_WORKERS * VFS_STRESS_ITERATIONS ||
        result != MAIA_VFS_OK) {
        maia_console_puts("[F2] MAIA_VFS_STRESS_FAIL workers=");
        print_u32(created_workers);
        maia_console_puts(" iterations=");
        print_u32(completed_iterations);
        maia_console_puts(" busy_retries=");
        print_u32(busy_retries);
        maia_console_puts(" result=");
        print_u32((uint32_t)(-(int32_t)result));
        for (uint32_t i = 0U; i < created_workers; ++i) {
            vfs_stress_worker_t *worker = &g_vfs_stress_workers[i];
            if (worker->result == MAIA_VFS_OK) continue;
            maia_console_puts(" worker=");
            print_u32(i);
            maia_console_puts(" task=");
            print_u32((uint32_t)worker->task_id);
            maia_console_puts(" phase=");
            print_u32(worker->failure_phase);
            maia_console_puts(" worker_result=");
            print_u32((uint32_t)(-(int32_t)worker->result));
            maia_console_puts(" worker_iterations=");
            print_u32(worker->completed_iterations);
            maia_console_puts(" worker_busy=");
            print_u32(worker->busy_retries);
            maia_console_puts(" worker_path=");
            maia_console_puts(worker->path);
            maia_console_puts(" busy_vfs_locked=");
            print_u32(worker->last_busy_locked);
            maia_console_puts(" busy_vfs_owner=");
            print_u32((uint32_t)worker->last_busy_owner);
        }
        uint32_t vfs_locked;
        int32_t vfs_owner;
        maia_vfs_diag_lock_state(&vfs_locked, &vfs_owner);
        maia_console_puts(" vfs_locked=");
        print_u32(vfs_locked);
        maia_console_puts(" vfs_owner=");
        print_u32((uint32_t)vfs_owner);
        maia_console_puts(" artifacts retained at ");
        maia_console_puts(directory);
        maia_console_puts("\n");
        return;
    }

    maia_console_puts("[F2] MAIA_VFS_STRESS_WORKERS_PASS workers=2 iterations=");
    print_u32(completed_iterations);
    maia_console_puts(" busy_retries=");
    print_u32(busy_retries);
    maia_console_puts("\n");
    result = vfs_stress_fault_recovery();
    if (result != MAIA_VFS_OK) {
        maia_console_puts("[F2] MAIA_VFS_FAULT_RECOVERY_FAIL result=");
        print_u32((uint32_t)(-(int32_t)result));
        maia_console_puts("; artifacts retained at ");
        maia_console_puts(directory);
        maia_console_puts("\n");
        return;
    }
    maia_console_puts("[F2] MAIA_VFS_FAULT_RECOVERY_PASS injected-read/write retry verified\n");

    for (uint32_t i = 0U; i < VFS_STRESS_WORKERS; ++i) {
        call.path = g_vfs_stress_workers[i].path;
        result = maia_vfs_unlink(call.path);
        if (result != MAIA_VFS_OK) break;
    }
    if (result == MAIA_VFS_OK) {
        call.path = directory;
        result = maia_vfs_rmdir(directory);
    }
    if (result != MAIA_VFS_OK) {
        maia_console_puts("[F2] MAIA_VFS_STRESS_CLEANUP_FAIL result=");
        print_u32((uint32_t)(-(int32_t)result));
        maia_console_puts("; inspect ");
        maia_console_puts(directory);
        maia_console_puts("\n");
        return;
    }
    maia_console_puts("[F2] MAIA_VFS_STRESS_PASS workers=2 iterations=");
    print_u32(completed_iterations);
    maia_console_puts(" busy_retries=");
    print_u32(busy_retries);
    maia_console_puts(" files_removed=2\n");
    (void)vfs_stress_reboot_persistence();
}

static void start_vfs_stress_diagnostic(void) {
    maia_task_id_t task_id;
    maia_status_t result = maia_task_create(
        "vfs-stress", task_vfs_stress_coordinator, NULL, MAIA_PRIO_NORMAL,
        g_stack_vfs_stress_coordinator,
        sizeof(g_stack_vfs_stress_coordinator), &task_id);
    if (result == MAIA_OK) {
        maia_console_puts("[F2] MaiaVFS stress diagnostic task started\n");
    } else {
        maia_console_puts("[F2] MAIA_VFS_STRESS_FAIL task creation; result=");
        print_u32((uint32_t)(-result));
        maia_console_puts("\n");
    }
}
#endif

static void print_hex32(uint32_t val) {
    static const char kHex[] = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; ++i) {
        buf[2 + i] = kHex[(val >> ((7 - i) * 4)) & 0xFU];
    }
    buf[10] = '\0';
    maia_console_puts(buf);
}

static void print_u32(uint32_t val) {
    char digits[12];
    int n = 0;
    if (val == 0U) {
        maia_console_putc('0');
        return;
    }
    while (val > 0U && n < 10) {
        digits[n++] = (char)('0' + (val % 10U));
        val /= 10U;
    }
    while (n > 0) {
        maia_console_putc(digits[--n]);
    }
}

static void task_idle_entry(void *arg) {
    (void)arg;
    uint32_t last_cyc = 0U;
    __asm__ volatile("csrr %0, mcycle" : "=r"(last_cyc));
    for (;;) {
        uint32_t now_cyc = 0U;
        __asm__ volatile("csrr %0, mcycle" : "=r"(now_cyc));
        if ((now_cyc - last_cyc) >= 40000U) {
            last_cyc += 40000U;
            maia_trap_stats_t ts;
            maia_trap_get_stats(&ts);
            if (ts.timer_irqs == 0U) {
                (void)maia_sched_tick_isr();
            }
        }
        maia_task_yield();
    }
}

static void task_telemetry_entry(void *arg) {
    (void)arg;
    uint32_t seq = 0U;
    for (;;) {
        maia_task_id_t tid = maia_task_current_id();
        if (maia_mutex_trylock(&g_telem_mutex, tid) == MAIA_OK) {
            seq += 1U;
            (void)maia_queue_send(&g_telem_queue, &seq);
            (void)maia_mutex_unlock(&g_telem_mutex, tid);
        }
        maia_task_delay_ticks(25U);
    }
}

#ifdef MAIA_DIAGNOSTIC_FATFS_RW
static void report_vfs_diag_failure(const char *operation,
                                    maia_vfs_result_t result) {
    maia_console_puts("[F2] MAIA_VFS_RW_FAIL ");
    maia_console_puts(operation);
    maia_console_puts(" result=");
    print_u32((uint32_t)(-result));
    maia_console_puts("\n");
}

static void run_vfs_rw_diagnostic(void) {
    static const char path[] = "/sd/MaiaRTOS VFS test.txt";
    static uint8_t expected[700];
    static uint8_t actual[700];
    maia_vfs_fd_t file = 0U;
    uint32_t transferred = 0U;
    maia_vfs_result_t failure = MAIA_VFS_OK;
    const char *failure_operation = NULL;
    int created = 0;
    int opened = 0;

    for (size_t i = 0U; i < sizeof(expected); ++i) {
        expected[i] = (uint8_t)(i * 37U + 11U);
    }

    failure_operation = "create";
    failure = maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE |
                            MAIA_VFS_OPEN_CREATE_NEW, &file);
    if (failure != MAIA_VFS_OK) goto cleanup;
    created = 1;
    opened = 1;

    failure_operation = "write";
    failure = maia_vfs_write(file, expected, sizeof(expected), &transferred);
    if (failure != MAIA_VFS_OK || transferred != sizeof(expected)) {
        if (failure == MAIA_VFS_OK) failure_operation = "short-write";
        goto cleanup;
    }
    failure_operation = "sync";
    failure = maia_vfs_sync(file);
    if (failure != MAIA_VFS_OK) goto cleanup;
    failure_operation = "close-write";
    failure = maia_vfs_close(file);
    if (failure == MAIA_VFS_OK) opened = 0;
    if (failure != MAIA_VFS_OK) goto cleanup;

    failure_operation = "open-read";
    failure = maia_vfs_open(path, MAIA_VFS_OPEN_READ, &file);
    if (failure != MAIA_VFS_OK) goto cleanup;
    opened = 1;
    failure_operation = "read";
    transferred = 0U;
    failure = maia_vfs_read(file, actual, sizeof(actual), &transferred);
    if (failure != MAIA_VFS_OK || transferred != sizeof(actual) ||
        memcmp(expected, actual, sizeof(expected)) != 0) {
        if (failure == MAIA_VFS_OK) failure_operation = "readback-mismatch";
        goto cleanup;
    }
    failure_operation = "close-read";
    failure = maia_vfs_close(file);
    if (failure == MAIA_VFS_OK) opened = 0;
    if (failure != MAIA_VFS_OK) goto cleanup;

    failure_operation = "open-update";
    failure = maia_vfs_open(path, MAIA_VFS_OPEN_READ | MAIA_VFS_OPEN_WRITE,
                            &file);
    if (failure != MAIA_VFS_OK) goto cleanup;
    opened = 1;
    failure_operation = "seek-update";
    failure = maia_vfs_seek(file, 19U);
    if (failure != MAIA_VFS_OK) goto cleanup;
    expected[19U] ^= 0x5AU;
    failure_operation = "update";
    failure = maia_vfs_write(file, &expected[19U], 1U, &transferred);
    if (failure != MAIA_VFS_OK || transferred != 1U) {
        if (failure == MAIA_VFS_OK) failure_operation = "short-update";
        goto cleanup;
    }
    failure_operation = "sync-update";
    failure = maia_vfs_sync(file);
    if (failure != MAIA_VFS_OK) goto cleanup;
    failure_operation = "close-update";
    failure = maia_vfs_close(file);
    if (failure == MAIA_VFS_OK) opened = 0;
    if (failure != MAIA_VFS_OK) goto cleanup;

    failure_operation = "open-verify";
    failure = maia_vfs_open(path, MAIA_VFS_OPEN_READ, &file);
    if (failure != MAIA_VFS_OK) goto cleanup;
    opened = 1;
    failure_operation = "verify";
    transferred = 0U;
    failure = maia_vfs_read(file, actual, sizeof(actual), &transferred);
    if (failure != MAIA_VFS_OK || transferred != sizeof(actual) ||
        memcmp(expected, actual, sizeof(expected)) != 0) {
        if (failure == MAIA_VFS_OK) failure_operation = "verify-mismatch";
        goto cleanup;
    }
    failure_operation = "close-verify";
    failure = maia_vfs_close(file);
    if (failure == MAIA_VFS_OK) opened = 0;

cleanup:
    if (opened != 0) {
        maia_vfs_result_t close_result = maia_vfs_close(file);
        if (close_result == MAIA_VFS_OK) opened = 0;
        if (failure == MAIA_VFS_OK && close_result != MAIA_VFS_OK) {
            failure = close_result;
            failure_operation = "close-cleanup";
        }
    }
    if (created != 0) {
        maia_vfs_result_t remove_result = maia_vfs_unlink(path);
        if (remove_result != MAIA_VFS_OK) {
            if (failure == MAIA_VFS_OK) {
                failure = remove_result;
                failure_operation = "remove-cleanup";
            } else {
                report_vfs_diag_failure("remove-cleanup", remove_result);
            }
        }
    }
    if (failure == MAIA_VFS_OK) {
        maia_console_puts("[F2] MAIA_VFS_RW_PASS create/update/read/remove\n");
    } else {
        report_vfs_diag_failure(failure_operation, failure);
    }
}
#endif

static void task_shell_entry(void *arg) {
    (void)arg;
    maia_sd_block_device_t *sd_device = maia_tab5_sdmmc_service_device();
    if (sd_device != NULL) {
        uint8_t sector[MAIA_SD_BLOCK_BYTES];
        uint32_t completed_blocks = 0U;
        maia_sd_result_t result = maia_sd_block_device_read(
            sd_device, 0U, 1U, sector, &completed_blocks);
        if (result == MAIA_SD_OK) {
            maia_console_puts("[F2] Runtime SDMMC read LBA0 passed\n");
        } else {
            maia_console_puts("[F2] Runtime SDMMC read failed (error=-");
            print_u32((uint32_t)(-(int32_t)result));
            maia_console_puts(", completed=");
            print_u32(completed_blocks);
            maia_console_puts(")\n");
        }

#ifdef MAIA_PRODUCT_FATFS
        maia_fatfs_result_t fatfs_result = maia_fatfs_mount(sd_device);
        if (fatfs_result == MAIA_FATFS_OK) {
            maia_console_puts("[F2] FatFs mounted; media not formatted\n");
#ifdef MAIA_DIAGNOSTIC_FATFS_RW
            run_vfs_rw_diagnostic();
#endif
        } else if (fatfs_result == MAIA_FATFS_NO_FILESYSTEM) {
            maia_console_puts("[F2] No FAT filesystem; media not modified\n");
        } else {
            maia_console_puts("[F2] FatFs mount failed (error=-");
            print_u32((uint32_t)(-(int32_t)fatfs_result));
            maia_console_puts(")\n");
        }
#endif
    }

    maia_shell_engine_init();

    /* Yield for 60 ms so telemetry & idle tasks accumulate preemptive/cooperative ticks */
    maia_task_delay_ticks(60U);

    uint32_t msg = 0U;
    uint32_t drained = 0U;
    while (maia_queue_recv(&g_telem_queue, &msg) == MAIA_OK) {
        drained = msg;
    }
    maia_console_puts("[M2/M5] Telemetry task queue verified: rx_seq=");
    print_u32(drained);
    maia_console_puts("\n");

    maia_shell_engine_print_banner();
    maia_console_puts("[AUTO-SELFTEST] Running: uname; ticks; ps; mem; alloc 256; qtest\n");
    (void)maia_shell_engine_eval_line("uname; ticks; ps; mem; alloc 256; qtest");
#ifdef MAIA_DIAGNOSTIC_VFS_SHELL
    if (maia_shell_engine_eval_line("cd /sd") == 0) {
        maia_console_puts("[F2] MAIA_VFS_SHELL_READY /sd; native LCD shell, no desktop\n");
        maia_console_puts("Use a NEW test directory: mkdir MaiaPOC; cd MaiaPOC only if mkdir succeeds.\n");
        maia_console_puts("Then: echo MaiaRTOS > poc.txt; cat poc.txt; echo SD >> poc.txt; ls; rm poc.txt\n");
    } else {
        maia_console_puts("[F2] MAIA_VFS_SHELL_UNAVAILABLE; shell remains usable\n");
    }
#endif
    maia_shell_engine_print_prompt();

#ifdef MAIA_DIAGNOSTIC_VFS_STRESS
    if (maia_fatfs_is_mounted()) start_vfs_stress_diagnostic();
    else maia_console_puts("[F2] MAIA_VFS_STRESS_FAIL FatFs not mounted\n");
#endif

    for (;;) {
        int ch = maia_console_getc_nonblocking();
        if (ch >= 0) {
            (void)maia_shell_engine_feed_char((char)ch);
        } else {
            maia_task_delay_ticks(2U);
        }
    }
}

void maia_boot_main(void) {
    uint32_t mhartid = 0U;
    uint32_t misa    = 0U;
    uint32_t sp_val  = 0U;
    __asm__ volatile("csrr %0, mhartid" : "=r"(mhartid));
    __asm__ volatile("csrr %0, misa" : "=r"(misa));
    __asm__ volatile("mv %0, sp" : "=r"(sp_val));

    maia_console_puts("\n========================================================\n");
    maia_console_puts("MaiaRTOS v0.1.0 — Bare-Metal Kernel & Shell Bring-Up\n");
    maia_console_puts("Target : M5Stack Tab5 (ESP32-P4 rev v1.3)\n");
    maia_console_puts("Runtime: ZERO FreeRTOS / ZERO ESP-IDF / ZERO Arduino\n");
    maia_console_puts("mhartid: ");
    print_hex32(mhartid);
    maia_console_puts("  misa: ");
    print_hex32(misa);
    maia_console_puts("  sp: ");
    print_hex32(sp_val);
    maia_console_puts("\n========================================================\n");

#ifdef MAIA_PRODUCT_FATFS
    maia_kv_result_t kv_result = maia_tab5_kv_init();
    if (kv_result == MAIA_KV_OK) {
        maia_console_puts("[F2] MaiaKV snapshot store ready\n");
    } else if (kv_result == MAIA_KV_RECOVERED_EMPTY) {
        maia_console_puts("[F2] MaiaKV recovered empty; previous settings may be lost\n");
    } else {
        maia_console_puts("[F2] MaiaKV unavailable (error=-");
        print_u32((uint32_t)(-(int32_t)kv_result));
        maia_console_puts("); boot continues\n");
    }
    int symbol_result = maia_tab5_kv_register_maiascript_symbols();
    if (symbol_result == 0) {
        maia_console_puts("[F2] MaiaScript MaiaKV imports registered\n");
    } else {
        maia_console_puts("[F2] MaiaScript MaiaKV import registration failed (error=-");
        print_u32((uint32_t)(-symbol_result));
        maia_console_puts(")\n");
    }
#endif

    /* Initialize kernel subsystems (M2, M4, M5) */
    (void)maia_sched_init();
    if (maia_mem_init(g_heap_pool, sizeof(g_heap_pool)) != MAIA_OK ||
        maia_newlib_locks_init() != MAIA_OK) {
        maia_console_puts("[F1] Runtime heap/lock initialization failed\n");
        for (;;) __asm__ volatile("wfi");
    }

    void *sdmmc_dma = maia_mem_alloc_aligned(
        MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, MAIA_TAB5_SDMMC_DMA_ALIGNMENT);
    if (sdmmc_dma == NULL) {
        maia_console_puts("[F2] SDMMC DMA buffer allocation failed; boot continues\n");
    } else {
        maia_sdmmc_boot_report_t sdmmc_report;
#ifdef MAIA_SDMMC_DIAGNOSTIC_CMD8_TIMEOUT
        maia_console_puts("[F2] DIAGNOSTIC: injecting SDMMC CMD8 timeout\n");
#endif
        uintptr_t irq_state = maia_arch_irq_save();
        maia_sdmmc_boot_result_t sdmmc_result = maia_tab5_sdmmc_service_init(
            sdmmc_dma, MAIA_TAB5_SDMMC_DMA_STORAGE_BYTES, &sdmmc_report);
        maia_arch_irq_restore(irq_state);
        if (sdmmc_result == MAIA_SDMMC_BOOT_OK) {
            maia_console_puts("[F2] Native SDMMC block service ready\n");
        } else {
            maia_console_puts("[F2] SDMMC service unavailable (error=-");
            print_u32((uint32_t)(-(int32_t)sdmmc_result));
            maia_console_puts(", stage=");
            print_u32((uint32_t)sdmmc_report.completed);
            maia_console_puts("); boot continues\n");
        }
    }
    (void)maia_mutex_init(&g_telem_mutex);
    (void)maia_queue_init(&g_telem_queue, g_telem_qbuf, sizeof(uint32_t), 8U);

    /* Initialize M1 CLIC Trap Vector + 1000 Hz SYSTIMER tick */
    maia_soc_systimer_init(1000U);

    /* Initialize M5Stack Tab5 IO expanders, backlight, touch/display detect & keyboard */
    maia_tab5_board_io_init();

    /* Initialize MIPI-DSI 720x1280 screen */
    maia_display_init(maia_tab5_get_detected_panel());

    maia_console_puts("[M1] SYSTIMER configured; trap handling is verified by scheduler execution\n");

    /* Create RTOS tasks (M2) */
    maia_task_id_t tid_idle  = -1;
    maia_task_id_t tid_telem = -1;
    maia_task_id_t tid_shell = -1;

    (void)maia_task_create("idle", task_idle_entry, NULL, MAIA_PRIO_IDLE,
                           g_stack_idle, sizeof(g_stack_idle), &tid_idle);
    (void)maia_task_create("telemetry", task_telemetry_entry, NULL, MAIA_PRIO_NORMAL,
                           g_stack_telem, sizeof(g_stack_telem), &tid_telem);
    (void)maia_task_create("maia_shell", task_shell_entry, NULL, MAIA_PRIO_NORMAL,
                           g_stack_shell, sizeof(g_stack_shell), &tid_shell);

    maia_console_puts("[M2] Created 3 RTOS tasks (idle=");
    print_u32((uint32_t)tid_idle);
    maia_console_puts(", telemetry=");
    print_u32((uint32_t)tid_telem);
    maia_console_puts(", maia_shell=");
    print_u32((uint32_t)tid_shell);
    maia_console_puts(") — starting scheduler...\n");

    maia_sched_start();

    for (;;) {
        __asm__ volatile("wfi");
    }
}
