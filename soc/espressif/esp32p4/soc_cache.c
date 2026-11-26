#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_cache.h>

/* Addresses from ESP-IDF v5.4 components/esp_rom/esp32p4/ld/esp32p4.rom.ld. */
#define ESP32P4_ROM_ICACHE_LINE_SIZE UINT32_C(0x4FC003C4)
#define ESP32P4_ROM_CACHE_LINE_SIZE UINT32_C(0x4FC003C8)
#define ESP32P4_ROM_CACHE_INVALIDATE UINT32_C(0x4FC003E4)
#define ESP32P4_ROM_CACHE_WRITEBACK UINT32_C(0x4FC003F4)
#define ESP32P4_ROM_CACHE_WB_INVALIDATE UINT32_C(0x4FC003FC)
#define ESP32P4_CACHE_MAP_L1I_MASK UINT32_C(0x03)
#define ESP32P4_CACHE_MAP_L1D UINT32_C(0x10)

typedef int (*rom_cache_addr_op_t)(uint32_t, uint32_t, uint32_t);

uint32_t maia_esp32p4_cache_line_size(void) {
    uint32_t (*get_line_size)(void) =
        (uint32_t (*)(void))(uintptr_t)ESP32P4_ROM_CACHE_LINE_SIZE;
    return get_line_size();
}

int maia_esp32p4_cache_writeback_invalidate(uint32_t address, uint32_t size) {
    uint32_t line_size = maia_esp32p4_cache_line_size();
    if ((line_size != 16U && line_size != 32U && line_size != 64U &&
         line_size != 128U) ||
        (address % line_size) != 0U || size == 0U ||
        (size % line_size) != 0U || size > UINT32_MAX - address) {
        return -1;
    }

    int (*writeback_invalidate)(uint32_t, uint32_t, uint32_t) =
        (int (*)(uint32_t, uint32_t, uint32_t))
            (uintptr_t)ESP32P4_ROM_CACHE_WB_INVALIDATE;
    uintptr_t irq_state = maia_arch_irq_save();
    int result = writeback_invalidate(ESP32P4_CACHE_MAP_L1D, address, size);
    maia_arch_irq_restore(irq_state);
    return result;
}

static int valid_line_size(uint32_t line_size) {
    return line_size == 16U || line_size == 32U || line_size == 64U ||
           line_size == 128U;
}

int maia_esp32p4_cache_sync_code(uint32_t address, uint32_t size) {
    uint32_t dline = maia_esp32p4_cache_line_size();
    uint32_t iline = ((uint32_t (*)(void))(uintptr_t)ESP32P4_ROM_ICACHE_LINE_SIZE)();
    if (!valid_line_size(dline) || !valid_line_size(iline) || size == 0U ||
        size > UINT32_MAX - address) {
        return -1;
    }
    uint32_t line = dline > iline ? dline : iline;
    uint32_t start = address & ~(line - 1U);
    uint32_t end = address + size;
    if (end > UINT32_MAX - (line - 1U)) {
        return -1;
    }
    end = (end + line - 1U) & ~(line - 1U);

    rom_cache_addr_op_t writeback =
        (rom_cache_addr_op_t)(uintptr_t)ESP32P4_ROM_CACHE_WRITEBACK;
    rom_cache_addr_op_t invalidate =
        (rom_cache_addr_op_t)(uintptr_t)ESP32P4_ROM_CACHE_INVALIDATE;
    uintptr_t irq_state = maia_arch_irq_save();
    int wb = writeback(ESP32P4_CACHE_MAP_L1D, start, end - start);
    int inv = invalidate(ESP32P4_CACHE_MAP_L1I_MASK, start, end - start);
    maia_arch_irq_restore(irq_state);
    __asm__ volatile("fence rw, rw\n\tfence.i" ::: "memory");
    return (wb != 0 || inv != 0) ? -1 : 0;
}
