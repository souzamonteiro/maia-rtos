#include <M5Unified.h>
#include <Wire.h>
#include <lvgl.h>
#include "maia_shell.h"

#define TAB5_KEYBOARD_ADDR 0x6D
#define KEYBOARD_SDA 0
#define KEYBOARD_SCL 1

#define DRAW_BUF_LINES 60

// Monospace terminal font with LVGL 9 static_bitmap support
extern const lv_font_t lv_font_mono_24;

static uint8_t *draw_buf1 = nullptr;
static uint8_t *draw_buf2 = nullptr;

static lv_obj_t *term_textarea = nullptr;
static lv_obj_t *status_label = nullptr;

// Keyboard input buffer
static uint32_t kbd_fifo[64];
static uint8_t kbd_fifo_head = 0;
static uint8_t kbd_fifo_tail = 0;
static bool kbd_key_pressed = false;

// Command line accumulator
static char cmd_buf[256];
static size_t cmd_len = 0;

static uint32_t my_tick_get(void) {
    return millis();
}

static void my_disp_flush(lv_display_t *disp, const lv_area_t *area, uint8_t *px_map) {
    uint32_t w = lv_area_get_width(area);
    uint32_t h = lv_area_get_height(area);
    M5.Display.pushImageDMA(area->x1, area->y1, w, h, (const uint16_t *)px_map);
    lv_display_flush_ready(disp);
}

static void my_touch_read(lv_indev_t *indev, lv_indev_data_t *data) {
    M5.update();
    auto t = M5.Touch.getDetail();
    if (t.isPressed()) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->point.x = t.x;
        data->point.y = t.y;
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}

static MaiaShell shell;

static void shell_out(const char *s) {
    if (term_textarea && s) {
        lv_textarea_add_text(term_textarea, s);
    }
    Serial.print(s);
}

// Shell command execution
static void execute_shell_cmd(const char *cmd, bool from_button = false) {
    if (from_button) {
        lv_textarea_add_text(term_textarea, cmd);
        Serial.println(cmd);
    }
    lv_textarea_add_text(term_textarea, "\n");
    Serial.println();

    if (strcmp(cmd, "clear") == 0) {
        lv_textarea_set_text(term_textarea, "MaiaRTOS VTX Terminal (M5Stack Tab5 ST7121 1280x720)\n");
    } else {
        shell.execute(cmd);
    }

    lv_textarea_add_text(term_textarea, "maia$ ");
    lv_textarea_set_cursor_pos(term_textarea, LV_TEXTAREA_CURSOR_LAST);
}

// STM32 A164 Keyboard polling via I2C (GPIO 0/1 @ 400kHz)
static void poll_physical_keyboard() {
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x40);
    if (Wire.endTransmission(false) != 0) return;
    if (Wire.requestFrom((uint8_t)TAB5_KEYBOARD_ADDR, (uint8_t)1) != 1) return;
    uint8_t qlen = Wire.read();
    if (qlen < 1 || qlen > 32) return;

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x50);
    if (Wire.endTransmission(false) != 0) return;
    if (Wire.requestFrom((uint8_t)TAB5_KEYBOARD_ADDR, (uint8_t)(qlen + 1)) != (size_t)(qlen + 1)) return;

    uint8_t mod = Wire.read(); // Byte 0: modifier flags
    (void)mod;

    String str = "";
    while (Wire.available()) {
        char c = (char)Wire.read();
        if (c != 0) {
            str += c;
        }
    }

    // Acknowledge / clear interrupt
    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x01);
    Wire.write(0x00);
    Wire.endTransmission();

    if (str.length() == 0) return;

    String lower = str;
    lower.toLowerCase();

    auto push_key = [](uint32_t key) {
        uint8_t next_head = (kbd_fifo_head + 1) % sizeof(kbd_fifo);
        if (next_head != kbd_fifo_tail) {
            kbd_fifo[kbd_fifo_head] = key;
            kbd_fifo_head = next_head;
        }
    };

    if (lower == "enter" || lower == "return" || str == "\r" || str == "\n") {
        push_key(LV_KEY_ENTER);
    } else if (lower == "backspace" || str == "\b" || (uint8_t)str[0] == 127 || (uint8_t)str[0] == 8) {
        push_key(LV_KEY_BACKSPACE);
    } else if (lower == "space") {
        push_key(' ');
    } else if (lower == "tab") {
        push_key('\t');
    } else if (lower == "esc" || lower == "escape") {
        push_key(LV_KEY_ESC);
    } else if (lower == "left") {
        push_key(LV_KEY_LEFT);
    } else if (lower == "right") {
        push_key(LV_KEY_RIGHT);
    } else if (lower == "up") {
        push_key(LV_KEY_UP);
    } else if (lower == "down") {
        push_key(LV_KEY_DOWN);
    } else if (lower == "del" || lower == "delete") {
        push_key(LV_KEY_DEL);
    } else if (lower == "shift" || lower == "ctrl" || lower == "alt" || lower == "fn" || lower == "caps" || lower == "capslock") {
        // Ignore standalone modifier strings
    } else {
        // Ordinary string or single character
        for (size_t i = 0; i < str.length(); ++i) {
            char c = str[i];
            if (c >= 32 && c <= 126) {
                push_key((uint32_t)(uint8_t)c);
            }
        }
    }
}

static void my_keyboard_read(lv_indev_t *indev, lv_indev_data_t *data) {
    if (kbd_key_pressed) {
        data->state = LV_INDEV_STATE_RELEASED;
        kbd_key_pressed = false;
        return;
    }

    poll_physical_keyboard();

    if (kbd_fifo_head != kbd_fifo_tail) {
        uint32_t key = kbd_fifo[kbd_fifo_tail];
        kbd_fifo_tail = (kbd_fifo_tail + 1) % sizeof(kbd_fifo);

        if (key == LV_KEY_ENTER) {
            cmd_buf[cmd_len] = '\0';
            execute_shell_cmd(cmd_buf, false);
            cmd_len = 0;
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        } else if (key == LV_KEY_BACKSPACE) {
            if (cmd_len > 0) {
                cmd_len--;
                cmd_buf[cmd_len] = '\0';
                data->state = LV_INDEV_STATE_PRESSED;
                data->key = LV_KEY_BACKSPACE;
                kbd_key_pressed = true;
                return;
            }
        } else if (key == LV_KEY_ESC) {
            while (cmd_len > 0) {
                cmd_len--;
                lv_textarea_delete_char(term_textarea);
            }
            cmd_buf[0] = '\0';
            data->state = LV_INDEV_STATE_RELEASED;
            return;
        } else if (key >= 32 && key <= 126) {
            if (cmd_len < sizeof(cmd_buf) - 1) {
                cmd_buf[cmd_len++] = (char)key;
                cmd_buf[cmd_len] = '\0';
                data->state = LV_INDEV_STATE_PRESSED;
                data->key = key;
                kbd_key_pressed = true;
                return;
            }
        } else {
            // Other navigation keys (LEFT, RIGHT, etc.)
            data->state = LV_INDEV_STATE_PRESSED;
            data->key = key;
            kbd_key_pressed = true;
            return;
        }
    }

    data->state = LV_INDEV_STATE_RELEASED;
}

// Quick action button callback
static void btn_event_cb(lv_event_t *e) {
    const char *cmd = (const char *)lv_event_get_user_data(e);
    if (cmd) {
        cmd_len = 0;
        execute_shell_cmd(cmd, true);
    }
}

void setup() {
    Serial.begin(115200);
    shell.init(shell_out);

    // 1. Initialize M5Unified
    auto cfg = M5.config();
    cfg.internal_imu = true;
    cfg.internal_mic = false;
    cfg.internal_spk = false;
    M5.begin(cfg);

    // 2. Set Landscape rotation to match physical keyboard (1280 x 720)
    M5.Display.setRotation(3);
    M5.Display.setBrightness(200);

    int disp_w = M5.Display.width();
    int disp_h = M5.Display.height();

    // 3. Initialize Physical Keyboard I2C (STM32 A164 @ 0x6D on GPIO 0/1)
    Wire.begin(KEYBOARD_SDA, KEYBOARD_SCL, 400000U);
    delay(50);

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x10); // Working Mode
    Wire.write(0x02); // Character Mode
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x00); // INT_CFG
    Wire.write(0x07); // Enable Interrupts
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x02); // Clear EVENT_NUM FIFO
    Wire.write(0x00);
    Wire.endTransmission();

    Wire.beginTransmission(TAB5_KEYBOARD_ADDR);
    Wire.write(0x01); // Clear INT_STA
    Wire.write(0x00);
    Wire.endTransmission();

    // 4. Initialize LVGL 9
    lv_init();
    lv_tick_set_cb(my_tick_get);

    // 5. Allocate draw buffers in Octal PSRAM (32 MB)
    size_t buf_size = disp_w * DRAW_BUF_LINES * sizeof(lv_color16_t);
    draw_buf1 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    draw_buf2 = (uint8_t *)heap_caps_malloc(buf_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    lv_display_t *disp = lv_display_create(disp_w, disp_h);
    lv_display_set_color_format(disp, LV_COLOR_FORMAT_RGB565);
    lv_display_set_flush_cb(disp, my_disp_flush);
    lv_display_set_buffers(disp, draw_buf1, draw_buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // 6. Register Touch input device
    lv_indev_t *touch_indev = lv_indev_create();
    lv_indev_set_type(touch_indev, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch_indev, my_touch_read);

    // 7. Register Keyboard input device
    lv_indev_t *kbd_indev = lv_indev_create();
    lv_indev_set_type(kbd_indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(kbd_indev, my_keyboard_read);

    // 8. Build MaiaShell UI - Classic VTX Terminal Theme (Pure Black + Phosphor Green)
    lv_obj_t *scr = lv_screen_active();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0x000000), 0); // Pure Black screen background

    // Title Bar (White background with blue text)
    lv_obj_t *title_bar = lv_obj_create(scr);
    lv_obj_set_size(title_bar, disp_w, 46);
    lv_obj_set_pos(title_bar, 0, 0);
    lv_obj_set_style_bg_color(title_bar, lv_color_hex(0xFFFFFF), 0); // White background
    lv_obj_set_style_bg_opa(title_bar, LV_OPA_COVER, 0);
    lv_obj_set_style_border_side(title_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_color(title_bar, lv_color_hex(0x0047AB), 0); // Cobalt blue border
    lv_obj_set_style_border_width(title_bar, 2, 0);
    lv_obj_set_style_radius(title_bar, 0, 0);
    lv_obj_set_style_pad_all(title_bar, 8, 0);

    lv_obj_t *title_label = lv_label_create(title_bar);
    lv_label_set_text(title_label, "MaiaRTOS VTX Terminal  |  M5Stack Tab5 (ESP32-P4 @ 400MHz)");
    lv_obj_set_style_text_color(title_label, lv_color_hex(0x003399), 0); // Deep royal blue
    lv_obj_set_style_text_font(title_label, &lv_font_montserrat_20, 0);
    lv_obj_align(title_label, LV_ALIGN_LEFT_MID, 15, 0);

    status_label = lv_label_create(title_bar);
    char stat_str[64];
    snprintf(stat_str, sizeof(stat_str), "PSRAM: 32MB | ST7121 1280x720");
    lv_label_set_text(status_label, stat_str);
    lv_obj_set_style_text_color(status_label, lv_color_hex(0x0055D4), 0); // Vibrant blue
    lv_obj_set_style_text_font(status_label, &lv_font_montserrat_18, 0);
    lv_obj_align(status_label, LV_ALIGN_RIGHT_MID, -15, 0);

    // Touch Action Buttons Bar (Styled with Electric Cobalt Blue and White text)
    const char *btn_labels[] = {"[ HELP ]", "[ PS ]", "[ MEM ]", "[ UNAME ]", "[ CLEAR ]"};
    const char *btn_cmds[]   = {"help", "ps", "mem", "uname", "clear"};
    int btn_w = 136;
    int btn_h = 44;
    int btn_spacing = 16;
    int start_x = 24;

    for (int i = 0; i < 5; ++i) {
        lv_obj_t *btn = lv_button_create(scr);
        lv_obj_set_size(btn, btn_w, btn_h);
        lv_obj_set_pos(btn, start_x + i * (btn_w + btn_spacing), 54);
        
        // Solid Royal Blue background (#1D4ED8) with Sky Blue border (#38BDF8)
        lv_obj_set_style_bg_color(btn, lv_color_hex(0x1D4ED8), 0);
        lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, 0);
        lv_obj_set_style_border_color(btn, lv_color_hex(0x38BDF8), 0);
        lv_obj_set_style_border_width(btn, 2, 0);
        lv_obj_set_style_radius(btn, 6, 0);
        lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_CLICKED, (void *)btn_cmds[i]);

        lv_obj_t *lbl = lv_label_create(btn);
        lv_label_set_text(lbl, btn_labels[i]);
        lv_obj_set_style_text_color(lbl, lv_color_hex(0xFFFFFF), 0); // Pure White on blue
        lv_obj_set_style_text_font(lbl, &lv_font_montserrat_20, 0);
        lv_obj_center(lbl);
    }

    // Terminal Textarea (Monospace font, Pure White text on Black background)
    term_textarea = lv_textarea_create(scr);
    lv_obj_set_size(term_textarea, disp_w - 48, disp_h - 116);
    lv_obj_set_pos(term_textarea, 24, 106);
    lv_obj_set_style_bg_color(term_textarea, lv_color_hex(0x000000), 0); // Pure Black
    lv_obj_set_style_bg_opa(term_textarea, LV_OPA_COVER, 0);
    lv_obj_set_style_text_color(term_textarea, lv_color_hex(0xFFFFFF), 0); // Pure White
    lv_obj_set_style_text_font(term_textarea, &lv_font_mono_24, 0);       // Monospace terminal font
    lv_obj_set_style_border_color(term_textarea, lv_color_hex(0x334155), 0);
    lv_obj_set_style_border_width(term_textarea, 2, 0);
    lv_obj_set_style_radius(term_textarea, 6, 0);
    lv_obj_set_style_pad_all(term_textarea, 14, 0);

    lv_textarea_set_text(term_textarea,
        "----------------------------------------------------------------------\n"
        "           MaiaRTOS Kernel VTX Terminal on M5Stack Tab5               \n"
        "     Backend: M5GFX + M5Unified  |  Graphics Subsystem: LVGL 9        \n"
        "----------------------------------------------------------------------\n"
        "[BOOT] ESP32-P4 Dual-Core RISC-V @ 400 MHz active\n"
        "[BOOT] Octal PSRAM 32MB active with Hardware 2D-DMA\n"
        "[BOOT] Display ST7121 1280x720 VTX Classic Mode (Green Phosphor)\n"
        "[BOOT] Physical keyboard STM32 A164 @ 0x6D active\n"
        "[BOOT] Capacitive multi-touch active\n\n"
        "Type commands on keyboard (help, ps, mem, uname, clear) or tap buttons:\n"
        "maia$ ");

    // Focus keyboard to terminal textarea
    lv_group_t *g = lv_group_create();
    lv_group_add_obj(g, term_textarea);
    lv_indev_set_group(kbd_indev, g);
}

void loop() {
    lv_timer_handler();
    delay(5);
}
