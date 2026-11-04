# Mandatory instructions for ALL coding agents

Read README.md, docs/architecture/OVERVIEW.md, docs/requirements/REQUIREMENTS.md, docs/plans/ROADMAP.md, and docs/hardware/ESP32P4_TAB5.md BEFORE changes.

## Hard prohibitions

1. Do not introduce FreeRTOS, Arduino, or ESP-IDF runtime code, headers, link dependencies, compatibility shims, or copied implementations. Using standalone vendor compiler/binutils/flashing tools is permitted.
2. Do not put MaiaOS, MaiaWASMVM, or WebAssembly assumptions in the MaiaRTOS kernel.
3. Do not invent ESP32-P4 MMIO addresses, reset/boot protocols, interrupt routing, second-core startup, PSRAM timings, or board pinouts.
4. Do not silently replace unknowns with plausible values. Create a verification task in docs/hardware/OPEN_QUESTIONS.md instead.
5. Do not mark a feature implemented based on compilation alone, mocks, or host tests.
6. Do not perform broad refactors or implement multiple milestones in one change.
7. Do not assert hard real-time guarantees without measured worst-case bounds and stated conditions.

## Agent work protocol

- State task, files to modify, prerequisites, and acceptance tests before coding.
- Make one small, reviewable change at a time. Prefer a minimal interface over speculative functionality.
- Preserve a clean dependency direction: app -> services/drivers/kernel -> architecture/SoC/BSP as applicable.
- Every hardware fact must cite a primary source URL, document revision and section/page in docs/hardware/EVIDENCE.md.
- Add tests and a negative test when applicable; record command, environment and actual result.
- For hardware-only tests, mark NOT RUN and provide exact procedure. Never fake UART logs.
- End each task with: implemented; not implemented; files changed; test results; hardware evidence; known risks; next small task.
- Keep docs/plans/STATUS.md synchronized with evidence.

## C/C++ policy

- Kernel C11, assembly where required; public headers must compile in both C and C++.
- No implicit heap allocation in ISR; no blocking in ISR; no unchecked arithmetic in memory managers.
- Define memory ordering for cross-core atomics, critical sections, cache/DMA ownership, and lock ordering.
- Avoid unbounded loops with interrupts disabled; bound ISR work.
- All public functions have error contracts and thread/ISR safety annotations.

## Definition of Done

An item is DONE only when implementation, automated tests, documented limitations, and target-hardware evidence (if applicable) exist. Otherwise label DESIGN, STUB, HOST-TESTED, or HARDWARE-TESTED.
