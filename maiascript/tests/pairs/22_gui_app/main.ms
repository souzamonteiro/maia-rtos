import maia.app;
import maia.ui;

i32 g_closed = 0;

void onClose() {
    g_closed = 1;
    app.quit(42);
}

i32 main() {
    ui.Window* win = ui.Window("Hello MaiaOS", 420, 260);
    ui.Label* title = ui.Label(win, "Hello from MaiaScript Native!");
    title->setPosition(24, 30);

    ui.Button* close = ui.Button(win, "Close");
    close->setPosition(24, 90);
    close->onClick(onClose);

    win->show();

    // Trigger user click event
    close->triggerClick();

    i32 exitCode = app.run();

    delete win;
    delete title;
    delete close;

    return exitCode;
}
