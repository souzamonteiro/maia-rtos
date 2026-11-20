#include <maia/kernel.h>
#include <maia/runtime/newlib_reent.h>

#include <errno.h>
#include <pthread.h>
#include <reent.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <mutex>
#include <new>

namespace {

constexpr uint32_t kRounds = 8U;
constexpr uint32_t kGlobalConstructorValue = 0xF1C00101U;
constexpr uint32_t kGuardConstructorValue = 0xF1C00202U;

volatile uint32_t g_global_constructor_calls;

struct GlobalConstructorProbe {
    uint32_t value;

    GlobalConstructorProbe() : value(kGlobalConstructorValue) {
        ++g_global_constructor_calls;
    }
};

GlobalConstructorProbe g_global_constructor_probe;
volatile uint32_t g_guard_constructor_calls;

struct GuardConstructorProbe {
    uint32_t value;

    GuardConstructorProbe() : value(0U) {
        ++g_guard_constructor_calls;
        maia_task_delay_ticks(5U);
        value = kGuardConstructorValue;
    }
};

GuardConstructorProbe &guard_constructor_probe() {
    static GuardConstructorProbe probe;
    return probe;
}

alignas(std::mutex) unsigned char g_mutex_storage[sizeof(std::mutex)];
std::mutex *g_mutex;
pthread_key_t g_key;
struct _reent *g_reent[2];
GuardConstructorProbe * volatile g_guard_instances[2];
volatile uint32_t g_worker_errors[2];
volatile uint32_t g_worker_done[2];
volatile uint32_t g_destructor_seen[2];
volatile uint32_t g_destructor_error;
volatile uint32_t g_mutex_count;
volatile uint32_t g_in_critical;

void record_error(uint32_t worker) {
    ++g_worker_errors[worker];
}

bool check_errno(uint32_t worker, int expected) {
    if (errno == expected && __getreent()->_errno == expected) return true;
    record_error(worker);
    return false;
}

void key_destructor(void *value) {
    uintptr_t token = reinterpret_cast<uintptr_t>(value);
    if (token == 1U) {
        g_destructor_seen[0] = 1U;
    } else if (token == 2U) {
        g_destructor_seen[1] = 1U;
    } else {
        g_destructor_error = 1U;
    }
}

void worker_entry(void *arg) {
    uint32_t worker = static_cast<uint32_t>(reinterpret_cast<uintptr_t>(arg));
    if (worker >= 2U) return;
    GuardConstructorProbe &guarded = guard_constructor_probe();
    g_guard_instances[worker] = &guarded;
    if (guarded.value != kGuardConstructorValue) record_error(worker);

    int expected_errno = 40 + static_cast<int>(worker);
    uintptr_t token = worker + 1U;
    g_reent[worker] = __getreent();
    if (g_reent[worker] == nullptr ||
        pthread_setspecific(g_key, reinterpret_cast<void *>(token)) != 0 ||
        pthread_getspecific(g_key) != reinterpret_cast<void *>(token)) {
        record_error(worker);
    }

    for (uint32_t round = 0U; round < kRounds; ++round) {
        errno = expected_errno;
        if (!check_errno(worker, expected_errno)) continue;

        struct timespec before = {};
        struct timespec after = {};
        struct timespec request = {0, 1000000L};
        struct timespec remaining = {1, 1};
        if (clock_gettime(CLOCK_MONOTONIC, &before) != 0 ||
            nanosleep(&request, &remaining) != 0 ||
            clock_gettime(CLOCK_MONOTONIC, &after) != 0 ||
            after.tv_sec < before.tv_sec ||
            (after.tv_sec == before.tv_sec && after.tv_nsec < before.tv_nsec) ||
            remaining.tv_sec != 0 || remaining.tv_nsec != 0) {
            record_error(worker);
        }
        check_errno(worker, expected_errno);

        {
            std::lock_guard<std::mutex> lock(*g_mutex);
            if (g_in_critical != 0U) record_error(worker);
            g_in_critical = 1U;
            uint32_t count = g_mutex_count;
            g_mutex_count = count + 1U;
            maia_task_delay_ticks(1U);
            if (g_in_critical != 1U || g_mutex_count != count + 1U) {
                record_error(worker);
            }
            g_in_critical = 0U;
        }
        check_errno(worker, expected_errno);

        uint32_t *memory = static_cast<uint32_t *>(calloc(4U, sizeof(uint32_t)));
        if (memory == nullptr) {
            record_error(worker);
            continue;
        }
        for (uint32_t i = 0U; i < 4U; ++i) {
            if (memory[i] != 0U) record_error(worker);
        }
        memory[0] = 0xF1000000U | worker;
        uint32_t *resized = static_cast<uint32_t *>(realloc(memory, 8U * sizeof(uint32_t)));
        if (resized == nullptr) {
            record_error(worker);
            free(memory);
            continue;
        }
        if (resized[0] != (0xF1000000U | worker)) record_error(worker);
        free(resized);

        if (pthread_getspecific(g_key) != reinterpret_cast<void *>(token)) {
            record_error(worker);
        }
        check_errno(worker, expected_errno);
    }

    g_worker_done[worker] = 1U;
}

void report_entry(void *) {
    uint32_t wait_ticks = 0U;
    while ((g_worker_done[0] == 0U || g_worker_done[1] == 0U ||
            g_destructor_seen[0] == 0U || g_destructor_seen[1] == 0U) &&
           wait_ticks < 500U) {
        maia_task_delay_ticks(1U);
        ++wait_ticks;
    }

    char input = 0;
    errno = 0;
    bool read_empty = read(STDIN_FILENO, &input, 1U) == -1 && errno == EAGAIN;
    bool pass = read_empty &&
        g_worker_done[0] != 0U && g_worker_done[1] != 0U &&
        g_guard_constructor_calls == 1U &&
        g_guard_instances[0] != nullptr &&
        g_guard_instances[0] == g_guard_instances[1] &&
        g_destructor_seen[0] != 0U && g_destructor_seen[1] != 0U &&
        g_destructor_error == 0U &&
        g_worker_errors[0] == 0U && g_worker_errors[1] == 0U &&
        g_reent[0] != nullptr && g_reent[1] != nullptr && g_reent[0] != g_reent[1] &&
        g_mutex_count == 2U * kRounds && g_in_critical == 0U;

    bool key_deleted = pthread_key_delete(g_key) == 0;
    if (!key_deleted) pass = false;
    g_mutex->~mutex();

    static const char pass_message[] = "MAIA_F1_RUNTIME_PASS\n";
    static const char fail_message[] = "MAIA_F1_RUNTIME_FAIL\n";
    const char *message = pass ? pass_message : fail_message;
    size_t length = pass ? sizeof(pass_message) - 1U : sizeof(fail_message) - 1U;
    (void)write(STDOUT_FILENO, message, length);
    for (;;) __asm__ volatile("wfi");
}

}  // namespace

extern "C" int maia_f1_runtime_smoke_start(
    void *stack_a, size_t stack_a_size,
    void *stack_b, size_t stack_b_size,
    void *stack_report, size_t stack_report_size) {
    if (g_global_constructor_calls != 1U ||
        g_global_constructor_probe.value != kGlobalConstructorValue) {
        return 3;
    }

    g_mutex = new (g_mutex_storage) std::mutex();
    if (pthread_key_create(&g_key, key_destructor) != 0) return 1;

    maia_task_id_t task_id = -1;
    if (maia_task_create("f1_worker_a", worker_entry,
                         reinterpret_cast<void *>(0U), MAIA_PRIO_NORMAL,
                         stack_a, stack_a_size, &task_id) != MAIA_OK ||
        maia_task_create("f1_worker_b", worker_entry,
                         reinterpret_cast<void *>(1U), MAIA_PRIO_NORMAL,
                         stack_b, stack_b_size, &task_id) != MAIA_OK ||
        maia_task_create("f1_report", report_entry, nullptr, MAIA_PRIO_NORMAL,
                         stack_report, stack_report_size, &task_id) != MAIA_OK) {
        return 2;
    }
    return 0;
}
