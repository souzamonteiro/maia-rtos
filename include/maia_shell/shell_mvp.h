#ifndef MAIA_SHELL_MVP_H
#define MAIA_SHELL_MVP_H

/*
 * MaiaShell Native Execution MVP (N2)
 *
 * Instance-based (zero global mutable state) interactive command shell
 * bound exclusively to the msh_host_v1 contract and MaiaRTOS public APIs.
 * Compiles cleanly under C11 and C++98 (-Wall -Wextra -Wpedantic -Werror).
 */

#include <maia_shell/host_v1.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MSH_MVP_LINE_MAX 128U
#define MSH_MVP_ARGS_MAX 12U
#define MSH_MVP_VARS_MAX 8U
#define MSH_MVP_KEY_MAX  16U
#define MSH_MVP_VAL_MAX  32U

typedef struct msh_var {
    char key[MSH_MVP_KEY_MAX];
    char val[MSH_MVP_VAL_MAX];
    int  used;
} msh_var_t;

typedef struct msh_ctx {
    const msh_host_v1 *host;
    char               line[MSH_MVP_LINE_MAX];
    size_t             line_len;
    uint32_t           cmd_count;
    int                last_status;
    char               cwd[32];
    msh_var_t          vars[MSH_MVP_VARS_MAX];
} msh_ctx_t;

void msh_mvp_bind_console_host(msh_host_v1 *out_host,
                               void *user,
                               int32_t (*read_fn)(void *user, msh_fd_t fd, void *dst, uint32_t n),
                               int32_t (*write_fn)(void *user, msh_fd_t fd, const void *src, uint32_t n));

int  msh_mvp_init(msh_ctx_t *ctx, const msh_host_v1 *host);
void msh_mvp_print_banner(msh_ctx_t *ctx);
void msh_mvp_print_prompt(msh_ctx_t *ctx);
int  msh_mvp_eval_line(msh_ctx_t *ctx, const char *cmdline);
int  msh_mvp_feed_char(msh_ctx_t *ctx, char ch);

#ifdef __cplusplus
}
#endif

#endif /* MAIA_SHELL_MVP_H */

