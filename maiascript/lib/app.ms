#pragma once

extern void host_app_quit(i32 code);
extern i32 host_app_run();

namespace app {
    void quit(i32 code) {
        host_app_quit(code);
    }

    i32 run() {
        return host_app_run();
    }
}

