#pragma once

extern void draw_rect(i32 x, i32 y, i32 w, i32 h, i32 color);
extern void fill_rect(i32 x, i32 y, i32 w, i32 h, i32 color);
extern void draw_line(i32 x1, i32 y1, i32 x2, i32 y2, i32 color, i32 width);
extern void draw_text(i32 x, i32 y, string text, i32 fg, i32 bg);
extern void display_sync();

namespace graphics {
    i32 Color(i32 r, i32 g, i32 b) {
        i32 r5 = (r >> 3) & 31;
        i32 g6 = (g >> 2) & 63;
        i32 b5 = (b >> 3) & 31;
        return (r5 << 11) | (g6 << 5) | b5;
    }

    class Canvas {
        void* parent;
        i32 x;
        i32 y;
        i32 w;
        i32 h;
        i32 draw_cb;

        void clear(i32 color) {
            fill_rect(x, y, w, h, color);
        }

        void line(i32 x1, i32 y1, i32 x2, i32 y2, i32 color, i32 width) {
            draw_line(x + x1, y + y1, x + x2, y + y2, color, width);
        }

        void rect(i32 rx, i32 ry, i32 rw, i32 rh, i32 color) {
            draw_rect(x + rx, y + ry, rw, rh, color);
        }

        void fillRect(i32 rx, i32 ry, i32 rw, i32 rh, i32 color) {
            fill_rect(x + rx, y + ry, rw, rh, color);
        }

        void text(i32 tx, i32 ty, string s, i32 color) {
            draw_text(x + tx, y + ty, s, color, 0);
        }

        void fill() {
            // Ajusta dimensões baseado no container pai se aplicável
        }

        void onDraw(i32 cb) {
            draw_cb = cb;
            this.render();
        }

        void render() {
            if (draw_cb != 0) {
                i32 cb = draw_cb;
                cb(this);
                display_sync();
            }
        }
    }
}

