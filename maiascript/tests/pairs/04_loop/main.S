.text
.globl main
.align 2
main:
    addi t0, zero, 1
    addi t1, zero, 0
    addi t2, zero, 6
loop:
    bge t0, t2, done
    add t1, t1, t0
    addi t0, t0, 1
    jal zero, loop
done:
    add a0, t1, zero
    ret
