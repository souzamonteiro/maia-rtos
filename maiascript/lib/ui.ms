#pragma once
#include "graphics.ms"

extern void draw_window(i32 x, i32 y, i32 w, i32 h, string title, i32 accent);
extern void draw_text(i32 x, i32 y, string text, i32 fg, i32 bg);
extern void draw_button(i32 x, i32 y, i32 w, i32 h, string label, i32 bg, i32 fg, i32 pressed);
extern void draw_rect(i32 x, i32 y, i32 w, i32 h, i32 color);
extern void fill_rect(i32 x, i32 y, i32 w, i32 h, i32 color);
extern void draw_line(i32 x1, i32 y1, i32 x2, i32 y2, i32 color, i32 width);
extern void display_sync();
extern i32 touch_poll();
extern i32 touch_x();
extern i32 touch_y();
extern void delay_ms(i32 ms);
extern void host_bind_button(i32 x, i32 y, i32 cb);

namespace ui {
    class KeyEvent {
        i32 pressed;
        i32 key;
        i32 modifiers;
    }

    class PointerEvent {
        i32 type;
        i32 x;
        i32 y;
    }

    namespace Key {
        const i32 Escape = 27;
        const i32 Enter = 13;
        const i32 Backspace = 8;
        const i32 Tab = 9;
        const i32 Space = 32;
    }

    namespace Pointer {
        const i32 Down = 1;
        const i32 Up = 2;
        const i32 Move = 3;
    }

    class Window {
        i32 x;
        i32 y;
        i32 w;
        i32 h;
        string title;
        i32 visible;
        i32 key_cb;
        i32 pointer_cb;

        void show() {
            visible = 1;
            draw_window(x, y, w, h, title, 0x07ff);
            display_sync();
        }

        void close() {
            visible = 0;
        }

        void onKey(i32 cb) {
            key_cb = cb;
        }

        void onPointer(i32 cb) {
            pointer_cb = cb;
        }

        void triggerKey(i32 pressed, i32 key) {
            if (key_cb != 0) {
                KeyEvent* e = new KeyEvent();
                e->pressed = pressed;
                e->key = key;
                e->modifiers = 0;
                i32 cb = key_cb;
                cb(e);
                delete e;
            }
        }

        void triggerPointer(i32 type, i32 px, i32 py) {
            if (pointer_cb != 0) {
                PointerEvent* p = new PointerEvent();
                p->type = type;
                p->x = px;
                p->y = py;
                i32 cb = pointer_cb;
                cb(p);
                delete p;
            }
        }

        i32 getX() const { return x; }
        i32 getY() const { return y; }
        i32 getW() const { return w; }
        i32 getH() const { return h; }
    }

    Window* Window(string title, i32 w, i32 h) {
        Window* win = new Window();
        win->w = w;
        win->h = h;
        win->x = (1280 - w) / 2;
        win->y = (720 - h) / 2;
        win->title = title;
        win->visible = 0;
        return win;
    }

    class Column {
        i32 x;
        i32 y;
        i32 w;
        i32 h;
        i32 padding;
        i32 gap;
        i32 next_y;

        void setPadding(i32 p) {
            padding = p;
            next_y = p;
        }

        void setGap(i32 g) {
            gap = g;
        }

        i32 allocHeight(i32 child_h) {
            i32 cur = next_y;
            next_y = next_y + child_h + gap;
            return cur;
        }

        i32 getX() const { return x; }
        i32 getY() const { return y; }
        i32 getW() const { return w; }
        i32 getH() const { return h; }
    }

    Column* Column(Window* win) {
        Column* col = new Column();
        col->x = win->getX() + 20;
        col->y = win->getY() + 45;
        col->w = win->getW() - 40;
        col->h = win->getH() - 60;
        col->padding = 16;
        col->gap = 12;
        col->next_y = 16;
        return col;
    }

    class Row {
        i32 x;
        i32 y;
        i32 w;
        i32 h;
        i32 padding;
        i32 gap;
        i32 next_x;

        void setPadding(i32 p) {
            padding = p;
            next_x = p;
        }

        void setGap(i32 g) {
            gap = g;
        }

        i32 allocWidth(i32 child_w) {
            i32 cur = next_x;
            next_x = next_x + child_w + gap;
            return cur;
        }

        i32 getX() const { return x; }
        i32 getY() const { return y; }
        i32 getW() const { return w; }
        i32 getH() const { return h; }
    }

    Row* Row(Column* col) {
        Row* row = new Row();
        row->x = col->x + col->padding;
        row->y = col->y + col->allocHeight(40);
        row->w = col->w - col->padding * 2;
        row->h = 40;
        row->padding = 0;
        row->gap = 16;
        row->next_x = 0;
        return row;
    }

    class Label {
        Window* parent;
        string text;
        i32 rel_x;
        i32 rel_y;
        i32 font_size;

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

        void setFontSize(i32 sz) {
            font_size = sz;
        }
    }

    Label* Label(Window* win, string text) {
        Label* lbl = new Label();
        lbl->parent = win;
        lbl->text = text;
        lbl->rel_x = 24;
        lbl->rel_y = 30;
        lbl->font_size = 16;
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
            if (parent) {
                host_bind_button(parent->x + rel_x, parent->y + rel_y, cb);
            }
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
        btn->render(0);
        return btn;
    }

    Button* Button(Row* row, string label) {
        Button* btn = new Button();
        btn->parent = row;
        btn->label = label;
        btn->rel_x = row->allocWidth(100);
        btn->rel_y = 4;
        btn->w = 100;
        btn->h = 32;
        btn->callback_ptr = 0;
        btn->render(0);
        return btn;
    }

    class Slider {
        Window* parent;
        i32 min_val;
        i32 max_val;
        i32 cur_val;
        i32 rel_x;
        i32 rel_y;
        i32 w;
        i32 h;
        i32 callback_ptr;

        void setPosition(i32 x, i32 y) {
            rel_x = x;
            rel_y = y;
        }

        void setSize(i32 sw, i32 sh) {
            w = sw;
            h = sh;
        }

        void onChange(i32 cb) {
            callback_ptr = cb;
        }

        i32 getValue() const {
            return cur_val;
        }

        void setValue(i32 val) {
            cur_val = val;
            this.render();
            if (callback_ptr != 0) {
                i32 cb = callback_ptr;
                cb(cur_val);
            }
        }

        void render() {
            if (parent) {
                i32 px = parent->x + rel_x;
                i32 py = parent->y + rel_y;
                fill_rect(px, py + h / 2 - 2, w, 4, 0x39e7);
                i32 range = max_val - min_val;
                if (range <= 0) { range = 1; }
                i32 fill_w = (w * (cur_val - min_val)) / range;
                fill_rect(px, py + h / 2 - 2, fill_w, 4, 0x07ff);
                i32 kx = px + fill_w - 6;
                fill_rect(kx, py + h / 2 - 10, 12, 20, 0xffff);
                draw_rect(kx, py + h / 2 - 10, 12, 20, 0x0000);
                display_sync();
            }
        }
    }

    Slider* Slider(Window* win, i32 min, i32 max, i32 initial) {
        Slider* s = new Slider();
        s->parent = win;
        s->min_val = min;
        s->max_val = max;
        s->cur_val = initial;
        s->rel_x = 24;
        s->rel_y = 150;
        s->w = 200;
        s->h = 30;
        s->callback_ptr = 0;
        return s;
    }

    class CheckBox {
        Window* parent;
        string label;
        i32 checked;
        i32 rel_x;
        i32 rel_y;
        i32 callback_ptr;

        void setPosition(i32 x, i32 y) {
            rel_x = x;
            rel_y = y;
        }

        void onToggle(i32 cb) {
            callback_ptr = cb;
        }

        i32 isChecked() const {
            return checked;
        }

        void setChecked(i32 chk) {
            checked = chk;
            this.render();
            if (callback_ptr != 0) {
                i32 cb = callback_ptr;
                cb(checked);
            }
        }

        void toggle() {
            if (checked != 0) {
                this.setChecked(0);
            } else {
                this.setChecked(1);
            }
        }

        void render() {
            if (parent) {
                i32 px = parent->x + rel_x;
                i32 py = parent->y + rel_y;
                draw_rect(px, py, 20, 20, 0xffff);
                if (checked != 0) {
                    fill_rect(px + 3, py + 3, 14, 14, 0x07e0);
                } else {
                    fill_rect(px + 3, py + 3, 14, 14, 0x10a2);
                }
                draw_text(px + 28, py + 2, label, 0xffff, 0);
                display_sync();
            }
        }
    }

    CheckBox* CheckBox(Window* win, string label, i32 initial) {
        CheckBox* cb = new CheckBox();
        cb->parent = win;
        cb->label = label;
        cb->checked = initial;
        cb->rel_x = 24;
        cb->rel_y = 190;
        cb->callback_ptr = 0;
        return cb;
    }

    class TextArea {
        Window* parent;
        string content;
        i32 multiline;
        i32 expand;
        i32 rel_x;
        i32 rel_y;
        i32 w;
        i32 h;

        void setText(string t) {
            content = t;
            this.render();
        }

        string text() {
            return content;
        }

        void setExpand(i32 exp) {
            expand = exp;
            if (parent && exp != 0) {
                w = parent->w - 40;
                h = parent->h - rel_y - 20;
            }
            this.render();
        }

        void render() {
            if (parent) {
                i32 px = parent->x + rel_x;
                i32 py = parent->y + rel_y;
                fill_rect(px, py, w, h, 0x18e3);
                draw_rect(px, py, w, h, 0x52aa);
                if (content) {
                    draw_text(px + 8, py + 8, content, 0xffff, 0x18e3);
                }
                display_sync();
            }
        }
    }

    TextArea* TextArea(Column* col, i32 multiline) {
        TextArea* ta = new TextArea();
        ta->parent = col;
        ta->content = "";
        ta->multiline = multiline;
        ta->expand = 0;
        ta->rel_x = col->padding;
        ta->rel_y = col->allocHeight(150);
        ta->w = col->w - col->padding * 2;
        ta->h = 150;
        return ta;
    }

    TextArea* TextArea(Window* win, i32 multiline) {
        TextArea* ta = new TextArea();
        ta->parent = win;
        ta->content = "";
        ta->multiline = multiline;
        ta->expand = 0;
        ta->rel_x = 20;
        ta->rel_y = 50;
        ta->w = win->w - 40;
        ta->h = win->h - 80;
        return ta;
    }

    class ListView {
        Window* parent;
        i32 count;
        i32 rel_x;
        i32 rel_y;
        i32 w;
        i32 h;

        void clear() {
            count = 0;
            if (parent) {
                fill_rect(parent->x + rel_x, parent->y + rel_y, w, h, 0x1082);
                draw_rect(parent->x + rel_x, parent->y + rel_y, w, h, 0x4208);
                display_sync();
            }
        }

        void add(string item) {
            if (parent) {
                i32 iy = parent->y + rel_y + 8 + count * 24;
                if (iy + 20 < parent->y + rel_y + h) {
                    draw_text(parent->x + rel_x + 12, iy, item, 0xffff, 0x1082);
                    display_sync();
                }
            }
            count = count + 1;
        }

        void fill() {
            if (parent) {
                fill_rect(parent->x + rel_x, parent->y + rel_y, w, h, 0x1082);
                draw_rect(parent->x + rel_x, parent->y + rel_y, w, h, 0x4208);
                display_sync();
            }
        }
    }

    ListView* ListView(Window* win) {
        ListView* lv = new ListView();
        lv->parent = win;
        lv->count = 0;
        lv->rel_x = 20;
        lv->rel_y = 50;
        lv->w = win->w - 40;
        lv->h = win->h - 70;
        return lv;
    }

    class Dialog {
        string message;
        i32 result_cb;

        void onResult(i32 cb) {
            result_cb = cb;
        }

        void triggerResult(i32 res) {
            if (result_cb != 0) {
                i32 cb = result_cb;
                cb(res);
            }
        }
    }

    Dialog* confirm(string msg) {
        Dialog* d = new Dialog();
        d->message = msg;
        d->result_cb = 0;
        draw_window(440, 260, 400, 200, "Confirmation", 0xf800);
        draw_text(470, 320, msg, 0xffff, 0);
        draw_button(470, 390, 80, 36, "Yes", 0x07e0, 0x0000, 0);
        draw_button(590, 390, 80, 36, "No", 0xf800, 0xffff, 0);
        display_sync();
        return d;
    }

    void notify(string msg) {
        draw_rect(400, 20, 480, 40, 0x07ff);
        fill_rect(402, 22, 476, 36, 0x2169);
        draw_text(420, 32, msg, 0xffff, 0x2169);
        display_sync();
    }

    graphics.Canvas* Canvas(Window* win) {
        graphics.Canvas* c = new graphics.Canvas();
        c->parent = win;
        c->x = win->getX() + 20;
        c->y = win->getY() + 45;
        c->w = win->getW() - 40;
        c->h = win->getH() - 60;
        c->draw_cb = 0;
        return c;
    }
}
