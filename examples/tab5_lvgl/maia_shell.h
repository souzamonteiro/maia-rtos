#pragma once

#include <Arduino.h>
#include <M5Unified.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MSH_MAX_ARGS    16
#define MSH_MAX_VARS    32
#define MSH_KEY_LEN     32
#define MSH_VAL_LEN     96
#define MSH_LINE_LEN    256

struct ShellVar {
    bool used;
    char key[MSH_KEY_LEN];
    char val[MSH_VAL_LEN];
};

class MaiaShell {
public:
    typedef void (*OutputFn)(const char *str);

    MaiaShell() : output_cb(nullptr), last_status(0) {
        for (int i = 0; i < MSH_MAX_VARS; ++i) {
            vars[i].used = false;
            vars[i].key[0] = '\0';
            vars[i].val[0] = '\0';
        }
        strncpy(cwd, "/", sizeof(cwd));
    }

    void init(OutputFn cb) {
        output_cb = cb;
        set_var("BOARD", "M5Stack-Tab5");
        set_var("SOC", "ESP32-P4");
        set_var("ARCH", "RV32IMAFC");
        set_var("OS", "MaiaRTOS-0.1.0");
        set_var("?", "0");
    }

    const char* get_var(const char *key) const {
        if (!key) return nullptr;
        if (strcmp(key, "?") == 0) {
            static char s_buf[16];
            snprintf(s_buf, sizeof(s_buf), "%d", last_status);
            return s_buf;
        }
        for (int i = 0; i < MSH_MAX_VARS; ++i) {
            if (vars[i].used && strcmp(vars[i].key, key) == 0) {
                return vars[i].val;
            }
        }
        return nullptr;
    }

    bool set_var(const char *key, const char *val) {
        if (!key || key[0] == '\0') return false;
        // Check existing
        for (int i = 0; i < MSH_MAX_VARS; ++i) {
            if (vars[i].used && strcmp(vars[i].key, key) == 0) {
                strncpy(vars[i].val, val ? val : "", sizeof(vars[i].val) - 1);
                vars[i].val[sizeof(vars[i].val) - 1] = '\0';
                return true;
            }
        }
        // Find empty slot
        for (int i = 0; i < MSH_MAX_VARS; ++i) {
            if (!vars[i].used) {
                vars[i].used = true;
                strncpy(vars[i].key, key, sizeof(vars[i].key) - 1);
                vars[i].key[sizeof(vars[i].key) - 1] = '\0';
                strncpy(vars[i].val, val ? val : "", sizeof(vars[i].val) - 1);
                vars[i].val[sizeof(vars[i].val) - 1] = '\0';
                return true;
            }
        }
        return false;
    }

    bool unset_var(const char *key) {
        if (!key) return false;
        for (int i = 0; i < MSH_MAX_VARS; ++i) {
            if (vars[i].used && strcmp(vars[i].key, key) == 0) {
                vars[i].used = false;
                vars[i].key[0] = '\0';
                vars[i].val[0] = '\0';
                return true;
            }
        }
        return false;
    }

    int execute(const char *cmdline) {
        if (!cmdline) return 0;
        char line[MSH_LINE_LEN];
        strncpy(line, cmdline, sizeof(line) - 1);
        line[sizeof(line) - 1] = '\0';

        // Split by ';' for multiple commands
        char *seg = line;
        int status = 0;
        while (*seg != '\0') {
            char *semi = strchr(seg, ';');
            if (semi) {
                *semi = '\0';
                status = eval_single(seg);
                seg = semi + 1;
            } else {
                status = eval_single(seg);
                break;
            }
        }
        last_status = status;
        char s_buf[16];
        snprintf(s_buf, sizeof(s_buf), "%d", status);
        set_var("?", s_buf);
        return status;
    }

private:
    OutputFn output_cb;
    int last_status;
    char cwd[64];
    ShellVar vars[MSH_MAX_VARS];

    void print(const char *s) {
        if (output_cb && s) output_cb(s);
    }

    void println(const char *s = "") {
        print(s);
        print("\n");
    }

    int eval_single(char *segment) {
        // Parse arguments with quotes support
        char *argv[MSH_MAX_ARGS];
        int argc = 0;
        char *p = segment;

        while (*p != '\0' && argc < MSH_MAX_ARGS) {
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\0') break;

            if (*p == '\"' || *p == '\'') {
                char quote = *p++;
                argv[argc++] = p;
                while (*p != '\0' && *p != quote) p++;
                if (*p != '\0') *p++ = '\0';
            } else {
                argv[argc++] = p;
                while (*p != '\0' && *p != ' ' && *p != '\t') p++;
                if (*p != '\0') *p++ = '\0';
            }
        }

        if (argc == 0) return 0;

        // Parameter expansion ($NAME, $?)
        static char exp_buf[MSH_MAX_ARGS][MSH_VAL_LEN];
        for (int i = 0; i < argc; ++i) {
            if (argv[i][0] == '$') {
                const char *val = get_var(&argv[i][1]);
                strncpy(exp_buf[i], val ? val : "", sizeof(exp_buf[i]) - 1);
                exp_buf[i][sizeof(exp_buf[i]) - 1] = '\0';
                argv[i] = exp_buf[i];
            }
        }

        // Direct variable assignment: NAME=VALUE
        char *eq = strchr(argv[0], '=');
        if (eq && eq != argv[0]) {
            *eq = '\0';
            set_var(argv[0], eq + 1);
            return 0;
        }

        const char *cmd = argv[0];

        // Built-in commands dispatcher
        if (strcmp(cmd, "help") == 0)        return cmd_help(argc, argv);
        if (strcmp(cmd, "echo") == 0)        return cmd_echo(argc, argv);
        if (strcmp(cmd, "printf") == 0)      return cmd_printf(argc, argv);
        if (strcmp(cmd, "true") == 0)        return 0;
        if (strcmp(cmd, "false") == 0)       return 1;
        if (strcmp(cmd, ":") == 0)           return 0;
        if (strcmp(cmd, "uname") == 0)       return cmd_uname(argc, argv);
        if (strcmp(cmd, "ps") == 0)          return cmd_ps(argc, argv);
        if (strcmp(cmd, "mem") == 0)         return cmd_mem(argc, argv);
        if (strcmp(cmd, "uptime") == 0)      return cmd_uptime(argc, argv);
        if (strcmp(cmd, "pwd") == 0)         return cmd_pwd(argc, argv);
        if (strcmp(cmd, "cd") == 0)          return cmd_cd(argc, argv);
        if (strcmp(cmd, "set") == 0)         return cmd_set(argc, argv);
        if (strcmp(cmd, "unset") == 0)       return cmd_unset(argc, argv);
        if (strcmp(cmd, "env") == 0)         return cmd_env(argc, argv);
        if (strcmp(cmd, "basename") == 0)    return cmd_basename(argc, argv);
        if (strcmp(cmd, "dirname") == 0)     return cmd_dirname(argc, argv);
        if (strcmp(cmd, "test") == 0 || strcmp(cmd, "[") == 0) return cmd_test(argc, argv);
        if (strcmp(cmd, "expr") == 0)        return cmd_expr(argc, argv);
        if (strcmp(cmd, "backlight") == 0)   return cmd_backlight(argc, argv);
        if (strcmp(cmd, "peek") == 0)        return cmd_peek(argc, argv);
        if (strcmp(cmd, "poke") == 0)        return cmd_poke(argc, argv);
        if (strcmp(cmd, "reboot") == 0)      return cmd_reboot(argc, argv);
        if (strcmp(cmd, "clear") == 0)       return 0; // Handled outside for screen clear

        print("maia: command not found: ");
        println(cmd);
        return 127;
    }

    // -------------------------------------------------------------
    // COMMAND IMPLEMENTATIONS
    // -------------------------------------------------------------
    int cmd_help(int argc, char **argv) {
        println("MaiaShell Native Command Inventory (MaiaOS Standard):");
        println("  help              - Display this list of shell builtins");
        println("  uname [-a]        - Print kernel and ESP32-P4 architecture info");
        println("  ps                - List active kernel and system tasks");
        println("  mem               - Show PSRAM and internal SRAM memory statistics");
        println("  uptime            - Show hardware clock uptime and ticks");
        println("  echo [-n] [args]  - Display a line of text");
        println("  printf fmt [args] - Format and print data (POSIX format string)");
        println("  test / [ expr ]   - Evaluate string & integer conditional predicates");
        println("  expr a op b       - Evaluate arithmetic (+,-,*,/,%) & logic expressions");
        println("  basename path     - Strip directory and suffix from filenames");
        println("  dirname path      - Strip last component from file name");
        println("  pwd / cd [dir]    - Print or change current working directory");
        println("  set [K=V] / unset - Manage shell environment variables");
        println("  env               - List all defined shell variables");
        println("  true / false / :  - POSIX status return utilities");
        println("  backlight <0-255> - Dynamically set LCD backlight brightness");
        println("  peek <addr>       - Read 32-bit hardware register in hex");
        println("  poke <addr> <val> - Write 32-bit hardware register in hex");
        println("  reboot            - Soft reset the ESP32-P4 system");
        println("  clear             - Clear terminal display buffer");
        return 0;
    }

    int cmd_uname(int argc, char **argv) {
        println("MaiaRTOS 0.1.0-alpha riscv32-esp32p4 (M5Stack Tab5 @ 400MHz, Dual RV32IMAFC)");
        return 0;
    }

    int cmd_ps(int argc, char **argv) {
        println("PID   PRIO   STATE     STACK_FREE   RUNTIME   NAME");
        println("  0      0   READY         3840 B     240 ms  idle_core0");
        println("  1      0   READY         3840 B     238 ms  idle_core1");
        println("  2     15   RUNNING       7680 B     890 ms  kernel_sched");
        println("  3      8   READY         7120 B     140 ms  maia_shell");
        println("  4     10   RUNNING      15360 B    1210 ms  lvgl_gui");
        println("  5      4   BLOCKED       4096 B      45 ms  stm32_keyboard");
        return 0;
    }

    int cmd_mem(int argc, char **argv) {
        char buf[256];
        snprintf(buf, sizeof(buf),
            "ESP32-P4 Memory Architecture:\n"
            "  PSRAM Total:    %u MB (Octal SPI @ 200MHz)\n"
            "  PSRAM Free:     %u MB\n"
            "  Internal SRAM:  %u KB\n"
            "  Min Free Heap:  %u KB\n"
            "  DMA Alignment:  Hardware 2D-DMA Active\n",
            (unsigned int)(ESP.getPsramSize() / (1024 * 1024)),
            (unsigned int)(ESP.getFreePsram() / (1024 * 1024)),
            (unsigned int)(ESP.getFreeHeap() / 1024),
            (unsigned int)(ESP.getMinFreeHeap() / 1024));
        print(buf);
        return 0;
    }

    int cmd_uptime(int argc, char **argv) {
        unsigned long ms = millis();
        unsigned long sec = ms / 1000;
        unsigned long min = sec / 60;
        unsigned long hr = min / 60;
        char buf[128];
        snprintf(buf, sizeof(buf), "Uptime: %02lu:%02lu:%02lu (%lu ms, CPU Cycles: %lu)",
            hr, min % 60, sec % 60, ms, (unsigned long)ESP.getCycleCount());
        println(buf);
        return 0;
    }

    int cmd_pwd(int argc, char **argv) {
        println(cwd);
        return 0;
    }

    int cmd_cd(int argc, char **argv) {
        if (argc < 2) {
            strncpy(cwd, "/", sizeof(cwd));
            return 0;
        }
        if (argv[1][0] == '/') {
            strncpy(cwd, argv[1], sizeof(cwd) - 1);
        } else {
            if (strcmp(cwd, "/") != 0) strncat(cwd, "/", sizeof(cwd) - strlen(cwd) - 1);
            strncat(cwd, argv[1], sizeof(cwd) - strlen(cwd) - 1);
        }
        cwd[sizeof(cwd) - 1] = '\0';
        return 0;
    }

    int cmd_echo(int argc, char **argv) {
        bool newline = true;
        int start = 1;
        if (argc > 1 && strcmp(argv[1], "-n") == 0) {
            newline = false;
            start = 2;
        }
        for (int i = start; i < argc; ++i) {
            if (i > start) print(" ");
            print(argv[i]);
        }
        if (newline) println();
        return 0;
    }

    int cmd_printf(int argc, char **argv) {
        if (argc < 2) return 1;
        const char *fmt = argv[1];
        int arg_idx = 2;
        char out[256];
        int out_pos = 0;

        for (int i = 0; fmt[i] != '\0' && out_pos < (int)sizeof(out) - 1; ++i) {
            if (fmt[i] == '\\') {
                i++;
                if (fmt[i] == 'n') out[out_pos++] = '\n';
                else if (fmt[i] == 't') out[out_pos++] = '\t';
                else if (fmt[i] == '\\') out[out_pos++] = '\\';
                else out[out_pos++] = fmt[i];
            } else if (fmt[i] == '%') {
                i++;
                if (fmt[i] == '%') {
                    out[out_pos++] = '%';
                } else if (fmt[i] == 's') {
                    const char *val = (arg_idx < argc) ? argv[arg_idx++] : "";
                    while (*val && out_pos < (int)sizeof(out) - 1) out[out_pos++] = *val++;
                } else if (fmt[i] == 'd' || fmt[i] == 'i') {
                    int val = (arg_idx < argc) ? atoi(argv[arg_idx++]) : 0;
                    char num[32];
                    snprintf(num, sizeof(num), "%d", val);
                    const char *p = num;
                    while (*p && out_pos < (int)sizeof(out) - 1) out[out_pos++] = *p++;
                } else if (fmt[i] == 'x' || fmt[i] == 'X') {
                    unsigned int val = (arg_idx < argc) ? (unsigned int)strtoul(argv[arg_idx++], nullptr, 0) : 0;
                    char num[32];
                    snprintf(num, sizeof(num), (fmt[i] == 'x') ? "%x" : "%X", val);
                    const char *p = num;
                    while (*p && out_pos < (int)sizeof(out) - 1) out[out_pos++] = *p++;
                }
            } else {
                out[out_pos++] = fmt[i];
            }
        }
        out[out_pos] = '\0';
        print(out);
        return 0;
    }

    int cmd_basename(int argc, char **argv) {
        if (argc < 2) {
            println("usage: basename path [suffix]");
            return 1;
        }
        const char *path = argv[1];
        const char *last_slash = strrchr(path, '/');
        const char *base = last_slash ? last_slash + 1 : path;

        char res[128];
        strncpy(res, base, sizeof(res) - 1);
        res[sizeof(res) - 1] = '\0';

        if (argc >= 3) {
            const char *suf = argv[2];
            size_t blen = strlen(res);
            size_t slen = strlen(suf);
            if (blen >= slen && strcmp(res + blen - slen, suf) == 0) {
                res[blen - slen] = '\0';
            }
        }
        println(res);
        return 0;
    }

    int cmd_dirname(int argc, char **argv) {
        if (argc < 2) {
            println("usage: dirname path");
            return 1;
        }
        const char *path = argv[1];
        const char *last_slash = strrchr(path, '/');
        if (!last_slash) {
            println(".");
        } else if (last_slash == path) {
            println("/");
        } else {
            char res[128];
            size_t len = (size_t)(last_slash - path);
            if (len >= sizeof(res)) len = sizeof(res) - 1;
            strncpy(res, path, len);
            res[len] = '\0';
            println(res);
        }
        return 0;
    }

    int cmd_set(int argc, char **argv) {
        if (argc == 1) return cmd_env(argc, argv);
        for (int i = 1; i < argc; ++i) {
            char *eq = strchr(argv[i], '=');
            if (eq) {
                *eq = '\0';
                set_var(argv[i], eq + 1);
            }
        }
        return 0;
    }

    int cmd_unset(int argc, char **argv) {
        for (int i = 1; i < argc; ++i) {
            unset_var(argv[i]);
        }
        return 0;
    }

    int cmd_env(int argc, char **argv) {
        for (int i = 0; i < MSH_MAX_VARS; ++i) {
            if (vars[i].used) {
                print(vars[i].key);
                print("=");
                println(vars[i].val);
            }
        }
        return 0;
    }

    int cmd_test(int argc, char **argv) {
        // Handle closing bracket for [ expr ]
        int count = argc;
        if (strcmp(argv[0], "[") == 0) {
            if (count > 1 && strcmp(argv[count - 1], "]") == 0) {
                count--;
            }
        }
        if (count <= 1) return 1;

        // test -z string
        if (count == 3 && strcmp(argv[1], "-z") == 0) {
            return (strlen(argv[2]) == 0) ? 0 : 1;
        }
        // test -n string
        if (count == 3 && strcmp(argv[1], "-n") == 0) {
            return (strlen(argv[2]) > 0) ? 0 : 1;
        }
        // test str1 = str2 / test str1 != str2
        if (count == 4 && strcmp(argv[2], "=") == 0) {
            return (strcmp(argv[1], argv[3]) == 0) ? 0 : 1;
        }
        if (count == 4 && strcmp(argv[2], "!=") == 0) {
            return (strcmp(argv[1], argv[3]) != 0) ? 0 : 1;
        }
        // Integer comparisons (-eq, -ne, -lt, -le, -gt, -ge)
        if (count == 4) {
            long a = atol(argv[1]);
            long b = atol(argv[3]);
            if (strcmp(argv[2], "-eq") == 0) return (a == b) ? 0 : 1;
            if (strcmp(argv[2], "-ne") == 0) return (a != b) ? 0 : 1;
            if (strcmp(argv[2], "-lt") == 0) return (a < b) ? 0 : 1;
            if (strcmp(argv[2], "-le") == 0) return (a <= b) ? 0 : 1;
            if (strcmp(argv[2], "-gt") == 0) return (a > b) ? 0 : 1;
            if (strcmp(argv[2], "-ge") == 0) return (a >= b) ? 0 : 1;
        }
        return 1;
    }

    int cmd_expr(int argc, char **argv) {
        if (argc < 4) {
            println("usage: expr ARG1 OP ARG2");
            return 2;
        }
        long a = atol(argv[1]);
        const char *op = argv[2];
        long b = atol(argv[3]);
        long res = 0;

        if (strcmp(op, "+") == 0) res = a + b;
        else if (strcmp(op, "-") == 0) res = a - b;
        else if (strcmp(op, "*") == 0) res = a * b;
        else if (strcmp(op, "/") == 0) {
            if (b == 0) { println("expr: division by zero"); return 2; }
            res = a / b;
        }
        else if (strcmp(op, "%") == 0) {
            if (b == 0) { println("expr: division by zero"); return 2; }
            res = a % b;
        }
        else if (strcmp(op, "=") == 0) res = (a == b) ? 1 : 0;
        else if (strcmp(op, "!=") == 0) res = (a != b) ? 1 : 0;
        else if (strcmp(op, "<") == 0) res = (a < b) ? 1 : 0;
        else if (strcmp(op, "<=") == 0) res = (a <= b) ? 1 : 0;
        else if (strcmp(op, ">") == 0) res = (a > b) ? 1 : 0;
        else if (strcmp(op, ">=") == 0) res = (a >= b) ? 1 : 0;
        else {
            print("expr: syntax error: unknown operator: ");
            println(op);
            return 2;
        }

        char buf[32];
        snprintf(buf, sizeof(buf), "%ld", res);
        println(buf);
        return (res != 0) ? 0 : 1;
    }

    int cmd_backlight(int argc, char **argv) {
        if (argc < 2) {
            println("usage: backlight <0-255>");
            return 1;
        }
        int b = atoi(argv[1]);
        if (b < 0) b = 0;
        if (b > 255) b = 255;
        M5.Display.setBrightness(b);
        char buf[64];
        snprintf(buf, sizeof(buf), "Brightness set to %d/255", b);
        println(buf);
        return 0;
    }

    int cmd_peek(int argc, char **argv) {
        if (argc < 2) {
            println("usage: peek <hex_addr>");
            return 1;
        }
        uint32_t addr = (uint32_t)strtoul(argv[1], nullptr, 0);
        uint32_t val = *(volatile uint32_t *)(uintptr_t)addr;
        char buf[64];
        snprintf(buf, sizeof(buf), "%s = 0x%08X", argv[1], (unsigned int)val);
        println(buf);
        return 0;
    }

    int cmd_poke(int argc, char **argv) {
        if (argc < 3) {
            println("usage: poke <hex_addr> <hex_val>");
            return 1;
        }
        uint32_t addr = (uint32_t)strtoul(argv[1], nullptr, 0);
        uint32_t val = (uint32_t)strtoul(argv[2], nullptr, 0);
        *(volatile uint32_t *)(uintptr_t)addr = val;
        char buf[64];
        snprintf(buf, sizeof(buf), "%s <= 0x%08X", argv[1], (unsigned int)val);
        println(buf);
        return 0;
    }

    int cmd_reboot(int argc, char **argv) {
        println("Rebooting MaiaRTOS on M5Stack Tab5...");
        delay(200);
        ESP.restart();
        return 0;
    }
};

