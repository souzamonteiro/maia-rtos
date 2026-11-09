import maia.app;
import maia.ui;
import maia.graphics;

i32 g_key_handled = 0;
i32 g_pointer_handled = 0;
i32 g_draw_count = 0;

void onKey(ui.KeyEvent* e) {
    if (e->pressed != 0) {
        if (e->key == ui.Key.Escape) {
            g_key_handled = 20;
        }
    }
}

void onPointer(ui.PointerEvent* e) {
    if (e->type == ui.Pointer.Down) {
        g_pointer_handled = 22;
    }
}

void draw(graphics.Canvas* c) {
    c->clear(graphics.Color(18, 24, 38));
    c->line(10, 10, 200, 90, graphics.Color(0, 220, 180), 3);
    c->rect(40, 120, 120, 60, graphics.Color(80, 130, 255));
    c->text(20, 220, "Maia graphics", 0xffff);
    g_draw_count = g_draw_count + 1;
}

i32 main() {
    ui.Window* win = ui.Window("Canvas", 500, 320);
    graphics.Canvas* canvas = ui.Canvas(win);

    win->onKey(onKey);
    win->onPointer(onPointer);

    canvas->fill();
    canvas->onDraw(draw);

    win->show();

    // Simula disparos de eventos de teclado e pointer
    win->triggerKey(1, ui.Key.Escape);
    win->triggerPointer(ui.Pointer.Down, 150, 200);

    delete win;
    delete canvas;

    // Retorna 20 + 22 = 42
    return g_key_handled + g_pointer_handled;
}

