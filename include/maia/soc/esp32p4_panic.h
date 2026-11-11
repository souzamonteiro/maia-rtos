#ifndef MAIA_SOC_ESP32P4_PANIC_H
#define MAIA_SOC_ESP32P4_PANIC_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Native single-core fatal record. valid is published last after a fence.
 * Debugger-readable RAM only; reset clears it. No stack/double-fault recovery.
 */
typedef struct maia_fatal_record {
    uint32_t valid;
    uint32_t cause;
    uint32_t pc;
    uint32_t value;
    uintptr_t sp;
    uint32_t stack_reason; /* Zero for ordinary fatal traps. */
    uintptr_t handler_sp;  /* Debug evidence of the diagnostic stack actually used. */
} maia_fatal_record_t;
extern volatile maia_fatal_record_t maia_last_fatal_trap;

/* Trap-only, noreturn. Disable IRQs, retain record, bounded best-effort USB
 * output, then WFI halt. No heap/locks/display/scheduler. Reset is external.
 */
__attribute__((noreturn)) void maia_esp32p4_fatal_trap(uint32_t cause, uint32_t pc,
                                                    uint32_t value, uintptr_t sp);
#ifdef __cplusplus
}
#endif
#endif
