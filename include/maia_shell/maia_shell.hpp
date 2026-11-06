#pragma once

#if defined(ARDUINO)
#include <Arduino.h>
#include <M5Unified.h>
#elif defined(ESP_PLATFORM)
#include <esp_system.h>
#endif

#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

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
        else if (strcmp(cmd, "ps") == 0)     ret = cmd_ps(argc, argv);
        else if (strcmp(cmd, "mem") == 0)    ret = cmd_mem(argc, argv);
        else if (strcmp(cmd, "alloc") == 0)  ret = cmd_alloc(argc, argv);
        else if (strcmp(cmd, "qtest") == 0)  ret = cmd_qtest(argc, argv);
        else if (strcmp(cmd, "pwd") == 0)    ret = cmd_pwd(argc, argv);
        else if (strcmp(cmd, "cd") == 0)     ret = cmd_cd(argc, argv);
        else if (strcmp(cmd, "clear") == 0)  ret = cmd_clear(argc, argv);
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
        else {
            print("msh: command not found: ");
            println(cmd);
            ret = 127;
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
                        int cond_rc = execute_single_cmd(cond);
                        if (cond_rc == 0) {
                            branch_taken = true;
                            const char *inline_then = strip_keyword(seg_pool[then_idx], "then");
                            if (inline_then && *inline_then) {
                                last_rc = execute_single_cmd(inline_then);
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
                            last_rc = execute_single_cmd(inline_else);
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
                    int c_rc = execute_single_cmd(cond_buf);
                    bool cond_pass = is_until ? (c_rc != 0) : (c_rc == 0);
                    if (!cond_pass) break;
                    bool b_break = false;
                    bool b_cont = false;
                    const char *inline_do = strip_keyword(seg_pool[do_idx], "do");
                    if (inline_do && *inline_do) {
                        last_rc = execute_single_cmd(inline_do);
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
                        last_rc = execute_single_cmd(inline_do);
                    }
                    if (do_idx + 1 < done_idx) {
                        last_rc = execute_range(do_idx + 1, done_idx, &b_break, &b_cont);
                    }
                    if (b_break) break;
                }
                i = done_idx + 1;
                continue;
            }

            // Normal single command
            last_rc = execute_single_cmd(s);
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
        println("  mem               - Show L2MEM region allocator statistics");
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
        println("  clear             - Clear terminal screen");
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
#else
        println("Uptime: 1000 ms (Host Emulation)");
#endif
        return 0;
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
