.equ BUFFER_SIZE, (64 + 32) * 4
.text
main:
    addi a0, zero, 8 + 7
    slli a0, a0, 2
    ret
