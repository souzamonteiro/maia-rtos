# Architecture specification v0.1

## Product boundary
MaiaRTOS is a standalone embedded RTOS for native C/C++ workloads. MaiaWASMVM and MaiaOS are external products, never kernel dependencies. The first board is M5Stack Tab5 (ESP32-P4). No MMU-based process isolation is assumed.

## Layers and contracts

| Layer | Owns | Must not know |
|---|---|---|
| Applications | User entry points, business logic | SoC MMIO |
| Optional services | VFS, FS, graphics, network | Private scheduler internals |
| Device framework | Device registration, IRQ/DMA interfaces | Specific app policy |
| Kernel | TCB, scheduling, IPC, timers, memory | Board pinouts, WASM |
| SoC HAL | ESP32-P4 controllers, clocks, cache/DMA constraints | Application logic |
| CPU architecture | RISC-V traps, context, atomics, barriers | ESP32-P4 board wiring |
| BSP/boot | Image, startup, linker, pin assignments, memory init | Application features |

This is a dependency map, not a strict call stack: kernel calls arch and SoC clock/interrupt interfaces; device drivers call HAL and kernel services.

## Kernel strategy
- Fixed-priority preemptive scheduler, configurable CPU count, per-core ready queues, optional affinity and bounded balancing.
- Define task lifecycle: NEW, READY, RUNNING, BLOCKED, SLEEPING, TERMINATED; transitions under explicit synchronization.
- Preemption from a timer/interrupt path with architecture-specific context save/restore; define interrupt nesting and deferred reschedule semantics.
- SMP correctness: cross-core wakeups via verified IPI mechanism; atomics and memory fences; lock ordering and per-core data.
- Priorities: document numeric ordering and scheduling fairness. Implement priority inheritance only after base mutex correctness.
- No promise of zero-latency IRQ or deadline scheduling in v0.1.

## Memory
- Region descriptors with attributes (internal/external, DMA-capable, cacheable, executable where supported).
- Static task allocation supported; optional dynamic allocation; bounds and alignment checked.
- Memory allocation may fail deterministically; no implicit fallback to unsuitable memory.
- ESP32-P4 lacks a conventional MMU for general virtual-memory process isolation. Native tasks share an address space; protect against accidental corruption with guard strategies where hardware permits, but do not claim process isolation.
- 16 MiB MaiaOS / 16 MiB apps is **MaiaOS policy**, never a hardwired MaiaRTOS partition.

## Device/driver model
- Driver registration, initialization stages, capability queries, IRQ registration, DMA buffer ownership, asynchronous completion and teardown.
- A single graphics owner task is recommended for LVGL, but graphics remains an optional service, not the kernel.
- Board pin mapping stays in BSP; reusable peripheral controllers stay in SoC drivers.

## Boot
ROM boot is immutable. Initial boot chain must be verified from official TRM/boot documentation. Prefer a minimal independently authored stage and standalone vendor image/flashing tools; no FreeRTOS or ESP-IDF runtime. A fully custom image/bootloader is a separate milestone after basic hardware proof.

## C API design
- Stable C ABI usable by C++; opaque handles for tasks, queues and mutexes.
- Explicit ownership, lifetime, context restrictions (ISR vs thread), timeout units and error codes.
- Use public API sketches as contracts; no unimplemented functions should be exposed as working.

## Portability
`arch/riscv` contains generic RISC-V; `soc/espressif/esp32p4` contains chip details; `boards/m5stack/tab5` contains wiring. POSIX tests may validate portable algorithms but cannot validate hardware SMP, interrupt latency or context switching.

## Safety and failure modes
- Fatal trap reporting, watchdog strategy, bounded diagnostics and recovery policy.
- Define power loss, SD removal, allocation failure, IRQ storm and cross-core deadlock handling.
- All timing and performance assertions require measured data and environment.

## Non-goals for v0.1
Full POSIX, virtual-memory processes, dynamic linking, complete USB/network stacks, filesystem, graphics desktop, MaiaWASMVM integration, guaranteed hard real-time certification.
