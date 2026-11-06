#ifndef MAIA_SHELL_ENGINE_H
#define MAIA_SHELL_ENGINE_H

#ifdef __cplusplus
extern "C" {
#endif

void maia_shell_engine_init(void);
void maia_shell_engine_print_banner(void);
void maia_shell_engine_print_prompt(void);
int  maia_shell_engine_eval_line(const char *line);
int  maia_shell_engine_feed_char(char c);

#ifdef __cplusplus
}
#endif

#endif /* MAIA_SHELL_ENGINE_H */
