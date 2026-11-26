#include <maia/arch/irq.h>
#include <maia/services/kv.h>

#include <stddef.h>
#include <string.h>

#define KV_HEADER_BYTES 32U
#define KV_RECORD_HEADER_BYTES 8U
#define KV_BANK_EMPTY UINT32_MAX
#define KV_MAGIC UINT32_C(0x31564B4D)
#define KV_VERSION 1U
#define KV_COMMIT UINT32_C(0xA5C35A3C)

typedef enum kv_bank_state {
    KV_BANK_INVALID = 0,
    KV_BANK_VALID = 1,
    KV_BANK_ERASED = 2,
    KV_BANK_IO_ERROR = 3
} kv_bank_state_t;

typedef struct kv_bank_info {
    uint32_t sequence;
    uint32_t payload_bytes;
    uint32_t payload_crc;
} kv_bank_info_t;

static void put_u16(uint8_t *buffer, uint32_t offset, uint16_t value) {
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8U);
}

static void put_u32(uint8_t *buffer, uint32_t offset, uint32_t value) {
    buffer[offset] = (uint8_t)value;
    buffer[offset + 1U] = (uint8_t)(value >> 8U);
    buffer[offset + 2U] = (uint8_t)(value >> 16U);
    buffer[offset + 3U] = (uint8_t)(value >> 24U);
}

static uint16_t get_u16(const uint8_t *buffer, uint32_t offset) {
    return (uint16_t)((uint16_t)buffer[offset] |
                      ((uint16_t)buffer[offset + 1U] << 8U));
}

static uint32_t get_u32(const uint8_t *buffer, uint32_t offset) {
    return (uint32_t)buffer[offset] |
           ((uint32_t)buffer[offset + 1U] << 8U) |
           ((uint32_t)buffer[offset + 2U] << 16U) |
           ((uint32_t)buffer[offset + 3U] << 24U);
}

static uint32_t crc32_update(uint32_t crc, const uint8_t *data,
                             uint32_t length) {
    for (uint32_t i = 0U; i < length; ++i) {
        crc ^= data[i];
        for (uint32_t bit = 0U; bit < 8U; ++bit) {
            uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (UINT32_C(0xEDB88320) & mask);
        }
    }
    return crc;
}

static uint32_t crc32(const uint8_t *data, uint32_t length) {
    return ~crc32_update(UINT32_MAX, data, length);
}

static uint32_t bank_offset(uint32_t bank) {
    return bank * MAIA_KV_BANK_BYTES;
}

static int lock_store(maia_kv_store_t *store) {
    uintptr_t irq = maia_arch_irq_save();
    if (store->busy != 0U) {
        maia_arch_irq_restore(irq);
        return 0;
    }
    store->busy = 1U;
    maia_arch_irq_restore(irq);
    return 1;
}

static void unlock_store(maia_kv_store_t *store) {
    uintptr_t irq = maia_arch_irq_save();
    store->busy = 0U;
    maia_arch_irq_restore(irq);
}

static int key_length(const char *key, uint32_t *out_length) {
    if (key == NULL || out_length == NULL) return 0;
    uint32_t length = 0U;
    while (length <= MAIA_KV_KEY_MAX && key[length] != '\0') ++length;
    if (length == 0U || length > MAIA_KV_KEY_MAX) return 0;
    *out_length = length;
    return 1;
}

static uint32_t record_bytes(uint16_t key_bytes, uint16_t value_bytes) {
    uint32_t raw = KV_RECORD_HEADER_BYTES + (uint32_t)key_bytes +
                   (uint32_t)value_bytes;
    return (raw + 3U) & ~3U;
}

static int read_record(maia_kv_store_t *store, uint32_t bank,
                       uint32_t payload_offset, uint32_t payload_bytes,
                       uint32_t *out_record_bytes) {
    uint32_t base = bank_offset(bank) + KV_HEADER_BYTES;
    if (payload_offset > payload_bytes ||
        payload_bytes - payload_offset < KV_RECORD_HEADER_BYTES) {
        return 0;
    }
    uint8_t *record = (uint8_t *)store->scratch;
    if (store->flash.read(store->flash.context, base + payload_offset,
                          record, KV_RECORD_HEADER_BYTES) != 0) {
        return -1;
    }
    uint16_t key_bytes = get_u16(record, 0U);
    uint16_t value_bytes = get_u16(record, 2U);
    uint32_t bytes = record_bytes(key_bytes, value_bytes);
    if (key_bytes == 0U || key_bytes > MAIA_KV_KEY_MAX ||
        value_bytes > MAIA_KV_VALUE_MAX ||
        bytes > MAIA_KV_RECORD_MAX_BYTES ||
        bytes > payload_bytes - payload_offset) {
        return 0;
    }
    if (store->flash.read(store->flash.context, base + payload_offset,
                          record, bytes) != 0) {
        return -1;
    }
    for (uint32_t i = 0U; i < key_bytes; ++i) {
        if (record[KV_RECORD_HEADER_BYTES + i] == 0U) return 0;
    }
    if (get_u32(record, 4U) !=
        crc32(record + KV_RECORD_HEADER_BYTES,
              (uint32_t)key_bytes + (uint32_t)value_bytes)) {
        return 0;
    }
    *out_record_bytes = bytes;
    return 1;
}

static kv_bank_state_t scan_bank(maia_kv_store_t *store, uint32_t bank,
                                 kv_bank_info_t *out_info) {
    uint32_t header_words[KV_HEADER_BYTES / sizeof(uint32_t)];
    uint8_t *header = (uint8_t *)header_words;
    uint32_t base = bank_offset(bank);
    if (store->flash.read(store->flash.context, base, header,
                          KV_HEADER_BYTES) != 0) {
        return KV_BANK_IO_ERROR;
    }

    int erased = 1;
    for (uint32_t i = 0U; i < KV_HEADER_BYTES; ++i) {
        if (header[i] != UINT8_MAX) {
            erased = 0;
            break;
        }
    }
    if (erased != 0) return KV_BANK_ERASED;

    uint32_t sequence = get_u32(header, 8U);
    uint32_t payload_bytes = get_u32(header, 12U);
    uint32_t payload_crc = get_u32(header, 16U);
    if (get_u32(header, 0U) != KV_MAGIC ||
        get_u16(header, 4U) != KV_VERSION ||
        get_u16(header, 6U) != KV_HEADER_BYTES ||
        get_u32(header, 20U) != crc32(header, 20U) ||
        get_u32(header, 24U) != KV_COMMIT ||
        payload_bytes > MAIA_KV_BANK_BYTES - KV_HEADER_BYTES ||
        (payload_bytes & 3U) != 0U) {
        return KV_BANK_INVALID;
    }

    uint32_t offset = 0U;
    uint32_t entries = 0U;
    uint32_t crc = UINT32_MAX;
    while (offset < payload_bytes) {
        uint32_t bytes = 0U;
        int read_result = read_record(store, bank, offset, payload_bytes,
                                      &bytes);
        if (read_result < 0) return KV_BANK_IO_ERROR;
        if (read_result == 0 || ++entries > MAIA_KV_MAX_ENTRIES) {
            return KV_BANK_INVALID;
        }
        crc = crc32_update(crc, (const uint8_t *)store->scratch, bytes);
        offset += bytes;
    }
    if (offset != payload_bytes || ~crc != payload_crc) {
        return KV_BANK_INVALID;
    }
    out_info->sequence = sequence;
    out_info->payload_bytes = payload_bytes;
    out_info->payload_crc = payload_crc;
    return KV_BANK_VALID;
}

static int sequence_newer(uint32_t left, uint32_t right) {
    uint32_t distance = left - right;
    return distance != 0U && distance < UINT32_C(0x80000000);
}

static maia_kv_result_t find_key(maia_kv_store_t *store, const char *key,
                                 uint32_t key_bytes, uint32_t *out_entries,
                                 int *out_found) {
    *out_entries = 0U;
    *out_found = 0;
    if (store->active_bank == KV_BANK_EMPTY) return MAIA_KV_OK;

    uint32_t offset = 0U;
    while (offset < store->payload_bytes) {
        uint32_t bytes = 0U;
        int read_result = read_record(store, store->active_bank, offset,
                                      store->payload_bytes, &bytes);
        if (read_result < 0) return MAIA_KV_IO_ERROR;
        if (read_result == 0) return MAIA_KV_CORRUPT;
        ++*out_entries;
        uint8_t *record = (uint8_t *)store->scratch;
        if ((uint32_t)get_u16(record, 0U) == key_bytes &&
            memcmp(record + KV_RECORD_HEADER_BYTES, key, key_bytes) == 0) {
            *out_found = 1;
        }
        offset += bytes;
    }
    return offset == store->payload_bytes ? MAIA_KV_OK : MAIA_KV_CORRUPT;
}

static maia_kv_result_t copy_records(maia_kv_store_t *store,
                                    uint32_t target_bank,
                                    const char *key, uint32_t key_bytes,
                                    int *out_found, uint32_t *out_payload_bytes,
                                    uint32_t *out_crc, uint32_t *out_entries) {
    uint32_t payload_offset = 0U;
    uint32_t crc = UINT32_MAX;
    *out_found = 0;
    *out_entries = 0U;
    if (store->active_bank == KV_BANK_EMPTY) {
        *out_payload_bytes = 0U;
        *out_crc = ~crc;
        return MAIA_KV_OK;
    }

    uint32_t source_offset = 0U;
    uint32_t target_base = bank_offset(target_bank) + KV_HEADER_BYTES;
    while (source_offset < store->payload_bytes) {
        uint32_t bytes = 0U;
        int read_result = read_record(store, store->active_bank, source_offset,
                                      store->payload_bytes, &bytes);
        if (read_result < 0) return MAIA_KV_IO_ERROR;
        if (read_result == 0) return MAIA_KV_CORRUPT;
        uint8_t *record = (uint8_t *)store->scratch;
        uint16_t record_key_bytes = get_u16(record, 0U);
        int matches = (uint32_t)record_key_bytes == key_bytes &&
                      memcmp(record + KV_RECORD_HEADER_BYTES, key,
                             key_bytes) == 0;
        if (matches != 0) {
            *out_found = 1;
        } else {
            if (bytes > MAIA_KV_BANK_BYTES - KV_HEADER_BYTES -
                        payload_offset) {
                return MAIA_KV_NO_SPACE;
            }
            if (store->flash.write(store->flash.context,
                                   target_base + payload_offset,
                                   record, bytes) != 0) {
                return MAIA_KV_IO_ERROR;
            }
            crc = crc32_update(crc, record, bytes);
            payload_offset += bytes;
            ++*out_entries;
        }
        source_offset += bytes;
    }
    if (source_offset != store->payload_bytes) return MAIA_KV_CORRUPT;
    *out_payload_bytes = payload_offset;
    *out_crc = ~crc;
    return MAIA_KV_OK;
}

static maia_kv_result_t write_snapshot(maia_kv_store_t *store,
                                       uint32_t target_bank,
                                       const char *key, uint32_t key_bytes,
                                       const void *value, uint32_t value_bytes,
                                       int deleting) {
    uint32_t entries = 0U;
    int found = 0;
    maia_kv_result_t result = find_key(store, key, key_bytes, &entries, &found);
    if (result != MAIA_KV_OK) return result;
    if (deleting != 0 && found == 0) return MAIA_KV_NOT_FOUND;
    if (deleting == 0 && found == 0 && entries >= MAIA_KV_MAX_ENTRIES) {
        return MAIA_KV_NO_SPACE;
    }

    uint32_t target_base = bank_offset(target_bank);
    for (uint32_t offset = 0U; offset < MAIA_KV_BANK_BYTES;
         offset += store->flash.erase_bytes) {
        if (store->flash.erase(store->flash.context, target_base + offset,
                               store->flash.erase_bytes) != 0) {
            return MAIA_KV_IO_ERROR;
        }
    }

    uint32_t payload_bytes = 0U;
    uint32_t payload_crc = 0U;
    uint32_t copied_entries = 0U;
    result = copy_records(store, target_bank, key, key_bytes, &found,
                          &payload_bytes, &payload_crc, &copied_entries);
    if (result != MAIA_KV_OK) return result;
    if (deleting != 0 && found == 0) return MAIA_KV_NOT_FOUND;

    uint32_t crc_state = ~payload_crc;
    uint32_t target_payload = target_base + KV_HEADER_BYTES;
    if (deleting == 0) {
        uint32_t bytes = record_bytes((uint16_t)key_bytes,
                                      (uint16_t)value_bytes);
        if (bytes > MAIA_KV_BANK_BYTES - KV_HEADER_BYTES - payload_bytes) {
            return MAIA_KV_NO_SPACE;
        }
        uint8_t *record = (uint8_t *)store->scratch;
        memset(record, UINT8_MAX, bytes);
        put_u16(record, 0U, (uint16_t)key_bytes);
        put_u16(record, 2U, (uint16_t)value_bytes);
        memcpy(record + KV_RECORD_HEADER_BYTES, key, key_bytes);
        if (value_bytes != 0U) {
            memcpy(record + KV_RECORD_HEADER_BYTES + key_bytes, value,
                   value_bytes);
        }
        put_u32(record, 4U,
                crc32(record + KV_RECORD_HEADER_BYTES,
                      key_bytes + value_bytes));
        if (store->flash.write(store->flash.context,
                               target_payload + payload_bytes,
                               record, bytes) != 0) {
            return MAIA_KV_IO_ERROR;
        }
        crc_state = crc32_update(crc_state, record, bytes);
        payload_bytes += bytes;
        ++copied_entries;
    }

    uint32_t header_words[KV_HEADER_BYTES / sizeof(uint32_t)];
    uint8_t *header = (uint8_t *)header_words;
    memset(header, UINT8_MAX, KV_HEADER_BYTES);
    uint32_t sequence = store->active_bank == KV_BANK_EMPTY
        ? 1U : store->sequence + 1U;
    put_u32(header, 0U, KV_MAGIC);
    put_u16(header, 4U, KV_VERSION);
    put_u16(header, 6U, KV_HEADER_BYTES);
    put_u32(header, 8U, sequence);
    put_u32(header, 12U, payload_bytes);
    put_u32(header, 16U, ~crc_state);
    put_u32(header, 20U, crc32(header, 20U));

    if (store->flash.write(store->flash.context, target_base, header,
                           KV_HEADER_BYTES) != 0) {
        return MAIA_KV_IO_ERROR;
    }
    uint32_t commit = KV_COMMIT;
    if (store->flash.write(store->flash.context, target_base + 24U, &commit,
                           sizeof(commit)) != 0) {
        return MAIA_KV_IO_ERROR;
    }

    kv_bank_info_t verified;
    kv_bank_state_t state = scan_bank(store, target_bank, &verified);
    if (state == KV_BANK_IO_ERROR) return MAIA_KV_IO_ERROR;
    if (state != KV_BANK_VALID || verified.sequence != sequence ||
        verified.payload_bytes != payload_bytes) {
        return MAIA_KV_CORRUPT;
    }
    store->active_bank = target_bank;
    store->sequence = sequence;
    store->payload_bytes = payload_bytes;
    return MAIA_KV_OK;
}

maia_kv_result_t maia_kv_init(maia_kv_store_t *store,
                              const maia_kv_flash_t *flash) {
    if (store == NULL || flash == NULL || flash->read == NULL ||
        flash->write == NULL || flash->erase == NULL ||
        flash->region_bytes != MAIA_KV_REGION_BYTES ||
        flash->erase_bytes != MAIA_KV_ERASE_BYTES) {
        return MAIA_KV_INVALID;
    }
    memset(store, 0, sizeof(*store));
    store->flash = *flash;
    store->active_bank = KV_BANK_EMPTY;

    kv_bank_info_t info[2];
    kv_bank_state_t state[2];
    state[0] = scan_bank(store, 0U, &info[0]);
    state[1] = scan_bank(store, 1U, &info[1]);
    if (state[0] == KV_BANK_IO_ERROR || state[1] == KV_BANK_IO_ERROR) {
        return MAIA_KV_IO_ERROR;
    }
    if (state[0] == KV_BANK_VALID && state[1] == KV_BANK_VALID) {
        if (info[0].sequence == info[1].sequence &&
            (info[0].payload_bytes != info[1].payload_bytes ||
             info[0].payload_crc != info[1].payload_crc)) {
            return MAIA_KV_CORRUPT;
        }
        store->active_bank = sequence_newer(info[1].sequence,
                                             info[0].sequence) ? 1U : 0U;
    } else if (state[0] == KV_BANK_VALID) {
        store->active_bank = 0U;
    } else if (state[1] == KV_BANK_VALID) {
        store->active_bank = 1U;
    } else if (state[0] != KV_BANK_ERASED || state[1] != KV_BANK_ERASED) {
        store->ready = 1U;
        return MAIA_KV_RECOVERED_EMPTY;
    }
    if (store->active_bank != KV_BANK_EMPTY) {
        const kv_bank_info_t *active = &info[store->active_bank];
        store->sequence = active->sequence;
        store->payload_bytes = active->payload_bytes;
    }
    store->ready = 1U;
    return MAIA_KV_OK;
}

maia_kv_result_t maia_kv_get(maia_kv_store_t *store, const char *key,
                             void *value, uint32_t capacity,
                             uint32_t *out_length) {
    uint32_t key_bytes = 0U;
    if (store == NULL || out_length == NULL ||
        !key_length(key, &key_bytes) ||
        (value == NULL && capacity != 0U)) {
        return MAIA_KV_INVALID;
    }
    *out_length = 0U;
    if (!lock_store(store)) return MAIA_KV_BUSY;
    if (store->ready == 0U) {
        unlock_store(store);
        return MAIA_KV_NOT_READY;
    }
    if (store->active_bank == KV_BANK_EMPTY) {
        unlock_store(store);
        return MAIA_KV_NOT_FOUND;
    }

    uint32_t offset = 0U;
    maia_kv_result_t result = MAIA_KV_NOT_FOUND;
    while (offset < store->payload_bytes) {
        uint32_t bytes = 0U;
        int read_result = read_record(store, store->active_bank, offset,
                                      store->payload_bytes, &bytes);
        if (read_result < 0) {
            result = MAIA_KV_IO_ERROR;
            break;
        }
        if (read_result == 0) {
            result = MAIA_KV_CORRUPT;
            break;
        }
        uint8_t *record = (uint8_t *)store->scratch;
        uint16_t record_key_bytes = get_u16(record, 0U);
        uint16_t value_bytes = get_u16(record, 2U);
        if ((uint32_t)record_key_bytes == key_bytes &&
            memcmp(record + KV_RECORD_HEADER_BYTES, key, key_bytes) == 0) {
            *out_length = value_bytes;
            if ((uint32_t)value_bytes > capacity) {
                result = MAIA_KV_BUFFER_TOO_SMALL;
            } else if (value_bytes != 0U && value == NULL) {
                result = MAIA_KV_INVALID;
            } else {
                if (value_bytes != 0U) {
                    memcpy(value, record + KV_RECORD_HEADER_BYTES + key_bytes,
                           value_bytes);
                }
                result = MAIA_KV_OK;
            }
            break;
        }
        offset += bytes;
    }
    unlock_store(store);
    return result;
}

static maia_kv_result_t update_value(maia_kv_store_t *store, const char *key,
                                     const void *value, uint32_t length,
                                     int deleting) {
    uint32_t key_bytes = 0U;
    if (store == NULL || !key_length(key, &key_bytes) ||
        (deleting == 0 && length > MAIA_KV_VALUE_MAX) ||
        (deleting == 0 && value == NULL && length != 0U)) {
        return MAIA_KV_INVALID;
    }
    if (!lock_store(store)) return MAIA_KV_BUSY;
    if (store->ready == 0U) {
        unlock_store(store);
        return MAIA_KV_NOT_READY;
    }
    uint32_t target_bank = store->active_bank == 0U ? 1U : 0U;
    maia_kv_result_t result = write_snapshot(store, target_bank, key,
                                              key_bytes, value, length,
                                              deleting);
    unlock_store(store);
    return result;
}

maia_kv_result_t maia_kv_set(maia_kv_store_t *store, const char *key,
                             const void *value, uint32_t length) {
    return update_value(store, key, value, length, 0);
}

maia_kv_result_t maia_kv_delete(maia_kv_store_t *store, const char *key) {
    return update_value(store, key, NULL, 0U, 1);
}
