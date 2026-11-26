#ifdef NDEBUG
#undef NDEBUG
#endif

#include <cassert>
#include <cstdint>
#include <cstring>

#include <maia/kernel.h>
#include <maia_shell/maia_shell.hpp>
#include <maia_shell/shell_engine.h>

static unsigned module_calls;
static unsigned exclusive_calls;
static char console_output[4096];
static size_t console_length;

extern "C" void maia_console_putc(char ch) {
    assert(console_length + 1U < sizeof(console_output));
    console_output[console_length++] = ch;
    console_output[console_length] = '\0';
}

extern "C" void maia_console_puts(const char *text) {
    while (text && *text) maia_console_putc(*text++);
}

extern "C" int maia_tab5_keyboard_getc_nonblocking(void) {
    return 'K';
}

extern "C" void maia_display_set_exclusive(int) {
    ++exclusive_calls;
}

extern "C" int maia_tab5_touch_get(int32_t *x, int32_t *y, int32_t *pressed) {
    *x = 10;
    *y = 20;
    *pressed = 1;
    return 0;
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
    assert(host != nullptr && host->display == nullptr);
    assert(host->input != nullptr && host->input->read_key != nullptr);
    assert(host->input->get_touch != nullptr && host->input->read_key() == 'K');
    int32_t x = 0, y = 0, pressed = 0;
    assert(host->input->get_touch(&x, &y, &pressed) == 0);
    assert(x == 10 && y == 20 && pressed == 1);
    ++module_calls;
    *exit_code = 0;
    return MNM_LOAD_OK;
}

extern "C" int mnm_load_and_run_reader(const elf_reader_t *, int,
                                        const char *const *,
                                        const maia_host_api_v1 *, int *,
                                        void (*)(const char *)) {
    assert(false);
    return MNM_ERR_IO;
}

int main() {
    const maia_display_api_t stale_driver = {};
    MaiaShell::set_display_driver(&stale_driver);
    maia_shell_engine_init();
    assert(MaiaShell::get_global_display_api() == nullptr);
    assert(strstr(console_output, "Native module graphics unavailable") != nullptr);
    assert(maia_shell_engine_eval_line("runelf") == 0);
    assert(module_calls == 1U && exclusive_calls == 0U);
    return 0;
}
