# Implementation roadmap — gated milestones

**Do not skip gates.** Each milestone produces a short PR, tests, evidence and updated STATUS.md.

| Milestone | Scope | Gate |
|---|---|---|
| M0 | Toolchain, linker, image research, UART bare-metal | Actual UART banner on Tab5; no FreeRTOS linkage |
| M1 | Trap vector, timer, basic IRQ | Verified exception/interrupt logs |
| M2 | Single-core TCB, stacks, context switch, preemption | Repeated independent task preemption on hardware |
| M3 | Secondary core, IPI, per-core queues, atomics | Simultaneous SMP execution and contention stress |
| M4 | SRAM/PSRAM regions and allocator | Region, alignment, exhaustion and stress tests |
| M5 | Mutex, semaphore, queues, timers | Concurrent stress, timeout and wakeup tests |
| M6 | HAL and board drivers (SD, input, display) | Hardware-specific peripheral acceptance logs |
| M7 | Diagnostics, latency, robustness and release | Reproducible build, measured results, regression suite |
| M8 | Optional external MaiaWASMVM integration | WASM app on MaiaRTOS without kernel changes |

## Future optional POSIX subset — DESIGN

After the equivalent functional system runs without FreeRTOS, evaluate an
[optional minimal POSIX layer](POSIX_MINIMAL.md). Re-audit the APIs available at
that time; prefer adapters over existing public native services, with only small,
separately justified native extensions. This does not change the current driver
migration order or make full POSIX a v0.1 goal. No POSIX implementation is claimed.

## Milestone execution template
1. Research and record evidence.
2. Specify interface and invariants.
3. Implement minimum viable feature.
4. Add host tests where meaningful.
5. Run target tests; capture real logs.
6. Review failure cases and update status.
7. Request human approval before next milestone.

## Initial Antigravity scope
Only M0 research and build/toolchain scaffold. Do not implement scheduler, drivers or fake board support before hardware evidence.
