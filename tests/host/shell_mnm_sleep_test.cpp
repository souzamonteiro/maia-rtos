#include <cassert>
#include <cerrno>
#include <cstdint>
#include <ctime>

#include <maia_shell/maia_shell.hpp>

static unsigned sleep_calls;
static timespec last_request;
static int sleep_result;
static bool module_called;

extern "C" int nanosleep(const timespec *request, timespec *remaining) {
    assert(request != nullptr);
    assert(remaining == nullptr);
    ++sleep_calls;
    last_request = *request;
    if (sleep_result != 0) errno = EINTR;
    return sleep_result;
}

extern "C" int mnm_register_symbol(const char *, uintptr_t) {
    return 0;
}

extern "C" int mnm_get_registered_symbols(const char **, uintptr_t *, int) {
    return 0;
}

extern "C" int mnm_load_and_run(const void *elf, size_t elf_size,
                                 int, const char *const *,
                                 const maia_host_api_v1 *host, int *exit_code,
                                 void (*)(const char *)) {
    assert(elf != nullptr && elf_size != 0U);
    assert(host != nullptr && host->sleep_ms != nullptr);
    assert(exit_code != nullptr);
    module_called = true;

    assert(host->sleep_ms(0U) == 0);
    assert(sleep_calls == 1U && last_request.tv_sec == 0 && last_request.tv_nsec == 0);
    assert(host->sleep_ms(1U) == 0);
    assert(sleep_calls == 2U && last_request.tv_sec == 0 && last_request.tv_nsec == 1000000L);
    assert(host->sleep_ms(1500U) == 0);
    assert(sleep_calls == 3U && last_request.tv_sec == 1 && last_request.tv_nsec == 500000000L);
    assert(host->sleep_ms(UINT32_MAX) == 0);
    assert(sleep_calls == 4U && last_request.tv_sec == 4294967 &&
           last_request.tv_nsec == 295000000L);

    sleep_result = -1;
    errno = 0;
    assert(host->sleep_ms(20U) == -1);
    assert(sleep_calls == 5U && errno == EINTR);
    *exit_code = 0;
    return MNM_LOAD_OK;
}

static void discard_output(const char *) {}

int main() {
    MaiaShell shell;
    shell.init(discard_output);
    assert(shell.execute("runelf") == 0);
    assert(module_called);
    return 0;
}
