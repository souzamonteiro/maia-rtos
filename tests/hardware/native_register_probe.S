/* Live register stress for the RAM-only scheduler diagnostic.
 * Caller-saved registers must survive asynchronous preemption too.
 * gp/sp retain ABI values; tp is a task-specific sentinel, not allocated TLS.
 * No C calls, yields or synthetic ticks occur between seeding and validation.
 */
    .section .text.probe_registers, "ax"
    .option push
    .option norvc

    .macro CHECK_REG reg, number, seed, task
    li t0, (\seed + \number)
    beq \reg, t0, 1f
    mv a1, \reg
    li a0, \number
    li a2, (\seed + \number)
    li a3, \task
    tail maia_probe_register_error
1:
    .endm

    .macro WORKER name, seed, task
    .global \name
    .type \name, @function
\name:
    addi sp, sp, -32
    sw sp, 16(sp)
    li t0, (\seed + 0x55)
    sw t0, 20(sp)
    sw gp, 24(sp)
.Lagain\@:
    .irp number,1,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    li x\number, (\seed + \number)
    .endr
    .rept 128
    nop
    .endr
    /* Preserve observations of the two scratch registers before using them. */
    sw t0, 8(sp)
    sw t1, 12(sp)
    lw t1, 8(sp)
    CHECK_REG t1, 5, \seed, \task
    lw t1, 12(sp)
    CHECK_REG t1, 6, \seed, \task
    .irp number,1,4,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    CHECK_REG x\number, \number, \seed, \task
    .endr
    lw t0, 16(sp)
    beq sp, t0, 2f
    mv a1, sp
    li a0, 2
    mv a2, t0
    li a3, \task
    tail maia_probe_register_error
2:
    lw t0, 24(sp)
    beq gp, t0, 3f
    mv a1, gp
    li a0, 3
    mv a2, t0
    li a3, \task
    tail maia_probe_register_error
3:
    lw t1, 20(sp)
    CHECK_REG t1, 0x55, \seed, \task
    li a0, \task
    call maia_probe_register_checkpoint
    j .Lagain\@
    .size \name, .-\name
    .endm

    WORKER maia_probe_register_worker_a, 0xA1000000, 0
    WORKER maia_probe_register_worker_b, 0xB2000000, 1
    .option pop
