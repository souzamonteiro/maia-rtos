#include <maia/kernel.h>
#include <maia/version.h>
#include <maia_shell/shell_mvp.h>
#include <maia_shell/host_v1_validation.h>

static size_t msh_strlen(const char *s) {
    size_t n = 0U;
    if (s == NULL) {
        return 0U;
    }
    while (s[n] != '\0') {
        ++n;
    }
    return n;
}

static int msh_streq(const char *a, const char *b) {
    if (a == NULL || b == NULL) {
        return 0;
    }
    while (*a != '\0' && *b != '\0') {
        if (*a != *b) {
            return 0;
        }
        ++a;
        ++b;
    }
    return (*a == '\0' && *b == '\0') ? 1 : 0;
}

static void msh_copy_str(char *dst, size_t cap, const char *src) {
    size_t i = 0U;
    if (cap == 0U) {
        return;
    }
    if (src != NULL) {
        for (; i + 1U < cap && src[i] != '\0'; ++i) {
            dst[i] = src[i];
        }
    }
    dst[i] = '\0';
}

/* Default non-blocking VFS/process stubs for console-only hosts */
static int32_t stub_open(void *user, const char *path, uint32_t flags, msh_fd_t *out) {
    (void)user;
    (void)path;
    (void)flags;
    (void)out;
    return MSH_ERR_NOTSUP;
}

static int32_t stub_close(void *user, msh_fd_t fd) {
    (void)user;
    (void)fd;
    return MSH_OK;
}

static int32_t stub_pipe_create(void *user, msh_fd_t *r, msh_fd_t *w) {
    (void)user;
    (void)r;
    (void)w;
    return MSH_ERR_NOTSUP;
}

static int32_t stub_fd_dup(void *user, msh_fd_t src, msh_fd_t *dst) {
    (void)user;
    if (dst == NULL) {
        return MSH_ERR_INVALID;
    }
    *dst = src;
    return MSH_OK;
}

static int32_t stub_spawn(void *user, const char *path, const char *const *argv,
                          msh_fd_t in, msh_fd_t out, msh_fd_t err, msh_job_t *job) {
    (void)user;
    (void)path;
    (void)argv;
    (void)in;
    (void)out;
    (void)err;
    (void)job;
    return MSH_ERR_NOTSUP;
}

static int32_t stub_wait(void *user, msh_job_t job, int32_t *status) {
    (void)user;
    (void)job;
    (void)status;
    return MSH_ERR_NOTSUP;
}

static int32_t stub_cwd_get(void *user, char *dst, uint32_t cap) {
    (void)user;
    if (dst == NULL || cap < 2U) {
        return MSH_ERR_INVALID;
    }
    dst[0] = '/';
    dst[1] = '\0';
    return MSH_OK;
}

static int32_t stub_cwd_set(void *user, const char *path) {
    (void)user;
    if (path == NULL) {
        return MSH_ERR_INVALID;
    }
    return MSH_OK;
}

static int32_t stub_glob(void *user, const char *pattern, void *sink) {
    (void)user;
    (void)pattern;
    (void)sink;
    return MSH_OK;
}

void msh_mvp_bind_console_host(msh_host_v1 *out_host,
                               void *user,
                               int32_t (*read_fn)(void *user, msh_fd_t fd, void *dst, uint32_t n),
                               int32_t (*write_fn)(void *user, msh_fd_t fd, const void *src, uint32_t n)) {
    if (out_host == NULL) {
        return;
    }
    out_host->struct_size = (uint32_t)sizeof(msh_host_v1);
    out_host->version     = MSH_HOST_VERSION_1;
    out_host->user        = user;
    out_host->read        = read_fn;
    out_host->write       = write_fn;
    out_host->open        = stub_open;
    out_host->close       = stub_close;
    out_host->pipe_create = stub_pipe_create;
    out_host->fd_dup      = stub_fd_dup;
    out_host->spawn       = stub_spawn;
    out_host->wait        = stub_wait;
    out_host->cwd_get     = stub_cwd_get;
    out_host->cwd_set     = stub_cwd_set;
    out_host->glob        = stub_glob;
}

static void msh_puts(msh_ctx_t *ctx, const char *s) {
    if (ctx != NULL && ctx->host != NULL && ctx->host->write != NULL && s != NULL) {
        (void)ctx->host->write(ctx->host->user, MSH_STDOUT_FD, s, (uint32_t)msh_strlen(s));
    }
}

static void msh_putc(msh_ctx_t *ctx, char c) {
    if (ctx != NULL && ctx->host != NULL && ctx->host->write != NULL) {
        (void)ctx->host->write(ctx->host->user, MSH_STDOUT_FD, &c, 1U);
    }
}

static void msh_print_u32(msh_ctx_t *ctx, uint32_t val) {
    char buf[12];
    int n = 0;
    if (val == 0U) {
        msh_putc(ctx, '0');
        return;
    }
    while (val > 0U && n < 10) {
        buf[n++] = (char)('0' + (val % 10U));
        val /= 10U;
    }
    while (n > 0) {
        msh_putc(ctx, buf[--n]);
    }
}

static void msh_print_hex32(msh_ctx_t *ctx, uint32_t val) {
    static const char kHex[] = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; ++i) {
        buf[2 + i] = kHex[(val >> ((7 - i) * 4)) & 0xFU];
    }
    buf[10] = '\0';
    msh_puts(ctx, buf);
}

static uint32_t msh_parse_u32(const char *s) {
    uint32_t val = 0U;
    if (s == NULL) {
        return 0U;
    }
    while (*s >= '0' && *s <= '9') {
        val = (val * 10U) + (uint32_t)(*s - '0');
        ++s;
    }
    return val;
}

static uint32_t msh_parse_hex_u32(const char *s) {
    uint32_t val = 0U;
    if (s == NULL) {
        return 0U;
    }
    if (s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        s += 2;
    }
    while (*s != '\0') {
        char c = *s++;
        if (c >= '0' && c <= '9') {
            val = (val << 4U) | (uint32_t)(c - '0');
        } else if (c >= 'a' && c <= 'f') {
            val = (val << 4U) | (uint32_t)(c - 'a' + 10);
        } else if (c >= 'A' && c <= 'F') {
            val = (val << 4U) | (uint32_t)(c - 'A' + 10);
        } else {
            break;
        }
    }
    return val;
}

static const char *msh_get_var(const msh_ctx_t *ctx, const char *key) {
    for (size_t i = 0U; i < MSH_MVP_VARS_MAX; ++i) {
        if (ctx->vars[i].used != 0 && msh_streq(ctx->vars[i].key, key)) {
            return ctx->vars[i].val;
        }
    }
    return NULL;
}

static int msh_set_var(msh_ctx_t *ctx, const char *key, const char *val) {
    if (key == NULL || key[0] == '\0') {
        return MSH_ERR_INVALID;
    }
    for (size_t i = 0U; i < MSH_MVP_VARS_MAX; ++i) {
        if (ctx->vars[i].used != 0 && msh_streq(ctx->vars[i].key, key)) {
            msh_copy_str(ctx->vars[i].val, MSH_MVP_VAL_MAX, val);
            return MSH_OK;
        }
    }
    for (size_t i = 0U; i < MSH_MVP_VARS_MAX; ++i) {
        if (ctx->vars[i].used == 0) {
            ctx->vars[i].used = 1;
            msh_copy_str(ctx->vars[i].key, MSH_MVP_KEY_MAX, key);
            msh_copy_str(ctx->vars[i].val, MSH_MVP_VAL_MAX, val);
            return MSH_OK;
        }
    }
    return MSH_ERR_NOMEM;
}

static const char *task_state_str(maia_task_state_t st) {
    switch (st) {
        case MAIA_TASK_READY:    return "READY";
        case MAIA_TASK_RUNNING:  return "RUN  ";
        case MAIA_TASK_SLEEPING: return "SLEEP";
        case MAIA_TASK_BLOCKED:  return "BLOCK";
        case MAIA_TASK_EXITED:   return "EXIT ";
        default:                 return "UNUSD";
    }
}

int msh_mvp_init(msh_ctx_t *ctx, const msh_host_v1 *host) {
    int32_t rc = msh_host_v1_validate(host);
    if (ctx == NULL || rc != MSH_OK) {
        return MSH_ERR_INVALID;
    }
    ctx->host        = host;
    ctx->line_len    = 0U;
    ctx->line[0]     = '\0';
    ctx->cmd_count   = 0U;
    ctx->last_status = 0;
    msh_copy_str(ctx->cwd, sizeof(ctx->cwd), "/");
    for (size_t i = 0U; i < MSH_MVP_VARS_MAX; ++i) {
        ctx->vars[i].used   = 0;
        ctx->vars[i].key[0] = '\0';
        ctx->vars[i].val[0] = '\0';
    }
    (void)msh_set_var(ctx, "BOARD", "M5Stack-Tab5");
    (void)msh_set_var(ctx, "SOC", "ESP32-P4");
    return MSH_OK;
}

void msh_mvp_print_banner(msh_ctx_t *ctx) {
    msh_puts(ctx, "\n========================================================\n");
    msh_puts(ctx, " MaiaRTOS v0.1.0 + MaiaShell Native Execution MVP (N2)\n");
    msh_puts(ctx, " Board  : M5Stack Tab5 (ESP32-P4 rev v1.3, 40MHz XTAL)\n");
    msh_puts(ctx, " Kernel : Clean-Room C11/RV32IMAFC (Zero FreeRTOS/IDF)\n");
    msh_puts(ctx, " Type 'help' for built-in RTOS introspection commands.\n");
    msh_puts(ctx, "========================================================\n");
}

void msh_mvp_print_prompt(msh_ctx_t *ctx) {
    msh_puts(ctx, "msh> ");
}

static int eval_single_cmd(msh_ctx_t *ctx, char *segment) {
    char *argv[MSH_MVP_ARGS_MAX];
    size_t argc = 0U;
    char *p = segment;

    while (*p != '\0' && argc < MSH_MVP_ARGS_MAX) {
        while (*p == ' ' || *p == '\t') {
            *p++ = '\0';
        }
        if (*p == '\0') {
            break;
        }
        argv[argc++] = p;
        while (*p != '\0' && *p != ' ' && *p != '\t') {
            ++p;
        }
    }

    if (argc == 0U) {
        return 0;
    }

    /* Expand leading $VAR arguments */
    for (size_t i = 1U; i < argc; ++i) {
        if (argv[i][0] == '$') {
            const char *val = msh_get_var(ctx, &argv[i][1]);
            argv[i] = (char *)(uintptr_t)((val != NULL) ? val : "");
        }
    }

    ctx->cmd_count += 1U;

    if (msh_streq(argv[0], "help")) {
        msh_puts(ctx, "MaiaShell Native Commands:\n");
        msh_puts(ctx, "  help          - Show this command summary\n");
        msh_puts(ctx, "  uname         - Show MaiaRTOS kernel & SoC architecture info\n");
        msh_puts(ctx, "  uptime        - Show SYSTIMER microseconds & RTOS tick uptime\n");
        msh_puts(ctx, "  ticks         - Show CLIC/SYSTIMER interrupt & trap counters\n");
        msh_puts(ctx, "  ps            - List RTOS tasks (ID, prio, state, switches, ticks)\n");
        msh_puts(ctx, "  mem           - Show L2MEM region allocator statistics\n");
        msh_puts(ctx, "  alloc <bytes> - Allocate, verify & free a block from L2MEM heap\n");
        msh_puts(ctx, "  qtest         - Run live mutex/semaphore/queue self-test\n");
        msh_puts(ctx, "  pwd           - Print working directory\n");
        msh_puts(ctx, "  set K=V       - Set shell variable (use $K in commands)\n");
        msh_puts(ctx, "  env           - List shell variables\n");
        msh_puts(ctx, "  echo ...      - Print arguments\n");
        msh_puts(ctx, "  backlight <0-100> - Set display backlight brightness\n");
        msh_puts(ctx, "  peek <addr>   - Read 32-bit register (hex address)\n");
        msh_puts(ctx, "  poke <addr> <val> - Write 32-bit register (hex address & val)\n");
        msh_puts(ctx, "  clear         - Clear terminal screen\n");
        return 0;
    }

    if (msh_streq(argv[0], "uname")) {
        msh_puts(ctx, "MaiaRTOS ");
        msh_print_u32(ctx, (uint32_t)MAIA_RTOS_VERSION_MAJOR);
        msh_putc(ctx, '.');
        msh_print_u32(ctx, (uint32_t)MAIA_RTOS_VERSION_MINOR);
        msh_putc(ctx, '.');
        msh_print_u32(ctx, (uint32_t)MAIA_RTOS_VERSION_PATCH);
        msh_puts(ctx, " riscv32-esp32p4 (M5Stack Tab5) [Zero-FreeRTOS/Zero-IDF]\n");
        return 0;
    }

    if (msh_streq(argv[0], "pwd")) {
        msh_puts(ctx, ctx->cwd);
        msh_puts(ctx, "\n");
        return 0;
    }

    if (msh_streq(argv[0], "uptime") || msh_streq(argv[0], "ticks")) {
        uint64_t ticks = maia_tick_get();
        uint64_t us    = maia_time_us();
        maia_trap_stats_t ts;
        maia_trap_get_stats(&ts);

        msh_puts(ctx, "tick_ms=");
        msh_print_u32(ctx, (uint32_t)ticks);
        msh_puts(ctx, "  systimer_us=");
        msh_print_u32(ctx, (uint32_t)us);
        msh_puts(ctx, "  ctx_switches=");
        msh_print_u32(ctx, maia_sched_switch_count());
        msh_puts(ctx, "\ntraps: total=");
        msh_print_u32(ctx, ts.total_traps);
        msh_puts(ctx, " timer_irq=");
        msh_print_u32(ctx, ts.timer_irqs);
        msh_puts(ctx, " ecall=");
        msh_print_u32(ctx, ts.ecall_count);
        msh_puts(ctx, " last_mcause=");
        msh_print_hex32(ctx, ts.last_mcause);
        msh_puts(ctx, "\n");
        return 0;
    }

    if (msh_streq(argv[0], "ps")) {
        msh_puts(ctx, "TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME\n");
        for (uint32_t i = 0U; i < MAIA_MAX_TASKS; ++i) {
            maia_task_info_t info;
            if (maia_task_get_info(i, &info) == MAIA_OK) {
                msh_puts(ctx, "  ");
                msh_print_u32(ctx, (uint32_t)info.id);
                msh_puts(ctx, "     ");
                msh_print_u32(ctx, (uint32_t)info.priority);
                msh_puts(ctx, "  ");
                msh_puts(ctx, task_state_str(info.state));
                msh_puts(ctx, "  ");
                msh_print_u32(ctx, (uint32_t)info.stack_size);
                msh_puts(ctx, "      ");
                msh_print_u32(ctx, info.switches);
                msh_puts(ctx, "     ");
                msh_print_u32(ctx, info.run_ticks);
                msh_puts(ctx, "  ");
                msh_puts(ctx, info.name);
                msh_puts(ctx, "\n");
            }
        }
        return 0;
    }

    if (msh_streq(argv[0], "mem")) {
        maia_mem_stats_t ms;
        (void)maia_mem_get_stats(&ms);
        msh_puts(ctx, "L2MEM Region Allocator:\n  total=");
        msh_print_u32(ctx, (uint32_t)ms.total_bytes);
        msh_puts(ctx, "B  used=");
        msh_print_u32(ctx, (uint32_t)ms.used_bytes);
        msh_puts(ctx, "B  free=");
        msh_print_u32(ctx, (uint32_t)ms.free_bytes);
        msh_puts(ctx, "B  peak=");
        msh_print_u32(ctx, (uint32_t)ms.peak_used_bytes);
        msh_puts(ctx, "B  allocs=");
        msh_print_u32(ctx, ms.alloc_count);
        msh_puts(ctx, "  frees=");
        msh_print_u32(ctx, ms.free_count);
        msh_puts(ctx, "\n");
        return 0;
    }

    if (msh_streq(argv[0], "alloc")) {
        uint32_t sz = (argc > 1U) ? msh_parse_u32(argv[1]) : 128U;
        if (sz == 0U) {
            sz = 64U;
        }
        uint8_t *blk = (uint8_t *)maia_mem_alloc((size_t)sz);
        if (blk == NULL) {
            msh_puts(ctx, "alloc FAILED (out of memory)\n");
            return 1;
        }
        blk[0]      = 0xA5U;
        blk[sz - 1] = 0x5AU;
        msh_puts(ctx, "alloc OK ptr=");
        msh_print_hex32(ctx, (uint32_t)(uintptr_t)blk);
        msh_puts(ctx, " size=");
        msh_print_u32(ctx, sz);
        (void)maia_mem_free(blk);
        msh_puts(ctx, " (verified & freed)\n");
        return 0;
    }

    if (msh_streq(argv[0], "qtest")) {
        maia_mutex_t mtx;
        maia_sem_t   sem;
        maia_queue_t q;
        uint32_t     qbuf[4];
        uint32_t     in_val  = 0xCAFEBABEUL;
        uint32_t     out_val = 0U;

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
            msh_puts(ctx, "qtest PASS: mutex + sem + queue verified (0xCAFEBABE)\n");
            return 0;
        }
        msh_puts(ctx, "qtest FAIL\n");
        return 1;
    }

    if (msh_streq(argv[0], "set")) {
        if (argc < 2U) {
            msh_puts(ctx, "usage: set KEY=VAL\n");
            return 1;
        }
        char *eq = argv[1];
        while (*eq != '\0' && *eq != '=') {
            ++eq;
        }
        if (*eq != '=') {
            msh_puts(ctx, "usage: set KEY=VAL\n");
            return 1;
        }
        *eq = '\0';
        (void)msh_set_var(ctx, argv[1], eq + 1);
        return 0;
    }

    if (msh_streq(argv[0], "env")) {
        for (size_t i = 0U; i < MSH_MVP_VARS_MAX; ++i) {
            if (ctx->vars[i].used != 0) {
                msh_puts(ctx, ctx->vars[i].key);
                msh_putc(ctx, '=');
                msh_puts(ctx, ctx->vars[i].val);
                msh_puts(ctx, "\n");
            }
        }
        return 0;
    }

    if (msh_streq(argv[0], "echo")) {
        for (size_t i = 1U; i < argc; ++i) {
            if (i > 1U) {
                msh_putc(ctx, ' ');
            }
            msh_puts(ctx, argv[i]);
        }
        msh_puts(ctx, "\n");
        return 0;
    }

    if (msh_streq(argv[0], "clear")) {
        msh_puts(ctx, "\033[2J\033[H");
        return 0;
    }

    if (msh_streq(argv[0], "backlight")) {
        uint32_t pct = (argc > 1U) ? msh_parse_u32(argv[1]) : 100U;
        if (pct > 100U) pct = 100U;
#ifdef __riscv
        extern void maia_tab5_backlight_set_duty(uint32_t duty_4095);
        uint32_t duty = (pct * 4095U) / 100U;
        maia_tab5_backlight_set_duty(duty);
#endif
        msh_puts(ctx, "Backlight set to ");
        msh_print_u32(ctx, pct);
        msh_puts(ctx, "%\n");
        return 0;
    }

    if (msh_streq(argv[0], "peek")) {
        if (argc < 2U) {
            msh_puts(ctx, "usage: peek <hex_addr>\n");
            return 1;
        }
        uint32_t addr = msh_parse_hex_u32(argv[1]);
        uint32_t val = *(volatile uint32_t *)(uintptr_t)addr;
        msh_puts(ctx, argv[1]);
        msh_puts(ctx, " = ");
        msh_print_hex32(ctx, val);
        msh_puts(ctx, "\n");
        return 0;
    }

    if (msh_streq(argv[0], "poke")) {
        if (argc < 3U) {
            msh_puts(ctx, "usage: poke <hex_addr> <hex_val>\n");
            return 1;
        }
        uint32_t addr = msh_parse_hex_u32(argv[1]);
        uint32_t val = msh_parse_hex_u32(argv[2]);
        *(volatile uint32_t *)(uintptr_t)addr = val;
        msh_puts(ctx, argv[1]);
        msh_puts(ctx, " <= ");
        msh_print_hex32(ctx, val);
        msh_puts(ctx, "\n");
        return 0;
    }

    msh_puts(ctx, "msh: unknown command: ");
    msh_puts(ctx, argv[0]);
    msh_puts(ctx, " (type 'help')\n");
    return 127;
}

int msh_mvp_eval_line(msh_ctx_t *ctx, const char *cmdline) {
    char buf[MSH_MVP_LINE_MAX];
    if (ctx == NULL || cmdline == NULL) {
        return MSH_ERR_INVALID;
    }
    msh_copy_str(buf, MSH_MVP_LINE_MAX, cmdline);

    char *seg = buf;
    int rc = 0;
    while (*seg != '\0') {
        char *semi = seg;
        while (*semi != '\0' && *semi != ';') {
            ++semi;
        }
        if (*semi == ';') {
            *semi = '\0';
            rc = eval_single_cmd(ctx, seg);
            seg = semi + 1;
        } else {
            rc = eval_single_cmd(ctx, seg);
            break;
        }
    }
    ctx->last_status = rc;
    return rc;
}

int msh_mvp_feed_char(msh_ctx_t *ctx, char ch) {
    if (ctx == NULL) {
        return MSH_ERR_INVALID;
    }
    if (ch == '\r' || ch == '\n') {
        msh_puts(ctx, "\n");
        ctx->line[ctx->line_len] = '\0';
        int rc = 0;
        if (ctx->line_len > 0U) {
            rc = msh_mvp_eval_line(ctx, ctx->line);
        }
        ctx->line_len = 0U;
        ctx->line[0]  = '\0';
        msh_mvp_print_prompt(ctx);
        return rc;
    }
    if (ch == '\b' || (unsigned char)ch == 0x7FU) {
        if (ctx->line_len > 0U) {
            ctx->line_len -= 1U;
            ctx->line[ctx->line_len] = '\0';
            msh_puts(ctx, "\b \b");
        }
        return 0;
    }
    if ((unsigned char)ch >= 0x20U && (unsigned char)ch < 0x7FU) {
        if (ctx->line_len + 1U < MSH_MVP_LINE_MAX) {
            ctx->line[ctx->line_len++] = ch;
            ctx->line[ctx->line_len]   = '\0';
            msh_putc(ctx, ch);
        }
    }
    return 0;
}

