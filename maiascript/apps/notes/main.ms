import maia.app;
import maia.ui;
import maia.fs;

ui.TextArea* editor;
string currentPath = "/sd/notes.txt";

void saveFile() {
    fs.writeText(currentPath, editor->text())?;
    ui.notify("Salvo com sucesso: /sd/notes.txt");
}

void clearFile() {
    editor->setText("");
    ui.notify("Editor limpo");
}

i32 main() {
    ui.Window* win = ui.Window("Maia Notes", 800, 520);
    ui.Column* layout = ui.Column(win);
    layout->setPadding(20);
    layout->setGap(12);

    ui.Row* toolbar = ui.Row(layout);
    ui.Button* btnSave = ui.Button(toolbar, "Save");
    btnSave->onClick(saveFile);

    ui.Button* btnClear = ui.Button(toolbar, "Clear");
    btnClear->onClick(clearFile);

    editor = ui.TextArea(layout, 1);
    editor->setExpand(1);

    string initialText = fs.readText(currentPath).orElse("MaiaScript Native v0.1\nEditor de Texto & MaiaFS no M5Stack Tab5!\nZero GC - RV32IMAF ILP32F.");
    editor->setText(initialText);

    win->show();
    return app.run();
}

