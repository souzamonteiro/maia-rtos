#ifndef MAIA_ARCH_RISCV_CONTEXT_H
#define MAIA_ARCH_RISCV_CONTEXT_H

/* Private native context layout shared by C and preprocessed assembly.
 * Build every scheduler/trap object with the same ISA/ABI. No RV32D support.
 * Integer frame offsets stay stable; tp occupies the former reserved word.
 */
#define MAIA_RISCV_TP_OFFSET 124
#define MAIA_RISCV_FP_OFFSET 128
#define MAIA_RISCV_FCSR_OFFSET 256
#define MAIA_RISCV_FS_MASK 0x6000
/* Any task frame may be restored from an ISR, including a saved ecall frame. */
#define MAIA_RISCV_TASK_RETURN_FLAGS 0xB8000000

#if defined(__riscv_flen) && __riscv_flen == 32
#define MAIA_RISCV_FP32_CONTEXT 1
#define MAIA_RISCV_CONTEXT_BYTES 272
#define MAIA_RISCV_INITIAL_MSTATUS 0x00003880
#elif defined(__riscv_flen)
#error "Native context supports RV32 integer or RV32F only"
#else
#define MAIA_RISCV_FP32_CONTEXT 0
#define MAIA_RISCV_CONTEXT_BYTES 128
#define MAIA_RISCV_INITIAL_MSTATUS 0x00001880
#endif

#define MAIA_RISCV_CONTEXT_WORDS (MAIA_RISCV_CONTEXT_BYTES / 4)
#define MAIA_RISCV_STACK_GUARD_BYTES 16
#define MAIA_RISCV_STACK_GUARD_WORD 0x79B
/* Guard + worst alignment loss +128 bytes of initial task working room. */
#define MAIA_RISCV_MIN_STACK_BYTES (MAIA_RISCV_CONTEXT_BYTES + 176)
#endif
