.text
.globl main
.align 2
main:
    addi sp, sp, -16
    addi t0, zero, 7
    sw t0, 12(sp)
    addi t1, sp, 12
    addi t0, zero, 9
    sw t0, 0(t1)
    lw a0, 12(sp)
    addi sp, sp, 16
    ret
