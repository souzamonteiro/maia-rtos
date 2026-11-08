#pragma once

extern void draw_window(i32 x, i32 y, i32 w, i32 h, string title, i32 accent);
extern void draw_text(i32 x, i32 y, string text, i32 fg, i32 bg);
extern void draw_button(i32 x, i32 y, i32 w, i32 h, string label, i32 bg, i32 fg, i32 pressed);
extern void draw_rect(i32 x, i32 y, i32 w, i32 h, i32 color);
extern void fill_rect(i32 x, i32 y, i32 w, i32 h, i32 color);
extern void display_sync();
extern i32 touch_poll();
extern i32 touch_x();
extern i32 touch_y();
extern void delay_ms(i32 ms);

namespace ui {
    class Window {
        string title;
        i32 x;
        i32 y;
        i32 w;
        i32 h;
        i32 visible;

        void show() {
            visible = 1;
            draw_window(x, y, w, h, title, 0x07ff);
            display_sync();
        }

        void close() {
            visible = 0;
        }

        i32 getX() const { return x; }
        i32 getY() const { return y; }
        i32 getW() const { return w; }
        i32 getH() const { return h; }
    }

    Window* Window(string title, i32 w, i32 h) {
        Window* win = new Window();
        win->title = title;
        win->w = w;
        win->h = h;
        win->x = (1280 - w) / 2;
        win->y = (720 - h) / 2;
        win->visible = 0;
        return win;
    }

    class Label {
        Window* parent;
        string text;
        i32 rel_x;
        i32 rel_y;

        void setPosition(i32 x, i32 y) {
            rel_x = x;
            rel_y = y;
            if (parent) {
                draw_text(parent->x + rel_x, parent->y + rel_y, text, 0xffff, 0);
                display_sync();
            }
        }

        void setText(string t) {
            text = t;
            if (parent) {
                draw_text(parent->x + rel_x, parent->y + rel_y, text, 0xffff, 0);
                display_sync();
            }
        }
    }

    Label* Label(Window* win, string text) {
        Label* lbl = new Label();
        lbl->parent = win;
        lbl->text = text;
        lbl->rel_x = 24;
        lbl->rel_y = 30;
        return lbl;
    }

    class Button {
        Window* parent;
        string label;
        i32 rel_x;
        i32 rel_y;
        i32 w;
        i32 h;
        i32 callback_ptr;

        void setPosition(i32 x, i32 y) {
            rel_x = x;
            rel_y = y;
            if (parent) {
                draw_button(parent->x + rel_x, parent->y + rel_y, w, h, label, 0x2169, 0xffff, 0);
                display_sync();
            }
        }

        void setSize(i32 bw, i32 bh) {
            w = bw;
            h = bh;
        }

        void onClick(i32 cb) {
            callback_ptr = cb;
        }

        void triggerClick() {
            if (callback_ptr != 0) {
                i32 cb = callback_ptr;
                cb();
            }
        }

        void render(i32 pressed) {
            if (parent) {
                draw_button(parent->x + rel_x, parent->y + rel_y, w, h, label, 0x2169, 0xffff, pressed);
                display_sync();
            }
        }
    }

    Button* Button(Window* win, string label) {
        Button* btn = new Button();
        btn->parent = win;
        btn->label = label;
        btn->rel_x = 24;
        btn->rel_y = 90;
        btn->w = 120;
        btn->h = 40;
        btn->callback_ptr = 0;
        return btn;
    }
}
