#include <maia/boards/tab5_kv.h>
#include <maia/soc/esp32p4_flash.h>

#include <stddef.h>

static maia_kv_store_t tab5_kv_store;

static int tab5_flash_read(void *context, uint32_t offset, void *buffer,
                           uint32_t length) {
    (void)context;
    return maia_esp32p4_kv_flash_read(offset, buffer, length);
}

static int tab5_flash_write(void *context, uint32_t offset,
                            const void *buffer, uint32_t length) {
    (void)context;
    return maia_esp32p4_kv_flash_write(offset, buffer, length);
}

static int tab5_flash_erase(void *context, uint32_t offset,
                            uint32_t length) {
    (void)context;
    return maia_esp32p4_kv_flash_erase(offset, length);
}

maia_kv_result_t maia_tab5_kv_init(void) {
    const maia_kv_flash_t flash = {
        .read = tab5_flash_read,
        .write = tab5_flash_write,
        .erase = tab5_flash_erase,
        .context = NULL,
        .region_bytes = MAIA_ESP32P4_KV_FLASH_BYTES,
        .erase_bytes = MAIA_KV_ERASE_BYTES
    };
    return maia_kv_init(&tab5_kv_store, &flash);
}

maia_kv_store_t *maia_tab5_kv_store(void) {
    return &tab5_kv_store;
}
