#include <maia/soc/esp32p4_panic.h>
#include <maia/arch/riscv_stack.h>
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

static __attribute__((noreturn)) void fatal_halt(uint32_t cause, uint32_t pc,
                            uint32_t value, uintptr_t sp, uint32_t reason) {
    __asm__ volatile("csrci mstatus, 8" ::: "memory");
    if (s_panicking != 0U) {
        for (;;) __asm__ volatile("wfi");
    }
    s_panicking = 1U;
    uintptr_t handler_sp;
    __asm__ volatile("mv %0, sp" : "=r"(handler_sp));
    maia_last_fatal_trap.valid = 0U;
    maia_last_fatal_trap.cause = cause;
    maia_last_fatal_trap.pc = pc;
    maia_last_fatal_trap.value = value;
    maia_last_fatal_trap.sp = sp;
    maia_last_fatal_trap.stack_reason = reason;
    maia_last_fatal_trap.handler_sp = handler_sp;
    __asm__ volatile("fence rw, rw" ::: "memory");
    maia_last_fatal_trap.valid = 0x4D414941U;

    /* Flush a possible partial normal-console packet; use no console globals. */
    MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) = MAIA_ESP32P4_USB_WR_DONE;
    if (reason != 0U) {
        uintptr_t top = maia_trap_stack_metadata.emergency_top;
        int emergency = handler_sp >= top-MAIA_EMERGENCY_STACK_BYTES+MAIA_RISCV_STACK_GUARD_BYTES &&
                        handler_sp < top && maia_stack_guard_valid(top-MAIA_EMERGENCY_STACK_BYTES);
        int guard = maia_stack_guard_valid(maia_trap_stack_metadata.low-MAIA_RISCV_STACK_GUARD_BYTES);
        panic_literal("MAIA_STACK_FAULT reason="); panic_byte((char)('0'+reason));
        panic_literal(" emergency="); panic_byte(emergency ? '1' : '0');
        panic_literal(" guard="); panic_byte(guard ? '1' : '0');
        panic_literal("\r\n");
    }
    panic_literal("MAIA_NATIVE_FATAL cause="); panic_hex(cause);
    panic_literal(" pc="); panic_hex(pc);
    panic_literal(" value="); panic_hex(value);
    panic_literal(" sp="); panic_hex((uint32_t)sp);
    panic_literal("\r\n");
    MAIA_MMIO32(MAIA_ESP32P4_USB_EP1_CONF_REG) = MAIA_ESP32P4_USB_WR_DONE;
    for (;;) __asm__ volatile("wfi");
}

void maia_esp32p4_fatal_trap(uint32_t cause, uint32_t pc, uint32_t value, uintptr_t sp) {
    fatal_halt(cause,pc,value,sp,0U);
}
void maia_arch_stack_fault(uint32_t cause, uint32_t pc, uint32_t value,
                           uintptr_t sp, uint32_t reason) {
    fatal_halt(cause,pc,value,sp,reason);
}
