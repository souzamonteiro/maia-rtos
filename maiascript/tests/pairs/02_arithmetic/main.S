.text
.globl main
.align 2
main:
    addi t0, zero, 7
    addi t1, zero, 5
    mul a0, t0, t1
    addi a0, a0, 3
    ret
