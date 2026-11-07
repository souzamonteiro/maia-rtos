.text
.globl main
.align 2
main:
    addi t0, zero, 85
    andi t0, t0, 15
    addi t1, zero, 3
    slli t1, t1, 4
    or a0, t0, t1
    ret
