#include <maia/exec/elf_loader.h>
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

static void out_str(const maia_module_context_t *ctx, const char *s) {
    if (!ctx || !ctx->host || !ctx->host->write_fd || !s) return;
    uint32_t len = 0;
    const char *p = s;
    while (*p++) len++;
    ctx->host->write_fd(ctx->stdout_fd, s, len);
}

// Convert unsigned int to dec string
static void out_uint(const maia_module_context_t *ctx, uint32_t val) {
    char buf[16];
    int i = 0;
    if (val == 0) {
        out_str(ctx, "0");
        return;
    }
    while (val > 0 && i < 15) {
        buf[i++] = '0' + (val % 10);
        val /= 10;
    }
    char rev[16];
    for (int j = 0; j < i; ++j) {
        rev[j] = buf[i - 1 - j];
    }
    rev[i] = '\0';
    out_str(ctx, rev);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;

    out_str(ctx, "=== MaiaRTOS Native Display & Timer Module (banner.mobj) ===\n");

    uint64_t t0 = 0;
    if (ctx->host->get_time_us) {
        t0 = ctx->host->get_time_us();
        out_str(ctx, "Timer Start  : ");
        out_uint(ctx, (uint32_t)(t0 & 0xFFFFFFFF));
        out_str(ctx, " us\n");
    }

    if (ctx->host->display && ctx->host->display->fill_rect) {
        out_str(ctx, "Display Host : Active (Logical 1280x720 MIPI-DSI)\n");
        out_str(ctx, "Action       : Drawing animated banner at top (y=8, h=24)...\n");

        // Colors in RGB565:
        // Blue: 0x001F, Cyan: 0x07FF, Green: 0x07E0, Gold/Yellow: 0xFFE0, White: 0xFFFF
        uint16_t colors[] = { 0x001F, 0x07FF, 0x07E0, 0xFFE0, 0xF800, 0xFFFF };
        int num_colors = sizeof(colors) / sizeof(colors[0]);

        // Draw top banner border
        ctx->host->display->fill_rect(20, 8, 1240, 4, 0x07FF);

        // Animate a progressive status indicator
        for (int step = 0; step < num_colors; ++step) {
            int x = 20 + step * 200;
            ctx->host->display->fill_rect(x, 14, 180, 16, colors[step]);
            if (ctx->host->display->sync) {
                ctx->host->display->sync();
            }
            if (ctx->host->sleep_ms) {
                ctx->host->sleep_ms(30);
            }
        }
        out_str(ctx, "Display Sync : Banner rendered successfully to screen.\n");
    } else {
        out_str(ctx, "Display Host : Headless / Driver Not Attached\n");
    }

    if (ctx->host->get_time_us) {
        uint64_t t1 = ctx->host->get_time_us();
        out_str(ctx, "Timer End    : ");
        out_uint(ctx, (uint32_t)(t1 & 0xFFFFFFFF));
        out_str(ctx, " us\n");
        out_str(ctx, "Elapsed Time : ");
        out_uint(ctx, (uint32_t)(t1 - t0));
        out_str(ctx, " us\n");
    }

    out_str(ctx, "[SUCCESS] banner.mobj completed.\n");
    return 0;
}
