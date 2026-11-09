#pragma once

namespace lvgl {
    namespace Align {
        const i32 Center = 1;
        const i32 TopLeft = 2;
        const i32 TopMid = 3;
        const i32 TopRight = 4;
        const i32 BottomLeft = 5;
        const i32 BottomMid = 6;
        const i32 BottomRight = 7;
        const i32 LeftMid = 8;
        const i32 RightMid = 9;
    }

    class Obj {
        i32 handle;
        i32 x;
        i32 y;
        i32 w;
        i32 h;
        i32 align_mode;

        void align(i32 alignment) {
            align_mode = alignment;
        }

        void setSize(i32 width, i32 height) {
            w = width;
            h = height;
        }
    }

    class Container {
        i32 handle;
        i32 x;
        i32 y;
        i32 w;
        i32 h;
        i32 align_mode;
        i32 child_count;

        void align(i32 alignment) {
            align_mode = alignment;
        }

        void setSize(i32 width, i32 height) {
            w = width;
            h = height;
        }
    }

    class Label {
        i32 handle;
        i32 parent_handle;
        string text;
        i32 align_mode;

        void setText(string t) {
            text = t;
        }

        void align(i32 alignment) {
            align_mode = alignment;
        }
    }

    Container* Container(Container* root) {
        Container* c = new Container();
        c->handle = 1001;
        if (root != 0) {
            root->child_count = root->child_count + 1;
        }
        return c;
    }

    Label* Label(Container* root) {
        Label* l = new Label();
        if (root != 0) {
            l->parent_handle = root->handle;
            root->child_count = root->child_count + 1;
        }
        return l;
    }
}
