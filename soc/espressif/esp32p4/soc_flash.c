#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_flash.h>

#include <stddef.h>
#include <stdint.h>

#define ESP32P4_FLASH_ERASE_BYTES UINT32_C(0x1000)
#define ESP32P4_SRAM_LOW_START UINT32_C(0x4FF00000)
#define ESP32P4_SRAM_LOW_END UINT32_C(0x4FF28000)

#define ESP32P4_ROM_CACHE_SUSPEND_L2 UINT32_C(0x4FC00508)
#define ESP32P4_ROM_CACHE_RESUME_L2 UINT32_C(0x4FC0050C)
#define ESP32P4_ROM_CACHE_WRITEBACK_ALL UINT32_C(0x4FC00414)
#define ESP32P4_CACHE_MAP_L1D UINT32_C(0x10)
#define ESP32P4_MHCR_BRANCH_PREDICTION_MASK UINT32_C(0x1030)
#define ESP32P4_ROM_FLASH_ERASE_SECTOR UINT32_C(0x4FC0014C)
#define ESP32P4_ROM_FLASH_WRITE UINT32_C(0x4FC00154)
#define ESP32P4_ROM_FLASH_READ UINT32_C(0x4FC00158)

typedef enum flash_operation {
    FLASH_OPERATION_READ,
    FLASH_OPERATION_WRITE,
    FLASH_OPERATION_ERASE
} flash_operation_t;

typedef uint32_t (*rom_cache_suspend_fn)(void);
typedef void (*rom_cache_resume_fn)(uint32_t);
typedef int (*rom_cache_writeback_all_fn)(uint32_t);
typedef int (*rom_flash_read_fn)(uint32_t, uint32_t *, int32_t);
typedef int (*rom_flash_write_fn)(uint32_t, const uint32_t *, int32_t);
typedef int (*rom_flash_erase_fn)(uint32_t);

static int __attribute__((section(".maia_flash_iram"), noinline))
flash_transaction(flash_operation_t operation, uint32_t address,
                  uintptr_t buffer, uint32_t length) {
    uint32_t hart = 0U;
    __asm__ volatile("csrr %0, mhartid" : "=r"(hart));
    if (hart != 0U) return -1;

    rom_cache_suspend_fn suspend_l2 =
        (rom_cache_suspend_fn)(uintptr_t)ESP32P4_ROM_CACHE_SUSPEND_L2;
    rom_cache_resume_fn resume_l2 =
        (rom_cache_resume_fn)(uintptr_t)ESP32P4_ROM_CACHE_RESUME_L2;
    rom_cache_writeback_all_fn writeback_all =
        (rom_cache_writeback_all_fn)(uintptr_t)ESP32P4_ROM_CACHE_WRITEBACK_ALL;

    if (writeback_all(ESP32P4_CACHE_MAP_L1D) != 0) return -1;

    uint32_t branch_prediction_state;
    uint32_t branch_prediction_mask =
        ESP32P4_MHCR_BRANCH_PREDICTION_MASK;
    __asm__ volatile("csrrc %0, 0x7c1, %1"
                     : "=r"(branch_prediction_state)
                     : "r"(branch_prediction_mask)
                     : "memory");

    uint32_t l2_autoload = suspend_l2();

    __asm__ volatile("fence rw, rw" ::: "memory");
    int result = -1;
    if (operation == FLASH_OPERATION_READ) {
        rom_flash_read_fn read_flash =
            (rom_flash_read_fn)(uintptr_t)ESP32P4_ROM_FLASH_READ;
        result = read_flash(address, (uint32_t *)buffer, (int32_t)length);
    } else if (operation == FLASH_OPERATION_WRITE) {
        rom_flash_write_fn write_flash =
            (rom_flash_write_fn)(uintptr_t)ESP32P4_ROM_FLASH_WRITE;
        result = write_flash(address, (const uint32_t *)buffer,
                             (int32_t)length);
    } else if (operation == FLASH_OPERATION_ERASE) {
        rom_flash_erase_fn erase_sector =
            (rom_flash_erase_fn)(uintptr_t)ESP32P4_ROM_FLASH_ERASE_SECTOR;
        for (uint32_t offset = 0U; offset < length;
             offset += ESP32P4_FLASH_ERASE_BYTES) {
            result = erase_sector(
                (address + offset) / ESP32P4_FLASH_ERASE_BYTES);
            if (result != 0) break;
        }
    }
    __asm__ volatile("fence rw, rw" ::: "memory");

    resume_l2(l2_autoload);
    uint32_t branch_prediction_restore =
        branch_prediction_state & ESP32P4_MHCR_BRANCH_PREDICTION_MASK;
    __asm__ volatile("csrs 0x7c1, %0"
                     :
                     : "r"(branch_prediction_restore)
                     : "memory");
    return result;
}

static int valid_buffer(const void *buffer, uint32_t length) {
    uintptr_t address = (uintptr_t)buffer;
    if (buffer == NULL || length == 0U || (address & 3U) != 0U ||
        address < ESP32P4_SRAM_LOW_START ||
        address > ESP32P4_SRAM_LOW_END ||
        length > ESP32P4_SRAM_LOW_END - address) {
        return 0;
    }
    return 1;
}

static int valid_range(uint32_t offset, uint32_t length) {
    return length != 0U && offset <= MAIA_ESP32P4_KV_FLASH_BYTES &&
           length <= MAIA_ESP32P4_KV_FLASH_BYTES - offset;
}

int maia_esp32p4_kv_flash_read(uint32_t offset, void *buffer,
                               uint32_t length) {
    if (!valid_range(offset, length) || (offset & 3U) != 0U ||
        (length & 3U) != 0U || !valid_buffer(buffer, length)) {
        return -1;
    }
    uintptr_t irq = maia_arch_irq_save();
    int result = flash_transaction(FLASH_OPERATION_READ,
                                   MAIA_ESP32P4_KV_FLASH_BASE + offset,
                                   (uintptr_t)buffer, length);
    maia_arch_irq_restore(irq);
    return result;
}

int maia_esp32p4_kv_flash_write(uint32_t offset, const void *buffer,
                                uint32_t length) {
    if (!valid_range(offset, length) || (offset & 3U) != 0U ||
        (length & 3U) != 0U || !valid_buffer(buffer, length)) {
        return -1;
    }
    uintptr_t irq = maia_arch_irq_save();
    int result = flash_transaction(FLASH_OPERATION_WRITE,
                                   MAIA_ESP32P4_KV_FLASH_BASE + offset,
                                   (uintptr_t)buffer, length);
    maia_arch_irq_restore(irq);
    return result;
}

int maia_esp32p4_kv_flash_erase(uint32_t offset, uint32_t length) {
    if (!valid_range(offset, length) ||
        (offset % ESP32P4_FLASH_ERASE_BYTES) != 0U ||
        (length % ESP32P4_FLASH_ERASE_BYTES) != 0U) {
        return -1;
    }
    uintptr_t irq = maia_arch_irq_save();
    int result = flash_transaction(FLASH_OPERATION_ERASE,
                                   MAIA_ESP32P4_KV_FLASH_BASE + offset,
                                   0U, length);
    maia_arch_irq_restore(irq);
    return result;
}
