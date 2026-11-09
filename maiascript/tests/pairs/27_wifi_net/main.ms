import maia.ui;
import maia.net.wifi;

i32 g_scanned_count = 0;
i32 g_found_rssi = 0;

void scanDone(wifi.ScanResult* result, ui.ListView* list) {
    list->clear();
    i32 i = 0;
    i32 total = result->getCount();
    while (i < total) {
        wifi.Network* n = result->get(i);
        if (n != 0) {
            list->add(n->getSsid());
            if (i == 0) {
                g_found_rssi = n->getRssi();
            }
        }
        i = i + 1;
    }
    g_scanned_count = total;
}

i32 main() {
    ui.Window* win = ui.Window("Wi-Fi", 520, 380);
    ui.ListView* networks = ui.ListView(win);
    networks->fill();

    wifi.ScanResult* res = new wifi.ScanResult();
    wifi.scan(res);

    scanDone(res, networks);

    delete res;
    delete networks;
    delete win;

    // g_scanned_count = 2, g_found_rssi = -45
    // Retorna 2 * 21 = 42
    if (g_scanned_count == 2) {
        if (g_found_rssi == 0 - 45) {
            return 42;
        }
    }
    return 0;
}

