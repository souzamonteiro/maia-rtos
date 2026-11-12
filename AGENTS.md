# Mandatory instructions for ALL coding agents

Read README.md, docs/architecture/OVERVIEW.md, docs/requirements/REQUIREMENTS.md, docs/plans/ROADMAP.md, and docs/hardware/ESP32P4_TAB5.md BEFORE changes.

On agent/model/context handoff, also read docs/AGENT_HANDOFF.md and its current
audit references. The neighboring maia-rtos-temp checkout is experimental
reference material, not the working firmware baseline.

## Hard prohibitions

1. Do not introduce FreeRTOS as an underlying runtime, scheduler, API dependency or compatibility shim, or depend on Arduino/ESP-IDF runtime libraries/headers. Using standalone vendor compiler/binutils/flashing tools is permitted.
   Owner clarification (2026-11-11): open-source M5Stack, ESP-IDF and owner code may be adapted/ported to native MaiaRTOS APIs, preserving applicable licenses and attribution. Portable open-source drivers, including ones found in the FreeRTOS tree, may be ported too; studying FreeRTOS is permitted. This supersedes the blanket prohibition on copied implementations, not the zero-FreeRTOS runtime requirement. Remove foreign OS dependencies from a port and validate it incrementally; record source revision, license and remaining dependencies.
2. Do not put MaiaOS, MaiaWASMVM, or WebAssembly assumptions in the MaiaRTOS kernel.
3. Do not invent ESP32-P4 MMIO addresses, reset/boot protocols, interrupt routing, second-core startup, PSRAM timings, or board pinouts.
4. Do not silently replace unknowns with plausible values. Create a verification task in docs/hardware/OPEN_QUESTIONS.md instead.
5. Do not mark a feature implemented based on compilation alone, mocks, or host tests.
6. Do not perform broad refactors or implement multiple milestones in one change.
7. Do not assert hard real-time guarantees without measured worst-case bounds and stated conditions.

## Parser/lexer ownership — explicit owner instruction (2026-11-10)

- MaiaCC at `/Volumes/External_SSD/Documentos/Projects/maia-rtos/tools/maiacc`
  is the official compiler compiler for the Maia project.
- For changes to the MaiaScript or shell parser/lexer, change the owning EBNF
  and regenerate the parser/lexer with that MaiaCC toolchain. Do not maintain
  syntax in handwritten replacement parsers or manually edit generated output.
- MaiaScript grammar: `maiascript/docs/MaiaScript_Native_v0_1_MaiaCC.ebnf`.
  Shell grammars: `shell/grammar/maia-shell*.ebnf`.
- Preserve reproducible generation and run positive/negative grammar and
  semantic regressions. Fix generator defects at their owning source and
  regenerate consumers; do not insert behavior through output-patching scripts.
- Existing `maiascript/tests/tools/patch_parser_try.py`, invoked by the parser
  Makefile recipe, is legacy generation debt, not an approved exception. Its
  behavior must move into the authoritative EBNF/MaiaCC generation path before
  the next parser regeneration. Do not silently remove it and change semantics.

## Agent work protocol

- State task, files to modify, prerequisites, and acceptance tests before coding.
- Make one small, reviewable change at a time. Prefer a minimal interface over speculative functionality.
- Preserve a clean dependency direction: app -> services/drivers/kernel -> architecture/SoC/BSP as applicable.
- Every hardware fact must cite a primary source URL, document revision and section/page in docs/hardware/EVIDENCE.md.
- Add tests and a negative test when applicable; record command, environment and actual result.
- For hardware-only tests, mark NOT RUN and provide exact procedure. Never fake UART logs.
- End each task with: implemented; not implemented; files changed; test results; hardware evidence; known risks; next small task.
- Keep docs/plans/STATUS.md synchronized with evidence.
- Owner instruction (2026-11-10): after each completed, verified unit, create a
  descriptive commit and push it. State the change, validation and material
  limitations so rollback points are easy to identify. Keep later work in
  separate commits; do not force-push or rewrite published history.

## C/C++ policy

- Kernel C11, assembly where required; public headers must compile in both C and C++.
- No implicit heap allocation in ISR; no blocking in ISR; no unchecked arithmetic in memory managers.
- Define memory ordering for cross-core atomics, critical sections, cache/DMA ownership, and lock ordering.
- Avoid unbounded loops with interrupts disabled; bound ISR work.
- All public functions have error contracts and thread/ISR safety annotations.

## Definition of Done

An item is DONE only when implementation, automated tests, documented limitations, and target-hardware evidence (if applicable) exist. Otherwise label DESIGN, STUB, HOST-TESTED, or HARDWARE-TESTED.
