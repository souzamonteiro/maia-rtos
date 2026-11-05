/*
 * Clean-Room ESP32-P4 Application Descriptor Header for Cold Flash Boot
 * Zero FreeRTOS / Zero ESP-IDF Runtime / Zero Arduino
 */

#include <stdint.h>

#define ESP_APP_DESC_MAGIC_WORD 0xABCD5432UL

typedef struct {
    uint32_t magic_word;
    uint32_t secure_version;
    uint32_t reserv1[2];
    char     version[32];
    char     project_name[32];
    char     time[16];
    char     date[16];
    char     idf_ver[32];
    uint8_t  app_elf_sha256[32];
    uint16_t min_efuse_blk_rev_full;
    uint16_t max_efuse_blk_rev_full;
    uint8_t  mmu_page_size;
    uint8_t  reserv3[3];
    uint32_t reserv2[18];
} __attribute__((packed)) esp_app_desc_t;

_Static_assert(sizeof(esp_app_desc_t) == 256, "esp_app_desc_t must be exactly 256 bytes");

const __attribute__((section(".rodata_desc"), used)) esp_app_desc_t g_maia_app_desc = {
    .magic_word = ESP_APP_DESC_MAGIC_WORD,
    .secure_version = 0U,
    .reserv1 = {0U, 0U},
    .version = "0.1.0",
    .project_name = "MaiaRTOS",
    .time = "12:00:00",
    .date = "Nov  4 2026",
    .idf_ver = "Zero-FreeRTOS/Zero-IDF",
    .app_elf_sha256 = {0U},
    .min_efuse_blk_rev_full = 0U,
    .max_efuse_blk_rev_full = 65535U,
    .mmu_page_size = 16U, /* 64KB page size */
    .reserv3 = {0U, 0U, 0U},
    .reserv2 = {0U}
};

const __attribute__((section(".flash.rodata"), used)) uint32_t g_dummy_flash_rodata = 0xCAFEBABEUL;
const __attribute__((section(".flash.text"), used)) uint32_t g_dummy_flash_text = 0xDEADBEEFUL;
