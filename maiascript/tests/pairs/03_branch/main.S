.text
.globl main
.align 2
main:
    addi t0, zero, 9
    addi t1, zero, 4
    blt t1, t0, greater
    add a0, t1, zero
    ret
greater:
    add a0, t0, zero
    ret
