#ifndef MAIA_SOC_ESP32P4_FLASH_H
#define MAIA_SOC_ESP32P4_FLASH_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MAIA_ESP32P4_KV_FLASH_BASE UINT32_C(0x00C74000)
#define MAIA_ESP32P4_KV_FLASH_BYTES UINT32_C(0x00004000)

int maia_esp32p4_kv_flash_read(uint32_t offset, void *buffer,
                               uint32_t length);
int maia_esp32p4_kv_flash_write(uint32_t offset, const void *buffer,
                                uint32_t length);
int maia_esp32p4_kv_flash_erase(uint32_t offset, uint32_t length);

#ifdef __cplusplus
}
#endif

#endif
