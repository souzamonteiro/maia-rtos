#ifndef MAIA_NATIVE_DISPLAY_H
#define MAIA_NATIVE_DISPLAY_H

#include <maia/exec/mnm.h>

#ifdef __cplusplus
extern "C" {
#endif

const maia_display_api_t *maia_native_display_api(void);
int maia_native_display_scope_begin(void);
int maia_native_display_scope_end(void);

#ifdef __cplusplus
}
#endif

#endif
