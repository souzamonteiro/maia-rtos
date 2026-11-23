#include <maia/exec/mnm.h>
#include <stdint.h>

extern int32_t fill_rect(int32_t x, int32_t y, int32_t w, int32_t h,
                         uint16_t color);

int maia_module_main(const maia_module_context_t *ctx) {
    static const char message[] = "MNM_GRAPHICS_UNAVAILABLE\n";
    if (!ctx || !ctx->host || !ctx->host->write_fd) return 101;
    if (ctx->host->display) return 102;
    if (!ctx->host->input || !ctx->host->input->get_touch ||
        !ctx->host->input->read_key) return 103;
    if (fill_rect(1, 2, 3, 4, 0x1234U) >= 0) return 104;
    if (ctx->host->write_fd(ctx->stdout_fd, message, sizeof(message) - 1U) !=
        (int32_t)(sizeof(message) - 1U)) return 105;
    return 0;
}
