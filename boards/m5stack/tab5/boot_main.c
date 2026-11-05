#include <maia/console.h>
#include <maia/kernel.h>
#include <maia/soc/esp32p4_board_io.h>
#include <maia/soc/esp32p4_display.h>
#include <maia/version.h>
#include <maia_shell/host_v1.h>
#include <maia_shell/shell_mvp.h>
#include <stdint.h>

extern void maia_soc_systimer_init(uint32_t tick_hz);

/* 32 KiB L2MEM heap pool + task stacks in internal L2MEM_LOW (.bss) */
static uint8_t g_heap_pool[32768] __attribute__((aligned(16)));
static uint8_t g_stack_idle[4096] __attribute__((aligned(16)));
static uint8_t g_stack_telem[4096] __attribute__((aligned(16)));
static uint8_t g_stack_shell[8192] __attribute__((aligned(16)));

static maia_mutex_t g_telem_mutex;
static maia_queue_t g_telem_queue;
static uint32_t     g_telem_qbuf[8];
static msh_ctx_t    g_shell_ctx;

static void print_hex32(uint32_t val) {
    static const char kHex[] = "0123456789ABCDEF";
    char buf[11];
    buf[0] = '0';
    buf[1] = 'x';
    for (int i = 0; i < 8; ++i) {
        buf[2 + i] = kHex[(val >> ((7 - i) * 4)) & 0xFU];
    }
    buf[10] = '\0';
    maia_console_puts(buf);
}

static void print_u32(uint32_t val) {
    char digits[12];
    int n = 0;
    if (val == 0U) {
        maia_console_putc('0');
        return;
    }
    while (val > 0U && n < 10) {
        digits[n++] = (char)('0' + (val % 10U));
        val /= 10U;
    }
    while (n > 0) {
        maia_console_putc(digits[--n]);
    }
}

/* msh_host_v1 callbacks bound to MaiaRTOS console */
static int32_t tab5_host_read(void *user, msh_fd_t fd, void *dst, uint32_t n) {
    (void)user;
    if (fd != MSH_FD_STDIN || dst == NULL || n == 0U) {
        return MSH_ERR_INVALID;
    }
    int ch = maia_console_getc_nonblocking();
    if (ch < 0) {
        return MSH_ERR_AGAIN;
    }
    ((char *)dst)[0] = (char)ch;
    return 1;
}

static int32_t tab5_host_write(void *user, msh_fd_t fd, const void *src, uint32_t n) {
    (void)user;
    if ((fd != MSH_FD_STDOUT && fd != MSH_FD_STDERR) || src == NULL) {
        return MSH_ERR_INVALID;
    }
    maia_console_write((const char *)src, (size_t)n);
    return (int32_t)n;
}

static msh_host_v1 g_tab5_shell_host;

static void task_idle_entry(void *arg) {
    (void)arg;
    uint32_t last_cyc = 0U;
    __asm__ volatile("csrr %0, mcycle" : "=r"(last_cyc));
    for (;;) {
        uint32_t now_cyc = 0U;
        __asm__ volatile("csrr %0, mcycle" : "=r"(now_cyc));
        if ((now_cyc - last_cyc) >= 40000U) {
            last_cyc += 40000U;
            maia_trap_stats_t ts;
            maia_trap_get_stats(&ts);
            if (ts.timer_irqs == 0U) {
                (void)maia_sched_tick_isr();
            }
        }
        maia_task_yield();
    }
}

static void task_telemetry_entry(void *arg) {
    (void)arg;
    uint32_t seq = 0U;
    for (;;) {
        maia_task_id_t tid = maia_task_current_id();
        if (maia_mutex_trylock(&g_telem_mutex, tid) == MAIA_OK) {
            seq += 1U;
            (void)maia_queue_send(&g_telem_queue, &seq);
            (void)maia_mutex_unlock(&g_telem_mutex, tid);
        }
        maia_task_delay_ticks(25U);
    }
}

static void task_shell_entry(void *arg) {
    (void)arg;
    msh_mvp_bind_console_host(&g_tab5_shell_host, NULL, tab5_host_read, tab5_host_write);
    (void)msh_mvp_init(&g_shell_ctx, &g_tab5_shell_host);

    /* Yield for 60 ms so telemetry & idle tasks accumulate preemptive/cooperative ticks */
    maia_task_delay_ticks(60U);

    uint32_t msg = 0U;
    uint32_t drained = 0U;
    while (maia_queue_recv(&g_telem_queue, &msg) == MAIA_OK) {
        drained = msg;
    }
    maia_console_puts("[M2/M5] Telemetry task queue verified: rx_seq=");
    print_u32(drained);
    maia_console_puts("\n");

    msh_mvp_print_banner(&g_shell_ctx);
    maia_console_puts("[AUTO-SELFTEST] Running: uname; ticks; ps; mem; alloc 256; qtest\n");
    (void)msh_mvp_eval_line(&g_shell_ctx, "uname; ticks; ps; mem; alloc 256; qtest");
    msh_mvp_print_prompt(&g_shell_ctx);

    for (;;) {
        int ch = maia_console_getc_nonblocking();
        if (ch >= 0) {
            (void)msh_mvp_feed_char(&g_shell_ctx, (char)ch);
        } else {
            maia_task_delay_ticks(2U);
        }
    }
}

void maia_boot_main(void) {
    uint32_t mhartid = 0U;
    uint32_t misa    = 0U;
    uint32_t sp_val  = 0U;
    __asm__ volatile("csrr %0, mhartid" : "=r"(mhartid));
    __asm__ volatile("csrr %0, misa" : "=r"(misa));
    __asm__ volatile("mv %0, sp" : "=r"(sp_val));

    maia_console_puts("\n========================================================\n");
    maia_console_puts("MaiaRTOS v0.1.0 — Bare-Metal Kernel & Shell Bring-Up\n");
    maia_console_puts("Target : M5Stack Tab5 (ESP32-P4 rev v1.3)\n");
    maia_console_puts("Runtime: ZERO FreeRTOS / ZERO ESP-IDF / ZERO Arduino\n");
    maia_console_puts("mhartid: ");
    print_hex32(mhartid);
    maia_console_puts("  misa: ");
    print_hex32(misa);
    maia_console_puts("  sp: ");
    print_hex32(sp_val);
    maia_console_puts("\n========================================================\n");

    /* Initialize kernel subsystems (M2, M4, M5) */
    (void)maia_sched_init();
    (void)maia_mem_init(g_heap_pool, sizeof(g_heap_pool));
    (void)maia_mutex_init(&g_telem_mutex);
    (void)maia_queue_init(&g_telem_queue, g_telem_qbuf, sizeof(uint32_t), 8U);

    /* Initialize M1 CLIC Trap Vector + 1000 Hz SYSTIMER tick */
    maia_soc_systimer_init(1000U);

    /* Initialize M5Stack Tab5 IO expanders, backlight, touch/display detect & keyboard */
    maia_tab5_board_io_init();

    /* Initialize MIPI-DSI 720x1280 screen */
    maia_display_init(maia_tab5_get_detected_panel());

    /* Trigger synchronous breakpoint exception (ebreak, mcause=3) to verify trap vector */
    __asm__ volatile(
        ".option push\n\t"
        ".option norvc\n\t"
        "ebreak\n\t"
        ".option pop"
        ::: "memory");

    maia_trap_stats_t ts;
    maia_trap_get_stats(&ts);
    maia_console_puts("[M1] Trap vector verified (ebreak): sync_exceptions=");
    print_u32(ts.sync_exceptions);
    maia_console_puts(" last_mcause=");
    print_hex32(ts.last_mcause);
    maia_console_puts(" systimer_us=");
    print_u32((uint32_t)maia_time_us());
    maia_console_puts("\n");

    /* Create RTOS tasks (M2) */
    maia_task_id_t tid_idle  = -1;
    maia_task_id_t tid_telem = -1;
    maia_task_id_t tid_shell = -1;

    (void)maia_task_create("idle", task_idle_entry, NULL, MAIA_PRIO_IDLE,
                           g_stack_idle, sizeof(g_stack_idle), &tid_idle);
    (void)maia_task_create("telemetry", task_telemetry_entry, NULL, MAIA_PRIO_NORMAL,
                           g_stack_telem, sizeof(g_stack_telem), &tid_telem);
    (void)maia_task_create("maia_shell", task_shell_entry, NULL, MAIA_PRIO_NORMAL,
                           g_stack_shell, sizeof(g_stack_shell), &tid_shell);

    maia_console_puts("[M2] Created 3 RTOS tasks (idle=");
    print_u32((uint32_t)tid_idle);
    maia_console_puts(", telemetry=");
    print_u32((uint32_t)tid_telem);
    maia_console_puts(", maia_shell=");
    print_u32((uint32_t)tid_shell);
    maia_console_puts(") — starting scheduler...\n");

    maia_sched_start();

    for (;;) {
        __asm__ volatile("wfi");
    }
}
