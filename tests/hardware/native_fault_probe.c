/* RAM-only controlled fault injection; never a desktop replacement. */
#include <maia/console.h>
#include <maia/soc/esp32p4_display.h>
#include <stdint.h>

extern void maia_soc_trap_init(void);
extern void maia_probe_fault_entry(void);
extern const uint8_t maia_probe_fault_instruction[];
void maia_display_putc(char c) { (void)c; }
void maia_display_refresh_dma_if_needed(void) { }

static void hex32(uint32_t value) {
    static const char digits[] = "0123456789abcdef";
    for (unsigned i = 0U; i < 8U; ++i) {
        maia_console_putc(digits[(value >> (28U - 4U * i)) & 15U]);
    }
}

void maia_probe_fault_resumed(void);
void maia_probe_fault_resumed(void) {
    maia_console_puts("FAULT_RESUMED\n");
    for (;;) __asm__ volatile("wfi");
}

void maia_boot_main(void) {
    maia_soc_trap_init();
    maia_console_puts("FAULT_ARMED pc=");
    hex32((uint32_t)(uintptr_t)maia_probe_fault_instruction);
    maia_console_puts("\n");
    maia_probe_fault_entry();
    maia_probe_fault_resumed();
}
