# Copy/paste prompts

## Antigravity — start
You are engineering MaiaRTOS, an independent bare-metal SMP RTOS in C11 for ESP32-P4 / M5Stack Tab5. Read AGENTS.md, README.md, docs/architecture/OVERVIEW.md, docs/requirements/REQUIREMENTS.md, docs/plans/ROADMAP.md, docs/hardware/ESP32P4_TAB5.md, docs/agents/ANTIGRAVITY.md. HARD RULES: no FreeRTOS, Arduino, ESP-IDF runtime, MaiaOS or MaiaWASMVM dependency; no invented register values or tests. Work only on M0-A: research official hardware and boot references, update evidence and unknowns, propose a minimal toolchain/startup plan. Provide exact sources/revisions and stop before writing unverified hardware code. Report facts vs assumptions and exact tests run.

## Codex — handoff
Review the MaiaRTOS repository and the most recent Antigravity changes. Read AGENTS.md and docs/agents/CODEX.md. Audit all claims against official ESP32-P4/Tab5 sources, inspect dependency and linker assumptions, verify C11/C++ API contracts, run available tests, and produce a prioritized defect list. Do not expand scope beyond the currently approved milestone. Only implement fixes backed by evidence. Never fabricate hardware test output.

## Next milestone request
Implement ONLY milestone <ID> subtask <NAME> according to docs/plans/ROADMAP.md. First list prerequisites, sources, touched files, acceptance tests, and risks. Do not start if hardware facts are missing; update OPEN_QUESTIONS.md and stop. After changes, report exact test commands/results and update STATUS.md.
