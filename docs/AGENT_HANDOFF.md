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

## Stack protection — 2026-11-11

Read `audits/native-stack-protection.md` and updated context contract. Task/idle
guards, pre-save SP checks via mscratch, dedicated 4 KiB IRQ + 2 KiB emergency
stacks, and incoming-frame validation are implemented independently. Native
minimums now 304 integer/ 448 RV32F; guards/alignment consume caller-buffer space.
Seven RAM fault injections pass exact cause/PC/SP/reason and emergency-stack
checks, including SP=0 and in-range-too-small SP. Host 13/13, sanitizer 6/6 and
C++98 headers pass. No general memory isolation, SMP or double-fault guarantee.
Boot trap init must precede protected startup; mscratch is reserved to this port.
No parser/foreign runtime or flash writes. Next authorized order: native driver
services incrementally, keeping complete working desktop on preserved flash.

Final regressions PASS: lifecycle/FP and integer 250 real IRQs each; both idle
profiles 150 each; actual C IRQ-stack checks all pass. Three ordinary fatal
profiles preserve raw cause/exact ELF PC. Preserved flash shell, SciCalc
`2+3*4=14` and desktop verified after reset. Logs and exact recovery artifacts:
`../maia-rtos-recovery/native-stack-protection-20261111/` (SHA256SUMS).

## Future POSIX intent — 2026-11-11

Owner requested [a documented future minimal POSIX layer](plans/POSIX_MINIMAL.md),
with minimal changes to the native system available when that stage starts.
Re-audit then; use optional adapters over public native APIs, preserve ABI and
existing apps, isolate necessary native primitives in small tested commits.
DESIGN only, no POSIX implementation or PSE51 conformance claim. This request
records future scope and does not replace the next native-driver migration task.

## Driver communication prerequisite: native queues — 2026-11-11

Read [native driver queue audit/evidence](audits/native-driver-queues.md).
Queue send/recv/count now mask local IRQs around the complete copy/index update,
preserving nesting; init requires quiescence and rejects overflow/items >256B.
Existing struct layout/signatures unchanged. No SMP, blocking wait or wakeup.
Mutex/semaphore implementations are NOT covered by this queue fix. Do not
transplant the temporary checkout's delay-loop waits as atomic blocking waits.
Host 14/14, sanitizer queue/kernel fixtures 3/3 and C++98 header checks pass.
Actual Tab5 RAM burst workload: 253 IRQs, 688 packets, zero errors; raw logs also
retain an initial test-floor failure, not a corruption reproduction. Diagnostic
ISR producer replaces only its test display callback, not a production driver.
Desktop/shell/SciCalc 14 restored after reset; final artifacts/checksums in
`../maia-rtos-recovery/native-driver-queues-20261111/`. Legacy flash still uses
FreeRTOS. Next: separate native serial I/O ownership from keyboard/display and
integrate queue-backed input service; keep hardware drivers and blocking waits
as separate verified increments. Future POSIX remains DESIGN only.

## Managed native serial input — 2026-11-11

Read [serial input audit/evidence](audits/native-serial-input.md). Raw USB/UART
RX is now in soc_serial_rx.c, independent of keyboard/display. Optional service
binds one READY task in boot (IRQs off), reserves port until reset, and moves
at most 16 bytes/poll into a static 64-byte native queue. Owner cannot exit or
reuse its slot; teardown is not implemented. Wrong-task/masked poll is rejected;
legacy console returns -1 without I/O after claim. Service callers use read API;
this does not automatically migrate shell or provide blocking waits/RX IRQ wakeup.
Host 15/15, sanitizer service/queue 2/2, headers and four mutants pass. Real USB
0..255 test PASS with full queue, 317 IRQs, auxiliary progress and no errors;
initial auxiliary-progress FAIL is preserved separately. Service-off queue
regression passes. USB tested, physical UART fallback not revalidated, no SMP.
Recovery artifacts: `../maia-rtos-recovery/native-serial-input-20261111/`.
Preserved flash shell/SciCalc/desktop verified after reset; complete firmware
still uses FreeRTOS. Next: separate/serialize serial TX before multiple worker
writers; other drivers/blocking primitives remain separate. No parser changes.

## Managed native serial output — 2026-11-11

Read [serial output audit/evidence](audits/native-serial-output.md). Optional TX
service binds a boot-created owner until reset, queues up to 8 records (1..64B)
plus one active record, and makes at most 16 byte attempts/poll. Multiple writers
use whole-record write; FULL does not accept a prefix, BUSY preserves offset.
TX is raw to one selected USB/UART sink, no display. Legacy console writes are
suppressed after claim; convert callers to the status-returning service API.
Fatal diagnostics bypass this queue deliberately. TX/RX reservations independent.
No task/heap creation, teardown, blocking wait, SMP or host-delivery ACK.
Host 16/16, sanitizer 3/3, C++98 headers and five mutants pass; actual USB 80 records/
3840 bytes, 260 IRQs, FULL=344, BUSY=239, errors=0. RX regression passes 256 bytes/305 IRQs.
Desktop/shell/SciCalc restored after RAM reset. Exact artifacts/checksums:
`../maia-rtos-recovery/native-serial-output-20261111/`. First TX pass also retained;
final adds real BUSY observation. Legacy complete firmware still uses FreeRTOS.
Next: atomic native worker waits/wakeups, auditing semaphore/mutex contracts
separately and avoiding temporary-tree delay-loop waits. No grammar changes;
future POSIX remains DESIGN only.

## Native notification waits — 2026-11-11

Read [task notification contract/evidence](audits/native-task-notifications.md).
Public notify(id) is single-core ISR-safe/coalescing; wait(timeout) is thread-only
with IRQs enabled. Check/consume/block occurs in labeled ECALL after context save.
First timeout/post wins; late notification is pending for next wait. Completion
is stored in TCB and installed in a0 once only after saved-frame validation.
FOREVER=UINT32_MAX is untimed; timeout0 returns EMPTY. Notify marks READY, no
implicit yield; slots reusable, caller must keep valid task identity. No SMP,
counted semaphore, mutex/PI, cancellation or general driver IRQ routing.
Host 18/18, sanitizer 4/4, C++98 headers and five mutants pass. Native 40 rounds/
282 waits/80 timeouts/750 real IRQs/errors=0; blocked run_ticks stay stable, idle
runs, invalid IRQ waits generate no ECALL. FP/lifecycle and fatal-PC regressions
pass. Preserved flash shell/SciCalc/desktop verified after reset. Exact artifacts:
`../maia-rtos-recovery/native-task-notifications-20261111/`.
Next: make TX worker wait on empty and notify after enqueue, preserving predicate
ordering; FIFO BUSY still needs finite retry or a real IRQ wake source. Display
configuration remains untouched and requires its own hardware audit. POSIX stays
DESIGN; full desktop firmware is still FreeRTOS-dependent. No grammar changes.

## Notification-driven TX worker — 2026-11-11

Read [TX worker contract/evidence](audits/native-tx-worker-notifications.md).
write now notifies the dedicated owner after successful whole-record publication,
under local exclusion; FULL/INVALID do not signal. New serial_output_wait is
owner-thread/IRQs-on only; queued OR active pending bytes prevent blocking.
Empty -> FOREVER notification wait, with sticky event covering pre-ECALL gap.
Always recheck predicate (stale/coalesced hints). Owner reserves/coordinates
its mailbox; lifetime until reset remains required. BUSY requires finite retry
until a real peripheral wake source exists, never indefinite wait with pending TX.
Native event profile is opt-in, no hidden task creation or default firmware switch.
Host 18/18, sanitizer 3/3, header and five mutants pass. Target 80 payload records,
302 IRQs, stable polls/run_ticks over head20/tail21, and actual IRQ enqueue wakes
owner; 240 finite retry timeouts, errors=0. Old polling TX regression passes.
Desktop/shell/SciCalc restored. Artifacts/checksums:
`../maia-rtos-recovery/native-tx-worker-notifications-20261111/`.
Next: audit and preserve display initialization/power/panel/DMA/cache facts before
porting that driver. Config remains untouched; USB results are not display proof.
No parser changes; POSIX remains DESIGN and full desktop is still FreeRTOS-based.

## Display baseline audit — 2026-11-11

Read [display baseline audit](audits/display-baseline-audit.md) and its pinned
manifest. Documentation/evidence only: no native driver or firmware change.
Preserved flash boot selects ST7121 via touch firmware1; later ST7123 log is
generic. Active config is 70 MHz DPI/965 Mbps lane, not native 80/960. All30
ST7121 table entries match, but extra delays differ. PSRAM startup/ownership,
cache/DMA, safe reset, error propagation and memory reservations remain open.
Native Cache_WriteBack_All address/signature/0x30 match pinned ROM references;
policy/safety still need validation. Configured legacy L2 cache line is128B.
Retained panel drivers are required; discarded LVGL must not be grouped with them.
Recovery: `../maia-rtos-recovery/display-baseline-audit-20261111/`. Known baseline
artifacts rehashed/copied; current build/bin is a different experimental image.
Boot captured and maia.py shell/SciCalc14/desktop reverified. Native display
NOT RUN; complete desktop remains FreeRTOS-dependent, POSIX remains DESIGN.
Next: bounded system-I2C identification API/probe, read-only touch firmware
transaction, explicit timeout/missing-device tests; no LDO/reset/DMA yet.

## Native boot-only touch identification — 2026-11-11

Read [native touch transport/evidence](audits/native-touch-identify.md). New
independent GPIO register-read engine has explicit NACK/TIMEOUT/BUSY, full high
hold after stretching, bounded waits and output commit after successful STOP.
Boot-exclusive core0/IRQs-off SoC backend uses SYSTIMER, not presumed CPU MHz;
caller must ensure pre-scheduler pin/timer ownership. No runtime locking/teardown.
Tab5 status0/4 permits read-only firmware ID; other states return BUSY, unknown
firmware is preserved without panel fallback. No production driver is replaced.
Host19/19, sanitizer fixture, C++98 headers, six negative mutants and invalid
profile rejection PASS. Target warm-reset RAM probe reads status4/firmware1
16 times plus recovery read, actual0x56 NACK, sampled-SCL injection TIMEOUT,
errors0. Initial too-strict Normal-only gate failures are retained. Physical
stuck-wire/frozen-timer/cold-panel tests NOT RUN. No power/reset/DMA/flash/SD writes.
Legacy shell/SciCalc14/desktop verified after final reset. Artifacts/manifest:
`../maia-rtos-recovery/native-touch-identify-20261111/`. User's local Tab5 PDF
and Sitronix protocol now pin the inspected pages; see EVIDENCE. Full firmware
still uses FreeRTOS; POSIX DESIGN. Next: checked bounded expander-register access
and safe bit-preserving reset/power contract, separately from memory/display DMA.

## Native expander configuration port — 2026-11-11

Owner permits open-source driver adaptation/porting (M5Stack, ESP-IDF, owner
code and portable drivers found in FreeRTOS); zero underlying FreeRTOS remains
mandatory. AGENTS.md supersedes its prior blanket copying prohibition, requiring
license/attribution and native dependencies. Read [expander port](audits/native-expander-port.md).
M5Unified configuration/bit-update model ported to C over bounded native I2C;
MIT notice/source revision retained. New read8 repeated START/write8 transport,
restricted config register set, mask-preserving update, no-op and full readback
VERIFY error. Exclusive pre-scheduler ownership only; no retries/rollback/locks.
Host20/20, two sanitizer fixtures, C++98 header and five mutants PASS. Actual
RAM probe: banks43/44 accept identical OUT05 writes, five config bytes unchanged
over16 rounds each; touch16 reads firmware1, actual absent-write NACK, errors0.
Changed reset/power bits are NOT tested. Original flash shell/SciCalc14/desktop
verified after reset. Artifacts: `../maia-rtos-recovery/native-expander-port-20261111/`.
No flash/SD write; full desktop still depends on FreeRTOS. Next: checked board
reset sequence with safe LCD input/pull-up release and explicit partial-failure
contract, separately from LDO/DSI/PSRAM/DMA. POSIX remains DESIGN.

## Native LCD/touch reset — 2026-11-11

Read [native panel reset port](audits/native-panel-reset.md). ST712x-only checked
bank43 P4/P5 sequence ports functional BSP100/100ms waits, cross-checked with
M5Stack input/pull-up LCD release. Apache-2.0 Espressif notice/license retained;
all dependencies are native, no FreeRTOS/SDK runtime. Snapshot, masked updates,
full verification and report last completed stage; error stops, no compensating
writes/retry/rollback. Caller exclusively owns boot bus/pins, stable supplies,
GPIO23 input and inactive display/DMA. Bounded pauses use SYSTIMER, not CPU MHz.
Host21/21, sanitizer fixture, C++98, six mutants, profile-negative PASS. Target
RAM reset reaches stage9, preserved1, elapsed244643us; touch status0/firmware1
16 reads plus recovery, errors0. Both-bank snapshots preserve unrelated config.
No voltage/waveform/pixels/cold-power/SMP/runtime claim. Preserved flash shell,
SciCalc14/desktop verified after reset; no flash/SD write. Exact artifacts/logs:
`../maia-rtos-recovery/native-panel-reset-20261111/`. Legacy full desktop remains
FreeRTOS-based and POSIX DESIGN. Next: audit/port MIPI PHY LDO acquisition with
calibration/ownership/error contracts before DSI clocks or framebuffer/DMA.

## Native VO3 calibration planning — 2026-11-12

Read [VO3 calibration audit/port](audits/native-ldo3-calibration.md). Pure native
C port of SDK voltage conversion and read-only ROM-shadow/PMU snapshot; Apache
notice retained. No regulator acquisition/enable/write yet. Scope chip1.3, block
schema0/minor>=1; disabled/unknown revision flags and uncorrectable block1 reject.
SDK sign-magnitude/zero defaults,128-candidate first-minimum order, int64 math,
modeled-voltage/default diagnostics. Snapshot boot/core0/IRQs-off only.
Host22/22, sanitizer/C++98/six mutants PASS; exact SDK differential matches all
1048576 trim triples. Actual block0.3 raw55/33/33 ->1030/-4/989 ->dref13/mul3,
modeled2504637uV/error4637uV; sixteen snapshots unchanged, errors0. Current PMU
is already enabled/SW-selected with those codes: inherited state, NOT native
power-init proof. Old fixed code matches this chip but is not a generic port.
Artifacts: `../maia-rtos-recovery/native-ldo3-calibration-20261112/`. Builds retain
20261111 start-date suffix. Next: boot-exclusive acquire/lifetime/error policy,
adopt matching active rail, reject conflicts, masked write/verify for off rail.
No voltage/settling/cold-enable/display parity proof; full firmware still uses
FreeRTOS, POSIX remains DESIGN.

Preserved flash shell/SciCalc14/desktop verified after reset with paced keys;
initial unpaced3s capture failed to collect the result and is retained in the
VO3 report. No firmware fix or flash/SD write was made for this retest.

## Native VO3 acquisition — 2026-11-12

Read [VO3 acquisition/evidence](audits/native-ldo3-acquire.md). Boot-only fixed
reservation until reset, stable calibration snapshots, active matching adoption
without writes; active mismatch BUSY. Off path guards DSI/CSI clocks, preserves
other fields, verifies analog/source/enable in order. Failure after programming
starts is sticky FAILED until reset; no rollback/retry/release. SDK runtime and
locks are not imported; Apache notice retained. Host23/23, sanitizer/C++98/six
mutants PASS. Actual target: stage5/adopted1/changed0/preserved1/duplicate_busy1.
Important finding: MIPI clock enables persist through ROM load_ram (initial
words0x0C0D5678/0x2B3). First overly broad guard rejected adoption; final policy
permits write-free adoption only, preserving all guards for power writes. No
regulator power write occurred on target. Off programming is HOST-TESTED; cycle
candidate compiled, physical run NOT RUN due active-clock precondition. Never
assume RAM entry quiesces display/DMA or just clear clocks to pass the guard.
Paced shell/SciCalc14/desktop verified after reset; no flash/SD writes. Artifacts:
`../maia-rtos-recovery/native-ldo3-acquire-20261112/`. Next: joint audit/port of
ordered DSI/CSI/DMA quiesce before hardware off->on test and DSI bring-up. Full
legacy desktop still uses FreeRTOS; native cold display remains pending.

The earlier panel-reset probe did not establish consumer/DMA quiescence. Its
report is now qualified and its diagnostic rejects active MIPI clocks before
reset; guarded probe compile passes, physical rerun remains pending quiesce.

## Read-only inherited MIPI/DW-GDMA state — 2026-11-12

Read [MIPI state audit](audits/native-mipi-state.md). New isolated RAM diagnostic observes only
clock-enabled blocks, rechecks full clock words, never follows descriptors or
writes MIPI/DMA/power/reset. Host24/24, ASan/UBSan, three mutation negatives and
two mixed-profile rejection checks PASS; strict native runtime gate PASS.
Actual Tab5 eight samples PASS: clocksE6DFB7FF/0C0D5678/2B3 stable; validity0D
(DSI, CSI bridge, DW-GDMA; CSI host skipped). DSI bridge/DPI enable0, host power0/
PHY reset0; DMA global/channel/abort0 and all four channel FIFO/address/status0.
Active clocks are not evidence of enabled DMA in this observed state. This is
not an atomic snapshot, AXI outstanding-traffic proof or ordered shutdown test;
CSI host fields are unmeasured, not verified idle. Desktop/shell/SciCalc14 restored
via maia.py after reset; no flash/SD write. Full artifacts/reference hashes:
`../maia-rtos-recovery/native-mipi-state-20261112/`. Next: bounded native shutdown
contract covering reset/AXI completion, shared channels and PHY state before
clock gating/power tests. Full desktop still uses FreeRTOS; physical VO3 cycle,
guarded reset rerun, native cold display and POSIX implementation remain pending.

## Bounded native VDMA stop — 2026-11-12

Read [native VDMA stop](audits/native-vdma-stop.md). Original native C whole-controller boot
reservation: require both existing bus clocks/exclusive all-channel ownership,
reject abort/reset/inconsistent states, issue channel disable WE strobes only,
clear global EN preserving other bits, bounded total poll budget and checked
completion. TRM v0.4 CFG0 p349 readback stays1 until activity terminates; checking
channel bits alone is insufficient. No IRQ-clear/abort/reset/clock/power/memory
writes. FIFO residue reported, no lossless-transfer guarantee. Failure in stop
phase is sticky until reset; success makes duplicate BUSY. Pending IRQ flags
retained; handle before native IRQ enable. No ISR/task/SMP/runtime ownership.
Host25/25, ASan/UBSan/C++98, four mutants and two invalid CMake profiles PASS.
Target empty-controller arm/stop PASS: CFG0->1 verified, native stop->0 in one
poll, stage3/armed1/adopted0/changed1/requested0/preserved1/duplicate_busy1, no DMA
transfer started. Active-channel stop remains HOST-TESTED; PHY/MIPI clocks and
VO3/panel-reset guards untouched. Paced shell/SciCalc14/desktop verified after
reset, no flash/SD write. Artifacts: `../maia-rtos-recovery/native-vdma-stop-20261112/`.
Next: reserved DMA-visible SRAM/cache ownership and actual active-transfer stop
fixture, then separate PHY/bridge stop before clock gating/power tests. Full
legacy desktop still uses FreeRTOS; POSIX DESIGN and native cold display pending.

## Native SRAM transfer and active VDMA stop — 2026-11-12

Read [active DMA fixture/evidence](audits/native-vdma-transfer.md). Opt-in diagnostic reserves66048B
NOLOAD outside BSS/stack/ROM; checks actual L1D line64, invokes pinned silicon ROM
range writeback/invalidate, then uses only CPU NC alias8FF/physical DMA4FF.
No SDK headers/runtime/FreeRTOS; explicit ROM entries documented. Channel0 M2M
single-block8-bit/master1: actual full32768-byte copy, then active-channel stop
(initialCFG1/CHEN1/requested1) stage3/two polls/finalglobal0. Correct partial1152B,
zero tail31616B, source/guards intact and eight later full scans stable; sampled
MIPI/clocks/other channels unchanged. This supersedes HOST-TESTED-only active
stop for this SRAM workload, not peripheral/display/PSRAM/SMP/DMA IRQ behavior.
Host26/26, sanitizer, four compiled mutants and two invalid profiles PASS. Initial
RO/reserved INTSIGNAL readback refusal fixed with documented writable mask;
first post-fix run lost early banner, correctly rejected by capture validator.
Final complete capture PASS; no relaxed checks. Prior empty-controller binary
remains byte-identical after CMake changes. Paced shell/SciCalc14/desktop restored
and no flash/SD write. Artifacts: `../maia-rtos-recovery/native-vdma-transfer-20261112/`.
Next: DSI/CSI PHY/bridge shutdown under explicit failure/ownership policy before
clock gating/power tests. VO3 and panel-reset guards untouched; full desktop still
uses FreeRTOS, native cold display pending, POSIX DESIGN.

## Native boot MIPI reset parking — 2026-11-12

Read [MIPI reset parking](audits/native-mipi-park.md). New native boot-exclusive API requires
VDMA global/channel/abort/reset/FIFO0, already-shutdown DSI/CSI hosts/analog PHYs,
DPI0 and DSI FIFO0. Three MIPI/both DMA bus clocks +PHY config clocks must already
be enabled. Four masked/verified bridge/digital reset steps, preserved other bits,
fresh guards/no-op skipping and sticky failure until reset. No analog/clock/power/
DMA/IRQ-status writes. Live states reject; generic live-stream stop is not ported.
TRM38.7.2 p1974 also requires shared ISP reset for CSI stop; do not skip ownership
or treat this boot-only API as complete live teardown/power-cycle permission.
Host27/27, sanitizer/C++98/four mutants/two invalid profiles PASS. Target RAM
probe enables only CSI host bus SYS11 (0C0D5678->0C0D5E78) for inspection, arms
both bridge EN and two digital reset-release bits while hosts/analog remain low.
Native stage4/attemptmaskF/four control words0/preserved1/duplicate_busy1 PASS;
clocks (prepared baseline) and sampled DMA/channel/other fields unchanged.
DSI PHY clock-lane/forcePLL clearing host-tested only. Runner host-ready byte
precedes setup/capture. Shell/SciCalc14/desktop restored after reset, no flash/SD
write. Artifacts: `../maia-rtos-recovery/native-mipi-park-20261112/`.
Next: shared ISP/boot consumer prerequisites, checked reset-state clock gating;
then VO3/reset hardware gates and native display/PSRAM. Full desktop still uses
FreeRTOS; live-stream shutdown and POSIX implementation remain pending.
