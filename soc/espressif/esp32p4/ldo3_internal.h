#ifndef MAIA_ESP32P4_LDO3_INTERNAL_H
#define MAIA_ESP32P4_LDO3_INTERNAL_H
#include <maia/soc/esp32p4_regs.h>
#ifdef MAIA_LDO3_TEST
int maia_ldo3_test_context(void);
uint32_t maia_ldo3_test_read(uintptr_t address);
void maia_ldo3_test_write(uintptr_t address,uint32_t value);
#define maia_ldo3_boot_context maia_ldo3_test_context
#define maia_ldo3_read maia_ldo3_test_read
#define maia_ldo3_write maia_ldo3_test_write
#else
static inline int maia_ldo3_boot_context(void) {
    uint32_t status,hart;
    __asm__ volatile("csrr %0, mstatus\n\tcsrr %1, mhartid" : "=r"(status),"=r"(hart));
    return !(status&8U) && !hart;
}
static inline uint32_t maia_ldo3_read(uintptr_t address) { return MAIA_MMIO32(address); }
static inline void maia_ldo3_write(uintptr_t address,uint32_t value) {
    MAIA_MMIO32(address)=value;
    __asm__ volatile("fence iorw, iorw" ::: "memory");
}
#endif
#endif
