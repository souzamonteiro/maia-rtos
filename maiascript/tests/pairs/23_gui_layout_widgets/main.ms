import maia.app;
import maia.ui;

i32 g_brightness = 0;
i32 g_checked = 0;

void onBrightnessChanged(i32 val) {
    g_brightness = val;
}

void onToggleNightMode(i32 chk) {
    g_checked = chk;
}

i32 main() {
    ui.Window* win = ui.Window("Preferences", 520, 360);
    ui.Column* root = ui.Column(win);
    root->setPadding(16);
    root->setGap(12);

    ui.Label* heading = ui.Label(win, "Display settings");
    heading->setFontSize(22);

    ui.Row* row = ui.Row(root);

    ui.Slider* brightness = ui.Slider(win, 0, 100, 70);
    brightness->onChange(onBrightnessChanged);
    brightness->setValue(85);

    ui.CheckBox* nightMode = ui.CheckBox(win, "Night Mode", 0);
    nightMode->onToggle(onToggleNightMode);
    nightMode->toggle();

    win->show();
    brightness->render();
    nightMode->render();

    i32 bVal = brightness->getValue();
    i32 chkVal = nightMode->isChecked();

    delete win;
    delete root;
    delete heading;
    delete row;
    delete brightness;
    delete nightMode;

    return (bVal - 43); // 85 - 43 = 42
}

