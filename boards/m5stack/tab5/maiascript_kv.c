#include <maia/boards/tab5_kv.h>
#include <maia/boards/tab5_maiascript_kv.h>
#include <maia/exec/elf_loader.h>

#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static _Thread_local int32_t last_get_error;

static int32_t host_kv_set_text(const char *key, const char *value) {
    if (key == NULL || value == NULL) return MAIA_KV_INVALID;

    size_t length = 0U;
    while (length <= MAIA_KV_VALUE_MAX && value[length] != '\0') ++length;
    if (length > MAIA_KV_VALUE_MAX) return MAIA_KV_INVALID;

    return (int32_t)maia_kv_set(maia_tab5_kv_store(), key, value,
                                (uint32_t)length);
}

static char *host_kv_get_text(const char *key) {
    uint8_t value[MAIA_KV_VALUE_MAX];
    uint32_t length = 0U;
    last_get_error = (int32_t)maia_kv_get(
        maia_tab5_kv_store(), key, value, sizeof(value), &length);
    if (last_get_error != MAIA_KV_OK) return NULL;
    if (memchr(value, '\0', length) != NULL) {
        last_get_error = MAIA_KV_INVALID;
        return NULL;
    }

    char *result = (char *)malloc((size_t)length + 1U);
    if (result == NULL) {
        last_get_error = MAIA_KV_NO_SPACE;
        return NULL;
    }
    memcpy(result, value, length);
    result[length] = '\0';
    return result;
}

static int32_t host_kv_last_get_error(void) {
    return last_get_error;
}

static void host_kv_free_text(char *value) {
    free(value);
}

static int32_t host_kv_delete_text(const char *key) {
    return (int32_t)maia_kv_delete(maia_tab5_kv_store(), key);
}

static int32_t host_kv_contains_text(const char *key) {
    uint8_t probe = 0U;
    uint32_t length = 0U;
    maia_kv_result_t result = maia_kv_get(
        maia_tab5_kv_store(), key, &probe, sizeof(probe), &length);
    if (result == MAIA_KV_OK || result == MAIA_KV_BUFFER_TOO_SMALL) return 1;
    if (result == MAIA_KV_NOT_FOUND) return 0;
    return (int32_t)result;
}

int maia_tab5_kv_register_maiascript_symbols(void) {
    int result = mnm_register_symbol("host_kv_set_text",
                                     (uintptr_t)host_kv_set_text);
    if (result != 0) return result;
    result = mnm_register_symbol("host_kv_get_text",
                                 (uintptr_t)host_kv_get_text);
    if (result != 0) return result;
    result = mnm_register_symbol("host_kv_last_get_error",
                                 (uintptr_t)host_kv_last_get_error);
    if (result != 0) return result;
    result = mnm_register_symbol("host_kv_free_text",
                                 (uintptr_t)host_kv_free_text);
    if (result != 0) return result;
    result = mnm_register_symbol("host_kv_delete_text",
                                 (uintptr_t)host_kv_delete_text);
    if (result != 0) return result;
    return mnm_register_symbol("host_kv_contains_text",
                               (uintptr_t)host_kv_contains_text);
}
