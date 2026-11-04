# Requirements and acceptance criteria

## Functional
- R-001: Independently buildable native C11 kernel, C++ compatible API.
- R-002: No FreeRTOS/Arduino/ESP-IDF runtime dependency in final target image.
- R-003: Architecture/SoC/BSP separated and testable.
- R-004: ESP32-P4 bare-metal startup, trap handler and UART diagnostic output.
- R-005: Preemptive task scheduling with documented context-save ABI.
- R-006: SMP execution on both cores, cross-core wakeups and atomic synchronization.
- R-007: Mutex, semaphore, queue and timer primitives with defined ISR safety.
- R-008: Internal SRAM and external PSRAM managed as distinct capabilities.
- R-009: Tab5 peripheral support built incrementally after core scheduler.
- R-010: Optional external MaiaWASMVM integration without modifying kernel internals.

## Nonfunctional
- N-001: No false hardware claims; preserve UART captures and test configuration.
- N-002: Deterministic failure behavior for allocation exhaustion.
- N-003: Public APIs documented for error cases and concurrency.
- N-004: Reproducible build with pinned toolchain version and checksums.
- N-005: Static analysis, warnings-as-errors for project code, host unit tests.
- N-006: No unreviewed third-party source imports.

## Release gates
- Alpha: UART boot + timer + preemptive tasks on target.
- SMP alpha: two cores, concurrent workloads, lock stress and cross-core wakeups verified.
- Beta: memory pools + core IPC + recovery/diagnostics.
- v1.0: regression suite, board support, reproducible release, API and limitation documentation.
