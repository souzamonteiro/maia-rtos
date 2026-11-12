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
