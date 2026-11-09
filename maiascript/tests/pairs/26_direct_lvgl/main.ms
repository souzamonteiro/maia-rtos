import maia.lvgl;

i32 g_res = 0;

void buildAdvancedWidget(lvgl.Container* root) {
    lvgl.Label* label = lvgl.Label(root);
    label->setText("LVGL backend");
    label->align(lvgl.Align.Center);

    if (label->align_mode == lvgl.Align.Center) {
        g_res = 40;
    }
    if (root->child_count == 1) {
        g_res = g_res + 2;
    }
    delete label;
}

i32 main() {
    lvgl.Container* root = lvgl.Container(0);
    root->setSize(800, 480);
    root->align(lvgl.Align.Center);

    buildAdvancedWidget(root);

    delete root;
    return g_res; // Retorna 42
}
