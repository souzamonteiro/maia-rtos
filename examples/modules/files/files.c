#include "gui_kit.h"

#define FM_MAX_ENTRIES 128
#define FM_ROWS_PER_PAGE 9

static maia_dir_entry_t g_fm_entries[FM_MAX_ENTRIES];
static int g_fm_entry_count = 0;
static int g_fm_scroll_offset = 0;
static char g_fm_current_path[128] = "/sd";
static int g_fm_drive = 0; /* 0 = /sd, 1 = /usb */
static int g_fm_selected_idx = -1;
static bool g_fm_has_parent = false;
static char g_fm_status_msg[96] = "Ready. Select an item to manage.";
static uint16_t g_fm_status_col = COLOR_WHITE;

static int str_len(const char *s) {
    int l = 0; while (s[l]) l++; return l;
}

static void str_copy(char *dst, const char *src, int max) {
    int i = 0;
    while (src[i] && i < max - 1) { dst[i] = src[i]; i++; }
    dst[i] = '\0';
}

static void set_fm_status(const char *msg, uint16_t col) {
    str_copy(g_fm_status_msg, msg, sizeof(g_fm_status_msg));
    g_fm_status_col = col;
}

static void draw_quick_icon(int x, int y, const char *label, uint16_t col) {
    gui_fill_rect(x + 8, y, 52, 52, COLOR_WIN_BG);
    gui_draw_rect(x + 8, y, 52, 52, COLOR_WIN_BD);
    gui_fill_rect(x + 12, y + 4, 44, 44, 0x0821);
    gui_draw_rect(x + 12, y + 4, 44, 44, col);
    gui_draw_str(x + 16, y + 18, label, col, 0x0821, true);
}

static void restore_screen_bg(void) {
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

    /* Render Top Taskbar */
    gui_fill_rect(0, 0, SCREEN_W, 36, COLOR_TASKBAR);
    gui_draw_rect(0, 35, SCREEN_W, 1, COLOR_TASK_ACC);
    gui_draw_str_2x(12, 2, "MaiaOS", COLOR_TASK_ACC, COLOR_TASKBAR, true);
    gui_draw_str(116, 10, "M5Stack Tab5 RV32", COLOR_TEXT_DIM, COLOR_TASKBAR, true);
    gui_draw_button(SCREEN_W - 80, 4, 70, 28, "EXIT", COLOR_BTN_BG, COLOR_RED, COLOR_BTN_BD, false);

    /* Render Left Desktop Icons */
    draw_quick_icon(24, 44, "FILES", COLOR_GOLD);
    draw_quick_icon(24, 122, "EDIT", COLOR_CYAN);
    draw_quick_icon(24, 200, "TERM", COLOR_GREEN);
    draw_quick_icon(24, 278, "CALC", COLOR_GREEN);
    draw_quick_icon(24, 356, "TIME", COLOR_YELLOW);
    draw_quick_icon(24, 434, "SYS", COLOR_GREEN);
    draw_quick_icon(24, 512, "BANR", COLOR_MAGENTA);
    draw_quick_icon(24, 590, "CAM", COLOR_MAGENTA);
    draw_quick_icon(116, 44, "WIFI", COLOR_CYAN);
}

static int str_case_cmp(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        char ca = (*a >= 'A' && *a <= 'Z') ? (*a + 32) : *a;
        char cb = (*b >= 'A' && *b <= 'Z') ? (*b + 32) : *b;
        if (ca != cb) return (int)(unsigned char)ca - (int)(unsigned char)cb;
        a++; b++;
    }
    return (int)(unsigned char)*a - (int)(unsigned char)*b;
}

static void fm_refresh(void) {
    if (!g_gui_ctx || !g_gui_ctx->host || !g_gui_ctx->host->list_dir) return;

    g_fm_entry_count = g_gui_ctx->host->list_dir(g_fm_current_path, g_fm_entries, FM_MAX_ENTRIES);
    if (g_fm_entry_count < 0) g_fm_entry_count = 0;
    g_fm_selected_idx = -1;
    g_fm_scroll_offset = 0;

    /* Sort entries: folders first, then alphabetical */
    for (int i = 0; i < g_fm_entry_count - 1; ++i) {
        for (int j = i + 1; j < g_fm_entry_count; ++j) {
            bool swap = false;
            if (g_fm_entries[j].is_dir && !g_fm_entries[i].is_dir) {
                swap = true;
            } else if (g_fm_entries[j].is_dir == g_fm_entries[i].is_dir) {
                if (str_case_cmp(g_fm_entries[i].name, g_fm_entries[j].name) > 0) {
                    swap = true;
                }
            }
            if (swap) {
                maia_dir_entry_t tmp = g_fm_entries[i];
                g_fm_entries[i] = g_fm_entries[j];
                g_fm_entries[j] = tmp;
            }
        }
    }

    g_fm_has_parent = (str_len(g_fm_current_path) > 3);
}

static void fm_enter_dir(const char *dir_name) {
    int l = str_len(g_fm_current_path);
    if (l + str_len(dir_name) + 2 >= (int)sizeof(g_fm_current_path)) return;
    if (l > 0 && g_fm_current_path[l - 1] != '/') {
        g_fm_current_path[l++] = '/';
        g_fm_current_path[l] = '\0';
    }
    str_copy(g_fm_current_path + l, dir_name, sizeof(g_fm_current_path) - l);
    fm_refresh();
}

static void fm_go_parent(void) {
    int l = str_len(g_fm_current_path);
    while (l > 3 && g_fm_current_path[l - 1] != '/') l--;
    if (l > 3) g_fm_current_path[l - 1] = '\0';
    else if (l == 3) g_fm_current_path[l] = '\0';
    fm_refresh();
}

static void render_files_chrome(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "File & Media Manager", COLOR_GOLD);

    int ty_bar = wy + 44;
    int bh = 30;
    gui_draw_button(wx + 192, ty_bar, 64, bh, "EJECT", 0x8180, COLOR_WHITE, COLOR_RED, false);
    gui_draw_button(wx + 260, ty_bar, 76, bh, "REFRESH", COLOR_BTN_BG, COLOR_WHITE, COLOR_BTN_BD, false);
    gui_draw_button(wx + 340, ty_bar, 74, bh, "< PREV", COLOR_BTN_BG, COLOR_CYAN, COLOR_BTN_BD, false);
    gui_draw_button(wx + 418, ty_bar, 74, bh, "NEXT >", COLOR_BTN_BG, COLOR_CYAN, COLOR_BTN_BD, false);

    /* Path Bar Box */
    int py = ty_bar + 36;
    gui_fill_rect(wx + 16, py, ww - 32, 26, 0x0821);
    gui_draw_rect(wx + 16, py, ww - 32, 26, COLOR_WIN_BD);
    gui_draw_str(wx + 24, py + 5, "Path: ", COLOR_YELLOW, 0x0821, true);

    /* Table Container Box */
    int table_y = py + 30;
    int table_h = wh - 180;
    gui_fill_rect(wx + 16, table_y, ww - 32, table_h, 0x10a2);
    gui_draw_rect(wx + 16, table_y, ww - 32, table_h, COLOR_WIN_BD);

    /* Header Row */
    gui_fill_rect(wx + 18, table_y + 2, ww - 36, 26, 0x2128);
    gui_draw_str(wx + 28, table_y + 7, "TYPE", COLOR_TEXT_DIM, 0x2128, true);
    gui_draw_str(wx + 110, table_y + 7, "ITEM NAME", COLOR_TEXT_DIM, 0x2128, true);
    gui_draw_str(wx + 440, table_y + 7, "SIZE", COLOR_TEXT_DIM, 0x2128, true);
    gui_draw_str(wx + 580, table_y + 7, "AVAILABLE ACTION", COLOR_TEXT_DIM, 0x2128, true);
}

static void render_files_tabs(int wx, int wy) {
    int ty_bar = wy + 44;
    int tab_w = 84, tab_h = 30;

    /* Tabs: /sd e /usb */
    uint16_t sd_bg = (g_fm_drive == 0) ? COLOR_GOLD : 0x10a2;
    uint16_t sd_fg = (g_fm_drive == 0) ? COLOR_BLACK : COLOR_WHITE;
    gui_draw_button(wx + 16, ty_bar, tab_w, tab_h, "SD CARD", sd_bg, sd_fg, COLOR_GOLD, false);

    uint16_t usb_bg = (g_fm_drive == 1) ? COLOR_CYAN : 0x10a2;
    uint16_t usb_fg = (g_fm_drive == 1) ? COLOR_BLACK : COLOR_WHITE;
    gui_draw_button(wx + 104, ty_bar, tab_w, tab_h, "USB PEN", usb_bg, usb_fg, COLOR_CYAN, false);
}

static void render_files_path(int wx, int wy, int ww) {
    int py = wy + 44 + 36;
    gui_fill_rect(wx + 80, py + 2, ww - 100, 24, 0x0821);
    gui_draw_str_2x(wx + 80, py + 1, g_fm_current_path, COLOR_WHITE, 0x0821, true);
}

static void render_files_status(int wx, int wy, int ww, int wh) {
    int fy = wy + wh - 38;
    gui_fill_rect(wx + 16, fy, ww - 32, 28, 0x10a2);
    gui_draw_str_2x(wx + 26, fy + 2, g_fm_status_msg, g_fm_status_col, 0x10a2, true);
}

static void render_files_items(int wx, int wy, int ww, int wh) {
    int ty_bar = wy + 44;
    int py = ty_bar + 36;
    int table_y = py + 30;
    int table_h = wh - 180;

    /* Clear items body */
    gui_fill_rect(wx + 18, table_y + 30, ww - 36, table_h - 32, 0x10a2);

    int row_y = table_y + 32;
    int display_row = 0;
    int total_items = g_fm_entry_count + (g_fm_has_parent ? 1 : 0);

    for (int k = g_fm_scroll_offset; k < total_items && display_row < FM_ROWS_PER_PAGE; ++k) {
        if (g_fm_has_parent && k == 0) {
            gui_fill_rect(wx + 18, row_y, ww - 44, 32, 0x18e5);
            gui_draw_str_2x(wx + 24, row_y + 1, "[D]", COLOR_FOLDER, 0x18e5, true);
            gui_draw_str_2x(wx + 80, row_y + 1, ".. (Parent Dir)", COLOR_YELLOW, 0x18e5, true);
            gui_draw_button(wx + 580, row_y + 2, 110, 28, "UP", COLOR_BTN_BG, COLOR_CYAN, COLOR_BTN_BD, false);
        } else {
            int i = g_fm_has_parent ? (k - 1) : k;
            uint16_t row_bg = (i == g_fm_selected_idx) ? 0x29ec : 0x18e5;
            gui_fill_rect(wx + 18, row_y, ww - 44, 32, row_bg);

            if (g_fm_entries[i].is_dir) {
                gui_draw_str_2x(wx + 24, row_y + 1, "[D]", COLOR_FOLDER, row_bg, true);
                gui_draw_str_2x(wx + 80, row_y + 1, g_fm_entries[i].name, COLOR_WHITE, row_bg, true);
                gui_draw_str_2x(wx + 440, row_y + 1, "--", COLOR_TEXT_DIM, row_bg, true);
                gui_draw_button(wx + 580, row_y + 2, 110, 28, "OPEN", 0x1b24, COLOR_GREEN, COLOR_GREEN, false);
            } else {
                gui_draw_str_2x(wx + 24, row_y + 1, "[F]", COLOR_CYAN, row_bg, true);
                gui_draw_str_2x(wx + 80, row_y + 1, g_fm_entries[i].name, COLOR_WHITE, row_bg, true);

                /* Format size */
                char sz_str[16];
                uint32_t sz = g_fm_entries[i].size;
                if (sz < 1024) {
                    sz_str[0] = '0' + (sz % 10); sz_str[1] = 'B'; sz_str[2] = '\0';
                } else {
                    uint32_t kb = sz / 1024;
                    char tmp[12]; int tp = 0;
                    while (kb > 0) { tmp[tp++] = '0' + (kb % 10); kb /= 10; }
                    int p = 0; while (tp > 0) sz_str[p++] = tmp[--tp];
                    sz_str[p++] = ' '; sz_str[p++] = 'K'; sz_str[p++] = 'B'; sz_str[p] = '\0';
                }
                gui_draw_str_2x(wx + 440, row_y + 1, sz_str, COLOR_TEXT_DIM, row_bg, true);

                int nlen = str_len(g_fm_entries[i].name);
                if (g_fm_entries[i].is_exec) {
                    gui_draw_button(wx + 580, row_y + 2, 110, 28, "RUN", 0x03a8, COLOR_CYAN, COLOR_CYAN, false);
                } else if (nlen > 4 && g_fm_entries[i].name[nlen - 4] == '.' &&
                           (g_fm_entries[i].name[nlen - 3] == 'r' || g_fm_entries[i].name[nlen - 3] == 'b')) {
                    gui_draw_button(wx + 580, row_y + 2, 110, 28, "WALLPAPER", 0x518a, COLOR_GOLD, COLOR_GOLD, false);
                } else if (nlen > 4 && g_fm_entries[i].name[nlen - 4] == '.' &&
                           g_fm_entries[i].name[nlen - 3] == 'w' && g_fm_entries[i].name[nlen - 2] == 'a') {
                    gui_draw_button(wx + 580, row_y + 2, 110, 28, "PLAY AUDIO", 0x518a, COLOR_MAGENTA, COLOR_MAGENTA, false);
                } else if (nlen > 5 && g_fm_entries[i].name[nlen - 5] == '.' &&
                           g_fm_entries[i].name[nlen - 4] == 'm' && g_fm_entries[i].name[nlen - 3] == 'v') {
                    gui_draw_button(wx + 580, row_y + 2, 110, 28, "PLAY VIDEO", 0x518a, COLOR_YELLOW, COLOR_YELLOW, false);
                } else {
                    gui_draw_button(wx + 580, row_y + 2, 110, 28, "EDIT", COLOR_BTN_BG, COLOR_WHITE, COLOR_BTN_BD, false);
                }
            }
        }

        row_y += 36;
        display_row++;
    }

    /* Scrollbar track and thumb on right edge */
    int sb_x = wx + ww - 24;
    int sb_y = table_y + 32;
    int sb_h = table_h - 36;
    gui_fill_rect(sb_x, sb_y, 6, sb_h, 0x18e5);
    if (total_items > FM_ROWS_PER_PAGE) {
        int thumb_h = sb_h * FM_ROWS_PER_PAGE / total_items;
        if (thumb_h < 18) thumb_h = 18;
        int max_scroll = total_items - FM_ROWS_PER_PAGE;
        int thumb_y = sb_y + (sb_h - thumb_h) * g_fm_scroll_offset / max_scroll;
        gui_fill_rect(sb_x, thumb_y, 6, thumb_h, COLOR_CYAN);
    } else {
        gui_fill_rect(sb_x, sb_y, 6, sb_h, COLOR_CYAN);
    }
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting File Manager GUI (/sd/apps/files.mobj)...\n");

    fm_refresh();

    int wx = 260, wy = 60, ww = 760, wh = 580;
    render_files_chrome(wx, wy, ww, wh);
    render_files_tabs(wx, wy);
    render_files_path(wx, wy, ww);
    render_files_items(wx, wy, ww, wh);
    render_files_status(wx, wy, ww, wh);
    gui_sync();

    bool running = true;
    bool was_touched = false;

    while (running) {
        /* Keyboard */
        if (ctx->host->input && ctx->host->input->read_key) {
            int32_t k = ctx->host->input->read_key();
            while (k >= 0) {
                if (k == 27 || k == 'q' || k == 'Q') {
                    running = false;
                    break;
                } else if (k == 0x80) { /* Up Arrow */
                    if (g_fm_scroll_offset > 0) {
                        g_fm_scroll_offset--;
                        render_files_items(wx, wy, ww, wh);
                        gui_sync();
                    }
                } else if (k == 0x81) { /* Down Arrow */
                    int total_items = g_fm_entry_count + (g_fm_has_parent ? 1 : 0);
                    if (g_fm_scroll_offset + FM_ROWS_PER_PAGE < total_items) {
                        g_fm_scroll_offset++;
                        render_files_items(wx, wy, ww, wh);
                        gui_sync();
                    }
                } else if (k == 0x82) { /* Left Arrow */
                    if (g_fm_has_parent) {
                        fm_go_parent();
                        render_files_path(wx, wy, ww);
                        render_files_items(wx, wy, ww, wh);
                        gui_sync();
                    }
                }
                k = ctx->host->input->read_key();
            }
        }

        /* Touch */
        int32_t tx = 0, ty = 0;
        bool pressed = false;
        if (gui_poll_touch(&tx, &ty, &pressed)) {
                if (pressed && !was_touched) {
                    was_touched = true;
                    if (gui_is_close_clicked(wx, wy, ww, tx, ty)) {
                        running = false;
                        break;
                    }

                    int ty_bar = wy + 44;

                    /* Tab: /sd */
                    if (ty >= ty_bar && ty <= ty_bar + 30 && tx >= wx + 16 && tx <= wx + 100) {
                        g_fm_drive = 0;
                        str_copy(g_fm_current_path, "/sd", sizeof(g_fm_current_path));
                        fm_refresh();
                        render_files_tabs(wx, wy);
                        render_files_path(wx, wy, ww);
                        render_files_items(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* Tab: /usb */
                    else if (ty >= ty_bar && ty <= ty_bar + 30 && tx >= wx + 104 && tx <= wx + 188) {
                        g_fm_drive = 1;
                        str_copy(g_fm_current_path, "/usb", sizeof(g_fm_current_path));
                        fm_refresh();
                        render_files_tabs(wx, wy);
                        render_files_path(wx, wy, ww);
                        render_files_items(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* EJECT */
                    else if (ty >= ty_bar && ty <= ty_bar + 30 && tx >= wx + 192 && tx <= wx + 280) {
                        if (ctx->host->usb_unmount && ctx->host->usb_unmount() == 0) {
                            set_fm_status("[OK] USB drive safely ejected.", COLOR_GREEN);
                        } else {
                            set_fm_status("[INFO] USB drive was not mounted.", COLOR_YELLOW);
                        }
                        g_fm_drive = 0;
                        str_copy(g_fm_current_path, "/sd", sizeof(g_fm_current_path));
                        fm_refresh();
                        render_files_tabs(wx, wy);
                        render_files_path(wx, wy, ww);
                        render_files_items(wx, wy, ww, wh);
                        render_files_status(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* REFRESH */
                    else if (ty >= ty_bar && ty <= ty_bar + 30 && tx >= wx + 260 && tx <= wx + 336) {
                        fm_refresh();
                        set_fm_status("File list refreshed.", COLOR_CYAN);
                        render_files_items(wx, wy, ww, wh);
                        render_files_status(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* PREV */
                    else if (ty >= ty_bar && ty <= ty_bar + 30 && tx >= wx + 340 && tx <= wx + 414) {
                        if (g_fm_scroll_offset > 0) {
                            g_fm_scroll_offset = (g_fm_scroll_offset >= 3) ? (g_fm_scroll_offset - 3) : 0;
                            render_files_items(wx, wy, ww, wh);
                            gui_sync();
                        }
                    }
                    /* NEXT */
                    else if (ty >= ty_bar && ty <= ty_bar + 30 && tx >= wx + 418 && tx <= wx + 492) {
                        int total_items = g_fm_entry_count + (g_fm_has_parent ? 1 : 0);
                        if (g_fm_scroll_offset + FM_ROWS_PER_PAGE < total_items) {
                            g_fm_scroll_offset += 3;
                            if (g_fm_scroll_offset + FM_ROWS_PER_PAGE > total_items) {
                                g_fm_scroll_offset = total_items - FM_ROWS_PER_PAGE;
                            }
                            render_files_items(wx, wy, ww, wh);
                            gui_sync();
                        }
                    }

                    /* Table Row Touches */
                    int py = ty_bar + 36;
                    int table_y = py + 30;
                    int table_h = wh - 180;
                    int row_y = table_y + 32;
                    int display_row = 0;
                    int total_items = g_fm_entry_count + (g_fm_has_parent ? 1 : 0);

                    /* Scrollbar track touch */
                    if (tx >= wx + ww - 32 && tx <= wx + ww - 8 && ty >= table_y + 32 && ty <= table_y + table_h - 4) {
                        if (total_items > FM_ROWS_PER_PAGE) {
                            int mid_y = table_y + 32 + (table_h - 36) / 2;
                            if (ty < mid_y) {
                                g_fm_scroll_offset = (g_fm_scroll_offset >= 3) ? (g_fm_scroll_offset - 3) : 0;
                            } else {
                                g_fm_scroll_offset += 3;
                                if (g_fm_scroll_offset + FM_ROWS_PER_PAGE > total_items) {
                                    g_fm_scroll_offset = total_items - FM_ROWS_PER_PAGE;
                                }
                            }
                            render_files_items(wx, wy, ww, wh);
                            gui_sync();
                        }
                    }

                    /* Rows */
                    for (int k = g_fm_scroll_offset; k < total_items && display_row < FM_ROWS_PER_PAGE; ++k) {
                        if (ty >= row_y && ty <= row_y + 34 && tx >= wx + 16 && tx <= wx + ww - 36) {
                            if (g_fm_has_parent && k == 0) {
                                fm_go_parent();
                                render_files_path(wx, wy, ww);
                                render_files_items(wx, wy, ww, wh);
                                gui_sync();
                            } else {
                                int i = g_fm_has_parent ? (k - 1) : k;
                                char full_p[160];
                                str_copy(full_p, g_fm_current_path, sizeof(full_p));
                                int l = str_len(full_p);
                                if (l > 0 && full_p[l - 1] != '/') { full_p[l++] = '/'; full_p[l] = '\0'; }
                                str_copy(full_p + l, g_fm_entries[i].name, sizeof(full_p) - l);

                                /* Click on action button on the right */
                                if (tx >= wx + 580 && tx <= wx + 700) {
                                if (g_fm_entries[i].is_dir) {
                                    fm_enter_dir(g_fm_entries[i].name);
                                    render_files_path(wx, wy, ww);
                                    render_files_items(wx, wy, ww, wh);
                                    gui_sync();
                                } else if (g_fm_entries[i].is_exec) {
                                    char cmd[180];
                                    str_copy(cmd, "run ", sizeof(cmd));
                                    int cl = str_len(cmd);
                                    str_copy(cmd + cl, full_p, sizeof(cmd) - cl);
                                    if (ctx->host->shell_exec) ctx->host->shell_exec(cmd, NULL, NULL);
                                    while (gui_poll_key() >= 0) {}
                                    int32_t dtx, dty; bool dpr;
                                    while (gui_poll_touch(&dtx, &dty, &dpr) && dpr) {
                                        gui_delay_ms(10);
                                    }
                                    gui_delay_ms(40);
                                    while (gui_poll_touch(&dtx, &dty, &dpr) && dpr) {
                                        gui_delay_ms(10);
                                    }
                                    restore_screen_bg();
                                    render_files_chrome(wx, wy, ww, wh);
                                    render_files_tabs(wx, wy);
                                    render_files_path(wx, wy, ww);
                                    render_files_items(wx, wy, ww, wh);
                                    set_fm_status("[OK] Module executed successfully.", COLOR_GREEN);
                                    render_files_status(wx, wy, ww, wh);
                                    gui_sync();
                                } else {
                                    int nlen = str_len(g_fm_entries[i].name);
                                    if (nlen > 4 && g_fm_entries[i].name[nlen - 4] == '.' &&
                                        (g_fm_entries[i].name[nlen - 3] == 'r' || g_fm_entries[i].name[nlen - 3] == 'b')) {
                                        if (ctx->host->apply_wallpaper && ctx->host->apply_wallpaper(full_p) == 0) {
                                            set_fm_status("[OK] Wallpaper applied successfully!", COLOR_GOLD);
                                            render_files_status(wx, wy, ww, wh);
                                            gui_sync();
                                        }
                                    } else if (nlen > 4 && g_fm_entries[i].name[nlen - 4] == '.' &&
                                               g_fm_entries[i].name[nlen - 3] == 'w' && g_fm_entries[i].name[nlen - 2] == 'a') {
                                        if (ctx->host->play_wav) {
                                            ctx->host->play_wav(full_p);
                                            set_fm_status("[AUDIO] Playing WAV audio file...", COLOR_MAGENTA);
                                            render_files_status(wx, wy, ww, wh);
                                            gui_sync();
                                        }
                                    } else if (nlen > 5 && g_fm_entries[i].name[nlen - 5] == '.' &&
                                               g_fm_entries[i].name[nlen - 4] == 'm' && g_fm_entries[i].name[nlen - 3] == 'v') {
                                        if (ctx->host->video_play) {
                                            ctx->host->video_play(full_p, 0);
                                            set_fm_status("[VIDEO] Video playback completed.", COLOR_YELLOW);
                                            render_files_status(wx, wy, ww, wh);
                                            gui_sync();
                                        }
                                    } else {
                                        /* Open in external editor */
                                        char cmd[180];
                                        str_copy(cmd, "runelf /sd/apps/editor.mobj ", sizeof(cmd));
                                        int cl = str_len(cmd);
                                        str_copy(cmd + cl, full_p, sizeof(cmd) - cl);
                                        if (ctx->host->shell_exec) ctx->host->shell_exec(cmd, NULL, NULL);
                                        while (gui_poll_key() >= 0) {}
                                        int32_t dtx, dty; bool dpr;
                                        while (gui_poll_touch(&dtx, &dty, &dpr) && dpr) {
                                            gui_delay_ms(10);
                                        }
                                        gui_delay_ms(40);
                                        while (gui_poll_touch(&dtx, &dty, &dpr) && dpr) {
                                            gui_delay_ms(10);
                                        }
                                        restore_screen_bg();
                                        render_files_chrome(wx, wy, ww, wh);
                                        render_files_tabs(wx, wy);
                                        render_files_path(wx, wy, ww);
                                        render_files_items(wx, wy, ww, wh);
                                        set_fm_status("[OK] File editing completed.", COLOR_GREEN);
                                        render_files_status(wx, wy, ww, wh);
                                        gui_sync();
                                    }
                                }
                            } else {
                                g_fm_selected_idx = i;
                                set_fm_status(g_fm_entries[i].name, COLOR_CYAN);
                                render_files_items(wx, wy, ww, wh);
                                render_files_status(wx, wy, ww, wh);
                                gui_sync();
                            }
                        }
                    }
                        row_y += 36;
                        display_row++;
                    }
                } else if (!pressed) {
                    was_touched = false;
                }
            }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(30);
    }

    gui_out_str("<<< File Manager GUI exited cleanly.\n");
    return 0;
}

