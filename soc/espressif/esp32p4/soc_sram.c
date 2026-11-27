#include <maia/soc/esp32p4_sram.h>
#include <maia/soc/esp32p4_cache.h>
#include <maia/soc/esp32p4_regs.h>
#include <stdint.h>

#define L2_CACHESIZE_CONF_REG 0x3FF10278U
#define L2_CACHESIZE_128K (1U << 9)
#define L2_CACHESIZE_256K (1U << 10)

extern uint32_t __maia_high_bss_start[], __maia_high_bss_end[];
extern unsigned char __maia_sram_reclaim_start[], __maia_sram_reclaim_end[];
extern unsigned char __maia_high_heap_start[], __maia_high_heap_end[];

int maia_esp32p4_sram_high_init(void) {
    uint32_t conf = MAIA_MMIO32(L2_CACHESIZE_CONF_REG);
    if (conf != L2_CACHESIZE_128K && conf != L2_CACHESIZE_256K) {
        return MAIA_SRAM_HIGH_CACHE_SIZE;
    }
    volatile uint32_t *word = __maia_high_bss_start;
    for (; word < __maia_high_bss_end; ++word) *word = 0U;
    __asm__ volatile("fence rw, rw" ::: "memory");
    for (word = __maia_high_bss_start; word < __maia_high_bss_end; ++word) {
        if (*word != 0U) return MAIA_SRAM_HIGH_VERIFY;
    }
    return MAIA_SRAM_HIGH_OK;
}

void maia_esp32p4_sram_reclaim_region(void **base, size_t *bytes) {
    *base = __maia_sram_reclaim_start;
    *bytes = (size_t)(__maia_sram_reclaim_end - __maia_sram_reclaim_start);
}

void maia_esp32p4_sram_high_heap_region(void **base, size_t *bytes) {
    *base = __maia_high_heap_start;
    *bytes = (size_t)(__maia_high_heap_end - __maia_high_heap_start);
}

int maia_esp32p4_sram_exec_probe(void *base) {
    volatile uint32_t *code = (volatile uint32_t *)base;
    code[0] = 0x05A00513U; /* li a0, 0x5A */
    code[1] = 0x00008067U; /* ret */
    /* L1D is write-back: push the words to L2MEM before fetching them. */
    if (maia_esp32p4_cache_sync_code((uint32_t)(uintptr_t)base, 8U) != 0) return 0;
    __asm__ volatile("fence rw, rw\n\tfence.i" ::: "memory");
    int (*fn)(void) = (int (*)(void))base;
    int result = fn();
    code[0] = 0U;
    code[1] = 0U;
    __asm__ volatile("fence rw, rw\n\tfence.i" ::: "memory");
    return result == 0x5A;
}
