.text
.align 2

.globl sum
sum:
    addi sp, sp, -96
    sw ra, 92(sp)
    sw a0, 64(sp)
    sw a1, 68(sp)
    addi t0, zero, 0
    sw t0, 72(sp)
    addi t0, zero, 0
    sw t0, 76(sp)
.Lfor_start_1:
    lw t0, 76(sp)
    sw t0, 0(sp)
    lw t1, 68(sp)
    lw t0, 0(sp)
    slt t0, t0, t1
    beq t0, zero, .Lfor_end_2
    lw t0, 64(sp)
    sw t0, 0(sp)
    lw t1, 76(sp)
    lw t0, 0(sp)
    slli t1, t1, 2
    add t0, t0, t1
    lw t0, 0(t0)
    lw t1, 72(sp)
    add t0, t1, t0
    sw t0, 72(sp)
    lw t0, 76(sp)
    addi t2, t0, 1
    sw t2, 76(sp)
    jal zero, .Lfor_start_1
.Lfor_end_2:
    lw a0, 72(sp)
    jal zero, .Lret_sum
    addi a0, zero, 0
.Lret_sum:
    lw ra, 92(sp)
    addi sp, sp, 96
    ret

.globl main
.globl maia_module_main
main:
maia_module_main:
    addi sp, sp, -80
    sw ra, 76(sp)
    addi t0, zero, 0
    sw t0, 68(sp)
.Lfor_start_3:
    lw t0, 68(sp)
    sw t0, 0(sp)
    addi t1, zero, 5
    lw t0, 0(sp)
    slt t0, t0, t1
    beq t0, zero, .Lfor_end_4
    lw t0, 68(sp)
    sw t0, 0(sp)
    addi t1, zero, 1
    lw t0, 0(sp)
    add t0, t0, t1
    sw t0, 0(sp)
    addi t1, zero, 10
    lw t0, 0(sp)
    mul t0, t0, t1
    sw t0, 0(sp)
    addi t1, sp, 64
    sw t1, 4(sp)
    lw t2, 68(sp)
    lw t1, 4(sp)
    lw t0, 0(sp)
    slli t2, t2, 2
    add t1, t1, t2
    sw t0, 0(t1)
    lw t0, 68(sp)
    addi t2, t0, 1
    sw t2, 68(sp)
    jal zero, .Lfor_start_3
.Lfor_end_4:
    addi t0, sp, 64
    sw t0, 0(sp)
    addi t0, zero, 5
    sw t0, 4(sp)
    lw a0, 0(sp)
    lw a1, 4(sp)
    jal ra, sum
    jal zero, .Lret_main
    addi a0, zero, 0
.Lret_main:
    lw ra, 76(sp)
    addi sp, sp, 80
    ret

