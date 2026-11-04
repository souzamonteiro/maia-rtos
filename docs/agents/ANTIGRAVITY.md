# Antigravity / Gemini execution playbook

## Purpose
Use Antigravity for low-cost, tightly bounded initial research and scaffolding. Gemini may generate convincing but incorrect low-level embedded code. Treat outputs as untrusted until verified.

## Mandatory behavior
- Read AGENTS.md and architecture/requirements/roadmap first.
- Work on **one milestone and one small subtask per iteration**.
- Never claim register addresses, second-core startup sequences, MMIO constants or hardware test results without exact official source and revision.
- Separate FACT (cited), DESIGN (proposed), ASSUMPTION (unverified), IMPLEMENTED (compiled/tested), and HARDWARE-VERIFIED (actual device evidence).
- Do not substitute ESP-IDF/FreeRTOS or Arduino convenience libraries when hardware details are hard.
- No huge code dumps or speculative driver trees. Prefer minimal, auditable changes.
- Stop and ask for documentation or hardware logs if blocked.

## First assignment — M0-A
1. Inspect repo and report current status accurately.
2. Find primary ESP32-P4 and Tab5 hardware documents; record exact revisions and citations.
3. Verify boot/image, flash tool, UART and debug route; mark unknowns explicitly.
4. Propose a minimal independent startup/linker/image workflow WITHOUT ESP-IDF runtime.
5. Do not implement unverified MMIO access.
6. Produce an evidence table, a file-by-file patch plan, and a minimal acceptance test.
7. Stop for review.

## Mandatory final report format
- Task and scope
- Facts with source+revision
- Assumptions/unresolved questions
- Files changed
- Exact commands and actual results
- Tests not run and why
- Dependency audit (FreeRTOS/Arduino/ESP-IDF runtime)
- Risks and next single task

## Quality guardrails
Do not report 'SMP supported' because two functions exist. Do not report 'bootable' because a binary links. Do not report 'isolated' because there is no MMU. Do not implement POSIX emulation or WASM in kernel.
