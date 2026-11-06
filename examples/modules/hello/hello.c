#include <maia/exec/mnm.h>

int maia_module_main(const maia_module_context_t *ctx) {
    if (ctx && ctx->host && ctx->host->write_fd) {
        const char msg[] = "Hello from Native ELF module running on ESP32-P4 RISC-V!\n";
        ctx->host->write_fd(ctx->stdout_fd, msg, sizeof(msg) - 1);
    }
    return 42;
}
