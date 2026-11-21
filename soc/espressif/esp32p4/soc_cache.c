#include <maia/arch/irq.h>
#include <maia/soc/esp32p4_cache.h>

#define ESP32P4_ROM_CACHE_LINE_SIZE UINT32_C(0x4FC003C8)
#define ESP32P4_ROM_CACHE_WB_INVALIDATE UINT32_C(0x4FC003FC)
#define ESP32P4_CACHE_MAP_L1D UINT32_C(0x10)

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
