#include "../../soc/espressif/esp32p4/core1_internal.h"

    .section .maia_core1.text, "ax"
    .option push
    .option norelax
    .balign 64
    .global maia_core1_entry
    .type maia_core1_entry, @function
maia_core1_entry:
    csrw mstatus, zero
    csrw mie, zero
    la gp, __global_pointer$
    li tp, 0
    la sp, maia_core1_stack
    addi sp, sp, MAIA_CORE1_STACK_BYTES
    la t0, .Lcore1_fault
    csrw mtvec, t0
    la t0, maia_core1_mailbox
    csrr t1, mhartid
    sw t1, MAIA_CORE1_HART(t0)
    sw sp, MAIA_CORE1_SP(t0)
    li t1, MAIA_CORE1_BOOT_REG
    sw zero, 0(t1)
    fence iorw, iorw
    li t1, 1
    fence rw, w
    sw t1, MAIA_CORE1_STATE(t0)

.Lcore1_wait:
    lw t1, MAIA_CORE1_COMMAND(t0)
    fence r, rw
    lw t2, MAIA_CORE1_ACK(t0)
    beq t1, t2, .Lcore1_wait
    li t2, MAIA_CORE1_STOP
    beq t1, t2, .Lcore1_stop
    lw t2, MAIA_CORE1_PAYLOAD(t0)
    li t3, MAIA_CORE1_XOR
    xor t2, t2, t3
    add t2, t2, t1
    sw t2, MAIA_CORE1_RESULT(t0)
    fence rw, w
    sw t1, MAIA_CORE1_ACK(t0)
    j .Lcore1_wait

.Lcore1_stop:
    li t2, 2
    sw t2, MAIA_CORE1_STATE(t0)
    fence rw, w
    sw t1, MAIA_CORE1_ACK(t0)
.Lcore1_park:
    wfi
    j .Lcore1_park
    .size maia_core1_entry, .-maia_core1_entry

    .balign 64
.Lcore1_fault:
    csrw mie, zero
    la t0, maia_core1_mailbox
    csrr t1, mcause
    sw t1, MAIA_CORE1_CAUSE(t0)
    csrr t1, mepc
    sw t1, MAIA_CORE1_PC(t0)
    li t1, 3
    fence rw, w
    sw t1, MAIA_CORE1_STATE(t0)
    j .Lcore1_park
    .option pop
