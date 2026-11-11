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

`maia_task_create` now reserves an aligned 16-byte guard prefix inside the caller's
buffer and rejects less than frame size + 176 bytes (**304 integer, 448 RV32F**),
pointer+length overflow, live-task overlap or private port/idle stack reservations.
The minimum preserves guard/alignment space and initial working room, not a bound
on arbitrary task call depth. Unaligned input buffers remain supported by rounding
the guard start up and top down. Complete frame range/alignment and guard are
checked before selection/publication and again in assembly before restoration.

Trap entry uses mscratch-owned metadata to retain t0/t1/t2 and original SP in
fixed kernel storage, then validates interrupted SP/frame room and guard **before
any interrupted-stack store**. Once integer/optional FP state is saved, all C
trap work runs on a separate 4096-byte IRQ stack. Its guard is checked before and
after C dispatch. Bad SP/guard/destination uses a separate 2048-byte emergency
stack for the bounded fatal diagnostic. Idle has its own guard and bounds too.
All of these buffers remain in internal SRAM; no heap or external driver runtime.

This is single-core/non-nested protection after trap initialization. It is not
memory isolation: ordinary task writes may damage memory before the next check.
It does not guarantee recovery from corrupted mscratch/metadata, NMI, hardware
double fault or an emergency-stack failure. Fixed IRQ/emergency stack sizes are
not proven worst-case bounds for future handlers; no latency guarantee is made.

All objects/modules crossing a C call boundary need matching float calling ABI;
this diagnostic does not migrate existing MaiaScript modules or prove their ABI
equivalence. The working legacy desktop remains a separate flash application.
Mixed FS-state, yield/delay/priority and controlled fatal traps have dedicated
hardware tests. General service integration still needs operation atomicity,
memory/cache/DMA contracts and validation of native driver services. Idle/no-ready and local
scheduler exclusion are now covered by dedicated hardware profiles. SMP is separate.

## Fatal-trap policy

Only an interrupt-enabled M-mode ecall from an active scheduler task may advance
PC by 4 and select another task. Pure policy validates cause/MINHV/MPP/MPIE and
PC alignment/overflow; dispatch also checks task lifecycle. Yields with IRQs
disabled are invalid. Unknown synchronous exceptions, breakpoints (including
16-bit C.EBREAK) and unmapped IRQs enter the native SoC fatal path. It retains
raw cause/original PC/mtval/saved-frame sp; it never guesses instruction length.

Fatal handling disables MIE, publishes a debugger-readable volatile RAM record
(valid marker last after a memory fence), emits a fixed 73-byte hexadecimal
message through bounded best-effort USB polling, then halts in WFI until reset.
No normal console/display callback, heap, lock or scheduler is used. RAM state
is not persistent across reset. Stack faults switch to the dedicated emergency
buffer and add reason/emergency/source-guard information. This is fail-stop
diagnostics, not per-task recovery or a general nested/double-fault guarantee.
Trap metadata and internal SRAM must remain usable; USB output is best-effort.

## Scheduler transactions and idle

Delay and normal task return use private, labeled ECALL stubs. Dispatch changes
the sleeping/exited state only after saving the outgoing context, while IRQs
are disabled; PC+4 applies only to the validated ECALL. This removes the gap
between task-side state mutation and context save. Raw yield ECALLs remain valid.
Local MIE guards preserve nesting for metadata/tick/stat snapshots; they are
not SMP locks. Deadlines use unsigned half-range comparison for uint32 waits.

No-ready selects a separate internal idle context with a 2048-byte static stack
and FS Off; no application slot is consumed and current_id is -1. Idle retains
its own saved sp through timer traps and waits with interrupts enabled/WFI.
Blocked/exited apps keep their states; an application at any priority outranks
idle when READY. Idle diagnostics count actual timer ticks spent in that context.
The instrumentation hook used by the RAM idle profile is described in
`../audits/native-idle.md`; it does not inject ticks or task wakes.

Stack protection source, host negatives and exact injected-SP/emergency results:
`../audits/native-stack-protection.md`. No native desktop or SMP equivalence
is inferred from these limited context/stack tests.
