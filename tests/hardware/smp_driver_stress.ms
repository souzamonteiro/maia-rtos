extern i32 host_fs_exists(string path);
extern i32 host_fs_list_count(string path);
extern i32 host_kv_contains_text(string key);
extern i32 millis();
extern void delay_ms(i32 ms);
extern void fill_rect(i32 x, i32 y, i32 width, i32 height, i32 color);
extern void draw_text(i32 x, i32 y, string text, i32 foreground, i32 background);
extern void display_sync();
extern void println(string text);
extern void print_int(i32 value);

i32 main() {
    i32 started = millis();
    i32 last_report = started;
    i32 iterations = 0;
    i32 checksum = 0;

    if (host_fs_exists("/sd/desktop.mobj") != 1) {
        println("MAIA_SMP_STRESS_FAIL sd_desktop");
        return 1;
    }
    if (host_kv_contains_text("maia_p4_smp_stress_20261126") != 0) {
        println("MAIA_SMP_STRESS_FAIL kv_probe_key");
        return 2;
    }

    fill_rect(0, 0, 1280, 720, 0x0000);
    fill_rect(24, 288, 1232, 28, 0x2104);
    draw_text(24, 32, "MaiaRTOS P4 - SMP driver stress", 0xffff, 0x0000);
    draw_text(24, 72, "10 minutes: core 1, KV, SD, LCD DMA", 0x07ff, 0x0000);
    display_sync();
    println("MAIA_SMP_STRESS_STARTED seconds=600");

    while (millis() - started < 600000) {
        i32 present = host_kv_contains_text("maia_p4_smp_stress_20261126");
        if (present != 0) {
            println("MAIA_SMP_STRESS_FAIL kv_read");
            return 3;
        }

        if (host_fs_exists("/sd/desktop.mobj") != 1) {
            println("MAIA_SMP_STRESS_FAIL sd_stat");
            return 4;
        }
        if ((iterations & 15) == 0 && host_fs_list_count("/sd") < 1) {
            println("MAIA_SMP_STRESS_FAIL sd_list");
            return 5;
        }

        i32 work = 0;
        while (work < 128) {
            checksum = checksum + work + iterations;
            work = work + 1;
        }

        i32 x = 32 + (iterations % 1180);
        fill_rect(x, 336, 12, 12, 0x07e0);
        display_sync();
        iterations = iterations + 1;

        i32 now = millis();
        if (now - last_report >= 30000) {
            i32 elapsed = now - started;
            i32 width = elapsed * 1200 / 600000;
            fill_rect(40, 294, width, 16, 0x07e0);
            display_sync();
            println("MAIA_SMP_STRESS_PROGRESS");
            print_int(iterations);
            last_report = now;
        }
        delay_ms(1);
    }

    println("MAIA_SMP_STRESS_PASS");
    print_int(iterations);
    print_int(checksum);
    return 0;
}
