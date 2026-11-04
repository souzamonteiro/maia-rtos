# MaiaRTOS

**Independent, bare-metal, SMP-capable real-time operating system** for embedded C and C++ applications. First target: **Espressif ESP32-P4 / M5Stack Tab5**. Future ports are intended.

> **Status: architectural bootstrap, NOT a working RTOS.** No scheduler, board bring-up, bootloader, driver, or hardware support is claimed as implemented.

## Non-negotiable principles

- Zero FreeRTOS source, headers, APIs, binary linkage or runtime dependencies.
- Zero Arduino framework or IDE dependencies.
- Kernel is independent of MaiaWASMVM and MaiaOS; those are optional downstream consumers.
- C11 core, RISC-V assembly where necessary; C ABI usable from C++.
- Layered architecture; no ESP32-P4 register accesses in generic kernel code.
- Toolchain and flashing utilities from Espressif may be used; ESP-IDF is **not** a runtime dependency.
- Document uncertainty; do not fabricate register addresses, peripheral behavior, or successful tests.

## Architecture

```text
Applications (C/C++) / optional MaiaWASMVM -> MaiaOS
                  |
Optional services: VFS / graphics / networking / USB
                  |
Device framework and drivers
                  |
MaiaRTOS kernel: scheduler / SMP / IPC / memory / timers
                  |
SoC HAL (ESP32-P4) + Architecture port (RISC-V)
                  |
BSP (M5Stack Tab5) / startup / image / linker
                  |
Hardware
```

The diagram expresses responsibilities, **not** a strict linear call hierarchy. The architecture port is CPU-specific; the SoC layer is chip-specific; BSP is board-specific.

## Start here

1. Read [Architecture](docs/architecture/OVERVIEW.md), [Requirements](docs/requirements/REQUIREMENTS.md), and [Hardware evidence](docs/hardware/ESP32P4_TAB5.md).
2. Agents: read [AGENTS.md](AGENTS.md) and your specific [agent playbook](docs/agents/ANTIGRAVITY.md) or [Codex handoff](docs/agents/CODEX.md).
3. Follow [implementation plan](docs/plans/ROADMAP.md); begin with M0 only.
4. Never claim hardware functionality without captured evidence.

## Build status

`cmake -S . -B build && cmake --build build && ctest --test-dir build --output-on-failure` builds a **host-side architecture-contract test only**. It does **not** produce ESP32-P4 firmware.

## Repository structure

`arch/` CPU port; `soc/` SoC support; `boards/` board definitions; `kernel/` OS-independent mechanisms; `drivers/` peripherals; `services/` optional facilities; `include/maia/` public API; `tests/` validation; `examples/` applications; `docs/` specs and agent plans.

## License

MIT for original project files. Third-party code must not be imported without explicit review, attribution and license compatibility. Vendor technical documentation is reference material, not a license to copy code.
