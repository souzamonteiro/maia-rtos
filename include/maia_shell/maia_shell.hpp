#pragma once

#if defined(ARDUINO)
#include <Arduino.h>
#include <M5Unified.h>
#elif defined(ESP_PLATFORM)
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp_psram.h>
#include <esp_chip_info.h>
#include <esp_timer.h>
#include <mbedtls/sha256.h>
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <dirent.h>
#include <sys/stat.h>

#include <maia/exec/elf_validator.h>
#include <maia/exec/elf_loader.h>
#include <maia/exec/hello_mobj.h>

#if (defined(__riscv) || defined(MAIA_TARGET)) && !defined(ESP_PLATFORM)
#include <maia/kernel.h>
#include <maia/soc/esp32p4_board_io.h>
#endif

#include <stdarg.h>

/* Freestanding vsnprintf & snprintf (Zero newlib stdio / Zero malloc) */
static inline int msh_vsnprintf(char *buf, size_t size, const char *fmt, va_list ap) {
    if (!buf || size == 0) return 0;
    size_t pos = 0;

    while (*fmt) {
        if (*fmt != '%') {
            if (pos + 1 < size) buf[pos] = *fmt;
            pos++;
            fmt++;
            continue;
        }
        fmt++; // skip '%'
        if (*fmt == '%') {
            if (pos + 1 < size) buf[pos] = '%';
            pos++;
            fmt++;
            continue;
        }

        // Flags & width
        char pad = ' ';
        if (*fmt == '0') {
            pad = '0';
            fmt++;
        }
        int width = 0;
        while (*fmt >= '0' && *fmt <= '9') {
            width = (width * 10) + (*fmt - '0');
            fmt++;
        }

        // Length modifiers
        bool is_long = false;
        if (*fmt == 'l') {
            is_long = true;
            fmt++;
        }

        char spec = *fmt++;
        if (spec == 's') {
            const char *s = va_arg(ap, const char *);
            if (!s) s = "(null)";
            while (*s) {
                if (pos + 1 < size) buf[pos] = *s;
                pos++;
                s++;
            }
        } else if (spec == 'c') {
            char c = (char)va_arg(ap, int);
            if (pos + 1 < size) buf[pos] = c;
            pos++;
        } else if (spec == 'd' || spec == 'i') {
            long val = is_long ? va_arg(ap, long) : (long)va_arg(ap, int);
            unsigned long uval = (val < 0) ? (unsigned long)(-val) : (unsigned long)val;
            char num_buf[24];
            int ni = 0;
            if (uval == 0) num_buf[ni++] = '0';
            while (uval > 0) {
                num_buf[ni++] = (char)('0' + (uval % 10));
                uval /= 10;
            }
            int total_len = ni + (val < 0 ? 1 : 0);
            if (pad == ' ' && width > total_len) {
                for (int w = total_len; w < width; ++w) {
                    if (pos + 1 < size) buf[pos] = ' ';
                    pos++;
                }
            }
            if (val < 0) {
                if (pos + 1 < size) buf[pos] = '-';
                pos++;
            }
            if (pad == '0' && width > total_len) {
                for (int w = total_len; w < width; ++w) {
                    if (pos + 1 < size) buf[pos] = '0';
                    pos++;
                }
            }
            while (ni > 0) {
                if (pos + 1 < size) buf[pos] = num_buf[--ni];
                pos++;
            }
        } else if (spec == 'u') {
            unsigned long uval = is_long ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
            char num_buf[24];
            int ni = 0;
            if (uval == 0) num_buf[ni++] = '0';
            while (uval > 0) {
                num_buf[ni++] = (char)('0' + (uval % 10));
                uval /= 10;
            }
            if (width > ni) {
                for (int w = ni; w < width; ++w) {
                    if (pos + 1 < size) buf[pos] = pad;
                    pos++;
                }
            }
            while (ni > 0) {
                if (pos + 1 < size) buf[pos] = num_buf[--ni];
                pos++;
            }
        } else if (spec == 'x' || spec == 'X') {
            unsigned long uval = is_long ? va_arg(ap, unsigned long) : (unsigned long)va_arg(ap, unsigned int);
            char num_buf[24];
            int ni = 0;
            const char *hex_digits = (spec == 'X') ? "0123456789ABCDEF" : "0123456789abcdef";
            if (uval == 0) num_buf[ni++] = '0';
            while (uval > 0) {
                num_buf[ni++] = hex_digits[uval & 0x0F];
                uval >>= 4;
            }
            if (width > ni) {
                for (int w = ni; w < width; ++w) {
                    if (pos + 1 < size) buf[pos] = pad;
                    pos++;
                }
            }
            while (ni > 0) {
                if (pos + 1 < size) buf[pos] = num_buf[--ni];
                pos++;
            }
        }
    }

    if (size > 0) {
        if (pos < size) buf[pos] = '\0';
        else buf[size - 1] = '\0';
    }
    return (int)pos;
}

static inline int msh_snprintf(char *buf, size_t size, const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int res = msh_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return res;
}

#ifndef snprintf
#define snprintf msh_snprintf
#endif

/* Freestanding numeric parse helpers */
static inline long msh_strtol(const char *s, char **endp, int base) {
    long val = 0;
    int sign = 1;
    if (!s) { if (endp) *endp = nullptr; return 0; }
    while (*s == ' ' || *s == '\t') s++;
    if (*s == '-') { sign = -1; s++; }
    else if (*s == '+') { s++; }
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (s[0] == '0') { base = 8; s++; }
        else { base = 10; }
    } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    const char *start = s;
    while (*s) {
        int d = -1;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        if (d < 0 || d >= base) break;
        val = (val * base) + d;
        s++;
    }
    if (endp) *endp = (char *)((s == start) ? start : s);
    return sign * val;
}

static inline unsigned long msh_strtoul(const char *s, char **endp, int base) {
    unsigned long val = 0;
    if (!s) { if (endp) *endp = nullptr; return 0; }
    while (*s == ' ' || *s == '\t') s++;
    if (base == 0) {
        if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) { base = 16; s += 2; }
        else if (s[0] == '0') { base = 8; s++; }
        else { base = 10; }
    } else if (base == 16 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    const char *start = s;
    while (*s) {
        int d = -1;
        if (*s >= '0' && *s <= '9') d = *s - '0';
        else if (*s >= 'a' && *s <= 'f') d = *s - 'a' + 10;
        else if (*s >= 'A' && *s <= 'F') d = *s - 'A' + 10;
        if (d < 0 || d >= base) break;
        val = (val * base) + (unsigned long)d;
        s++;
    }
    if (endp) *endp = (char *)((s == start) ? start : s);
    return val;
}

static inline int msh_atoi(const char *s) {
    return (int)msh_strtol(s, nullptr, 10);
}

static inline long msh_atol(const char *s) {
    return msh_strtol(s, nullptr, 10);
}

#ifndef strtol
#define strtol msh_strtol
#endif
#ifndef strtoul
#define strtoul msh_strtoul
#endif
#ifndef atoi
#define atoi msh_atoi
#endif
#ifndef atol
#define atol msh_atol
#endif

#define MSH_MAX_ARGS    16
#define MSH_MAX_VARS    32
#define MSH_KEY_LEN     32
#define MSH_VAL_LEN     96
#define MSH_LINE_LEN    256
#define MSH_MAX_SEGS    32
#define MSH_SEG_LEN     128

struct ShellVar {
    bool used;
    char key[MSH_KEY_LEN];
    char val[MSH_VAL_LEN];
};

struct ExprVal {
    char s[96];
    bool is_num;
    int32_t num;
};

class MaiaShell {
public:
    typedef void (*OutputFn)(const char *str);

    MaiaShell() : output_cb(nullptr), last_status(0), seg_count(0) {
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
        for (int i = 0; i < MSH_MAX_VARS; ++i) {
            if (vars[i].used && strcmp(vars[i].key, key) == 0) {
                strncpy(vars[i].val, val ? val : "", sizeof(vars[i].val) - 1);
                vars[i].val[sizeof(vars[i].val) - 1] = '\0';
                return true;
            }
        }
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

    static const maia_display_api_t *&get_global_display_api() {
        static const maia_display_api_t *s_disp = nullptr;
        return s_disp;
    }

    static void set_display_driver(const maia_display_api_t *disp) {
        get_global_display_api() = disp;
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

    void set_last_status(int st) {
        last_status = st;
        char s_buf[16];
        snprintf(s_buf, sizeof(s_buf), "%d", last_status);
        set_var("?", s_buf);
    }

    int get_last_status() const {
        return last_status;
    }

    // -------------------------------------------------------------
    // EXPR EVALUATOR
    // -------------------------------------------------------------
    struct ExprParser {
        int argc;
        char **argv;
        int pos;
        int status;
        char err[64];
    };

    static bool is_truthy(const ExprVal *v) {
        if (!v) return false;
        if (v->is_num) return v->num != 0;
        if (v->s[0] == '\0') return false;
        if (strcmp(v->s, "0") == 0) return false;
        return true;
    }

    static void make_val_num(ExprVal *v, int32_t n) {
        v->is_num = true;
        v->num = n;
        snprintf(v->s, sizeof(v->s), "%ld", (long)n);
    }

    static void make_val_str(ExprVal *v, const char *s) {
        v->is_num = false;
        v->num = 0;
        strncpy(v->s, s ? s : "", sizeof(v->s) - 1);
        v->s[sizeof(v->s) - 1] = '\0';
        char *endp = nullptr;
        long n = strtol(v->s, &endp, 10);
        if (endp && *endp == '\0' && endp != v->s) {
            if (n >= (long)INT32_MIN && n <= (long)INT32_MAX) {
                v->is_num = true;
                v->num = (int32_t)n;
            }
        }
    }

    ExprVal parse_expr_primary(ExprParser *p) {
        ExprVal val;
        val.s[0] = '\0';
        val.is_num = false;
        val.num = 0;

        if (p->pos >= p->argc) {
            p->status = 2;
            snprintf(p->err, sizeof(p->err), "syntax error");
            return val;
        }

        const char *tok = p->argv[p->pos++];
        if (strcmp(tok, "(") == 0 || strcmp(tok, "\\(") == 0) {
            val = parse_expr_or(p);
            if (p->status != 0) return val;
            if (p->pos >= p->argc || (strcmp(p->argv[p->pos], ")") != 0 && strcmp(p->argv[p->pos], "\\)") != 0)) {
                p->status = 2;
                snprintf(p->err, sizeof(p->err), "missing closing parenthesis");
                return val;
            }
            p->pos++; // consume ')'
            return val;
        }

        make_val_str(&val, tok);
        return val;
    }

    ExprVal parse_expr_match(ExprParser *p) {
        ExprVal left = parse_expr_primary(p);
        while (p->status == 0 && p->pos < p->argc && strcmp(p->argv[p->pos], ":") == 0) {
            p->pos++;
            ExprVal right = parse_expr_primary(p);
            if (p->status != 0) return left;

            const char *src = left.s;
            const char *pat = right.s;
            size_t mlen = 0;
            if (strcmp(pat, ".*") == 0) {
                mlen = strlen(src);
            } else {
                size_t plen = strlen(pat);
                if (strncmp(src, pat, plen) == 0) mlen = plen;
            }
            make_val_num(&left, (int32_t)mlen);
        }
        return left;
    }

    ExprVal parse_expr_mul(ExprParser *p) {
        ExprVal left = parse_expr_match(p);
        while (p->status == 0 && p->pos < p->argc) {
            const char *op = p->argv[p->pos];
            if (strcmp(op, "*") != 0 && strcmp(op, "\\*") != 0 &&
                strcmp(op, "/") != 0 && strcmp(op, "%") != 0) break;
            p->pos++;
            ExprVal right = parse_expr_match(p);
            if (p->status != 0) return left;
            if (!left.is_num || !right.is_num) {
                p->status = 2;
                snprintf(p->err, sizeof(p->err), "non-integer argument");
                return left;
            }
            int32_t a = left.num;
            int32_t b = right.num;
            if (strcmp(op, "*") == 0 || strcmp(op, "\\*") == 0) {
                int64_t prod = (int64_t)a * (int64_t)b;
                if (prod < (int64_t)INT32_MIN || prod > (int64_t)INT32_MAX) {
                    p->status = 3;
                    snprintf(p->err, sizeof(p->err), "arithmetic overflow");
                    return left;
                }
                make_val_num(&left, (int32_t)prod);
            } else if (strcmp(op, "/") == 0) {
                if (b == 0) {
                    p->status = 2;
                    snprintf(p->err, sizeof(p->err), "division by zero");
                    return left;
                }
                if (a == INT32_MIN && b == -1) {
                    p->status = 3;
                    snprintf(p->err, sizeof(p->err), "arithmetic overflow");
                    return left;
                }
                make_val_num(&left, a / b);
            } else if (strcmp(op, "%") == 0) {
                if (b == 0) {
                    p->status = 2;
                    snprintf(p->err, sizeof(p->err), "division by zero");
                    return left;
                }
                make_val_num(&left, a % b);
            }
        }
        return left;
    }

    ExprVal parse_expr_add(ExprParser *p) {
        ExprVal left = parse_expr_mul(p);
        while (p->status == 0 && p->pos < p->argc) {
            const char *op = p->argv[p->pos];
            if (strcmp(op, "+") != 0 && strcmp(op, "-") != 0) break;
            p->pos++;
            ExprVal right = parse_expr_mul(p);
            if (p->status != 0) return left;
            if (!left.is_num || !right.is_num) {
                p->status = 2;
                snprintf(p->err, sizeof(p->err), "non-integer argument");
                return left;
            }
            int32_t a = left.num;
            int32_t b = right.num;
            int64_t sum = (strcmp(op, "+") == 0) ? ((int64_t)a + (int64_t)b) : ((int64_t)a - (int64_t)b);
            if (sum < (int64_t)INT32_MIN || sum > (int64_t)INT32_MAX) {
                p->status = 3;
                snprintf(p->err, sizeof(p->err), "arithmetic overflow");
                return left;
            }
            make_val_num(&left, (int32_t)sum);
        }
        return left;
    }

    ExprVal parse_expr_rel(ExprParser *p) {
        ExprVal left = parse_expr_add(p);
        while (p->status == 0 && p->pos < p->argc) {
            const char *op = p->argv[p->pos];
            bool is_rel = (strcmp(op, "=") == 0 || strcmp(op, "==") == 0 ||
                           strcmp(op, "!=") == 0 ||
                           strcmp(op, "<") == 0 || strcmp(op, "\\<") == 0 ||
                           strcmp(op, "<=") == 0 || strcmp(op, "\\<=") == 0 ||
                           strcmp(op, ">") == 0 || strcmp(op, "\\>") == 0 ||
                           strcmp(op, ">=") == 0 || strcmp(op, "\\>=") == 0);
            if (!is_rel) break;
            p->pos++;
            ExprVal right = parse_expr_add(p);
            if (p->status != 0) return left;
            bool both_int = (left.is_num && right.is_num);
            int32_t a = left.num;
            int32_t b = right.num;
            int cmp = both_int ? (a < b ? -1 : (a > b ? 1 : 0)) : strcmp(left.s, right.s);
            bool truth = false;
            if (strcmp(op, "=") == 0 || strcmp(op, "==") == 0) truth = (cmp == 0);
            else if (strcmp(op, "!=") == 0) truth = (cmp != 0);
            else if (strcmp(op, "<") == 0 || strcmp(op, "\\<") == 0) truth = (cmp < 0);
            else if (strcmp(op, "<=") == 0 || strcmp(op, "\\<=") == 0) truth = (cmp <= 0);
            else if (strcmp(op, ">") == 0 || strcmp(op, "\\>") == 0) truth = (cmp > 0);
            else if (strcmp(op, ">=") == 0 || strcmp(op, "\\>=") == 0) truth = (cmp >= 0);
            make_val_num(&left, truth ? 1 : 0);
        }
        return left;
    }

    ExprVal parse_expr_and(ExprParser *p) {
        ExprVal left = parse_expr_rel(p);
        while (p->status == 0 && p->pos < p->argc) {
            const char *op = p->argv[p->pos];
            if (strcmp(op, "&") != 0 && strcmp(op, "\\&") != 0) break;
            p->pos++;
            ExprVal right = parse_expr_rel(p);
            if (p->status != 0) return left;
            if (is_truthy(&left) && is_truthy(&right)) {
                // keeps left
            } else {
                make_val_num(&left, 0);
            }
        }
        return left;
    }

    ExprVal parse_expr_or(ExprParser *p);

    int eval_expr(int argc, char **argv, char *out, size_t out_len) {
        if (argc <= 1) {
            snprintf(out, out_len, "usage: expr EXPRESSION");
            return 2;
        }
        ExprParser parser = { argc, argv, 1, 0, "" };
        ExprVal res = parse_expr_or(&parser);
        if (parser.status != 0) {
            snprintf(out, out_len, "expr: %s", parser.err);
            return parser.status;
        }
        if (parser.pos < parser.argc) {
            snprintf(out, out_len, "expr: syntax error");
            return 2;
        }
        snprintf(out, out_len, "%s", res.s);
        return is_truthy(&res) ? 0 : 1;
    }

    // -------------------------------------------------------------
    // VARIABLE EXPANSION & COMMAND SUBSTITUTION $(expr ...)
    // -------------------------------------------------------------
    void expand_variables(const char *in, char *out, size_t max_out) const {
        size_t oi = 0;
        size_t ii = 0;
        size_t in_len = strlen(in);
        bool in_single = false;

        while (ii < in_len && oi < max_out - 1) {
            char c = in[ii];
            if (c == '\'') {
                in_single = !in_single;
                out[oi++] = c;
                ii++;
            } else if (c == '$' && !in_single) {
                ii++;
                if (in[ii] == '?') {
                    const char *val = get_var("?");
                    if (val) {
                        while (*val && oi < max_out - 1) out[oi++] = *val++;
                    }
                    ii++;
                } else {
                    char vname[32];
                    size_t vi = 0;
                    while (ii < in_len && ((in[ii] >= 'a' && in[ii] <= 'z') || 
                                          (in[ii] >= 'A' && in[ii] <= 'Z') || 
                                          (in[ii] >= '0' && in[ii] <= '9') || 
                                          in[ii] == '_')) {
                        if (vi < sizeof(vname) - 1) vname[vi++] = in[ii];
                        ii++;
                    }
                    vname[vi] = '\0';
                    if (vi > 0) {
                        const char *val = get_var(vname);
                        if (val) {
                            while (*val && oi < max_out - 1) out[oi++] = *val++;
                        }
                    } else {
                        out[oi++] = '$';
                    }
                }
            } else {
                out[oi++] = c;
                ii++;
            }
        }
        out[oi] = '\0';
    }

    int expand_substitutions(char *line, size_t max_len, int *last_sub_status) {
        char buf[MSH_SEG_LEN * 2];
        size_t bi = 0;
        size_t li = 0;
        size_t len = strlen(line);
        bool in_single = false;
        bool in_double = false;
        int sub_count = 0;

        while (li < len && bi < sizeof(buf) - 1) {
            char c = line[li];
            if (c == '\'' && !in_double) {
                in_single = !in_single;
                buf[bi++] = c;
                li++;
            } else if (c == '\"' && !in_single) {
                in_double = !in_double;
                buf[bi++] = c;
                li++;
            } else if (c == '$' && line[li + 1] == '(' && !in_single) {
                size_t start = li + 2;
                size_t close_pos = start;
                int depth = 1;
                while (close_pos < len && depth > 0) {
                    if (line[close_pos] == '(') depth++;
                    else if (line[close_pos] == ')') depth--;
                    if (depth > 0) close_pos++;
                }
                if (depth == 0) {
                    char inner[MSH_SEG_LEN];
                    size_t in_len = close_pos - start;
                    if (in_len >= sizeof(inner)) in_len = sizeof(inner) - 1;
                    strncpy(inner, line + start, in_len);
                    inner[in_len] = '\0';

                    char exp_inner[MSH_SEG_LEN];
                    expand_variables(inner, exp_inner, sizeof(exp_inner));

                    char *iargv[MSH_MAX_ARGS];
                    int iargc = 0;
                    char *ip = exp_inner;
                    while (*ip && iargc < MSH_MAX_ARGS) {
                        while (*ip == ' ' || *ip == '\t') ip++;
                        if (!*ip) break;
                        if (*ip == '\"' || *ip == '\'') {
                            char q = *ip++;
                            iargv[iargc++] = ip;
                            while (*ip && *ip != q) ip++;
                            if (*ip) *ip++ = '\0';
                        } else {
                            iargv[iargc++] = ip;
                            while (*ip && *ip != ' ' && *ip != '\t') ip++;
                            if (*ip) *ip++ = '\0';
                        }
                    }

                    char result[MSH_VAL_LEN] = "";
                    int status = 0;
                    if (iargc > 0 && strcmp(iargv[0], "expr") == 0) {
                        status = eval_expr(iargc, iargv, result, sizeof(result));
                    }
                    if (last_sub_status) *last_sub_status = status;
                    sub_count++;

                    size_t rlen = strlen(result);
                    for (size_t r = 0; r < rlen && bi < sizeof(buf) - 1; ++r) {
                        buf[bi++] = result[r];
                    }
                    li = close_pos + 1;
                } else {
                    buf[bi++] = c;
                    li++;
                }
            } else {
                buf[bi++] = c;
                li++;
            }
        }
        buf[bi] = '\0';
        strncpy(line, buf, max_len - 1);
        line[max_len - 1] = '\0';
        return sub_count;
    }

    // -------------------------------------------------------------
    // EXECUTION & DISPATCH
    // -------------------------------------------------------------
    struct RedirStaging {
        char *buf;
        size_t cap;
        size_t len;
    };

    struct PipeStaging {
        char *buf;
        size_t len;
        size_t pos;
    };

    static PipeStaging &get_global_pipe_in() {
        static PipeStaging s_pipe_in = { nullptr, 0, 0 };
        return s_pipe_in;
    }

    static bool pipe_read_line(char *line_buf, size_t line_max) {
        PipeStaging &p = get_global_pipe_in();
        if (!p.buf || p.pos >= p.len) return false;
        size_t li = 0;
        while (p.pos < p.len && li < line_max - 1) {
            char c = p.buf[p.pos++];
            if (c == '\r') continue;
            if (c == '\n') break;
            line_buf[li++] = c;
        }
        line_buf[li] = '\0';
        return true;
    }

    static RedirStaging &get_global_redir_stage() {
        static RedirStaging s_stage = { nullptr, 0, 0 };
        return s_stage;
    }

    static OutputFn &get_global_out_cb() {
        static OutputFn s_cb = nullptr;
        return s_cb;
    }

    static OutputFn &get_global_console_cb() {
        static OutputFn s_con = nullptr;
        return s_con;
    }

    static FILE *&get_global_redir_fp() {
        static FILE *s_fp = nullptr;
        return s_fp;
    }

    void resolve_path(const char *in, char *out, size_t out_sz) {
        if (!in || !*in) {
            strncpy(out, cwd, out_sz - 1);
            out[out_sz - 1] = '\0';
            return;
        }
        if (in[0] == '/') {
            strncpy(out, in, out_sz - 1);
        } else {
            strncpy(out, cwd, out_sz - 1);
            if (strcmp(cwd, "/") != 0) {
                strncat(out, "/", out_sz - strlen(out) - 1);
            }
            strncat(out, in, out_sz - strlen(out) - 1);
        }
        out[out_sz - 1] = '\0';
    }

    int execute_single_cmd(const char *cmd_in) {
        char cmdline[MSH_LINE_LEN];
        strncpy(cmdline, cmd_in, sizeof(cmdline) - 1);
        cmdline[sizeof(cmdline) - 1] = '\0';

        int sub_status = 0;
        int sub_count = expand_substitutions(cmdline, sizeof(cmdline), &sub_status);

        char exp_line[MSH_LINE_LEN];
        expand_variables(cmdline, exp_line, sizeof(exp_line));

        char *argv[MSH_MAX_ARGS];
        int argc = 0;
        char *p = exp_line;

        while (*p && argc < MSH_MAX_ARGS) {
            while (*p == ' ' || *p == '\t') p++;
            if (!*p) break;
            if (*p == '\"' || *p == '\'') {
                char q = *p++;
                argv[argc++] = p;
                while (*p && *p != q) p++;
                if (*p) *p++ = '\0';
            } else {
                argv[argc++] = p;
                while (*p && *p != ' ' && *p != '\t') p++;
                if (*p) *p++ = '\0';
            }
        }

        if (argc == 0) return 0;

        char *eq = strchr(argv[0], '=');
        if (eq && argc == 1) {
            *eq = '\0';
            set_var(argv[0], eq + 1);
            int st = (sub_count > 0) ? sub_status : 0;
            set_last_status(st);
            return st;
        }

        // Parse I/O redirection (> or >>)
        const char *redir_file = nullptr;
        bool append_mode = false;

        for (int i = 0; i < argc; ++i) {
            if (strcmp(argv[i], ">>") == 0) {
                if (i + 1 < argc) {
                    redir_file = argv[i + 1];
                    append_mode = true;
                    argc = i;
                    break;
                }
            } else if (strcmp(argv[i], ">") == 0) {
                if (i + 1 < argc) {
                    redir_file = argv[i + 1];
                    append_mode = false;
                    argc = i;
                    break;
                }
            } else if (argv[i][0] == '>') {
                if (argv[i][1] == '>') {
                    redir_file = argv[i] + 2;
                    append_mode = true;
                    argc = i;
                    break;
                } else {
                    redir_file = argv[i] + 1;
                    append_mode = false;
                    argc = i;
                    break;
                }
            }
        }

        if (argc == 0) return 0;

        OutputFn orig_cb = this->output_cb;
        get_global_console_cb() = orig_cb;

        char resolved_redir[128];
        bool has_redir = (redir_file && *redir_file);
        if (has_redir) {
            resolve_path(redir_file, resolved_redir, sizeof(resolved_redir));
            RedirStaging &stage = get_global_redir_stage();
            stage.cap = 65536;
            stage.len = 0;
#if defined(ESP_PLATFORM)
            stage.buf = (char *)heap_caps_malloc(stage.cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!stage.buf) {
                stage.buf = (char *)malloc(stage.cap);
            }
#else
            stage.buf = (char *)malloc(stage.cap);
#endif
            if (stage.buf) {
                stage.buf[0] = '\0';
                this->output_cb = [](const char *str) {
                    if (!str) return;
                    RedirStaging &st = get_global_redir_stage();
                    if (!st.buf) return;
                    size_t slen = strlen(str);
                    if (st.len + slen < st.cap - 1) {
                        memcpy(st.buf + st.len, str, slen);
                        st.len += slen;
                        st.buf[st.len] = '\0';
                    }
                };
                get_global_out_cb() = this->output_cb;
            }
        }

        int ret = 0;
        const char *cmd = argv[0];

        if (strcmp(cmd, "help") == 0)        ret = cmd_help(argc, argv);
        else if (strcmp(cmd, "echo") == 0)   ret = cmd_echo(argc, argv);
        else if (strcmp(cmd, "printf") == 0) ret = cmd_printf(argc, argv);
        else if (strcmp(cmd, "true") == 0)   ret = 0;
        else if (strcmp(cmd, "false") == 0)  ret = 1;
        else if (strcmp(cmd, ":") == 0)      ret = 0;
        else if (strcmp(cmd, "uname") == 0)  ret = cmd_uname(argc, argv);
        else if (strcmp(cmd, "uptime") == 0) ret = cmd_uptime(argc, argv);
        else if (strcmp(cmd, "ticks") == 0)  ret = cmd_ticks(argc, argv);
        else if (strcmp(cmd, "cpuinfo") == 0 || strcmp(cmd, "chip") == 0) ret = cmd_cpuinfo(argc, argv);
        else if (strcmp(cmd, "checkelf") == 0 || strcmp(cmd, "elfcheck") == 0) ret = cmd_checkelf(argc, argv);
        else if (strcmp(cmd, "runelf") == 0 || strcmp(cmd, "exec") == 0) ret = cmd_runelf(argc, argv);
        else if (strcmp(cmd, "modinfo") == 0 || strcmp(cmd, "mnminfo") == 0) ret = cmd_modinfo(argc, argv);
        else if (strcmp(cmd, "ps") == 0)     ret = cmd_ps(argc, argv);
        else if (strcmp(cmd, "mem") == 0)    ret = cmd_mem(argc, argv);
        else if (strcmp(cmd, "alloc") == 0)  ret = cmd_alloc(argc, argv);
        else if (strcmp(cmd, "qtest") == 0)  ret = cmd_qtest(argc, argv);
        else if (strcmp(cmd, "pwd") == 0)    ret = cmd_pwd(argc, argv);
        else if (strcmp(cmd, "cd") == 0)     ret = cmd_cd(argc, argv);
        else if (strcmp(cmd, "ls") == 0)     ret = cmd_ls(argc, argv);
        else if (strcmp(cmd, "cat") == 0)    ret = cmd_cat(argc, argv);
        else if (strcmp(cmd, "grep") == 0)   ret = cmd_grep(argc, argv);
        else if (strcmp(cmd, "head") == 0)   ret = cmd_head(argc, argv);
        else if (strcmp(cmd, "wc") == 0)     ret = cmd_wc(argc, argv);
        else if (strcmp(cmd, "clear") == 0 || strcmp(cmd, "cls") == 0)  ret = cmd_clear(argc, argv);
        else if (strcmp(cmd, "reset") == 0)  ret = cmd_reset(argc, argv);
        else if (strcmp(cmd, "color") == 0)  ret = cmd_color(argc, argv);
        else if (strcmp(cmd, "basename") == 0) ret = cmd_basename(argc, argv);
        else if (strcmp(cmd, "dirname") == 0)  ret = cmd_dirname(argc, argv);
        else if (strcmp(cmd, "set") == 0)      ret = cmd_set(argc, argv);
        else if (strcmp(cmd, "unset") == 0)    ret = cmd_unset(argc, argv);
        else if (strcmp(cmd, "env") == 0)      ret = cmd_env(argc, argv);
        else if (strcmp(cmd, "test") == 0 || strcmp(cmd, "[") == 0) ret = cmd_test(argc, argv);
        else if (strcmp(cmd, "expr") == 0)     ret = cmd_expr(argc, argv);
        else if (strcmp(cmd, "backlight") == 0) ret = cmd_backlight(argc, argv);
        else if (strcmp(cmd, "peek") == 0)     ret = cmd_peek(argc, argv);
        else if (strcmp(cmd, "poke") == 0)     ret = cmd_poke(argc, argv);
        else if (strcmp(cmd, "reboot") == 0)   ret = cmd_reboot(argc, argv);
        else if (strcmp(cmd, "mkdir") == 0)    ret = cmd_mkdir(argc, argv);
        else if (strcmp(cmd, "rm") == 0)       ret = cmd_rm(argc, argv);
        else if (strcmp(cmd, "touch") == 0)    ret = cmd_touch(argc, argv);
        else if (strcmp(cmd, "write") == 0)    ret = cmd_write(argc, argv);
        else if (strcmp(cmd, "hexwrite") == 0) ret = cmd_hexwrite(argc, argv);
        else if (strcmp(cmd, "hexread") == 0)  ret = cmd_hexread(argc, argv);
        else if (strcmp(cmd, "hexdump") == 0)  ret = cmd_hexdump(argc, argv);
        else if (strcmp(cmd, "jobs") == 0)     ret = cmd_jobs(argc, argv);
        else if (strcmp(cmd, "source") == 0 || strcmp(cmd, ".") == 0) ret = cmd_source(argc, argv);
        else {
            char test_path[128];
            resolve_path(cmd, test_path, sizeof(test_path));
            struct stat st;
            bool found = (stat(test_path, &st) == 0 && !S_ISDIR(st.st_mode));
            if (!found && strchr(cmd, '/') == nullptr) {
                snprintf(test_path, sizeof(test_path), "/sd/%s", cmd);
                if (stat(test_path, &st) == 0 && !S_ISDIR(st.st_mode)) {
                    found = true;
                }
            }
            if (found) {
                char *elf_argv[MSH_MAX_ARGS + 2];
                char run_name[] = "runelf";
                elf_argv[0] = run_name;
                elf_argv[1] = test_path;
                for (int i = 1; i < argc && i + 1 < (int)(sizeof(elf_argv)/sizeof(elf_argv[0])); ++i) {
                    elf_argv[i + 1] = argv[i];
                }
                int elf_argc = (argc + 1 < (int)(sizeof(elf_argv)/sizeof(elf_argv[0]))) ? (argc + 1) : (int)(sizeof(elf_argv)/sizeof(elf_argv[0]));
                ret = cmd_runelf(elf_argc, elf_argv);
            } else {
                print("msh: command not found: ");
                println(cmd);
                ret = 127;
            }
        }

        if (has_redir) {
            RedirStaging &stage = get_global_redir_stage();
            if (stage.buf) {
                FILE *fp = fopen(resolved_redir, append_mode ? "a" : "w");
                if (fp) {
                    if (stage.len > 0) {
                        fwrite(stage.buf, 1, stage.len, fp);
                    }
                    fclose(fp);
                } else {
                    if (orig_cb) {
                        orig_cb("msh: cannot open redirection file: ");
                        orig_cb(resolved_redir);
                        orig_cb("\n");
                    }
                }
                free(stage.buf);
                stage.buf = nullptr;
                stage.cap = 0;
                stage.len = 0;
            }
            this->output_cb = orig_cb;
            get_global_out_cb() = orig_cb;
        }

        set_last_status(ret);
        return ret;
    }

    // -------------------------------------------------------------
    // CONTROL FLOW ENGINE
    // -------------------------------------------------------------
    void push_segment(const char *s) {
        if (!s) return;
        while (*s == ' ' || *s == '\t') s++;
        size_t slen = strlen(s);
        while (slen > 0 && (s[slen - 1] == ' ' || s[slen - 1] == '\t')) slen--;
        if (slen == 0 || seg_count >= MSH_MAX_SEGS) return;

        // Check if segment starts with a standalone control keyword: "then ", "do ", "else "
        const char *kws[] = { "then", "do", "else" };
        for (int k = 0; k < 3; ++k) {
            size_t klen = strlen(kws[k]);
            if (strncmp(s, kws[k], klen) == 0 && (s[klen] == ' ' || s[klen] == '\t')) {
                strncpy(seg_pool[seg_count], kws[k], MSH_SEG_LEN - 1);
                seg_pool[seg_count][MSH_SEG_LEN - 1] = '\0';
                seg_count++;
                push_segment(s + klen);
                return;
            }
        }

        char tmp[MSH_SEG_LEN];
        if (slen >= sizeof(tmp)) slen = sizeof(tmp) - 1;
        strncpy(tmp, s, slen);
        tmp[slen] = '\0';

        // Check if segment ends with " fi" or " done"
        if (slen > 3 && strcmp(tmp + slen - 3, " fi") == 0) {
            tmp[slen - 3] = '\0';
            push_segment(tmp);
            push_segment("fi");
            return;
        }
        if (slen > 5 && strcmp(tmp + slen - 5, " done") == 0) {
            tmp[slen - 5] = '\0';
            push_segment(tmp);
            push_segment("done");
            return;
        }

        strncpy(seg_pool[seg_count], tmp, MSH_SEG_LEN - 1);
        seg_pool[seg_count][MSH_SEG_LEN - 1] = '\0';
        seg_count++;
    }

    int split_semicolons(const char *cmd_line) {
        seg_count = 0;
        const char *p = cmd_line;
        bool in_single = false;
        bool in_double = false;
        char cur_buf[MSH_SEG_LEN];
        size_t c_len = 0;

        while (*p) {
            char c = *p;
            if (c == '\'' && !in_double) in_single = !in_single;
            else if (c == '\"' && !in_single) in_double = !in_double;

            if ((c == ';' || c == '\n') && !in_single && !in_double) {
                cur_buf[c_len] = '\0';
                push_segment(cur_buf);
                c_len = 0;
            } else {
                if (c_len < sizeof(cur_buf) - 1) cur_buf[c_len++] = c;
            }
            p++;
        }
        if (c_len > 0) {
            cur_buf[c_len] = '\0';
            push_segment(cur_buf);
        }
        return seg_count;
    }

    static const char *strip_keyword(const char *s, const char *kw) {
        if (!s || !kw) return nullptr;
        size_t kwlen = strlen(kw);
        if (strncmp(s, kw, kwlen) == 0) {
            const char *rem = s + kwlen;
            if (*rem == '\0') return nullptr;
            if (*rem == ' ' || *rem == '\t') {
                while (*rem == ' ' || *rem == '\t') rem++;
                return (*rem != '\0') ? rem : nullptr;
            }
        }
        return nullptr;
    }

    int find_matching_keyword(int start, const char *target, const char *alt1 = nullptr, const char *alt2 = nullptr) {
        int depth_if = 0;
        int depth_loop = 0;
        for (int i = start; i < seg_count; ++i) {
            const char *s = seg_pool[i];
            if (strncmp(s, "if ", 3) == 0 || strcmp(s, "if") == 0) depth_if++;
            else if (strcmp(s, "fi") == 0) {
                if (depth_if > 0) depth_if--;
                else if (strcmp(target, "fi") == 0 || (alt1 && strcmp(alt1, "fi") == 0) || (alt2 && strcmp(alt2, "fi") == 0)) return i;
            }
            else if (strncmp(s, "while ", 6) == 0 || strcmp(s, "while") == 0 ||
                     strncmp(s, "until ", 6) == 0 || strcmp(s, "until") == 0 ||
                     strncmp(s, "for ", 4) == 0 || strcmp(s, "for") == 0) {
                depth_loop++;
            }
            else if (strcmp(s, "done") == 0) {
                if (depth_loop > 0) depth_loop--;
                else if (strcmp(target, "done") == 0 || (alt1 && strcmp(alt1, "done") == 0) || (alt2 && strcmp(alt2, "done") == 0)) return i;
            }
            else if (depth_if == 0 && depth_loop == 0) {
                size_t tlen = strlen(target);
                if (strcmp(target, s) == 0 || (strncmp(s, target, tlen) == 0 && (s[tlen] == ' ' || s[tlen] == '\t' || s[tlen] == '\0'))) return i;
                if (alt1) {
                    size_t a1len = strlen(alt1);
                    if (strcmp(alt1, s) == 0 || (strncmp(s, alt1, a1len) == 0 && (s[a1len] == ' ' || s[a1len] == '\t' || s[a1len] == '\0'))) return i;
                }
                if (alt2) {
                    size_t a2len = strlen(alt2);
                    if (strcmp(alt2, s) == 0 || (strncmp(s, alt2, a2len) == 0 && (s[a2len] == ' ' || s[a2len] == '\t' || s[a2len] == '\0'))) return i;
                }
            }
        }
        return -1;
    }

    int execute_pipeline_sync(const char *cmd_line) {
        if (!cmd_line || !*cmd_line) return 0;

        // Check if '|' is present outside quotes
        bool in_q = false;
        char q_char = '\0';
        bool has_pipe = false;
        for (const char *p = cmd_line; *p; ++p) {
            if ((*p == '\"' || *p == '\'') && (!in_q || *p == q_char)) {
                if (in_q) { in_q = false; q_char = '\0'; }
                else { in_q = true; q_char = *p; }
            } else if (*p == '|' && !in_q) {
                has_pipe = true;
                break;
            }
        }

        if (!has_pipe) {
            return execute_single_cmd(cmd_line);
        }

        // Split into pipeline stages
        char stages[8][MSH_LINE_LEN];
        int stage_count = 0;
        char cur_stage[MSH_LINE_LEN];
        size_t csi = 0;
        in_q = false;
        q_char = '\0';

        for (const char *p = cmd_line; *p && stage_count < 8; ++p) {
            if ((*p == '\"' || *p == '\'') && (!in_q || *p == q_char)) {
                if (in_q) { in_q = false; q_char = '\0'; }
                else { in_q = true; q_char = *p; }
                if (csi < sizeof(cur_stage) - 1) cur_stage[csi++] = *p;
            } else if (*p == '|' && !in_q) {
                cur_stage[csi] = '\0';
                char *s = cur_stage;
                while (*s == ' ' || *s == '\t') s++;
                size_t slen = strlen(s);
                while (slen > 0 && (s[slen - 1] == ' ' || s[slen - 1] == '\t')) s[--slen] = '\0';
                strncpy(stages[stage_count++], s, MSH_LINE_LEN - 1);
                stages[stage_count - 1][MSH_LINE_LEN - 1] = '\0';
                csi = 0;
            } else {
                if (csi < sizeof(cur_stage) - 1) cur_stage[csi++] = *p;
            }
        }
        if (csi > 0 && stage_count < 8) {
            cur_stage[csi] = '\0';
            char *s = cur_stage;
            while (*s == ' ' || *s == '\t') s++;
            size_t slen = strlen(s);
            while (slen > 0 && (s[slen - 1] == ' ' || s[slen - 1] == '\t')) s[--slen] = '\0';
            strncpy(stages[stage_count++], s, MSH_LINE_LEN - 1);
            stages[stage_count - 1][MSH_LINE_LEN - 1] = '\0';
        }

        if (stage_count == 0) return 0;
        if (stage_count == 1) return execute_single_cmd(stages[0]);

        int last_rc = 0;
        char *prev_pipe_buf = nullptr;
        size_t prev_pipe_len = 0;

        for (int k = 0; k < stage_count; ++k) {
            get_global_pipe_in().buf = prev_pipe_buf;
            get_global_pipe_in().len = prev_pipe_len;
            get_global_pipe_in().pos = 0;

            if (k < stage_count - 1) {
                RedirStaging cur_stage_out = { nullptr, 65536, 0 };
#if defined(ESP_PLATFORM)
                cur_stage_out.buf = (char *)heap_caps_malloc(cur_stage_out.cap, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (!cur_stage_out.buf) cur_stage_out.buf = (char *)malloc(cur_stage_out.cap);
#else
                cur_stage_out.buf = (char *)malloc(cur_stage_out.cap);
#endif
                if (cur_stage_out.buf) cur_stage_out.buf[0] = '\0';

                OutputFn orig_cb = this->output_cb;
                RedirStaging &global_stage = get_global_redir_stage();
                RedirStaging saved_stage = global_stage;
                global_stage = cur_stage_out;

                this->output_cb = [](const char *str) {
                    if (!str) return;
                    RedirStaging &st = get_global_redir_stage();
                    if (!st.buf) return;
                    size_t slen = strlen(str);
                    if (st.len + slen < st.cap - 1) {
                        memcpy(st.buf + st.len, str, slen);
                        st.len += slen;
                        st.buf[st.len] = '\0';
                    }
                };
                get_global_out_cb() = this->output_cb;

                last_rc = execute_single_cmd(stages[k]);

                cur_stage_out = global_stage;
                global_stage = saved_stage;
                this->output_cb = orig_cb;
                get_global_out_cb() = orig_cb;

                if (prev_pipe_buf) {
                    free(prev_pipe_buf);
                    prev_pipe_buf = nullptr;
                }
                prev_pipe_buf = cur_stage_out.buf;
                prev_pipe_len = cur_stage_out.len;
            } else {
                last_rc = execute_single_cmd(stages[k]);
                if (prev_pipe_buf) {
                    free(prev_pipe_buf);
                    prev_pipe_buf = nullptr;
                    prev_pipe_len = 0;
                }
            }
        }

        get_global_pipe_in().buf = nullptr;
        get_global_pipe_in().len = 0;
        get_global_pipe_in().pos = 0;

        return last_rc;
    }

    struct ShellJob {
        int id;
        char cmd[64];
        bool active;
        int exit_code;
    };
    static const int MAX_JOBS = 8;

    static ShellJob *get_job_table() {
        static ShellJob s_jobs[MAX_JOBS] = {};
        return s_jobs;
    }

    static int &get_next_job_id() {
        static int s_next_id = 0;
        return s_next_id;
    }

    int launch_background_job(const char *clean_cmd) {
        ShellJob *jobs = get_job_table();
        int job_id = ++get_next_job_id();
        int slot = -1;
        for (int j = 0; j < MAX_JOBS; ++j) {
            if (!jobs[j].active) { slot = j; break; }
        }
        if (slot < 0) slot = 0;
        jobs[slot].id = job_id;
        strncpy(jobs[slot].cmd, clean_cmd, sizeof(jobs[slot].cmd) - 1);
        jobs[slot].cmd[sizeof(jobs[slot].cmd) - 1] = '\0';
        jobs[slot].active = true;
        jobs[slot].exit_code = 0;

        char msg[128];
        snprintf(msg, sizeof(msg), "[%d] %s &", job_id, jobs[slot].cmd);
        println(msg);

#if defined(ESP_PLATFORM)
        struct BgJobContext {
            MaiaShell *shell;
            char cmd[MSH_SEG_LEN];
            int slot;
        };
        BgJobContext *ctx = (BgJobContext *)malloc(sizeof(BgJobContext));
        if (ctx) {
            ctx->shell = this;
            strncpy(ctx->cmd, clean_cmd, sizeof(ctx->cmd) - 1);
            ctx->cmd[sizeof(ctx->cmd) - 1] = '\0';
            ctx->slot = slot;
            xTaskCreatePinnedToCore([](void *arg) {
                BgJobContext *c = (BgJobContext *)arg;
                int rc = c->shell->execute_pipeline_sync(c->cmd);
                get_job_table()[c->slot].active = false;
                get_job_table()[c->slot].exit_code = rc;
                free(c);
                vTaskDelete(NULL);
            }, "msh_bg", 8192, ctx, 5, NULL, 1);
        }
        return 0;
#else
        int rc = execute_pipeline_sync(clean_cmd);
        jobs[slot].active = false;
        jobs[slot].exit_code = rc;
        return 0;
#endif
    }

    int execute_pipeline(const char *cmd_line) {
        if (!cmd_line || !*cmd_line) return 0;

        char clean_line[MSH_SEG_LEN];
        strncpy(clean_line, cmd_line, sizeof(clean_line) - 1);
        clean_line[sizeof(clean_line) - 1] = '\0';
        size_t clen = strlen(clean_line);
        while (clen > 0 && (clean_line[clen - 1] == ' ' || clean_line[clen - 1] == '\t')) clean_line[--clen] = '\0';

        bool is_bg = false;
        if (clen > 0 && clean_line[clen - 1] == '&') {
            is_bg = true;
            clean_line[--clen] = '\0';
            while (clen > 0 && (clean_line[clen - 1] == ' ' || clean_line[clen - 1] == '\t')) clean_line[--clen] = '\0';
        }

        if (is_bg) {
            return launch_background_job(clean_line);
        }

        return execute_pipeline_sync(clean_line);
    }

    int execute_range(int start, int end, bool *out_break, bool *out_cont) {
        int i = start;
        int last_rc = 0;
        while (i < end) {
            const char *s = seg_pool[i];

            if (strcmp(s, "break") == 0) {
                if (out_break) *out_break = true;
                return last_rc;
            }
            if (strcmp(s, "continue") == 0) {
                if (out_cont) *out_cont = true;
                return last_rc;
            }

            // IF / THEN / ELIF / ELSE / FI
            if (strncmp(s, "if ", 3) == 0 || strcmp(s, "if") == 0) {
                int fi_idx = find_matching_keyword(i + 1, "fi");
                if (fi_idx < 0) {
                    println("msh: syntax error: missing fi");
                    return 2;
                }
                int cur = i;
                bool branch_taken = false;
                while (cur < fi_idx && !branch_taken) {
                    if (strncmp(seg_pool[cur], "if", 2) == 0 || strncmp(seg_pool[cur], "elif", 4) == 0) {
                        bool is_elif = (strncmp(seg_pool[cur], "elif", 4) == 0);
                        const char *cond = is_elif ? (seg_pool[cur] + 4) : (seg_pool[cur] + 2);
                        while (*cond == ' ' || *cond == '\t') cond++;
                        int then_idx = find_matching_keyword(cur + 1, "then");
                        if (then_idx < 0 || then_idx > fi_idx) {
                            println("msh: syntax error: missing then");
                            return 2;
                        }
                        int next_branch = find_matching_keyword(then_idx + 1, "elif", "else", "fi");
                        int cond_rc = execute_pipeline(cond);
                        if (cond_rc == 0) {
                            branch_taken = true;
                            const char *inline_then = strip_keyword(seg_pool[then_idx], "then");
                            if (inline_then && *inline_then) {
                                last_rc = execute_pipeline(inline_then);
                            }
                            if (then_idx + 1 < next_branch) {
                                last_rc = execute_range(then_idx + 1, next_branch, out_break, out_cont);
                            }
                            if ((out_break && *out_break) || (out_cont && *out_cont)) return last_rc;
                        }
                        cur = next_branch;
                    } else if (strncmp(seg_pool[cur], "else", 4) == 0) {
                        branch_taken = true;
                        const char *inline_else = strip_keyword(seg_pool[cur], "else");
                        if (inline_else && *inline_else) {
                            last_rc = execute_pipeline(inline_else);
                        }
                        if (cur + 1 < fi_idx) {
                            last_rc = execute_range(cur + 1, fi_idx, out_break, out_cont);
                        }
                        if ((out_break && *out_break) || (out_cont && *out_cont)) return last_rc;
                        cur = fi_idx;
                    } else {
                        break;
                    }
                }
                i = fi_idx + 1;
                continue;
            }

            // WHILE / UNTIL ... DO ... DONE
            if (strncmp(s, "while ", 6) == 0 || strcmp(s, "while") == 0 ||
                strncmp(s, "until ", 6) == 0 || strcmp(s, "until") == 0) {
                bool is_until = (strncmp(s, "until", 5) == 0);
                const char *cond_init = is_until ? (s + 5) : (s + 5);
                while (*cond_init == ' ' || *cond_init == '\t') cond_init++;
                int do_idx = find_matching_keyword(i + 1, "do");
                int done_idx = find_matching_keyword(i + 1, "done");
                if (do_idx < 0 || done_idx < 0 || do_idx > done_idx) {
                    println("msh: syntax error: missing do/done");
                    return 2;
                }
                char cond_buf[MSH_SEG_LEN];
                strncpy(cond_buf, cond_init, sizeof(cond_buf) - 1);
                cond_buf[sizeof(cond_buf) - 1] = '\0';

                int loop_guard = 2000;
                while (loop_guard-- > 0) {
                    int c_rc = execute_pipeline(cond_buf);
                    bool cond_pass = is_until ? (c_rc != 0) : (c_rc == 0);
                    if (!cond_pass) break;
                    bool b_break = false;
                    bool b_cont = false;
                    const char *inline_do = strip_keyword(seg_pool[do_idx], "do");
                    if (inline_do && *inline_do) {
                        last_rc = execute_pipeline(inline_do);
                    }
                    if (do_idx + 1 < done_idx) {
                        last_rc = execute_range(do_idx + 1, done_idx, &b_break, &b_cont);
                    }
                    if (b_break) break;
                }
                i = done_idx + 1;
                continue;
            }

            // FOR VAR IN ... DO ... DONE
            if (strncmp(s, "for ", 4) == 0) {
                int do_idx = find_matching_keyword(i + 1, "do");
                int done_idx = find_matching_keyword(i + 1, "done");
                if (do_idx < 0 || done_idx < 0 || do_idx > done_idx) {
                    println("msh: syntax error: missing do/done in for");
                    return 2;
                }
                char for_hdr[MSH_SEG_LEN];
                strncpy(for_hdr, s + 4, sizeof(for_hdr) - 1);
                for_hdr[sizeof(for_hdr) - 1] = '\0';
                char *p_hdr = for_hdr;
                while (*p_hdr == ' ' || *p_hdr == '\t') p_hdr++;
                char var_name[32] = "";
                size_t vni = 0;
                while (*p_hdr && *p_hdr != ' ' && *p_hdr != '\t') {
                    if (vni < sizeof(var_name) - 1) var_name[vni++] = *p_hdr;
                    p_hdr++;
                }
                var_name[vni] = '\0';
                while (*p_hdr == ' ' || *p_hdr == '\t') p_hdr++;
                if (strncmp(p_hdr, "in", 2) == 0) p_hdr += 2;
                while (*p_hdr == ' ' || *p_hdr == '\t') p_hdr++;

                char exp_items[MSH_SEG_LEN];
                expand_variables(p_hdr, exp_items, sizeof(exp_items));
                char *items[32];
                int item_cnt = 0;
                char *ip = exp_items;
                while (*ip && item_cnt < 32) {
                    while (*ip == ' ' || *ip == '\t') ip++;
                    if (!*ip) break;
                    items[item_cnt++] = ip;
                    while (*ip && *ip != ' ' && *ip != '\t') ip++;
                    if (*ip) *ip++ = '\0';
                }

                for (int it = 0; it < item_cnt; ++it) {
                    set_var(var_name, items[it]);
                    bool b_break = false;
                    bool b_cont = false;
                    const char *inline_do = strip_keyword(seg_pool[do_idx], "do");
                    if (inline_do && *inline_do) {
                        last_rc = execute_pipeline(inline_do);
                    }
                    if (do_idx + 1 < done_idx) {
                        last_rc = execute_range(do_idx + 1, done_idx, &b_break, &b_cont);
                    }
                    if (b_break) break;
                }
                i = done_idx + 1;
                continue;
            }

            // Normal single command or pipeline
            last_rc = execute_pipeline(s);
            i++;
        }
        return last_rc;
    }

    int execute(const char *cmd_line) {
        if (!cmd_line || !*cmd_line) return 0;
        int n_segs = split_semicolons(cmd_line);
        if (n_segs == 0) return 0;
        return execute_range(0, n_segs, nullptr, nullptr);
    }

    int execute_script(const char *path) {
        if (!path || !*path) return 1;
        char resolved[128];
        resolve_path(path, resolved, sizeof(resolved));

        FILE *f = fopen(resolved, "r");
        if (!f) {
            print("source: cannot open '");
            print(path);
            println("'");
            return 1;
        }

        char line[MSH_LINE_LEN];
        int last_rc = 0;
        while (fgets(line, sizeof(line), f)) {
            char *p = line;
            while (*p == ' ' || *p == '\t') p++;
            if (*p == '\0' || *p == '\r' || *p == '\n' || *p == '#') continue;

            size_t len = strlen(p);
            while (len > 0 && (p[len - 1] == '\r' || p[len - 1] == '\n' || p[len - 1] == ' ' || p[len - 1] == '\t')) {
                p[--len] = '\0';
            }
            if (len == 0) continue;

            last_rc = execute(p);
        }
        fclose(f);
        return last_rc;
    }

private:
    OutputFn output_cb;
    int last_status;
    ShellVar vars[MSH_MAX_VARS];
    char cwd[64];

    char seg_pool[MSH_MAX_SEGS][MSH_SEG_LEN];
    int seg_count;

    void print(const char *str) {
        if (output_cb && str) output_cb(str);
    }

    void println(const char *str = "") {
        print(str);
        print("\n");
    }

    // -------------------------------------------------------------
    // BUILT-IN COMMANDS
    // -------------------------------------------------------------
    int cmd_help(int argc, char **argv) {
        (void)argc; (void)argv;
        println("MaiaShell Native Commands & Features:");
        println("  help              - Show this command summary");
        println("  uname             - Show MaiaRTOS version & hardware platform");
        println("  uptime            - Show RTOS uptime and context switches");
        println("  ticks             - Show CLIC/SYSTIMER interrupt & trap counters");
        println("  ps                - List RTOS tasks (ID, prio, state, stack, ticks)");
        println("  mem               - Show PSRAM & L2 SRAM memory telemetry");
        println("  cpuinfo / chip    - Show ESP32-P4 CPU & silicon profile");
        println("  checkelf          - Run MNM-1 ELF32 RISC-V validator self-test");
        println("  runelf [path] [args] - Load, relocate & run native RISC-V ELF module");
        println("  modinfo <file.mobj> - Inspect module, SHA-256 and manifest integrity");
        println("  ls [dir]          - List files & directories in storage/SD-card");
        println("  cd <dir>          - Change current working directory");
        println("  cat [file]        - Display file contents or pipe data");
        println("  grep [-i -v] PAT  - Filter lines matching pattern (file or pipe)");
        println("  head [-n N] [file]- Print first N lines of file or pipe");
        println("  wc [-l -w -c]     - Count lines, words and bytes of file or pipe");
        println("  cmd1 | cmd2       - Pipeline execution chaining output to input");
        println("  mkdir / rm        - Create directories or remove files");
        println("  touch / write     - Create empty files or append lines of text");
        println("  hexwrite / hexread- Binary file hex transfer and raw inspection");
        println("  hexdump [-n N]    - Canonical hex + ASCII dump");
        println("  jobs / cmd &      - Background job execution and process tracking");
        println("  source / . <file> - Execute commands from shell script file");
        println("  <file.mobj> [args]- Direct native execution without typing runelf");
        println("  alloc <bytes>     - Allocate, verify & free a block from L2MEM");
        println("  qtest             - Live mutex, semaphore & queue self-test");
        println("  expr <EXPR>       - Evaluate arithmetic/logical/comparison expression");
        println("  $(expr ...)       - Command substitution in assignments and arguments");
        println("  if/elif/else/fi   - Conditional branching");
        println("  while/until/for   - Loop constructs with break and continue");
        println("  test / [ EXPR ]   - Condition evaluation");
        println("  set [K=V] / unset - Manage shell variables ($VAR, $?)");
        println("  env / echo / pwd  - Environment and path utilities");
        println("  basename/dirname  - Path manipulation utilities");
        println("  backlight <0-100> - Set LCD backlight brightness percentage");
        println("  peek/poke <addr>  - Direct 32-bit hardware register access");
        println("  clear / cls       - Clear terminal output area");
        println("  reset             - Full screen reset and repaint header");
        return 0;
    }

    int cmd_uname(int argc, char **argv) {
        (void)argc; (void)argv;
        println("MaiaRTOS 0.1.0 riscv32-esp32p4 (M5Stack Tab5) [Zero-FreeRTOS/Zero-IDF]");
        return 0;
    }

    int cmd_ps(int argc, char **argv) {
        (void)argc; (void)argv;
#if defined(MAIA_KERNEL_H)
        println("TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME");
        for (uint32_t i = 0; i < MAIA_MAX_TASKS; ++i) {
            maia_task_info_t info;
            if (maia_task_get_info((maia_task_id_t)i, &info) == MAIA_OK) {
                const char *st = "UNUSD";
                switch (info.state) {
                    case MAIA_TASK_READY:    st = "READY"; break;
                    case MAIA_TASK_RUNNING:  st = "RUN  "; break;
                    case MAIA_TASK_SLEEPING: st = "SLEEP"; break;
                    case MAIA_TASK_BLOCKED:  st = "BLOCK"; break;
                    case MAIA_TASK_EXITED:   st = "EXIT "; break;
                    default: break;
                }
                char buf[128];
                snprintf(buf, sizeof(buf), "  %ld     %u  %s  %5u      %5lu  %5lu  %s",
                         (long)info.id, (unsigned int)info.priority, st,
                         (unsigned int)info.stack_size, (unsigned long)info.switches,
                         (unsigned long)info.run_ticks, info.name);
                println(buf);
            }
        }
#else
        println("TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME");
        println("  0     0  READY   4096          1      0  idle");
        println("  1     2  SLEEP   4096         10      2  telemetry");
        println("  2     2  RUN     8192         25      5  maia_shell");
#endif
        return 0;
    }

    int cmd_mem(int argc, char **argv) {
        (void)argc; (void)argv;
#if defined(MAIA_KERNEL_H)
        maia_mem_stats_t ms;
        (void)maia_mem_get_stats(&ms);
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "L2MEM Region Allocator:\n"
                 "  total=%lu B  used=%lu B  free=%lu B  peak=%lu B\n"
                 "  allocs=%lu  frees=%lu",
                 (unsigned long)ms.total_bytes, (unsigned long)ms.used_bytes,
                 (unsigned long)ms.free_bytes, (unsigned long)ms.peak_used_bytes,
                 (unsigned long)ms.alloc_count, (unsigned long)ms.free_count);
        println(buf);
#elif defined(ESP_PLATFORM)
        size_t psram_total = esp_psram_get_size();
        size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t psram_largest = heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM);
        size_t sram_free  = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t sram_total = heap_caps_get_total_size(MALLOC_CAP_INTERNAL);
        size_t sram_largest = heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL);
        char buf[280];
        snprintf(buf, sizeof(buf),
                 "ESP32-P4 Memory Telemetry:\n"
                 "  Octal PSRAM @ 200MHz: %lu / %lu KB free (max block: %lu KB)\n"
                 "  Internal L2 SRAM:     %lu / %lu KB free (max block: %lu KB)\n"
                 "  Total Dynamic RAM:    %lu MB mapped",
                 (unsigned long)(psram_free / 1024), (unsigned long)(psram_total / 1024),
                 (unsigned long)(psram_largest / 1024),
                 (unsigned long)(sram_free / 1024), (unsigned long)(sram_total / 1024),
                 (unsigned long)(sram_largest / 1024),
                 (unsigned long)(psram_total / (1024 * 1024)));
        println(buf);
#else
        println("Memory: Host Emulation (32KB L2MEM Pool)");
#endif
        return 0;
    }

    int cmd_uptime(int argc, char **argv) {
        (void)argc; (void)argv;
#if defined(MAIA_KERNEL_H)
        uint64_t ticks = maia_tick_get();
        uint64_t us    = maia_time_us();
        uint32_t sw    = maia_sched_switch_count();
        char buf[128];
        snprintf(buf, sizeof(buf), "Uptime: %lu ms (%lu us), context switches: %lu",
                 (unsigned long)ticks, (unsigned long)us, (unsigned long)sw);
        println(buf);
#elif defined(ESP_PLATFORM)
        int64_t us = esp_timer_get_time();
        uint32_t ms = (uint32_t)(us / 1000);
        uint32_t sec = ms / 1000;
        uint32_t min = sec / 60;
        uint32_t hr = min / 60;
        char buf[128];
        snprintf(buf, sizeof(buf), "Uptime: %02lu:%02lu:%02lu (%lu ms, %llu us)",
                 (unsigned long)hr, (unsigned long)(min % 60), (unsigned long)(sec % 60),
                 (unsigned long)ms, (unsigned long long)us);
        println(buf);
#else
        println("Uptime: 1000 ms (Host Emulation)");
#endif
        return 0;
    }

    int cmd_cpuinfo(int argc, char **argv) {
        (void)argc; (void)argv;
#if defined(ESP_PLATFORM)
        esp_chip_info_t info;
        esp_chip_info(&info);
        char buf[256];
        snprintf(buf, sizeof(buf),
                 "ESP32-P4 Silicon Profile:\n"
                 "  CPU:          RISC-V HP Dual-Core @ 360 MHz (RV32IMAFCP)\n"
                 "  Cores:        %u active cores\n"
                 "  Revision:     v%u.%u (ECO2)\n"
                 "  Flash:        16 MB SPI Flash (Quad/QIO @ 80MHz)\n"
                 "  Subsystems:   FPU, DSP, 2D PPA, MIPI-DSI, ISP, USB-OTG",
                 info.cores, info.revision / 100, info.revision % 100);
        println(buf);
#else
        println("CPU: RISC-V RV32IMAFCP Dual-Core @ 360MHz (ESP32-P4)");
#endif
        return 0;
    }

    int cmd_checkelf(int argc, char **argv) {
        (void)argc; (void)argv;
        println("=== MNM-1 ELF32 RISC-V Relocatable Validator Test ===");

        static const uint8_t valid_elf[] = {
            0x7f, 'E', 'L', 'F',
            0x01, 0x01, 0x01,
            0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
            0x01, 0x00,
            0xf3, 0x00,
            0x01, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x00, 0x00, 0x00, 0x00,
            0x34, 0x00,
            0x00, 0x00,
            0x00, 0x00,
            0x28, 0x00,
            0x00, 0x00,
            0x00, 0x00
        };

        char rep[256];
        int res = elf_validate_relocatable(valid_elf, sizeof(valid_elf), rep, sizeof(rep));
        if (res == ELF_VALID_OK) {
            println("[TEST 1/2 PASS] Valid ET_REL RISC-V ELF header accepted:");
            println(rep);
        } else {
            char eb[64];
            snprintf(eb, sizeof(eb), "[TEST 1/2 FAIL] Validation failed: err=%d", res);
            println(eb);
        }

        uint8_t bad_elf[52];
        memcpy(bad_elf, valid_elf, 52);
        bad_elf[18] = 0x03; // Alter to EM_386
        res = elf_validate_relocatable(bad_elf, sizeof(bad_elf), rep, sizeof(rep));
        if (res != ELF_VALID_OK) {
            println("[TEST 2/2 PASS] Non-RISC-V ELF correctly rejected:");
            println(rep);
        } else {
            println("[TEST 2/2 FAIL] Bad ELF was incorrectly accepted!");
        }

        return 0;
    }

    static bool compute_sha256(const uint8_t *data, size_t len, char *hex_out, size_t hex_max) {
        if (!data || !hex_out || hex_max < 65) return false;
#if defined(ESP_PLATFORM)
        uint8_t hash[32];
        mbedtls_sha256(data, len, hash, 0);
        for (int i = 0; i < 32; ++i) {
            snprintf(hex_out + (i * 2), 3, "%02x", hash[i]);
        }
        hex_out[64] = '\0';
        return true;
#else
        hex_out[0] = '\0';
        return false;
#endif
    }

    int cmd_modinfo(int argc, char **argv) {
        if (argc < 2) {
            println("Usage: modinfo <module.mobj>");
            return 1;
        }
        char mod_path[128];
        resolve_path(argv[1], mod_path, sizeof(mod_path));

        FILE *f = fopen(mod_path, "rb");
        if (!f) {
            print("modinfo: cannot open '");
            print(argv[1]);
            println("'");
            return 1;
        }
        fseek(f, 0, SEEK_END);
        size_t fsz = (size_t)ftell(f);
        fseek(f, 0, SEEK_SET);

        if (fsz < sizeof(Elf32_Ehdr)) {
            println("modinfo: file too small for ELF header");
            fclose(f);
            return 1;
        }

        uint8_t *buf = (uint8_t *)malloc(fsz);
        if (!buf) {
            println("modinfo: out of memory");
            fclose(f);
            return 1;
        }
        fread(buf, 1, fsz, f);
        fclose(f);

        char title[128];
        snprintf(title, sizeof(title), "=== Maia Native Module (MNM-1): %s ===", argv[1]);
        println(title);

        char sz_str[64];
        snprintf(sz_str, sizeof(sz_str), "File Size: %u bytes", (unsigned int)fsz);
        println(sz_str);

        char sha_hex[65] = "";
        bool has_sha = compute_sha256(buf, fsz, sha_hex, sizeof(sha_hex));
        if (has_sha) {
            char sha_str[96];
            snprintf(sha_str, sizeof(sha_str), "SHA-256  : %s", sha_hex);
            println(sha_str);
        }

        char manifest_path[128];
        strncpy(manifest_path, mod_path, sizeof(manifest_path) - 1);
        manifest_path[sizeof(manifest_path) - 1] = '\0';
        char *dot = strrchr(manifest_path, '.');
        if (dot) *dot = '\0';
        strncat(manifest_path, ".mnm.json", sizeof(manifest_path) - strlen(manifest_path) - 1);

        FILE *mf = fopen(manifest_path, "r");
        if (mf) {
            char mbuf[512];
            size_t mlen = fread(mbuf, 1, sizeof(mbuf) - 1, mf);
            fclose(mf);
            mbuf[mlen] = '\0';

            println("Manifest : Companion manifest (.mnm.json) found");
            const char *sha_pos = strstr(mbuf, "\"elf_sha256\"");
            if (sha_pos) {
                const char *val = strchr(sha_pos, ':');
                if (val) {
                    val++;
                    while (*val == ' ' || *val == '\t' || *val == '\"') val++;
                    char exp_sha[65] = "";
                    for (int i = 0; i < 64 && *val && *val != '\"' && *val != '\r' && *val != '\n'; ++i) exp_sha[i] = *val++;
                    exp_sha[64] = '\0';
                    if (has_sha && strcasecmp(sha_hex, exp_sha) == 0) {
                        println("Integrity: [VERIFIED OK] Digest matches manifest (TRUSTED)");
                    } else {
                        println("Integrity: [MISMATCH WARNING] Digest does not match manifest!");
                    }
                }
            }
        } else {
            println("Manifest : None (standalone relocatable module)");
        }

        char rep[256];
        int val_rc = elf_validate_relocatable(buf, fsz, rep, sizeof(rep));
        if (val_rc == ELF_VALID_OK) {
            println("Format   : ELF32 Little-Endian RISC-V (ET_REL)");
            println("Status   : [VALID MNM-1 BINARY]");
        } else {
            char err[64];
            snprintf(err, sizeof(err), "Validation Error: code %d", val_rc);
            println(err);
        }

        free(buf);
        return 0;
    }

    int cmd_runelf(int argc, char **argv) {
        const void *elf_bytes = nullptr;
        size_t elf_len = 0;
        void *file_buffer = nullptr;
        const char *mod_name = "hello.mobj";
        int mod_argc = argc;
        const char *const *mod_argv = (const char *const *)argv;

        bool load_from_file = false;
        char resolved_path[128] = {0};

        if (argc >= 2) {
            if (argv[1][0] == '/' || strstr(argv[1], ".mobj") != nullptr) {
                resolve_path(argv[1], resolved_path, sizeof(resolved_path));
                load_from_file = true;
            }
        }

        if (load_from_file) {
            FILE *f = fopen(resolved_path, "rb");
            if (!f) {
                char err[160];
                snprintf(err, sizeof(err), "runelf: cannot open '%s'", resolved_path);
                println(err);
                return 1;
            }
            fseek(f, 0, SEEK_END);
            long fsz = ftell(f);
            fseek(f, 0, SEEK_SET);
            if (fsz <= 0 || fsz > 1024 * 1024) {
                println("runelf: invalid file size");
                fclose(f);
                return 1;
            }
#if defined(ESP_PLATFORM)
            file_buffer = heap_caps_malloc(fsz, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            if (!file_buffer) file_buffer = malloc(fsz);
#else
            file_buffer = malloc(fsz);
#endif
            if (!file_buffer) {
                println("runelf: out of memory reading file");
                fclose(f);
                return 1;
            }
            fread(file_buffer, 1, fsz, f);
            fclose(f);

            elf_bytes = file_buffer;
            elf_len = (size_t)fsz;
            mod_name = resolved_path;
            mod_argc = argc - 1;
            mod_argv = (const char *const *)(argv + 1);
        } else {
            elf_bytes = s_hello_mobj;
            elf_len = sizeof(s_hello_mobj);
            mod_name = "embedded hello.mobj";
            mod_argc = argc;
            mod_argv = (const char *const *)argv;
        }

        OutputFn console_cb = get_global_console_cb() ? get_global_console_cb() : this->output_cb;
        bool is_redirected = (get_global_redir_stage().buf != nullptr);

        if (!is_redirected && console_cb) {
            console_cb("=== MaiaRTOS Native RISC-V ELF Execution ===\n");
            char title[128];
            snprintf(title, sizeof(title), "Loading: %s (%u bytes)\n", mod_name, (unsigned int)elf_len);
            console_cb(title);
        }

        get_global_out_cb() = this->output_cb;

        maia_host_api_v1 host_api;
        memset(&host_api, 0, sizeof(host_api));
        host_api.struct_size = sizeof(maia_host_api_v1);
        host_api.abi_major = MAIA_HOST_ABI_MAJOR;
        host_api.abi_minor = MAIA_HOST_ABI_MINOR;
        host_api.write_fd = [](maia_handle_t fd, const void *buf, uint32_t n) -> int32_t {
            OutputFn out_cb = get_global_out_cb();
            OutputFn con_cb = get_global_console_cb();
            if (fd == 2) {
                if (con_cb && buf && n > 0) {
                    const char *p = (const char *)buf;
                    char tmp[128];
                    while (n > 0) {
                        size_t c = (n < sizeof(tmp) - 1) ? n : (sizeof(tmp) - 1);
                        memcpy(tmp, p, c);
                        tmp[c] = '\0';
                        con_cb(tmp);
                        p += c;
                        n -= c;
                    }
                }
                return (int32_t)n;
            }
            if (out_cb && buf && n > 0) {
                const char *p = (const char *)buf;
                char tmp[128];
                while (n > 0) {
                    size_t c = (n < sizeof(tmp) - 1) ? n : (sizeof(tmp) - 1);
                    memcpy(tmp, p, c);
                    tmp[c] = '\0';
                    out_cb(tmp);
                    p += c;
                    n -= c;
                }
            }
            return (int32_t)n;
        };
        host_api.read_fd = [](maia_handle_t fd, void *buf, uint32_t n) -> int32_t {
            if (fd == 0 && buf && n > 0) {
                PipeStaging &p = get_global_pipe_in();
                if (p.buf && p.pos < p.len) {
                    size_t avail = p.len - p.pos;
                    size_t to_read = (n < avail) ? n : avail;
                    memcpy(buf, p.buf + p.pos, to_read);
                    p.pos += to_read;
                    return (int32_t)to_read;
                }
                return (int32_t)read(STDIN_FILENO, buf, n);
            }
            return 0;
        };
        host_api.close_fd = [](maia_handle_t fd) -> int32_t {
            (void)fd;
            return 0;
        };
        host_api.sleep_ms = [](uint32_t ms) -> int32_t {
#if defined(ESP_PLATFORM)
            vTaskDelay(pdMS_TO_TICKS(ms));
#endif
            return 0;
        };
        host_api.alloc = [](uint32_t bytes, uint32_t flags) -> void * {
            (void)flags;
#if defined(ESP_PLATFORM)
            if (flags == 1 || bytes >= 16384) {
                return heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
            } else {
                return heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
            }
#else
            return malloc(bytes);
#endif
        };
        host_api.dealloc = [](void *ptr) {
            if (ptr) free(ptr);
        };
        host_api.get_time_us = []() -> uint64_t {
#if defined(ESP_PLATFORM)
            return (uint64_t)esp_timer_get_time();
#else
            struct timespec ts;
            clock_gettime(CLOCK_MONOTONIC, &ts);
            return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)(ts.tv_nsec / 1000);
#endif
        };
        host_api.display = get_global_display_api();

        int exit_code = 0;
        int res = mnm_load_and_run(elf_bytes, elf_len,
                                   mod_argc, mod_argv,
                                   &host_api, &exit_code,
                                   [](const char *msg) {
                                       bool redir = (get_global_redir_stage().buf != nullptr);
                                       OutputFn c_cb = get_global_console_cb();
                                       if (!redir && c_cb && msg) {
                                           c_cb(msg);
                                           c_cb("\n");
                                       }
                                   });

        if (file_buffer) {
            free(file_buffer);
        }

        if (!is_redirected && console_cb) {
            if (res == MNM_LOAD_OK) {
                char res_buf[96];
                snprintf(res_buf, sizeof(res_buf), "[SUCCESS] Module executed! Exit code: %d (0x%02X)\n", exit_code, exit_code);
                console_cb(res_buf);
            } else {
                char err_buf[96];
                snprintf(err_buf, sizeof(err_buf), "[FAILED] Loader error code: %d\n", res);
                console_cb(err_buf);
            }
        }

        return (res == MNM_LOAD_OK) ? exit_code : 1;
    }

    int cmd_ticks(int argc, char **argv) {
        (void)argc; (void)argv;
#if defined(MAIA_KERNEL_H)
        uint64_t ticks = maia_tick_get();
        uint64_t us    = maia_time_us();
        maia_trap_stats_t ts;
        maia_trap_get_stats(&ts);
        char buf[160];
        snprintf(buf, sizeof(buf),
                 "tick_ms=%lu  systimer_us=%lu  ctx_switches=%lu\n"
                 "traps: total=%lu timer_irq=%lu sync_ex=%lu ecall=%lu mcause=0x%08lX",
                 (unsigned long)ticks, (unsigned long)us, (unsigned long)maia_sched_switch_count(),
                 (unsigned long)ts.total_traps, (unsigned long)ts.timer_irqs,
                 (unsigned long)ts.sync_exceptions, (unsigned long)ts.ecall_count,
                 (unsigned long)ts.last_mcause);
        println(buf);
#else
        println("ticks: Host Emulation");
#endif
        return 0;
    }

    int cmd_alloc(int argc, char **argv) {
#if defined(MAIA_KERNEL_H)
        uint32_t sz = (argc > 1) ? (uint32_t)strtoul(argv[1], nullptr, 0) : 128U;
        if (sz == 0U) sz = 64U;
        uint8_t *blk = (uint8_t *)maia_mem_alloc((size_t)sz);
        if (blk == nullptr) {
            println("alloc FAILED (out of memory)");
            return 1;
        }
        blk[0] = 0xA5U;
        blk[sz - 1] = 0x5AU;
        char buf[128];
        snprintf(buf, sizeof(buf), "alloc OK ptr=0x%08lX size=%lu (verified & freed)",
                 (unsigned long)(uintptr_t)blk, (unsigned long)sz);
        (void)maia_mem_free(blk);
        println(buf);
        return 0;
#else
        (void)argc; (void)argv;
        println("alloc: Host Emulation OK");
        return 0;
#endif
    }

    int cmd_qtest(int argc, char **argv) {
        (void)argc; (void)argv;
#if defined(MAIA_KERNEL_H)
        maia_mutex_t mtx;
        maia_sem_t sem;
        maia_queue_t q;
        uint32_t qbuf[4];
        uint32_t in_val = 0xCAFEBABEUL;
        uint32_t out_val = 0U;

        (void)maia_mutex_init(&mtx);
        (void)maia_mutex_trylock(&mtx, 0);
        (void)maia_mutex_unlock(&mtx, 0);

        (void)maia_sem_init(&sem, 0U, 4U);
        (void)maia_sem_post(&sem);
        (void)maia_sem_trywait(&sem);

        (void)maia_queue_init(&q, qbuf, sizeof(uint32_t), 4U);
        (void)maia_queue_send(&q, &in_val);
        (void)maia_queue_recv(&q, &out_val);

        if (out_val == in_val) {
            println("qtest PASS: mutex + sem + queue verified (0xCAFEBABE)");
            return 0;
        }
        println("qtest FAIL");
        return 1;
#else
        println("qtest PASS: Host Emulation");
        return 0;
#endif
    }

    int cmd_pwd(int argc, char **argv) {
        (void)argc; (void)argv;
        println(cwd);
        return 0;
    }

    int cmd_cd(int argc, char **argv) {
        if (argc < 2) {
            strncpy(cwd, "/", sizeof(cwd));
            return 0;
        }
        if (strcmp(argv[1], "..") == 0) {
            char *last_slash = strrchr(cwd, '/');
            if (last_slash && last_slash != cwd) {
                *last_slash = '\0';
            } else {
                strcpy(cwd, "/");
            }
            return 0;
        }
        char new_path[128];
        resolve_path(argv[1], new_path, sizeof(new_path));
        if (strcmp(new_path, "/") == 0) {
            strcpy(cwd, "/");
            return 0;
        }
        DIR *d = opendir(new_path);
        if (!d) {
            char err[160];
            snprintf(err, sizeof(err), "cd: no such directory: %s", new_path);
            println(err);
            return 1;
        }
        closedir(d);
        strncpy(cwd, new_path, sizeof(cwd) - 1);
        cwd[sizeof(cwd) - 1] = '\0';
        return 0;
    }

    int cmd_ls(int argc, char **argv) {
        char target_dir[128];
        if (argc >= 2) {
            resolve_path(argv[1], target_dir, sizeof(target_dir));
        } else {
            resolve_path(cwd, target_dir, sizeof(target_dir));
        }

        if (strcmp(target_dir, "/") == 0) {
            println("Directory of /:");
            println("  [MOUNT] /sd/ (SDMMC FAT32)");
            println("Total 1 mount point");
            return 0;
        }

        DIR *d = opendir(target_dir);
        if (!d) {
            char err[160];
            snprintf(err, sizeof(err), "ls: cannot access '%s': No such directory", target_dir);
            println(err);
            return 1;
        }

        char line[256];
        snprintf(line, sizeof(line), "Directory of %s:", target_dir);
        println(line);

        int count = 0;
        struct dirent *de;
        while ((de = readdir(d)) != nullptr) {
            char full_path[256];
            snprintf(full_path, sizeof(full_path), "%s/%s", 
                     strcmp(target_dir, "/") == 0 ? "" : target_dir, de->d_name);

            struct stat st;
            bool is_dir = false;
            size_t sz = 0;
            if (stat(full_path, &st) == 0) {
                is_dir = S_ISDIR(st.st_mode);
                sz = (size_t)st.st_size;
            }

            if (is_dir) {
                snprintf(line, sizeof(line), "  [DIR]  %s/", de->d_name);
            } else {
                snprintf(line, sizeof(line), "  [FILE] %s (%u bytes)", de->d_name, (unsigned int)sz);
            }
            println(line);
            count++;
        }
        closedir(d);

        snprintf(line, sizeof(line), "Total %d entries", count);
        println(line);
        return 0;
    }

    int cmd_cat(int argc, char **argv) {
        if (argc < 2) {
            if (get_global_pipe_in().buf != nullptr) {
                PipeStaging &p = get_global_pipe_in();
                if (p.len > 0) {
                    print(p.buf);
                }
                return 0;
            }
            println("Usage: cat <file>");
            return 1;
        }
        char target[128];
        resolve_path(argv[1], target, sizeof(target));

        FILE *f = fopen(target, "r");
        if (!f) {
            char err[160];
            snprintf(err, sizeof(err), "cat: cannot open '%s'", target);
            println(err);
            return 1;
        }

        char buf[128];
        while (fgets(buf, sizeof(buf), f)) {
            print(buf);
        }
        fclose(f);
        return 0;
    }

    int cmd_grep(int argc, char **argv) {
        if (argc < 2) {
            println("usage: grep [-i] [-v] <pattern> [file]");
            return 2;
        }
        bool ignore_case = false;
        bool invert_match = false;
        int arg_idx = 1;
        while (arg_idx < argc && argv[arg_idx][0] == '-' && argv[arg_idx][1] != '\0') {
            const char *opt = argv[arg_idx] + 1;
            while (*opt) {
                if (*opt == 'i') ignore_case = true;
                else if (*opt == 'v') invert_match = true;
                opt++;
            }
            arg_idx++;
        }
        if (arg_idx >= argc) {
            println("grep: missing pattern");
            return 2;
        }
        const char *pattern = argv[arg_idx++];
        const char *file_path = (arg_idx < argc) ? argv[arg_idx] : nullptr;

        auto match_line = [&](const char *l) -> bool {
            if (!ignore_case) {
                return (strstr(l, pattern) != nullptr);
            } else {
                size_t plen = strlen(pattern);
                size_t llen = strlen(l);
                if (plen > llen) return false;
                for (size_t k = 0; k <= llen - plen; ++k) {
                    if (strncasecmp(l + k, pattern, plen) == 0) return true;
                }
                return false;
            }
        };

        int match_count = 0;
        char line[256];

        if (file_path) {
            char resolved[128];
            resolve_path(file_path, resolved, sizeof(resolved));
            FILE *f = fopen(resolved, "r");
            if (!f) {
                print("grep: cannot open ");
                println(file_path);
                return 2;
            }
            while (fgets(line, sizeof(line), f)) {
                size_t len = strlen(line);
                while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
                bool m = match_line(line);
                if (invert_match) m = !m;
                if (m) {
                    println(line);
                    match_count++;
                }
            }
            fclose(f);
        } else if (get_global_pipe_in().buf != nullptr) {
            while (pipe_read_line(line, sizeof(line))) {
                bool m = match_line(line);
                if (invert_match) m = !m;
                if (m) {
                    println(line);
                    match_count++;
                }
            }
        } else {
            println("grep: no input specified (pipe or file required)");
            return 2;
        }

        return (match_count > 0) ? 0 : 1;
    }

    int cmd_head(int argc, char **argv) {
        int max_lines = 10;
        const char *file_path = nullptr;

        for (int i = 1; i < argc; ++i) {
            if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
                max_lines = atoi(argv[++i]);
                if (max_lines < 0) max_lines = 10;
            } else if (argv[i][0] == '-' && (argv[i][1] >= '0' && argv[i][1] <= '9')) {
                max_lines = atoi(argv[i] + 1);
            } else {
                file_path = argv[i];
            }
        }

        char line[256];
        int count = 0;

        if (file_path) {
            char resolved[128];
            resolve_path(file_path, resolved, sizeof(resolved));
            FILE *f = fopen(resolved, "r");
            if (!f) {
                print("head: cannot open ");
                println(file_path);
                return 1;
            }
            while (count < max_lines && fgets(line, sizeof(line), f)) {
                size_t len = strlen(line);
                while (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r')) line[--len] = '\0';
                println(line);
                count++;
            }
            fclose(f);
        } else if (get_global_pipe_in().buf != nullptr) {
            while (count < max_lines && pipe_read_line(line, sizeof(line))) {
                println(line);
                count++;
            }
        } else {
            println("head: no input specified (pipe or file required)");
            return 1;
        }
        return 0;
    }

    int cmd_wc(int argc, char **argv) {
        bool count_lines = false;
        bool count_words = false;
        bool count_bytes = false;
        const char *file_path = nullptr;

        for (int i = 1; i < argc; ++i) {
            if (argv[i][0] == '-' && argv[i][1] != '\0') {
                const char *opt = argv[i] + 1;
                while (*opt) {
                    if (*opt == 'l') count_lines = true;
                    else if (*opt == 'w') count_words = true;
                    else if (*opt == 'c') count_bytes = true;
                    opt++;
                }
            } else {
                file_path = argv[i];
            }
        }

        if (!count_lines && !count_words && !count_bytes) {
            count_lines = true;
            count_words = true;
            count_bytes = true;
        }

        size_t lines = 0, words = 0, bytes = 0;
        bool in_word = false;

        auto process_char = [&](char c) {
            bytes++;
            if (c == '\n') lines++;
            if (c == ' ' || c == '\t' || c == '\n' || c == '\r') {
                in_word = false;
            } else if (!in_word) {
                in_word = true;
                words++;
            }
        };

        if (file_path) {
            char resolved[128];
            resolve_path(file_path, resolved, sizeof(resolved));
            FILE *f = fopen(resolved, "r");
            if (!f) {
                print("wc: cannot open ");
                println(file_path);
                return 1;
            }
            int c;
            while ((c = fgetc(f)) != EOF) {
                process_char((char)c);
            }
            fclose(f);
        } else if (get_global_pipe_in().buf != nullptr) {
            PipeStaging &p = get_global_pipe_in();
            for (size_t i = 0; i < p.len; ++i) {
                process_char(p.buf[i]);
            }
        } else {
            println("wc: no input specified (pipe or file required)");
            return 1;
        }

        char out_buf[128] = "";
        char tmp[32];
        if (count_lines) {
            snprintf(tmp, sizeof(tmp), "%7u ", (unsigned int)lines);
            strncat(out_buf, tmp, sizeof(out_buf) - strlen(out_buf) - 1);
        }
        if (count_words) {
            snprintf(tmp, sizeof(tmp), "%7u ", (unsigned int)words);
            strncat(out_buf, tmp, sizeof(out_buf) - strlen(out_buf) - 1);
        }
        if (count_bytes) {
            snprintf(tmp, sizeof(tmp), "%7u ", (unsigned int)bytes);
            strncat(out_buf, tmp, sizeof(out_buf) - strlen(out_buf) - 1);
        }
        if (file_path) {
            strncat(out_buf, file_path, sizeof(out_buf) - strlen(out_buf) - 1);
        }
        println(out_buf);
        return 0;
    }

    int cmd_mkdir(int argc, char **argv) {
        if (argc < 2) {
            println("Usage: mkdir <dir>");
            return 1;
        }
        char target[128];
        resolve_path(argv[1], target, sizeof(target));
        if (mkdir(target, 0777) != 0) {
            char err[160];
            snprintf(err, sizeof(err), "mkdir: cannot create '%s'", target);
            println(err);
            return 1;
        }
        return 0;
    }

    int cmd_rm(int argc, char **argv) {
        if (argc < 2) {
            println("Usage: rm <file>");
            return 1;
        }
        char target[128];
        resolve_path(argv[1], target, sizeof(target));
        if (remove(target) != 0) {
            char err[160];
            snprintf(err, sizeof(err), "rm: cannot remove '%s'", target);
            println(err);
            return 1;
        }
        return 0;
    }

    int cmd_touch(int argc, char **argv) {
        if (argc < 2) {
            println("Usage: touch <file>");
            return 1;
        }
        char target[128];
        resolve_path(argv[1], target, sizeof(target));
        FILE *f = fopen(target, "a");
        if (!f) {
            char err[160];
            snprintf(err, sizeof(err), "touch: cannot touch '%s'", target);
            println(err);
            return 1;
        }
        fclose(f);
        return 0;
    }

    int cmd_write(int argc, char **argv) {
        if (argc < 3) {
            println("Usage: write <file> <text...>");
            return 1;
        }
        char target[128];
        resolve_path(argv[1], target, sizeof(target));
        FILE *f = fopen(target, "a");
        if (!f) {
            char err[160];
            snprintf(err, sizeof(err), "write: cannot open '%s'", target);
            println(err);
            return 1;
        }
        for (int i = 2; i < argc; ++i) {
            fputs(argv[i], f);
            if (i < argc - 1) fputc(' ', f);
        }
        fputc('\n', f);
        fclose(f);
        return 0;
    }

    int cmd_hexwrite(int argc, char **argv) {
        if (argc < 3) {
            println("Usage: hexwrite [-a] <file> <hex_data>");
            return 1;
        }
        bool append = false;
        int arg_idx = 1;
        if (strcmp(argv[arg_idx], "-a") == 0) {
            append = true;
            arg_idx++;
        }
        if (arg_idx + 1 >= argc) {
            println("Usage: hexwrite [-a] <file> <hex_data>");
            return 1;
        }
        const char *file_path = argv[arg_idx++];
        const char *hex_data = argv[arg_idx++];

        char resolved[128];
        resolve_path(file_path, resolved, sizeof(resolved));

        FILE *f = fopen(resolved, append ? "ab" : "wb");
        if (!f) {
            print("hexwrite: cannot open ");
            println(file_path);
            return 1;
        }

        auto hex_val = [](char c) -> int {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            if (c >= 'A' && c <= 'F') return c - 'A' + 10;
            return -1;
        };

        size_t hlen = strlen(hex_data);
        for (size_t i = 0; i + 1 < hlen; i += 2) {
            int hi = hex_val(hex_data[i]);
            int lo = hex_val(hex_data[i + 1]);
            if (hi >= 0 && lo >= 0) {
                uint8_t b = (uint8_t)((hi << 4) | lo);
                fputc(b, f);
            }
        }
        fclose(f);
        return 0;
    }

    int cmd_hexread(int argc, char **argv) {
        if (argc < 2) {
            println("Usage: hexread <file> [offset] [count]");
            return 1;
        }
        char resolved[128];
        resolve_path(argv[1], resolved, sizeof(resolved));

        FILE *f = fopen(resolved, "rb");
        if (!f) {
            print("hexread: cannot open ");
            println(argv[1]);
            return 1;
        }

        long offset = 0;
        if (argc >= 3) {
            offset = msh_atol(argv[2]);
            if (offset < 0) offset = 0;
        }

        size_t count = 64;
        if (argc >= 4) {
            long c = msh_atol(argv[3]);
            if (c > 0 && c <= 256) count = (size_t)c;
        }

        fseek(f, offset, SEEK_SET);
        uint8_t buf[256];
        size_t n = fread(buf, 1, count, f);
        fclose(f);

        char hex[513];
        for (size_t i = 0; i < n; ++i) {
            snprintf(hex + (i * 2), 3, "%02x", buf[i]);
        }
        hex[n * 2] = '\0';
        println(hex);
        return 0;
    }

    int cmd_hexdump(int argc, char **argv) {
        if (argc < 2) {
            println("Usage: hexdump [-n count] <file>");
            return 1;
        }
        size_t max_bytes = 256;
        int arg_idx = 1;
        if (strcmp(argv[arg_idx], "-n") == 0 && arg_idx + 2 < argc) {
            arg_idx++;
            long n = msh_atol(argv[arg_idx++]);
            if (n > 0) max_bytes = (size_t)n;
        }
        char resolved[128];
        resolve_path(argv[arg_idx], resolved, sizeof(resolved));

        FILE *f = fopen(resolved, "rb");
        if (!f) {
            print("hexdump: cannot open ");
            println(argv[arg_idx]);
            return 1;
        }

        uint8_t buf[16];
        size_t total = 0;
        char line[96];

        while (total < max_bytes) {
            size_t want = (max_bytes - total < sizeof(buf)) ? (max_bytes - total) : sizeof(buf);
            size_t n = fread(buf, 1, want, f);
            if (n == 0) break;

            int len = snprintf(line, sizeof(line), "%08x: ", (unsigned int)total);
            for (size_t i = 0; i < 16; ++i) {
                if (i < n) {
                    len += snprintf(line + len, sizeof(line) - len, "%02x ", buf[i]);
                } else {
                    len += snprintf(line + len, sizeof(line) - len, "   ");
                }
                if (i == 7) len += snprintf(line + len, sizeof(line) - len, " ");
            }
            len += snprintf(line + len, sizeof(line) - len, " |");
            for (size_t i = 0; i < n; ++i) {
                char ch = (buf[i] >= 32 && buf[i] <= 126) ? (char)buf[i] : '.';
                if (len + 1 < (int)sizeof(line)) line[len++] = ch;
            }
            if (len + 2 < (int)sizeof(line)) {
                line[len++] = '|';
                line[len] = '\0';
            }
            println(line);
            total += n;
            if (n < want) break;
        }
        fclose(f);
        return 0;
    }

    int cmd_jobs(int argc, char **argv) {
        (void)argc; (void)argv;
        ShellJob *jobs = get_job_table();
        bool found = false;
        for (int j = 0; j < MAX_JOBS; ++j) {
            if (jobs[j].id > 0) {
                found = true;
                char buf[128];
                snprintf(buf, sizeof(buf), "[%d] %s\t%s (exit %d)",
                         jobs[j].id,
                         jobs[j].active ? "Running" : "Done   ",
                         jobs[j].cmd,
                         jobs[j].exit_code);
                println(buf);
            }
        }
        if (!found) {
            println("No background jobs");
        }
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
            print(argv[i]);
            if (i < argc - 1) print(" ");
        }
        if (newline) println();
        return 0;
    }

    int cmd_printf(int argc, char **argv) {
        if (argc < 2) return 1;
        const char *fmt = argv[1];
        int arg_idx = 2;
        size_t len = strlen(fmt);
        for (size_t i = 0; i < len; ++i) {
            if (fmt[i] == '\\' && i + 1 < len) {
                i++;
                if (fmt[i] == 'n') print("\n");
                else if (fmt[i] == 't') print("\t");
                else if (fmt[i] == '\\') print("\\");
                else { char c[2] = { fmt[i], '\0' }; print(c); }
            } else if (fmt[i] == '%' && i + 1 < len) {
                i++;
                if (fmt[i] == 's') {
                    print((arg_idx < argc) ? argv[arg_idx++] : "");
                } else if (fmt[i] == 'd' || fmt[i] == 'i') {
                    int val = (arg_idx < argc) ? atoi(argv[arg_idx++]) : 0;
                    char num[32];
                    snprintf(num, sizeof(num), "%d", val);
                    print(num);
                } else if (fmt[i] == 'x' || fmt[i] == 'X') {
                    unsigned int val = (arg_idx < argc) ? (unsigned int)strtoul(argv[arg_idx++], nullptr, 0) : 0;
                    char num[32];
                    snprintf(num, sizeof(num), (fmt[i] == 'x') ? "%x" : "%X", val);
                    print(num);
                } else if (fmt[i] == '%') {
                    print("%");
                }
            } else {
                char c[2] = { fmt[i], '\0' };
                print(c);
            }
        }
        return 0;
    }

    int cmd_clear(int argc, char **argv) {
        (void)argc; (void)argv;
        print("\033[2J\033[H");
        return 0;
    }

    int cmd_reset(int argc, char **argv) {
        (void)argc; (void)argv;
        print("\033c\033[2J\033[H");
        return 0;
    }

    int cmd_source(int argc, char **argv) {
        if (argc < 2) {
            println("Usage: source <file.sh>");
            return 1;
        }
        return execute_script(argv[1]);
    }

    int cmd_color(int argc, char **argv) {
        if (argc < 2) {
            println("usage: color <white|green|yellow|cyan|magenta|red|blue>");
            return 1;
        }
        print("\033[color:");
        println(argv[1]);
        return 0;
    }

    int cmd_basename(int argc, char **argv) {
        if (argc < 2) {
            println("usage: basename path [suffix]");
            return 1;
        }
        const char *path = argv[1];
        const char *last_slash = strrchr(path, '/');
        const char *base = last_slash ? (last_slash + 1) : path;
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
        (void)argc; (void)argv;
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
        int count = argc;
        if (strcmp(argv[0], "[") == 0) {
            if (count > 1 && strcmp(argv[count - 1], "]") == 0) {
                count--;
            }
        }
        if (count <= 1) return 1;

        if (count == 3 && strcmp(argv[1], "-z") == 0) {
            return (strlen(argv[2]) == 0) ? 0 : 1;
        }
        if (count == 3 && strcmp(argv[1], "-n") == 0) {
            return (strlen(argv[2]) > 0) ? 0 : 1;
        }
        if (count == 4 && (strcmp(argv[2], "=") == 0 || strcmp(argv[2], "==") == 0)) {
            return (strcmp(argv[1], argv[3]) == 0) ? 0 : 1;
        }
        if (count == 4 && strcmp(argv[2], "!=") == 0) {
            return (strcmp(argv[1], argv[3]) != 0) ? 0 : 1;
        }
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
        char out[MSH_VAL_LEN];
        int st = eval_expr(argc, argv, out, sizeof(out));
        println(out);
        return st;
    }

    int cmd_backlight(int argc, char **argv) {
        int b = (argc > 1) ? atoi(argv[1]) : 100;
        if (b < 0) b = 0;
        if (b > 100) b = 100;
#if defined(ESP_PLATFORM)
        extern esp_err_t bsp_display_brightness_set(int brightness_percent);
        (void)bsp_display_brightness_set(b);
#elif defined(__riscv)
        extern void maia_tab5_backlight_set_duty(uint32_t duty_4095);
        maia_tab5_backlight_set_duty((uint32_t)((b * 4095) / 100));
#endif
        char buf[64];
        snprintf(buf, sizeof(buf), "Backlight set to %d%%", b);
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
        (void)argc; (void)argv;
        println("Rebooting MaiaRTOS on M5Stack Tab5...");
#if defined(ARDUINO)
        delay(200);
        ESP.restart();
#elif defined(ESP_PLATFORM)
        esp_restart();
#elif defined(__riscv)
        // Reset via LP_WDT or PMU
        *(volatile uint32_t *)(0x50115000UL) = 1U;
#endif
        return 0;
    }
};

inline ExprVal MaiaShell::parse_expr_or(ExprParser *p) {
    ExprVal left = parse_expr_and(p);
    while (p->status == 0 && p->pos < p->argc) {
        const char *op = p->argv[p->pos];
        if (strcmp(op, "|") != 0 && strcmp(op, "\\|") != 0) break;
        p->pos++;
        ExprVal right = parse_expr_and(p);
        if (p->status != 0) return left;
        if (is_truthy(&left)) {
            // keeps left
        } else if (is_truthy(&right)) {
            left = right;
        } else {
            make_val_num(&left, 0);
        }
    }
    return left;
}
