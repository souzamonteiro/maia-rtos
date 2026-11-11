# Mixed FS states, yield, delay and priority — 2026-11-11

Test-only profile `MAIA_NATIVE_PROBE_MIXED=ON` requires FP32. Two equal-priority
FP tasks seed all integer/FP registers and FCSR, then execute **ecall with the
sentinels still live** before checking them. A third, higher-priority C task
clears FS and repeatedly verifies: FS stays Off; yielding while ready does not
let either lower task run; sleeping two ticks never wakes early; both lower
tasks progress while it sleeps. All tests still require250 real timer IRQs,
250 ticks, register integrity and active MIL=0. ecalls are counted separately.
No synthetic ticks, vendor code/runtime, parser edits or flash writes.

## Reproduced return defect

On `1e0da88`, the mixed test fails: it reaches250 IRQs but observes
`mcause=0x0800000b` and `mintstatus=0x3f000000` while normal task code runs.
Saved ecall frames still had INT=0. Selecting one from a timer ISR can leave
its active interrupt level in place, just as the earlier fresh-frame defect.

The independently authored fix normalizes **saved task return state** with the
same INT=1/MPP=M/MPIE=1 flags as fresh frames, preserving MPIL. The raw cause is
copied into the C handler argument **before** normalization, so dispatch and
statistics still distinguish real timer IRQs from ecalls. No counter is faked.
The frame flag constant is shared by C/assembly. The final snapshot disables
interrupts before reading cause/status/counters to keep the result consistent.

## Actual validation

ESP32-P4 rev v1.3, GCC esp-14.2.0_20241119, RV32IMAFC/ILP32F, esptool4.12.
Explicit RAM target; reset in finally, no writes to flash/SD.

- Before correction: `mixed-before.log`, FAIL at nonzero active MIL.
- Corrected run1: 250 IRQ/ticks, 74947 ecalls (all synchronous traps are ecalls),
  75072 switches, 374 FS-Off checks, 124 completed two-tick sleeps, 125 priority
  yield checks, reg_errors0, MIL0. Both FP tasks progress and float ABI checks pass.
- Corrected repeat: `mixed-fixed-2.log`; exact counters captured, same acceptance.
- Host suite7/7 PASS; integer/FP frame ASan/UBSan2/2 PASS; native runtime gate PASS.

Captures: `native-probe-logs/mixed-*.log`. Build with a fresh directory:

```sh
cmake -S . -B /tmp/maia-native-mixed -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_FP32=ON -DMAIA_NATIVE_PROBE_MIXED=ON
cmake --build /tmp/maia-native-mixed --target maia_sched_probe_ram -j4
```

Use the established elf2image/RAM runner commands. This mixed workload includes
cooperative switches deliberately; it is not evidence that ecalls are preemption.
Only the separately recorded IRQ/tick counters demonstrate hardware timer delivery.

Production files: `arch/riscv/trap_context.S`, `kernel/ksched.c`, private context
header. CMake/tests add the opt-in workload; default diagnostic stays without
yields. Primary chip references and measured interpretation: hardware/EVIDENCE.md.

## Limits and next task

This validates the exercised single-core SRAM FS-Off/on, equal-priority yield,
higher-priority readiness and two-tick suspension interleavings. It does not
establish all race cases, SMP, hard timing bounds, nested interrupts, general
TLS, drivers or native desktop equivalence. Existing forced MPIE/MPP policy
still needs API/critical-section review. Stack overflow protection is absent.

Next small task: replace unknown-fault/breakpoint skipping with a bounded fatal
record/output and halt, preserving original PC/cause; test illegal16/32 and
breakpoint traps plus permitted yields without replacing the working flash app.

Final artifacts/checksums: `../maia-rtos-recovery/native-mixed-20261111/`.
