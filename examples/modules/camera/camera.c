#include "gui_kit.h"

static char g_cam_msg[96] = "Pronto para captura ou gravacao MJPEG.";
static uint16_t g_cam_msg_col = COLOR_WHITE;

static void set_cam_msg(const char *msg, uint16_t col) {
    int i = 0;
    while (msg[i] && i < 95) { g_cam_msg[i] = msg[i]; i++; }
    g_cam_msg[i] = '\0';
    g_cam_msg_col = col;
}

static void render_camera(int wx, int wy, int ww, int wh) {
    gui_draw_window_frame(wx, wy, ww, wh, "Estudio de Camera & Video MJPEG", COLOR_MAGENTA);

    /* Viewfinder container placeholder */
    int vx = wx + 40, vy = wy + 48, vw = ww - 80, vh = 320;
    gui_fill_rect(vx, vy, vw, vh, COLOR_BLACK);
    gui_draw_rect(vx, vy, vw, vh, COLOR_MAGENTA);

    /* Camera lens graphical indicator */
    int cx = vx + vw / 2, cy = vy + vh / 2;
    gui_fill_rect(cx - 30, cy - 30, 60, 60, 0x2128);
    gui_draw_rect(cx - 30, cy - 30, 60, 60, COLOR_CYAN);
    gui_fill_rect(cx - 15, cy - 15, 30, 30, 0x0182);
    gui_draw_rect(cx - 15, cy - 15, 30, 30, COLOR_WHITE);

    gui_draw_str(cx - 110, cy + 45, "SENSOR SC202CS (MIPI CSI 1080p)", COLOR_WHITE, COLOR_BLACK, true);
    gui_draw_str(cx - 130, cy + 65, "PPA Hardware SRM + HW JPEG 60 FPS", COLOR_CYAN, COLOR_BLACK, true);

    /* Status Bar */
    int sy = vy + vh + 16;
    gui_fill_rect(vx, sy, vw, 34, 0x10a2);
    gui_draw_rect(vx, sy, vw, 34, COLOR_WIN_BD);
    gui_draw_str(vx + 16, sy + 10, g_cam_msg, g_cam_msg_col, 0x10a2, true);

    /* Action Buttons (4 Buttons) */
    int by = sy + 50;
    int bw = (vw - 36) / 4;
    int bh = 46;

    gui_draw_button(vx,                     by, bw, bh, "LIVE VIEW (5s)",  0x03a8, COLOR_CYAN,   COLOR_CYAN,   false);
    gui_draw_button(vx + (bw + 12),         by, bw, bh, "TIRAR FOTO",      0x1b24, COLOR_GREEN,  COLOR_GREEN,  false);
    gui_draw_button(vx + (bw + 12) * 2,     by, bw, bh, "GRAVAR VÍDEO",    0x8180, COLOR_YELLOW, COLOR_YELLOW, false);
    gui_draw_button(vx + (bw + 12) * 3,     by, bw, bh, "REPRODUZIR",      0x518a, COLOR_WHITE,  COLOR_WHITE,  false);
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host) return 1;
    gui_init(ctx);

    gui_out_str(">>> Starting Camera GUI (/sd/apps/camera.mobj)...\n");

    int wx = 280, wy = 80, ww = 720, wh = 520;
    render_camera(wx, wy, ww, wh);
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
                    int by = sy + 50;
                    int bw = (vw - 36) / 4;
                    int bh = 46;

                    /* Button 1: LIVE VIEW (5s) */
                    if (tx >= vx && tx <= vx + bw && ty >= by && ty <= by + bh) {
                        set_cam_msg("Iniciando preview ao vivo por 5 segundos...", COLOR_CYAN);
                        render_camera(wx, wy, ww, wh);
                        gui_sync();
                        if (ctx->host->camera_view) {
                            ctx->host->camera_view(5);
                        }
                        set_cam_msg("Preview de video concluido.", COLOR_GREEN);
                        render_camera(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* Button 2: TIRAR FOTO */
                    else if (tx >= vx + (bw + 12) && tx <= vx + (bw + 12) + bw && ty >= by && ty <= by + bh) {
                        set_cam_msg("Capturando foto JPEG...", COLOR_YELLOW);
                        render_camera(wx, wy, ww, wh);
                        gui_sync();
                        int ret = -1;
                        if (ctx->host->camera_snap) {
                            ret = ctx->host->camera_snap("/sd/DCIM/foto.jpg");
                        }
                        if (ret == 0) set_cam_msg("[OK] Foto gravada em /sd/DCIM/foto.jpg", COLOR_GREEN);
                        else set_cam_msg("[ERRO] Falha ao capturar imagem.", COLOR_RED);
                        render_camera(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* Button 3: GRAVAR VÍDEO (5s) */
                    else if (tx >= vx + (bw + 12) * 2 && tx <= vx + (bw + 12) * 2 + bw && ty >= by && ty <= by + bh) {
                        set_cam_msg("Gravando video MJPEG 5s...", COLOR_RED);
                        render_camera(wx, wy, ww, wh);
                        gui_sync();
                        int ret = -1;
                        if (ctx->host->video_record) {
                            ret = ctx->host->video_record("/sd/DCIM/video.mvid", 5, 15);
                        }
                        if (ret > 0) set_cam_msg("[OK] Video MJPEG gravado com sucesso!", COLOR_GREEN);
                        else set_cam_msg("[ERRO] Falha na gravacao de video.", COLOR_RED);
                        render_camera(wx, wy, ww, wh);
                        gui_sync();
                    }
                    /* Button 4: REPRODUZIR */
                    else if (tx >= vx + (bw + 12) * 3 && tx <= vx + (bw + 12) * 3 + bw && ty >= by && ty <= by + bh) {
                        set_cam_msg("Reproduzindo /sd/DCIM/video.mvid...", COLOR_YELLOW);
                        render_camera(wx, wy, ww, wh);
                        gui_sync();
                        int ret = -1;
                        if (ctx->host->video_play) {
                            ret = ctx->host->video_play("/sd/DCIM/video.mvid", 15);
                        }
                        if (ret >= 0) set_cam_msg("[OK] Reproducao concluida com exito.", COLOR_GREEN);
                        else set_cam_msg("[AVISO] Arquivo nao encontrado ou erro.", COLOR_YELLOW);
                        render_camera(wx, wy, ww, wh);
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

