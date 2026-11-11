# Working firmware checkpoint — 2026-11-10

Read this file on agent/model/context change. Follow root `AGENTS.md`, the
architecture and R-002. This checkpoint records evidence; it does not authorize
replacing the complete working system with a smaller demonstration.

Owner clarification (2026-11-10): changes to the MaiaScript/shell parser or
lexer must start from the owning EBNF and regenerate with the official
`tools/maiacc` compiler compiler. No handwritten syntax patches or direct edits
to generated output. Grammar locations and the mandatory rule are in AGENTS.md.
Inspection found a pre-existing `patch_parser_try.py` step in the MaiaScript
`make parser` recipe. Before the next regeneration, move that behavior into
the authoritative grammar/generator, preserving try/ternary behavior with tests.
This clarification changed no parser, grammar or generated artifact.

## Owner's objective and current trees

Continue MaiaRTOS toward an independent runtime without FreeRTOS underneath,
preserving the complete desktop, shell, MaiaScript compiler/apps and device
control interface. The owner requested a careful inventory before changes.

- Working tree: `/Volumes/External_SSD/Documentos/Projects/maia-rtos`.
- Experimental/reference tree: `../maia-rtos-temp`. Preserve its local changes.
- Known working source commit: `bf044d37e43c3b8db2011d6cbf81c9df2d2e1649`.
- Audit documents and an opt-in one-core driver experiment are tracked through
  checkpoints; inspect Git status afresh. The active device was restored to the
  preserved `bf044d3` application after the experiment.
- Recovery artifacts: `../maia-rtos-recovery/bf044d3-20261110/manifest.json`.
  These are copied build artifacts, not a full device flash/SD backup.

The working firmware uses ESP-IDF/FreeRTOS and is not yet compliant with R-002.
Its project-native kernel is a separate build, not a second running scheduler.
Do not claim that the current functional desktop is already an independent RTOS.

## Read next

1. [Current status](plans/STATUS.md).
2. [Detailed runtime audit and staged plan](audits/freertos-dependencies-2026-11-10.md).
3. [File/line inventory](audits/freertos-source-inventory-2026-11-10.tsv).
4. [Previous panel correction and hardware test](audits/maiascript-panel-text.md).
5. [First migration experiment, failures and restoration](audits/tab5-singlecore-stage-1-2026-11-10.md).
6. [CST ownership fix and successful device reproducer](audits/maiascript-cst-ownership-2026-11-10.md).
7. [Host contract build repair](audits/shell-host-contracts.md).
8. Temporary checkout's `docs/AGENT_HANDOFF.md`, recovery audits and raw logs
   only as historical evidence of the experimental native console.

The temporary handoff describes the old failing flash image and the native
RAM-console experiment. The active device was subsequently restored and works.
Do not repeat that old image diagnosis as a description of the current device.
Do not copy the temporary CMake files that disable the working Tab5 build.
Do not transplant its platform API substitutions: that application was excluded
from its native build and is not a validated desktop port.

## What works and what was tested

The owner tested SciCalc/Alarm and confirmed the corrected panel appearance.
The preceding firmware test verified desktop shortcuts, `2+3*4=14`, powers,
domain error and timer firing/cancellation over serial. The shared rasterizer
now respects explicit newlines; no automatic-width wrapping API was added.

This audit reran MaiaScript tests (32/32), raster tests (PASS), and fresh builds
of current/temporary native and host targets. Current full host build FAILS on
shell test/header mismatch; current native build FAILS on `riscv_asm.hpp` include.
Temporary host tests pass 8/8 and its native console builds, but that is not
equivalence with the complete working system. Its prior `timer_irq=0`, display
stall and parked secondary core remain unresolved. No hardware was accessed in
this audit. Raw logs and the evidence collector are in the recovery `audit/` dir.

The subsequent one-core experiment compiled and passed SciCalc/Alarm and shell
background-job tests. It is NOT accepted as the default: on-device compilation
hits a mutex fault, and the same source also asserts in the preserved original
image. Initial compiler priority 5 also starved idle in the one-core profile;
the experimental priority was lowered, with watchdog retained. Do not equate
passing host compiler tests with successful compilation on the device.
The device was restored, test-created files removed, and SciCalc `2+3*4=14` plus
return to desktop verified over serial. Bootloader/partitions were not changed.
Experiment artifacts/logs live in the recovery `experiments/singlecore-20261110/`
directory. `MAIA_TAB5_SINGLECORE_DRIVERS` remains OFF by default.

Subsequent CST fix: target libstdc++ uses mutex-bearing shared control blocks.
Private temporary CST nodes now have one owner (`unique_ptr` storage) and borrowed
edges, removing per-CST-node mutexes. Public AST/Parser ABI, grammar/generated
parser and codegen remain unchanged. Normal host tests now include AST lifetime
after CST release, large/repeated parses and syntax-error recovery; ASan/UBSan
also pass. The same on-device 32-addition reproducer now compiles to a valid
680-byte module and returns 32. SciCalc/Alarm device regressions pass on the
one-core fixed image. Both profiles build; two-core execution and large on-device
compilation have not been retested. AST shared_ptr/runtime locks still remain.
The device was again restored to the original preserved image after this test;
the CST fix is in source/experimental artifacts, not the currently installed app.
Evidence: recovery `experiments/cst-ownership-20261110/` and the report above.

Latest host-contract step supersedes the old host-build failure: root build
and 5/5 CTest now pass, with real tool dependencies linked for full-shell
regressions. The diagnostic console adapter/fixtures were aligned to the
unchanged native v1 struct/values, with a separate validation helper; 2/2
ASan/UBSan and C++98 header checks pass. Default Tab5 build passes, no flash.
No lexer/parser/EBNF or shell command implementation changes. Full freestanding
shell/native-driver integration is still incomplete; do not equate host success
with a bootable native desktop.

## Next small implementation and constraints

The recorded minimal compiler reproducer is fixed; do not repeat it as an
outstanding failure of the patched source. Full allocation-failure behavior and
large AST lock pressure still require separate validation. Continue toward a
separately verified native scheduling target: restore its build/test contracts,
then validate timer-driven preemption (not synthetic ticks/cooperative yields)
and FP context before integrating driver services. Reconcile shell tests with
the existing ABI and dependency reporting as supporting work. Preserve grammar, semantic
behavior, module ABI and the working desktop build. Do not change a public
contract merely to satisfy old test names. Keep experimental native targets
separate while hardware/driver equivalence is missing. The owner authorized
incremental implementation and device testing; report concrete results and risks.
The owner now requires commit + push after each completed unit, with descriptive
changes/tests/limitations and separate rollback points. No force-push or history
rewriting is authorized. Use Git history to identify the latest checkpoint.

Native migration must establish real timer preemption, floating-point context
and ABI support, memory/cache/DMA and synchronization before it can host the
same apps safely. Treat SMP separately. Never substitute a console-only result
for the requested desktop. A smaller source set passing a dependency guard is
not proof that omitted features have been ported.

## Existing device tools

Build/flash toolchain is `/Users/roberto/esp-idf`; device interaction is
`tools/maia-cli/maia.py`, port `/dev/cu.usbmodem1301`. The owner authorized build,
flash and interactive testing earlier. The CLI is for control/transfer; firmware
flashing uses ESP-IDF/esptool. This audit deliberately did not flash anything.

The device normally boots the desktop. Its shell is an integrated window:
Escape twice closes an app/window, `t` opens terminal, shell `exit` returns to
desktop, `g` opens SciCalc and `l` opens Alarm. Sending shell commands directly
to the desktop can trigger shortcuts. Prefer the existing
`tests/hardware/test_maia_apps.py` sequence; it sounds an alarm and verifies
serial results, not physical pixels/audio. Avoid simultaneous serial clients.
Use `PYTHONDONTWRITEBYTECODE=1` when importing the CLI: a pycache is tracked.
The desktop terminal accepts only 62 useful characters per input line. Pace
serial bursts to avoid its 63-character pending-key ring. The worker diagnostic
checks both constraints; the usual CLI push method expects an `msh>` prompt,
which the integrated desktop terminal does not emit.

Before any future device update, preserve the known working artifacts, verify
the proposed image's scope and retain a recovery path. Do not erase flash or SD
as a routine build/test step. Update this checkpoint with actual results.

## Latest native probe checkpoint

Read [native scheduler probe](audits/native-scheduler-probe.md) and captured logs.
Explicit target `maia_sched_probe_ram` now builds independently of the incomplete
full native shell. Host tests pass 6/6. Verified SYSTIMER base is **0x500E2000**,
not historical 0x500C6000 (I2S0); target-0 source is **53**, map offset **0xd4**.
After correcting both, real hardware delivers **one** IRQ and switches tasks,
then stalls with an enabled pending alarm. Zero ecalls/synchronous exceptions.
Do not claim working preemption. Next: measure CLIC CSR state across first
IRQ/context return and fix repeated delivery separately. Diagnostic ran in RAM
only and reset to preserved flash; no native image was flashed. No grammar edits.

## CLIC return correction — 2026-11-11 (supersedes one-IRQ stall above)

See [CLIC return report](audits/native-clic-return.md). Read-only study of the
owner's full FreeRTOS tree and silicon references led to an independently
authored one-bit fix: fresh task mcause must have INT=1 (`0xb8000000`) so mret
releases the active interrupt level when first selected inside an ISR. Previous
INT=0 frame left MIL=0x3f. Four corrected RAM runs: 250 IRQs/ticks, 251 switches,
both tasks progressing, zero ecalls/exceptions, MIL=0. No synthetic events.
Host 6/6 PASS and ASan/UBSan kernel PASS; negative old-frame test exits 52.
This is limited single-core integer-workload evidence, not full ABI/FP/SMP or
native desktop completion. Next: live integer-register and FP context tests.
No FreeRTOS code copied or linked into native target; no grammar changes.
After the final RAM reset, the preserved flash desktop was verified through
maia.py: shell echo, SciCalc 2+3*4=14, zero module exit and desktop left active.
Native test artifacts/manifest: `../maia-rtos-recovery/native-clic-return-20261111/`.

## Live integer context — 2026-11-11

The RAM probe now seeds x1/x4/x5–x31 with distinct task/register values and checks
them after interruptible busy windows, plus sp/gp snapshots and stack canary.
It reproduced **tp/x4 corruption after three IRQs** on `3f3bd80`. Saving/restoring
tp at byte124 (existing reserved final frame word) fixes it; frame remains 128 bytes.
Corrected workload reaches 250 real IRQs/251 switches, reg_errors=0, both tasks
progress, no ecalls/exceptions. Host 6/6/sanitized kernel pass. No TLS allocation,
FP/SMP or native desktop claim. See `audits/native-integer-context.md`.
Next small task: RV32F/FCSR context and consistent initialization/ABI tests.

## RV32F context — 2026-11-11 (supersedes FP-pending notes above)

See `audits/native-fp-context.md` and `architecture/RISCV_CONTEXT.md`. The private
shared frame layout selects 128-byte integer or 272-byte RV32F context. Independent
assembly saves all 32 FP registers and FCSR for enabled FS; fresh FP state is zero.
Two final FP RAM runs and integer regression each pass 250 IRQs/251 switches with
all sentinels intact, no ecalls/exceptions. Missing FP context and a temporary
FCSR-zero mutant both fail the hardware test. Host 7/7 and sanitized frame tests
2/2 pass. Diagnostic FP option `MAIA_NATIVE_PROBE_FP32=ON` uses RV32IMAFC/ILP32F.
Toolchain ASM flags now match C; use fresh build directories, old caches can
retain absent ASM flags and are rejected by layout FLEN guard.
No foreign code/runtime, parser edits or flash writes. No native desktop/SMP
completion. Next: mixed FS-Off/on and priority/yield/delay tests, plus fatal-trap
policy before driver integration. Preserve original flash/working desktop.
Final reset recovery was verified via maia.py: shell, SciCalc 2+3*4=14, module
exit0 and desktop active. Artifacts/checksums and FCSR mutant are preserved in
`../maia-rtos-recovery/native-fp-context-20261111/`.

## Mixed scheduling — 2026-11-11

See `audits/native-mixed-scheduling.md`. Mixed profile exposed INT=0 saved-ecall
frames restored from a timer ISR leaving MIL=0x3f. Saved task return frames now
normalize INT=1 while raw cause is passed separately for dispatch/statistics.
The mixed workload passes250 realIRQs,74947 ecalls,124 two-tick sleeps,125
high-priority yield checks and374 FS-Off checks, with register integrity/MIL0.
Host7/7, sanitized frame2/2 pass. Single-core SRAM scope; no flash/parser/imports.
Next authorized step is bounded fatal diagnostics/halt instead of unknown PC+4.

## Native fatal traps — 2026-11-11

Unknown PC+4 skipping is removed. Only enabled M-mode task ECALL resumes, with
alignment/overflow/lifecycle checks. Other exceptions/breakpoints/unknown IRQs
enter bounded USB fatal diagnostics + RAM record + WFI halt. No display/heap/lock
or scheduler in fatal sink. No alternate fault stack/double-fault recovery.
See `audits/native-fatal-traps.md`; do not infer native desktop/SMP completion.
Host 9/9, ASan/UBSan C fixtures 3/3 and C++98 headers pass. Hardware injected
illegal16/32 and C.EBREAK compare fatal PC against actual ELF symbol. Runner
requires both --expect-fatal-cause and --expect-fatal-pc; early armed serial
banner is optional, but if present must match. No diagnostic is flashed.
Next small task: scheduler operation atomicity/IRQ-state contract and idle path.
Final hardware: illegal16/illegal32/C.EBREAK fatal checks PASS at exact ELF PC;
mixed scheduler regression PASS (250 realIRQs,18142 ecalls,124 sleeps, MIL0).
After reset, shell/SciCalc 2+3*4=14/desktop verified. Artifacts+symbol metadata:
`../maia-rtos-recovery/native-fatal-traps-20261111/`. No flash/SD writes.

## Scheduler atomicity — 2026-11-11

See `audits/native-scheduler-atomicity.md`. Private local IRQ guards preserve
nesting; task/tick/stats snapshots coherent single-core. READY last; delay/exit
commit after frame save via dedicated ECALL labels. Raw yields remain supported.
Count is occupied slots including EXITED; reuse does not increase it. Init after
start BUSY; masked create after start PERM. No SMP claim. Host10/10, sanitized
fixtures3/3 and lifecycle hardware250 IRQs/125 transients pass.
Next in authorized order: fix no-ready behavior/internal idle, then stack bounds.
No native diagnostic flashed; functional desktop remains preserved on flash.

## Internal idle — 2026-11-11

See `audits/native-idle.md`. No-ready no longer revives blocked/exited apps.
Private idle owns 2048-byte stack, no public slot, MIE on/FS off/WFI; saved sp
is retained. current_id=-1 while idle; stats getter is coherent single-core.
Host 11/11 and ASan/UBSan fixtures 4/4 pass, revival mutant rejected. Hardware
all-sleeping/all-exited and empty boot each pass 150 real IRQs with no early wakes
or revived apps. Instrumented architecture idle wait hook observes state before
real WFI; no scheduling/tick substitution or power/timing claim.
Next in authorized order: stack protection/bounds before driver integration/SMP.
Final lifecycle regression after idle PASS (250 IRQs/125 transient tasks).
After reset, original flash shell/SciCalc 2+3*4=14/desktop verified. Exact recovery
artifacts/checksums: `../maia-rtos-recovery/native-idle-20261111/`.
