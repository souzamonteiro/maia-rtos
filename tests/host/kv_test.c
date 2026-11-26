#include <maia/services/kv.h>

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct mock_flash {
    uint8_t bytes[MAIA_KV_REGION_BYTES];
    uint32_t operations;
    int32_t fail_at;
} mock_flash_t;

static uintptr_t irq_enabled = 8U;

uintptr_t maia_arch_irq_save(void) {
    uintptr_t previous = irq_enabled;
    irq_enabled = 0U;
    return previous;
}

void maia_arch_irq_restore(uintptr_t previous) {
    irq_enabled = previous & 8U;
}

static int fail_operation(mock_flash_t *flash) {
    uint32_t operation = flash->operations++;
    return flash->fail_at >= 0 && operation == (uint32_t)flash->fail_at;
}

static int mock_read(void *context, uint32_t offset, void *buffer,
                     uint32_t length) {
    mock_flash_t *flash = (mock_flash_t *)context;
    if (flash == NULL || buffer == NULL || length == 0U ||
        ((uintptr_t)buffer & 3U) != 0U ||
        (offset & 3U) != 0U || (length & 3U) != 0U ||
        offset > MAIA_KV_REGION_BYTES ||
        length > MAIA_KV_REGION_BYTES - offset) {
        return -1;
    }
    if (fail_operation(flash)) return -1;
    memcpy(buffer, flash->bytes + offset, length);
    return 0;
}

static int mock_write(void *context, uint32_t offset, const void *buffer,
                      uint32_t length) {
    mock_flash_t *flash = (mock_flash_t *)context;
    const uint8_t *source = (const uint8_t *)buffer;
    if (flash == NULL || source == NULL || length == 0U ||
        ((uintptr_t)source & 3U) != 0U ||
        (offset & 3U) != 0U || (length & 3U) != 0U ||
        offset > MAIA_KV_REGION_BYTES ||
        length > MAIA_KV_REGION_BYTES - offset) {
        return -1;
    }
    uint32_t write_bytes = length;
    int fail = fail_operation(flash);
    if (fail != 0) write_bytes /= 2U;
    for (uint32_t i = 0U; i < write_bytes; ++i) {
        if ((flash->bytes[offset + i] & source[i]) != source[i]) return -1;
        flash->bytes[offset + i] &= source[i];
    }
    return fail != 0 ? -1 : 0;
}

static int mock_erase(void *context, uint32_t offset, uint32_t length) {
    mock_flash_t *flash = (mock_flash_t *)context;
    if (flash == NULL || length != MAIA_KV_ERASE_BYTES ||
        offset % MAIA_KV_ERASE_BYTES != 0U ||
        offset > MAIA_KV_REGION_BYTES ||
        length > MAIA_KV_REGION_BYTES - offset) {
        return -1;
    }
    int fail = fail_operation(flash);
    uint32_t erase_bytes = fail != 0 ? length / 2U : length;
    memset(flash->bytes + offset, UINT8_MAX, erase_bytes);
    return fail != 0 ? -1 : 0;
}

static maia_kv_flash_t mock_interface(mock_flash_t *flash) {
    maia_kv_flash_t interface = {
        .read = mock_read,
        .write = mock_write,
        .erase = mock_erase,
        .context = flash,
        .region_bytes = MAIA_KV_REGION_BYTES,
        .erase_bytes = MAIA_KV_ERASE_BYTES
    };
    return interface;
}

static void initialize_erased(mock_flash_t *flash) {
    memset(flash, 0, sizeof(*flash));
    memset(flash->bytes, UINT8_MAX, sizeof(flash->bytes));
    flash->fail_at = -1;
}

static void test_round_trip_and_recovery(void) {
    mock_flash_t flash;
    initialize_erased(&flash);
    maia_kv_flash_t interface = mock_interface(&flash);
    maia_kv_store_t store;
    assert(maia_kv_init(&store, &interface) == MAIA_KV_OK);

    uint8_t value[32];
    uint32_t length = 0U;
    assert(maia_kv_get(&store, "theme", value, sizeof(value), &length) ==
           MAIA_KV_NOT_FOUND);
    assert(maia_kv_set(&store, "theme", "dark", 4U) == MAIA_KV_OK);
    assert(maia_kv_set(&store, "language", "pt-BR", 5U) == MAIA_KV_OK);
    assert(maia_kv_set(&store, "theme", "light", 5U) == MAIA_KV_OK);
    assert(maia_kv_get(&store, "theme", value, 2U, &length) ==
           MAIA_KV_BUFFER_TOO_SMALL);
    assert(length == 5U);
    assert(maia_kv_get(&store, "theme", value, sizeof(value), &length) ==
           MAIA_KV_OK);
    assert(length == 5U && memcmp(value, "light", 5U) == 0);
    assert(maia_kv_get(&store, "language", value, sizeof(value), &length) ==
           MAIA_KV_OK);
    assert(length == 5U && memcmp(value, "pt-BR", 5U) == 0);
    assert(maia_kv_delete(&store, "language") == MAIA_KV_OK);
    assert(maia_kv_delete(&store, "language") == MAIA_KV_NOT_FOUND);

    maia_kv_store_t rebooted;
    assert(maia_kv_init(&rebooted, &interface) == MAIA_KV_OK);
    assert(maia_kv_get(&rebooted, "theme", value, sizeof(value), &length) ==
           MAIA_KV_OK);
    assert(length == 5U && memcmp(value, "light", 5U) == 0);
    assert(maia_kv_get(&rebooted, "language", value, sizeof(value), &length) ==
           MAIA_KV_NOT_FOUND);
}

static void test_validation_and_corruption(void) {
    mock_flash_t flash;
    initialize_erased(&flash);
    maia_kv_flash_t interface = mock_interface(&flash);
    maia_kv_store_t store;
    assert(maia_kv_init(&store, &interface) == MAIA_KV_OK);
    assert(maia_kv_set(&store, "", "x", 1U) == MAIA_KV_INVALID);
    assert(maia_kv_set(&store, "x", NULL, 1U) == MAIA_KV_INVALID);
    assert(maia_kv_set(&store, "x", "x", MAIA_KV_VALUE_MAX + 1U) ==
           MAIA_KV_INVALID);
    assert(maia_kv_get(&store, "x", NULL, 1U, &(uint32_t){ 0U }) ==
           MAIA_KV_INVALID);

    assert(maia_kv_set(&store, "x", "old", 3U) == MAIA_KV_OK);
    assert(maia_kv_set(&store, "x", "new", 3U) == MAIA_KV_OK);
    flash.bytes[MAIA_KV_BANK_BYTES + 40U] ^= 0x01U;
    uint8_t value[8];
    uint32_t length = 0U;
    assert(maia_kv_get(&store, "x", value, sizeof(value), &length) ==
           MAIA_KV_CORRUPT);
    maia_kv_store_t recovered;
    assert(maia_kv_init(&recovered, &interface) == MAIA_KV_OK);
    assert(maia_kv_get(&recovered, "x", value, sizeof(value), &length) ==
           MAIA_KV_OK);
    assert(length == 3U && memcmp(value, "old", 3U) == 0);

    flash.bytes[0] = 0U;
    flash.bytes[MAIA_KV_BANK_BYTES] = 0U;
    assert(maia_kv_init(&recovered, &interface) == MAIA_KV_RECOVERED_EMPTY);
    assert(maia_kv_set(&recovered, "x", "again", 5U) == MAIA_KV_OK);
}

static void test_interrupted_first_commit(void) {
    mock_flash_t baseline;
    initialize_erased(&baseline);

    for (int32_t fail_at = 0; fail_at < 32; ++fail_at) {
        mock_flash_t trial;
        memcpy(&trial, &baseline, sizeof(trial));
        maia_kv_flash_t interface = mock_interface(&trial);
        maia_kv_store_t store;
        assert(maia_kv_init(&store, &interface) == MAIA_KV_OK);

        trial.operations = 0U;
        trial.fail_at = fail_at;
        (void)maia_kv_set(&store, "first", "value", 5U);
        trial.fail_at = -1;

        maia_kv_store_t rebooted;
        maia_kv_result_t init = maia_kv_init(&rebooted, &interface);
        assert(init == MAIA_KV_OK || init == MAIA_KV_RECOVERED_EMPTY);
        uint8_t value[8];
        uint32_t length = 0U;
        maia_kv_result_t get = maia_kv_get(
            &rebooted, "first", value, sizeof(value), &length);
        if (get == MAIA_KV_OK) {
            assert(length == 5U && memcmp(value, "value", 5U) == 0);
        } else {
            assert(get == MAIA_KV_NOT_FOUND);
        }
    }
}

static void test_failed_update_is_atomic(void) {
    mock_flash_t baseline;
    initialize_erased(&baseline);
    maia_kv_flash_t baseline_interface = mock_interface(&baseline);
    maia_kv_store_t baseline_store;
    assert(maia_kv_init(&baseline_store, &baseline_interface) == MAIA_KV_OK);
    assert(maia_kv_set(&baseline_store, "mode", "safe", 4U) == MAIA_KV_OK);

    uint32_t failure_points = 0U;
    for (int32_t fail_at = 0; fail_at < 96; ++fail_at) {
        mock_flash_t trial;
        memcpy(&trial, &baseline, sizeof(trial));
        trial.operations = 0U;
        trial.fail_at = -1;
        maia_kv_flash_t interface = mock_interface(&trial);
        maia_kv_store_t store;
        assert(maia_kv_init(&store, &interface) == MAIA_KV_OK);

        trial.operations = 0U;
        trial.fail_at = fail_at;
        maia_kv_result_t update = maia_kv_set(&store, "mode", "fast", 4U);
        if (update != MAIA_KV_OK) ++failure_points;
        trial.fail_at = -1;

        maia_kv_store_t rebooted;
        assert(maia_kv_init(&rebooted, &interface) == MAIA_KV_OK);
        uint8_t value[8];
        uint32_t length = 0U;
        assert(maia_kv_get(&rebooted, "mode", value, sizeof(value), &length) ==
               MAIA_KV_OK);
        assert(length == 4U);
        assert(memcmp(value, "safe", 4U) == 0 ||
               memcmp(value, "fast", 4U) == 0);
        if (update == MAIA_KV_OK) {
            assert(memcmp(value, "fast", 4U) == 0);
        }
    }
    assert(failure_points > 0U);
}

int main(void) {
    test_round_trip_and_recovery();
    test_validation_and_corruption();
    test_interrupted_first_commit();
    test_failed_update_is_atomic();
    assert(irq_enabled == 8U);
    puts("maia_kv_test: all tests passed");
    return 0;
}
