#include <maia/exec/mnm.h>
#include <stdint.h>

int maia_module_main(const maia_module_context_t *ctx) {
    static const char text[] =
        "MNM_WRITE_STDOUT "
        "012345678901234567890123456789012345678901234567890123456789"
        "012345678901234567890123456789012345678901234567890123456789\n";
    static const char error[] = "MNM_WRITE_STDERR\n";
    static const char binary[] = {'A', '\0', 'B'};

    if (!ctx || !ctx->host || !ctx->host->write_fd) return 101;
    if (ctx->host->write_fd(ctx->stdout_fd, text, sizeof(text) - 1U) !=
        (int32_t)(sizeof(text) - 1U)) return 102;
    if (ctx->host->write_fd(ctx->stderr_fd, error, sizeof(error) - 1U) !=
        (int32_t)(sizeof(error) - 1U)) return 103;
    if (ctx->host->write_fd(ctx->stdout_fd, binary, sizeof(binary)) >= 0) return 104;
    if (ctx->host->write_fd(ctx->stderr_fd, binary, sizeof(binary)) >= 0) return 105;
    if (ctx->host->write_fd(ctx->stdin_fd, text, 1U) >= 0) return 106;
    if (ctx->host->write_fd(ctx->stdout_fd, NULL, 0U) != 0) return 107;
    if (ctx->host->write_fd(ctx->stdout_fd, NULL, 1U) >= 0) return 108;
    if (ctx->host->write_fd(ctx->stdout_fd, text, UINT32_MAX) >= 0) return 109;
    return 0;
}
