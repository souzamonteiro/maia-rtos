#include <maia/console.h>
#include <maia/services/lcd_output.h>
#include <maia/services/native_display.h>
#include <maia/soc/esp32p4_display.h>

#include <limits.h>

#define NATIVE_DISPLAY_WIDTH 1280
#define NATIVE_DISPLAY_HEIGHT 720
#define NATIVE_DISPLAY_PHYSICAL_WIDTH 720
#define NATIVE_DISPLAY_PHYSICAL_HEIGHT 1280
#define NATIVE_DISPLAY_MAX_SCOPES 16U

static uint16_t *leased_pixels;
static unsigned exclusive_depth;
static unsigned scope_depth;
static unsigned scope_baselines[NATIVE_DISPLAY_MAX_SCOPES];
static maia_status_t graphics_error;

static int32_t ensure_lease(void) {
    if (exclusive_depth == 0U) return -1;
    if (leased_pixels) return 0;
    uint16_t *pixels = 0;
    maia_status_t status = maia_lcd_output_edit_begin(&pixels);
    if (status != MAIA_OK || !pixels) {
        graphics_error = status == MAIA_OK ? MAIA_ERR_INVALID : status;
        return -1;
    }
    leased_pixels = pixels;
    graphics_error = MAIA_OK;
    return 0;
}

static int32_t put_pixel(int32_t x, int32_t y, uint16_t color) {
    if (x < 0 || x >= NATIVE_DISPLAY_WIDTH || y < 0 || y >= NATIVE_DISPLAY_HEIGHT) return -1;
    if (ensure_lease() != 0) return -1;
    int32_t px = y;
    int32_t py = NATIVE_DISPLAY_PHYSICAL_HEIGHT - 1 - x;
    leased_pixels[py * NATIVE_DISPLAY_PHYSICAL_WIDTH + px] = color;
    return 0;
}

static int32_t fill_rect(int32_t x, int32_t y, int32_t width, int32_t height, uint16_t color) {
    if (width < 0 || height < 0) return -1;
    if (width == 0 || height == 0) return 0;
    int64_t left = x;
    int64_t top = y;
    int64_t right = left + (int64_t)width;
    int64_t bottom = top + (int64_t)height;
    if (right <= 0 || bottom <= 0 || left >= NATIVE_DISPLAY_WIDTH || top >= NATIVE_DISPLAY_HEIGHT) return 0;
    if (left < 0) left = 0;
    if (top < 0) top = 0;
    if (right > NATIVE_DISPLAY_WIDTH) right = NATIVE_DISPLAY_WIDTH;
    if (bottom > NATIVE_DISPLAY_HEIGHT) bottom = NATIVE_DISPLAY_HEIGHT;
    if (ensure_lease() != 0) return -1;
    for (int64_t logical_x = left; logical_x < right; ++logical_x) {
        int32_t physical_y = NATIVE_DISPLAY_PHYSICAL_HEIGHT - 1 - (int32_t)logical_x;
        uint16_t *row = leased_pixels + physical_y * NATIVE_DISPLAY_PHYSICAL_WIDTH + (int32_t)top;
        for (int64_t logical_y = top; logical_y < bottom; ++logical_y) {
            *row++ = color;
        }
    }
    return 0;
}

static int32_t publish_lease(void) {
    if (!leased_pixels) return graphics_error == MAIA_OK ? 0 : -1;
    maia_status_t status = maia_lcd_output_edit_publish();
    leased_pixels = 0;
    if (status != MAIA_OK) {
        graphics_error = status;
        return -1;
    }
    graphics_error = MAIA_OK;
    return 0;
}

static void sync_display(void) {
    if (publish_lease() != 0) {
        maia_console_puts("[DISPLAY] Native graphics transaction failed\n");
    }
}

static int32_t acquire_exclusive(void) {
    if (exclusive_depth == UINT_MAX) return -1;
    if (exclusive_depth == 0U) maia_display_set_exclusive(1);
    ++exclusive_depth;
    return 0;
}

static int32_t release_exclusive(void) {
    unsigned floor = scope_depth != 0U ? scope_baselines[scope_depth - 1U] : 0U;
    if (exclusive_depth <= floor) return -1;
    if (exclusive_depth == floor + 1U && publish_lease() != 0) return -1;
    if (--exclusive_depth == 0U) maia_display_set_exclusive(0);
    return 0;
}

static uint16_t *get_framebuffer(void) {
    return 0;
}

static const maia_display_api_t display_api = {
    .width = NATIVE_DISPLAY_WIDTH,
    .height = NATIVE_DISPLAY_HEIGHT,
    .put_pixel = put_pixel,
    .fill_rect = fill_rect,
    .sync = sync_display,
    .acquire_exclusive = acquire_exclusive,
    .release_exclusive = release_exclusive,
    .get_framebuffer = get_framebuffer
};

const maia_display_api_t *maia_native_display_api(void) {
    return &display_api;
}

int maia_native_display_scope_begin(void) {
    if (scope_depth >= NATIVE_DISPLAY_MAX_SCOPES) {
        maia_console_puts("[DISPLAY] Native graphics nesting limit reached\n");
        return -1;
    }
    scope_baselines[scope_depth++] = exclusive_depth;
    return 0;
}

int maia_native_display_scope_end(void) {
    if (scope_depth == 0U) return -1;
    unsigned baseline = scope_baselines[scope_depth - 1U];
    if (exclusive_depth < baseline || publish_lease() != 0) {
        maia_console_puts("[DISPLAY] Native graphics scope cleanup failed\n");
        return -1;
    }
    unsigned was_exclusive = exclusive_depth != 0U;
    exclusive_depth = baseline;
    if (was_exclusive && exclusive_depth == 0U) maia_display_set_exclusive(0);
    --scope_depth;
    return 0;
}

#ifdef MAIA_NATIVE_DISPLAY_TEST
void maia_native_display_test_reset(void) {
    leased_pixels = 0;
    exclusive_depth = 0;
    scope_depth = 0;
    graphics_error = MAIA_OK;
}
#endif
