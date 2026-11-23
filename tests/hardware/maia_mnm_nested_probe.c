#include <maia/exec/mnm.h>
#include <stdint.h>
#include <string.h>

extern uint32_t millis(void);
extern void delay_ms(uint32_t ms);

struct captured_output {
    uint32_t bytes;
    int saw_validation_error;
};

static void capture_output(const char *text, void *user_data) {
    struct captured_output *capture = user_data;
    if (!text) return;

    static const char error[] = "ELF Error: Buffer too small";
    if (strncmp(text, error, sizeof(error) - 1U) == 0) {
        capture->saw_validation_error = 1;
    }
    while (*text++) ++capture->bytes;
}

static int wait_works(void) {
    uint32_t before = millis();
    delay_ms(100);
    uint32_t elapsed = millis() - before;
    return elapsed >= 95U && elapsed <= 5000U;
}

int maia_module_main(const maia_module_context_t *ctx) {
    if (!ctx || !ctx->host || !ctx->host->shell_exec) return 101;
    if (!wait_works()) return 102;

    struct captured_output output = {0, 0};
    int32_t result = ctx->host->shell_exec("runelf", capture_output, &output);
    if (result != 42 || output.bytes == 0U) return 103;
    if (!wait_works()) return 104;

    output.bytes = 0;
    output.saw_validation_error = 0;
    result = ctx->host->shell_exec("runelf bad.mobj", capture_output, &output);
    if (result != 1 || !output.saw_validation_error) return 105;
    if (!wait_works()) return 106;

    return 77;
}
