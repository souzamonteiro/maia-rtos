/*
 * MaiaRTOS RISC-V Trap Vector & Preemptive/Cooperative Context Switch (M1/M2)
 * Target: ESP32-P4 HP RISC-V Core (RV32IMAFC, CLIC mode 3)
 * Zero FreeRTOS / Zero ESP-IDF runtime dependencies.
 */

#include <maia/arch/riscv_context.h>
#include <maia/arch/riscv_stack.h>

    .section .text.trap, "ax"
    .option push
    .option norvc

    .macro STACK_GUARD address, temporary, failure
    .irp offset,0,4,8,12
    lw \temporary, \offset(\address)
    xori \temporary, \temporary, MAIA_RISCV_STACK_GUARD_WORD
    bnez \temporary, \failure
    .endr
    .endm

    .global _maia_trap_entry
    .type   _maia_trap_entry, @function
    /* CLIC MTVEC mode 3 masks bits [5:0], so trap entry MUST be 64-byte aligned */
    .balign 64
_maia_trap_entry:
    /* No interrupted-stack store until bounds and guard are checked.
     * mscratch and these three scratch slots belong to this single-core port. */
    csrrw   t0, mscratch, t0
    sw      t1, MAIA_STACK_T1(t0)
    csrr    t1, mscratch
    sw      t1, MAIA_STACK_T0(t0)
    csrw    mscratch, t0
    sw      t2, MAIA_STACK_T2(t0)
    sw      sp, MAIA_STACK_SOURCE_SP(t0)
    csrr    t1, mcause
    sw      t1, MAIA_STACK_CAUSE(t0)
    csrr    t1, mepc
    sw      t1, MAIA_STACK_PC(t0)
    csrr    t1, mtval
    sw      t1, MAIA_STACK_VALUE(t0)
    lw      t1, MAIA_STACK_ACTIVE(t0)
    bnez    t1, .Lstack_nested
    li      t1, 1
    sw      t1, MAIA_STACK_ACTIVE(t0)
    andi    t1, sp, 15
    bnez    t1, .Lstack_sp
    addi    t1, sp, -MAIA_RISCV_CONTEXT_BYTES
    bltu    sp, t1, .Lstack_sp /* Detect unsigned subtraction underflow. */
    lw      t2, MAIA_STACK_LOW(t0)
    bltu    t1, t2, .Lstack_sp
    lw      t2, MAIA_STACK_HIGH(t0)
    bltu    t2, sp, .Lstack_sp
    lw      t1, MAIA_STACK_LOW(t0)
    addi    t1, t1, -MAIA_RISCV_STACK_GUARD_BYTES
    STACK_GUARD t1, t2, .Lstack_guard
    lw      t1, MAIA_STACK_IRQ_TOP(t0)
    li      t2, MAIA_IRQ_STACK_BYTES
    sub     t1, t1, t2
    STACK_GUARD t1, t2, .Lstack_irq_guard
    lw      t2, MAIA_STACK_T2(t0)
    lw      t1, MAIA_STACK_T1(t0)
    lw      t0, MAIA_STACK_T0(t0)
    addi    sp, sp, -MAIA_RISCV_CONTEXT_BYTES
    sw      ra,   0(sp)
    sw      t0,   4(sp)
    sw      t1,   8(sp)
    sw      t2,  12(sp)
    sw      s0,  16(sp)
    sw      s1,  20(sp)
    sw      a0,  24(sp)
    sw      a1,  28(sp)
    sw      a2,  32(sp)
    sw      a3,  36(sp)
    sw      a4,  40(sp)
    sw      a5,  44(sp)
    sw      a6,  48(sp)
    sw      a7,  52(sp)
    sw      s2,  56(sp)
    sw      s3,  60(sp)
    sw      s4,  64(sp)
    sw      s5,  68(sp)
    sw      s6,  72(sp)
    sw      s7,  76(sp)
    sw      s8,  80(sp)
    sw      s9,  84(sp)
    sw      s10, 88(sp)
    sw      s11, 92(sp)
    sw      t3,  96(sp)
    sw      t4, 100(sp)
    sw      t5, 104(sp)
    sw      t6, 108(sp)
    sw      tp, MAIA_RISCV_TP_OFFSET(sp) /* Per-task thread pointer; last word was reserved. */

    csrr    t0, mepc
    csrr    t1, mstatus
    csrr    t2, mcause
    sw      t0, 112(sp)

    mv      a0, sp
    mv      a1, t2
    mv      a2, t0
    csrr    a3, mtval

    /* On ESP32-P4 CLIC, mret restores MPP[29:28], MPIE[27], and MPIL[23:16] from mcause/mstatus */
    li      t3, 0x00001880
    or      t1, t1, t3
    sw      t1, 116(sp)
    /* a1 retains the raw trap cause for dispatch/statistics. A suspended ecall
     * frame can later be selected from a timer ISR: INT=1 is required on this
     * silicon to restore its saved MPIL. This changes return state, not events. */
    li      t3, MAIA_RISCV_TASK_RETURN_FLAGS
    or      t2, t2, t3
    sw      t2, 120(sp)

#if MAIA_RISCV_FP32_CONTEXT
    /* FS=Off has no usable FP state. All enabled states are saved eagerly. */
    li      t3, MAIA_RISCV_FS_MASK
    and     t3, t1, t3
    beqz    t3, .Lfp_saved
    .irp number,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    fsw f\number, (MAIA_RISCV_FP_OFFSET + 4 * \number)(sp)
    .endr
    csrr    t3, fcsr
    sw      t3, MAIA_RISCV_FCSR_OFFSET(sp)
.Lfp_saved:
#endif

    /* C handlers never borrow the application's remaining stack room. */
    csrr    t0, mscratch
    lw      sp, MAIA_STACK_IRQ_TOP(t0)
    call    maia_riscv_trap_handler
    csrr    t0, mscratch
    lw      t1, MAIA_STACK_IRQ_TOP(t0)
    li      t2, MAIA_IRQ_STACK_BYTES
    sub     t1, t1, t2
    STACK_GUARD t1, t2, .Lstack_irq_guard
    mv      sp, a0

_maia_trap_restore:
    /* Validate the selected frame again before even its first load. */
    csrr    t0, mscratch
    andi    t1, sp, 15
    bnez    t1, .Lstack_incoming
    lw      t1, MAIA_STACK_LOW(t0)
    bltu    sp, t1, .Lstack_incoming
    addi    t1, sp, MAIA_RISCV_CONTEXT_BYTES
    bltu    t1, sp, .Lstack_incoming
    lw      t2, MAIA_STACK_HIGH(t0)
    bltu    t2, t1, .Lstack_incoming
    lw      t1, MAIA_STACK_LOW(t0)
    addi    t1, t1, -MAIA_RISCV_STACK_GUARD_BYTES
    STACK_GUARD t1, t2, .Lstack_incoming
    lw      t0, 112(sp)
    lw      t1, 116(sp)
    lw      t2, 120(sp)
    csrw    mepc, t0
    csrw    mcause, t2
    andi    t1, t1, -9 /* Keep MIE off until mret, even for a damaged frame. */
    csrw    mstatus, t1

#if MAIA_RISCV_FP32_CONTEXT
    /* mstatus above restores FS before any FP instruction/CSR is accessed. */
    li      t3, MAIA_RISCV_FS_MASK
    and     t3, t1, t3
    beqz    t3, .Lfp_restored
    .irp number,0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31
    flw f\number, (MAIA_RISCV_FP_OFFSET + 4 * \number)(sp)
    .endr
    lw      t3, MAIA_RISCV_FCSR_OFFSET(sp)
    csrw    fcsr, t3
.Lfp_restored:
#endif

    lw      ra,   0(sp)
    lw      t1,   8(sp)
    lw      t2,  12(sp)
    lw      s0,  16(sp)
    lw      s1,  20(sp)
    lw      a0,  24(sp)
    lw      a1,  28(sp)
    lw      a2,  32(sp)
    lw      a3,  36(sp)
    lw      a4,  40(sp)
    lw      a5,  44(sp)
    lw      a6,  48(sp)
    lw      a7,  52(sp)
    lw      s2,  56(sp)
    lw      s3,  60(sp)
    lw      s4,  64(sp)
    lw      s5,  68(sp)
    lw      s6,  72(sp)
    lw      s7,  76(sp)
    lw      s8,  80(sp)
    lw      s9,  84(sp)
    lw      s10, 88(sp)
    lw      s11, 92(sp)
    lw      t3,  96(sp)
    lw      t4, 100(sp)
    lw      t5, 104(sp)
    lw      t6, 108(sp)
    lw      tp, MAIA_RISCV_TP_OFFSET(sp)
    csrr    t0, mscratch
    sw      zero, MAIA_STACK_ACTIVE(t0)
    lw      t0,   4(sp)
    addi    sp, sp, MAIA_RISCV_CONTEXT_BYTES
    mret

    .size _maia_trap_entry, .-_maia_trap_entry

    .global maia_arch_start_first_task
    .type   maia_arch_start_first_task, @function
maia_arch_start_first_task:
    mv      sp, a0
    j       _maia_trap_restore
    .size   maia_arch_start_first_task, .-maia_arch_start_first_task

    /* Dedicated ecall labels identify operations without consuming task-register
     * sentinels or changing the existing raw-yield ecall convention. */
    .global maia_arch_delay_trap
    .global maia_arch_delay_ecall
    .type maia_arch_delay_trap, @function
maia_arch_delay_trap:
maia_arch_delay_ecall:
    ecall /* a0 holds the uint32 delay; commit happens with IRQs disabled. */
    ret
    .size maia_arch_delay_trap, .-maia_arch_delay_trap

    .global maia_arch_exit_trap
    .global maia_arch_exit_ecall
    .type maia_arch_exit_trap, @function
maia_arch_exit_trap:
maia_arch_exit_ecall:
    ecall
    .4byte 0 /* An exited task must never resume here. */
    .size maia_arch_exit_trap, .-maia_arch_exit_trap

    /* Generic port transfer for C-detected bad selected contexts. a0..a4
     * already carry raw cause/PC/value/source-SP/reason. No return is possible. */
    .global maia_arch_stack_panic
    .type maia_arch_stack_panic, @function
maia_arch_stack_panic:
    csrci   mstatus, 8
    csrr    t0, mscratch
    lw      sp, MAIA_STACK_EMERGENCY_TOP(t0)
    tail    maia_arch_stack_fault
    .size maia_arch_stack_panic, .-maia_arch_stack_panic

.Lstack_sp:
    li      a4, MAIA_STACK_BAD_SP
    j       .Lstack_emergency
.Lstack_guard:
    li      a4, MAIA_STACK_BAD_GUARD
    j       .Lstack_emergency
.Lstack_incoming:
    li      a4, MAIA_STACK_BAD_CONTEXT
    j       .Lstack_emergency
.Lstack_nested:
    li      a4, MAIA_STACK_NESTED
    j       .Lstack_emergency
.Lstack_irq_guard:
    li      a4, MAIA_STACK_BAD_IRQ_GUARD
.Lstack_emergency:
    csrr    t0, mscratch
    lw      a0, MAIA_STACK_CAUSE(t0)
    lw      a1, MAIA_STACK_PC(t0)
    lw      a2, MAIA_STACK_VALUE(t0)
    lw      a3, MAIA_STACK_SOURCE_SP(t0)
    lw      sp, MAIA_STACK_EMERGENCY_TOP(t0)
    tail    maia_arch_stack_fault

    .option pop
