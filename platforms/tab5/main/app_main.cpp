/*
 * M5Stack Tab5 Display + Ext.Port1 Physical Keyboard (Rotation 3: 1280x720 Landscape)
 * Integrated with MaiaShell Native Execution Engine (MaiaRTOS)
 */
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <unistd.h>
#include <math.h>
#include <time.h>
#include <sys/time.h>

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_netif_sntp.h"
#include "esp_http_server.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_err.h"
#include "esp_cache.h"
#include "driver/i2c_master.h"
#include "driver/gpio.h"
#include "esp_lcd_mipi_dsi.h"
#include "esp_lcd_panel_ops.h"
#include "bsp/esp-bsp.h"
#include "esp_lcd_touch_st7123.h"
#include "font_8x16.h"
#include "usb/usb_host.h"
#include "usb/msc_host.h"
#include "usb/msc_host_vfs.h"
#include <dirent.h>
#include "maia_shell/maia_shell.hpp"
#include "maia/exec/desktop_mobj.h"
#include "maia/exec/banner_mobj.h"
#include "maia/exec/calc_mobj.h"
#include "maia/exec/clock_mobj.h"
#include "maia/exec/sysinfo_mobj.h"
#include "maia/exec/camera_mobj.h"
#include "maia/exec/wifi_mobj.h"
#include "maia/exec/editor_mobj.h"
#include "maia/exec/files_mobj.h"
#include "maia/exec/term_mobj.h"
#include "t_gui_mobj.h"
#include "t_bench_mobj.h"
#include "t_class_mobj.h"
#include "t_mod_mobj.h"
#include "maia/exec/maiascript_lib_bundle.h"

static const char *TAG = "KBD_TAB5";

extern "C" esp_err_t bsp_display_new_with_handles_to_st7123(const bsp_display_config_t *config, bsp_lcd_handles_t *ret_handles);
extern "C" void bsp_set_ext_5v_en(bool en);
extern "C" void bsp_set_usb_5v_en(bool en);
extern "C" esp_err_t bsp_usb_host_start(bsp_usb_host_power_mode_t mode, bool limit_500mA);
extern "C" esp_err_t bsp_usb_host_stop(void);
extern "C" void esp_hosted_host_init(void);
extern "C" void bsp_set_ext_antenna_enable(bool en);

/* Physical hardware panel dimensions */
#define PHYS_W 720
#define PHYS_H 1280

/* Logical Landscape dimensions (Rotation 3: 270 deg) */
#define LOG_W 1280
#define LOG_H 720

#define COLOR_BLACK   0x0000
#define COLOR_WHITE   0xFFFF
#define COLOR_GREEN   0x07E0
#define COLOR_CYAN    0x07FF
#define COLOR_YELLOW  0xFFE0
#define COLOR_GRAY    0x7BEF
#define COLOR_RED     0xF800
#define COLOR_BLUE    0x001F
#define COLOR_MAGENTA 0xF81F

/* Tab5 Keyboard Official Registers */
#define M5_TAB5_KB_ADDR                0x6D
#define M5_TAB5_KB_REG_INT_STA         0x01
#define M5_TAB5_KB_REG_EVENT_NUM       0x02
#define M5_TAB5_KB_REG_KEYBOARD_MODE   0x10
#define M5_TAB5_KB_REG_CHAR_EVENT_LEN  0x40
#define M5_TAB5_KB_REG_CHAR_EVENT_BASE 0x50
#define M5_TAB5_KB_REG_VERSION         0xFE

#define M5_TAB5_KB_MODE_STRING 2

static bsp_lcd_handles_t s_lcd_handles;
static uint16_t *s_fb = NULL;
static const uint32_t FB_BYTES = (uint32_t)(PHYS_W * PHYS_H * sizeof(uint16_t));

#define CHAR_W 16
#define CHAR_H 32
#define MAX_COLS (LOG_W / CHAR_W) // 80 cols
#define MAX_ROWS (LOG_H / CHAR_H) // 22 rows

#define PROMPT_ROW_START 4
static int s_cur_col = 0;
static int s_cur_row = PROMPT_ROW_START;

static MaiaShell s_shell;
static char s_term_lines[MAX_ROWS][MAX_COLS + 1];
static char s_cmd_line[256];
static size_t s_cmd_len = 0;
static size_t s_cmd_pos = 0;
static char s_pending_cmd[256] = {0};
static volatile bool s_has_pending_cmd = false;
static bool s_graphics_exclusive = false;
static void gui_kb_push(uint8_t c);

static inline void fb_sync(void)
{
    if (s_fb) {
        esp_cache_msync((void *)s_fb, FB_BYTES, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
    }
}

static uint16_t *s_wallpaper_cache = NULL;
static bool s_has_custom_wallpaper = false;

extern "C" int32_t host_apply_wallpaper(const char *path)
{
    if ((!path || !*path) && s_has_custom_wallpaper && s_wallpaper_cache && s_fb) {
        memcpy(s_fb, s_wallpaper_cache, FB_BYTES);
        fb_sync();
        return 0;
    }

    const char *try_path = path;
    if (!try_path || !*try_path) {
        struct stat st;
        if (stat("/sd/wallpaper.raw", &st) == 0) {
            try_path = "/sd/wallpaper.raw";
        } else if (stat("/usb/wallpaper.raw", &st) == 0) {
            try_path = "/usb/wallpaper.raw";
        } else {
            return -1;
        }
    }

    FILE *f = fopen(try_path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open wallpaper file: '%s'", try_path);
        return -2;
    }

    if (!s_wallpaper_cache) {
        s_wallpaper_cache = (uint16_t *)heap_caps_malloc(FB_BYTES, MALLOC_CAP_SPIRAM);
    }
    if (!s_wallpaper_cache) {
        fclose(f);
        ESP_LOGE(TAG, "No PSRAM memory for wallpaper cache");
        return -3;
    }

    size_t rd = fread(s_wallpaper_cache, 1, FB_BYTES, f);
    fclose(f);

    if (rd < 100000) {
        ESP_LOGE(TAG, "Wallpaper file too small (%zu bytes)", rd);
        return -4;
    }

    s_has_custom_wallpaper = true;
    if (s_fb) {
        memcpy(s_fb, s_wallpaper_cache, FB_BYTES);
        fb_sync();
    }
    ESP_LOGI(TAG, "Real wallpaper from '%s' applied (%zu bytes)!", try_path, rd);
    return 0;
}

/* Rotation 3 pixel transformation:
 * Logical (x, y) with 0 <= x < 1280, 0 <= y < 720
 * Physical panel (px, py) with 0 <= px < 720, 0 <= py < 1280
 *
 * Rotation 3:
 * px = y
 * py = 1279 - x
 */
static inline void put_pixel_rot3(int x, int y, uint16_t color)
{
    if (x < 0 || x >= LOG_W || y < 0 || y >= LOG_H) return;
    int px = y;
    int py = 1279 - x;
    s_fb[py * PHYS_W + px] = color;
}

static void fill_rect(int x, int y, int w, int h, uint16_t color)
{
    int max_x = x + w;
    int max_y = y + h;
    if (x < 0) x = 0;
    if (y < 0) y = 0;
    if (max_x > LOG_W) max_x = LOG_W;
    if (max_y > LOG_H) max_y = LOG_H;

    for (int cy = y; cy < max_y; ++cy) {
        int px = cy;
        for (int cx = x; cx < max_x; ++cx) {
            int py = 1279 - cx;
            s_fb[py * PHYS_W + px] = color;
        }
    }
}

static void draw_char_2x(int col, int row, char c, uint16_t fg, uint16_t bg)
{
    if (col < 0 || col >= MAX_COLS || row < 0 || row >= MAX_ROWS) return;
    int px_x = col * CHAR_W;
    int px_y = row * CHAR_H;
    if (c < 32 || c > 126) c = ' ';
    const uint8_t *glyph = &g_font_8x16[((int)c - 32) * 16];

    for (int y = 0; y < 16; ++y) {
        uint8_t line = glyph[y];
        for (int x = 0; x < 8; ++x) {
            bool on = (line & (0x80 >> x)) != 0;
            uint16_t color = on ? fg : bg;
            int dst_x = px_x + (x * 2);
            int dst_y = px_y + (y * 2);
            put_pixel_rot3(dst_x,     dst_y,     color);
            put_pixel_rot3(dst_x + 1, dst_y,     color);
            put_pixel_rot3(dst_x,     dst_y + 1, color);
            put_pixel_rot3(dst_x + 1, dst_y + 1, color);
        }
    }
}

static void draw_str_2x(int col, int row, const char *str, uint16_t fg, uint16_t bg)
{
    fill_rect(col * CHAR_W, row * CHAR_H, (int)strlen(str) * CHAR_W, CHAR_H, bg);
    while (*str && col < MAX_COLS) {
        draw_char_2x(col, row, *str++, fg, bg);
        col++;
    }
}

static uint16_t s_font_color = COLOR_WHITE;
static int s_esc_mode = 0;
static char s_color_buf[24];
static size_t s_color_pos = 0;

static void init_terminal_screen(void);

static void clear_terminal_lines(void)
{
    for (int r = PROMPT_ROW_START; r < MAX_ROWS; ++r) {
        for (int c = 0; c < MAX_COLS; ++c) {
            s_term_lines[r][c] = ' ';
        }
        s_term_lines[r][MAX_COLS] = '\0';
    }
    fill_rect(0, PROMPT_ROW_START * CHAR_H, LOG_W, (MAX_ROWS - PROMPT_ROW_START) * CHAR_H, COLOR_BLACK);
    s_cur_col = 0;
    s_cur_row = PROMPT_ROW_START;
    fb_sync();
}

static void term_scroll(void)
{
    // Fast memory shift for Rotation 3 (Landscape 1280x720)
    // Logical (x, y) maps to physical px = y, py = 1279 - x.
    // Moving logical y up by CHAR_H (32 pixels) shifts physical px.
    const int y_start = PROMPT_ROW_START * CHAR_H;
    const size_t shift_bytes = (LOG_H - y_start - CHAR_H) * sizeof(uint16_t);
    const size_t clear_bytes = CHAR_H * sizeof(uint16_t);

    for (int py = 0; py < 1280; ++py) {
        uint16_t *row_ptr = &s_fb[py * PHYS_W];
        memmove(&row_ptr[y_start], &row_ptr[y_start + CHAR_H], shift_bytes);
        memset(&row_ptr[LOG_H - CHAR_H], 0, clear_bytes);
    }

    for (int r = PROMPT_ROW_START; r < (MAX_ROWS - 1); ++r) {
        memcpy(s_term_lines[r], s_term_lines[r + 1], MAX_COLS);
    }
    memset(s_term_lines[MAX_ROWS - 1], ' ', MAX_COLS);
    s_cur_row = MAX_ROWS - 1;
    s_cur_col = 0;
}

static void term_putc(char c)
{
    if (s_graphics_exclusive) return;

    // ANSI Escape sequence decoder (\033[2J, \033[H, \033[color:...)
    if (s_esc_mode == 0) {
        if (c == '\033') {
            s_esc_mode = 1;
            return;
        }
    } else if (s_esc_mode == 1) {
        if (c == 'c') { // ESC c -> Full Terminal & Display Reset (RIS)
            init_terminal_screen();
            s_esc_mode = 0;
            return;
        } else if (c == '[') {
            s_esc_mode = 2;
            s_color_pos = 0;
            return;
        }
        s_esc_mode = 0;
    } else if (s_esc_mode == 2) {
        if (c == '2') {
            s_esc_mode = 3;
            return;
        } else if (c == 'H') {
            s_cur_col = 0;
            s_cur_row = PROMPT_ROW_START;
            s_esc_mode = 0;
            return;
        } else if (c == 'c') {
            s_esc_mode = 4;
            s_color_pos = 0;
            return;
        }
        s_esc_mode = 0;
    } else if (s_esc_mode == 3) {
        if (c == 'J') {
            clear_terminal_lines();
            s_esc_mode = 0;
            return;
        }
        s_esc_mode = 0;
    } else if (s_esc_mode == 4) {
        if (c == '\n' || c == '\r') {
            s_color_buf[s_color_pos] = '\0';
            if (strstr(s_color_buf, "green")) s_font_color = COLOR_GREEN;
            else if (strstr(s_color_buf, "yellow")) s_font_color = COLOR_YELLOW;
            else if (strstr(s_color_buf, "cyan")) s_font_color = COLOR_CYAN;
            else if (strstr(s_color_buf, "magenta")) s_font_color = COLOR_MAGENTA;
            else if (strstr(s_color_buf, "red")) s_font_color = COLOR_RED;
            else if (strstr(s_color_buf, "blue")) s_font_color = COLOR_BLUE;
            else s_font_color = COLOR_WHITE;
            s_esc_mode = 0;
            return;
        } else if (s_color_pos + 1 < sizeof(s_color_buf)) {
            s_color_buf[s_color_pos++] = c;
            return;
        } else {
            s_esc_mode = 0;
        }
    }

    if (c == '\r') {
        s_cur_col = 0;
        return;
    }
    if (c == '\n') {
        s_cur_col = 0;
        s_cur_row++;
        if (s_cur_row >= MAX_ROWS) {
            term_scroll();
        }
        return;
    }
    if (c == '\b' || c == 127) {
        if (s_cur_col > 0) {
            printf("\b \b");
            fflush(stdout);
            s_cur_col--;
            s_term_lines[s_cur_row][s_cur_col] = ' ';
            draw_char_2x(s_cur_col, s_cur_row, ' ', COLOR_WHITE, COLOR_BLACK);
            fb_sync();
        }
        return;
    }
    if (c == '\t') {
        int next_tab = (s_cur_col + 4) & ~3;
        while (s_cur_col < next_tab && s_cur_col < MAX_COLS) {
            term_putc(' ');
        }
        return;
    }
    if ((uint8_t)c < 32 || (uint8_t)c > 126) {
        return;
    }

    printf("%c", c);
    fflush(stdout);

    if (s_cur_col >= MAX_COLS) {
        s_cur_col = 0;
        s_cur_row++;
        if (s_cur_row >= MAX_ROWS) {
            term_scroll();
        }
    }

    s_term_lines[s_cur_row][s_cur_col] = c;
    draw_char_2x(s_cur_col, s_cur_row, c, s_font_color, COLOR_BLACK);
    s_cur_col++;
    fb_sync();
}

static void shell_output_cb(const char *str)
{
    if (!str) return;
    printf("%s", str);
    fflush(stdout);
    if (s_graphics_exclusive) return;
    while (*str) {
        term_putc(*str++);
    }
    fb_sync();
}

static void redraw_prompt_and_cmd(void)
{
    if (s_graphics_exclusive) return;

    /* Draw prompt 'msh> ' */
    const char *p = "msh> ";
    int col = 0;
    while (*p && col < MAX_COLS) {
        s_term_lines[s_cur_row][col] = *p;
        draw_char_2x(col, s_cur_row, *p, COLOR_GREEN, COLOR_BLACK);
        col++;
        p++;
    }

    /* Draw current command buffer */
    for (size_t i = 0; i < s_cmd_len && col < MAX_COLS; ++i) {
        s_term_lines[s_cur_row][col] = s_cmd_line[i];
        draw_char_2x(col, s_cur_row, s_cmd_line[i], COLOR_WHITE, COLOR_BLACK);
        col++;
    }

    /* Clear remainder of line */
    for (int c = col; c < MAX_COLS; ++c) {
        s_term_lines[s_cur_row][c] = ' ';
        draw_char_2x(c, s_cur_row, ' ', COLOR_WHITE, COLOR_BLACK);
    }

    /* Draw cursor at 5 + s_cmd_pos */
    int cur_col = 5 + (int)s_cmd_pos;
    if (cur_col < MAX_COLS) {
        if (s_cmd_pos < s_cmd_len) {
            draw_char_2x(cur_col, s_cur_row, s_cmd_line[s_cmd_pos], COLOR_BLACK, COLOR_CYAN);
        } else {
            draw_char_2x(cur_col, s_cur_row, ' ', COLOR_BLACK, COLOR_CYAN);
        }
    }
    s_cur_col = cur_col;
    fb_sync();
}

static void print_prompt(void)
{
    if (s_graphics_exclusive) return;
    s_cmd_len = 0;
    s_cmd_pos = 0;
    s_cmd_line[0] = '\0';
    redraw_prompt_and_cmd();
    printf("msh> ");
    fflush(stdout);
}

static void init_terminal_screen(void)
{
    fill_rect(0, 0, LOG_W, LOG_H, COLOR_BLACK);
    for (int r = 0; r < MAX_ROWS; ++r) {
        for (int c = 0; c < MAX_COLS; ++c) {
            s_term_lines[r][c] = ' ';
        }
        s_term_lines[r][MAX_COLS] = '\0';
    }

    draw_str_2x(1, 0, "=== MaiaRTOS Native Shell (M5Stack Tab5 Landscape 1280x720) ===", COLOR_CYAN, COLOR_BLACK);
    draw_str_2x(1, 1, "Display: ST7121 MIPI-DSI @ Rot 3 | Keyboard: Ext.Port1 (A164 @ 0x6D)", COLOR_YELLOW, COLOR_BLACK);
    draw_str_2x(1, 2, "Core: RV32IMAFCP Dual-Core @ 360 MHz | Storage: FAT32 (/sd)", COLOR_GREEN, COLOR_BLACK);
    draw_str_2x(0, 3, "--------------------------------------------------------------------------------", COLOR_GRAY, COLOR_BLACK);

    s_cur_col = 0;
    s_cur_row = PROMPT_ROW_START;
    fb_sync();
    print_prompt();
}

#define HIST_MAX 8
static char s_history[HIST_MAX][sizeof(s_cmd_line)];
static int s_hist_count = 0;
static int s_hist_browse = -1;

static void set_cmd_line(const char *cmd)
{
    if (!cmd) {
        s_cmd_line[0] = '\0';
        s_cmd_len = 0;
        s_cmd_pos = 0;
    } else {
        strncpy(s_cmd_line, cmd, sizeof(s_cmd_line) - 1);
        s_cmd_line[sizeof(s_cmd_line) - 1] = '\0';
        s_cmd_len = strlen(s_cmd_line);
        s_cmd_pos = s_cmd_len;
    }
    redraw_prompt_and_cmd();
}

static void clear_cmd_line_on_screen(void)
{
    set_cmd_line("");
}

static void println_term(const char *str)
{
    if (str) {
        while (*str) term_putc(*str++);
    }
    term_putc('\n');
}

static int64_t s_last_enter_us = 0;

static void handle_enter(void)
{
    if (s_graphics_exclusive) {
        gui_kb_push('\n');
        return;
    }

    int64_t now = esp_timer_get_time();
    if (s_cmd_len == 0 && (now - s_last_enter_us < 50000)) {
        return; // Ignore redundant \n right after \r
    }
    s_last_enter_us = now;

    /* Restore normal character under cursor before newline */
    int cur_col = 5 + (int)s_cmd_pos;
    if (cur_col < MAX_COLS) {
        char ch = (s_cmd_pos < s_cmd_len) ? s_cmd_line[s_cmd_pos] : ' ';
        draw_char_2x(cur_col, s_cur_row, ch, COLOR_WHITE, COLOR_BLACK);
    }
    term_putc('\n');

    s_cmd_line[s_cmd_len] = '\0';
    if (s_cmd_len > 0) {
        if (strcmp(s_cmd_line, "history") == 0) {
            println_term("Command History:");
            for (int i = 0; i < s_hist_count; ++i) {
                char hbuf[96];
                snprintf(hbuf, sizeof(hbuf), "  %2d: %s", i + 1, s_history[i]);
                println_term(hbuf);
            }
            s_cmd_len = 0;
            s_cmd_pos = 0;
            s_cmd_line[0] = '\0';
            print_prompt();
            return;
        }

        if (s_hist_count == 0 || strcmp(s_history[s_hist_count - 1], s_cmd_line) != 0) {
            if (s_hist_count < HIST_MAX) {
                strncpy(s_history[s_hist_count++], s_cmd_line, sizeof(s_cmd_line) - 1);
            } else {
                for (int i = 0; i < HIST_MAX - 1; ++i) {
                    strcpy(s_history[i], s_history[i + 1]);
                }
                strncpy(s_history[HIST_MAX - 1], s_cmd_line, sizeof(s_cmd_line) - 1);
            }
        }
        s_hist_browse = -1;

        /* Queue command to app_main worker */
        strncpy(s_pending_cmd, s_cmd_line, sizeof(s_pending_cmd) - 1);
        s_pending_cmd[sizeof(s_pending_cmd) - 1] = '\0';
        s_has_pending_cmd = true;

        s_cmd_len = 0;
        s_cmd_pos = 0;
        s_cmd_line[0] = '\0';
        return;
    }
    print_prompt();
}

static void handle_up(void)
{
    if (s_hist_count == 0) return;
    if (s_hist_browse == -1) {
        s_hist_browse = s_hist_count - 1;
    } else if (s_hist_browse > 0) {
        s_hist_browse--;
    }
    set_cmd_line(s_history[s_hist_browse]);
}

static void handle_down(void)
{
    if (s_hist_browse == -1) return;
    if (s_hist_browse + 1 < s_hist_count) {
        s_hist_browse++;
        set_cmd_line(s_history[s_hist_browse]);
    } else {
        s_hist_browse = -1;
        set_cmd_line("");
    }
}

static void handle_escape(void)
{
    if (s_graphics_exclusive) {
        gui_kb_push(27);
        return;
    }
    clear_cmd_line_on_screen();
    s_hist_browse = -1;
}

static esp_lcd_touch_handle_t s_tp_handle = NULL;
static uint8_t s_gui_kb_buf[64];
static size_t s_gui_kb_head = 0;
static size_t s_gui_kb_tail = 0;

static void gui_kb_push(uint8_t c)
{
    size_t next = (s_gui_kb_head + 1) % sizeof(s_gui_kb_buf);
    if (next != s_gui_kb_tail) {
        s_gui_kb_buf[s_gui_kb_head] = c;
        s_gui_kb_head = next;
    }
}

static int32_t host_read_key(void)
{
    if (s_gui_kb_head != s_gui_kb_tail) {
        uint8_t c = s_gui_kb_buf[s_gui_kb_tail];
        s_gui_kb_tail = (s_gui_kb_tail + 1) % sizeof(s_gui_kb_buf);
        return (int32_t)c;
    }
    return -1;
}

static void init_touch(void)
{
    esp_lcd_panel_io_handle_t tp_io_handle = NULL;
    esp_lcd_panel_io_i2c_config_t tp_io_config = {
        .dev_addr = 0x55,
        .control_phase_bytes = 1,
        .dc_bit_offset = 0,
        .lcd_cmd_bits = 16,
        .flags = {
            .disable_control_phase = 1,
        },
    };
    tp_io_config.scl_speed_hz = 400000;

    esp_err_t ret = esp_lcd_new_panel_io_i2c_v2(bsp_i2c_get_handle(), &tp_io_config, &tp_io_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create ST7123 touch I2C IO: %s", esp_err_to_name(ret));
        return;
    }

    const esp_lcd_touch_config_t tp_cfg = {
        .x_max = 720,
        .y_max = 1280,
        .rst_gpio_num = GPIO_NUM_NC,
        .int_gpio_num = GPIO_NUM_23,
        .levels = {
            .reset = 0,
            .interrupt = 0,
        },
        .flags = {
            .swap_xy = 0,
            .mirror_x = 0,
            .mirror_y = 0,
        },
    };

    ret = esp_lcd_touch_new_i2c_st7123(tp_io_handle, &tp_cfg, &s_tp_handle);
    if (ret != ESP_OK || !s_tp_handle) {
        ESP_LOGE(TAG, "Failed to create ST7123 touch driver: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI(TAG, "ST7123 Touch initialized successfully!");
    }
}

static int32_t host_get_touch(int32_t *out_x, int32_t *out_y, int32_t *out_pressed)
{
    if (!s_tp_handle) {
        if (out_pressed) *out_pressed = 0;
        return -1;
    }

    esp_err_t err = esp_lcd_touch_read_data(s_tp_handle);
    if (err != ESP_OK) {
        if (out_pressed) *out_pressed = 0;
        return 0;
    }

    uint16_t tx[1] = {0};
    uint16_t ty[1] = {0};
    uint16_t str[1] = {0};
    uint8_t cnt = 0;
    bool pressed = esp_lcd_touch_get_coordinates(s_tp_handle, tx, ty, str, &cnt, 1);

    if (pressed && cnt > 0) {
        int py = ty[0];
        int px = tx[0];
        int lx = 1279 - py;
        int ly = px;

        if (lx < 0) lx = 0;
        if (lx >= LOG_W) lx = LOG_W - 1;
        if (ly < 0) ly = 0;
        if (ly >= LOG_H) ly = LOG_H - 1;

        if (out_x) *out_x = lx;
        if (out_y) *out_y = ly;
        if (out_pressed) *out_pressed = 1;
        return 1;
    }

    if (out_pressed) *out_pressed = 0;
    return 0;
}

/* --- MaiaScript Native Host GUI & Touch API --- */
static void draw_text_px(int x, int y, const char *str, uint16_t fg, uint16_t bg)
{
    if (!str) return;
    int cur_x = x;
    while (*str) {
        char c = *str++;
        if (c < 32 || c > 126) c = ' ';
        const uint8_t *glyph = &g_font_8x16[((int)c - 32) * 16];
        for (int r = 0; r < 16; ++r) {
            uint8_t line = glyph[r];
            for (int b = 0; b < 8; ++b) {
                if (line & (0x80 >> b)) {
                    put_pixel_rot3(cur_x + b * 2,     y + r * 2,     fg);
                    put_pixel_rot3(cur_x + b * 2 + 1, y + r * 2,     fg);
                    put_pixel_rot3(cur_x + b * 2,     y + r * 2 + 1, fg);
                    put_pixel_rot3(cur_x + b * 2 + 1, y + r * 2 + 1, fg);
                } else if (bg != 0) {
                    put_pixel_rot3(cur_x + b * 2,     y + r * 2,     bg);
                    put_pixel_rot3(cur_x + b * 2 + 1, y + r * 2,     bg);
                    put_pixel_rot3(cur_x + b * 2,     y + r * 2 + 1, bg);
                    put_pixel_rot3(cur_x + b * 2 + 1, y + r * 2 + 1, bg);
                }
            }
        }
        cur_x += 16;
    }
}

static void draw_rect_api(int x, int y, int w, int h, uint16_t color)
{
    fill_rect(x, y, w, 1, color);
    fill_rect(x, y + h - 1, w, 1, color);
    fill_rect(x, y, 1, h, color);
    fill_rect(x + w - 1, y, 1, h, color);
}

static void draw_line_api(int x1, int y1, int x2, int y2, uint16_t color, int width)
{
    if (width <= 0) width = 1;
    int dx = abs(x2 - x1);
    int sx = x1 < x2 ? 1 : -1;
    int dy = -abs(y2 - y1);
    int sy = y1 < y2 ? 1 : -1;
    int err = dx + dy;

    while (true) {
        if (width == 1) {
            put_pixel_rot3(x1, y1, color);
        } else {
            int half = width / 2;
            for (int wx = -half; wx <= half; ++wx) {
                for (int wy = -half; wy <= half; ++wy) {
                    put_pixel_rot3(x1 + wx, y1 + wy, color);
                }
            }
        }
        if (x1 == x2 && y1 == y2) break;
        int e2 = 2 * err;
        if (e2 >= dy) { err += dy; x1 += sx; }
        if (e2 <= dx) { err += dx; y1 += sy; }
    }
}

static void draw_window_frame_api(int x, int y, int w, int h, const char *title, uint16_t accent)
{
    fill_rect(x + 6, y + 6, w, h, 0x0821);
    fill_rect(x, y, w, h, 0x18E5);
    fill_rect(x, y, w, 2, accent);
    fill_rect(x, y, 2, h, 0x3A0C);
    fill_rect(x + w - 2, y, 2, h, 0x3A0C);
    fill_rect(x, y + h - 2, w, 2, 0x3A0C);
    fill_rect(x + 2, y + 2, w - 4, 38, 0x2128);
    draw_text_px(x + 14, y + 8, title ? title : "Window", 0xFFFF, 0);
    fill_rect(x + w - 36, y + 6, 28, 28, 0xF800);
    draw_text_px(x + w - 28, y + 8, "X", 0xFFFF, 0);
}

static void draw_button_api(int x, int y, int w, int h, const char *label, uint16_t bg, uint16_t fg, int pressed)
{
    uint16_t fill = pressed ? 0x04BF : bg;
    fill_rect(x, y, w, h, fill);
    draw_rect_api(x, y, w, h, 0x4A4D);
    if (!label) return;
    int len = strlen(label);
    int tx = x + (w - len * 16) / 2;
    int ty = y + (h - 32) / 2;
    draw_text_px(tx, ty, label, fg, 0);
}

static int32_t s_last_touch_x = 0;
static int32_t s_last_touch_y = 0;
static int32_t s_last_touch_pressed = 0;

static int32_t api_touch_poll(void) {
    host_get_touch(&s_last_touch_x, &s_last_touch_y, &s_last_touch_pressed);
    return s_last_touch_pressed;
}
static int32_t api_touch_x(void) { return s_last_touch_x; }
static int32_t api_touch_y(void) { return s_last_touch_y; }


static int s_serial_esc = 0;

static void handle_char(char c)
{
    // ANSI Escape Sequence Decoder for Serial input (Up/Down arrows: ESC [ A / ESC [ B)
    if (s_serial_esc == 0) {
        if ((uint8_t)c == 27) {
            s_serial_esc = 1;
            return;
        }
    } else if (s_serial_esc == 1) {
        if (c == '[') {
            s_serial_esc = 2;
            return;
        } else {
            s_serial_esc = 0;
            // Standalone ESC pressed
            if (s_graphics_exclusive) {
                gui_kb_push(27);
            } else {
                handle_escape();
            }
        }
    } else if (s_serial_esc == 2) {
        s_serial_esc = 0;
        if (c == 'A') { // Up arrow
            if (s_graphics_exclusive) {
                gui_kb_push(0x80);
            } else {
                handle_up();
            }
            return;
        } else if (c == 'B') { // Down arrow
            if (s_graphics_exclusive) {
                gui_kb_push(0x81);
            } else {
                handle_down();
            }
            return;
        } else if (c == 'C') { // Right arrow
            if (!s_graphics_exclusive && s_cmd_pos < s_cmd_len) {
                s_cmd_pos++;
                redraw_prompt_and_cmd();
            }
            return;
        } else if (c == 'D') { // Left arrow
            if (!s_graphics_exclusive && s_cmd_pos > 0) {
                s_cmd_pos--;
                redraw_prompt_and_cmd();
            }
            return;
        }
        return;
    }

    static char s_prev_nl = 0;
    if (c == '\r' || c == '\n') {
        if ((c == '\n' && s_prev_nl == '\r') || (c == '\r' && s_prev_nl == '\n')) {
            s_prev_nl = 0;
            return;
        }
        s_prev_nl = c;
        c = '\n';
    } else {
        s_prev_nl = 0;
    }

    if (s_graphics_exclusive) {
        gui_kb_push((uint8_t)c);
        return;
    }
    if (c == 0x03) { // Ctrl+C
        term_putc('^');
        term_putc('C');
        term_putc('\n');
        s_cmd_len = 0;
        s_cmd_pos = 0;
        print_prompt();
        return;
    }
    if (c == '\r' || c == '\n') {
        handle_enter();
    } else if (c == '\b' || c == 127) {
        if (s_cmd_pos > 0) {
            for (size_t i = s_cmd_pos - 1; i < s_cmd_len; ++i) {
                s_cmd_line[i] = s_cmd_line[i + 1];
            }
            s_cmd_pos--;
            s_cmd_len--;
            s_cmd_line[s_cmd_len] = '\0';
            redraw_prompt_and_cmd();
        }
    } else if (c >= 32 && c <= 126) {
        if (s_cmd_len + 1 < sizeof(s_cmd_line)) {
            for (size_t i = s_cmd_len; i > s_cmd_pos; --i) {
                s_cmd_line[i] = s_cmd_line[i - 1];
            }
            s_cmd_line[s_cmd_pos] = c;
            s_cmd_pos++;
            s_cmd_len++;
            s_cmd_line[s_cmd_len] = '\0';
            redraw_prompt_and_cmd();
        }
    }
}

static esp_err_t kb_write_reg(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t val)
{
    uint8_t buf[2] = {reg, val};
    return i2c_master_transmit(dev, buf, 2, 50);
}

static esp_err_t kb_read_bytes(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev, &reg, 1, data, len, 50);
}

static void kb_poll_task(void *pvParameters)
{
    (void)pvParameters;

    /* 1. Init I2C on GPIO 0 (SDA) and GPIO 1 (SCL) for Ext.Port1 */
    i2c_master_bus_config_t ext_bus_cfg = {
        .i2c_port = I2C_NUM_1,
        .sda_io_num = (gpio_num_t)0,
        .scl_io_num = (gpio_num_t)1,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .intr_priority = 0,
        .trans_queue_depth = 0,
        .flags = {
            .enable_internal_pullup = true,
        },
    };
    i2c_master_bus_handle_t ext_bus = NULL;
    ESP_ERROR_CHECK(i2c_new_master_bus(&ext_bus_cfg, &ext_bus));

    i2c_device_config_t kbd_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = M5_TAB5_KB_ADDR,
        .scl_speed_hz = 100000,
        .scl_wait_us = 0,
        .flags = {
            .disable_ack_check = 0,
        },
    };
    i2c_master_dev_handle_t kbd_dev = NULL;
    ESP_ERROR_CHECK(i2c_master_bus_add_device(ext_bus, &kbd_cfg, &kbd_dev));

    /* 2. Probe and Configure Keyboard */
    uint8_t fw_ver = 0;
    esp_err_t ver_err = kb_read_bytes(kbd_dev, M5_TAB5_KB_REG_VERSION, &fw_ver, 1);
    ESP_LOGI(TAG, "Keyboard Version read: err=%d, ver=0x%02X", ver_err, fw_ver);

    // Switch keyboard into STRING MODE (Mode 2)
    esp_err_t mode_err = kb_write_reg(kbd_dev, M5_TAB5_KB_REG_KEYBOARD_MODE, M5_TAB5_KB_MODE_STRING);
    ESP_LOGI(TAG, "Set STRING MODE (0x10=0x02): err=%d", mode_err);

    // Reg 0x00: INT_CFG (Enable interrupts)
    kb_write_reg(kbd_dev, 0x00, 0x07);
    // Reg 0x02: Clear EVENT_NUM queue
    kb_write_reg(kbd_dev, 0x02, 0x00);
    // Reg 0x01: Clear interrupt status
    kb_write_reg(kbd_dev, M5_TAB5_KB_REG_INT_STA, 0x00);

    char status_msg[80];
    snprintf(status_msg, sizeof(status_msg), "Status: Keyboard (0x6D, FW 0x%02X) Ready! Type commands now.", fw_ver);
    draw_str_2x(1, 2, status_msg, COLOR_GREEN, COLOR_BLACK);
    fb_sync();

    // Make stdin non-blocking for serial console
    int stdin_flags = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, stdin_flags | O_NONBLOCK);

    ESP_LOGI(TAG, "Keyboard & Serial polling task active!");

    while (1) {
        // Poll USB serial input (mirror terminal)
        char sch = 0;
        while (read(STDIN_FILENO, &sch, 1) == 1) {
            handle_char(sch);
        }
        uint8_t qlen = 0;
        esp_err_t r = kb_read_bytes(kbd_dev, M5_TAB5_KB_REG_CHAR_EVENT_LEN, &qlen, 1);
        if (r == ESP_OK && qlen >= 1) {
            uint8_t read_cnt = (qlen > 16) ? 16 : qlen;
            uint8_t buf[20] = {0};
            r = kb_read_bytes(kbd_dev, M5_TAB5_KB_REG_CHAR_EVENT_BASE, buf, read_cnt + 1);
            if (r == ESP_OK) {
                char raw_str[20] = {0};
                size_t raw_len = 0;
                for (size_t k = 1; k <= read_cnt && k < sizeof(buf); ++k) {
                    char c = (char)buf[k];
                    if (c != '\0' && c != '\r' && c != '\n') {
                        raw_str[raw_len++] = c;
                    }
                }
                raw_str[raw_len] = '\0';

                char lower_str[20] = {0};
                for (size_t k = 0; k < raw_len; ++k) {
                    char c = raw_str[k];
                    if (c >= 'A' && c <= 'Z') c += 32;
                    lower_str[k] = c;
                }
                lower_str[raw_len] = '\0';

                if (s_graphics_exclusive) {
                    if (strcmp(lower_str, "enter") == 0 || strcmp(lower_str, "return") == 0) {
                        gui_kb_push('\n');
                    } else if (strcmp(lower_str, "backspace") == 0) {
                        gui_kb_push('\b');
                    } else if (strcmp(lower_str, "space") == 0) {
                        gui_kb_push(' ');
                    } else if (strcmp(lower_str, "tab") == 0) {
                        gui_kb_push('\t');
                    } else if (strcmp(lower_str, "esc") == 0 || strcmp(lower_str, "escape") == 0) {
                        gui_kb_push(27);
                    } else if (strcmp(lower_str, "up") == 0) {
                        gui_kb_push(0x80);
                    } else if (strcmp(lower_str, "down") == 0) {
                        gui_kb_push(0x81);
                    } else if (strcmp(lower_str, "left") == 0) {
                        gui_kb_push(0x82);
                    } else if (strcmp(lower_str, "right") == 0) {
                        gui_kb_push(0x83);
                    } else if (strcmp(lower_str, "del") == 0 || strcmp(lower_str, "delete") == 0) {
                        gui_kb_push(127);
                    } else if (strcmp(lower_str, "shift") == 0 || strcmp(lower_str, "ctrl") == 0 ||
                               strcmp(lower_str, "alt") == 0 || strcmp(lower_str, "fn") == 0 ||
                               strcmp(lower_str, "caps") == 0 || strcmp(lower_str, "capslock") == 0 ||
                               strcmp(lower_str, "gui") == 0 || strcmp(lower_str, "win") == 0 ||
                               strcmp(lower_str, "cmd") == 0 || strcmp(lower_str, "opt") == 0 ||
                               strcmp(lower_str, "home") == 0 || strcmp(lower_str, "end") == 0 ||
                               strcmp(lower_str, "pageup") == 0 || strcmp(lower_str, "pagedown") == 0 ||
                               strcmp(lower_str, "pgup") == 0 || strcmp(lower_str, "pgdn") == 0 ||
                               strcmp(lower_str, "insert") == 0 || strcmp(lower_str, "prtsc") == 0 ||
                               (raw_len >= 2 && lower_str[0] == 'f' && lower_str[1] >= '1' && lower_str[1] <= '9')) {
                        // Ignore modifiers
                    } else {
                        for (size_t k = 0; k < raw_len; ++k) {
                            gui_kb_push((uint8_t)raw_str[k]);
                        }
                    }
                    kb_write_reg(kbd_dev, M5_TAB5_KB_REG_INT_STA, 0x00);
                    if (qlen > 16) {
                        kb_write_reg(kbd_dev, 0x02, 0x00);
                    }
                    vTaskDelay(pdMS_TO_TICKS(5));
                    continue;
                }

                if (strcmp(lower_str, "enter") == 0 || strcmp(lower_str, "return") == 0) {
                    handle_enter();
                } else if (strcmp(lower_str, "backspace") == 0) {
                    handle_char('\b');
                } else if (strcmp(lower_str, "space") == 0) {
                    handle_char(' ');
                } else if (strcmp(lower_str, "tab") == 0) {
                    handle_char('\t');
                } else if (strcmp(lower_str, "esc") == 0 || strcmp(lower_str, "escape") == 0) {
                    handle_escape();
                } else if (strcmp(lower_str, "up") == 0) {
                    handle_up();
                } else if (strcmp(lower_str, "down") == 0) {
                    handle_down();
                } else if (strcmp(lower_str, "left") == 0) {
                    if (s_cmd_pos > 0) {
                        s_cmd_pos--;
                        redraw_prompt_and_cmd();
                    }
                } else if (strcmp(lower_str, "right") == 0) {
                    if (s_cmd_pos < s_cmd_len) {
                        s_cmd_pos++;
                        redraw_prompt_and_cmd();
                    }
                } else if (strcmp(lower_str, "del") == 0 || strcmp(lower_str, "delete") == 0) {
                    handle_char('\b');
                } else if (strcmp(lower_str, "shift") == 0 || strcmp(lower_str, "ctrl") == 0 ||
                           strcmp(lower_str, "alt") == 0 || strcmp(lower_str, "fn") == 0 ||
                           strcmp(lower_str, "caps") == 0 || strcmp(lower_str, "capslock") == 0 ||
                           strcmp(lower_str, "gui") == 0 || strcmp(lower_str, "win") == 0 ||
                           strcmp(lower_str, "cmd") == 0 || strcmp(lower_str, "opt") == 0 ||
                           strcmp(lower_str, "home") == 0 || strcmp(lower_str, "end") == 0 ||
                           strcmp(lower_str, "pageup") == 0 || strcmp(lower_str, "pagedown") == 0 ||
                           strcmp(lower_str, "pgup") == 0 || strcmp(lower_str, "pgdn") == 0 ||
                           strcmp(lower_str, "insert") == 0 || strcmp(lower_str, "prtsc") == 0 ||
                           (raw_len >= 2 && lower_str[0] == 'f' && lower_str[1] >= '1' && lower_str[1] <= '9')) {
                    // Ignore modifiers
                } else {
                    for (size_t k = 0; k < raw_len; ++k) {
                        handle_char(raw_str[k]);
                    }
                }

                // Clear INT_STA
                kb_write_reg(kbd_dev, M5_TAB5_KB_REG_INT_STA, 0x00);
                if (qlen > 16) {
                    kb_write_reg(kbd_dev, 0x02, 0x00);
                }
            }
        } else if (r != ESP_OK) {
            static int s_i2c_err_cnt = 0;
            if (++s_i2c_err_cnt > 200) {
                s_i2c_err_cnt = 0;
                kb_write_reg(kbd_dev, M5_TAB5_KB_REG_KEYBOARD_MODE, M5_TAB5_KB_MODE_STRING);
                kb_write_reg(kbd_dev, 0x00, 0x07);
                kb_write_reg(kbd_dev, 0x02, 0x00);
                kb_write_reg(kbd_dev, M5_TAB5_KB_REG_INT_STA, 0x00);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

static QueueHandle_t s_msc_queue = NULL;
enum msc_queue_evt_t {
    MSC_EVT_CONNECTED,
    MSC_EVT_DISCONNECTED,
};

typedef struct {
    msc_queue_evt_t type;
    uint8_t dev_addr;
} msc_queue_msg_t;

static msc_host_device_handle_t s_msc_device = NULL;
static msc_host_vfs_handle_t s_vfs_handle = NULL;
static bool s_usb_mounted = false;

static void msc_event_callback(const msc_host_event_t *event, void *arg)
{
    msc_queue_msg_t msg;
    if (event->event == msc_host_event_t::MSC_DEVICE_CONNECTED) {
        msg.type = MSC_EVT_CONNECTED;
        msg.dev_addr = event->device.address;
        if (s_msc_queue) {
            xQueueSend(s_msc_queue, &msg, 0);
        }
    } else if (event->event == msc_host_event_t::MSC_DEVICE_DISCONNECTED) {
        msg.type = MSC_EVT_DISCONNECTED;
        msg.dev_addr = 0;
        if (s_msc_queue) {
            xQueueSend(s_msc_queue, &msg, 0);
        }
    }
}

static void usb_msc_task(void *pvParameters)
{
    while (1) {
        msc_queue_msg_t msg;
        if (xQueueReceive(s_msc_queue, &msg, portMAX_DELAY) == pdTRUE) {
            if (msg.type == MSC_EVT_CONNECTED) {
                if (s_msc_device == NULL) {
                    ESP_LOGI(TAG, "Opening USB MSC device at address %d...", msg.dev_addr);
                    esp_err_t err = msc_host_install_device(msg.dev_addr, &s_msc_device);
                    if (err == ESP_OK) {
                        msc_host_device_info_t info;
                        if (msc_host_get_device_info(s_msc_device, &info) == ESP_OK) {
                            ESP_LOGI(TAG, "USB MSC Info: VID=0x%04X PID=0x%04X SectorSize=%u Sectors=%u",
                                     info.idVendor, info.idProduct,
                                     (unsigned int)info.sector_size, (unsigned int)info.sector_count);
                        }

                        const esp_vfs_fat_mount_config_t mount_config = {
                            .format_if_mount_failed = false,
                            .max_files = 4,
                            .allocation_unit_size = 8192,
                            .disk_status_check_enable = false,
                            .use_one_fat = false,
                        };
                        err = msc_host_vfs_register(s_msc_device, "/usb", &mount_config, &s_vfs_handle);
                        if (err == ESP_OK) {
                            s_usb_mounted = true;
                            ESP_LOGI(TAG, "=== USB Storage mounted successfully at /usb! ===");
                            DIR *d = opendir("/usb");
                            if (d) {
                                struct dirent *entry;
                                while ((entry = readdir(d)) != NULL) {
                                    ESP_LOGI(TAG, "  /usb/%s", entry->d_name);
                                }
                                closedir(d);
                            }
                        } else {
                            ESP_LOGE(TAG, "Failed to mount /usb VFS: %d", err);
                            msc_host_uninstall_device(s_msc_device);
                            s_msc_device = NULL;
                        }
                    } else {
                        ESP_LOGE(TAG, "Failed to install MSC device: %d", err);
                    }
                }
            } else if (msg.type == MSC_EVT_DISCONNECTED) {
                ESP_LOGI(TAG, "USB Storage disconnected.");
                if (s_vfs_handle) {
                    msc_host_vfs_unregister(s_vfs_handle);
                    s_vfs_handle = NULL;
                }
                if (s_msc_device) {
                    msc_host_uninstall_device(s_msc_device);
                    s_msc_device = NULL;
                }
                s_usb_mounted = false;
            }
        }
    }
}

extern "C" bool host_is_usb_mounted(void) {
    return s_usb_mounted;
}

extern "C" int32_t host_usb_unmount(void) {
    if (!s_usb_mounted) return -1;
    if (s_vfs_handle) {
        msc_host_vfs_unregister(s_vfs_handle);
        s_vfs_handle = NULL;
    }
    if (s_msc_device) {
        msc_host_uninstall_device(s_msc_device);
        s_msc_device = NULL;
    }
    s_usb_mounted = false;
    ESP_LOGI(TAG, "USB Storage cleanly unmounted.");
    return 0;
}

/* -------------------------------------------------------------------------- */
/*                 Audio Subsystem (ES8388 Speaker + ES7210 Mic)               */
/* -------------------------------------------------------------------------- */
static bool s_audio_initialized = false;
static uint32_t s_audio_volume = 80;
static bool s_key_click_enabled = false;

extern "C" void host_audio_init(void) {
    if (s_audio_initialized) return;
    ESP_LOGI(TAG, "Initializing Audio Subsystem (ES8388 DAC + ES7210 Mic)...");
    bsp_codec_init();
    bsp_codec_config_t *codec = bsp_get_codec_handle();
    if (codec && codec->set_volume) {
        codec->set_volume((int)s_audio_volume);
    }
    s_audio_initialized = true;
    ESP_LOGI(TAG, "Audio Subsystem ready. Speaker Volume: %u%%", (unsigned int)s_audio_volume);
}

extern "C" int32_t host_set_volume(uint32_t vol) {
    if (vol > 100) vol = 100;
    s_audio_volume = vol;
    if (!s_audio_initialized) host_audio_init();
    bsp_codec_config_t *codec = bsp_get_codec_handle();
    if (codec && codec->set_volume) {
        codec->set_volume((int)vol);
    }
    return 0;
}

extern "C" int32_t host_get_volume(void) {
    return (int32_t)s_audio_volume;
}

extern "C" int32_t host_play_beep(uint32_t freq_hz, uint32_t duration_ms) {
    if (!s_audio_initialized) host_audio_init();
    bsp_codec_config_t *codec = bsp_get_codec_handle();
    if (!codec || !codec->i2s_write || !codec->i2s_reconfig_clk_fn) return -1;

    if (freq_hz == 0) freq_hz = 800;
    if (duration_ms == 0) duration_ms = 60;
    if (duration_ms > 5000) duration_ms = 5000;

    const uint32_t sample_rate = 48000;
    codec->i2s_reconfig_clk_fn(sample_rate, 16, I2S_SLOT_MODE_STEREO);

    uint32_t total_samples = (sample_rate * duration_ms) / 1000;
    int16_t chunk[256 * 2]; // 256 stereo samples (1024 bytes)
    const uint32_t chunk_samples = 256;

    uint32_t samples_written = 0;
    float phase = 0.0f;
    float phase_inc = (2.0f * 3.14159265f * (float)freq_hz) / (float)sample_rate;
    int16_t amp = 10000;

    while (samples_written < total_samples) {
        uint32_t to_write = total_samples - samples_written;
        if (to_write > chunk_samples) to_write = chunk_samples;

        for (uint32_t i = 0; i < to_write; ++i) {
            int16_t sample = (int16_t)(sinf(phase) * (float)amp);
            chunk[i * 2] = sample;     // Left
            chunk[i * 2 + 1] = sample; // Right
            phase += phase_inc;
            if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;
        }

        size_t written = 0;
        codec->i2s_write(chunk, to_write * sizeof(int16_t) * 2, &written, portMAX_DELAY);
        samples_written += to_write;
    }

    // Flush small zero buffer to avoid DAC pop/clicks
    memset(chunk, 0, sizeof(chunk));
    size_t written = 0;
    codec->i2s_write(chunk, 64 * sizeof(int16_t) * 2, &written, portMAX_DELAY);
    return 0;
}

extern "C" void host_play_chime(void) {
    // 3 pleasant harmonic tones: C5 (523Hz), E5 (659Hz), G5 (784Hz)
    host_play_beep(523, 70);
    vTaskDelay(pdMS_TO_TICKS(20));
    host_play_beep(659, 70);
    vTaskDelay(pdMS_TO_TICKS(20));
    host_play_beep(784, 140);
}

struct WavHeader_t {
    char riff[4];
    uint32_t file_size;
    char wave[4];
    char fmt[4];
    uint32_t fmt_len;
    uint16_t format_tag;
    uint16_t channels;
    uint32_t sample_rate;
    uint32_t byte_rate;
    uint16_t block_align;
    uint16_t bits_per_sample;
};

extern "C" int32_t host_play_wav(const char *path) {
    if (!path || !*path) return -1;
    if (!s_audio_initialized) host_audio_init();

    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "Cannot open WAV file: %s", path);
        return -2;
    }

    WavHeader_t hdr;
    if (fread(&hdr, 1, sizeof(hdr), f) < sizeof(hdr)) {
        fclose(f);
        return -3;
    }
    if (memcmp(hdr.riff, "RIFF", 4) != 0 || memcmp(hdr.wave, "WAVE", 4) != 0) {
        fclose(f);
        return -4;
    }

    // Skip fmt extra bytes if fmt_len > 16
    if (hdr.fmt_len > 16) {
        fseek(f, hdr.fmt_len - 16, SEEK_CUR);
    }

    // Locate "data" chunk
    char chunk_id[4];
    uint32_t data_bytes = 0;
    while (fread(chunk_id, 1, 4, f) == 4) {
        if (fread(&data_bytes, 1, 4, f) != 4) break;
        if (memcmp(chunk_id, "data", 4) == 0) break;
        fseek(f, data_bytes, SEEK_CUR);
    }

    bsp_codec_config_t *codec = bsp_get_codec_handle();
    if (!codec || !codec->i2s_write || !codec->i2s_reconfig_clk_fn) {
        fclose(f);
        return -5;
    }

    codec->i2s_reconfig_clk_fn(hdr.sample_rate, hdr.bits_per_sample,
                               (hdr.channels == 1) ? I2S_SLOT_MODE_MONO : I2S_SLOT_MODE_STEREO);

    char buf[2048];
    uint32_t remaining = data_bytes;
    while (remaining > 0 && !feof(f)) {
        size_t to_read = (remaining > sizeof(buf)) ? sizeof(buf) : remaining;
        size_t n = fread(buf, 1, to_read, f);
        if (n == 0) break;
        size_t written = 0;
        codec->i2s_write(buf, n, &written, portMAX_DELAY);
        remaining -= (uint32_t)n;
    }

    fclose(f);
    return 0;
}

extern "C" int32_t host_record_wav(const char *path, uint32_t duration_sec) {
    if (!path || !*path) return -1;
    if (duration_sec == 0) duration_sec = 5;
    if (duration_sec > 60) duration_sec = 60;
    if (!s_audio_initialized) host_audio_init();

    bsp_codec_config_t *codec = bsp_get_codec_handle();
    if (!codec || !codec->i2s_read || !codec->codec_reconfig_fn) return -2;

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Cannot create record file: %s", path);
        return -3;
    }

    const uint32_t sample_rate = 48000;
    const uint16_t channels = 2; // Stereo
    const uint16_t bits = 16;
    uint32_t byte_rate = sample_rate * channels * (bits / 8);
    uint16_t block_align = channels * (bits / 8);

    uint8_t wav_hdr[44];
    memset(wav_hdr, 0, sizeof(wav_hdr));
    memcpy(wav_hdr, "RIFF", 4);
    memcpy(wav_hdr + 8, "WAVEfmt ", 8);
    uint32_t fmt_len = 16;
    uint16_t fmt_tag = 1;
    memcpy(wav_hdr + 16, &fmt_len, 4);
    memcpy(wav_hdr + 20, &fmt_tag, 2);
    memcpy(wav_hdr + 22, &channels, 2);
    memcpy(wav_hdr + 24, &sample_rate, 4);
    memcpy(wav_hdr + 28, &byte_rate, 4);
    memcpy(wav_hdr + 32, &block_align, 2);
    memcpy(wav_hdr + 34, &bits, 2);
    memcpy(wav_hdr + 36, "data", 4);
    fwrite(wav_hdr, 1, 44, f);

    // Configure ES7210 4-channel microphone
    if (codec->set_in_gain) {
        codec->set_in_gain(180.0f);
    }
    codec->codec_reconfig_fn(sample_rate, bits, (i2s_slot_mode_t)4);

    const size_t frames_per_chunk = 512;
    int16_t mic_raw[frames_per_chunk * 4];
    int16_t stereo_out[frames_per_chunk * 2];

    uint32_t total_frames = sample_rate * duration_sec;
    uint32_t frames_recorded = 0;
    uint32_t total_data_bytes = 0;

    while (frames_recorded < total_frames) {
        size_t frames_to_read = total_frames - frames_recorded;
        if (frames_to_read > frames_per_chunk) frames_to_read = frames_per_chunk;

        size_t bytes_read = 0;
        esp_err_t err = codec->i2s_read((char *)mic_raw, frames_to_read * 4 * sizeof(int16_t), &bytes_read, portMAX_DELAY);
        if (err != ESP_OK || bytes_read == 0) break;

        size_t actual_frames = bytes_read / (4 * sizeof(int16_t));
        for (size_t i = 0; i < actual_frames; ++i) {
            stereo_out[i * 2 + 0] = mic_raw[i * 4 + 0]; // MIC-L
            stereo_out[i * 2 + 1] = mic_raw[i * 4 + 2]; // MIC-R
        }

        size_t out_bytes = actual_frames * 2 * sizeof(int16_t);
        fwrite(stereo_out, 1, out_bytes, f);
        total_data_bytes += (uint32_t)out_bytes;
        frames_recorded += (uint32_t)actual_frames;
    }

    uint32_t riff_size = total_data_bytes + 36;
    fseek(f, 4, SEEK_SET);
    fwrite(&riff_size, 4, 1, f);
    fseek(f, 40, SEEK_SET);
    fwrite(&total_data_bytes, 4, 1, f);
    fclose(f);

    ESP_LOGI(TAG, "Recorded %u bytes to %s", (unsigned int)total_data_bytes, path);
    return (int32_t)total_data_bytes;
}

/* =========================================================================
 * Wi-Fi Subsystem (ESP32-C6 Hosted over SDIO + SNTP Time Sync)
 * ========================================================================= */
static bool s_wifi_initialized = false;
static bool s_wifi_connected = false;
static char s_wifi_ip[32] = "0.0.0.0";
static char s_wifi_ssid[64] = "";
static esp_netif_t *s_sta_netif = NULL;

extern "C" int32_t host_webserver_start(uint16_t port);
extern "C" int32_t host_webserver_stop(void);
extern "C" int32_t host_webserver_status(char *buf, size_t max_len);

static void wifi_event_handler(void *arg, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT) {
        if (id == WIFI_EVENT_STA_START) {
            ESP_LOGI(TAG, "Wi-Fi STA started");
        } else if (id == WIFI_EVENT_STA_CONNECTED) {
            ESP_LOGI(TAG, "Wi-Fi connected to AP '%s'", s_wifi_ssid);
        } else if (id == WIFI_EVENT_STA_DISCONNECTED) {
            ESP_LOGW(TAG, "Wi-Fi disconnected from AP");
            s_wifi_connected = false;
            strcpy(s_wifi_ip, "0.0.0.0");
            host_webserver_stop();
        }
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)data;
        snprintf(s_wifi_ip, sizeof(s_wifi_ip), IPSTR, IP2STR(&event->ip_info.ip));
        s_wifi_connected = true;
        ESP_LOGI(TAG, "Wi-Fi got IP: %s", s_wifi_ip);
        host_webserver_start(80);
    }
}

extern "C" int32_t host_wifi_init(void)
{
    if (s_wifi_initialized) return 0;

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    ESP_LOGI(TAG, "Initializing ESP-Hosted SDIO communication with ESP32-C6...");
    esp_hosted_host_init();

    ESP_ERROR_CHECK(esp_netif_init());
    esp_err_t loop_err = esp_event_loop_create_default();
    if (loop_err != ESP_OK && loop_err != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "esp_event_loop_create_default: %d", loop_err);
    }

    if (!s_sta_netif) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }
    assert(s_sta_netif);

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ret = esp_wifi_init(&cfg);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed: %d", ret);
        return -1;
    }

    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, NULL);

    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_start();

    // Enable on-board antenna via PI4IO GPIO expander
    bsp_set_ext_antenna_enable(true);

    s_wifi_initialized = true;
    ESP_LOGI(TAG, "Wi-Fi Station subsystem initialized and started!");
    return 0;
}

extern "C" int32_t host_wifi_scan(char *buf, size_t max_len)
{
    if (!buf || max_len == 0) return -1;
    if (!s_wifi_initialized) {
        int32_t init_rc = host_wifi_init();
        if (init_rc != 0) return -2;
    }

    wifi_scan_config_t scan_config;
    memset(&scan_config, 0, sizeof(scan_config));
    scan_config.show_hidden = true;
    scan_config.scan_type = WIFI_SCAN_TYPE_ACTIVE;
    scan_config.scan_time.active.min = 100;
    scan_config.scan_time.active.max = 250;

    esp_err_t err = esp_wifi_scan_start(&scan_config, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_scan_start failed: %d", err);
        return -3;
    }

    uint16_t ap_count = 0;
    esp_wifi_scan_get_ap_num(&ap_count);
    if (ap_count == 0) {
        snprintf(buf, max_len, "No Wi-Fi networks found.\n");
        return 0;
    }

    if (ap_count > 20) ap_count = 20;
    wifi_ap_record_t *ap_list = (wifi_ap_record_t *)malloc(ap_count * sizeof(wifi_ap_record_t));
    if (!ap_list) return -4;

    esp_wifi_scan_get_ap_records(&ap_count, ap_list);

    size_t offset = 0;
    offset += snprintf(buf + offset, max_len - offset,
                       " %-4s %-24s %-6s %-8s %s\n", "CH", "SSID", "RSSI", "AUTH", "BSSID");
    offset += snprintf(buf + offset, max_len - offset,
                       " ------------------------------------------------------------\n");

    for (int i = 0; i < ap_count && offset < max_len - 80; i++) {
        const char *auth = "OPEN";
        switch (ap_list[i].authmode) {
            case WIFI_AUTH_WEP: auth = "WEP"; break;
            case WIFI_AUTH_WPA_PSK: auth = "WPA"; break;
            case WIFI_AUTH_WPA2_PSK: auth = "WPA2"; break;
            case WIFI_AUTH_WPA_WPA2_PSK: auth = "WPA/2"; break;
            case WIFI_AUTH_WPA3_PSK: auth = "WPA3"; break;
            case WIFI_AUTH_WPA2_WPA3_PSK: auth = "WPA2/3"; break;
            default: break;
        }

        char bssid_str[20];
        snprintf(bssid_str, sizeof(bssid_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                 ap_list[i].bssid[0], ap_list[i].bssid[1], ap_list[i].bssid[2],
                 ap_list[i].bssid[3], ap_list[i].bssid[4], ap_list[i].bssid[5]);

        char ssid_safe[33];
        strncpy(ssid_safe, (const char *)ap_list[i].ssid, 32);
        ssid_safe[32] = '\0';
        if (strlen(ssid_safe) == 0) strcpy(ssid_safe, "<hidden>");

        offset += snprintf(buf + offset, max_len - offset,
                           " [%02d] %-24.24s %4d dBm %-8s %s\n",
                           ap_list[i].primary, ssid_safe, ap_list[i].rssi, auth, bssid_str);
    }
    free(ap_list);
    return (int32_t)ap_count;
}

extern "C" int32_t host_wifi_connect(const char *ssid, const char *pass)
{
    if (!ssid || !*ssid) return -1;
    if (!s_wifi_initialized) {
        int32_t init_rc = host_wifi_init();
        if (init_rc != 0) return -2;
    }

    esp_wifi_disconnect();
    vTaskDelay(pdMS_TO_TICKS(100));

    wifi_config_t wifi_cfg;
    memset(&wifi_cfg, 0, sizeof(wifi_cfg));
    strncpy((char *)wifi_cfg.sta.ssid, ssid, sizeof(wifi_cfg.sta.ssid) - 1);
    if (pass && *pass) {
        strncpy((char *)wifi_cfg.sta.password, pass, sizeof(wifi_cfg.sta.password) - 1);
    }
    wifi_cfg.sta.threshold.authmode = (pass && *pass) ? WIFI_AUTH_WPA2_PSK : WIFI_AUTH_OPEN;

    esp_err_t err = esp_wifi_set_config(WIFI_IF_STA, &wifi_cfg);
    if (err != ESP_OK) return -3;

    strncpy(s_wifi_ssid, ssid, sizeof(s_wifi_ssid) - 1);
    err = esp_wifi_connect();
    if (err != ESP_OK) return -4;

    return 0;
}

extern "C" int32_t host_wifi_disconnect(void)
{
    if (!s_wifi_initialized) return 0;
    esp_wifi_disconnect();
    s_wifi_connected = false;
    strcpy(s_wifi_ip, "0.0.0.0");
    s_wifi_ssid[0] = '\0';
    return 0;
}

extern "C" int32_t host_wifi_status(char *buf, size_t max_len)
{
    if (!buf || max_len == 0) return -1;
    if (!s_wifi_initialized) {
        snprintf(buf, max_len, "Wi-Fi state: Uninitialized (run 'wifi scan' or 'wifi connect')\n");
        return 0;
    }
    if (s_wifi_connected) {
        wifi_ap_record_t ap_info;
        int rssi = 0;
        if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
            rssi = ap_info.rssi;
        }
        snprintf(buf, max_len, "Wi-Fi state: Connected\n  SSID: %s\n  IPv4: %s\n  RSSI: %d dBm\n",
                 s_wifi_ssid, s_wifi_ip, rssi);
        return 1;
    } else {
        snprintf(buf, max_len, "Wi-Fi state: Disconnected\n  Interface: STA (ESP32-C6 Hosted)\n");
        return 0;
    }
}

extern "C" int32_t host_sntp_sync(const char *server)
{
    if (!s_wifi_connected) {
        return -1;
    }
    const char *srv = (server && *server) ? server : "pool.ntp.org";
    esp_sntp_config_t config = ESP_NETIF_SNTP_DEFAULT_CONFIG(srv);
    config.wait_for_sync = true;
    config.start = true;

    esp_netif_sntp_init(&config);
    esp_err_t err = esp_netif_sntp_sync_wait(pdMS_TO_TICKS(8000));
    if (err == ESP_OK) {
        setenv("TZ", "BRT3", 1);
        tzset();
        return 0;
    }
    return -2;
}

extern "C" int32_t host_get_datetime(char *buf, size_t max_len)
{
    if (!buf || max_len == 0) return -1;
    time_t now = time(NULL);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);
    strftime(buf, max_len, "%Y-%m-%d %H:%M:%S", &timeinfo);
    return 0;
}

/* =========================================================================
 * Subsystem: MIPI CSI Camera, Video Recorder & Video Player (Etapa 6)
 * ========================================================================= */
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include "linux/videodev2.h"
#include "esp_video_init.h"
#include "esp_video_device.h"
#include "driver/ppa.h"
#include "driver/jpeg_encode.h"
#include "driver/jpeg_decode.h"

#define CAM_BUF_COUNT 2
#define CAM_WIDTH 1280
#define CAM_HEIGHT 720
#define MVID_MAGIC 0x4449564D  /* "MVID" in little-endian */

typedef struct {
    int fd;
    uint32_t width;
    uint32_t height;
    uint32_t pixel_format;
    uint8_t *buffer[CAM_BUF_COUNT];
    bool streaming;
} host_cam_t;

static host_cam_t s_host_cam = { -1, 0, 0, 0, {NULL, NULL}, false };
static bool s_video_subsys_inited = false;
static ppa_client_handle_t s_ppa_client = NULL;
static SemaphoreHandle_t s_cam_web_mutex = NULL;

static void ensure_cam_mutex(void)
{
    if (!s_cam_web_mutex) {
        s_cam_web_mutex = xSemaphoreCreateMutex();
    }
}

static esp_err_t ensure_ppa_client(void)
{
    if (s_ppa_client) return ESP_OK;
    ppa_client_config_t ppa_cfg = {
        .oper_type = PPA_OPERATION_SRM,
        .max_pending_trans_num = 1,
        .data_burst_length = PPA_DATA_BURST_LENGTH_128,
    };
    return ppa_register_client(&ppa_cfg, &s_ppa_client);
}

extern "C" int32_t host_camera_init(void)
{
    if (s_host_cam.streaming && s_host_cam.fd >= 0) return 0;

    static esp_video_init_csi_config_t csi_config = {
        .sccb_config = {
            .init_sccb = false,
            .i2c_handle = NULL,
            .freq = 400000,
        },
        .reset_pin = -1,
        .pwdn_pin = -1,
    };
    csi_config.sccb_config.i2c_handle = bsp_i2c_get_handle();

    esp_video_init_config_t cam_config = {
        .csi = &csi_config,
        .dvp = NULL,
        .jpeg = NULL,
    };

    if (!s_video_subsys_inited) {
        esp_err_t err = esp_video_init(&cam_config);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "esp_video_init failed: %d (%s)", err, esp_err_to_name(err));
            return -1;
        }
        s_video_subsys_inited = true;
    }

    int fd = open(ESP_VIDEO_MIPI_CSI_DEVICE_NAME, O_RDWR);
    if (fd < 0) {
        ESP_LOGE(TAG, "Failed to open video device %s (errno %d)", ESP_VIDEO_MIPI_CSI_DEVICE_NAME, errno);
        return -2;
    }

    struct v4l2_format fmt;
    memset(&fmt, 0, sizeof(fmt));
    fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_G_FMT, &fmt) != 0) {
        ESP_LOGE(TAG, "VIDIOC_G_FMT failed");
        close(fd);
        return -3;
    }

    if (fmt.fmt.pix.pixelformat != V4L2_PIX_FMT_RGB565) {
        fmt.fmt.pix.pixelformat = V4L2_PIX_FMT_RGB565;
        if (ioctl(fd, VIDIOC_S_FMT, &fmt) != 0) {
            ESP_LOGE(TAG, "VIDIOC_S_FMT failed");
            close(fd);
            return -4;
        }
    }

    struct v4l2_requestbuffers req;
    memset(&req, 0, sizeof(req));
    req.count = CAM_BUF_COUNT;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if (ioctl(fd, VIDIOC_REQBUFS, &req) != 0) {
        ESP_LOGE(TAG, "VIDIOC_REQBUFS failed");
        close(fd);
        return -5;
    }

    for (int i = 0; i < CAM_BUF_COUNT; i++) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = i;
        if (ioctl(fd, VIDIOC_QUERYBUF, &buf) != 0) {
            ESP_LOGE(TAG, "VIDIOC_QUERYBUF failed (%d)", i);
            close(fd);
            return -6;
        }

        s_host_cam.buffer[i] = (uint8_t *)mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
        if (!s_host_cam.buffer[i]) {
            ESP_LOGE(TAG, "mmap buffer failed (%d)", i);
            close(fd);
            return -7;
        }

        if (ioctl(fd, VIDIOC_QBUF, &buf) != 0) {
            ESP_LOGE(TAG, "VIDIOC_QBUF failed (%d)", i);
            close(fd);
            return -8;
        }
    }

    int type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    if (ioctl(fd, VIDIOC_STREAMON, &type) != 0) {
        ESP_LOGE(TAG, "VIDIOC_STREAMON failed");
        close(fd);
        return -9;
    }

    s_host_cam.fd = fd;
    s_host_cam.width = fmt.fmt.pix.width;
    s_host_cam.height = fmt.fmt.pix.height;
    s_host_cam.pixel_format = fmt.fmt.pix.pixelformat;
    s_host_cam.streaming = true;

    ensure_ppa_client();
    ESP_LOGI(TAG, "MIPI CSI Camera opened successfully: %lux%lu", (unsigned long)s_host_cam.width, (unsigned long)s_host_cam.height);
    return 0;
}

extern "C" int32_t host_camera_status(char *buf, size_t max_len)
{
    if (!buf || max_len == 0) return -1;
    if (s_host_cam.fd >= 0 && s_host_cam.streaming) {
        snprintf(buf, max_len, "Camera: Active (%lux%lu RGB565 on %s)\n",
                 (unsigned long)s_host_cam.width, (unsigned long)s_host_cam.height,
                 ESP_VIDEO_MIPI_CSI_DEVICE_NAME);
    } else {
        snprintf(buf, max_len, "Camera: Ready / Idle (SC202CS MIPI CSI 1280x720)\n");
    }
    return 0;
}

#pragma pack(push, 1)
typedef struct {
    uint16_t bfType;          // 'BM' (0x4D42)
    uint32_t bfSize;          // file size
    uint16_t bfReserved1;     // 0
    uint16_t bfReserved2;     // 0
    uint32_t bfOffBits;       // 66
    uint32_t biSize;          // 40
    int32_t  biWidth;         // 1280
    int32_t  biHeight;        // -720
    uint16_t biPlanes;        // 1
    uint16_t biBitCount;      // 16
    uint32_t biCompression;   // 3 (BI_BITFIELDS)
    uint32_t biSizeImage;     // 1280 * 720 * 2
    int32_t  biXPelsPerMeter; // 2835
    int32_t  biYPelsPerMeter; // 2835
    uint32_t biClrUsed;       // 0
    uint32_t biClrImportant;  // 0
    uint32_t red_mask;        // 0xF800
    uint32_t green_mask;      // 0x07E0
    uint32_t blue_mask;       // 0x001F
} bmp_rgb565_header_t;

typedef struct {
    uint32_t magic;           // "MVID"
    uint16_t width;           // e.g. 640
    uint16_t height;          // e.g. 360
    uint16_t fps;             // e.g. 15
    uint16_t pixel_fmt;       // 0 = RGB565
    uint32_t frame_count;     // number of frames
    uint32_t frame_size;      // width * height * 2
    uint32_t reserved[4];
} mvid_header_t;
#pragma pack(pop)

static void ensure_parent_dirs(const char *path)
{
    if (!path || !*path) return;
    char tmp[256];
    strncpy(tmp, path, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';
    char *p = strchr(tmp + 1, '/');
    while (p) {
        *p = '\0';
        mkdir(tmp, 0777);
        *p = '/';
        p = strchr(p + 1, '/');
    }
}

extern "C" int32_t host_camera_snap(const char *path)
{
    ensure_cam_mutex();
    if (xSemaphoreTake(s_cam_web_mutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
        ESP_LOGE(TAG, "Camera busy (mutex held)");
        return -10;
    }

    if (host_camera_init() != 0) {
        xSemaphoreGive(s_cam_web_mutex);
        return -1;
    }
    if (!path || !*path) {
        xSemaphoreGive(s_cam_web_mutex);
        return -2;
    }

    ensure_parent_dirs(path);

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    int dq_ret = -1;
    for (int retry = 0; retry < 50; retry++) {
        dq_ret = ioctl(s_host_cam.fd, VIDIOC_DQBUF, &buf);
        if (dq_ret == 0) break;
        vTaskDelay(pdMS_TO_TICKS(20));
    }

    if (dq_ret != 0) {
        ESP_LOGE(TAG, "Camera snap DQBUF failed after retries: %d (errno %d)", dq_ret, errno);
        xSemaphoreGive(s_cam_web_mutex);
        return -3;
    }

    uint8_t *frame_ptr = s_host_cam.buffer[buf.index];
    size_t frame_bytes = s_host_cam.width * s_host_cam.height * 2;

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "Camera snap failed to open '%s' (errno %d)", path, errno);
        ioctl(s_host_cam.fd, VIDIOC_QBUF, &buf);
        xSemaphoreGive(s_cam_web_mutex);
        return -4;
    }

    const char *ext = strrchr(path, '.');
    int32_t res = 0;

    if (ext && (strcasecmp(ext, ".bmp") == 0)) {
        bmp_rgb565_header_t bmp_hdr;
        memset(&bmp_hdr, 0, sizeof(bmp_hdr));
        bmp_hdr.bfType = 0x4D42;
        bmp_hdr.bfSize = sizeof(bmp_hdr) + frame_bytes;
        bmp_hdr.bfOffBits = sizeof(bmp_hdr);
        bmp_hdr.biSize = 40;
        bmp_hdr.biWidth = s_host_cam.width;
        bmp_hdr.biHeight = -(int32_t)s_host_cam.height;
        bmp_hdr.biPlanes = 1;
        bmp_hdr.biBitCount = 16;
        bmp_hdr.biCompression = 3;
        bmp_hdr.biSizeImage = frame_bytes;
        bmp_hdr.biXPelsPerMeter = 2835;
        bmp_hdr.biYPelsPerMeter = 2835;
        bmp_hdr.red_mask = 0xF800;
        bmp_hdr.green_mask = 0x07E0;
        bmp_hdr.blue_mask = 0x001F;

        fwrite(&bmp_hdr, 1, sizeof(bmp_hdr), f);
        fwrite(frame_ptr, 1, frame_bytes, f);
        fflush(f);
    } else if (ext && (strcasecmp(ext, ".jpg") == 0 || strcasecmp(ext, ".jpeg") == 0)) {
        jpeg_encoder_handle_t jpeg_enc = NULL;
        jpeg_encode_engine_cfg_t eng_cfg = {
            .intr_priority = 0,
            .timeout_ms = 500,
        };
        esp_err_t jerr = jpeg_new_encoder_engine(&eng_cfg, &jpeg_enc);
        if (jerr == ESP_OK) {
            jpeg_encode_cfg_t enc_cfg = {
                .height = (uint32_t)s_host_cam.height,
                .width = (uint32_t)s_host_cam.width,
                .src_type = JPEG_ENCODE_IN_FORMAT_RGB565,
                .sub_sample = JPEG_DOWN_SAMPLING_YUV422,
                .image_quality = 85,
            };
            jpeg_encode_memory_alloc_cfg_t rx_mem_cfg = {
                .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER,
            };
            size_t alloc_size = 0;
            uint32_t max_out = 512 * 1024;
            uint8_t *jpg_out = (uint8_t *)jpeg_alloc_encoder_mem(max_out, &rx_mem_cfg, &alloc_size);
            if (jpg_out) {
                uint32_t actual_out = 0;
                jerr = jpeg_encoder_process(jpeg_enc, &enc_cfg, frame_ptr, frame_bytes, jpg_out, alloc_size, &actual_out);
                if (jerr == ESP_OK && actual_out > 0) {
                    fwrite(jpg_out, 1, actual_out, f);
                    fflush(f);
                } else {
                    ESP_LOGE(TAG, "jpeg_encoder_process failed: %d, actual: %lu", jerr, (unsigned long)actual_out);
                    res = -5;
                }
                free(jpg_out);
            } else {
                ESP_LOGE(TAG, "jpeg_alloc_encoder_mem failed");
                res = -6;
            }
            jpeg_del_encoder_engine(jpeg_enc);
        } else {
            ESP_LOGE(TAG, "jpeg_new_encoder_engine failed: %d", jerr);
            res = -7;
        }
    } else {
        /* Raw RGB565 */
        fwrite(frame_ptr, 1, frame_bytes, f);
        fflush(f);
    }

    fclose(f);
    ioctl(s_host_cam.fd, VIDIOC_QBUF, &buf);
    xSemaphoreGive(s_cam_web_mutex);
    return res;
}

extern "C" int32_t host_camera_view(uint32_t duration_sec)
{
    ensure_cam_mutex();
    if (xSemaphoreTake(s_cam_web_mutex, pdMS_TO_TICKS(2500)) != pdTRUE) {
        ESP_LOGE(TAG, "Camera busy (mutex held)");
        return -10;
    }

    if (host_camera_init() != 0) {
        xSemaphoreGive(s_cam_web_mutex);
        return -1;
    }
    ensure_ppa_client();
    if (!s_ppa_client || !s_fb) {
        xSemaphoreGive(s_cam_web_mutex);
        return -2;
    }

    uint32_t max_frames = (duration_sec > 0) ? (duration_sec * 30) : 300;
    uint32_t frame_count = 0;

    /* Drain any residual key press from command submission */
    vTaskDelay(pdMS_TO_TICKS(50));
    while (host_read_key() > 0) {}

    struct v4l2_buffer buf;
    while (frame_count < max_frames) {
        /* Check if ESC or q pressed */
        int32_t k = host_read_key();
        if (k == 27 || k == 'q' || k == 'Q') break;

        /* Check if screen touched */
        int32_t tx, ty, tp;
        if (host_get_touch(&tx, &ty, &tp) == 0 && tp) break;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(s_host_cam.fd, VIDIOC_DQBUF, &buf) != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        ppa_srm_oper_config_t srm;
        memset(&srm, 0, sizeof(srm));
        srm.in.buffer = s_host_cam.buffer[buf.index];
        srm.in.pic_w = 1280;
        srm.in.pic_h = 720;
        srm.in.block_w = 1280;
        srm.in.block_h = 720;
        srm.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

        srm.out.buffer = s_fb;
        srm.out.buffer_size = 720 * 1280 * 2;
        srm.out.pic_w = 720;
        srm.out.pic_h = 1280;
        srm.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

        srm.rotation_angle = PPA_SRM_ROTATION_ANGLE_90;
        srm.scale_x = 1.0f;
        srm.scale_y = 1.0f;
        srm.mode = PPA_TRANS_MODE_BLOCKING;

        ppa_do_scale_rotate_mirror(s_ppa_client, &srm);
        fb_sync();

        ioctl(s_host_cam.fd, VIDIOC_QBUF, &buf);
        frame_count++;
        vTaskDelay(pdMS_TO_TICKS(5));
    }

    xSemaphoreGive(s_cam_web_mutex);
    if (!s_graphics_exclusive) {
        init_terminal_screen();
    }
    return frame_count;
}

extern "C" int32_t host_video_record(const char *path, uint32_t duration_sec, uint32_t fps)
{
    ensure_cam_mutex();
    if (xSemaphoreTake(s_cam_web_mutex, pdMS_TO_TICKS(3000)) != pdTRUE) {
        ESP_LOGE(TAG, "Camera busy (mutex held)");
        return -10;
    }

    if (host_camera_init() != 0) {
        xSemaphoreGive(s_cam_web_mutex);
        return -1;
    }
    if (!path || !*path) {
        xSemaphoreGive(s_cam_web_mutex);
        return -2;
    }

    ensure_parent_dirs(path);

    if (fps == 0 || fps > 30) fps = 15;
    if (duration_sec == 0) duration_sec = 5;

    FILE *f = fopen(path, "wb");
    if (!f) {
        ESP_LOGE(TAG, "host_video_record failed to open '%s' (errno %d)", path, errno);
        xSemaphoreGive(s_cam_web_mutex);
        return -3;
    }

    setvbuf(f, NULL, _IOFBF, 32 * 1024);

    mvid_header_t hdr;
    memset(&hdr, 0, sizeof(hdr));
    hdr.magic = MVID_MAGIC;
    hdr.width = (uint16_t)s_host_cam.width;
    hdr.height = (uint16_t)s_host_cam.height;
    hdr.fps = (uint16_t)fps;
    hdr.pixel_fmt = 1; /* 1 = Hardware MJPEG */
    hdr.frame_count = 0;
    hdr.frame_size = 0;

    fwrite(&hdr, 1, sizeof(hdr), f);

    jpeg_encoder_handle_t jpeg_enc = NULL;
    jpeg_encode_engine_cfg_t eng_cfg = {
        .intr_priority = 0,
        .timeout_ms = 200,
    };
    esp_err_t jerr = jpeg_new_encoder_engine(&eng_cfg, &jpeg_enc);
    if (jerr != ESP_OK) {
        ESP_LOGE(TAG, "jpeg_new_encoder_engine failed: %d", jerr);
        fclose(f);
        xSemaphoreGive(s_cam_web_mutex);
        return -4;
    }

    jpeg_encode_cfg_t enc_cfg = {
        .height = (uint32_t)s_host_cam.height,
        .width = (uint32_t)s_host_cam.width,
        .src_type = JPEG_ENCODE_IN_FORMAT_RGB565,
        .sub_sample = JPEG_DOWN_SAMPLING_YUV422,
        .image_quality = 75,
    };
    jpeg_encode_memory_alloc_cfg_t rx_mem_cfg = {
        .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER,
    };
    size_t alloc_size = 0;
    uint32_t max_out = 384 * 1024;
    uint8_t *jpg_out = (uint8_t *)jpeg_alloc_encoder_mem(max_out, &rx_mem_cfg, &alloc_size);
    if (!jpg_out) {
        ESP_LOGE(TAG, "jpeg_alloc_encoder_mem failed");
        jpeg_del_encoder_engine(jpeg_enc);
        fclose(f);
        xSemaphoreGive(s_cam_web_mutex);
        return -5;
    }

    uint32_t target_frames = duration_sec * fps;
    uint32_t delay_ms = 1000 / fps;
    struct v4l2_buffer buf;
    size_t frame_raw_bytes = s_host_cam.width * s_host_cam.height * 2;

    /* Drain any residual key press */
    vTaskDelay(pdMS_TO_TICKS(50));
    while (host_read_key() > 0) {}

    for (uint32_t i = 0; i < target_frames; i++) {
        int32_t k = host_read_key();
        if (k == 27 || k == 'q' || k == 'Q') break;

        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        int dq_ret = -1;
        for (int r = 0; r < 5; r++) {
            dq_ret = ioctl(s_host_cam.fd, VIDIOC_DQBUF, &buf);
            if (dq_ret == 0) break;
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        if (dq_ret != 0) {
            continue;
        }

        uint8_t *frame_ptr = s_host_cam.buffer[buf.index];
        uint32_t actual_out = 0;

        jerr = jpeg_encoder_process(jpeg_enc, &enc_cfg, frame_ptr, frame_raw_bytes, jpg_out, alloc_size, &actual_out);
        ioctl(s_host_cam.fd, VIDIOC_QBUF, &buf);

        if (jerr == ESP_OK && actual_out > 0) {
            uint32_t fsz = actual_out;
            fwrite(&fsz, 1, sizeof(fsz), f);
            fwrite(jpg_out, 1, actual_out, f);
            hdr.frame_count++;
        }

        vTaskDelay(pdMS_TO_TICKS(delay_ms > 15 ? delay_ms - 5 : 10));
    }

    fseek(f, 0, SEEK_SET);
    fwrite(&hdr, 1, sizeof(hdr), f);
    fflush(f);
    fclose(f);

    free(jpg_out);
    jpeg_del_encoder_engine(jpeg_enc);
    xSemaphoreGive(s_cam_web_mutex);

    ESP_LOGI(TAG, "Hardware MJPEG Video recorded: %u frames to %s", (unsigned int)hdr.frame_count, path);
    return (int32_t)hdr.frame_count;
}

extern "C" int32_t host_video_play(const char *path, uint32_t fps_override)
{
    ensure_ppa_client();
    if (!s_ppa_client || !s_fb) return -1;
    if (!path || !*path) return -2;

    FILE *f = fopen(path, "rb");
    if (!f) return -3;

    mvid_header_t hdr;
    if (fread(&hdr, 1, sizeof(hdr), f) != sizeof(hdr) || hdr.magic != MVID_MAGIC) {
        fclose(f);
        return -4;
    }

    uint32_t fps = (fps_override > 0) ? fps_override : (hdr.fps > 0 ? hdr.fps : 15);
    uint32_t delay_ms = 1000 / fps;
    uint32_t played = 0;

    setvbuf(f, NULL, _IOFBF, 32 * 1024);

    if (hdr.pixel_fmt == 1) {
        /* Hardware MJPEG playback */
        jpeg_decode_engine_cfg_t dec_eng_cfg = {
            .intr_priority = 0,
            .timeout_ms = 100,
        };
        jpeg_decoder_handle_t jpeg_dec = NULL;
        esp_err_t jerr = jpeg_new_decoder_engine(&dec_eng_cfg, &jpeg_dec);
        if (jerr != ESP_OK) {
            fclose(f);
            return -5;
        }

        jpeg_decode_cfg_t dec_cfg = {
            .output_format = JPEG_DECODE_OUT_FORMAT_RGB565,
            .rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR,
            .conv_std = JPEG_YUV_RGB_CONV_STD_BT601,
        };

        size_t raw_size = 1280 * 720 * 2;
        uint8_t *raw_buf = (uint8_t *)heap_caps_aligned_alloc(64, raw_size, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
        uint8_t *jpg_in = (uint8_t *)heap_caps_aligned_alloc(64, 384 * 1024, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);

        if (!raw_buf || !jpg_in) {
            if (raw_buf) free(raw_buf);
            if (jpg_in) free(jpg_in);
            jpeg_del_decoder_engine(jpeg_dec);
            fclose(f);
            return -6;
        }

        /* Drain keypress */
        vTaskDelay(pdMS_TO_TICKS(50));
        while (host_read_key() > 0) {}

        for (uint32_t i = 0; i < hdr.frame_count; i++) {
            int32_t k = host_read_key();
            if (k == 27 || k == 'q' || k == 'Q' || k == ' ') break;

            int32_t tx, ty, tp;
            if (host_get_touch(&tx, &ty, &tp) == 0 && tp) break;

            uint32_t fsz = 0;
            if (fread(&fsz, 1, sizeof(fsz), f) != sizeof(fsz)) break;
            if (fsz == 0 || fsz > 384 * 1024) break;

            if (fread(jpg_in, 1, fsz, f) != fsz) break;

            uint32_t actual_raw = 0;
            jerr = jpeg_decoder_process(jpeg_dec, &dec_cfg, jpg_in, fsz, raw_buf, raw_size, &actual_raw);
            if (jerr == ESP_OK) {
                ppa_srm_oper_config_t srm;
                memset(&srm, 0, sizeof(srm));
                srm.in.buffer = raw_buf;
                srm.in.pic_w = 1280;
                srm.in.pic_h = 720;
                srm.in.block_w = 1280;
                srm.in.block_h = 720;
                srm.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

                srm.out.buffer = s_fb;
                srm.out.buffer_size = 720 * 1280 * 2;
                srm.out.pic_w = 720;
                srm.out.pic_h = 1280;
                srm.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

                srm.rotation_angle = PPA_SRM_ROTATION_ANGLE_90;
                srm.scale_x = 1.0f;
                srm.scale_y = 1.0f;
                srm.mode = PPA_TRANS_MODE_BLOCKING;

                ppa_do_scale_rotate_mirror(s_ppa_client, &srm);
                fb_sync();
                played++;
            }
            vTaskDelay(pdMS_TO_TICKS(delay_ms > 10 ? delay_ms - 5 : 5));
        }

        free(raw_buf);
        free(jpg_in);
        jpeg_del_decoder_engine(jpeg_dec);
    } else {
        /* Legacy RAW RGB565 playback */
        uint8_t *frame_buf = (uint8_t *)heap_caps_aligned_alloc(64, hdr.frame_size, MALLOC_CAP_DMA | MALLOC_CAP_SPIRAM);
        if (frame_buf) {
            float sx = 1280.0f / (float)hdr.width;
            float sy = 720.0f / (float)hdr.height;

            vTaskDelay(pdMS_TO_TICKS(50));
            while (host_read_key() > 0) {}

            for (uint32_t i = 0; i < hdr.frame_count; i++) {
                int32_t k = host_read_key();
                if (k == 27 || k == 'q' || k == 'Q' || k == ' ') break;

                int32_t tx, ty, tp;
                if (host_get_touch(&tx, &ty, &tp) == 0 && tp) break;

                size_t rd = fread(frame_buf, 1, hdr.frame_size, f);
                if (rd != hdr.frame_size) break;

                ppa_srm_oper_config_t srm;
                memset(&srm, 0, sizeof(srm));
                srm.in.buffer = frame_buf;
                srm.in.pic_w = hdr.width;
                srm.in.pic_h = hdr.height;
                srm.in.block_w = hdr.width;
                srm.in.block_h = hdr.height;
                srm.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

                srm.out.buffer = s_fb;
                srm.out.buffer_size = 720 * 1280 * 2;
                srm.out.pic_w = 720;
                srm.out.pic_h = 1280;
                srm.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;

                srm.rotation_angle = PPA_SRM_ROTATION_ANGLE_90;
                srm.scale_x = sx;
                srm.scale_y = sy;
                srm.mode = PPA_TRANS_MODE_BLOCKING;

                ppa_do_scale_rotate_mirror(s_ppa_client, &srm);
                fb_sync();
                played++;
                vTaskDelay(pdMS_TO_TICKS(delay_ms));
            }
            free(frame_buf);
        }
    }

    fclose(f);
    if (!s_graphics_exclusive) {
        init_terminal_screen();
    }
    return played;
}

/* =========================================================================
 * HTTP Web Server: Live MJPEG Webcam Stream, HTML5 Mission Control & VFS
 * ========================================================================= */

#define MJPEG_PART_BOUNDARY "123456789000000000000987654321"
static const char* _STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" MJPEG_PART_BOUNDARY;
static const char* _STREAM_BOUNDARY = "\r\n--" MJPEG_PART_BOUNDARY "\r\n";
static const char* _STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\n\r\n";

static httpd_handle_t s_webserver = NULL;
static uint16_t s_webserver_port = 80;

static esp_err_t http_capture_handler(httpd_req_t *req)
{
    ensure_cam_mutex();
    if (xSemaphoreTake(s_cam_web_mutex, pdMS_TO_TICKS(1500)) != pdTRUE) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (host_camera_init() != 0) {
        xSemaphoreGive(s_cam_web_mutex);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    struct v4l2_buffer buf;
    memset(&buf, 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if (ioctl(s_host_cam.fd, VIDIOC_DQBUF, &buf) != 0) {
        xSemaphoreGive(s_cam_web_mutex);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    uint8_t *frame_ptr = s_host_cam.buffer[buf.index];
    size_t frame_bytes = s_host_cam.width * s_host_cam.height * 2;

    jpeg_encoder_handle_t jpeg_enc = NULL;
    jpeg_encode_engine_cfg_t eng_cfg = {
        .intr_priority = 0,
        .timeout_ms = 100,
    };
    esp_err_t jerr = jpeg_new_encoder_engine(&eng_cfg, &jpeg_enc);
    if (jerr != ESP_OK) {
        ioctl(s_host_cam.fd, VIDIOC_QBUF, &buf);
        xSemaphoreGive(s_cam_web_mutex);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    jpeg_encode_cfg_t enc_cfg = {
        .height = (uint32_t)s_host_cam.height,
        .width = (uint32_t)s_host_cam.width,
        .src_type = JPEG_ENCODE_IN_FORMAT_RGB565,
        .sub_sample = JPEG_DOWN_SAMPLING_YUV422,
        .image_quality = 85,
    };
    jpeg_encode_memory_alloc_cfg_t rx_mem_cfg = {
        .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER,
    };
    size_t alloc_size = 0;
    uint32_t max_out = 512 * 1024;
    uint8_t *jpg_out = (uint8_t *)jpeg_alloc_encoder_mem(max_out, &rx_mem_cfg, &alloc_size);
    if (!jpg_out) {
        jpeg_del_encoder_engine(jpeg_enc);
        ioctl(s_host_cam.fd, VIDIOC_QBUF, &buf);
        xSemaphoreGive(s_cam_web_mutex);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    uint32_t actual_out = 0;
    jerr = jpeg_encoder_process(jpeg_enc, &enc_cfg, frame_ptr, frame_bytes, jpg_out, alloc_size, &actual_out);
    ioctl(s_host_cam.fd, VIDIOC_QBUF, &buf);
    xSemaphoreGive(s_cam_web_mutex);

    if (jerr == ESP_OK && actual_out > 0) {
        httpd_resp_set_type(req, "image/jpeg");
        httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=\"tab5_snap.jpg\"");
        httpd_resp_send(req, (const char *)jpg_out, actual_out);
    } else {
        httpd_resp_send_500(req);
    }

    free(jpg_out);
    jpeg_del_encoder_engine(jpeg_enc);
    return ESP_OK;
}

static esp_err_t http_stream_handler(httpd_req_t *req)
{
    ensure_cam_mutex();
    if (xSemaphoreTake(s_cam_web_mutex, pdMS_TO_TICKS(2000)) != pdTRUE) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    if (host_camera_init() != 0) {
        xSemaphoreGive(s_cam_web_mutex);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    jpeg_encoder_handle_t jpeg_enc = NULL;
    jpeg_encode_engine_cfg_t eng_cfg = {
        .intr_priority = 0,
        .timeout_ms = 100,
    };
    esp_err_t jerr = jpeg_new_encoder_engine(&eng_cfg, &jpeg_enc);
    if (jerr != ESP_OK) {
        xSemaphoreGive(s_cam_web_mutex);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    jpeg_encode_cfg_t enc_cfg = {
        .height = (uint32_t)s_host_cam.height,
        .width = (uint32_t)s_host_cam.width,
        .src_type = JPEG_ENCODE_IN_FORMAT_RGB565,
        .sub_sample = JPEG_DOWN_SAMPLING_YUV422,
        .image_quality = 70,
    };
    jpeg_encode_memory_alloc_cfg_t rx_mem_cfg = {
        .buffer_direction = JPEG_ENC_ALLOC_OUTPUT_BUFFER,
    };
    size_t alloc_size = 0;
    uint32_t max_out = 384 * 1024;
    uint8_t *jpg_out = (uint8_t *)jpeg_alloc_encoder_mem(max_out, &rx_mem_cfg, &alloc_size);
    if (!jpg_out) {
        jpeg_del_encoder_engine(jpeg_enc);
        xSemaphoreGive(s_cam_web_mutex);
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);

    char part_buf[128];
    esp_err_t res = ESP_OK;

    while (res == ESP_OK) {
        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;

        if (ioctl(s_host_cam.fd, VIDIOC_DQBUF, &buf) != 0) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        uint8_t *frame_ptr = s_host_cam.buffer[buf.index];
        size_t frame_bytes = s_host_cam.width * s_host_cam.height * 2;
        uint32_t actual_out = 0;

        jerr = jpeg_encoder_process(jpeg_enc, &enc_cfg, frame_ptr, frame_bytes, jpg_out, alloc_size, &actual_out);
        ioctl(s_host_cam.fd, VIDIOC_QBUF, &buf);

        if (jerr == ESP_OK && actual_out > 0) {
            res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
            if (res == ESP_OK) {
                snprintf(part_buf, sizeof(part_buf), _STREAM_PART, (unsigned int)actual_out);
                res = httpd_resp_send_chunk(req, part_buf, strlen(part_buf));
            }
            if (res == ESP_OK) {
                res = httpd_resp_send_chunk(req, (const char *)jpg_out, actual_out);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(35)); // ~25 FPS rate limit
    }

    free(jpg_out);
    jpeg_del_encoder_engine(jpeg_enc);
    xSemaphoreGive(s_cam_web_mutex);
    return res;
}

static esp_err_t http_download_handler(httpd_req_t *req)
{
    char query[256];
    char filepath[256] = "";
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "file", filepath, sizeof(filepath));
    }
    if (strlen(filepath) == 0 || strstr(filepath, "..") != NULL ||
        (strncmp(filepath, "/sd", 3) != 0 && strncmp(filepath, "/usb", 4) != 0)) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    FILE *f = fopen(filepath, "rb");
    if (!f) {
        httpd_resp_send_404(req);
        return ESP_FAIL;
    }

    const char *basename = strrchr(filepath, '/');
    if (basename) basename++; else basename = filepath;

    char disp[160];
    snprintf(disp, sizeof(disp), "attachment; filename=\"%s\"", basename);
    httpd_resp_set_hdr(req, "Content-Disposition", disp);
    httpd_resp_set_type(req, "application/octet-stream");

    char chunk[4096];
    size_t nread = 0;
    while ((nread = fread(chunk, 1, sizeof(chunk), f)) > 0) {
        if (httpd_resp_send_chunk(req, chunk, nread) != ESP_OK) {
            fclose(f);
            return ESP_FAIL;
        }
    }
    fclose(f);
    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

static esp_err_t http_delete_handler(httpd_req_t *req)
{
    char query[256];
    char filepath[256] = "";
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        httpd_query_key_value(query, "file", filepath, sizeof(filepath));
    }
    if (strlen(filepath) > 0 && strstr(filepath, "..") == NULL &&
        (strncmp(filepath, "/sd", 3) == 0 || strncmp(filepath, "/usb", 4) == 0)) {
        unlink(filepath);
    }
    httpd_resp_set_status(req, "302 Found");
    httpd_resp_set_hdr(req, "Location", "/");
    httpd_resp_sendstr(req, "Redirecting to Dashboard...");
    return ESP_OK;
}

static esp_err_t http_upload_handler(httpd_req_t *req)
{
    char query[256];
    char filename[128] = "upload.bin";
    if (httpd_req_get_url_query_str(req, query, sizeof(query)) == ESP_OK) {
        char param[128];
        if (httpd_query_key_value(query, "name", param, sizeof(param)) == ESP_OK) {
            strncpy(filename, param, sizeof(filename) - 1);
        }
    }

    // Sanitize filename
    char clean_name[128];
    int cn_idx = 0;
    for (int i = 0; filename[i] && cn_idx < sizeof(clean_name) - 1; ++i) {
        char c = filename[i];
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') continue;
        clean_name[cn_idx++] = c;
    }
    clean_name[cn_idx] = '\0';
    if (strlen(clean_name) == 0) strcpy(clean_name, "upload.bin");

    char filepath[256];
    snprintf(filepath, sizeof(filepath), "/sd/%s", clean_name);

    FILE *f = fopen(filepath, "wb");
    if (!f) {
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    char buf[2048];
    int remaining = req->content_len;
    while (remaining > 0) {
        int to_read = (remaining < sizeof(buf)) ? remaining : sizeof(buf);
        int received = httpd_req_recv(req, buf, to_read);
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) continue;
            fclose(f);
            unlink(filepath);
            httpd_resp_send_500(req);
            return ESP_FAIL;
        }
        fwrite(buf, 1, received, f);
        remaining -= received;
    }
    fclose(f);

    httpd_resp_set_type(req, "text/plain");
    httpd_resp_sendstr(req, "Upload OK");
    return ESP_OK;
}

static esp_err_t http_index_handler(httpd_req_t *req)
{
    httpd_resp_set_type(req, "text/html; charset=utf-8");

    /* 1. Header & CSS Styles */
    const char *html_header =
        "<!DOCTYPE html><html lang='pt-BR'><head><meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<title>MaiaOS - Tab5 Mission Control</title>"
        "<style>"
        ":root { --bg:#0b0f19; --card:#161f30; --border:#25334d; --accent:#00f0ff; --green:#39ff14; --text:#e2e8f0; --sub:#94a3b8; }"
        "* { box-sizing: border-box; margin:0; padding:0; }"
        "body { font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, monospace; background: var(--bg); color: var(--text); padding: 16px; }"
        ".header { display: flex; align-items: center; justify-content: space-between; padding-bottom: 16px; border-bottom: 1px solid var(--border); margin-bottom: 20px; }"
        ".logo { font-size: 20px; font-weight: bold; color: var(--accent); letter-spacing: 1px; }"
        ".badge-online { background: rgba(57,255,20,0.15); color: var(--green); border: 1px solid var(--green); padding: 4px 10px; border-radius: 12px; font-size: 12px; font-weight: bold; }"
        ".grid { display: grid; grid-template-columns: 1fr; gap: 20px; }"
        "@media (min-width: 900px) { .grid { grid-template-columns: 3fr 2fr; } }"
        ".card { background: var(--card); border: 1px solid var(--border); border-radius: 12px; padding: 18px; box-shadow: 0 4px 20px rgba(0,0,0,0.4); margin-bottom: 20px; }"
        ".card-title { font-size: 16px; font-weight: 600; color: var(--accent); margin-bottom: 14px; display: flex; align-items: center; gap: 8px; }"
        ".video-wrap { text-align: center; position: relative; background: #000; border-radius: 8px; overflow: hidden; min-height: 240px; display: flex; align-items: center; justify-content: center; }"
        ".video-feed { width: 100%; height: auto; max-height: 480px; object-fit: contain; }"
        ".btn-row { display: flex; gap: 10px; margin-top: 12px; flex-wrap: wrap; }"
        ".btn { background: #1e293b; color: var(--text); border: 1px solid var(--border); padding: 8px 14px; border-radius: 6px; cursor: pointer; text-decoration: none; font-size: 13px; font-weight: 500; transition: all 0.2s; display: inline-flex; align-items: center; gap: 6px; }"
        ".btn:hover { background: var(--accent); color: #000; }"
        ".btn-accent { background: var(--accent); color: #000; font-weight: bold; }"
        ".btn-danger { background: #ef4444; color: #fff; border-color: #dc2626; }"
        ".btn-danger:hover { background: #b91c1c; color:#fff; }"
        ".stats-table { width: 100%; border-collapse: collapse; font-size: 13px; margin-top: 8px; }"
        ".stats-table td { padding: 8px 6px; border-bottom: 1px solid var(--border); }"
        ".stats-table td:first-child { color: var(--sub); font-weight: 500; width: 40%; }"
        ".stats-table td:last-child { color: var(--text); font-family: monospace; font-weight: 600; }"
        ".file-table { width: 100%; border-collapse: collapse; font-size: 12px; margin-top: 10px; }"
        ".file-table th { text-align: left; padding: 8px 6px; color: var(--sub); border-bottom: 1px solid var(--border); }"
        ".file-table td { padding: 8px 6px; border-bottom: 1px solid rgba(255,255,255,0.05); }"
        ".tag { padding: 2px 6px; border-radius: 4px; font-size: 10px; font-weight: bold; }"
        ".tag-img { background: #8b5cf6; color: #fff; }"
        ".tag-vid { background: #ec4899; color: #fff; }"
        ".tag-snd { background: #06b6d4; color: #000; }"
        ".tag-app { background: #10b981; color: #000; }"
        ".tag-doc { background: #64748b; color: #fff; }"
        ".upload-box { margin-top: 16px; padding: 14px; border: 2px dashed var(--border); border-radius: 8px; text-align: center; background: rgba(0,0,0,0.2); }"
        ".progress-bar { width: 100%; height: 8px; background: var(--border); border-radius: 4px; overflow: hidden; margin-top: 10px; display: none; }"
        ".progress-fill { height: 100%; width: 0%; background: var(--accent); transition: width 0.2s; }"
        "</style></head><body>";
    httpd_resp_send_chunk(req, html_header, strlen(html_header));

    /* 2. Top Header */
    char top_header[512];
    snprintf(top_header, sizeof(top_header),
             "<div class='header'><div class='logo'>&#9889; MAIA OS &bull; TAB5 CONTROL</div>"
             "<div class='badge-online'>&bull; ONLINE &nbsp;[%s]</div></div><div class='grid'><div>",
             s_wifi_ip);
    httpd_resp_send_chunk(req, top_header, strlen(top_header));

    /* 3. Live Webcam Stream Card */
    const char *cam_card =
        "<div class='card'><div class='card-title'>&#128249; LIVE CAMERA FEED &bull; MIPI CSI (SC202CS 720p)</div>"
        "<div class='video-wrap'><img id='camStream' class='video-feed' src='/stream' alt='Loading Stream...'></div>"
        "<div class='btn-row'>"
        "<a href='/capture' target='_blank' class='btn btn-accent'>&#128248; Take Photo (JPEG)</a>"
        "<button onclick=\"document.getElementById('camStream').src='/stream?t='+Date.now();\" class='btn'>&#128260; Reload Feed</button>"
        "<button onclick=\"if(document.fullscreenElement){document.exitFullscreen();}else{document.getElementById('camStream').requestFullscreen();}\" class='btn'>&#9974; Fullscreen</button>"
        "</div></div>";
    httpd_resp_send_chunk(req, cam_card, strlen(cam_card));

    /* 4. VFS File Manager Card */
    const char *files_intro =
        "<div class='card'><div class='card-title'>&#128193; VFS FILE EXPLORER (/sd &amp; /usb)</div>"
        "<table class='file-table'><thead><tr><th>Name</th><th>Location</th><th>Size</th><th>Actions</th></tr></thead><tbody>";
    httpd_resp_send_chunk(req, files_intro, strlen(files_intro));

    // Scan /sd
    DIR *d = opendir("/sd");
    if (d) {
        struct dirent *de;
        int count = 0;
        while ((de = readdir(d)) != NULL && count < 25) {
            if (de->d_name[0] == '.') continue;
            char path[256];
            snprintf(path, sizeof(path), "/sd/%s", de->d_name);
            struct stat st;
            uint32_t sz = 0;
            if (stat(path, &st) == 0) sz = (uint32_t)st.st_size;

            const char *tag_cls = "tag-doc";
            const char *tag_txt = "DOC";
            const char *ext = strrchr(de->d_name, '.');
            if (ext) {
                if (!strcasecmp(ext, ".bmp") || !strcasecmp(ext, ".jpg") || !strcasecmp(ext, ".raw")) { tag_cls = "tag-img"; tag_txt = "IMG"; }
                else if (!strcasecmp(ext, ".mvid")) { tag_cls = "tag-vid"; tag_txt = "VID"; }
                else if (!strcasecmp(ext, ".wav")) { tag_cls = "tag-snd"; tag_txt = "SND"; }
                else if (!strcasecmp(ext, ".mobj") || !strcasecmp(ext, ".elf")) { tag_cls = "tag-app"; tag_txt = "APP"; }
            }

            char row[512];
            snprintf(row, sizeof(row),
                     "<tr><td><span class='tag %s'>%s</span> <strong>%s</strong></td><td>/sd</td><td>%lu KB</td>"
                     "<td><a href='/download?file=/sd/%s' class='btn' style='padding:3px 8px;font-size:11px;'>Baixar</a> "
                     "<a href='/delete?file=/sd/%s' onclick=\"return confirm('Excluir %s?')\" class='btn btn-danger' style='padding:3px 8px;font-size:11px;'>Excluir</a></td></tr>",
                     tag_cls, tag_txt, de->d_name, (unsigned long)((sz + 1023) / 1024), de->d_name, de->d_name, de->d_name);
            httpd_resp_send_chunk(req, row, strlen(row));
            count++;
        }
        closedir(d);
    }

    // Scan /usb if mounted
    if (host_is_usb_mounted()) {
        DIR *du = opendir("/usb");
        if (du) {
            struct dirent *de;
            int count = 0;
            while ((de = readdir(du)) != NULL && count < 15) {
                if (de->d_name[0] == '.') continue;
                char path[256];
                snprintf(path, sizeof(path), "/usb/%s", de->d_name);
                struct stat st;
                uint32_t sz = 0;
                if (stat(path, &st) == 0) sz = (uint32_t)st.st_size;

                char row[512];
                snprintf(row, sizeof(row),
                         "<tr><td><span class='tag tag-doc'>USB</span> <strong>%s</strong></td><td>/usb</td><td>%lu KB</td>"
                         "<td><a href='/download?file=/usb/%s' class='btn' style='padding:3px 8px;font-size:11px;'>Baixar</a></td></tr>",
                         de->d_name, (unsigned long)((sz + 1023) / 1024), de->d_name);
                httpd_resp_send_chunk(req, row, strlen(row));
                count++;
            }
            closedir(du);
        }
    }

    const char *files_outro =
        "</tbody></table>"
        "<div class='upload-box'>"
        "<input type='file' id='fileInput' style='display:none' onchange='handleUpload()'>"
        "<button onclick=\"document.getElementById('fileInput').click()\" class='btn btn-accent'>&#128228; Upload File to /sd</button>"
        "<div id='pBar' class='progress-bar'><div id='pFill' class='progress-fill'></div></div>"
        "<div id='upStatus' style='margin-top:6px;font-size:12px;color:var(--sub)'></div>"
        "</div></div></div>";
    httpd_resp_send_chunk(req, files_outro, strlen(files_outro));

    /* 5. Telemetry & Hardware Card (Column 2) */
    uint32_t free_sram = esp_get_free_internal_heap_size() / 1024;
    uint32_t free_psram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / (1024 * 1024);
    uint32_t uptime_sec = (uint32_t)(esp_timer_get_time() / 1000000ULL);
    uint32_t up_h = uptime_sec / 3600;
    uint32_t up_m = (uptime_sec % 3600) / 60;
    uint32_t up_s = uptime_sec % 60;

    char dt[64] = "RTC N/A";
    host_get_datetime(dt, sizeof(dt));

    char telem_card[1024];
    snprintf(telem_card, sizeof(telem_card),
             "<div><div class='card'><div class='card-title'>&#128736; SYSTEM TELEMETRY</div>"
             "<table class='stats-table'>"
             "<tr><td>Platform</td><td>ESP32-P4NRW32 (RV32 400MHz)</td></tr>"
             "<tr><td>Wi-Fi Module</td><td>ESP32-C6 Hosted (SDIO)</td></tr>"
             "<tr><td>IP Address</td><td>%s</td></tr>"
             "<tr><td>Uptime</td><td>%luh %lum %lus</td></tr>"
             "<tr><td>RTC Date / Time</td><td>%s</td></tr>"
             "<tr><td>Free SRAM</td><td>%lu KB</td></tr>"
             "<tr><td>Free PSRAM</td><td>%lu MB / 32 MB</td></tr>"
             "<tr><td>USB Drive</td><td>%s</td></tr>"
             "</table></div>",
             s_wifi_ip, (unsigned long)up_h, (unsigned long)up_m, (unsigned long)up_s,
             dt, (unsigned long)free_sram, (unsigned long)free_psram,
             host_is_usb_mounted() ? "<span style='color:var(--green)'>Mounted (/usb)</span>" : "<span style='color:var(--sub)'>Disconnected</span>");
    httpd_resp_send_chunk(req, telem_card, strlen(telem_card));

    /* 6. Quick Controls & JavaScript Footer */
    const char *html_footer =
        "<div class='card'><div class='card-title'>&#9881; QUICK CONTROLS</div>"
        "<div class='btn-row'>"
        "<button onclick=\"fetch('/download?file=/sd/autorun.sh').then(r=>alert('Script read successfully'));\" class='btn'>&#128196; Read Script</button>"
        "<button onclick=\"location.reload();\" class='btn'>&#128260; Refresh Status</button>"
        "</div></div></div></div>"
        "<script>"
        "function handleUpload(){"
        "  const fi=document.getElementById('fileInput');"
        "  if(!fi.files.length)return;"
        "  const f=fi.files[0];"
        "  const pBar=document.getElementById('pBar');"
        "  const pFill=document.getElementById('pFill');"
        "  const st=document.getElementById('upStatus');"
        "  pBar.style.display='block'; st.innerText='Uploading '+f.name+'...';"
        "  const xhr=new XMLHttpRequest();"
        "  xhr.open('POST','/upload?name='+encodeURIComponent(f.name),true);"
        "  xhr.upload.onprogress=function(e){"
        "    if(e.lengthComputable){const pct=Math.round((e.loaded/e.total)*100);pFill.style.width=pct+'%';st.innerText='Uploading: '+pct+'%';}"
        "  };"
        "  xhr.onload=function(){"
        "    if(xhr.status==200){st.innerText='Upload completed successfully!';setTimeout(()=>location.reload(),1000);}"
        "    else{st.innerText='Upload error ('+xhr.status+')';}"
        "  };"
        "  xhr.onerror=function(){st.innerText='Connection failed';};"
        "  xhr.send(f);"
        "}"
        "</script></body></html>";
    httpd_resp_send_chunk(req, html_footer, strlen(html_footer));

    httpd_resp_send_chunk(req, NULL, 0);
    return ESP_OK;
}

extern "C" int32_t host_webserver_start(uint16_t port)
{
    if (s_webserver) {
        ESP_LOGW(TAG, "Web server already running on port %u", s_webserver_port);
        return 0;
    }
    if (port == 0) port = 80;
    s_webserver_port = port;

    ensure_cam_mutex();

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = port;
    config.stack_size = 10240;
    config.max_uri_handlers = 12;
    config.lru_purge_enable = true;

    esp_err_t ret = httpd_start(&s_webserver, &config);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server: %d", ret);
        s_webserver = NULL;
        return -1;
    }

    httpd_uri_t uri_get_root = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = http_index_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_webserver, &uri_get_root);

    httpd_uri_t uri_get_stream = {
        .uri = "/stream",
        .method = HTTP_GET,
        .handler = http_stream_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_webserver, &uri_get_stream);

    httpd_uri_t uri_get_capture = {
        .uri = "/capture",
        .method = HTTP_GET,
        .handler = http_capture_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_webserver, &uri_get_capture);

    httpd_uri_t uri_get_download = {
        .uri = "/download",
        .method = HTTP_GET,
        .handler = http_download_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_webserver, &uri_get_download);

    httpd_uri_t uri_post_upload = {
        .uri = "/upload",
        .method = HTTP_POST,
        .handler = http_upload_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_webserver, &uri_post_upload);

    httpd_uri_t uri_get_delete = {
        .uri = "/delete",
        .method = HTTP_GET,
        .handler = http_delete_handler,
        .user_ctx = NULL,
    };
    httpd_register_uri_handler(s_webserver, &uri_get_delete);

    ESP_LOGI(TAG, "HTTP Web Server started on port %u (http://%s:%u/)", port, s_wifi_ip, port);
    return 0;
}

extern "C" int32_t host_webserver_stop(void)
{
    if (!s_webserver) return 0;
    httpd_stop(s_webserver);
    s_webserver = NULL;
    ESP_LOGI(TAG, "HTTP Web Server stopped");
    return 0;
}

extern "C" int32_t host_webserver_status(char *buf, size_t max_len)
{
    if (!buf || max_len == 0) return -1;
    if (s_webserver) {
        snprintf(buf, max_len, "Webcam & File Server: Running on http://%s:%u/\n"
                               "  Dashboard: http://%s:%u/\n"
                               "  MJPEG Stream: http://%s:%u/stream\n"
                               "  JPEG Snapshot: http://%s:%u/capture\n"
                               "  VFS File Download/Upload: active\n",
                 s_wifi_ip, s_webserver_port,
                 s_wifi_ip, s_webserver_port,
                 s_wifi_ip, s_webserver_port,
                 s_wifi_ip, s_webserver_port);
    } else {
        snprintf(buf, max_len, "Webcam & File Server: Stopped (Wi-Fi IP: %s)\n", s_wifi_ip);
    }
    return 0;
}

static void init_usb_msc(void)
{
    ESP_LOGI(TAG, "Initializing USB Host & MSC subsystem...");
    s_msc_queue = xQueueCreate(4, sizeof(msc_queue_msg_t));
    if (!s_msc_queue) {
        ESP_LOGE(TAG, "Failed to create MSC queue");
        return;
    }

    esp_err_t err = bsp_usb_host_start(BSP_USB_HOST_POWER_MODE_USB_DEV, true);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "bsp_usb_host_start failed: %d", err);
        return;
    }

    const msc_host_driver_config_t msc_config = {
        .create_backround_task = true,
        .task_priority = 5,
        .stack_size = 4096,
        .core_id = 0,
        .callback = msc_event_callback,
        .callback_arg = NULL,
    };
    err = msc_host_install(&msc_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "msc_host_install failed: %d", err);
        return;
    }

    xTaskCreatePinnedToCore(usb_msc_task, "usb_msc", 4096, NULL, 5, NULL, 0);
    ESP_LOGI(TAG, "USB MSC Driver active. Ready for USB storage.");
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "Starting Tab5 Display (Rotation 3) + Ext.Port1 Keyboard + MaiaShell...");

    /* 1. Init SysI2C & IO Expander */
    ESP_ERROR_CHECK(bsp_i2c_init());
    bsp_io_expander_pi4ioe_init(bsp_i2c_get_handle());
    bsp_reset_tp();

    /* 2. Enable Ext 5V and USB 5V rails for Ext.Port1 Keyboard */
    ESP_LOGI(TAG, "Enabling EXT_5V and USB_5V power rails...");
    bsp_set_ext_5v_en(true);
    bsp_set_usb_5v_en(true);
    vTaskDelay(pdMS_TO_TICKS(300)); // Allow keyboard STM32 to boot up

    /* 2b. Start USB Host MSC Driver */
    init_usb_msc();

    /* 2c. Init Audio Subsystem (ES8388 Speaker + ES7210 Mic) */
    host_audio_init();

    /* 3. Init ST7121 Display */
    ESP_ERROR_CHECK(bsp_display_new_with_handles_to_st7123(NULL, &s_lcd_handles));
    bsp_display_backlight_on();
    ESP_ERROR_CHECK(esp_lcd_dpi_panel_get_frame_buffer(s_lcd_handles.panel, 1, (void **)&s_fb));
    esp_lcd_panel_disp_on_off(s_lcd_handles.panel, true);

    /* 4. Init MaiaShell */
    s_shell.init(shell_output_cb);

    static const maia_display_api_t s_disp_api = {
        .width = LOG_W,
        .height = LOG_H,
        .put_pixel = [](int32_t x, int32_t y, uint16_t color) -> int32_t {
            put_pixel_rot3(x, y, color);
            return 0;
        },
        .fill_rect = [](int32_t x, int32_t y, int32_t w, int32_t h, uint16_t color) -> int32_t {
            fill_rect(x, y, w, h, color);
            return 0;
        },
        .sync = []() {
            fb_sync();
        },
        .acquire_exclusive = []() -> int32_t {
            s_graphics_exclusive = true;
            return 0;
        },
        .release_exclusive = []() -> int32_t {
            s_graphics_exclusive = false;
            init_terminal_screen();
            return 0;
        },
        .get_framebuffer = []() -> uint16_t * {
            return s_fb;
        }
    };
    MaiaShell::set_display_driver(&s_disp_api);

    /* 4b. Init Touch & Input API */
    init_touch();

    static const maia_input_api_t s_input_api = {
        .get_touch = [](int32_t *x, int32_t *y, int32_t *pressed) -> int32_t {
            return host_get_touch(x, y, pressed);
        },
        .read_key = []() -> int32_t {
            return host_read_key();
        }
    };
    MaiaShell::set_input_driver(&s_input_api);

    /* 4c. Register MaiaScript Native Host GUI Symbols */
    mnm_register_symbol("draw_text", (uintptr_t)draw_text_px);
    mnm_register_symbol("draw_rect", (uintptr_t)draw_rect_api);
    mnm_register_symbol("draw_line", (uintptr_t)draw_line_api);
    mnm_register_symbol("fill_rect", (uintptr_t)fill_rect);
    mnm_register_symbol("draw_window", (uintptr_t)draw_window_frame_api);
    mnm_register_symbol("draw_button", (uintptr_t)draw_button_api);
    mnm_register_symbol("display_sync", (uintptr_t)fb_sync);
    mnm_register_symbol("delay_ms", (uintptr_t)+[](int32_t ms) {
        if (ms > 0) vTaskDelay(pdMS_TO_TICKS(ms));
    });
    mnm_register_symbol("play_beep", (uintptr_t)+[](int32_t freq, int32_t ms) {
        host_play_beep(freq, ms);
    });
    mnm_register_symbol("println", (uintptr_t)+[](const char* s) {
        if (s) {
            printf("%s\n", s);
        }
    });
    mnm_register_symbol("print_int", (uintptr_t)+[](int32_t v) {
        printf("%d\n", (int)v);
    });
    mnm_register_symbol("print", (uintptr_t)+[](const char* s) {
        if (s) printf("%s", s);
    });
    mnm_register_symbol("millis", (uintptr_t)+[]() -> int32_t {
        return (int32_t)(esp_timer_get_time() / 1000);
    });
    mnm_register_symbol("cycles", (uintptr_t)+[]() -> int32_t {
        uint32_t cyc;
        asm volatile("rdcycle %0" : "=r"(cyc));
        return (int32_t)cyc;
    });
    mnm_register_symbol("touch_poll", (uintptr_t)api_touch_poll);
    mnm_register_symbol("touch_x", (uintptr_t)api_touch_x);
    mnm_register_symbol("touch_y", (uintptr_t)api_touch_y);

    static int32_t s_app_exit_code = 0;
    static int32_t s_app_running = 1;
    mnm_register_symbol("host_app_quit", (uintptr_t)+[](int32_t code) {
        s_app_running = 0;
        s_app_exit_code = code;
    });
    mnm_register_symbol("host_app_run", (uintptr_t)+[]() -> int32_t {
        s_app_running = 1;
        while (s_app_running) {
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        return s_app_exit_code;
    });

    mnm_register_symbol("host_fs_exists", (uintptr_t)+[](const char* path) -> int32_t {
        if (!path) return 0;
        struct stat st;
        return (stat(path, &st) == 0) ? 1 : 0;
    });
    mnm_register_symbol("host_fs_read_text", (uintptr_t)+[](const char* path) -> const char* {
        if (!path) return "";
        FILE *f = fopen(path, "rb");
        if (!f) return "";
        fseek(f, 0, SEEK_END);
        long sz = ftell(f);
        fseek(f, 0, SEEK_SET);
        if (sz < 0) { fclose(f); return ""; }
        char *buf = (char*)malloc(sz + 1);
        if (!buf) { fclose(f); return ""; }
        size_t rd = fread(buf, 1, sz, f);
        buf[rd] = '\0';
        fclose(f);
        return buf;
    });
    mnm_register_symbol("host_fs_write_text", (uintptr_t)+[](const char* path, const char* content) -> int32_t {
        if (!path) return -1;
        FILE *f = fopen(path, "wb");
        if (!f) return -1;
        size_t len = content ? strlen(content) : 0;
        if (len > 0) {
            fwrite(content, 1, len, f);
        }
        fclose(f);
        return (int32_t)len;
    });
    mnm_register_symbol("host_fs_remove", (uintptr_t)+[](const char* path) -> int32_t {
        if (!path) return -1;
        return remove(path);
    });
    mnm_register_symbol("host_fs_list_count", (uintptr_t)+[](const char* path) -> int32_t {
        if (!path) return 0;
        DIR *d = opendir(path);
        if (!d) return 0;
        int32_t cnt = 0;
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
            cnt++;
        }
        closedir(d);
        return cnt;
    });
    mnm_register_symbol("host_fs_list_item_name", (uintptr_t)+[](const char* path, int32_t idx) -> const char* {
        if (!path || idx < 0) return "";
        DIR *d = opendir(path);
        if (!d) return "";
        int32_t cur = 0;
        struct dirent *de;
        static char s_last_item_name[128];
        s_last_item_name[0] = '\0';
        while ((de = readdir(d)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
            if (cur == idx) {
                strncpy(s_last_item_name, de->d_name, sizeof(s_last_item_name) - 1);
                s_last_item_name[sizeof(s_last_item_name) - 1] = '\0';
                break;
            }
            cur++;
        }
        closedir(d);
        return s_last_item_name;
    });
    mnm_register_symbol("host_fs_list_item_is_dir", (uintptr_t)+[](const char* path, int32_t idx) -> int32_t {
        if (!path || idx < 0) return 0;
        DIR *d = opendir(path);
        if (!d) return 0;
        int32_t cur = 0;
        int32_t is_dir = 0;
        struct dirent *de;
        while ((de = readdir(d)) != NULL) {
            if (strcmp(de->d_name, ".") == 0 || strcmp(de->d_name, "..") == 0) continue;
            if (cur == idx) {
                is_dir = (de->d_type == DT_DIR) ? 1 : 0;
                break;
            }
            cur++;
        }
        closedir(d);
        return is_dir;
    });

    /* 5. Mount SD Card (if present) */
    esp_err_t sd_err = bsp_sdcard_init((char*)"/sd", 25);
    if (sd_err == ESP_OK) {
        ESP_LOGI(TAG, "SD Card mounted successfully at /sd");
        /* Sync embedded desktop.mobj to SD card so runelf /sd/desktop.mobj always has latest version */
        FILE *f_sync = fopen("/sd/desktop.mobj", "wb");
        if (f_sync) {
            fwrite(desktop_mobj, 1, desktop_mobj_len, f_sync);
            fclose(f_sync);
            ESP_LOGI(TAG, "Synchronized latest desktop.mobj to /sd (%u bytes)", (unsigned int)desktop_mobj_len);
        }

        /* Ensure /sd/apps, /sd/DCIM, and /sd/lib directories exist and sync all modular apps */
        mkdir("/sd/apps", 0777);
        mkdir("/sd/DCIM", 0777);
        mkdir("/sd/lib", 0777);

        struct AppSyncItem {
            const char *path;
            const unsigned char *data;
            unsigned int len;
        };

        const AppSyncItem apps_to_sync[] = {
            { "/sd/apps/banner.mobj",  banner_mobj,  banner_mobj_len },
            { "/sd/apps/calc.mobj",    calc_mobj,    calc_mobj_len },
            { "/sd/apps/clock.mobj",   clock_mobj,   clock_mobj_len },
            { "/sd/apps/sysinfo.mobj", sysinfo_mobj, sysinfo_mobj_len },
            { "/sd/apps/camera.mobj",  camera_mobj,  camera_mobj_len },
            { "/sd/apps/wifi.mobj",    wifi_mobj,    wifi_mobj_len },
            { "/sd/apps/editor.mobj",  editor_mobj,  editor_mobj_len },
            { "/sd/apps/files.mobj",   files_mobj,   files_mobj_len },
            { "/sd/apps/term.mobj",    term_mobj,    term_mobj_len },
            { "/sd/t_gui.mobj",        _tmp_t_gui_mobj, _tmp_t_gui_mobj_len },
            { "/sd/t_bench.mobj",      _tmp_t_bench_mobj, _tmp_t_bench_mobj_len },
            { "/sd/t_class.mobj",      _tmp_t_class_mobj, _tmp_t_class_mobj_len },
            { "/sd/t_mod.mobj",        _tmp_t_mod_mobj, _tmp_t_mod_mobj_len },
        };

        for (size_t i = 0; i < sizeof(apps_to_sync)/sizeof(apps_to_sync[0]); ++i) {
            FILE *f_app = fopen(apps_to_sync[i].path, "wb");
            if (f_app) {
                fwrite(apps_to_sync[i].data, 1, apps_to_sync[i].len, f_app);
                fclose(f_app);
                ESP_LOGI(TAG, "Synchronized %s (%u bytes)", apps_to_sync[i].path, apps_to_sync[i].len);
            }
        }

        struct ManifestSyncItem {
            const char *path;
            const char *json;
        };

        const ManifestSyncItem manifests_to_sync[] = {
            { "/sd/apps/files.mnm.json",   "{\"name\":\"files\",\"title\":\"Files\",\"icon\":\"files\",\"color\":\"0xfe00\",\"hotkey\":\"f\",\"exec\":\"/sd/apps/files.mobj\"}" },
            { "/sd/apps/editor.mnm.json",  "{\"name\":\"editor\",\"title\":\"Editor\",\"icon\":\"editor\",\"color\":\"0x07ff\",\"hotkey\":\"e\",\"exec\":\"/sd/apps/editor.mobj\"}" },
            { "/sd/apps/term.mnm.json",    "{\"name\":\"term\",\"title\":\"Terminal\",\"icon\":\"term\",\"color\":\"0x07e0\",\"hotkey\":\"t\",\"exec\":\"/sd/apps/term.mobj\"}" },
            { "/sd/apps/camera.mnm.json",  "{\"name\":\"camera\",\"title\":\"Camera\",\"icon\":\"camera\",\"color\":\"0xf81f\",\"hotkey\":\"a\",\"exec\":\"/sd/apps/camera.mobj\"}" },
            { "/sd/apps/wifi.mnm.json",    "{\"name\":\"wifi\",\"title\":\"Wi-Fi\",\"icon\":\"wifi\",\"color\":\"0x07ff\",\"hotkey\":\"w\",\"exec\":\"/sd/apps/wifi.mobj\"}" },
            { "/sd/apps/clock.mnm.json",   "{\"name\":\"clock\",\"title\":\"Clock\",\"icon\":\"clock\",\"color\":\"0xffe0\",\"hotkey\":\"r\",\"exec\":\"/sd/apps/clock.mobj\"}" },
            { "/sd/apps/calc.mnm.json",    "{\"name\":\"calc\",\"title\":\"Calc\",\"icon\":\"calc\",\"color\":\"0x07e0\",\"hotkey\":\"c\",\"exec\":\"/sd/apps/calc.mobj\"}" },
            { "/sd/apps/banner.mnm.json",     "{\"name\":\"banner\",\"title\":\"Banner\",\"icon\":\"banner\",\"color\":\"0xf81f\",\"hotkey\":\"b\",\"exec\":\"/sd/apps/banner.mobj\"}" },
            { "/sd/apps/sysinfo.mnm.json",    "{\"name\":\"sysinfo\",\"title\":\"System\",\"icon\":\"sysinfo\",\"color\":\"0x07e0\",\"hotkey\":\"s\",\"exec\":\"/sd/apps/sysinfo.mobj\"}" },
            { "/sd/apps/maiascript.mnm.json", "{\"name\":\"maiascript\",\"title\":\"MaiaScript\",\"icon\":\"maia\",\"color\":\"0x07ff\",\"hotkey\":\"m\",\"exec\":\"/sd/t_gui.mobj\"}" },
            { "/sd/apps/notes.mnm.json",      "{\"name\":\"notes\",\"title\":\"Notes MS\",\"icon\":\"edit\",\"color\":\"0x07e0\",\"hotkey\":\"n\",\"exec\":\"/sd/notes.ms\"}" },
            { "/sd/apps/profiling.mnm.json",  "{\"name\":\"profiling\",\"title\":\"Profile\",\"icon\":\"calc\",\"color\":\"0xf800\",\"hotkey\":\"p\",\"exec\":\"/sd/t_bench.mobj\"}" },
        };

        /* Write source code of t_bench.ms to SD for interactive compilation */
        FILE *f_tbench = fopen("/sd/t_bench.ms", "w");
        if (f_tbench) {
            fputs("extern i32 println(string s);\n"
                  "extern i32 print_int(i32 v);\n"
                  "extern i32 print(string s);\n"
                  "extern i32 cycles();\n\n"
                  "i32 fib(i32 n) {\n"
                  "    if (n <= 1) return n;\n"
                  "    return fib(n - 1) + fib(n - 2);\n"
                  "}\n\n"
                  "i32 sum_loop(i32 limit) {\n"
                  "    i32 s = 0;\n"
                  "    for (i32 i = 0; i < limit; i++) {\n"
                  "        s += i * 3;\n"
                  "    }\n"
                  "    return s;\n"
                  "}\n\n"
                  "i32 bitwise_test(i32 count) {\n"
                  "    i32 val = 4660;\n"
                  "    for (i32 i = 0; i < count; i++) {\n"
                  "        val = (val ^ (i << 2)) & 65535;\n"
                  "    }\n"
                  "    return val;\n"
                  "}\n\n"
                  "i32 main() {\n"
                  "    println(\"=== MAIASCRIPT RISC-V PROFILING BENCHMARK ===\");\n"
                  "    i32 t0 = cycles();\n"
                  "    i32 sum = sum_loop(500);\n"
                  "    i32 t1 = cycles();\n"
                  "    print(\"1. Arithmetic Loop: cycles = \");\n"
                  "    print_int(t1 - t0);\n"
                  "    i32 t2 = cycles();\n"
                  "    i32 f = fib(12);\n"
                  "    i32 t3 = cycles();\n"
                  "    print(\"2. Fibonacci(12) = \");\n"
                  "    print_int(f);\n"
                  "    print(\"   Fib cycles = \");\n"
                  "    print_int(t3 - t2);\n"
                  "    i32 t4 = cycles();\n"
                  "    i32 bw = bitwise_test(500);\n"
                  "    i32 t5 = cycles();\n"
                  "    print(\"3. Bitwise cycles = \");\n"
                  "    print_int(t5 - t4);\n"
                  "    println(\"===============================================\");\n"
                  "    return 42;\n"
                  "}\n", f_tbench);
            fclose(f_tbench);
            ESP_LOGI(TAG, "Synchronized source /sd/t_bench.ms");
        }

        /* Write source code of t_class.ms to SD for interactive compilation */
        FILE *f_tclass = fopen("/sd/t_class.ms", "w");
        if (f_tclass) {
            fputs("extern i32 println(string s);\n"
                  "extern i32 print_int(i32 v);\n"
                  "extern i32 print(string s);\n\n"
                  "class Counter {\n"
                  "    private i32 value = 0;\n\n"
                  "    void increment() {\n"
                  "        value += 1;\n"
                  "    }\n\n"
                  "    i32 getValue() const {\n"
                  "        return value;\n"
                  "    }\n"
                  "}\n\n"
                  "i32 main() {\n"
                  "    println(\"=== MAIASCRIPT SECTION 10: CLASSES & METHODS ===\");\n"
                  "    Counter counter = Counter();\n"
                  "    counter.increment();\n"
                  "    counter.increment();\n"
                  "    i32 val = counter.getValue();\n"
                  "    print(\"Counter value = \");\n"
                  "    print_int(val);\n"
                  "    println(\" (expected 2)\");\n"
                  "    println(\"Class instantiation & static dispatch OK!\");\n"
                  "    return val;\n"
                  "}\n", f_tclass);
            fclose(f_tclass);
            ESP_LOGI(TAG, "Synchronized source /sd/t_class.ms");
        }

        /* Ensure standard library directories exist */
        mkdir("/sd/lib", 0777);
        mkdir("/sd/lib/maia", 0777);
        mkdir("/sd/lib/hardware", 0777);
        mkdir("/sd/lib/maia/hardware", 0777);
        mkdir("/sd/lib/net", 0777);
        mkdir("/sd/lib/maia/net", 0777);

        /* Synchronize all bundled MaiaScript standard library files & applications */
        for (size_t i = 0; i < g_maiascript_bundle_files_count; ++i) {
            char target_path[128];
            if (strncmp(g_maiascript_bundle_files[i].rel_path, "../", 3) == 0) {
                snprintf(target_path, sizeof(target_path), "/sd/%s", g_maiascript_bundle_files[i].rel_path + 3);
            } else {
                snprintf(target_path, sizeof(target_path), "/sd/lib/%s", g_maiascript_bundle_files[i].rel_path);
            }

            FILE *f_out = fopen(target_path, "wb");
            if (f_out) {
                fwrite(g_maiascript_bundle_files[i].data, 1, g_maiascript_bundle_files[i].len, f_out);
                fclose(f_out);
                ESP_LOGI(TAG, "Synchronized MaiaScript bundle: %s (%u bytes)", target_path, (unsigned int)g_maiascript_bundle_files[i].len);
            }
        }

        /* Write source code of t_mod.ms to SD for interactive compilation */
        FILE *f_tmod = fopen("/sd/t_mod.ms", "w");
        if (f_tmod) {
            fputs("import math;\n"
                  "import sys;\n"
                  "import str;\n\n"
                  "i32 main() {\n"
                  "    sys.println(\"=== MAIASCRIPT SECT 11: MODULES & STD LIB ===\");\n"
                  "    i32 p = math.pow(2, 4);\n"
                  "    i32 c = math.clamp(120, 0, 100);\n"
                  "    i32 slen = str.len(\"MaiaScript\");\n"
                  "    sys.print(\"math.pow(2, 4) = \");\n"
                  "    sys.printInt(p);\n"
                  "    sys.print(\"math.clamp(120, 0, 100) = \");\n"
                  "    sys.printInt(c);\n"
                  "    sys.print(\"str.len('MaiaScript') = \");\n"
                  "    sys.printInt(slen);\n"
                  "    sys.println(\"Modules and standard library test PASSED!\");\n"
                  "    return p + c + slen;\n"
                  "}\n", f_tmod);
            fclose(f_tmod);
            ESP_LOGI(TAG, "Synchronized source /sd/t_mod.ms");
        }

        for (size_t i = 0; i < sizeof(manifests_to_sync)/sizeof(manifests_to_sync[0]); ++i) {
            FILE *f_man = fopen(manifests_to_sync[i].path, "w");
            if (f_man) {
                fputs(manifests_to_sync[i].json, f_man);
                fclose(f_man);
                ESP_LOGI(TAG, "Synchronized manifest %s", manifests_to_sync[i].path);
            }
        }

        /* Ensure /sd/desktop.json exists with default font_scale: 2 */
        struct stat st_dj;
        if (stat("/sd/desktop.json", &st_dj) != 0) {
            FILE *f_dj = fopen("/sd/desktop.json", "w");
            if (f_dj) {
                const char dj_content[] = "{\n  \"theme\": \"dark\",\n  \"font_scale\": 2,\n  \"wallpaper\": \"/sd/wallpaper.raw\"\n}\n";
                fputs(dj_content, f_dj);
                fclose(f_dj);
                ESP_LOGI(TAG, "Created default /sd/desktop.json (font_scale: 2)");
            }
        }

        /* Ensure /sd/test.ms and /sd/math.ms exist with sample MaiaScript code */
        {
            FILE *f_ms = fopen("/sd/test.ms", "w");
            if (f_ms) {
                const char ms_sample[] =
                    "// MaiaScript Native Demo (test.ms)\n"
                    "i32 add(i32 a, i32 b) {\n"
                    "    return a + b;\n"
                    "}\n"
                    "\n"
                    "i32 factorial(i32 n) {\n"
                    "    if (n <= 1) {\n"
                    "        return 1;\n"
                    "    }\n"
                    "    return n * factorial(n - 1);\n"
                    "}\n"
                    "\n"
                    "i32 main() {\n"
                    "    i32 a = 10;\n"
                    "    i32 b = 32;\n"
                    "    i32 sum = add(a, b);\n"
                    "    i32 fact = factorial(5);\n"
                    "    return sum + fact; // 42 + 120 = 162\n"
                    "}\n";
                fputs(ms_sample, f_ms);
                fclose(f_ms);
                ESP_LOGI(TAG, "Synchronized sample /sd/test.ms");
            }

            FILE *f_math = fopen("/sd/math.ms", "w");
            if (f_math) {
                const char math_sample[] =
                    "// MaiaScript Math Demo (math.ms)\n"
                    "i32 square(i32 x) {\n"
                    "    return x * x;\n"
                    "}\n"
                    "\n"
                    "i32 main() {\n"
                    "    return square(7); // 49\n"
                    "}\n";
                fputs(math_sample, f_math);
                fclose(f_math);
                ESP_LOGI(TAG, "Synchronized sample /sd/math.ms");
            }

            FILE *f_str = fopen("/sd/t_str.ms", "w");
            if (f_str) {
                const char str_sample[] =
                    "i32 main() {\n"
                    "    string msg = \"Hello, MaiaOS!\";\n"
                    "    return msg[7]; // 'M' = 77\n"
                    "}\n";
                fputs(str_sample, f_str);
                fclose(f_str);
                ESP_LOGI(TAG, "Synchronized sample /sd/t_str.ms");
            }

            FILE *f_struct = fopen("/sd/t_struct.ms", "w");
            if (f_struct) {
                const char struct_sample[] =
                    "struct Point {\n"
                    "    i32 x;\n"
                    "    i32 y;\n"
                    "};\n"
                    "\n"
                    "i32 manhattan(Point* p) {\n"
                    "    return p->x + p->y;\n"
                    "}\n"
                    "\n"
                    "i32 main() {\n"
                    "    Point pt;\n"
                    "    pt.x = 20;\n"
                    "    pt.y = 22;\n"
                    "    return manhattan(&pt); // 42\n"
                    "}\n";
                fputs(struct_sample, f_struct);
                fclose(f_struct);
                ESP_LOGI(TAG, "Synchronized sample /sd/t_struct.ms");
            }

            FILE *f_arr = fopen("/sd/t_array.ms", "w");
            if (f_arr) {
                const char arr_sample[] =
                    "i32 sum(i32* ptr, i32 n) {\n"
                    "    i32 s = 0;\n"
                    "    for (i32 i = 0; i < n; i++) {\n"
                    "        s += ptr[i];\n"
                    "    }\n"
                    "    return s;\n"
                    "}\n"
                    "\n"
                    "i32 main() {\n"
                    "    i32[5] arr;\n"
                    "    for (i32 i = 0; i < 5; i++) {\n"
                    "        arr[i] = (i + 1) * 10;\n"
                    "    }\n"
                    "    return sum(arr, 5); // 150\n"
                    "}\n";
                fputs(arr_sample, f_arr);
                fclose(f_arr);
                ESP_LOGI(TAG, "Synchronized sample /sd/t_array.ms");
            }

            FILE *f_flt = fopen("/sd/t_float.ms", "w");
            if (f_flt) {
                const char flt_sample[] =
                    "f32 compute(f32 a, f32 b) {\n"
                    "    f32 c = a * b + 2.5;\n"
                    "    return c;\n"
                    "}\n"
                    "\n"
                    "i32 main() {\n"
                    "    f32 res = compute(4.0, 5.0);\n"
                    "    i32 truncated = res;\n"
                    "    return truncated + 20; // 42\n"
                    "}\n";
                fputs(flt_sample, f_flt);
                fclose(f_flt);
                ESP_LOGI(TAG, "Synchronized sample /sd/t_float.ms");
            }

            FILE *f_ext = fopen("/sd/t_extern.ms", "w");
            if (f_ext) {
                const char ext_sample[] =
                    "extern void println(string s);\n"
                    "extern void print_int(i32 n);\n"
                    "extern void delay_ms(i32 ms);\n"
                    "extern i32 millis();\n"
                    "\n"
                    "i32 main() {\n"
                    "    println(\"--- MaiaScript Host API Call Test ---\");\n"
                    "    i32 t0 = millis();\n"
                    "    delay_ms(50);\n"
                    "    i32 t1 = millis();\n"
                    "    i32 diff = t1 - t0;\n"
                    "    println(\"Elapsed ms:\");\n"
                    "    print_int(diff);\n"
                    "    if (diff >= 45) {\n"
                    "        return 42;\n"
                    "    }\n"
                    "    return 1;\n"
                    "}\n";
                fputs(ext_sample, f_ext);
                fclose(f_ext);
                ESP_LOGI(TAG, "Synchronized sample /sd/t_extern.ms");
            }

            FILE *f_gui = fopen("/sd/t_gui.ms", "w");
            if (f_gui) {
                const char gui_sample[] =
                    "extern void println(string s);\n"
                    "extern void fill_rect(i32 x, i32 y, i32 w, i32 h, i32 color);\n"
                    "extern void draw_rect(i32 x, i32 y, i32 w, i32 h, i32 color);\n"
                    "extern void draw_window(i32 x, i32 y, i32 w, i32 h, string title, i32 accent);\n"
                    "extern void draw_button(i32 x, i32 y, i32 w, i32 h, string label, i32 bg, i32 fg, i32 pressed);\n"
                    "extern void draw_text(i32 x, i32 y, string text, i32 fg, i32 bg);\n"
                    "extern void display_sync();\n"
                    "extern void delay_ms(i32 ms);\n"
                    "extern void play_beep(i32 freq, i32 ms);\n"
                    "\n"
                    "i32 main() {\n"
                    "    println(\"--- MaiaScript Native GUI Demo ---\");\n"
                    "    draw_window(140, 100, 1000, 520, \"MaiaScript Native GUI Application\", 0x07FF);\n"
                    "    draw_text(180, 170, \"Hello from MaiaScript Native (RV32IMAF ILP32F)!\", 0xFFFF, 0);\n"
                    "    draw_text(180, 210, \"High-Performance AOT execution on M5Stack Tab5 ESP32-P4\", 0x07E0, 0);\n"
                    "    fill_rect(180, 260, 280, 140, 0x001F);\n"
                    "    draw_text(200, 290, \"Pure RISC-V Code\", 0xFFFF, 0);\n"
                    "    draw_text(200, 330, \"Zero-GC & Fast\", 0x07FF, 0);\n"
                    "    fill_rect(500, 260, 280, 140, 0x10A2);\n"
                    "    draw_text(520, 290, \"Hardware FPU\", 0xFFFF, 0);\n"
                    "    draw_text(520, 330, \"Single-cycle Float\", 0xFFE0, 0);\n"
                    "    fill_rect(820, 260, 280, 140, 0x39E7);\n"
                    "    draw_text(840, 290, \"Host C Dynamic APIs\", 0xFFFF, 0);\n"
                    "    draw_text(840, 330, \"R_RISCV_CALL_PLT\", 0xF81F, 0);\n"
                    "    draw_button(360, 470, 240, 50, \"Test Button\", 0x2169, 0xFFFF, 0);\n"
                    "    draw_button(680, 470, 240, 50, \"Status: OK\", 0x07E0, 0x0000, 0);\n"
                    "    display_sync();\n"
                    "    play_beep(523, 70);\n"
                    "    delay_ms(80);\n"
                    "    play_beep(659, 70);\n"
                    "    delay_ms(80);\n"
                    "    play_beep(784, 70);\n"
                    "    delay_ms(80);\n"
                    "    play_beep(1046, 140);\n"
                    "    println(\"GUI Window, text, cards and chime rendered successfully!\");\n"
                    "    return 42;\n"
                    "}\n";
                fputs(gui_sample, f_gui);
                fclose(f_gui);
                ESP_LOGI(TAG, "Synchronized sample /sd/t_gui.ms");
            }
        }

        s_shell.set_cwd("/sd");
    } else {
        ESP_LOGW(TAG, "SD Card not present or mount failed: %d", sd_err);
    }

    /* 6. Render terminal screen (Landscape 1280x720) */
    init_terminal_screen();
    ESP_LOGI(TAG, "Display terminal ready in Rotation 3 (1280x720) at %p", s_fb);

    /* 6b. Play MaiaOS harmonic startup chime */
    host_play_chime();

    /* 7. Launch Background Keyboard & Serial Polling Task */
    BaseType_t kb_res = xTaskCreatePinnedToCore(kb_poll_task, "kb_task", 8192, NULL, 5, NULL, 1);
    if (kb_res != pdPASS) {
        ESP_LOGW(TAG, "kb_task with 8192 bytes stack failed (%d), trying 4096...", (int)kb_res);
        kb_res = xTaskCreatePinnedToCore(kb_poll_task, "kb_task", 4096, NULL, 5, NULL, 1);
    }
    ESP_LOGI(TAG, "kb_task launched: status=%s (code %d)", (kb_res == pdPASS) ? "SUCCESS" : "FAILED", (int)kb_res);
    vTaskDelay(pdMS_TO_TICKS(150));

    /* 8. Auto-execute startup script (/sd/.shrc or /sd/init.sh) if present */
    if (sd_err == ESP_OK) {
        struct stat st_shrc;
        if (stat("/sd/.shrc", &st_shrc) == 0) {
            ESP_LOGI(TAG, "Executing startup script: /sd/.shrc");
            s_shell.execute_script("/sd/.shrc");
        } else if (stat("/sd/init.sh", &st_shrc) == 0) {
            ESP_LOGI(TAG, "Executing startup script: /sd/init.sh");
            s_shell.execute_script("/sd/init.sh");
        }
    }

    /* 9. Shell Command Execution Worker Loop in Core 0 */
    while (1) {
        if (s_has_pending_cmd) {
            char cmd_to_run[256];
            strncpy(cmd_to_run, s_pending_cmd, sizeof(cmd_to_run) - 1);
            cmd_to_run[sizeof(cmd_to_run) - 1] = '\0';
            s_has_pending_cmd = false;

            if (strcmp(cmd_to_run, "desktop") == 0 || strcmp(cmd_to_run, "startx") == 0) {
                s_shell.execute("runelf /sd/desktop.mobj");
                s_graphics_exclusive = false;
                init_terminal_screen();
            } else {
                s_shell.execute(cmd_to_run);
                if (s_graphics_exclusive) {
                    s_graphics_exclusive = false;
                    init_terminal_screen();
                }
            }
            print_prompt();
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
