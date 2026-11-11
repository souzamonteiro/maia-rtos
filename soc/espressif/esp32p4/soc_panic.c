#include <maia/soc/esp32p4_panic.h>
#include <maia/soc/esp32p4_regs.h>

#define PANIC_POLL_LIMIT 4096U
volatile maia_fatal_record_t maia_last_fatal_trap;
static volatile uint32_t s_panicking;
static uint32_t s_packet_bytes;

static void panic_byte(char c) {
    for (uint32_t i = 0U; i < PANIC_POLL_LIMIT; ++i) {
        if ((MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) &
             MAIA_ESP32P4_USB_SERIAL_IN_EP_DATA_FREE) != 0U) {
            MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_REG) = (uint8_t)c;
            if (++s_packet_bytes == 32U) {
                MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) = MAIA_ESP32P4_USB_WR_DONE;
                s_packet_bytes = 0U;
            }
            return;
        }
    }
}

static void panic_literal(const char *text) {
    for (uint32_t i = 0U; i < 64U && text[i] != '\0'; ++i) panic_byte(text[i]);
}

static void panic_hex(uint32_t value) {
    static const char digits[] = "0123456789abcdef";
    for (uint32_t i = 0U; i < 8U; ++i) panic_byte(digits[(value >> (28U - 4U*i)) & 15U]);
}

void maia_esp32p4_fatal_trap(uint32_t cause, uint32_t pc,
                            uint32_t value, uintptr_t sp) {
    __asm__ volatile("csrci mstatus, 8" ::: "memory");
    if (s_panicking != 0U) {
        for (;;) __asm__ volatile("wfi");
    }
    s_panicking = 1U;
    maia_last_fatal_trap.valid = 0U;
    maia_last_fatal_trap.cause = cause;
    maia_last_fatal_trap.pc = pc;
    maia_last_fatal_trap.value = value;
    maia_last_fatal_trap.sp = sp;
    __asm__ volatile("fence rw, rw" ::: "memory");
    maia_last_fatal_trap.valid = 0x4D414941U;

    /* Flush a possible partial normal-console packet; use no console globals. */
    MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) = MAIA_ESP32P4_USB_WR_DONE;
    panic_literal("MAIA_NATIVE_FATAL cause="); panic_hex(cause);
    panic_literal(" pc="); panic_hex(pc);
    panic_literal(" value="); panic_hex(value);
    panic_literal(" sp="); panic_hex((uint32_t)sp);
    panic_literal("\r\n");
    MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) = MAIA_ESP32P4_USB_WR_DONE;
    for (;;) __asm__ volatile("wfi");
}
