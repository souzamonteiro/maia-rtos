# Native scheduler atomicity — 2026-11-11

Scoped single-core IRQ exclusion, atomic task publication/snapshots, and
transactional delay/exit traps. No FreeRTOS source/runtime, parser changes or
flash writes. Idle/no-ready fallback remains a separate next step in this
checkpoint; do not mistake it for fixed behavior.

## Defects and independently authored changes

- Task creation previously published READY before constructing sp/frame. The
  bounded construction/publication now occurs with local IRQs masked; READY is
  last. Reusing EXITED slots no longer inflates registered-slot count.
- Sleep/exit state writes previously happened in task code before ECALL, allowing
  timer scheduling during an incomplete operation. Dedicated assembly ECALL
  labels carry delay in a0 and identify exit. Dispatch changes state only after
  saving context, with IRQs disabled, before selecting the next task. Existing
  raw-yield ECALL/sentinel convention is preserved; no parser/codegen edit.
- Local CSR save/restore keeps only MIE and restores disabled state on nesting;
  compiler barriers bracket shared memory accesses. Task metadata,64-bit ticks
  and trap stats have coherent single-core snapshots. It is not an SMP lock.
- Deadlines use unsigned half-range comparison, including64-bit wrap for uint32
  delays. Initialization after scheduler start returns BUSY. Creation after
  start with IRQs disabled returns PERM. Host hooks are explicitly non-concurrent
  algorithm models and cannot execute native return contexts.

Primary CSR/silicon reference: hardware/EVIDENCE.md. Frame size/register ABI is
unchanged. No heap or blocking is added to the masked sections. Loops are bounded
by16 slots,16 name bytes or frame word count; no measured latency guarantee.
Only the generic tick snapshot/fallback is covered, not driver-clock read locking.

## Tests

Host suite **10/10 PASS**; atomicity and integer/FP kernel fixtures under
ASan/UBSan **3/3 PASS**. The host IRQ model injects scheduler ticks only at
restore boundaries and checks published frames, nesting and preserved IRQ state.
It tests low-word carry and uint64 wrap deadlines; these rollover tests are host
algorithm evidence, not billions of measured hardware ticks. Runtime-only test
seeding is excluded from native builds.

Negative isolated source mutants are rejected: unprotected tick getter exits1
at line72 of the atomic fixture; unconditional count growth on EXITED reuse
exits1 at line59. They are project-owned temporary test mutations, not production
flags. No pre-fix on-device timing-race reproduction is claimed.

Hardware: connected ESP32-P4 rev v1.3, GCC esp-14.2.0_20241119, RV32IMAFC/ILP32F,
esptool4.12. Optional lifecycle stress extends the existing mixed profile:
high-priority task repeatedly creates a higher-priority transient, observes its
normal return/EXITED state, reuses its slot, checks IRQ nesting/snapshots and
masked-creation rejection, then sleeps. FP sentinel tasks keep exercising live
registers across ecalls and real IRQs. No synthetic ticks in native workload.

First run:250 real IRQ/ticks,125 creations/normal returns/nested checks,
registered slots remain4,124 sleeps,66696 ecalls,66821 switches,reg_errors0/MIL0.
Repeat is captured separately. Counters of distinct operations can differ by one
in a snapshot when a task is interrupted between two operations; no equality
claim is made for independent progress counters. Native runtime-symbol gate PASS.

Reproduce with a fresh build:

```sh
cmake -S . -B /tmp/maia-native-atomic -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_FP32=ON -DMAIA_NATIVE_PROBE_MIXED=ON -DMAIA_NATIVE_PROBE_LIFECYCLE=ON
cmake --build /tmp/maia-native-atomic --target maia_sched_probe_ram -j4
```

Use established elf2image/RAM runner/reset. Captures `native-probe-logs/atomic-*.log`.

## Scope and next step

Modified kernel scheduler, private IRQ/port-operation headers, RISC-V delay/exit
entry stubs, SoC trap dispatch, public contract comments, opt-in test/CMake and
evidence/status. Single-core SRAM only; no SMP, driver/cache/DMA migration or
native desktop equivalence. Protection of bounded operations does not prove all
possible interleavings. Stack bounds/emergency trap stack remain pending.

Next: remove the no-ready fallback which revives sleeping/exited tasks; provide
an internal idle context and verify all-sleeping, all-exited and wakeup behavior
before moving on to stack protection.

Repeated run has the same acceptance counts and PASS. Exact artifacts/checksums:
`../maia-rtos-recovery/native-atomic-20261111/`.
