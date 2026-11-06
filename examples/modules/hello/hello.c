#include <maia/exec/mnm.h>

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host || !ctx->host->write_fd) {
        return 1;
    }

    const char msg[] = "Hello from Native ELF module running on ESP32-P4 RISC-V!\n";
    ctx->host->write_fd(ctx->stdout_fd, msg, sizeof(msg) - 1);

    if (ctx->argc > 1) {
        const char hdr[] = "Module received arguments:\n";
        ctx->host->write_fd(ctx->stdout_fd, hdr, sizeof(hdr) - 1);

        for (uint32_t i = 1; i < ctx->argc; ++i) {
            if (ctx->argv && ctx->argv[i]) {
                const char pfx[] = "  [";
                ctx->host->write_fd(ctx->stdout_fd, pfx, sizeof(pfx) - 1);

                char num[4] = {'0' + (char)i, ']', ':', ' '};
                ctx->host->write_fd(ctx->stdout_fd, num, 4);

                const char *val = ctx->argv[i];
                uint32_t len = 0;
                while (val[len]) len++;
                ctx->host->write_fd(ctx->stdout_fd, val, len);
                ctx->host->write_fd(ctx->stdout_fd, "\n", 1);
            }
        }
    }

    return 42;
}
