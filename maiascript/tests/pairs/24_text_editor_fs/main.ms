import maia.app;
import maia.ui;
import maia.fs;

ui.TextArea* g_editor;
string currentPath = "/home/notes.txt";
i32 g_saved = 0;

void saveFile() {
    fs.writeText(currentPath, g_editor->text())?;
    ui.notify("Saved: /home/notes.txt");
    g_saved = 1;
}

i32 main() {
    // 1. Prepara sistema de arquivos
    fs.writeText(currentPath, "MaiaNative42")?;

    // 2. Constrói interface gráfica
    ui.Window* win = ui.Window("Maia Notes", 640, 480);
    ui.Column* layout = ui.Column(win);
    ui.Row* toolbar = ui.Row(layout);
    ui.Button* save = ui.Button(toolbar, "Save");
    save->onClick(saveFile);

    g_editor = ui.TextArea(layout, 1);
    g_editor->setExpand(1);

    // 3. Lê arquivo com fallback orElse
    string loaded = fs.readText(currentPath).orElse("empty");
    g_editor->setText(loaded);

    win->show();

    // 4. Salva alterações
    saveFile();

    // 5. Lê confirmação
    string txt = g_editor->text();

    delete win;
    delete layout;
    delete toolbar;
    delete save;
    delete g_editor;

    // Se salvou com sucesso e leu o texto ("MaiaNative42"[10]=='4' [11]=='2' -> (52-48)*10 + (50-48) = 42)
    if (g_saved != 0) {
        i32 c1 = txt[10] - 48;
        i32 c2 = txt[11] - 48;
        return c1 * 10 + c2;
    }
    return 0;
}
