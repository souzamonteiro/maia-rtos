#include "gui_kit.h"
#include <string.h>

/* Window Identifiers */
enum {
    WIN_NONE = 0,
    WIN_TERM = 1
};

/* Terminal State (Embedded inside desktop.mobj) */
#define TERM_MAX_LINES 16
#define TERM_LINE_LEN  72
static char g_term_lines[TERM_MAX_LINES][TERM_LINE_LEN];
static int g_term_line_count = 0;
static char g_term_input[64] = "";
static int g_term_input_len = 0;
static int g_term_cursor_pos = 0;
static bool g_term_initialized = false;

/* Terminal Command History */
#define TERM_HIST_MAX 16
static char g_term_history[TERM_HIST_MAX][64];
static int  g_term_hist_count = 0;
static int  g_term_hist_browse = -1;

/* Desktop State */
static int g_active_win = WIN_NONE;
static bool g_running = true;

/* Redraw Flags */
static bool g_dirty_canvas       = true;
static bool g_dirty_taskbar      = true;
static bool g_dirty_term_chrome  = true;
static bool g_dirty_term_content = true;
static uint32_t g_prev_sec       = 0xFFFFFFFF;

/* App Registration from Manifests (.mnm.json) */
#define MAX_DESKTOP_APPS 16

typedef struct {
    char name[24];
    char title[24];
    char icon[16];
    uint16_t color;
    char hotkey;
    char exec_path[80];
    bool is_builtin_term;
} desktop_app_t;

static desktop_app_t g_apps[MAX_DESKTOP_APPS];
static int g_app_count = 0;

/* String helpers */
static inline int str_eq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) { if (*a != *b) return 0; a++; b++; }
    return (*a == *b);
}

static inline void str_copy(char *dst, const char *src, size_t max) {
    if (!dst || max == 0) return;
    size_t i = 0;
    if (src) {
        while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    }
    dst[i] = '\0';
}

static void term_hist_add(const char *cmd) {
    if (!cmd || cmd[0] == '\0') return;
    if (g_term_hist_count > 0 && strcmp(g_term_history[g_term_hist_count - 1], cmd) == 0) {
        g_term_hist_browse = -1;
        return;
    }
    if (g_term_hist_count < TERM_HIST_MAX) {
        str_copy(g_term_history[g_term_hist_count++], cmd, sizeof(g_term_history[0]));
    } else {
        for (int i = 0; i < TERM_HIST_MAX - 1; ++i) {
            str_copy(g_term_history[i], g_term_history[i + 1], sizeof(g_term_history[0]));
        }
        str_copy(g_term_history[TERM_HIST_MAX - 1], cmd, sizeof(g_term_history[0]));
    }
    g_term_hist_browse = -1;
}

static void term_hist_up(void) {
    if (g_term_hist_count == 0) return;
    if (g_term_hist_browse == -1) {
        g_term_hist_browse = g_term_hist_count - 1;
    } else if (g_term_hist_browse > 0) {
        g_term_hist_browse--;
    }
    str_copy(g_term_input, g_term_history[g_term_hist_browse], sizeof(g_term_input));
    g_term_input_len = (int)strlen(g_term_input);
    g_term_cursor_pos = g_term_input_len;
    g_dirty_term_content = true;
}

static void term_hist_down(void) {
    if (g_term_hist_browse == -1) return;
    if (g_term_hist_browse + 1 < g_term_hist_count) {
        g_term_hist_browse++;
        str_copy(g_term_input, g_term_history[g_term_hist_browse], sizeof(g_term_input));
        g_term_input_len = (int)strlen(g_term_input);
        g_term_cursor_pos = g_term_input_len;
    } else {
        g_term_hist_browse = -1;
        g_term_input[0] = '\0';
        g_term_input_len = 0;
        g_term_cursor_pos = 0;
    }
    g_dirty_term_content = true;
}

static inline uint16_t parse_hex_color(const char *s) {
    if (!s) return COLOR_CYAN;
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) s += 2;
    uint32_t val = 0;
    while (*s) {
        char c = *s++;
        val <<= 4;
        if (c >= '0' && c <= '9') val |= (c - '0');
        else if (c >= 'a' && c <= 'f') val |= (c - 'a' + 10);
        else if (c >= 'A' && c <= 'F') val |= (c - 'A' + 10);
    }
    return (uint16_t)val;
}

static void json_get_string(const char *json, const char *key, char *out, size_t max_out) {
    out[0] = '\0';
    if (!json || !key) return;
    char pat[32];
    pat[0] = '"';
    size_t ki = 0;
    while (key[ki] && ki < 28) { pat[ki + 1] = key[ki]; ki++; }
    pat[ki + 1] = '"'; pat[ki + 2] = '\0';

    const char *p = json;
    while (*p) {
        bool match = true;
        for (size_t i = 0; pat[i]; ++i) {
            if (p[i] != pat[i]) { match = false; break; }
        }
        if (match) {
            p += ki + 2;
            while (*p == ' ' || *p == ':' || *p == '\t') p++;
            if (*p == '"') {
                p++;
                size_t oi = 0;
                while (*p && *p != '"' && oi < max_out - 1) {
                    out[oi++] = *p++;
                }
                out[oi] = '\0';
                return;
            }
        }
        p++;
    }
}

/* Embedded Terminal Shell (msh) */
static void term_append_line(const char *line) {
    if (g_term_line_count < TERM_MAX_LINES) {
        str_copy(g_term_lines[g_term_line_count], line, TERM_LINE_LEN);
        g_term_line_count++;
    } else {
        for (int i = 0; i < TERM_MAX_LINES - 1; ++i) {
            str_copy(g_term_lines[i], g_term_lines[i + 1], TERM_LINE_LEN);
        }
        str_copy(g_term_lines[TERM_MAX_LINES - 1], line, TERM_LINE_LEN);
    }
}

static void term_init(void) {
    g_term_line_count = 0;
    term_append_line("MaiaRTOS Embedded Terminal Shell v1.2 (msh)");
    term_append_line("ST7121 MIPI-DSI 1280x720 | 32MB Octal PSRAM @ 200MHz");
    term_append_line("Storage: SDMMC 4-bit FAT32 (/sd) mounted");
    term_append_line("Type 'help' for commands, or 'exit' to close.");
    term_append_line("");
    g_term_input[0] = '\0';
    g_term_input_len = 0;
    g_term_cursor_pos = 0;
    g_term_initialized = true;
}

static char g_term_out_line[TERM_LINE_LEN] = {0};
static int  g_term_out_pos = 0;

static void term_out_cb(const char *str, void *user_data) {
    (void)user_data;
    if (!str) return;

    while (*str) {
        char c = *str++;
        if (c == '\r') continue;
        if (c == '\n') {
            g_term_out_line[g_term_out_pos] = '\0';
            term_append_line(g_term_out_line);
            g_term_out_pos = 0;
            g_term_out_line[0] = '\0';
        } else {
            if (g_term_out_pos < TERM_LINE_LEN - 1) {
                g_term_out_line[g_term_out_pos++] = c;
            } else {
                g_term_out_line[g_term_out_pos] = '\0';
                term_append_line(g_term_out_line);
                g_term_out_pos = 0;
                g_term_out_line[g_term_out_pos++] = c;
            }
        }
    }
}

static void term_exec_cmd(const char *cmd) {
    char echo_buf[TERM_LINE_LEN];
    echo_buf[0] = 'm'; echo_buf[1] = 's'; echo_buf[2] = 'h'; echo_buf[3] = '>'; echo_buf[4] = ' ';
    int j = 5;
    for (int i = 0; cmd[i] && j < TERM_LINE_LEN - 1; ++i) {
        echo_buf[j++] = cmd[i];
    }
    echo_buf[j] = '\0';
    term_append_line(echo_buf);

    while (*cmd == ' ') cmd++;
    if (*cmd == '\0') return;

    if (cmd[0] == 'c' && cmd[1] == 'l' && cmd[2] == 'e' && cmd[3] == 'a' && cmd[4] == 'r' && cmd[5] == '\0') {
        g_term_line_count = 0;
        return;
    }
    if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'i' && cmd[3] == 't' && cmd[4] == '\0') {
        g_active_win = WIN_NONE;
        g_dirty_canvas = true;
        return;
    }

    g_term_out_pos = 0;
    g_term_out_line[0] = '\0';

    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->shell_exec) {
        g_gui_ctx->host->shell_exec(cmd, term_out_cb, NULL);
        if (g_term_out_pos > 0) {
            g_term_out_line[g_term_out_pos] = '\0';
            term_append_line(g_term_out_line);
            g_term_out_pos = 0;
            g_term_out_line[0] = '\0';
        }
    } else {
        term_append_line("[ERROR] shell_exec host API unavailable.");
    }
}

static void render_term_chrome(void) {
    int wx = 260, wy = 70, ww = 760, wh = 540;

    gui_set_clip(wx, wy, ww, wh);
    gui_draw_window_frame(wx, wy, ww, wh, "MaiaShell Terminal (msh)", COLOR_GREEN);

    /* Console Screen container */
    int cx = wx + 16, cy = wy + 48, cw = ww - 32, ch = wh - 64;
    gui_draw_rect(cx, cy, cw, ch, COLOR_GREEN);
    gui_reset_clip();
}

static void render_term_content(void) {
    int wx = 260, wy = 70, ww = 760, wh = 540;
    int cx = wx + 16, cy = wy + 48, cw = ww - 32, ch = wh - 64;

    gui_set_clip(cx + 1, cy + 1, cw - 2, ch - 2);
    gui_fill_rect(cx + 1, cy + 1, cw - 2, ch - 2, COLOR_BLACK);

    if (!g_term_initialized) {
        term_init();
    }

    /* Render History Lines in 2x typography */
    int line_h = 28;
    int max_vis = (ch - 40) / line_h;
    int start_l = 0;
    if (g_term_line_count > max_vis) {
        start_l = g_term_line_count - max_vis;
    }

    int ty = cy + 8;
    for (int i = start_l; i < g_term_line_count; ++i) {
        uint16_t col = COLOR_GREEN;
        if (g_term_lines[i][0] == 'm' && g_term_lines[i][1] == 's' && g_term_lines[i][2] == 'h') {
            col = COLOR_WHITE;
        } else if (g_term_lines[i][0] == 'M' || g_term_lines[i][0] == 'S') {
            col = COLOR_CYAN;
        } else if (g_term_lines[i][0] == 'C' && g_term_lines[i][0] == 'o') {
            col = COLOR_YELLOW;
        }
        gui_draw_str_2x(cx + 12, ty, g_term_lines[i], col, COLOR_BLACK, true);
        ty += line_h;
    }

    /* Render Active Input Line in 2x typography */
    char prompt_line[TERM_LINE_LEN];
    prompt_line[0] = 'm'; prompt_line[1] = 's'; prompt_line[2] = 'h'; prompt_line[3] = '>'; prompt_line[4] = ' ';
    int j = 5;
    for (int i = 0; g_term_input[i] && j < TERM_LINE_LEN - 1; ++i) {
        prompt_line[j++] = g_term_input[i];
    }
    prompt_line[j] = '\0';
    gui_draw_str_2x(cx + 12, ty, prompt_line, COLOR_TASK_ACC, COLOR_BLACK, true);

    /* Cursor */
    int cur_x = cx + 12 + (5 + g_term_cursor_pos) * 16;
    if (g_term_cursor_pos < g_term_input_len) {
        char c_cur[2] = { g_term_input[g_term_cursor_pos], '\0' };
        gui_draw_str_2x(cur_x, ty, c_cur, COLOR_BLACK, COLOR_TASK_ACC, true);
    } else {
        gui_fill_rect(cur_x, ty, 14, 24, COLOR_TASK_ACC);
    }
    gui_reset_clip();
}

/* Manifest Scanner: loads modular apps from /sd/apps/*.mnm.json */
static void scan_manifests(void) {
    g_app_count = 0;

    static const char *manifest_paths[] = {
        "/sd/apps/files.mnm.json",
        "/sd/apps/editor.mnm.json",
        "/sd/apps/term.mnm.json",
        "/sd/apps/calc.mnm.json",
        "/sd/apps/clock.mnm.json",
        "/sd/apps/sysinfo.mnm.json",
        "/sd/apps/banner.mnm.json",
        "/sd/apps/camera.mnm.json",
        "/sd/apps/wifi.mnm.json",
        "/sd/apps/maiascript.mnm.json",
    };

    for (size_t i = 0; i < sizeof(manifest_paths)/sizeof(manifest_paths[0]); ++i) {
        if (g_app_count >= MAX_DESKTOP_APPS) break;
        const char *p = manifest_paths[i];

        char json[512] = {0};
        bool read_ok = false;
        if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->read_text_file) {
            int rd = g_gui_ctx->host->read_text_file(p, json, sizeof(json) - 1);
            if (rd > 0) { json[rd] = '\0'; read_ok = true; }
        }

        desktop_app_t *app = &g_apps[g_app_count];
        memset(app, 0, sizeof(desktop_app_t));

        if (read_ok) {
            json_get_string(json, "name", app->name, sizeof(app->name));
            json_get_string(json, "title", app->title, sizeof(app->title));
            json_get_string(json, "icon", app->icon, sizeof(app->icon));
            char col_str[16];
            json_get_string(json, "color", col_str, sizeof(col_str));
            app->color = parse_hex_color(col_str);
            char hk[4];
            json_get_string(json, "hotkey", hk, sizeof(hk));
            app->hotkey = hk[0];
            json_get_string(json, "exec", app->exec_path, sizeof(app->exec_path));
        } else {
            /* Defaults */
            if (i == 0) { str_copy(app->name, "files", 24); str_copy(app->title, "Files", 24); str_copy(app->icon, "files", 16); app->color = COLOR_GOLD; app->hotkey = 'f'; str_copy(app->exec_path, "/sd/apps/files.mobj", 80); }
            else if (i == 1) { str_copy(app->name, "editor", 24); str_copy(app->title, "Editor", 24); str_copy(app->icon, "editor", 16); app->color = COLOR_CYAN; app->hotkey = 'e'; str_copy(app->exec_path, "/sd/apps/editor.mobj", 80); }
            else if (i == 2) { str_copy(app->name, "term", 24); str_copy(app->title, "Terminal", 24); str_copy(app->icon, "term", 16); app->color = COLOR_GREEN; app->hotkey = 't'; str_copy(app->exec_path, "", 80); }
            else if (i == 3) { str_copy(app->name, "calc", 24); str_copy(app->title, "Calc", 24); str_copy(app->icon, "calc", 16); app->color = COLOR_GREEN; app->hotkey = 'c'; str_copy(app->exec_path, "/sd/apps/calc.mobj", 80); }
            else if (i == 4) { str_copy(app->name, "clock", 24); str_copy(app->title, "Clock", 24); str_copy(app->icon, "clock", 16); app->color = COLOR_YELLOW; app->hotkey = 'r'; str_copy(app->exec_path, "/sd/apps/clock.mobj", 80); }
            else if (i == 5) { str_copy(app->name, "sysinfo", 24); str_copy(app->title, "System", 24); str_copy(app->icon, "sysinfo", 16); app->color = COLOR_GREEN; app->hotkey = 's'; str_copy(app->exec_path, "/sd/apps/sysinfo.mobj", 80); }
            else if (i == 6) { str_copy(app->name, "banner", 24); str_copy(app->title, "Banner", 24); str_copy(app->icon, "banner", 16); app->color = COLOR_MAGENTA; app->hotkey = 'b'; str_copy(app->exec_path, "/sd/apps/banner.mobj", 80); }
            else if (i == 7) { str_copy(app->name, "camera", 24); str_copy(app->title, "Camera", 24); str_copy(app->icon, "camera", 16); app->color = COLOR_MAGENTA; app->hotkey = 'a'; str_copy(app->exec_path, "/sd/apps/camera.mobj", 80); }
            else if (i == 8) { str_copy(app->name, "wifi", 24); str_copy(app->title, "Wi-Fi", 24); str_copy(app->icon, "wifi", 16); app->color = COLOR_CYAN; app->hotkey = 'w'; str_copy(app->exec_path, "/sd/apps/wifi.mobj", 80); }
            else if (i == 9) { str_copy(app->name, "maiascript", 24); str_copy(app->title, "MaiaScript", 24); str_copy(app->icon, "maia", 16); app->color = COLOR_CYAN; app->hotkey = 'm'; str_copy(app->exec_path, "/sd/t_gui.mobj", 80); }
        }

        if (str_eq(app->name, "term") || str_eq(app->icon, "term")) {
            app->is_builtin_term = true;
        }

        g_app_count++;
    }
}

/* Icon rendering */
static void draw_desktop_icon(int x, int y, int idx) {
    if (idx < 0 || idx >= g_app_count) return;
    desktop_app_t *app = &g_apps[idx];

    gui_fill_rect(x + 8, y, 52, 52, COLOR_WIN_BG);
    gui_draw_rect(x + 8, y, 52, 52, app->color);

    int gx = x + 18, gy = y + 10;
    if (str_eq(app->icon, "files")) {
        gui_fill_rect(gx, gy, 12, 4, COLOR_GOLD);
        gui_fill_rect(gx, gy + 4, 32, 24, COLOR_FOLDER);
        gui_fill_rect(gx + 3, gy + 8, 26, 17, COLOR_GOLD);
    } else if (str_eq(app->icon, "editor")) {
        gui_fill_rect(gx + 4, gy, 24, 30, COLOR_WHITE);
        gui_draw_rect(gx + 4, gy, 24, 30, COLOR_CYAN);
        gui_fill_rect(gx + 8, gy + 6, 16, 2, COLOR_BLACK);
        gui_fill_rect(gx + 8, gy + 12, 16, 2, COLOR_BLACK);
        gui_fill_rect(gx + 8, gy + 18, 12, 2, COLOR_CYAN);
    } else if (str_eq(app->icon, "term")) {
        gui_fill_rect(gx + 2, gy + 4, 28, 22, COLOR_BLACK);
        gui_draw_rect(gx + 2, gy + 4, 28, 22, COLOR_GREEN);
        gui_draw_str(gx + 6, gy + 7, ">_", COLOR_GREEN, COLOR_BLACK, true);
    } else if (str_eq(app->icon, "calc")) {
        gui_fill_rect(gx + 2, gy + 2, 28, 28, COLOR_TASKBAR);
        gui_draw_rect(gx + 2, gy + 2, 28, 28, COLOR_CYAN);
        gui_fill_rect(gx + 5, gy + 5, 22, 6, COLOR_BLACK);
        gui_fill_rect(gx + 5, gy + 13, 5, 4, COLOR_WHITE);
        gui_fill_rect(gx + 13, gy + 13, 5, 4, COLOR_WHITE);
        gui_fill_rect(gx + 21, gy + 13, 5, 4, COLOR_CYAN);
        gui_fill_rect(gx + 5, gy + 20, 5, 4, COLOR_WHITE);
        gui_fill_rect(gx + 13, gy + 20, 5, 4, COLOR_WHITE);
        gui_fill_rect(gx + 21, gy + 20, 5, 4, COLOR_GREEN);
    } else if (str_eq(app->icon, "clock")) {
        gui_fill_rect(gx + 2, gy + 2, 28, 28, COLOR_WIN_HDR);
        gui_draw_rect(gx + 2, gy + 2, 28, 28, COLOR_YELLOW);
        gui_put_pixel(gx + 16, gy + 16, COLOR_YELLOW);
        gui_fill_rect(gx + 16, gy + 8, 2, 8, COLOR_WHITE);
        gui_fill_rect(gx + 16, gy + 16, 7, 2, COLOR_CYAN);
    } else if (str_eq(app->icon, "sysinfo")) {
        gui_fill_rect(gx + 2, gy + 2, 28, 28, COLOR_BLACK);
        gui_draw_rect(gx + 2, gy + 2, 28, 28, COLOR_GREEN);
        gui_draw_str(gx + 6, gy + 8, "CPU", COLOR_GREEN, COLOR_BLACK, true);
    } else if (str_eq(app->icon, "banner")) {
        gui_fill_rect(gx + 2, gy + 2, 28, 28, COLOR_TASKBAR);
        gui_draw_rect(gx + 2, gy + 2, 28, 28, COLOR_MAGENTA);
        gui_draw_str(gx + 7, gy + 8, "FX", COLOR_MAGENTA, COLOR_TASKBAR, true);
    } else if (str_eq(app->icon, "camera")) {
        gui_fill_rect(gx + 2, gy + 6, 28, 20, COLOR_DARKGRAY);
        gui_draw_rect(gx + 2, gy + 6, 28, 20, COLOR_MAGENTA);
        gui_fill_rect(gx + 8, gy + 2, 10, 4, COLOR_TASKBAR);
        gui_fill_rect(gx + 10, gy + 10, 12, 12, COLOR_BLACK);
        gui_draw_rect(gx + 10, gy + 10, 12, 12, COLOR_CYAN);
    } else if (str_eq(app->icon, "wifi")) {
        gui_fill_rect(gx + 2, gy + 2, 28, 28, COLOR_BLACK);
        gui_draw_rect(gx + 2, gy + 2, 28, 28, COLOR_CYAN);
        gui_draw_str(gx + 6, gy + 8, "WFI", COLOR_CYAN, COLOR_BLACK, true);
    } else if (str_eq(app->icon, "maia") || str_eq(app->icon, "maiascript")) {
        gui_fill_rect(gx + 2, gy + 2, 28, 28, COLOR_BLACK);
        gui_draw_rect(gx + 2, gy + 2, 28, 28, COLOR_CYAN);
        gui_draw_str(gx + 6, gy + 8, ".MS", COLOR_CYAN, COLOR_BLACK, true);
    } else {
        gui_fill_rect(gx + 2, gy + 2, 28, 28, COLOR_TASKBAR);
        gui_draw_rect(gx + 2, gy + 2, 28, 28, app->color);
        gui_draw_str(gx + 10, gy + 8, "APP", app->color, COLOR_TASKBAR, true);
    }

    int len = 0;
    while (app->title[len]) len++;
    int lx = x + 34 - (len * 8) / 2;
    gui_draw_str(lx, y + 56, app->title, COLOR_TEXT, 0x1107, true);
}

/* Background & Wallpaper */
static void render_canvas_background(void) {
    bool wp_ok = false;
    if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->apply_wallpaper) {
        if (g_gui_ctx->host->apply_wallpaper("/sd/wallpaper.raw") == 0 ||
            g_gui_ctx->host->apply_wallpaper(NULL) == 0) {
            wp_ok = true;
        }
    }

    if (!wp_ok) {
        for (int y = 36; y < SCREEN_H; ++y) {
            int r = (y - 36) * 3 / 684;
            int g = 1 + (y - 36) * 4 / 684;
            int b = 5 + (y - 36) * 8 / 684;
            uint16_t line_col = (uint16_t)((r << 11) | (g << 5) | b);
            gui_fill_rect(0, y, SCREEN_W, 1, line_col);
        }
    }

    for (int i = 0; i < g_app_count; ++i) {
        int ix = (i < 8) ? 24 : 116;
        int iy = 44 + (i % 8) * 78;
        draw_desktop_icon(ix, iy, i);
    }
}

/* Top Taskbar */
static void render_taskbar_chrome(void) {
    gui_fill_rect(0, 0, SCREEN_W, 36, COLOR_TASKBAR);
    gui_draw_rect(0, 35, SCREEN_W, 1, COLOR_TASK_ACC);

    gui_draw_str_2x(12, 2, "MaiaOS", COLOR_TASK_ACC, COLOR_TASKBAR, true);
    gui_draw_str(116, 10, "M5Stack Tab5 RV32", COLOR_TEXT_DIM, COLOR_TASKBAR, true);

    gui_draw_button(SCREEN_W - 80, 4, 70, 28, "EXIT", COLOR_BTN_BG, COLOR_RED, COLOR_BTN_BD, false);
}

static void render_taskbar_clock(uint64_t now_us) {
    gui_fill_rect(SCREEN_W - 240, 2, 150, 32, COLOR_TASKBAR);

    uint32_t sec = (uint32_t)(now_us / 1000000ULL);
    uint32_t hrs = (sec / 3600) % 24;
    uint32_t min = (sec / 60) % 60;
    uint32_t s = sec % 60;
    char clk[12];
    clk[0] = '0' + (hrs / 10);
    clk[1] = '0' + (hrs % 10);
    clk[2] = ':';
    clk[3] = '0' + (min / 10);
    clk[4] = '0' + (min % 10);
    clk[5] = ':';
    clk[6] = '0' + (s / 10);
    clk[7] = '0' + (s % 10);
    clk[8] = '\0';
    gui_draw_str_2x(SCREEN_W - 240, 2, clk, COLOR_CYAN, COLOR_TASKBAR, true);
}

/* Launch App */
static void launch_app(int idx) {
    if (idx < 0 || idx >= g_app_count) return;
    desktop_app_t *app = &g_apps[idx];

    if (app->is_builtin_term) {
        if (!g_term_initialized) {
            term_init();
        }
        g_active_win = WIN_TERM;
        g_term_input[0] = '\0';
        g_term_input_len = 0;
        g_term_cursor_pos = 0;
        if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->display && g_gui_ctx->host->display->acquire_exclusive) {
            g_gui_ctx->host->display->acquire_exclusive();
        }
        g_dirty_canvas = true;
        g_dirty_term_chrome = true;
        g_dirty_term_content = true;
        return;
    }

    if (app->exec_path[0] && g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->shell_exec) {
        g_active_win = WIN_NONE;

        /* Clean canvas before launching external app so no ghost windows remain */
        render_canvas_background();
        render_taskbar_chrome();
        gui_sync();

        char cmd[160] = "runelf ";
        str_copy(cmd + 7, app->exec_path, sizeof(cmd) - 7);

        g_gui_ctx->host->shell_exec(cmd, NULL, NULL);

        /* Re-acquire exclusive mode for Desktop */
        if (g_gui_ctx && g_gui_ctx->host && g_gui_ctx->host->display && g_gui_ctx->host->display->acquire_exclusive) {
            g_gui_ctx->host->display->acquire_exclusive();
        }

        /* Flush lingering input events after app exits */
        while (gui_poll_key() >= 0) {}
        int32_t dtx, dty; bool dpr;
        while (gui_poll_touch(&dtx, &dty, &dpr) && dpr) {
            gui_delay_ms(10);
        }
        gui_delay_ms(40);
        while (gui_poll_touch(&dtx, &dty, &dpr) && dpr) {
            gui_delay_ms(10);
        }

        g_dirty_canvas = true;
        g_dirty_taskbar = true;
        g_dirty_term_chrome = true;
        g_dirty_term_content = true;
    }
}

static void handle_touch(int tx, int ty) {
    if (ty >= 0 && ty <= 36) {
        if (tx >= SCREEN_W - 80) {
            g_running = false;
            return;
        }
    }

    if (g_active_win == WIN_TERM) {
        int wx = 260, wy = 70, ww = 760;
        if (gui_is_close_clicked(wx, wy, ww, tx, ty)) {
            g_active_win = WIN_NONE;
            g_dirty_canvas = true;
            return;
        }
        return;
    }

    if (g_active_win == WIN_NONE) {
        for (int i = 0; i < g_app_count; ++i) {
            int ix = (i < 8) ? 24 : 116;
            int iy = 44 + (i % 8) * 78;
            if (tx >= ix && tx <= ix + 68 && ty >= iy && ty <= iy + 74) {
                launch_app(i);
                return;
            }
        }
    }
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    if (ctx->host->display && ctx->host->display->acquire_exclusive) {
        ctx->host->display->acquire_exclusive();
    }

    scan_manifests();

    g_running = true;
    bool was_pressed = false;

    while (g_running) {
        uint64_t now_us = (ctx->host->get_time_us) ? ctx->host->get_time_us() : 0;
        uint32_t sec = (uint32_t)(now_us / 1000000ULL);

        /* Touch */
        int32_t tx = 0, ty = 0;
        bool pressed = false;
        if (gui_poll_touch(&tx, &ty, &pressed)) {
            if (pressed && !was_pressed) {
                handle_touch(tx, ty);
                was_pressed = true;
            } else if (!pressed && was_pressed) {
                was_pressed = false;
            }
        }

        /* Keyboard */
        int32_t k = gui_poll_key();
        while (k >= 0) {
            if (k == 27) { /* ESC: close active window, never exit Desktop */
                if (g_active_win != WIN_NONE) {
                    g_active_win = WIN_NONE;
                    g_dirty_canvas = true;
                }
            } else if (g_active_win == WIN_TERM) {
                if (k == '\r' || k == '\n') {
                    if (g_term_input_len > 0) {
                        term_hist_add(g_term_input);
                    }
                    term_exec_cmd(g_term_input);
                    g_term_input[0] = '\0';
                    g_term_input_len = 0;
                    g_term_cursor_pos = 0;
                    g_dirty_term_content = true;
                } else if (k == 0x80) { /* Up Arrow (History Back) */
                    term_hist_up();
                } else if (k == 0x81) { /* Down Arrow (History Forward) */
                    term_hist_down();
                } else if (k == 0x82) { /* Left Arrow (Cursor Left) */
                    if (g_term_cursor_pos > 0) {
                        g_term_cursor_pos--;
                        g_dirty_term_content = true;
                    }
                } else if (k == 0x83) { /* Right Arrow (Cursor Right) */
                    if (g_term_cursor_pos < g_term_input_len) {
                        g_term_cursor_pos++;
                        g_dirty_term_content = true;
                    }
                } else if (k == '\b' || k == 127) {
                    if (g_term_cursor_pos > 0) {
                        for (int i = g_term_cursor_pos - 1; i < g_term_input_len; ++i) {
                            g_term_input[i] = g_term_input[i + 1];
                        }
                        g_term_cursor_pos--;
                        g_term_input_len--;
                        g_dirty_term_content = true;
                    }
                } else if (k >= 32 && k <= 126) {
                    if (g_term_input_len < (int)sizeof(g_term_input) - 2) {
                        for (int i = g_term_input_len; i >= g_term_cursor_pos; --i) {
                            g_term_input[i + 1] = g_term_input[i];
                        }
                        g_term_input[g_term_cursor_pos] = (char)k;
                        g_term_cursor_pos++;
                        g_term_input_len++;
                        g_dirty_term_content = true;
                    }
                }
            } else if (g_active_win == WIN_NONE) {
                char lk = (char)k;
                if (lk >= 'A' && lk <= 'Z') lk += 32;
                if (lk == 't') {
                    for (int i = 0; i < g_app_count; ++i) {
                        if (g_apps[i].is_builtin_term) {
                            launch_app(i);
                            break;
                        }
                    }
                    k = gui_poll_key();
                    continue;
                }
                for (int i = 0; i < g_app_count; ++i) {
                    if (g_apps[i].hotkey && (g_apps[i].hotkey == lk || (g_apps[i].hotkey >= 'A' && g_apps[i].hotkey <= 'Z' && g_apps[i].hotkey + 32 == lk))) {
                        launch_app(i);
                        break;
                    }
                }
            }
            k = gui_poll_key();
        }

        /* Render */
        bool need_sync = false;
        if (g_dirty_canvas) {
            render_canvas_background();
            g_dirty_taskbar = true;
            g_dirty_term_chrome  = true;
            g_dirty_term_content = true;
            g_dirty_canvas  = false;
            need_sync = true;
        }

        if (g_dirty_taskbar) {
            render_taskbar_chrome();
            render_taskbar_clock(now_us);
            g_dirty_taskbar = false;
            g_prev_sec = sec;
            need_sync = true;
        } else if (sec != g_prev_sec) {
            render_taskbar_clock(now_us);
            g_prev_sec = sec;
            need_sync = true;
        }

        if (g_active_win == WIN_TERM) {
            if (g_dirty_term_chrome) {
                render_term_chrome();
                g_dirty_term_chrome = false;
                need_sync = true;
            }
            if (g_dirty_term_content) {
                render_term_content();
                g_dirty_term_content = false;
                need_sync = true;
            }
        }

        if (need_sync) {
            gui_sync();
        }

        if (ctx->host->sleep_ms) {
            ctx->host->sleep_ms(20);
        }
    }

    if (ctx->host->display && ctx->host->display->release_exclusive) {
        ctx->host->display->release_exclusive();
    }

    return 0;
}
