.text
.align 2

.globl compute
compute:
    addi sp, sp, -80
    sw ra, 76(sp)
    fsw fa0, 64(sp)
    fsw fa1, 68(sp)
    flw ft0, 64(sp)
    fsw ft0, 0(sp)
    flw ft1, 68(sp)
    flw ft0, 0(sp)
    fmul.s ft0, ft0, ft1
    fsw ft0, 0(sp)
    li t0, 1075838976
    fmv.w.x ft1, t0
    flw ft0, 0(sp)
    fadd.s ft0, ft0, ft1
    fsw ft0, 72(sp)
    flw fa0, 72(sp)
    jal zero, .Lret_compute
    fmv.w.x fa0, zero
.Lret_compute:
    lw ra, 76(sp)
    addi sp, sp, 80
    ret

.globl main
.globl maia_module_main
main:
maia_module_main:
    addi sp, sp, -80
    sw ra, 76(sp)
    li t0, 1082130432
    fmv.w.x ft0, t0
    fsw ft0, 0(sp)
    li t0, 1084227584
    fmv.w.x ft0, t0
    fsw ft0, 4(sp)
    flw fa0, 0(sp)
    flw fa1, 4(sp)
    jal ra, compute
    fsgnj.s ft0, fa0, fa0
    fsw ft0, 64(sp)
    flw ft0, 64(sp)
    fcvt.w.s t0, ft0
    sw t0, 68(sp)
    lw t0, 68(sp)
    sw t0, 0(sp)
    addi t1, zero, 20
    lw t0, 0(sp)
    add a0, t0, t1
    jal zero, .Lret_main
    addi a0, zero, 0
.Lret_main:
    lw ra, 76(sp)
    addi sp, sp, 80
    ret

