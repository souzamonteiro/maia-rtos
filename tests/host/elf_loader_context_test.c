#ifdef NDEBUG
#undef NDEBUG
#endif

#include <assert.h>
#if defined(MAIA_ENABLE_COMPILER_TLS) && MAIA_ENABLE_COMPILER_TLS
#include <pthread.h>
#endif
#include <stdint.h>

#include "../../components/exec/elf_loader.c"

static unsigned outer_delays;
static unsigned inner_delays;

static uint64_t outer_time(void) { return 111000U; }
static uint64_t inner_time(void) { return 222000U; }

static int32_t outer_sleep(uint32_t ms) {
    assert(ms == 11U);
    ++outer_delays;
    return 0;
}

static int32_t inner_sleep(uint32_t ms) {
    assert(ms == 7U);
    ++inner_delays;
    return 0;
}

static const maia_host_api_v1 outer_host = {
    .sleep_ms = outer_sleep,
    .get_time_us = outer_time
};
static const maia_host_api_v1 inner_host = {
    .sleep_ms = inner_sleep,
    .get_time_us = inner_time
};
static const maia_host_api_v1 rejected_host = {0};

static void check_invalid_load_preserves_host(const maia_host_api_v1 *expected) {
    const uint8_t invalid_elf[] = {0};
    assert(mnm_load_and_run(invalid_elf, sizeof(invalid_elf), 0, NULL,
                            &rejected_host, NULL, NULL) == MNM_ERR_VALIDATION_FAILED);
    assert(s_active_host == expected);
}

static int inner_entry(const maia_module_context_t *ctx) {
    assert(s_active_host == ctx->host && ctx->host == &inner_host);
    assert(maia_host_millis() == 222U);
    maia_host_delay_ms(7U);
    check_invalid_load_preserves_host(&inner_host);
    return 17;
}

static int outer_entry(const maia_module_context_t *ctx) {
    assert(s_active_host == ctx->host && ctx->host == &outer_host);
    assert(maia_host_millis() == 111U);
    check_invalid_load_preserves_host(&outer_host);
    const maia_module_context_t inner = {.host = &inner_host};
    assert(mnm_invoke_entry(inner_entry, &inner) == 17);
    assert(s_active_host == &outer_host);
    assert(maia_host_millis() == 111U);
    maia_host_delay_ms(11U);
    return 42;
}

#if defined(MAIA_ENABLE_COMPILER_TLS) && MAIA_ENABLE_COMPILER_TLS
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
static unsigned entered;
static unsigned checked;

static void rendezvous(unsigned *count) {
    assert(pthread_mutex_lock(&lock) == 0);
    ++*count;
    assert(pthread_cond_broadcast(&cond) == 0);
    while (*count < 2U) assert(pthread_cond_wait(&cond, &lock) == 0);
    assert(pthread_mutex_unlock(&lock) == 0);
}

static int concurrent_entry(const maia_module_context_t *ctx) {
    rendezvous(&entered);
    assert(s_active_host == ctx->host);
    uint32_t expected = ctx->host == &outer_host ? 111U : 222U;
    assert(maia_host_millis() == expected);
    rendezvous(&checked);
    return (int)expected;
}

struct thread_arg {
    const maia_host_api_v1 *host;
};

static void *run_thread(void *arg) {
    const struct thread_arg *thread = arg;
    const maia_host_api_v1 *host = thread->host;
    const maia_module_context_t ctx = {.host = host};
    assert(s_active_host == NULL);
    assert(mnm_invoke_entry(concurrent_entry, &ctx) ==
           (host == &outer_host ? 111 : 222));
    assert(s_active_host == NULL);
    return NULL;
}
#endif

int main(void) {
    check_invalid_load_preserves_host(NULL);
    const maia_module_context_t outer = {.host = &outer_host};
    assert(mnm_invoke_entry(outer_entry, &outer) == 42);
    assert(s_active_host == NULL);
    assert(outer_delays == 1U && inner_delays == 1U);

#if defined(MAIA_ENABLE_COMPILER_TLS) && MAIA_ENABLE_COMPILER_TLS
    struct thread_arg first_arg = {&outer_host};
    struct thread_arg second_arg = {&inner_host};
    pthread_t first;
    pthread_t second;
    assert(pthread_create(&first, NULL, run_thread, &first_arg) == 0);
    assert(pthread_create(&second, NULL, run_thread, &second_arg) == 0);
    assert(pthread_join(first, NULL) == 0);
    assert(pthread_join(second, NULL) == 0);
#endif
    return 0;
}
