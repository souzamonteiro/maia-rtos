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
