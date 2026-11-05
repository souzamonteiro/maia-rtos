/*
 * MaiaRTOS RISC-V Bare-Metal Entry Point (M0)
 *
 * Responsibilities:
 * 1. Disable machine-mode interrupts during early bring-up.
 * 2. Park secondary hardware thread (mhartid != 0) until M3 SMP milestone.
 * 3. Initialize global pointer (gp) and 16-byte aligned stack pointer (sp).
 * 4. Zero the .bss section in internal L2MEM.
 * 5. Call maia_soc_early_init() and maia_boot_main().
 */

    .section .text.entry, "ax"
    .global _start
    .type _start, @function
_start:
    /* 1. Disable machine interrupts and clear pending masks */
    csrw mstatus, zero
    csrw mie, zero

    /* 2. Check hardware thread ID: only core 0 runs M0 boot */
    csrr t0, mhartid
    bnez t0, .Lsecondary_park

    /* 3. Set global pointer and stack pointer (16-byte aligned) */
    .option push
    .option norelax
    la gp, __global_pointer$
    .option pop
    la sp, __stack_top
    andi sp, sp, -16

    /* 4. Zero .bss [__bss_start, __bss_end) */
    la t0, __bss_start
    la t1, __bss_end
.Lbss_loop:
    bgeu t0, t1, .Lbss_done
    sw zero, 0(t0)
    addi t0, t0, 4
    j .Lbss_loop

.Lbss_done:
    /* 5. Early SoC init (disable ROM flashboot watchdogs) & enter C main */
    call maia_soc_early_init
    call maia_boot_main

.Lsecondary_park:
    wfi
    j .Lsecondary_park

    .size _start, . - _start
