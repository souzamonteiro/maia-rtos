#include <maia/exec/mnm.h>

static int parse_int(const char *s) {
    int res = 0;
    int sign = 1;
    if (*s == '-') {
        sign = -1;
        s++;
    } else if (*s == '+') {
        s++;
    }
    while (*s >= '0' && *s <= '9') {
        res = res * 10 + (*s - '0');
        s++;
    }
    return res * sign;
}

static uint32_t fmt_int(int val, char *dst) {
    char tmp[16];
    uint32_t t_len = 0;
    int sign = 0;
    if (val < 0) {
        sign = 1;
        val = -val;
    } else if (val == 0) {
        dst[0] = '0';
        return 1;
    }
    while (val > 0) {
        tmp[t_len++] = '0' + (val % 10);
        val /= 10;
    }
    uint32_t pos = 0;
    if (sign) {
        dst[pos++] = '-';
    }
    while (t_len > 0) {
        dst[pos++] = tmp[--t_len];
    }
    return pos;
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host || !ctx->host->write_fd) {
        return 1;
    }

    if (ctx->argc < 4) {
        const char usage[] = "Usage: calc <num1> <op: +|-|*|/> <num2>\nExample: calc 150 + 270\n";
        ctx->host->write_fd(ctx->stdout_fd, usage, sizeof(usage) - 1);
        return 1;
    }

    int a = parse_int(ctx->argv[1]);
    char op = ctx->argv[2][0];
    int b = parse_int(ctx->argv[3]);
    int res = 0;

    switch (op) {
        case '+': res = a + b; break;
        case '-': res = a - b; break;
        case '*': res = a * b; break;
        case '/':
            if (b == 0) {
                const char err[] = "Error: Division by zero!\n";
                ctx->host->write_fd(ctx->stdout_fd, err, sizeof(err) - 1);
                return 2;
            }
            res = a / b;
            break;
        default: {
            const char err[] = "Error: Unknown operator. Use +, -, * or /\n";
            ctx->host->write_fd(ctx->stdout_fd, err, sizeof(err) - 1);
            return 3;
        }
    }

    // Demonstrate dynamic memory allocation via Host API (alloc / dealloc)
    if (ctx->host->alloc && ctx->host->dealloc) {
        char *buf = (char *)ctx->host->alloc(128, 0);
        if (buf) {
            const char pfx[] = "Native Calc [RISC-V @ L2MEM]: ";
            uint32_t pos = 0;
            while (pfx[pos]) {
                buf[pos] = pfx[pos];
                pos++;
            }

            pos += fmt_int(a, buf + pos);
            buf[pos++] = ' ';
            buf[pos++] = op;
            buf[pos++] = ' ';
            pos += fmt_int(b, buf + pos);
            buf[pos++] = ' ';
            buf[pos++] = '=';
            buf[pos++] = ' ';
            pos += fmt_int(res, buf + pos);
            buf[pos++] = '\n';
            buf[pos] = '\0';

            ctx->host->write_fd(ctx->stdout_fd, buf, pos);

            // Free allocated host memory
            ctx->host->dealloc(buf);
            return 0;
        }
    }

    // Fallback if alloc is not implemented
    const char done[] = "Calculation computed.\n";
    ctx->host->write_fd(ctx->stdout_fd, done, sizeof(done) - 1);
    return 0;
}
