#ifndef MAIA_BOARDS_TAB5_KV_H
#define MAIA_BOARDS_TAB5_KV_H

#include <maia/services/kv.h>

#ifdef __cplusplus
extern "C" {
#endif

maia_kv_result_t maia_tab5_kv_init(void);
maia_kv_store_t *maia_tab5_kv_store(void);

#ifdef __cplusplus
}
#endif

#endif
