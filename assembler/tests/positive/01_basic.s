.text
.globl main
main:
    addi sp, sp, -16
    sw ra, 12(sp)
    li a0, 15
    lw ra, 12(sp)
    addi sp, sp, 16
    ret
