# Native stack protection — 2026-11-11

Implemented independent stack bounds/guards, pre-save entry checks, dedicated
IRQ and emergency stacks, and validation before restoring selected contexts.
All hardware tests execute in RAM; the preserved flash desktop is not replaced.
No FreeRTOS/ESP-IDF implementation, runtime, parser/lexer/EBNF or module ABI edit.

## Implementation and invariants

- A 16-byte aligned guard lies inside each task/idle/boot/IRQ/emergency buffer.
  Input buffers may be unaligned; alignment adjustment never writes outside the
  supplied region. New task minimum is **304 bytes integer / 448 RV32F**, including
  guard, worst alignment loss and initial working room. Callers still size for
  their actual workload. Active stack overlap and port/idle reservations are
  rejected before writing frames. Reusing an EXITED slot/buffer remains allowed.
- Scheduler validates complete saved-frame range/alignment and guards before
  storing supplied SP or publishing the next context. On failure it returns 0;
  native trap dispatch moves to emergency diagnostics, never pops that frame.
- The mscratch pointer references a project-owned 48-byte single-core metadata
  layout. Entry retains t0/t1/t2 there without using the interrupted stack,
  records raw cause/PC/value/SP and checks SP alignment, subtraction underflow,
  frame room and guard **before** the first store through sp. The context ABI
  and integer/FP slots themselves stay 128/272 bytes with unchanged offsets.
- C trap handling runs on a separate 4096-byte kernel IRQ stack; its guard is
  checked before/after dispatch. Return checks frame geometry/guard before its
  first load and keeps MIE off until mret. Active trap state clears only at the
  final GPR restoration. General nested interrupts remain unsupported.
- Bad stacks transfer in assembly to a separate 2048-byte emergency stack before
  invoking C diagnostics. The fatal record appends reason and measured handler
  SP; emitted output confirms actual SP is within the emergency buffer and its
  guard is intact. Raw fault cause/PC/value/source-SP are preserved. No guessed
  return, display callback, allocation, lock or scheduling occurs on this path.

Primary CSR/ISA/chip revision references: [hardware evidence](../hardware/EVIDENCE.md).
Software buffer addresses come from project/linker symbols, not fabricated MMIO.
Trap initialization establishes boot bounds/mscratch before interrupts/start;
faults before that initialization are outside this protection contract.

## Host evidence

Root build/CTest **13/13 PASS**. Integer/FP stack tests plus existing integer/FP
kernel, atomicity and idle fixtures under ASan/UBSan **6/6 PASS**. New headers
compile under strict C++98. Stack tests cover all 16 byte alignments, unchanged
bytes outside the supplied buffer, guard integrity, bad current/destination SP,
overflow, live overlap, adjacent non-overlap, reserved idle/port buffers and
no publication of a corrupt incoming context.

Isolated project-header mutants: disabling guard validation fails the stack
fixture at line 37; disabling alignment validation fails at line 27. An initial
misalignment mutation survived because the original test also exceeded the top
bound; the regression was strengthened to use an in-range misaligned frame,
then both mutants were rejected. This is test-quality evidence, not a claim
that host tests replace RV32 trap execution.

## Hardware fault matrix

Connected ESP32-P4 rev v1.3; GCC esp-14.2.0_20241119, esptool4.12. All cases
require exact cause 11 and PC from the matching ELF, expected reason, emergency=1,
expected source-guard state, no FAULT_RESUMED and exact injected SP where known.
The source-SP comparison includes zero, not a fabricated saved-frame address.

| Profile | Expected reason | Source guard | Actual result |
|---|---|---|---|
| underflow: SP has 16 bytes too little frame room |1| intact |PASS|
| guard: current task prefix overwritten |2| corrupt |PASS|
| incoming: other READY task prefix overwritten |3| intact source |PASS|
| irqguard: IRQ prefix overwritten |5| intact source |PASS|
| misaligned: deliberately odd SP |1| intact |PASS|
| zero: SP=0, including arithmetic-underflow check |1| intact |PASS|
| underflow-fp32: larger 272-byte frame |1| intact |PASS|

The deliberate corruptions are confined to this test firmware's owned RAM.
All cases also verify reserved IRQ-stack and live-stack overlap rejection before
injecting faults. No out-of-range write is deliberately executed: invalid SP is
installed immediately before ECALL, and the protected entry must catch it first.
The intact guard in underflow cases is evidence that no context was written over
that prefix. This is not a proof that arbitrary task code cannot corrupt memory.

Logs are `native-probe-logs/stack-<profile>.log`; exact symbol PCs and injected
SP values are preserved in the recovery manifest. Reason 4 (nested trap) has a
fail-stop code path but is **not hardware-tested**; no double-fault claim.

## Positive regressions and reproduction

Rebuilt lifecycle/mixed FP, integer-only, idle all-sleeping/all-exited and empty
boot profiles, plus ordinary illegal16/32 and compressed-breakpoint tests.
The diagnostic IRQ callback observes its actual C SP inside the 4096-byte IRQ
buffer and checks its guard, independently of the assembly implementation.
No scheduling/tick substitution is used. Final measured results:

| Positive profile | Actual target result |
|---|---|
| Mixed FP/lifecycle | 250 real IRQs/ticks, 125 creates/returns/nesting checks, 4 occupied slots; 54,688 ecalls, 54,813 switches; register errors=0 |
| Integer-only | 250 real IRQs/ticks, 0 ecalls, 251 switches; register errors=0 |
| Idle sleeping/exited | 150 IRQs, 149 idle ticks, 37 entries, 20 runs per app, 2 returns, 60 sleeping/50 exited checks; errors=0 |
| Empty boot | 150 IRQs/idle ticks, 1 idle entry, no app/return/ecall; errors=0 |
| illegal16 / illegal32 / breakpoint16 | Raw causes 0x30000002 / 0x30000002 / 0x30000003, exact matching ELF PC 0x4ff011b2, no resume or stack-fault marker |

The lifecycle/integer profiles each observe **250 IRQ-stack checks, zero errors**;
both idle profiles observe **150 checks, zero errors**. Every runner reports PASS
and resets to the preserved flash application. Final serial verification confirms
the shell responds, SciCalc evaluates `2+3*4=14`, and desktop is left active.
Logs include `stack-desktop-restored.log` and the positive profile names above.
All 14 matching ELF/RAM-bin/map sets, CMake caches, build/run scripts, expected
fault symbols/SPs and SHA256 manifest are preserved outside this checkout at
`../maia-rtos-recovery/native-stack-protection-20261111/`. Caches record the exact
profile flags; scripts assume those configured positive build directories.

Every native image passes the project/libgcc-only runtime symbol/archive gate.
Existing linker RWX and stripped empty flash-segment warnings remain.

```sh
cmake -S . -B /tmp/maia-native-stack-underflow -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_STACK=underflow
cmake --build /tmp/maia-native-stack-underflow --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-stack-underflow/maia_sched_probe_ram.elf
```

Use the matching ELF `maia_probe_fault_instruction` symbol for --expect-fatal-pc,
--expect-fatal-cause 11, --expect-stack-reason and --expect-stack-guard; optional
--expect-stack-sp validates the exact intentionally invalid SP. The runner checks
arguments/image before serial access and resets back to preserved flash in finally.
Three invalid CLI combinations (stack without fatal expectation, guard without
reason, source SP beyond RV32) are also rejected before serial access.
Other profiles: guard, incoming, irqguard, misaligned, zero; FP32 can be enabled.
Exact build/run orchestration is preserved with recovery artifacts.

## Limits and next step

Single-core internal-SRAM scope only. No SMP, native desktop or driver-equivalence
claim. Guards detect overwrites at checks; they do **not** prevent arbitrary
out-of-buffer task writes. Valid owned accessible buffers remain a caller
precondition. No PMP/MMU isolation, NMI/metadata corruption recovery or measured
worst-case stack depth/interrupt latency. Fixed IRQ/emergency buffers and guards
must be reassessed when real driver handlers are added. The raw normal-console
API is not used by stack fatal output; USB loss still cannot prevent RAM halt.

Files: new private stack layout/helpers and RISC-V stack storage; trap assembly,
scheduler, port dispatch/panic and public minimum-size comments; diagnostic/CMake,
host tests, evidence/status/handoff. Next in the owner's order is incremental
native driver-service integration, after a fresh dependency/IRQ-stack audit;
keep SMP separate and preserve the working desktop until feature equivalence.
