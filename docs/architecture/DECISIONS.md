# Architecture Decision Records (initial)

- ADR-001 ACCEPTED: Standalone bare-metal RTOS; no FreeRTOS or Arduino.
- ADR-002 ACCEPTED: C11 kernel, C-compatible public ABI and C++ applications.
- ADR-003 ACCEPTED: Separate CPU architecture, SoC HAL, BSP and drivers.
- ADR-004 ACCEPTED: Fixed-priority preemptive SMP design; exact algorithm subject to tests.
- ADR-005 ACCEPTED: MaiaWASMVM and MaiaOS are optional downstream projects.
- ADR-006 ACCEPTED: No conventional MMU process isolation claims.
- ADR-007 ACCEPTED: 16+16 MiB allocation is MaiaOS policy, not MaiaRTOS design.
- ADR-008 PROPOSED: Initially use independent image/flash tooling with minimal custom boot stage; verify ROM constraints first.
- ADR-009 ACCEPTED: Host tests are not hardware validation.
- ADR-010 ACCEPTED: Use M5Stack vendor BSP (M5GFX / M5Unified) and LVGL as driver/graphics backend for board support; keep kernel core freestanding C11.
