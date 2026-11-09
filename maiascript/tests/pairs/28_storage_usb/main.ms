import maia.storage;
import maia.ui;

i32 g_unmounted_ok = 0;

void onUnmountDone(storage.StorageResult* result) {
    if (result->isOk() != 0) {
        g_unmounted_ok = 40;
    } else {
        g_unmounted_ok = 0;
    }
}

i32 main() {
    storage.Device* dev = new storage.Device();
    dev->init("usb0", "Kingston USB", "/media/usb0", 16384, 1);

    if (dev->isMounted() != 0) {
        storage.StorageResult* res = storage.unmount(dev->getId());
        onUnmountDone(res);
        delete res;
    }

    delete dev;

    if (g_unmounted_ok == 40) {
        if (storage.getDeviceCount() == 2) {
            return g_unmounted_ok + 2; // 40 + 2 = 42
        }
    }
    return 0;
}

