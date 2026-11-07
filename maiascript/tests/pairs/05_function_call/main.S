.text
.globl main
.align 2
square:
    mul a0, a0, a0
    ret
main:
    addi sp, sp, -16
    sw ra, 12(sp)
    addi a0, zero, 6
    jal ra, square
    lw ra, 12(sp)
    addi sp, sp, 16
    ret
