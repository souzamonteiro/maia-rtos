/*
 * MaiaRTOS RISC-V Bare-Metal Entry Point (M0)
 *
 * Responsibilities:
 * 1. Disable machine-mode interrupts during early bring-up.
 * 2. Park secondary hardware thread (mhartid != 0) until M3 SMP milestone.
 * 3. Initialize global pointer (gp) and 16-byte aligned stack pointer (sp).
 * 4. Zero the .bss section in internal L2MEM.
 * 5. Initialize optional compiler TLS/C++ state before entering the boot main.
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
    /* Early SoC init (disable ROM flashboot watchdogs). */
    call maia_soc_early_init
#ifdef MAIA_ENABLE_COMPILER_TLS
    call maia_tls_boot_init
    beqz a0, .Ltls_init_done
    la a0, .Ltls_boot_failure_message
    call maia_console_puts
    j .Lstartup_failed
.Ltls_init_done:
#endif
#ifdef MAIA_ENABLE_CXX_INIT_ARRAY
    call __libc_init_array
#endif
    call maia_boot_main

.Lstartup_failed:
    wfi
    j .Lstartup_failed

.Lsecondary_park:
    wfi
    j .Lsecondary_park

    .size _start, . - _start

#ifdef MAIA_ENABLE_COMPILER_TLS
    .section .rodata.tls_boot_failure, "a"
.Ltls_boot_failure_message:
    .asciz "[MaiaRTOS] compiler TLS bootstrap failed\n"
#endif
