.text
.globl main
main:
    lui a0, %hi(msg)
    addi a0, a0, %lo(msg)
    auipc t0, %pcrel_hi(msg)
    addi t0, t0, %pcrel_lo(msg)
    ret
.data
msg: .word 42
