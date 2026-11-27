#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/version.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia_shell/shell_engine.h>
#include <maia_shell/maia_shell.hpp>
#if defined(MAIA_NATIVE_LCD_DISPLAY)
#include <maia/services/native_display.h>
#endif

static int32_t s_pending_module_keys[2];
static size_t s_pending_module_key_count;

static int32_t shell_module_read_key(void) {
    if (s_pending_module_key_count > 0U) {
        int32_t key = s_pending_module_keys[0];
        if (s_pending_module_key_count > 1U) {
            s_pending_module_keys[0] = s_pending_module_keys[1];
        }
        --s_pending_module_key_count;
        return key;
    }

    int32_t key = maia_tab5_keyboard_getc_nonblocking();
    if (key != 27) return key;

    int32_t second = maia_tab5_keyboard_getc_nonblocking();
    if (second != '[') {
        if (second >= 0) {
            s_pending_module_keys[0] = second;
            s_pending_module_key_count = 1U;
        }
        return 27;
    }

    int32_t third = maia_tab5_keyboard_getc_nonblocking();
    if (third == 'A') return 0x80;
    if (third == 'B') return 0x81;
    if (third == 'D') return 0x82;
    if (third == 'C') return 0x83;

    s_pending_module_keys[0] = '[';
    s_pending_module_key_count = 1U;
    if (third >= 0) {
        s_pending_module_keys[1] = third;
        s_pending_module_key_count = 2U;
    }
    return 27;
}

static maia_input_api_t s_shell_input_api = {
    .get_touch = [](int32_t *x, int32_t *y, int32_t *pressed) -> int32_t {
        return maia_tab5_touch_get(x, y, pressed);
    },
    .read_key = shell_module_read_key
};

extern "C" {

void *memset(void *s, int c, size_t n) {
    unsigned char *p = (unsigned char *)s;
    while (n--) *p++ = (unsigned char)c;
    return s;
}

void *memcpy(void *dst, const void *src, size_t n) {
    unsigned char *d = (unsigned char *)dst;
    const unsigned char *s = (const unsigned char *)src;
    while (n--) *d++ = *s++;
    return dst;
}

size_t strlen(const char *s) {
    size_t n = 0;
    if (!s) return 0;
    while (s[n] != '\0') n++;
    return n;
}

int strcmp(const char *s1, const char *s2) {
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (*s1 && (*s1 == *s2)) { s1++; s2++; }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

int strncmp(const char *s1, const char *s2, size_t n) {
    if (n == 0) return 0;
    if (!s1 && !s2) return 0;
    if (!s1) return -1;
    if (!s2) return 1;
    while (--n && *s1 && (*s1 == *s2)) { s1++; s2++; }
    return (int)(unsigned char)*s1 - (int)(unsigned char)*s2;
}

char *strncpy(char *dst, const char *src, size_t n) {
    if (!dst || n == 0) return dst;
    size_t i = 0;
    if (src) {
        for (; i < n && src[i] != '\0'; i++) dst[i] = src[i];
    }
    for (; i < n; i++) dst[i] = '\0';
    return dst;
}

char *strncat(char *dst, const char *src, size_t n) {
    if (!dst || !src || n == 0) return dst;
    size_t dlen = strlen(dst);
    size_t i = 0;
    for (; i < n && src[i] != '\0'; i++) dst[dlen + i] = src[i];
    dst[dlen + i] = '\0';
    return dst;
}

char *strchr(const char *s, int c) {
    if (!s) return nullptr;
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == '\0') ? (char *)s : nullptr;
}

char *strrchr(const char *s, int c) {
    if (!s) return nullptr;
    const char *last = nullptr;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    return (c == '\0') ? (char *)s : (char *)last;
}

static MaiaShell s_shell;
static char s_line_buf[MSH_LINE_LEN];
static size_t s_line_len = 0;

static void shell_console_out(const char *str) {
    maia_console_puts(str);
}

void maia_shell_engine_init(void) {
    s_pending_module_key_count = 0U;
    s_shell.init(shell_console_out);
    MaiaShell::set_input_driver(&s_shell_input_api);
#if defined(MAIA_NATIVE_LCD_DISPLAY)
    MaiaShell::set_display_driver(maia_native_display_api());
    maia_console_puts("[DISPLAY] Native module graphics integrated with LCD service\n");
#else
    MaiaShell::set_display_driver(nullptr);
    maia_console_puts("[DISPLAY] Native module graphics unavailable (framebuffer ownership not integrated)\n");
#endif
    s_line_len = 0;
    s_line_buf[0] = '\0';
}

void maia_shell_engine_print_banner(void) {
    maia_console_puts("\n========================================================\n");
    maia_console_puts(" MaiaRTOS v0.1.0 + MaiaShell Native Execution Engine\n");
    maia_console_puts(" Target : M5Stack Tab5 (ESP32-P4 rev v1.3, RV32IMAFC)\n");
    maia_console_puts(" Runtime: Clean-Room Bare-Metal (Zero FreeRTOS/Zero IDF)\n");
    maia_console_puts(" Features: expr, $(expr), loops (while/until/for), if/else,\n");
    maia_console_puts("           variables, arithmetic, RTOS kernel introspection\n");
    maia_console_puts(" Type 'help' for full command summary.\n");
    maia_console_puts("========================================================\n");
}

void maia_shell_engine_print_prompt(void) {
    maia_console_puts("msh> ");
}

int maia_shell_engine_eval_line(const char *line) {
    if (!line) return -1;
    return s_shell.execute(line);
}

int maia_shell_engine_feed_char(char ch) {
    if (ch == '\r' || ch == '\n') {
        maia_console_puts("\n");
        s_line_buf[s_line_len] = '\0';
        int rc = 0;
        if (s_line_len > 0) {
            rc = s_shell.execute(s_line_buf);
        }
        s_line_len = 0;
        s_line_buf[0] = '\0';
        maia_shell_engine_print_prompt();
        return rc;
    }
    if (ch == '\b' || (unsigned char)ch == 0x7FU) {
        if (s_line_len > 0) {
            s_line_len--;
            s_line_buf[s_line_len] = '\0';
            maia_console_puts("\b \b");
        }
        return 0;
    }
    if ((unsigned char)ch >= 0x20U && (unsigned char)ch < 0x7FU) {
        if (s_line_len + 1 < sizeof(s_line_buf)) {
            s_line_buf[s_line_len++] = ch;
            s_line_buf[s_line_len] = '\0';
            maia_console_putc(ch);
        }
    }
    return 0;
}

} // extern "C"
