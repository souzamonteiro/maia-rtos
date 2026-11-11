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
