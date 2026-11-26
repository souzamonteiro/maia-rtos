#include <maia/exec/mnm.h>
#include <stdint.h>

static int32_t write_marker(const maia_module_context_t *ctx,
                            const char *message, uint32_t length) {
    return ctx->host->write_fd(ctx->stdout_fd, message, length) ==
                   (int32_t)length
               ? 0
               : -1;
}

int maia_module_main(const maia_module_context_t *ctx) {
    static const char initial_marker[] = "MNM_GRAPHICS_INITIAL_FRAME\n";
    static const char update_marker[] = "MNM_GRAPHICS_PARTIAL_UPDATE\n";
    static const char released_marker[] = "MNM_GRAPHICS_RELEASED\n";
    const maia_display_api_t *display;

    if (!ctx || !ctx->host || !ctx->host->write_fd || !ctx->host->sleep_ms)
        return 101;
    display = ctx->host->display;
    if (!display || display->width != 1280 || display->height != 720 ||
        !display->put_pixel || !display->fill_rect || !display->sync ||
        !display->acquire_exclusive || !display->release_exclusive)
        return 102;
    if (display->get_framebuffer && display->get_framebuffer() != 0)
        return 103;
    if (display->acquire_exclusive() < 0)
        return 104;

    if (display->fill_rect(0, 0, display->width, display->height, 0x0841U) < 0 ||
        display->fill_rect(24, 24, 112, 112, 0xF800U) < 0 ||
        display->fill_rect(display->width - 136, 24, 112, 112, 0x07E0U) < 0 ||
        display->fill_rect(24, display->height - 136, 112, 112, 0x001FU) < 0 ||
        display->fill_rect(display->width - 136, display->height - 136,
                           112, 112, 0xFFE0U) < 0 ||
        display->put_pixel(display->width / 2, display->height / 2, 0xFFFFU) < 0)
        return 105;
    display->sync();
    if (write_marker(ctx, initial_marker, sizeof(initial_marker) - 1U) < 0)
        return 106;
    if (ctx->host->sleep_ms(8000U) < 0)
        return 107;

    if (display->fill_rect(display->width / 2 - 48, display->height / 2 - 48,
                           96, 96, 0xF81FU) < 0)
        return 108;
    display->sync();
    if (write_marker(ctx, update_marker, sizeof(update_marker) - 1U) < 0)
        return 109;
    if (ctx->host->sleep_ms(8000U) < 0)
        return 110;

    if (display->release_exclusive() < 0)
        return 111;
    return write_marker(ctx, released_marker, sizeof(released_marker) - 1U) < 0
               ? 112
               : 0;
}
