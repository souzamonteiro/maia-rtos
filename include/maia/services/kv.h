#ifndef MAIA_SERVICES_KV_H
#define MAIA_SERVICES_KV_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAIA_KV_REGION_BYTES 16384U
#define MAIA_KV_ERASE_BYTES 4096U
#define MAIA_KV_BANK_BYTES (MAIA_KV_REGION_BYTES / 2U)
#define MAIA_KV_KEY_MAX 31U
#define MAIA_KV_VALUE_MAX 1024U
#define MAIA_KV_MAX_ENTRIES 64U
#define MAIA_KV_RECORD_MAX_BYTES 1068U
#define MAIA_KV_SCRATCH_WORDS (MAIA_KV_RECORD_MAX_BYTES / 4U)

typedef enum maia_kv_result {
    MAIA_KV_OK = 0,
    MAIA_KV_INVALID = -1,
    MAIA_KV_NOT_READY = -2,
    MAIA_KV_NOT_FOUND = -3,
    MAIA_KV_BUFFER_TOO_SMALL = -4,
    MAIA_KV_NO_SPACE = -5,
    MAIA_KV_BUSY = -6,
    MAIA_KV_IO_ERROR = -7,
    MAIA_KV_CORRUPT = -8,
    MAIA_KV_RECOVERED_EMPTY = 1
} maia_kv_result_t;

typedef int (*maia_kv_read_fn)(void *context, uint32_t offset,
                               void *buffer, uint32_t length);
typedef int (*maia_kv_write_fn)(void *context, uint32_t offset,
                                const void *buffer, uint32_t length);
typedef int (*maia_kv_erase_fn)(void *context, uint32_t offset,
                                uint32_t length);

typedef struct maia_kv_flash {
    maia_kv_read_fn read;
    maia_kv_write_fn write;
    maia_kv_erase_fn erase;
    void *context;
    uint32_t region_bytes;
    uint32_t erase_bytes;
} maia_kv_flash_t;

typedef struct maia_kv_store {
    maia_kv_flash_t flash;
    uint32_t sequence;
    uint32_t payload_bytes;
    uint32_t active_bank;
    uint32_t ready;
    uint32_t busy;
    uint32_t scratch[MAIA_KV_SCRATCH_WORDS];
} maia_kv_store_t;

/*
 * The region contains two 8 KiB snapshot banks and must use 4 KiB erase
 * sectors. Reads/writes are four-byte aligned. Calls are thread-context-only;
 * concurrent calls on one store return BUSY. Init requires quiescence. Each
 * set/delete rebuilds and erases the inactive bank, so use this for infrequent
 * settings, not high-rate data. The Tab5 backend masks local interrupts during
 * each raw flash transaction. A failed update leaves the prior committed bank
 * intact. After a failure during final verification, reboot/re-init determines
 * whether the old or new snapshot committed.
 * If neither bank has a valid commit, init returns RECOVERED_EMPTY and leaves
 * the store usable while explicitly reporting that settings may be lost.
 */
maia_kv_result_t maia_kv_init(maia_kv_store_t *store,
                              const maia_kv_flash_t *flash);
maia_kv_result_t maia_kv_get(maia_kv_store_t *store, const char *key,
                             void *value, uint32_t capacity,
                             uint32_t *out_length);
maia_kv_result_t maia_kv_set(maia_kv_store_t *store, const char *key,
                             const void *value, uint32_t length);
maia_kv_result_t maia_kv_delete(maia_kv_store_t *store, const char *key);

#ifdef __cplusplus
}
#endif

#endif
