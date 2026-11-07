.text
.globl main
.align 2
fact:
    addi sp, sp, -16
    sw ra, 12(sp)
    sw a0, 8(sp)
    addi t0, zero, 2
    blt a0, t0, base
    addi a0, a0, -1
    jal ra, fact
    lw t0, 8(sp)
    mul a0, a0, t0
    jal zero, finish
base:
    addi a0, zero, 1
finish:
    lw ra, 12(sp)
    addi sp, sp, 16
    ret
main:
    addi sp, sp, -16
    sw ra, 12(sp)
    addi a0, zero, 5
    jal ra, fact
    lw ra, 12(sp)
    addi sp, sp, 16
    ret
