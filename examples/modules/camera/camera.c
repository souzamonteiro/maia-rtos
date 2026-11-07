#include "gui_kit.h"

static char g_cam_msg[96] = "Ready for snapshot or MJPEG video.";
static uint16_t g_cam_msg_col = COLOR_WHITE;

static int str_len(const char *s) {
    int l = 0; while (s && s[l]) l++; return l;
}

static void set_cam_msg(const char *msg, uint16_t col) {
    int i = 0;
    while (msg[i] && i < 95) { g_cam_msg[i] = msg[i]; i++; }
    g_cam_msg[i] = '\0';
    g_cam_msg_col = col;
}

static void render_camera_chrome(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "Camera Studio & MJPEG Video", COLOR_MAGENTA);

    /* Viewfinder container placeholder */
    int vx = wx + 40, vy = wy + 48, vw = ww - 80, vh = 320;
    gui_fill_rect(vx, vy, vw, vh, COLOR_BLACK);
    gui_draw_rect(vx, vy, vw, vh, COLOR_MAGENTA);

    /* Camera lens graphical indicator */
    int cx = vx + vw / 2, cy = vy + vh / 2;
    gui_fill_rect(cx - 36, cy - 40, 72, 72, 0x2128);
    gui_draw_rect(cx - 36, cy - 40, 72, 72, COLOR_CYAN);
    gui_fill_rect(cx - 18, cy - 22, 36, 36, 0x0182);
    gui_draw_rect(cx - 18, cy - 22, 36, 36, COLOR_WHITE);

    gui_draw_str_2x(cx - 160, cy + 42, "SC202CS (MIPI CSI 1080p)", COLOR_WHITE, COLOR_BLACK, true);
    gui_draw_str_2x(cx - 176, cy + 74, "PPA HW SRM + HW JPEG 60 FPS", COLOR_CYAN, COLOR_BLACK, true);

    /* Status Bar frame */
    int sy = vy + vh + 16;
    gui_fill_rect(vx, sy, vw, 40, 0x10a2);
    gui_draw_rect(vx, sy, vw, 40, COLOR_WIN_BD);

    /* Action Buttons (4 Buttons) */
    int by = sy + 54;
    int bw = (vw - 36) / 4;
    int bh = 46;

    gui_draw_button(vx,                     by, bw, bh, "LIVE VIEW",  0x03a8, COLOR_CYAN,   COLOR_CYAN,   false);
    gui_draw_button(vx + (bw + 12),         by, bw, bh, "SNAP PHOTO", 0x1b24, COLOR_GREEN,  COLOR_GREEN,  false);
    gui_draw_button(vx + (bw + 12) * 2,     by, bw, bh, "RECORD (5s)",0x8180, COLOR_YELLOW, COLOR_YELLOW, false);
    gui_draw_button(vx + (bw + 12) * 3,     by, bw, bh, "PLAYBACK",   0x518a, COLOR_WHITE,  COLOR_WHITE,  false);
}

static void render_camera_status(int wx, int wy, int ww) {
    int vx = wx + 40, vy = wy + 48, vw = ww - 80, vh = 320;
    int sy = vy + vh + 16;
    gui_fill_rect(vx + 2, sy + 2, vw - 4, 36, 0x10a2);
    gui_set_clip(vx + 12, sy + 2, vw - 24, 36);
    int len = str_len(g_cam_msg);
    if (len <= 38) {
        gui_draw_str_2x(vx + 16, sy + 4, g_cam_msg, g_cam_msg_col, 0x10a2, true);
    } else if (len <= 51) {
        gui_draw_str_1_5x(vx + 16, sy + 8, g_cam_msg, g_cam_msg_col, 0x10a2, true);
    } else {
        gui_draw_str(vx + 16, sy + 12, g_cam_msg, g_cam_msg_col, 0x10a2, true);
    }
    gui_reset_clip();
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Camera GUI (/sd/apps/camera.mobj)...\n");

    int wx = 280, wy = 80, ww = 720, wh = 520;
    render_camera_chrome(wx, wy, ww, wh);
    render_camera_status(wx, wy, ww);
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

                    int vx = wx + 40, vy = wy + 48, vw = ww - 80, vh = 320;
                    int sy = vy + vh + 16;
                    int by = sy + 54;
                    int bw = (vw - 36) / 4;
                    int bh = 46;

                    /* Button 1: LIVE VIEW */
                    if (tx >= vx && tx <= vx + bw && ty >= by && ty <= by + bh) {
                        set_cam_msg("Starting live preview (5 seconds)...", COLOR_CYAN);
                        render_camera_status(wx, wy, ww);
                        gui_sync();
                        if (ctx->host->camera_view) {
                            ctx->host->camera_view(5);
                        }
                        set_cam_msg("Live preview finished.", COLOR_GREEN);
                        render_camera_status(wx, wy, ww);
                        gui_sync();
                    }
                    /* Button 2: SNAP PHOTO */
                    else if (tx >= vx + (bw + 12) && tx <= vx + (bw + 12) + bw && ty >= by && ty <= by + bh) {
                        set_cam_msg("Capturing JPEG snapshot...", COLOR_YELLOW);
                        render_camera_status(wx, wy, ww);
                        gui_sync();
                        int ret = -1;
                        if (ctx->host->camera_snap) {
                            ret = ctx->host->camera_snap("/sd/DCIM/photo.jpg");
                        }
                        if (ret == 0) set_cam_msg("[OK] Photo saved to /sd/DCIM/photo.jpg", COLOR_GREEN);
                        else set_cam_msg("[ERROR] Failed to capture photo.", COLOR_RED);
                        render_camera_status(wx, wy, ww);
                        gui_sync();
                    }
                    /* Button 3: RECORD (5s) */
                    else if (tx >= vx + (bw + 12) * 2 && tx <= vx + (bw + 12) * 2 + bw && ty >= by && ty <= by + bh) {
                        set_cam_msg("Recording 5s MJPEG video...", COLOR_RED);
                        render_camera_status(wx, wy, ww);
                        gui_sync();
                        int ret = -1;
                        if (ctx->host->video_record) {
                            ret = ctx->host->video_record("/sd/DCIM/video.mvid", 5, 15);
                        }
                        if (ret > 0) set_cam_msg("[OK] Video MJPEG recorded successfully!", COLOR_GREEN);
                        else set_cam_msg("[ERROR] Video recording failed.", COLOR_RED);
                        render_camera_status(wx, wy, ww);
                        gui_sync();
                    }
                    /* Button 4: PLAYBACK */
                    else if (tx >= vx + (bw + 12) * 3 && tx <= vx + (bw + 12) * 3 + bw && ty >= by && ty <= by + bh) {
                        set_cam_msg("Playing /sd/DCIM/video.mvid...", COLOR_YELLOW);
                        render_camera_status(wx, wy, ww);
                        gui_sync();
                        int ret = -1;
                        if (ctx->host->video_play) {
                            ret = ctx->host->video_play("/sd/DCIM/video.mvid", 15);
                        }
                        if (ret >= 0) set_cam_msg("[OK] Video playback completed.", COLOR_GREEN);
                        else set_cam_msg("[WARN] File not found or playback error.", COLOR_YELLOW);
                        render_camera_status(wx, wy, ww);
                        gui_sync();
                    }
                } else if (!pressed) {
                    was_touched = false;
                }
            }

        if (ctx->host->sleep_ms) ctx->host->sleep_ms(30);
    }

    gui_out_str("<<< Camera GUI exited cleanly.\n");
    return 0;
}

