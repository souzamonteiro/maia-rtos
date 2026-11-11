# Native RISC-V context frame

Current scope: ESP32-P4 HP core, single-core M-mode, non-nested native traps.
Shared private layout: `include/maia/arch/riscv_context.h`. Scheduler and trap
objects must have the same ISA/ABI. The default remains RV32IMAC/ILP32. The
optional diagnostic profile uses RV32IMAFC/ILP32F; RV32D is rejected.

| Bytes from saved sp | Contents |
|---|---|
| 0–108 | Existing 28 words: ra, t0–t2, s0–s1, a0–a7, s2–s11, t3–t6 |
| 112 | mepc |
| 116 | mstatus |
| 120 | mcause, including CLIC previous priority |
| 124 | tp |
| 128–255, RV32F only | f0–f31, 32-bit bit-preserving slots |
| 256, RV32F only | entire fcsr |
| 260–271, RV32F only | alignment padding |

Integer context is 128 bytes; RV32F context is 272 bytes. Both preserve 16-byte
stack alignment. sp is represented by the selected stack/frame address and is
restored by adding the frame size. gp is the shared global pointer for the
current static single-image ABI and is not task-specific. A TLS runtime is not
provided, but tp itself is now preserved independently per task.

Fresh frames are zeroed, including tp, FP registers and fcsr. Initial mepc is the
task trampoline, a0 its task index, mstatus has MPP=M/MPIE=1/MIE=0, and mcause has
INT=1/MPP=M/MPIE=1/MPIL=0. RV32F also starts with FS=Initial. Actual hardware
return/assembly validation is documented in the native context audit reports.

RV32F traps eagerly save all FP state whenever FS is enabled (Initial, Clean or
Dirty). FS=Off skips FP instructions/CSRs on save and restore. Restoring enabled
FP state may conservatively mark FS Dirty; no lazy-switching optimization or
exact Initial/Clean-state fidelity is claimed. The mixed FS-Off/on trap path is hardware-tested in the dedicated mixed
profile; see `../audits/native-mixed-scheduling.md`. All state is saved before calling the C handler. Saved task return state also
sets INT=1 so a suspended ecall frame can be restored from an ISR without
leaving active MIL elevated. The original raw cause remains the handler argument,
so ecall/IRQ statistics are not altered by return normalization.
No heap allocation or blocking is added to the ISR path.

`maia_task_create` rejects less than frame size +128 bytes (256 integer, 400
RV32F) and pointer+length overflow before touching stack memory. This minimum
only bounds initial-frame construction; callers must size stacks for their
workload, interrupt depth and C call depth. There is no stack-overflow protection
or memory isolation claim. Caller buffer alignment may consume up to15 bytes.

All objects/modules crossing a C call boundary need matching float calling ABI;
this diagnostic does not migrate existing MaiaScript modules or prove their ABI
equivalence. The working legacy desktop remains a separate flash application.
Future general service integration still needs mixed FS-state tests, safe fatal
traps, priority/delay/yield stress and memory/cache/DMA contracts. SMP is separate.
