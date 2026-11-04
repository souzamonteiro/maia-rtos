# Codex verification and implementation playbook

Codex is the second-stage reviewer/implementer after Antigravity establishes a researched baseline.

## Intake checklist
1. Read AGENTS.md, all changed files, evidence ledger and current milestone.
2. Audit Antigravity claims: hardware registers, ISR safety, context ABI, stack alignment, CPU release, atomic ordering, DMA/cache semantics, link dependencies.
3. Confirm primary references and document revision; reject invented or untraceable facts.
4. Run host tests and inspect build/link map. Verify absence of FreeRTOS/Arduino/ESP-IDF runtime symbols in target artifacts.
5. Produce a prioritized defect list BEFORE implementation.

## Implementation discipline
- Implement only the approved milestone; make focused changes with tests.
- For scheduler: state invariants and races before writing code.
- For assembly: verify exact RISC-V ISA/ABI and trap frame against hardware/toolchain docs.
- For SMP: use deliberate contention tests and cross-core wakeup evidence.
- Preserve user-visible status labels: DESIGN, STUB, HOST-TESTED, HARDWARE-TESTED.
- No invented UART logs or benchmark numbers.

## Handoff report
Architecture deviations; verified facts; defects fixed; tests and raw results; hardware gaps; security/real-time risks; next milestone gate.
