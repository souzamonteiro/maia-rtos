# Project status

Date: 2026-11-10

## Restored baseline and initial audit

- Functional baseline: `bf044d3`, restored Tab5 desktop, full shell and MaiaScript
  apps. Owner confirmed the SciCalc/Alarm visual fix. Firmware binary SHA-256
  remains `21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd`.
- **AUDITED / NONCOMPLIANT WITH R-002:** the working image contains FreeRTOS
  startup/scheduler and uses ESP-IDF drivers/runtime. Project native kernel
  sources are absent from this image. LVGL/vendor panels are compiled but their
  code is discarded from the final image. No competing native scheduler was found.
- **HOST-TESTED:** MaiaScript 32/32 and the panel raster test pass. Fresh full host
  build fails on shell contract mismatch; current native target fails on missing
  `riscv_asm.hpp`. These are baseline gaps, not fixed in this audit.
- Temporary checkout: fresh host tests 8/8 and native console build pass. Its
  prior hardware evidence is limited to cooperative console operation; timer
  interrupts, display and SMP remain unresolved. It is not the active firmware.
- No firmware/source/configuration changes, serial access or flash in this audit.
  Working artifacts were copied to the sibling recovery directory.

Evidence, source inventory, exact limitations and staged migration plan:
[runtime dependency audit](../audits/freertos-dependencies-2026-11-10.md).
Next small implementation: reconcile shell contract tests without changing the
current ABI, and establish dependency checks per build target.

## First incremental experiment — 2026-11-10

- Added opt-in `MAIA_TAB5_SINGLECORE_DRIVERS`, isolated config/build and guards
  protecting the default configuration. Both profiles build; host isolation and
  panel tests pass. The default configuration remains two-core and unchanged.
- **HARDWARE-TESTED, PARTIAL:** the one-core experiment passes desktop app,
  scientific calculation, timer/cancellation and background-job tests. ELF no
  longer defines secondary-core startup, cross-core tick or IPC-call symbols.
  It still contains the FreeRTOS scheduler and is not compliant with R-002.
- **FAILED / NOT PROMOTED:** initial heavy compiler work starved idle and
  triggered its watchdog. With experimental compiler priority lowered, a small
  program exposed a C++/pthread mutex fault. The same source also asserts in
  `xQueueSemaphoreTake` on the original preserved firmware. Root cause remains
  open; no claim that removing SMP caused or fixed the compiler fault.
- Restored the preserved `bf044d3` application, cleaned test files and verified
  SciCalc `2+3*4=14` and return to desktop. Bootloader/partitions were untouched.
- Next small task: isolate that compiler/ownership fault, then continue native
  scheduling/driver-service migration behind independent acceptance checks.

Exact source changes, version comparison, results, hashes and logs:
[first migration experiment](../audits/tab5-singlecore-stage-1-2026-11-10.md).

## Compiler CST ownership fix — 2026-11-10

- Removed shared control blocks/mutexes from private temporary CST nodes; one
  storage owner releases them after AST conversion. Public AST/Parser ABI,
  grammar/generated parser, codegen and module ABI remain unchanged.
- **HOST-TESTED:** 32/32 compiler regressions plus AST lifetime, large/repeated
  parse and negative syntax/recovery tests PASS. ASan/UBSan PASS. Both firmware
  profiles build; already-versioned host compiler binaries rebuilt.
- **HARDWARE-TESTED, LIMITED:** the formerly failing 32-addition source now
  compiles on the one-core Tab5 profile, validates and returns 32. Background
  jobs and SciCalc/Alarm regressions PASS. Seven CST shared-control-block ELF
  symbols before the fix, zero afterwards. Runtime/AST locks still remain.
- Original application restored after tests. This does not promote the
  one-core profile or certify large compilation, allocation failures, native
  preemption or SMP. Continue with a verified native scheduling target.

Evidence: [CST fix and exact scope](../audits/maiascript-cst-ownership-2026-11-10.md).

## Host contract build restored

- **HOST-TESTED:** root CMake build and all 5/5 CTest entries pass. ABI/kernel
  adapter tests also pass 2/2 with ASan/UBSan; C/C++98 validation-header check
  passes. The default Tab5 profile builds; no device flash in this step.
- Existing native v1 struct/values unchanged. Console adapter and fixtures use
  current names; a separate helper validates prefix/version/callbacks. The
  full C++ shell test links real host tools and retains all ten assertions.
- Parser/lexer/EBNF and command implementations unchanged. These tests do not
  certify real VFS/async I/O, native preemption or SMP. The full freestanding
  shell remains incomplete. Next: isolated native timer/scheduler diagnostic.

Details and exact commands: [host contract repair](../audits/shell-host-contracts.md).

## Historical milestone claims

The table below is retained as history. It does not certify the current working
image as independent, preemptive under the native kernel, or SMP-capable under
the native kernel. Current evidence above takes precedence.

| Area | Status | Evidence |
|---|---|---|
| Architecture documents | DESIGN | Repository documents |
| M0-A / N0 ESP32-P4 & Tab5 hardware/memory audit | HARDWARE-TESTED | `docs/audits/esp32p4-execution-memory.md`, `docs/hardware/EVIDENCE.md`, `docs/hardware/OPEN_QUESTIONS.md` |
| N1 MaiaShell boundary & reentrancy audit | DESIGN | `docs/audits/shell-boundary.md` (inspected against `maiaos/apps/shell`) |
| Host build/test scaffold (`maia_contract_test`, `esp32p4_regs_test`, `kernel_mvp_test`) | HOST-TESTED | CMake/CTest (`100% tests passed out of 4` in `build_host`, plus C++98 header checks) |
| N2 MaiaShellHost v1 contract & Interactive Native Shell (`maia_shell_mvp.c`) | HARDWARE-TESTED | Host CTest (`tests/host/shell_host_contract_test.c`, `tests/host/kernel_mvp_test.c`) + live interactive session on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M0 ESP32-P4 bare-metal boot (`start.S`, `soc_early_init.c`, `linker.ld`) | HARDWARE-TESTED | Booted directly in `L2MEM_LOW` (`0x4FF00000`) on M5Stack Tab5 (`ESP32-P4 rev v1.3`) with zero FreeRTOS/ESP-IDF/Arduino runtime (`docs/hardware/EVIDENCE.md`) |
| M0 Console driver (`soc_console.c`: USB-Serial/JTAG + UART0) | HARDWARE-TESTED | Bidirectional TX/RX verified on M5Stack Tab5 `/dev/cu.usbmodem1301` (`docs/hardware/EVIDENCE.md`) |
| M1 RISC-V Trap Vector & Timebase (`trap_context.S`, `soc_timer.c`) | HARDWARE-TESTED | 64-byte aligned CLIC `_maia_trap_entry`, `ebreak` exception trap (`mcause=0x30000003`), `ecall` trap (`mcause=0x3800000B`), CLIC `mcause` `MPP`/`MPIE`/`MPIL` save/restore, and microsecond hardware timebase verified on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M2 Multitasking Scheduler (`kernel/ksched.c`) | HARDWARE-TESTED | Priority + round-robin task scheduler (`idle`, `telemetry`, `maia_shell`), tick sleep/wakeup (`maia_task_delay_ticks`), and task introspection (`ps`, `ticks`, `uptime`) verified on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M4 Deterministic Region Memory Allocator (`kernel/kmem.c`) | HARDWARE-TESTED | 16-byte aligned region allocator with free-block coalescing (`mem`, `alloc 256`) verified on host CTest and on M5Stack Tab5 `L2MEM` (`0x4FF076A8`) (`docs/hardware/EVIDENCE.md`) |
| M5 Synchronization & IPC (`kernel/ksync.c`) | HARDWARE-TESTED | Mutex (`maia_mutex_t`), counting semaphore (`maia_sem_t`), and bounded message queue (`maia_queue_t`) verified across `telemetry` and `maia_shell` tasks and `qtest` on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M3 SMP (Dual-Core HP Core 0 + Core 1) | NOT IMPLEMENTED | Core 1 parked safely in `start.S`; bring-up tracked in `UQ-006` |
| PSRAM external heap region | NOT IMPLEMENTED | Internal 32 KiB `L2MEM` pool active; Octal PSRAM MSPI init tracked in `UQ-007` |
| Board display driver (720x1280 MIPI-DSI ST7121 with LEDC backlight) & physical keyboard (A164 @ 0x6D) | HARDWARE-TESTED | Clean-room MIPI-DSI 800Mbps D-PHY + ST7121 28-cmd vendor DCS init + LEDC 5000Hz 12-bit PWM channel 1 backlight + PI4IOE 0x43/0x44 push-pull reset sequencing + DW_GDMA continuous 10-block ring in L2MEM + A164 STM32 keyboard String mode driver verified on Tab5 (`docs/hardware/EVIDENCE.md`) |
| Permanent Standalone Flash Boot (0x10000) | HARDWARE-TESTED | ESP-IDF 2nd-stage bootloader dual-segment Flash MMU mapping (`0x40000000`/`0x40010000`) and clean-room `esp_app_desc_t` (`0xABCD5432`) at Segment 0 (`0x4FF00000`), booting standalone directly from SPI Flash without host computer (`docs/hardware/EVIDENCE.md`) |
| MaiaScript Native Compiler (RV32IMAF ILP32F) & MaiaASM | IMPLEMENTED / HARDWARE-TESTED | Zero-GC AOT/JIT native compiler with 30/30 regression tests passing (Sections 01-32: Basics, OO, UI, Canvas, LVGL, Wi-Fi, Storage, Audio, I2C), EBNF-driven via MaiaCC (`make parser`), zero FreeRTOS dependencies, and live GUI execution verified on Tab5 hardware (`t_gui.mobj`) |
| Board touch/audio/SD drivers | NOT IMPLEMENTED | MicroSD pinout verified (GPIO39-44); driver tracked in `UQ-008` (M6) |
| MaiaWASMVM integration | OUT OF SCOPE for core | Optional user-space workload per `README.md` |

Do not change statuses without attached evidence.

## Scoped update — 2026-11-10

MaiaScript SciCalc/Alarm panel newlines: HOST-TESTED pixel positions and
panel bounds; updated firmware built/flashed and app operation
HARDWARE-TESTED over serial. Physical display appearance still requires
visual confirmation. See [test results and scope](../audits/maiascript-panel-text.md).
This update concerns the user's restored working firmware baseline and
does not revalidate the historical kernel/milestone claims above.

## Native scheduler diagnostic — partial hardware result

- Added isolated RAM acceptance probe and native dependency gate; host 6/6 PASS.
- Corrected verified SYSTIMER base (old address was I2S0) and off-by-one IRQ route.
- Hardware progresses from zero to one real timer interrupt/task switch, but
  repeated delivery fails with an enabled pending alarm. **M1/M2 NOT ACCEPTED.**
- No flash writes; no parser or desktop changes. Native FP/SMP remain unverified.
- Evidence/logs/reproduction and next CLIC diagnosis:
  [native probe report](../audits/native-scheduler-probe.md).

## CLIC fresh-context return — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** corrected fresh task INT bit; four RAM runs
  each deliver 250 real timer IRQs/ticks and 251 task switches, both busy tasks
  progressing, zero ecalls/exceptions, active MIL restored to zero.
- Supersedes prior one-IRQ stall for the corrected frame. Host 6/6 PASS, kernel
  ASan/UBSan PASS; old frame rejected by negative host/hardware regressions.
- No foreign source/runtime imported; no flash writes or parser changes.
- Full M2 context ABI/FP and SMP remain unverified; native desktop not ported.
- [Research, exact changes, captures and next test](../audits/native-clic-return.md).

## Live integer context — 2026-11-11

- Assembly register sentinel test reproduced missing tp preservation after three
  real IRQs. Save/restore tp in existing frame word 124 fixes that failure.
- **HARDWARE-TESTED, LIMITED:** corrected test reaches 250 IRQs/251 switches
  with all checked integer registers intact, both tasks progressing and no
  ecalls/exceptions. Host 6/6 and sanitized kernel PASS.
- No new runtime dependency, frame-size/public ABI or parser changes. FP pending.
- [Exact workload, failure and scope](../audits/native-integer-context.md).

## RV32F context — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** independently authored 272-byte RV32F frame
  preserves f0–f31/FCSR and zero-initializes fresh state. Final two FP runs and
  integer regression pass 250 real IRQs/251 switches, reg_errors0, no ecalls/faults.
- Negative old-context test fails on f0; isolated FCSR-zero mutant fails on
  rounding/flags. Real ILP32F C calls return 5.0 correctly. Host 7/7 and sanitized
  integer/FP frame fixtures 2/2 PASS; native runtime guard PASS.
- C and ASM ISA/ABI flags aligned; fresh builds required to clear old caches.
- No flash/parser/vendor-runtime changes; mixed FS-Off/on, priority/yield/delay,
  fault policy and full native desktop integration remain pending.
- [Exact evidence, layout, risks and commands](../audits/native-fp-context.md).

## Mixed scheduling — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** FS-Off high-priority task yields ready and
  sleeps two ticks while two FP tasks retain live sentinels across ecalls/IRQs.
- Reproduced saved-ecall INT=0 priority leak; normalize task return flags while
  passing raw cause separately. Corrected workload passes250 realIRQs plus
  explicit ecalls,124 sleeps,125 priority checks and374 FS-Off checks.
- Host7/7 and sanitized frame2/2 PASS. No flash/vendor/parser changes.
- [Evidence and limits](../audits/native-mixed-scheduling.md). Next: fatal traps.

## Native fatal-trap policy — 2026-11-11

- Reproduced illegal16 being skipped with FAULT_RESUMED on previous handler.
- Unknown exceptions/breakpoints/IRQs now retain raw diagnostics and halt; only
  valid enabled M-mode task ecalls advance PC. No guessed instruction width.
- Fatal output uses bounded USB polling and bypasses display/normal console,
  heap and scheduler. RAM record is reset-volatile; no persistent writes.
- Host 9/9, sanitizer C fixtures 3/3, C++98 headers pass. Controlled illegal16/32
  and compressed-breakpoint tests compare cause/PC against ELF metadata.
- [Full actual evidence and limits](../audits/native-fatal-traps.md).
- Hardware final: all3 controlled fatal injections PASS; post-policy mixed
  regression PASS (250 IRQs,18142 ecalls,124 sleeps). Desktop/shell/SciCalc
  restored and verified after RAM reset. No flash/SD writes.

## Scheduler atomicity — 2026-11-11

- Local IRQ nesting and coherent task/tick/stat snapshots added. READY published
  after construction; delay/exit commit within saved-context traps. EXITED slot
  reuse keeps registered count stable; wraparound deadlines use modular arithmetic.
- **HARDWARE-TESTED, LIMITED:** lifecycle/mixed profile250 IRQs, 125 creates/returns,
 125 nesting checks, 4 slots, live integer/FP sentinels intact.
- Host10/10, sanitized atomic/integer/FP fixtures3/3 PASS; negative mutants rejected.
- No flash/parser/vendor runtime changes. Next: no-ready idle fallback.
- [Scope, tests and limits](../audits/native-scheduler-atomicity.md).

## Internal idle — 2026-11-11

- Replaced task revival on no-ready with a private static idle context/WFI;
 16 public slots unchanged, idle current ID=-1, coherent diagnostics added.
- **HARDWARE-TESTED, LIMITED:** all-sleeping/all-exited workload and empty boot
  each pass 150 real IRQs, idle MIE on/FS off and correct terminal task states.
- Host 11/11, sanitized fixtures 4/4 PASS; revival mutant rejected.
- No flash/vendor/parser changes. Next ordered step: stack bounds/guards.
- [Exact results and instrumentation limits](../audits/native-idle.md).

## Stack protection — 2026-11-11

- Pre-save SP/frame/guard checks use mscratch metadata; complete saved contexts
  are checked before selection/restore. Reserved/overlapping stacks rejected.
- Dedicated 4096-byte IRQ and 2048-byte emergency buffers; task minimum304/ 448
  bytes for integer/RV32F. Raw diagnostics append stack reason/measured handler SP.
- **HARDWARE-TESTED, LIMITED:** seven deliberate fault profiles stop at exact
  ELF PC with emergency=1; bad-SP cases keep source guard intact, including SP=0.
- Host 13/13, ASan/UBSan stack/kernel/atomic/idle fixtures 6/6 PASS; header mutants
  rejected after strengthening in-range misalignment regression.
- [Exact evidence and limits](../audits/native-stack-protection.md). No isolation,
  NMI/double-fault, native desktop or SMP completion claim.

Final regressions PASS: lifecycle/FP and integer 250 real IRQs each; both idle
profiles 150 each; actual C IRQ-stack checks all pass. Three ordinary fatal
profiles preserve raw cause/exact ELF PC. Preserved flash shell, SciCalc
`2+3*4=14` and desktop verified after reset. Logs and exact recovery artifacts:
`../maia-rtos-recovery/native-stack-protection-20261111/` (SHA256SUMS).

## Future minimal POSIX layer — 2026-11-11

- **DESIGN ONLY:** owner requested a future optional POSIX subset with minimal
  changes to whatever native system is complete when implementation begins.
- [Plan and acceptance criteria](POSIX_MINIMAL.md): fresh API/libc gap audit,
  public-API adapters, small separate native extensions, explicit feature matrix,
  single-process/multithread model and regression with POSIX enabled/disabled.
- PSE51 is scope guidance, not a compliance claim. No runtime/header/build/parser
  changes, hardware access or execution tests; document links/diff checked.
- Current native-driver migration remains the next implementation task.

## Native driver communication queues — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** native queue send/recv/count now serialize full
  payload/index transactions with nested local IRQ exclusion; no SMP guarantee.
- Item size is capped at 256 bytes to bound copy work; initialization rejects
  size/address overflow. Struct layout and function signatures remain unchanged.
- Host 14/14 and ASan/UBSan queue/integer/FP fixtures 3/3 PASS; negative local
  mutants rejected. Tab5 RAM workload: 253 real IRQs, 688 intact ordered packets
  from two task producers and one IRQ producer, zero errors. Initial workload
  failed an unjustified per-producer progress floor; adjusted workload/logs are
  documented in the [driver queue report](../audits/native-driver-queues.md).
- Preserved flash shell/SciCalc/desktop verified after reset. No foreign source,
  parser, module ABI, flash or legacy runtime removal in this unit.
- Next: serial I/O ownership/frontier and queue-backed service integration.
  Blocking waits, mutex/semaphore concurrency and full driver ports remain open.

## Native serial input service — 2026-11-11

- **HARDWARE-TESTED, LIMITED (USB):** raw USB/UART RX separated from board/display;
  permanent boot claim prevents legacy reader from stealing service input.
- Optional service: fixed owner, 64-byte queue, bounded 16-byte poll; FULL avoids
  another FIFO read. No implicit task/heap, blocking wait or RX IRQ wakeup.
- Host 15/15, sanitizer service/queue 2/2, C++98 headers and four negative mutants
  PASS. Final real USB sequence 0..255 passes with FULL=1, 317 real IRQs,
  errors=0 and auxiliary progress. Initial test failed auxiliary-progress criterion;
  consumer now sleeps while empty. [Exact evidence](../audits/native-serial-input.md).
- Service-off queue regression passes 253 IRQs/688 packets. Preserved flash
  shell/SciCalc/desktop restored; no SDK runtime, parser, module ABI or flash edit.
- Limits: owner must remain alive until reset, single-core, UART physical test
  pending, paced traffic only; desktop firmware still depends on FreeRTOS.
- Next: serial TX ownership/separation, keeping blocking waits and other driver
  migrations in separate increments. POSIX remains DESIGN only.

## Native serial output service — 2026-11-11

- **HARDWARE-TESTED, LIMITED (USB):** raw USB/UART TX separated from display;
  optional sole-owner service serializes whole records from multiple producers.
- 1..64-byte records, queue 8 plus one active record, 16 byte attempts/poll.
  FULL rejects whole enqueue; BUSY retains active offset. No heap or spin waits.
  Legacy TX/display mirroring is suppressed after claim; fatal sink stays separate.
- Host 16/16, sanitizer TX/RX/queue 3/3, C++98 headers and five mutants PASS.
  Final Tab5 capture: 80 records/3840 bytes, 260 IRQs, FULL=344, BUSY=239, errors=0.
  RX regression passes all 256 bytes/305 IRQs. [Evidence](../audits/native-serial-output.md).
- Flash shell/SciCalc/desktop preserved and verified. No foreign runtime, parser,
  module ABI or flash/SD change. Complete desktop still depends on FreeRTOS.
- Limits: single-core polling, permanent owner until reset, FIFO acceptance is
  not host ACK, UART physical/reconnect pending. Next: native atomic worker
  waits/wakeups and synchronization audit; POSIX remains DESIGN only.

## Native task notification waits — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** sticky per-task notify + blocking wait via labeled
  ECALL; check/block atomic after context save, first timeout/post completion wins.
  Late/second post retained; result installed once after frame validation.
- Single-core, no payload/count/heap, timeout0 EMPTY, finite TIMEOUT, explicit
  FOREVER. IRQ wait returns PERM; notify is ISR-safe without implicit yield.
- Host 18/18, sanitizer integer/FP notification + atomic/idle 4/4, C++98 headers
  and five mutants PASS. Actual Tab5: 40 rounds, 282 waits, 80 timeouts, 750 real
  IRQs, 520 blocked checks, errors=0; FP/lifecycle/fatal regressions also PASS.
- [Exact evidence and limits](../audits/native-task-notifications.md). Original
  flash shell/SciCalc/desktop restored; no parser, display configuration or flash
  edits. This does not fix blocking semaphores/mutexes, SMP or the complete runtime.
- Next: notification-driven TX worker on empty queue; display/PSRAM/cache/DMA
  remain separate prerequisites and risks. POSIX remains DESIGN only.

## Notification-driven serial TX worker — 2026-11-11

- **HARDWARE-TESTED, LIMITED:** accepted TX records notify owner after publication;
  owner-only wait blocks indefinitely only with no queued/active bytes. Stale
  hints are consumed then predicate rechecked; FIFO BUSY retains a finite retry.
- Host 18/18, sanitizer TX + integer/FP notifications 3/3, C++98 header and five
  mutants PASS. Native: 80 intact records/3840 bytes, 302 IRQs, quiet head20/tail21
  with polls/run_ticks stable, and real IRQ write wakes TX; errors=0.
- Old polling TX profile remains compatible (80 records/260 IRQs PASS).
  [Exact evidence](../audits/native-tx-worker-notifications.md). Flash desktop,
  shell/SciCalc restored. No kernel/trap/parser/display configuration/flash edit.
- Limits: single-core static owner, shared mailbox must be coordinated, no
  TX-ready IRQ/reconnect/producer-space wait. Complete desktop still uses FreeRTOS.
- Next: audit/preserve the working display initialization before native driver
  changes; PSRAM/cache/DMA/power/panel variants remain separate hardware risks.

## Display baseline audit — 2026-11-11

Read [display baseline audit](../audits/display-baseline-audit.md) and its pinned
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

Read [native touch transport/evidence](../audits/native-touch-identify.md). New
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
license/attribution and native dependencies. Read [expander port](../audits/native-expander-port.md).
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

Read [native panel reset port](../audits/native-panel-reset.md). ST712x-only checked
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

Read [VO3 calibration audit/port](../audits/native-ldo3-calibration.md). Pure native
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

Read [VO3 acquisition/evidence](../audits/native-ldo3-acquire.md). Boot-only fixed
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

Read [MIPI state audit](../audits/native-mipi-state.md). New isolated RAM diagnostic observes only
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

Read [native VDMA stop](../audits/native-vdma-stop.md). Original native C whole-controller boot
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

Read [active DMA fixture/evidence](../audits/native-vdma-transfer.md). Opt-in diagnostic reserves66048B
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

Read [MIPI reset parking](../audits/native-mipi-park.md). New native boot-exclusive API requires
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

## Checked native MIPI clock gating — 2026-11-12

Read [MIPI gate evidence](../audits/native-mipi-gate.md). Gate API requires internal native park
claim, inactive VDMA and reset-state MIPI. Initial clock-off-only ISP guard
refused02E82000; actual inspection found CNTL40002442/CAM4 (CSI input disabled,
DVP disabled/held reset). Final guard accepts only that disconnected input mode
when clocked, or skips unreadable clock-off/held-reset ISP; enabled/other sources
reject. No ISP clock enabling or fabricated idle signal. Hold ISP/CSI host/CSI
bridge/DSI bridge resets, then clear PHY93 and SYS1C00, checking full readbacks/
preserved bits and fresh RCC/VDMA. No MIPI/ISP MMIO after reset; DMA clocks stay on.
Failure after guards sticky until reset, no rollback/retry; duplicate BUSY.
Target RAM PASS: reset40->3C000040, PHY2B3->220, SYS0C0D5E78->0C0D4278;
stage5/attempt7/preserved1/duplicate_busy1. ISP clock remains on, module held reset.
Host28/28, sanitizer/C++98/four mutants and invalid profile PASS. Preserved flash
shell/SciCalc14/desktop verified after reset; no flash/SD/LDO/analog-power writes.
Artifacts: `../maia-rtos-recovery/native-mipi-gate-20261112/`. This resolves only
boot-reset clock gating; not live ISP/CSI/video stop, electrical settling or an
unconditional power-cycle permission. Next: compose guarded VO3 off/acquire test
with native park/gate, retaining supply/discharge/stabilization limitations.
Full desktop still FreeRTOS-based; native cold display/PSRAM/POSIX pending.

## Guarded native VO3 off/acquire — 2026-11-12

Read [VO3 cycle audit](../audits/native-vo3-cycle.md). Opt-in RAM diagnostic composes native park/gate
with calibrated VO3 off request and unchanged native acquire. Boot CSI preparation
now checks idle DMA/disconnected or reset-held ISP, asserts CSI resets, enables
only its access/config clocks, releases CSI resets, then reads shutdown defaults.
Actual inherited RESET38000040 ->20000040 retains ISP reset; warm RAM/reset is
not a cold state. Early bus-clock refusal and CSI access-fault candidates are
preserved; neither reached VO3 writes. VO3 profile installs native trap vector.
Final target PASS: held reset3C000040/gated SYS0C0D4278/PHY220,
CTRL40200180 ->40200080 ->40200180, ANA D5800000/dref13/mul3 retained.
Native acquire verified5/non-adopted/changed,16 stable samples per phase, ten
other LDO words and clock/reset/VDMA guards preserved, duplicate BUSY. Host quiet
windows20.946/21.124ms are not physical voltage/settling proof; analog/source
programming was a no-op because values matched. Host30/30, two sanitizer fixtures,
four compiled negative mutants and invalid profile PASS. Shell/SciCalc14/desktop
restored, no flash/SD writes; artifacts `../maia-rtos-recovery/native-vo3-cycle-20261112/`.
No public API/default firmware change or new foreign runtime dependency. Full
desktop still FreeRTOS-based. Next: checked native DSI clock/PHY prerequisites;
cold panel/display/PSRAM, electrical characterization/live stream shutdown and
POSIX implementation remain pending.

## Native DSI configuration access — 2026-11-12

Read [native DSI access](../audits/native-dsi-access.md). New boot-exclusive SoC API requires all MIPI
clocks gated, shared resets held, overrides clear, inactive clocked VDMA and
matching active calibrated VO3. Enables only DSI SYS12/PHY config0, then releases
only DSI bridge RCC reset26, with fresh full guards/readbacks. No DSI MMIO before
access/config clocks and reset release; no ISP/CSI MMIO. Target RAM stage5/attempt7
PASS: SYS0C0D4278->0C0D5278, PHY220->221, RESET3C000040->38000040; VO3 unchanged.
VERSION3134312A, POWER/PHY_RSTZ/BRIDGE/FIFO0, DPI19D0(enable0), STATUS1528 observed,
not treated as PHY-ready proof. CSI/ISP resets held; reference/DPI clocks remain
gated. No PLL/source/test-port/analog/lane/video/panel/DMA/PSRAM writes.
Sticky failure/no rollback, duplicate BUSY without I/O; no runtime ownership or
release implementation. Host31/31, sanitizer, C++98, four compiled mutants and
invalid profile PASS. Shell/SciCalc14/desktop recovery verified; no flash/SD writes.
Recovery `../maia-rtos-recovery/native-dsi-access-20261112/`. Default production
firmware unchanged and still FreeRTOS-based. Next: verified native DSI reference
source/provider/frequency, then PHY PLL/lane sequence; cold display/PSRAM/electrical
settling and POSIX implementation remain pending. No parser changes.

## Native nominal20MHz DSI PHY reference — 2026-11-13

Read [DSI reference](../audits/native-dsi-reference.md). Existing native DSI module now requires internal
successful access claim, quiet DSI/VDMA, calibrated VO3, CSI/ISP reset-held state,
LP XTAL40/SPLL480 gates and shared REF20 divider24. Never reprograms shared divider
or root PLL/LP power. Enables provider if needed, gates config around mux change
(the mux feeds both config/reference), restores config, then enables reference.
No gated DSI MMIO; fresh full guards/readbacks, skipped matching writes, sticky
failure/no rollback, duplicate BUSY without I/O. Not a runtime clock manager.
Actual Tab5 RAM PASS stage7/mask10: divider58170503/LP1FFCFFFD/provider115/source
38862600 retained, PHY221->223 one actual reference-gate write;16 stable samples
and supplied VO3/eFuse preserved. Model20MHz, no physical measurement/PLL-ready
claim. Provider enable/nonzero mux switch branches HOST-TESTED only. Host32/32,
sanitizer ASan/UBSan, C++98, four compiled mutants/invalid profile PASS. Original
shell/SciCalc14/desktop restored, no flash/SD writes. Recovery:
`../maia-rtos-recovery/native-dsi-reference-20261113/`. Default firmware still
FreeRTOS-based. Next: native PHY PLL/test-port configuration with bounded failure,
then analog/lane lock/stop waits; no cold display/PSRAM/POSIX implementation yet.

## Native two-lane960Mbps DSI PHY — 2026-11-13

Read [native PHY](../audits/native-dsi-phy.md). New native PHY API requires internal reference claim,
quiet inactive DMA/bridge/DPI/host/PHY, no HS request, lock initially low and fresh
clock/reset/provider/divider/supply guards. Adapted pinned SDK init/test-port byte
sequence, Apache-2.0 attributed in dsi_phy_program.h; no foreign runtime imported.
Fixed M48/N1/hsfreq1A =>nominal960Mbps (working BSP requests965, SDK rounds960).
Masked checked lane/host/analog/reset/clock/forcePLL and five byte writes, RO test
output excluded from writable readback. Caller budget1..100000 observes lockbit0
and all lane-stop94; sticky failure/timeout until reset, no rollback. PHY can drive
LP idle, no packet/HS/video/panel command. Actual Tab5 RAM stage11 PASS:40 stores
(lanes already1), five bytes, seven observations, STATUS15BD/power1/PHY_RSTZF;
16 stable samples/guards preserved/duplicate BUSY. Not measured bitrate/timing.
Host33/33/sanitizer/C++98/four compiled mutants/invalid profile PASS;41 dropped-
store tests distinguish37 real failures from4 harmless identical clock-low stores.
Shell/SciCalc14/desktop restored; no flash/SD writes. Recovery
`../maia-rtos-recovery/native-dsi-phy-20261113/`. Default desktop still FreeRTOS.
Next: native command-mode/escape+timeout clock configuration and bounded LP FIFO
transfers, then panel DCS initialization; video/PSRAM/full native desktop pending.

## Native DSI command-mode host configuration — 2026-11-13

Read [command configuration](../audits/native-dsi-command.md). Native boot API requires internal PHY claim,
locked/stopped2lanes/quiet VDMA/bridge/DPI, stable clock/supply and generic FIFOs
empty/not busy.14 masked checked steps: mode1, timeoutdiv12/escapediv6, SDK PHY
switch/read/stop timings, CRC/ECC/HS-EoTp. Hardware timeout counts0 by setup policy;
no transaction/unbounded wait introduced. Packet-specific LP/BTA/read handling pending.
Fresh full table/root/PMU/PHY/FIFO guards, no-op skipping, sticky failure/duplicate
BUSY without I/O. Attributed Apache-2.0 table port; no foreign runtime headers/API.
Actual Tab5 RAM stage16/mask181E PASS: six writes, CLKMGR0C06/DATA00320068/
CLK002E0080/PCK19/MAXRD1770/PHY_IF3F01, MODE already1. STATUS15BD/FIFO00050015
(low7=15),16 later stable samples/guards preserved. Modeled20/10MHz, not measured.
No header/payload/HS/panel/video/DMA/PSRAM writes. Host34/34/sanitizer/C++98/four
compiled mutants/invalid profile PASS. Original shell/SciCalc14/desktop restored,
no flash/SD writes. Recovery `../maia-rtos-recovery/native-dsi-command-20261113/`.
Default desktop still FreeRTOS. Next: packet-specific LP/VC settings and bounded
short writes, distinguishing host completion from panel ACK; then panel DCS/video/
framebuffer/PSRAM. No full native desktop or POSIX implementation claimed.

## Bounded native LP DCS short writes — 2026-11-13

Read [LP short transport](../audits/native-dsi-short.md). Native boot API requires internal command claim,
stable cached14 config/root/supply/PHY, empty generic FIFOs and ACK/TE/BTA off.
DCS0/1, VC0..3/byte validation, LP bit16/17 masked, one GEN_HDR publication (no
readback/payload), finite budget1..100000 observations, allow lane-stop activity
while PLL stays locked. Read/OR error0mask1FFFFF/error1mask81FFF; reads may consume
latches, no clear/mask writes. Sticky failure/timeout, no rollback/retry after
possible publication. Success allows next exclusive call; not a runtime queue.
Clock guard before LP readback fixed under host fault injection before target load.
Actual Tab516 VC0 NOPs PASS: stage5/header05/attempted1, each2polls/busy witnessed;
mode0->10000 once, final STATUS15BD/FIFO50015/errors0, prior config/root/supply
preserved. panel_ack0: host drain/stop, not panel reception proof. Fast/unobservable
publication limits explicit; DCS1/nonzeroVC host-tested only. Host35/35/sanitizer/
C++98/five compiled mutants/invalid profile PASS. Shell/SciCalc14/desktop restored,
no flash/SD writes. Recovery `../maia-rtos-recovery/native-dsi-short-20261113/`.
New packing helper Apache-2.0 attributed, no foreign runtime or parser changes.
Default desktop still FreeRTOS. Next: bounded LP long payload writes for vendor
panel startup, then panel-specific reset/DCS integration; ACK/read/video/PSRAM/
full native desktop and POSIX implementation remain pending.

## Bounded native LP DCS long framing — 2026-11-14

Read [long transport](../audits/native-dsi-long.md). New boot API stages1..64 payload bytes (command
included) then exact WC/DCS39 header.64 is policy, not verified FIFO depth. Bytewise
unaligned/end-safe packing and zero high tail bytes; no SDK overread/unbounded wait.
Aggregate budget covers space/header/drain, no reset per word. Partial payload
failure poisons BOTH long/short paths until reset, no flush/retry/header on partial
WC. Shared guards renamed, short semantics otherwise preserved. Attributed Apache
packing helper, no runtime imports. First target capture exposed internal buffer
pending while front FIFO empty (10015). Preserved initial image/log; strengthened
long initial/final maskF007F==50015, command30003==10001, room80008, then retested.
Final Tab54 long VC0/NOP00/WC1 PASS: stage6/header139/1word/1byte,4polls/2space/busy
witness each, pre10015->final50015/STATUS15BD/errors0, LP10000->90000 once; short
NOP followup PASS. Full prior config/root/supply retained, panel_ack0. Multiword/
lengths2..64/nonzeroVC host-only; no panel acceptance or initialization claim.
Host36/36/sanitizer/C++98/seven compiled mutants/invalid profile PASS. Shell/SciCalc
14/desktop restored, no flash/SD writes. Recovery
`../maia-rtos-recovery/native-dsi-long-20261114/` (build paths13 reflect start date).
Default desktop still FreeRTOS. Next: bounded read/BTA for panel response/identity,
then verified reset/DCS startup; video/PSRAM/full native desktop/POSIX pending.

## Native bounded RDDID read: reproducible PHY refusal — 2026-11-14

Read [read/BTA audit](../audits/native-dsi-read.md). Fixed VC0 DCS04/MRPS3 native API, bounded aggregate
poll budget, one RX-word maximum, error/guard checks, sticky shared failure and
success-only restoration of packet/RXVC/LP controls. No SDK/FreeRTOS runtime/header
imports; Apache-2.0 attributed packet definitions. HOST-TESTED successful path;
actual target successful read NOT DEMONSTRATED. Two RAM loads reproduced IO at
stage6/headers3/polls6, FIFO50005 (data available), PHY15BD, ST0=00100000
(DPHY_ERRORS_4). Rejected BEFORE pop, words0/restored0; zero ID fields mean unread.
No masking/clearing/automatic retry. Both preceding native short/long runs pass.
37/37 host tests/sanitizers/C++98/seven mutants/invalid profile PASS; runtime gate
clean. Exact error+RX-ready and capture false-success regressions included.
Both shell/SciCalc14/desktop recoveries verified; no flash/SD/default/parser changes.
Recovery `../maia-rtos-recovery/native-dsi-read-20261114/`. Working desktop remains
FreeRTOS-based. Next: resolve P4-specific error semantics and BTA/turnaround cause;
require error-free panel responses before panel-specific startup/video/PSRAM.
Do not ignore error20 based on SDK omissions or a different controller's manual.

## Native RDDID response after escape divider correction — 2026-11-14

Read [escape/BTA correction](../audits/native-dsi-bta-escape.md). The prior ede1698 refusal is historical;
successful bounded response is now HARDWARE-TESTED, LIMITED. Fixed CLKMGR0C06->
0C07 follows official Espressif cebb3dd4d8704a5aa1c2b3a23c4c4b9b26e71194 rounded
18MHz-target divider policy for960Mbps: byte120MHz/7 = modeled17.142857MHz.
Report derives integer17142857Hz from checked divider. No relaxed error masks,
BTA/read guards or invented turnaround registers; exact electrical cause unmeasured.
Two FINAL RAM loads each pass4 RDDID responses: stage10/headers3/oneword00FBA080,
low bytes80 A0 FB/polls7/errors0/PHY15BD/FIFO50015, mode90000 restored, mask5,
stable1/preserved1. Model/exact response length NOT established. Preceding native
16short/4long/followup and all clock/supply checks pass. Intermediate successful
runs with stale frequency report are preserved separately. Host37/37, four
ASan+UBSan fixtures/C++98/two targeted mutants PASS; native runtime gate clean.
Shell/SciCalc14/desktop recovered, no flash/SD/foreign-runtime/parser changes.
Recovery `../maia-rtos-recovery/native-dsi-bta-escape-20261114/`. Default desktop
still FreeRTOS. Next: guarded native panel reset integration and variant-verified
bounded DCS startup, then video/framebuffer/PSRAM; no native desktop/POSIX claim.

## Quiet native panel reset before DSI responses — 2026-11-14

Read [integrated reset](../audits/native-dsi-reset.md). New opt-in DSI_RESET profile reuses existing
native I2C/expander/board reset after proven MIPI gate+VO3, before DSI open. Exact
RCC/ISP/VDMA/supply guards, supported firmware1/3, verified GPIO23 input, both-bank
preservation and touch recovery; any error halts, no rollback/retry. Private boot
diagnostic only, no new production/public driver/parser changes. Actual two final
RAM loads PASS: sys0C0D4278/phy220/reset3C000040, stage9/reset~244ms, FW1/status0,
banks/clocks/supply/GPIO preserved, then four stable00FBA080 error-free RDDID words
each and prior native DSI/short/long checks. Host38/38, two ASan+UBSan fixtures/four
compiled mutants/invalid profile PASS, native runtime gate clean. Shell/SciCalc14/
desktop restored, no flash/SD writes. Recovery
`../maia-rtos-recovery/native-dsi-reset-20261114/`. Earlier standalone reset capture
remains qualified; new gate evidence applies to this integrated profile only.
Important: preserved BSP maps observed touch FW1 to ST7121, FW3 to ST7123. Do not
reverse this or guess from DSI raw word. Next: selected ST7121 command-table port
with bounded delays/failure policy; video/framebuffer/PSRAM/full native desktop/
POSIX pending. Default full desktop still FreeRTOS-based.

## Selected ST7121 command program and generic FIFO space fix — 2026-11-14

Read [program audit](../audits/native-st7121-program.md). New native board API executes exact883e9a6 software
reset+vendor table:31 packets/461bytes/requested1150ms, max61byte packet, firmware1
only. Portable checked callbacks, no heap/MMIO/foreign runtime; single lifetime
claim, success duplicate BUSY/failure sticky FAILED, no rollback/retry. Apache-2.0
table attribution retained; no DPI init/final duplicate29 imported. First target
attempt stopped A3/40bytes at stage3 after8 staged bytes/FIFO90015/internal-full
but generic-front not full. Corrected only payload-space predicate per pinnedSDK
LL: use front fullbit3, not internalbit19. Both initial/final buffer completion,
errors/guards/budget/partial-sticky policy retained. Refusal images/logs preserved.
Two final RAM loads pass31 packets including A7/61bytes(stage6/23polls), full waits,
then4 stable00FBA080 words each/stage10/errors0/modeB0000 restored/mask5. Host39/39,
two ASan+UBSan fixtures/C++98/five runtime mutants/invalid profile/table comparison
PASS; native runtime gate clean. Original shell/SciCalc14/desktop restored, no
flash/SD/parser changes. Recovery `../maia-rtos-recovery/native-st7121-20261114/`.
Next bounded panel-state readback before DPI/video/framebuffer/PSRAM. Program
completion does NOT prove vendor registers/sleep/display state/pixels. Default
complete desktop still FreeRTOS-based; POSIX remains DESIGN.

## Native panel power-state readback — 2026-11-14

Read [power-state audit](../audits/native-panel-power.md). New fixed VC0 RDDPM0A/MRPS1 API shares the existing
private bounded read state machine with RDDID04/MRPS3; guards/errors/sticky claim,
one RX pop and success-only host restoration unchanged. Low byte only is power
payload; upper word/id[1..2] are not requested data. MRPS1 persists until next
request. No arbitrary read API/foreign runtime/parser/board-table change.
Two final RAM loads each read4 stable08 before and4 stable9C after unchanged
ST7121 program. Every power read stage10/headerA06/headers3/polls6/oneword/errors0,
PHY15BD/FIFO50015, mode90000 orB0000 restored/mask5. Standard DCS bit4 SleepOut
andbit2 DisplayOn transition0->1; stronger than command drain, still no pixels.
Primary pinnedSDK0A and Linuxv6.12 bit facts, corroborating Sitronix ST7121P V1.0
2024/01 p103; P suffix not established for this device. No Linux code imported.
Host39/39/ASan+UBSan/C++98/five runtime mutants/invalid profile PASS. Capture tests
reject already-on baseline/missing transition and allow unrelated bit changes.
Runtime gate clean; prior native program/reset/read checks pass. Shell/SciCalc14/
desktop recovered, no flash/SD writes. Recovery
`../maia-rtos-recovery/native-panel-power-20261114/`. Next native DPI format/clock/
timing with DMA inactive and matched panel format, then video/framebuffer/PSRAM.
Complete installed desktop still FreeRTOS-based; no pixels/POSIX claim.

## Inactive native DPI format/clock-route/timing stage — 2026-11-14

Read [DPI staging audit](../audits/native-dpi-stage.md). New generic SoC timing API stages19 words,
RGB565/VC0/burst+LP, nominalPLL240/div3=80MHz for existingPHY960. Geometry fromBSP
fixture720x1280/H2,40,40/V20,24,200; no resolution in generic kernel. Working BSP
requests70MHz but pinned integer divider produces80; report modeled only, gateoff.
Native exact RMW/readback/root/supply/VDMA/command/PHY/FIFO guards; frameACK/pattern
mustoff. No bridge update/output/video/HS/DMA/framebuffer/IRQ/provider/PLL writes.
Partial failure poisons stage+transfers until reboot; duplicate BUSY without I/O;
post-stage command reads work. Host40/40/ASan+UBSan/C++98/five runtime mutants/
invalid profile PASS, including20 dropped stores, clock-loss readback protection,
no-op profile and invalid geometry. Native runtime gate clean.
Two final RAM loads PASS stage22/maskFFF30(14stores), route223->223,19 words checked,
MODE1/bridge0/DPI19D0/clock223 gateoff/update0/DMAquiet1/preserved1/duplicateBUSY1;
four post-stage power words9C each/stage10/errors0/restored controls. Nonmatching
route-write branch HOST-TESTED only; live timing latch/actual frequency not proven.
Original shell/SciCalc14/desktop restored; no flash/SD/parser/default changes.
Recovery `../maia-rtos-recovery/native-dpi-stage-20261114/`. Next panel-format
verification and native clock source/provider/enable with output inactive, then
latch/video/framebuffer/PSRAM. No pixels/full native desktop/POSIX; installed
working desktop still FreeRTOS-based.

## Reported RGB565 selection and inactive DPI clock enable — 2026-11-14

Read [format/clock audit](../audits/native-dpi-clock.md). Fixed color0C/MRPS1 read shares native guarded
transport; private staging cache and new clock API require actual stage claim,
exact19 words/root/supply/PHY/mode, bridge/output/DMA inactive. Enable REF240 bit30
atHP+28 (preserve shared20MHz divider), then DPIgate7 atHP+3C, no source retune/
PLL/latch/video/HS/DMA/IRQ writes. Duplicate BUSY/no I/O; partial failure poisons
all transfers until reboot. LP preflights accept clock-on only with private claim,
correct240/div3 route/provider; external enable and provider/route/gate drift reject.
First format read00 was refused before writes; documented initial00 and M5GFX
28d47949 COLMOD55 policy support explicit selection, not accepting00 as16-bit.
Two final RAM loads: color00->05 after one3A55 (header553A15), then clock223->2A3/
provider58170503 unchanged/mask2/stage4, MODE1/bridge0/DPI19D0/update0/DMAquiet1.
Four post-clock color05 and power9C reads each/errors0/restored LP controls.
Nominal80MHz only, no measurement; provider-off/skip-format branches host-only.
Host40/40/two ASan+UBSan fixtures/C++98/six runtime mutants/invalid profile PASS;
runtime gate clean. Shell/SciCalc14/desktop restored, no flash/SD/parser/table edits.
Recovery `../maia-rtos-recovery/native-dpi-clock-20261114/`, includes initial refusal.
DBI16-bit readback is not actual wire/pixel proof; supplier ST7121P suffix unproven
and its datatype table lists24-bit only. Working BSP/M5GFX use565; next guarded
latch/native test pattern (noDMA) to prove pixels/compatibility before framebuffer/
PSRAM. No full native desktop/POSIX; installed complete desktop still FreeRTOS.

## Native host color bars visible; bounded return to commands — 2026-11-14

Read [pattern audit](../audits/native-dpi-pattern.md). Native boot-only pattern start/stop requires private
stage+clock ownership, full cached configuration/root/supply/PHY guards, shadow0,
bridge/output/DMA inactive. Checked SDK vertical VPG mode (owner observed horizontal bars), automatic HS request,
video mode; exclusive transport BUSY during active pattern. Stop command/LP,
bounded stopped/empty TX wait, exact VPG restore; partial failure sticky/reboot.
First attempt had error-free controls but owner saw black. Final diagnostic adds
private saved/restored GPIO22 DC HIGH lighting (Tab5 PDF p12 G22/LEDA, pinned GPIO
fields); owner confirmed stable bars. Final native RAM SHA22fa5cedf860437a2f1ec529
1119654cecab8008a6d0d452e3f8c1a84e4a91b2. VID1BF02/MODE0/LP3 for5 samples,
PHY1529/15B9/errors0; stop VIDBF02/MODE1/LP0 in1 poll, GPIO restored,4 power9C
responses/errors0/controls restored. Bridge/update/output/DMA stayed inactive.
Host41/41, two ASan+UBSan fixtures, C++98, six rejected runtime mutants and invalid
profile PASS; native runtime gate clean. No flash/SD/parser/table/timing changes.
Recovery `../maia-rtos-recovery/native-dpi-pattern-20261114/`. Installed full desktop
still FreeRTOS; restored shell/SciCalc14/desktop. Host pattern bypasses bridge, so
no DMA/framebuffer/PSRAM or full native desktop proof. Next guarded bridge/DPI latch
with output/DMA off, then owned SRAM framebuffer; POSIX remains DESIGN.

Orientation clarification: the owner observed HORIZONTAL stable bars. Production
`platforms/tab5/main/hal/hal_esp32.cpp:104` uses `LV_DISPLAY_ROTATION_90`, software
rotation: native panel720x1280, logical desktop1280x720. The host VPG bypasses LVGL
rotation. Preserve desktop and input-coordinate orientation when implementing
native framebuffer presentation; verify direction with an asymmetric pattern.
See [correction](../audits/native-dpi-pattern-orientation.md).

## Native inactive bridge enable and update publication — 2026-11-14

Read [bridge audit](../audits/native-dpi-bridge.md). New native boot/core0/exclusive API requires real
stage+clock claims and cached19/root/supply/PHY/LP-command guards, stopped/empty TX,
bridge/output/DMA/VPG/shadow/frameACK/IRQ enables off. Checked bridge EN0->1,
then literal1 to DPI_UPDATE+44. Register is WT, no ACK: no read/poll/RMW/retry;
latch_verified0, dropped publication explicitly unobservable in host fixture.
Private expected bridge bit permits LP traffic only after checked native enable;
external adoption/loss refuses. Original reference-acquisition bridge-off guard
preserved. Duplicate BUSY/no I/O; partial failure sticky/reboot, VPG refused once
bridge owned. Final native RAM load PASS stage4/mask3/EN1/DPI19D0/MODE1/LP0/
DMAquiet1/IRQ_RAW0/errors0 and four power9C words/errors0/restored controls. Earlier
candidate also passed, images/captures separate. Final SHA213a32b2288b58ebe4ea8363
d267a3b38c5d4cfbfc15a29ee5ac6999b476e10d. Host41/41/ASan+UBSan/C++98/six rejected
runtime mutants/invalid profile PASS; native runtime gate clean. Shell/SciCalc14/
desktop restored; no flash/SD/parser/table/timing/production rotation changes.
Recovery `../maia-rtos-recovery/native-dpi-bridge-20261114/`. No live latch or
DMA/framebuffer pixels claim. Next owned SRAM DMA/bridge flow/descriptor contract
and inactive staging before pixels; full frame1,843,200bytes cannot fit the tested
RAM-image window. PSRAM and full native desktop pending; installed complete desktop
still FreeRTOS. Logical1280x720/productionLV_DISPLAY_ROTATION_90 and owner's
horizontal visible-pattern clarification retained. POSIX remains DESIGN.

## Owned SRAM tile and finite DMA descriptors, no engine activation — 2026-11-15

Read [tile/list audit](../audits/native-dpi-dma-tile.md). New private SoC pure plan/encoder validates bounded
RGB565 geometry, disjoint/aligned diagnostic SRAM capacities and item indices.
Source8rows/11,520bytes reused by160x64-byte LLIs, finite tail0/LAST only tail,
source/list master1, DSI destination50105000/master0/fixed,64-bit/item counts.
Host-only independent pinned SDK types/map oracle confirms constants. No target
SDK headers/runtime/allocator imported. Opt-in22,272byte NOLOAD reservation outside
BSS/stack, ROM cache WB/invalidate only owned range, then CPU-only NC access.
One RAM load: reserve4FF0FC80..4FF15380, source4FF0FD00/list4FF12B00/cacheline64;
source hashE18C2E45/listF8CAF0B9 independently match, guards/preserved/inert1.
All160 items read back,3 samples correct; four power9C reads/errors0/restored.
No DMA register programming or LLP publication/channel/video/pixel enable. Thus
HARDWARE-TESTED memory construction only, not descriptor fetch/pixels/PSRAM.
Host42/42/ASan+UBSan/C++98/six rejected mutants/invalid profile/oracle PASS; runtime
gate clean. Image SHA dfc11fe6b6012e7411e3cfaac2175c80383ee53ca307ab9ccc18d5a85d69f57b.
Shell/SciCalc14/desktop restored; no flash/SD/parser/table/timing/rotation changes.
Recovery `../maia-rtos-recovery/native-dpi-dma-tile-20261115/`. Next checked inactive
channel/list/handshake/bridge flow staging, then bounded transfer/stop/pixels.
Full installed desktop still FreeRTOS; native diagnostic independent. Keep owner's
horizontal-bar observation and logical1280x720/software90 rotation contract; POSIX
DESIGN. SDK marker-helper comment ambiguity remains for future actual fetch gate.

## Inactive single-block channel0 and DSI flow staging — 2026-11-15

Read [DMA stage audit](../audits/native-dpi-dma-stage.md). New boot-only native solid-source API requires
actual bridge/stage/clock claims and full idle/root/config guards. Bridge frame
count field4bits cannot directly encode160; prior tile/list remains CPU-only and
unused. Stage contiguous single frame230400x64-bit, fixed8-byte source and fixed
DSI50105000, no LLP, DMAC flow/HW DSI0 handshake, priority1/outstanding5,2. Source
single-item/no ARLEN override, destination256items/AWLENfield15. Sixteen masked
words checked, reserved bits preserved; no enable/IRQ/WT/status-clear writes.
First optional status-address read50081148 trapped before configuration stores;
all four optional status-fetch addresses now excluded/writeback disabled. AWLEN16
read15; strict refusal retained, final explicit15 verified. Refusal images/logs
preserved; one failed diagnostic rebuild repeated prior image, not new evidence.
Final RAM PASS stage18/maskD975/source4FF10E80/CTL1F8000/CFG1A020001/globalCFG0/
channels0/DPIoff, hashes unchanged and four power9C replies/errors0. Host42/42,
ASan+UBSan/C++98/seven rejected mutants/invalid profile/SDK oracle PASS; runtime
gate clean. Final SHA20fe0dcdba0dc61ee1fbaf34163fc039a45a6788d408ff021795696a11d19287.
Shell/SciCalc14/desktop restored; no flash/SD/parser/table/timing/rotation changes.
Recovery `../maia-rtos-recovery/native-dpi-dma-stage-20261115/`. No transfer/pixels
proof yet. Next bounded solid single-frame activation/completion/stop and visual
color with temporary light; do not enable160-LLI plan. Optional-register fault and
AWLEN normalization root causes open. Native full desktop/PSRAM/rotation-input
acceptance pending; prior bars observed horizontal; POSIX DESIGN.

## One finite DMA frame completed/recovered; visual red unconfirmed — 2026-11-15

Read [frame audit](../audits/native-dpi-dma-frame.md). New boot-only API consumes private solid-stage claim:
globalCFG1(no IRQ), channel101, MODE0/LP3/output1/WT, bounded completion/drain and
guarded cleanup channel100/global0/output0/WT/MODE1/LP0. Transfer/recovery results
separate; errors retained; lost ownership can prevent cleanup. Single use/BUSY,
partial failure sticky. Active guards allow live progress; source immutable8bytes.
Initial frame DMA-done/FIFOempty but raw count383FF was refused against38400.
Separate SRAM copies verify actual8/64/256/32768bytes with rawN-1 for8/64-bit data.
Expose raw/normalized separately: only done+natural-disable+expectedraw confirms
38400items. Qualification first HOST FAIL was an unnecessary done-bit requirement
(M2M target status0 with exact bytes); corrected byte-proof evaluator revalidated
the exact rows, preserving original raw FAIL. No display done requirement relaxed.
Final frame PASS: stage9/maskFFF/active1/done1/raw383FF/items38400/status12/errors0,
967 completion/2 recovery polls/13550us sample, all enables0/commands restored,
GPIO restored/hashes unchanged/four power9C replies. Visible red NOT CONFIRMED.
Host42/42/ASan+UBSan/C++98/six rejected mutants/invalid profile PASS; runtime gates
clean. Final SHAde826aeeb87f82a3a43d2e3e17bf4856b0d470525ffaebae56083924e2cc2e03.
Shell/SciCalc14/desktop restored; no flash/SD/parser/table/timing/rotation changes.
Recovery `../maia-rtos-recovery/native-dpi-dma-frame-20261115/`. Bridge raw0 is at
existing discard threshold413, not an all-pixel quality proof. Next bounded visual
refresh/color/underrun qualification, then framebuffer/PSRAM. Native full desktop
pending; installed complete desktop FreeRTOS; prior bars horizontal; POSIX DESIGN.

## Bounded native DMA refresh: stable red confirmed — 2026-11-15

Read [refresh audit](../audits/native-dpi-dma-refresh.md). Boot-only exclusive immutable-source batch1..400,
per-frame completion/recovery and fresh idle guards before status-clear/rearm;
only completion/disabled mask4000001B cleared, failures sticky/no retry. Old
standalone frame stays one-shot. RAM opt-in400 frames after explicit user readiness:
HOST PASS, 399 rearms/436369 polls/92160000 items/6109505us sample, last active/done1,
raw383FF/items38400/status12/errors0/FIFO0, recovered and four power9C replies.
User: “Sim, vermelho estável”. Previous short test had no valid visual observation
because user was not in front. Source/list hashes unchanged, light restored;
reset shell/SciCalc14/desktop PASS. Host42/42, sanitizers/C++98/five rejected
mutants/invalid profile/runtime gate PASS. SHA48ff157bbb7e68df5a2b65c32468d829371628ae251ae750c0df864b0d561650.
Recovery `../maia-rtos-recovery/native-dpi-dma-refresh-20261115/`. No flash/SD writes,
foreign runtime/parser/timing/rotation change. Diagnostic IRQs off; bounded batch
is not a production stream. Threshold413 still limits underrun interpretation.
Next owned finite SRAM pixel tile/source-increment spatial-color qualification;
PSRAM/full native desktop/rotation-input still pending, installed desktop original.

## Finite SRAM descriptor fetch/copy qualified — 2026-11-15

Read [list audit](../audits/native-vdma-list.md). New isolated VDMA_LIST probe: two finite64-byte LLIs,
first RGB565720x8 tile source increment11520bytes, second fixed8-byte8410 source
21248bytes; destination32768 bytes. Initial refusal16832bytes and descriptor
changes preserved; sampling proves8correct+24A5 per32bytes with automatic ARLEN.
Explicit second-node ARLEN_EN1/field0 gives exact32768bytes; not a universal AXI
encoding claim. LLI writeback clears VALID, writes per-block raw counts/status
(first59F/40000000, tailA5F/C0000000) despite SSTAT/DSTAT fetch disabled. New
contract requires writable exclusive descriptors, exact expected writeback; no
arbitrary mutation mask. Guards/source/unrelated peripherals retained; global/
channels/FIFO0, natural stop1, polls4170, runner PASS. Host43/43+ASan/UBSan PASS,
runtime gate clean. SHA c5a06dd084445025f0e314024b8cff93447da9e34ae33b09994dde2750affcc7.
Reset shell/SciCalc14/desktop PASS; no flash/SD/parser/panel/rotation changes.
Recovery `../maia-rtos-recovery/native-vdma-list-20261115/`. No display pixels
proven in this unit. Prior solid stage's automatic ARLEN does not prove claimed
8-byte footprint: red observation holds, containing tile was owned/red, but force/
qualify explicit source length before future fixed-source reuse. Next <=15-block
finite spatial display diagnostic with source increment and LLI writeback handling;
no160-LLI activation. PSRAM/full native desktop/production mapping still pending.

## Five-node native display chain; four stable bands confirmed — 2026-11-15

Read [spatial audit](../audits/native-dpi-dma-spatial.md). New opt-in private boot diagnostic, actual source tile
and writable five-LLI list, no160-LLI activation. Source increment for8 native rows,
then four solid bands312/320/320/320 rows; count5 fits bridge. Explicit ARLEN_EN1/
field0, source64-bit, finite LAST/terminal0, all exact LLI writeback checked. Existing
contiguous-frame completion contract unchanged; spatial path accepts only all five
writeback completion records/final DMA-done plus natural disable/rawE0FF/drain.
400 individually completed/recovered frames,399 checked rearms/414370 polls/
92160000 items/6109911us sample; stage18/frame9/maskFFF/status12/errors0/FIFOs0.
All80 descriptor words/source hashE18C2E45/unused tail/guards retained as specified.
Output/global/channel/command/LP/light restored; four9C replies; runner PASS.
Fresh readiness then one RAM upload. User confirmed four large stable bands but
DID NOT identify narrow strip: no complete incremental-pixel visual acceptance.
Initial capture45 command rejected before serial; actual capture30, same image.
Host44/44, ASan/UBSan, C++98/four rejected runtime mutants/invalid profile/runtime
gate PASS. SHA b2ae7a91e8f42a93283d39349cd639ca58df4b76db58a6f810b415f3e83a7648.
Reset shell/SciCalc14/desktop PASS. Recovery
`../maia-rtos-recovery/native-dpi-spatial-20261115/`. No flash/SD/parser/panel timing/
rotation change. Diagnostic boot IRQs off; not production streaming/desktop.
Next make incremental-source marker thicker/central within owned SRAM bounds,
then fresh visual readiness/test. Threshold413/WT no-ACK/counter-doc limitations
remain. PSRAM/full native framebuffer/desktop/production rotation-input pending;
installed desktop still original runtime. Prior solid API auto-ARLEN footprint
limitation remains documented in SRAM-list audit; new spatial path forces length.

## Central32-row native tile: stable aligned segments confirmed — 2026-11-15

Read [central audit](../audits/native-dpi-dma-center.md). Separate opt-in CENTER profile/reservation46848bytes,
source46080/list320used+64unused; legacy eight-row profile remains available.
Five nodes: fixed red320rows, green304, incrementing tile32rows at native624–655,
blue304, yellow320. Full frame unchanged1843200bytes, native720x1280/rotation90
untouched. Guarded source span grows; actual descriptor/cleanup protocol unchanged.
RAM PASS:400frames/399rearms/414268polls/92160000items/6109902us sample, last rawE0FF/
status12/errors0/FIFOs0, all80 LLI words exact, source hash4B6A57C5/tail/guards kept,
GPIO/output/global/channel/command/LP restored, four power9C replies. Buffer:
4FF12580..4FF1DC80; source4FF12600/list4FF1DA80;16KiB stack ends4FF21C80<4FF28000.
Host46/46+ASan/UBSan/central C++98/four rejected runtime mutants/invalid profile/
runtime gate PASS; prior actual eight-row capture still validates as legacy only.
SHA834ce950de16f5a39b6aca27bf0c7c136ee7d6f554621b00549bff1e16e02435.
Fresh readiness, ONE RAM upload. User initially selected visual fault/other, then
said small colored pieces at center; after expected32x180segment explanation,
confirmed “Sim, estáveis e alinhados no centro”. Preserve original answer and
clarification, not a bug fix/retest. Accept qualitative central marker, no all-pixel/
tear-free/full framebuffer acceptance. Reset shell/SciCalc14/desktop PASS.
Recovery `../maia-rtos-recovery/native-dpi-center-20261115/`, original digests intact.
No flash/SD/parser/panel timing/rotation changes. Next primary-source audit of native
PSRAM initialization/cache/DMA ownership and full-framebuffer memory budget.
Native production streaming/desktop and rotation-input acceptance still pending;
installed desktop original runtime. WT no-ACK/threshold413/counter-doc/legacy
solid auto-ARLEN footprint limitations remain; new spatial path constrains length.

## LCD/touch priority; native HEX PSRAM state observed — 2026-11-15

Owner prioritizes finishing native LCD+touch, remaining drivers, then MaiaScript.
See [delivery plan](LCD_TOUCH_NATIVE.md) and [PSRAM audit](../audits/native-psram-state.md). Correct earlier Octal shorthand:
working P4 SDK config/actual boot use HEX/X16/AP vendor0D/gen4/density7/32MB;
not S3 Octal driver. SDK startup reference: MPLL, VO2, dedicatedMSPI2/3, mode regs,
DQS/DLL/tuning, cache/MMU; these dependencies must be ported, not linked.
New boot-only read-only snapshot/isolated probe, nine roots/27 controller words,
conservative gated policy and fresh RCC comparison before every read; no array/
cache/MMU/power/clock writes. Host47/47+sanitizers+C++98/four runtime mutants/SDK
36-address oracle/invalid profile PASS. Actual result0/54samples stable: RCC
E6DFB7FF/D00C/38000040/norst0, VO2 40200180/76800000, bothCMD0/CLK30103;
MSPI0 AXI_REQ_EN0/read-write cmd0. Access is NOT memory readiness/init proof.
SHA a059fe491269d2cec18e4402a2edb45f441dcf32dd30f16d47744c7929bc29e1,
RAM4FF00000/6608bytes/entry4FF00100. Reset shell/SciCalc14/desktop PASS;
no flash/SD/parser/display changes. Recovery
`../maia-rtos-recovery/native-psram-state-20261115/`. Next bounded HEX MR identity/
restore, native power/clock/mode/MMU/cache and owned memory before framebuffer.
Touch coordinates/events/runtime bus ownership still pending. Native full desktop
not integrated; preserved complete installed firmware still original runtime.

## Native HEX mode-register identity and restore — 2026-11-15

Read [identity audit](../audits/native-psram-identity.md). New boot-only direct PSRAM command-port read, no SPI
ROM helpers (reference helper has unbounded wait/mutable callback). Source facts
SDKf5c3654 HEX s_get_psram_mode_reg + P4 registers. Inherited powered/timed device,
AXI off; temp OPI_DTR/4040/cmd16/address32/dummy12/CS1/receive poison, bounded poll,
guarded ten-word restore; no device mode/array/clock/LDO/MMU write. Failure sticky,
no active cleanup/abort/retry. Actual one RAM run: MR0/1=8D30, MR2/3=A0DF, MR4=20,
MR8=4B twice; vendorD/density7/32MiB/device3(gen4)/KGD6/X16 confirmed. Every read
stage5/result0/setup+restore3FF/polls1/busy0/idle/restored1; busy edge not sampled,
response proof is poison replacement and repeatable device fields. Roots/27 words
unchanged after all eight reads, runner PASS. Host48/48 + ASan/UBSan/C++98/six
runtime mutants/invalid profile/SDK mask oracle PASS. SHA
95cec29d4b5fbd6ff0b635c2bf420a9eafc37be8e3c6509ae0f0c3421f8d8471,
RAM10448bytes/entry4FF00100. Reset shell/SciCalc14/desktop PASS, no flash/SD writes.
Recovery `../maia-rtos-recovery/native-psram-identity-20261115/`. No native cold
PSRAM/array/capability/mapping/framebuffer claim. Next checked native power/clock/
mode startup + owned memory, then cache/MMU/display. LCD+touch remain priority,
other drivers then MaiaScript; delivery checklist updated without claiming cold init.
