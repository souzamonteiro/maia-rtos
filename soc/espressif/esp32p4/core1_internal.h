#ifndef MAIA_ESP32P4_CORE1_INTERNAL_H
#define MAIA_ESP32P4_CORE1_INTERNAL_H

#define MAIA_CORE1_COMMAND 0
#define MAIA_CORE1_ACK 4
#define MAIA_CORE1_PAYLOAD 8
#define MAIA_CORE1_RESULT 12
#define MAIA_CORE1_STATE 16
#define MAIA_CORE1_HART 20
#define MAIA_CORE1_SP 24
#define MAIA_CORE1_CAUSE 28
#define MAIA_CORE1_PC 32
#define MAIA_CORE1_STOP 0xffffffff
#define MAIA_CORE1_XOR 0x6d616961
#define MAIA_CORE1_STACK_BYTES 1024

#define MAIA_CORE1_CLOCK_REG 0x500e6014
#define MAIA_CORE1_CLOCK_BIT 0x10
#define MAIA_CORE1_RESET_REG 0x500e60c0
#define MAIA_CORE1_RESET_BIT 0x100
#define MAIA_CORE1_STALL_REG 0x50115200
#define MAIA_CORE1_STALL_MASK 0x00ff0000
#define MAIA_CORE1_STALLED_REG 0x500e5064
#define MAIA_CORE1_STALLED_BIT 2
#define MAIA_CORE1_BOOT_REG 0x50110164

#ifndef __ASSEMBLER__
#include <stddef.h>
#include <stdint.h>
typedef struct {
    uint32_t command, ack, payload, result, state, hart, sp, cause, pc;
} maia_core1_mailbox_t;
_Static_assert(offsetof(maia_core1_mailbox_t, pc) == MAIA_CORE1_PC,
               "Core-1 assembly mailbox layout");
extern maia_core1_mailbox_t maia_core1_mailbox;
extern uint32_t maia_core1_stack[MAIA_CORE1_STACK_BYTES / sizeof(uint32_t)];
void maia_core1_entry(void);
#endif
#endif
