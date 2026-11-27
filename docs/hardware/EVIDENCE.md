# Hardware evidence register

## Native Tab5 I2S playback and ES8388 startup — 2026-11-26

The native I2S1 TX clock now uses the ESP32-P4 XTAL source (40 MHz), with a
fractional divider for 12.288 MHz MCLK and a 7:1 BCLK divider for stereo
48 kHz/16-bit audio. This matches the ESP-IDF v5.4.2 Tab5 clock-source
configuration; the earlier unsupported source selector left the I2S update
handshake pending.

The focused host tests (`product_touch_test`, `tab5_i2s_tx_test`, and
`tab5_audio_test`) passed, and the diagnostic P4 image passed packaging and
image-structure validation. On `/dev/cu.usbmodem1301`, the guarded hardware
probe completed two boots; both logged
`[AUDIO] ES8388 playback and speaker amp ready` and
`[AUDIO] 1kHz probe tone completed`, with no fatal marker. The probe report
recorded `probe_pass=true` and `restore_verified=true`. The original factory
app SHA-256 was
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Captures and report are retained at
`/Users/roberto/.copilot/session-state/79e3a5d6-7653-4cda-9830-edbdcc7ecc6a/files/tab5-audio-probe-xtal-20261126/`.

A repeat using the cleaned diagnostic image also completed both boots, with no
fatal marker and verified restoration. Its image SHA-256 was
`343c6e133303bd9037d30b12c8e9532ea0fb2a45ae691b603d8d2caafdfcbd47`;
the report and captures are retained at
`/Users/roberto/.copilot/session-state/79e3a5d6-7653-4cda-9830-edbdcc7ecc6a/files/tab5-audio-probe-repeat-20261126/`.
The user confirmed the 1 kHz tone was audible, validating the speaker output
path. ES7210 microphone capture, WAV/MP3 playback and recording, and a
product boot sound remain open. Temporary register-snapshot and timeout-delay
instrumentation was removed after the first successful probe.

## Native MNM graphics capability gate on Tab5 — 2026-11-23

The native shell previously exposed a fixed framebuffer at `0x48100000`,
outside the qualified PSRAM window at `0x4B800000`, while its drawing callbacks
were null and exclusive-mode acquisition returned success. The shell now
registers no module display table and reports the limitation at boot; the
touch/keyboard input table is still registered. The loader's imported
`fill_rect` wrapper returns `-1` if the display or callback is absent, and
continues forwarding both success and failure results when present. The ESP
display adapter is separate and was not changed.

Before the fix, the native-shell capability and missing-display wrapper tests
failed. Afterward, five focused host tests passed, including LCD-service and
MNM-write regressions; the shell/loader tests also passed in Release with
assertions and under ASan/UBSan. The P4 build and image validation passed.
An isolated snapshot with pinned submodules passed the same five host tests in
Release with `-UNDEBUG` and built a valid 847,088-byte P4 image
(`56b7baa7c1207ff60adedfe27d55db5b426ec47652e3ea002a2605936f9cef49`).
That isolated image was not flashed and lacks the uncommitted cache fix.
With stock Release `-DNDEBUG`, the existing `lcd_output_test` does not compile:
assertions disappear and leave unused parameters/variables under `-Werror`.
The isolated run explicitly retained assertions rather than changing that
pre-existing test.

On one warm-reset ESP32-P4 Tab5 boot over USB, the
[fixture](../../tests/hardware/maia_mnm_graphics_unavailable_probe.c) ran as a
1,748-byte RISC-V module. It checked the null display table, registered input
callbacks and negative `fill_rect` result; it returned zero and the shell
subsequently echoed `MNM_SHELL_ALIVE`. The boot warning and these results were
checked separately from the runner's prompt marker. The probe was transferred
in 32-byte chunks with every chunk and EOF verified, and its temporary file and
new `/sd/tc42h` directory were removed (`MNM_GRAPHICS_CLEAN`).

The protected flash runner observed no fatal marker and verified restoration
of the previous factory-app write. The backed-up app SHA-256 was
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`,
the tested image was
`1db3dae86cba0b371171eed427cb1b3e3ff78146e476ab14b6190a8bf05b7006`
and the probe module was
`ebd2482c48907e6f057f945851be6c72e958ab7407b9016bf0503f6e5091a81e`.
The image includes earlier uncommitted changes, including instruction-cache
synchronization; no full-flash comparison was made. FAT metadata may have
changed, and the restored firmware synchronizes its desktop module to SD.
The images, runner, input/output, backup, report and restored boot capture are
in [the recovery evidence](../../../maia-rtos-recovery/native-tab5-mnm-graphics-gate-20261123/).

This is a safety gate, not a graphics bridge: `display_sync` is void, modules
that ignore unavailable capabilities can run invisibly, and no native desktop
or SciCalc rendering is established.

```sh
cmake --build /tmp/maia-rtos-battery-host --target native_shell_display_test elf_loader_context_test elf_loader_context_no_tls_test shell_mnm_write_test lcd_output_test -j4
ctest --test-dir /tmp/maia-rtos-battery-host --output-on-failure -R '^(native_shell_display_test|elf_loader_context_test|elf_loader_context_no_tls_test|shell_mnm_write_test|lcd_output_test)$'
cmake --build /tmp/maia-rtos-battery-p4 --target maia_m0_image -j4
```

## MNM shell write callback on Tab5 — 2026-11-23

The shell's `write_fd` returned zero after successful writes because the
remaining count was decremented to zero. It now returns the number of bytes
handed to the callback, or `-1` for invalid input, missing output callbacks,
and observed redirection/pipeline staging failure. Only stdout/stderr and
NUL-free text are supported; a zero-length write to a valid handle succeeds.
Callbacks return `void`, so their physical delivery cannot be confirmed.
See the [current shell contract](../shell/MaiaRTOS_Native_Execution_Architecture.md#54-native-host-abi-v1).

The host regression exercises direct output, chunking, stdout redirection,
stderr, pipeline, missing callbacks, invalid arguments and embedded NUL.
It confirms 65,535 staged bytes succeed, 65,536 fail, and an existing target
is preserved on staging exhaustion. Five focused host tests passed; the write
test also passed in Release with assertions active and under ASan/UBSan.
The P4 `maia_m0_image` build and image-structure validation passed.
An isolated snapshot of the staged source with pinned submodules also passed
the five host tests in Release and built a valid 847,200-byte P4 image
(`85d4ee77d01cb3bda5b506431cbdbd71f3530601d4b1c9cd6efc3060e08ea845`).
That isolated image was not flashed and lacks the uncommitted cache fix.

On one warm-reset ESP32-P4 Tab5 boot over USB, the
[fixture](../../tests/hardware/maia_mnm_write_probe.c) executed as a 1,836-byte
RISC-V module. It checked a 138-byte stdout return, stderr, invalid handles,
embedded NUL, null buffers and oversized length, then returned zero. A second
execution redirected stdout; stderr remained in the capture, and the 138-byte
file and EOF were verified by reading them back. The module was transferred in
32-byte chunks with every chunk and EOF checked. The test removed its files
and newly created `/sd/tc42g` directory (`MNM_WRITE_CLEAN`); FAT metadata may
have changed.

The protected flash runner observed no fatal marker and verified restoration
of the previous factory-app write. The backed-up app SHA-256 was
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`,
the tested image was
`cc1e7484c84787d0b7276ae591980ff2347942769f77e23536c54b381ace4aa6`
and the module was
`7192ffc3072e6d189d6c24cd53b05c72b69f66ec31a069d7a00f71f2d414937a`.
The image includes earlier uncommitted work, including instruction-cache
synchronization; no full-flash comparison was made. The restored firmware's
boot log shows it synchronized its desktop module to the SD card.
The images, runner, input/output, backup, report and restored boot capture are
in [the recovery evidence](../../../maia-rtos-recovery/native-tab5-mnm-write-20261123/).
This smoke does not qualify binary output, physical serial reliability, native
desktop rendering or SciCalc.

```sh
cmake --build /tmp/maia-rtos-battery-host --target shell_mnm_write_test shell_mnm_sleep_test elf_loader_context_test elf_loader_context_no_tls_test shell_expr_control_test -j4
ctest --test-dir /tmp/maia-rtos-battery-host --output-on-failure -R '^(shell_mnm_write_test|shell_mnm_sleep_test|elf_loader_context_test|elf_loader_context_no_tls_test|shell_expr_control_test)$'
cmake --build /tmp/maia-rtos-battery-p4 --target maia_m0_image -j4
```

## Nested MNM host context on Tab5 — 2026-11-23

The loader now sets the wrappers' active host only while invoking a module
entry and restores the previous host on return. Validation failures no longer
replace it. The native build with `MAIA_ENABLE_COMPILER_TLS=1` stores the
pointer in static task TLS; the non-TLS and ESP fallback is global. All
concurrent loader invocations still require caller serialization; neither this
change nor the test claims that the loader or shell is generally reentrant.
Nonlocal exits from a module are unsupported.

On macOS, `elf_loader_context_test` and `elf_loader_context_no_tls_test`
passed for invalid ELF and synchronous nesting with distinct host tables.
Only the TLS variant interleaves two host threads. Successful nesting in
these tests invokes the internal entry helper directly, whereas the invalid
ELF case exercises `mnm_load_and_run`. Both variants passed ASan/UBSan, and
both plus `shell_mnm_sleep_test` passed a Release build with assertions
retained. A deliberately global-pointer mutant failed the isolation
assertion. The public loader header compiled as C11 and C++98. The P4
`maia_m0_image` target built; `riscv32-esp-elf-readelf -sW` showed
`s_active_host` as a local, four-byte TLS symbol.

On one warm-reset ESP32-P4 Tab5 boot over `/dev/cu.usbmodem1301`, the C fixture
`tests/hardware/maia_mnm_nested_probe.c` executed as a 2,024-byte RISC-V
module. It used the real shell callback to run the embedded `hello` module,
checked its return code 42, then ran a one-byte invalid ELF and checked the
validation error. Calls to `millis`/`delay_ms` before and after those operations
each satisfied the smoke-test window of 95–5,000 ms for a requested 100 ms.
The parent returned 77 (`0x4D`). These checks are encoded in the fixture; the
inner output is diverted to the parent rather than printed in the capture.
The two real host tables use the same callback functions, so the hardware
smoke is not a distinct-host identity or task-isolation proof.

The module was transferred in 32-byte chunks and verified by reading each
chunk and EOF back before execution; the invalid fixture was read back too.
The test created `/sd/tc42f` only after successful `mkdir`, then removed both
files and the empty directory (`MNM_CONTEXT_CLEAN`). FAT metadata may have
changed. A prior attempt timed out after reopening the serial port following
the native prompt, before confirming directory creation; it restored the
application. The successful attempt kept one serial connection and confirmed
that `/sd/tc42e` from the first attempt was absent.

The protected flash runner reported no fatal marker and verified the restored
factory-app write. The backed-up app SHA-256 was
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`;
the tested image was
`386761ccde44636cba5b7dbd2456bba864e9d4e90f2929e26bca97f41b840713`
and the probe module was
`9ff0931b09768398774f201cef8c39673aa5895d235351aa18272f7b0244972e`.
The image included earlier uncommitted work, including native instruction-cache
synchronization; no full-flash comparison was made.
The image, module, interactive driver, input/output, backup, report and restored
boot capture are in
`../../../maia-rtos-recovery/native-tab5-mnm-context-retry-20261123/`;
the failed attempt is in
`../../../maia-rtos-recovery/native-tab5-mnm-context-20261123/`.

Focused host validation:

```sh
cmake --build /tmp/maia-rtos-battery-host --target elf_loader_context_test elf_loader_context_no_tls_test shell_mnm_sleep_test tls_runtime_test newlib_syscalls_test -j4
ctest --test-dir /tmp/maia-rtos-battery-host --output-on-failure -R '^(elf_loader_context_test|elf_loader_context_no_tls_test|shell_mnm_sleep_test|tls_runtime_test|newlib_syscalls_test)$'
cmake --build /tmp/maia-rtos-battery-p4 --target maia_m0_image -j4
```

Result: 5/5 focused host tests and P4 build PASS. This does not qualify desktop,
graphics, multi-task loader execution, complete shell nesting or timing bounds.
An isolated snapshot of the staged source, with the three pinned submodules,
also passed these 5 host tests in Release and built a valid 847,184-byte P4
image with `s_active_host` in TLS. That isolated image was not flashed and
does not include the earlier uncommitted cache-synchronization work.

## Native MNM scheduler delay on Tab5 — 2026-11-23

In the current native worktree, the shell's MNM `sleep_ms` callback now uses
the existing Newlib `nanosleep` and MaiaRTOS task delay instead of returning
immediately. It returns Newlib failures to the MNM caller; zero duration
returns immediately without yielding. The native syscall requires a running
task for positive delays and rejects durations above `INT32_MAX` milliseconds.
The existing MaiaScript `delay_ms` wrapper returns `void` and cannot report
the callback's error.

On one warm-reset ESP32-P4 Tab5 boot over `/dev/cu.usbmodem1301`, the source
in `tests/hardware/maia_mnm_sleep_probe.ms` was entered in a newly created
directory, compiled on-device and executed as an MNM module. It requested
300 ms via `delay_ms` and returned the `millis()` difference: **309 ms**
(`0x135`). The creation was guarded by `mkdir` and `cd`; after the run,
removal of both files and the empty directory produced `SLEEP_PROBE_CLEAN`.
The card was not formatted, although FAT metadata may have changed.

The protected flash runner observed `msh>` with no fatal marker and restored
the previous factory app, with esptool's write hash verification and backed-up
app SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
The tested image SHA-256 was
`b2091a1d4388a3cdb076cebe170d2fe205e65830f403b2c1f31bd18f1c834105`.
The exact serial input, output, backup and report are in
`../../../maia-rtos-recovery/native-tab5-mnm-sleep-20261123/`. The command's
309 ms result and cleanup marker were checked separately from `probe_pass`;
no full-flash comparison was made. This single measurement is not a latency
or real-time bound and does not qualify desktop rendering.

The hardware image included earlier uncommitted work in the local worktree.
An independent snapshot of the staged commit, using the pinned local regex
and glob submodules, also passed the three host tests and built a valid P4
image (847,184 bytes); that separate image was not flashed.

Validation commands used the existing configured build directories:

```sh
cmake --build /tmp/maia-rtos-battery-host --target shell_mnm_sleep_test newlib_syscalls_test shell_expr_control_test -j4
ctest --test-dir /tmp/maia-rtos-battery-host --output-on-failure -R '^(shell_mnm_sleep_test|newlib_syscalls_test|shell_expr_control_test)$'
cmake --build /tmp/maia-rtos-battery-p4 --target maia_m0_image -j4
```

## Tab5 battery read and shared-I2C contention — 2026-11-23

- The read-only `battery` command obtains bus voltage from INA226 `0x41` and
  reads Input Status `0x0F` from expander `0x44`, pin P6. The output is
  `Monitor bus=7987 mV, IP2326 charge signal=LOW (raw; no SOC percent)`.
  This does not establish charge-signal polarity or battery state-of-charge.
- The first USB attempt encountered shared-bus `MAIA_I2C_BUSY`. Read-only
  shell commands now retry that result at most twice after yielding; they do
  not retry wire errors. A subsequent probe completed the battery read without
  a touch failure message. The touch service retains its last valid snapshot
  on BUSY without refreshing its timestamp; the existing age timeout still
  applies. The product suppresses BUSY as a failure even before the first
  valid report.
- Six focused host tests passed: touch input, battery, PI4IOE5V6408, INA226,
  RX8130 and I2C bus. The runner reported `probe_pass=true` and
  `restore_verified=true`, restoring the original application with SHA-256
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
  [Report](../../../maia-rtos-recovery/native-tab5-battery-touch-busy-final-20261123/report.json)
  and [probe log](../../../maia-rtos-recovery/native-tab5-battery-touch-busy-final-20261123/probe-boot-1.log).

## Native MaiaRTOS Tab5 power-off — 2026-11-23

- Native `poweroff` pulses PI4IOE5V6408 `0x44` P4 high/low three times (100 ms
  each) over the shared System I2C bus, ported from the M5Stack BSP
  `bsp_generate_poweroff_signal()`; backlight is turned off first.
- On hardware with USB attached, the shell printed `Powering off...`, the USB
  serial port disappeared and the owner confirmed the device was off. The
  automatic restore therefore failed as expected; after the owner powered the
  device on, the backed-up app was written manually ("Hash of data verified",
  SHA-256 `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`)
  and the captured boot reached the desktop.
  [Probe log](../../../maia-rtos-recovery/native-tab5-poweroff-20261123/probe-boot-1.log),
  [manual restore](../../../maia-rtos-recovery/native-tab5-poweroff-20261123/restore-manual.log),
  [restore boot](../../../maia-rtos-recovery/native-tab5-poweroff-20261123/restore-boot.log).

## Native BMI270 identification, initialization and sampling — 2026-11-23

- `imu id` reads only register `CHIP_ID` (`0x00`) at BMI270 I2C address `0x68`
  through the shared native System I2C bus. No sensor configuration or register
  writes occur in this probe; on Tab5 it returned `0x24`.
- Host test `tab5_bmi270_test` passed for expected ID `0x24`, unexpected ID,
  BUSY propagation, unchanged output on I2C failure and zero writes. On Tab5,
  `imu init` loaded the Bosch configuration and enabled accelerometer/gyroscope
  at 200 Hz (±4 g / ±1000 dps); `imu read` returned raw values
  `accel=(7593,5,-2916)`, `gyro=(3,-1,-3)` while stationary.
- Focused host tests `tab5_bmi270_test`, `i2c_bus_test` and `i2c_bitbang_test`
  passed; the `maia_m0_image` target built successfully.
- The runner reported `probe_pass=true` and `restore_verified=true`, restoring
  the original application with SHA-256
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
  [Report](../../../maia-rtos-recovery/native-tab5-bmi270-id-20261123/report.json)
  and [ID probe log](../../../maia-rtos-recovery/native-tab5-bmi270-id-20261123/probe-boot-1.log).
  [Init/read report](../../../maia-rtos-recovery/native-tab5-bmi270-clean-init-read-20261123/report.json)
  and [init/read log](../../../maia-rtos-recovery/native-tab5-bmi270-clean-init-read-20261123/probe-boot-1.log).

## Native MaiaRTOS Tab5 backlight control — 2026-11-23

- Over the USB shell, `backlight 50` reported success. Read-only inspection
  returned `LEDC_CH1_DUTY` `0x00001FF0` at `0x500D301C` and GPIO22 output
  selector `0x0000007F` at `0x500E05B0` (LEDC).
- `backlight 100` reported success and switched GPIO22 back to GPIO output:
  `GPIO_OUT` at `0x500E0004` was `0x00400003` (bit 22 high), and the GPIO22
  output selector was `0x00000100`. The check changes brightness only through
  the shell API; register inspection is read-only.
- Visual sweep `100→75→50→25→5→0→100` (5 s per step) was confirmed by the
  owner ("Brilho funcionou."), with the original application restored and
  verified. [Report](../../../maia-rtos-recovery/native-tab5-backlight-visual-20261123/report.json).
- The recovery runner reported `probe_pass=true` and `restore_verified=true`,
  restoring the original application with SHA-256
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
  [Report](../../../maia-rtos-recovery/native-tab5-backlight-20261123/report.json),
  [probe log](../../../maia-rtos-recovery/native-tab5-backlight-20261123/probe-boot-1.log),
  and [restore log](../../../maia-rtos-recovery/native-tab5-backlight-20261123/restore-boot.log).

## RX8130CE read-only calendar service — implementation, 2026-11-23

- Primary component reference: Epson [RX8130CE datasheet](https://download.epsondevice.com/td/pdf/app/RX8130CE_en.pdf),
  document ETM50E-10, revision 2026-01-30. Register table (p. 22) places
  seconds through year at `0x10..0x16` and VLF at bit 1 of `0x1D`.
  Clock/calendar behavior (pp. 25-26) specifies BCD seconds/minutes/hours,
  one-hot weekday, BCD day/month/year and a calendar snapshot during continuous
  access. A set VLF reports data loss; the implementation reports that state
  without clearing flags or writing any RTC register.
- The native service uses the managed Tab5 System I2C bus at 7-bit address
  `0x32`, shares ownership with touch, and propagates I2C failures. Host tests
  cover a valid leap-day reading, invalid BCD/calendar/weekday, VLF, NACK and
  timeout; failed reads leave the caller's output unchanged.
- The `rtc` shell command is read-only. P4 image build and targeted host test
  passed. On Tab5, USB command output was
  `rtc: invalid data (raw=47 45 16 0C 28 00 00 flags=25)`: month is zero and
  weekday `0x0C` has multiple bits set; VLF bit 1 is clear. The service
  correctly rejected the calendar and made no writes.
- The recovery runner reports `probe_pass=true` and `restore_verified=true`;
  it restored the original application with SHA-256
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
  [Report](../../../maia-rtos-recovery/native-rtc-rx8130-raw-20261123/report.json),
  [probe log](../../../maia-rtos-recovery/native-rtc-rx8130-raw-20261123/probe-boot-1.log),
  and [restore log](../../../maia-rtos-recovery/native-rtc-rx8130-raw-20261123/restore-boot.log).
- Valid physical date/time remains unverified. Do not initialize or alter the
  RTC until an authoritative time source/value is agreed.

## Native Tab5 A164 keyboard input — 2026-11-23

- The native product identified the physical A164 keyboard coprocessor as
  firmware `0x01` at address `0x6D` on ExtI2C GPIO0/1. During the USB recovery
  run, the physical keyboard entered `ls` and `pwd` at the native MaiaShell;
  their output appeared as `Directory of /` and `/`. This confirms character
  input and Enter delivery through the native console.
- A typed character sample also reached the shell. Arrow keys currently arrive
  as literal `[A`/`[B`/`[D` text and are not handled as line-edit/history
  controls; that remains a shell UX limitation. This run validates the A164,
  not the alternate TCA8418 path.
- The runner reported `probe_pass=true` and `restore_verified=true`, restoring
  the original application with SHA-256
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
  [Report](../../../maia-rtos-recovery/native-keyboard-a164-live-20261123/report.json),
  [probe log](../../../maia-rtos-recovery/native-keyboard-a164-live-20261123/probe-boot-1.log),
  and [restore log](../../../maia-rtos-recovery/native-keyboard-a164-live-20261123/restore-boot.log).

## PSRAM startup closure resident in internal SRAM — 2026-11-18

- `flash_product.ld` now collects the selected PSRAM boot, clock, supply,
  MPLL, array, mode, snapshot and window code plus constants into
  `.maia_psram_iram` at `0x4ff00000`. Linker assertions check the critical
  entry points, and `NOCROSSREFS_TO` rejects relocations from that section to
  flash text or rodata. Disassembly confirms that the remaining indirect
  cache calls target ROM (`0x4fc...`); direct calls stay in SRAM.
- The generated ELF has separate RX code and RW BSS/stack program headers
  (no RWX segment). `maia_f0_kernel_psram_probe_image` built a valid 79,600-byte
  app image with four records, including the 0x3710-byte IRAM segment at
  `0x4ff00000`; image SHA-256:
  `5f447bff77ccab515a29619a999515e1729cad86162f5518871af0eac688644a`.
- On Tab5 ESP32-P4 v1.3, the recovery runner wrote the image at `0x10000`,
  observed `MAIA_F0_SRAM_HEAP_PASS`, the read-only PSRAM snapshot
  (`result=0`, 54 controller samples), and `MAIA_NATIVE_PREEMPT_PASS`
  (250 timer IRQs, 251 switches). It restored and verified the factory app;
  the restore capture loaded the MaiaOS desktop and Notes app. The factory
  backup SHA-256 is
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
  Exact image, backup, report and logs are in
  `maia-rtos-recovery/native-f0-psram-iram-20261118-084629/`.
- Host CTest passed 78/78. This warm-reset probe verifies image acceptance and
  loading of the SRAM segment; it does not call the PSRAM startup routines,
  validate cold startup, access the PSRAM array, or qualify a product linker
  or PSRAM heap. Those remain open.

## Flash-XIP product-layout baseline — 2026-11-18

Primary references: Espressif ESP-IDF v5.4.2 second-stage
[boot flow](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-guides/startup.html#second-stage-bootloader),
[application image format](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-reference/system/app_image_format.html#application-description),
and pinned source revision `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:
[P4 memory linker template](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_system/ld/esp32p4/memory.ld.in),
[P4 section template](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_system/ld/esp32p4/sections.ld.in),
the [P4 SoC memory map](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/soc.h),
and [shared D/I image loader](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/bootloader_support/src/bootloader_utility.c).

- `boards/m5stack/tab5/flash_product.ld` applies the pinned IROM/DROM image
  structure to the scheduler/heap diagnostic. The descriptor is first in DROM;
  initialized image data, BSS and boot stack stay in the validated low-SRAM
  window. Linker assertions bound the entry, descriptor and stack.
- The script exports `__maia_psram_window_start/end` as absolute symbols for the
  previously qualified CPU window `0x4b800000..0x4ba00000`
  ([native PSRAM window audit](../audits/native-psram-window.md)). These symbols
  create no section or load segment. The heap is not registered, and the linker
  does not place code/data in PSRAM; cold startup and full-product IRAM closure
  remain unqualified.
- Standalone GCC 14.2 and esptool 4.12.0 produced an 80,080-byte app image.
  `f0_flash_image_test.py --sram-window 0x4FF00000:0x4FF28000` passed; image
  inspection found two mapped records plus the reserved padding record, entry
  `0x40000020`, descriptor `0x40010020`, valid checksum/hash. ELF symbols:
  BSS `0x4ff00000..0x4ff056c0`, stack
  `0x4ff056c0..0x4ff096c0`, PSRAM window symbols as above. Image SHA-256:
  `3aa5179dd6a5c8e95daee485571353edf7cfa2f70bd10b6bd22ee34709deeff4`.
- On Tab5 ESP32-P4 v1.3, the existing recovery runner backed up and validated
  the factory app, wrote the probe at `0x10000`, captured
  `MAIA_F0_SRAM_HEAP_PASS` and `MAIA_NATIVE_PREEMPT_PASS` (250 timer IRQs,
  251 switches), then restored the app with esptool hash verification. The
  restore capture shows the MaiaOS desktop module and wallpaper loaded.
  This is a warm RTS reset test; it does not qualify cold PSRAM startup or
  PSRAM array access. Exact image (`flash-probe.bin`), report, backup and
  boot captures are in `maia-rtos-recovery/native-f0-product-linker-20261118/`.

## ESP-IDF second-stage image and offline native probe — 2026-11-17

Primary references: Espressif ESP-IDF Programming Guide
[second-stage boot flow](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-guides/startup.html#second-stage-bootloader)
and [application image format](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-reference/system/app_image_format.html#application-description),
plus the repository's pinned ESP-IDF v5.4.2 source revision
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`.

- The P4 source linker template selects `0x40000020` for flash-mapped IROM
  and DROM when PSRAM code/rodata mapping is disabled. The `0x20` offset
  satisfies the 64 KiB flash-MMU page alignment with the image header and
  segment header. Its `.flash_rodata_dummy` reserves the DROM page after the
  text section, then puts `esp_app_desc_t` first in `.flash.appdesc`.
  References: [memory.ld.in](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_system/ld/esp32p4/memory.ld.in)
  and [sections.ld.in](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_system/ld/esp32p4/sections.ld.in).
- P4 uses a shared D/I virtual address window (`0x40000000..0x44000000`).
  The pinned bootloader expects exactly two flash/PSRAM mapped segments in
  that window. It also treats addresses below `0x10000000` as reserved
  non-loaded blocks, including the image `PADDING` block.
  References: [P4 memory map](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/soc.h),
  [shared D/I loader](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/bootloader_support/src/bootloader_utility.c),
  and [reserved-segment handling](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/bootloader_support/src/esp_image_format.c).
- The checked-in Tab5 build configuration still maps code and constants to
  PSRAM (`CONFIG_SPIRAM_FETCH_INSTRUCTIONS=y`,
  `CONFIG_SPIRAM_RODATA=y`, 64 KiB MMU pages). The local
  `platforms/tab5/build/m5stack_tab5.bin` artifact is 2,092,288 bytes with
  eight segments, entry `0x4ff003fe`, and its mapped code/descriptor use
  `0x48000020` / `0x48130020`. This is a local build artifact, not a readback
  of the connected device and not a direct-flash native image.
- Project configuration, not verified physical flash contents:
  `partitions.csv` places the factory app at `0x10000` with size `10M`;
  `sdkconfig` sets the partition table offset to `0x8000`, DIO/80 MHz/16 MB
  image settings, and the 64 KiB MMU page size.
- The new C-only `maia_f0_flash_probe_image` target was built offline with
  GCC `esp-14.2.0_20241119` and esptool 4.12.0. Its 66,192-byte candidate has
  three image records: the descriptor starts at DROM `0x40010020`, a
  reserved address-zero `PADDING` record, and the second shared D/I mapping
  at `0x40000020`, which contains the entry point. `image_info` reports a
  valid checksum and validation hash
  `6bcf9958a74e79a23e173e30d99fb55e127b26f496cdb23a01ade08554895029`
  (file SHA-256 `c29163763556f391edd59277af7992c27d84b2db09cb1d47f1f90b5c8e3ca3a6`).
  The header carries chip ID 18 and an unrestricted chip-revision range.
  The Python validator checks the header, descriptor, entry point, exactly
  two mapped segments, and 64 KiB MMU alignment; malformed descriptor,
  extra-mapping and alignment cases are rejected. The probe ELF has no
  undefined symbols and no FreeRTOS/ESP-IDF runtime symbols.
- Use esptool's default section-based `elf2image` packaging. `--use_segments`
  is unsuitable for this linker layout because the ELF's NOLOAD DROM
  reservation produces overlapping program-header address ranges; the
  generated image's two mapped records follow the pinned bootloader's shared
  D/I format.

Device run, 2026-11-17 (`tests/hardware/run_f0_flash_probe.py`, ESP32-P4
v1.3, `/dev/cu.usbmodem1301`):

- Readback of the factory app at `0x10000` validated (2,093,696 bytes) and is
  byte-identical to the preserved `bf044d3` `m5stack_tab5.bin`.
- After writing the probe with esptool verification and an RTS reset, the
  installed bootloader (`ESP-IDF v5.4.2-dirty`, built Nov 10 2026) printed the
  partition table (nvs `0x9000`, phy_init `0xf000`, factory `0x10000` size
  `0xa00000`, then two data partitions), mapped `paddr 0x10020 → 0x40010020`
  and `paddr 0x20020 → 0x40000020`, did not load the padding record, reported
  "Loaded app from partition at offset 0x10000", and the probe printed
  `MAIA_F0_FLASH_PROBE` over USB Serial/JTAG.
- The probe clears `USB_UART_CHIP_RST_DIS` (set by `maia_soc_early_init`), so
  host RTS reset still works from a flash-resident native image.
- The backup was written back and verified; the restored firmware reached the
  desktop and `notes.mobj`, confirmed by `maia.py info`.
- Logs, readback and report: `maia-rtos-recovery/native-f0-flash-probe-20261117-215248/`.

This proves second-stage bootloader acceptance and execution of a minimal
native image from flash (warm, after RTS reset). It does not cover the product
image, PSRAM, cold power-on or repeated boots; those remain under UQ-016.

## Flash-mapped kernel, tasks and PSRAM-controller snapshot — 2026-11-17

The `maia_f0_kernel_psram_probe_image` target reuses the native scheduler
preemption workload and adds the existing read-only PSRAM snapshot. Its
83,376-byte image has entry and executable code at IROM `0x40000020`,
app descriptor/constants at DROM `0x40010020`, and exactly two mapped D/I
segments. The kernel scheduler, SYSTIMER path, both task workers and entry
resolve into IROM. BSS ends at `0x4ff04650`; the 16 KiB boot stack ends at
`0x4ff08650`, within the linker-bounded SRAM range
`0x4ff00000..0x4ff28000`. ELF audit found no undefined symbols or FreeRTOS/
ESP-IDF runtime references.

Device run (ESP32-P4 v1.3, `/dev/cu.usbmodem1301`): after a verified factory-app
backup, the image was written at `0x10000` and booted by the installed second
stage. UART reported `MAIA_F0_PSRAM_STATE result=0 roots_valid=1
controller_samples=54`; this means the 27 whitelisted controller registers
matched across two read-only passes and the nine roots remained stable. It did
not initialize PSRAM, test its array, establish usable capacity or prove cold
startup. The kernel then recorded 250 real SYSTIMER IRQs/ticks, 251 task
switches, progress by both workers, zero register errors, zero IRQ-stack errors
and `MAIA_NATIVE_PREEMPT_PASS`.

The original factory app was rewritten and verified. Its SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`
matches the previous recovery image; `maia.py info` and serial output confirmed
desktop, wallpaper and `notes.mobj` restored. Full capture/readback/report:
`maia-rtos-recovery/native-f0-kernel-psram-flash-20261117-220134/`; the exact
flashed artifact is saved there as `flash-probe.bin`.

This demonstrates flash XIP for the native scheduler and task code with working
SRAM state/stack on this warm boot. Product linker/heaps, native cold PSRAM
initialization and PSRAM memory allocation/access remain unverified.

## Multi-region SRAM heap under flash XIP — 2026-11-18

The flash probe now links `kernel/kmem.c` and tests the public region allocator
against two disjoint 2 KiB pools in SRAM. The 85,008-byte image SHA-256 is
`72198537e34c141beb6c934686381d0ac5da3c9e6d0fa44d317a9ca79553e908`.
Both pools are in BSS at `0x4ff046c0..0x4ff056c0`; the 16 KiB boot stack ends at
`0x4ff096c0`, within the existing 160 KiB linker bound.

On the Tab5 the probe reported `MAIA_F0_SRAM_HEAP_PASS`: a 64-byte-aligned
allocation succeeded in pool 1, an 1,800-byte allocation was served from pool 2,
both frees succeeded, and aggregate `used_bytes` returned to zero with
`free_bytes == total_bytes`. It then passed the read-only PSRAM-controller
snapshot and the existing 250-tick/two-worker scheduler test. The app backup was
verified and restored; desktop and `notes.mobj` returned. Exact binary and logs:
`maia-rtos-recovery/native-f0-multiheap-flash-20261118-080619/`.

This qualifies allocator use over multiple SRAM regions only. It does not add a
PSRAM pool, prove allocator concurrency, or establish PSRAM initialization,
capacity or array access. `maia_mem_add_region` must receive only an exclusive,
already initialized and mapped pool; caller serialization remains required.

## Notification-driven native TX worker — 2026-11-11

Reuses the verified queue, notification/ECALL, SYSTIMER/CLIC and USB TX paths;
no new MMIO, pinout or display initialization fact. Rev-v1.3 RAM evidence:
80 intact payload records/3840 bytes, 302 real IRQs, stable TX polls/run_ticks
through 20 initial and 21 final quiet IRQ observations, then one actual record
published in IRQ context wakes the owner and is captured by the host. BUSY
uses finite retries (240 timeouts); empty queue uses an indefinite kernel wait.
This does not establish a USB TX-ready IRQ, SMP, power/WCET bounds, reconnect
or panel/DMA correctness. [Exact logs and limits](../audits/native-tx-worker-notifications.md).

## Native task notification waits — 2026-11-11

Uses the already verified MIE exclusion, M-mode ECALL/context restore and
SYSTIMER/CLIC IRQ. No new MMIO, route, pinout, clock or display configuration.
Kernel stores completion in TCB and writes a0 only after validating the saved
frame. Rev-v1.3 RAM workload passes 40 rounds/282 waits/80 timeouts, 750 real IRQs,
520 blocked-state observations and 750 rejected IRQ-context waits; idle runs.
FP/mixed lifecycle and exact-PC illegal16 regressions pass. This does not prove
SMP, arbitrary driver IRQ routing, power/WCET bounds or panel/DMA correctness.
[Raw logs, API, race ordering and limits](../audits/native-task-notifications.md).

## Native serial output ownership — 2026-11-11

Reuses EP1/EP1_CONF WR_DONE[0] and SERIAL_IN_EP_DATA_FREE[1], plus UART0
STATUS TXFIFO_CNT[23:16]. Primary SDK register definitions, commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` (v5.4.2 checkout), sections
[USB_SERIAL_JTAG_EP1_CONF_REG / WR_DONE / SERIAL_IN_EP_DATA_FREE](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/usb_serial_jtag_reg.h)
and [UART_STATUS_REG / TXFIFO_CNT](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/uart_reg.h).
These definitions were read locally, not compiled/imported. No new MMIO or pins.
32-byte USB watermark is software policy; UART threshold 120 is inherited
conservative policy. Actual rev-v1.3 RAM USB capture: 80 intact ordered records,
3840 bytes, 260 real IRQs, 344 FULL and 239 BUSY events with correct resume.
This does not establish physical reconnect/UART, SMP or a time bound.
[Exact captures, reproduction and limitations](../audits/native-serial-output.md).

## Native serial input ownership — 2026-11-11

RX reuses existing USB-Serial/JTAG EP1 availability bit2 and data register,
then UART0 STATUS RXFIFO_CNT[7:0] and FIFO. Primary register definitions at SDK
commit `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` (v5.4.2 checkout), sections
[USB_SERIAL_JTAG_EP1_CONF_REG / SERIAL_OUT_EP_DATA_AVAIL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/usb_serial_jtag_reg.h)
and [UART_STATUS_REG / RXFIFO_CNT](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/uart_reg.h).
Local reference headers match this commit (no diff); read only, no SDK header
or runtime linked. Software ownership/batching does not introduce new MMIO.
Actual rev-v1.3 USB RAM test: all 256 byte values in order, full queue observed,
317 real IRQs and zero errors. UART physical wiring/traffic is not revalidated;
its fallback is host-model evidence only. [Capture, initial test failure and
limits](../audits/native-serial-input.md).

## Native queue task/IRQ exclusion — 2026-11-11

Uses the existing single-core CSR MIE exclusion and validated SYSTIMER/CLIC path;
no new MMIO, pin, clock or boot claim. References remain the local-exclusion and
timer/context sections below. The RAM-only diagnostic checks real C IRQ-stack
SP and MIE preservation while a timer-IRQ producer shares a copying queue with
task producers/consumer. Final result: 253 real IRQs, 688 intact ordered packets,
zero errors. This is not an SMP, blocking-wait, driver-equivalence or latency
bound claim. [Exact workload, initial failure, logs and recovery](../audits/native-driver-queues.md).

## Native stack entry protection — 2026-11-11

Primary references: [ESP32-P4 rev-v1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
register 1.11 p.72 (`mscratch`, CSR 0x340 R/W), and
[RISC-V Privileged ISA 20240411, Machine Scratch Register](https://docs.riscv.org/reference/isa/v20240411/_attachments/riscv-privileged.pdf),
section 3.1.13. Scratch CSR swapping provides initial working registers without
writing through the interrupted SP. All metadata layouts, guard patterns and
IRQ/emergency buffers here are project software, not invented MMIO addresses.
Boot bounds come from the existing linker __stack_bottom/__stack_top symbols.
Single-core/non-nested only; this does not establish SMP or memory isolation.
Actual rev-v1.3 RAM evidence: seven bad-stack injections stop on the emergency
stack with exact ELF cause/PC and expected guard/SP; normal lifecycle/integer
and idle handlers independently observe their C SP within the IRQ buffer.
See [stack protection report and raw logs](../audits/native-stack-protection.md)
for final counts, negative tests, recovery and limitations.


## Internal idle and WFI diagnostic — 2026-11-11

Uses the already verified timer/CLIC and M-mode context-return paths. Standard
instruction reference: [RISC-V privileged ISA,20240411, Machine ISA: Wait for Interrupt](https://docs.riscv.org/reference/isa/v20240411/_attachments/riscv-privileged.pdf).
The private idle context keeps MIE enabled, FS off, and uses WFI rather than
reviving blocked/exited applications. The architecture wait hook is instrumented
only in the RAM diagnostic to inspect coherent idle/task/IRQ snapshots before
the same WFI instruction. No interrupt, tick or application wake is synthesized.
No power-saving/latency claim is inferred: WFI may return spuriously.


## Scheduler local exclusion and transactional task traps — 2026-11-11

Primary ISA reference: [Zicsr v2.0, CSR Instructions](https://docs.riscv.org/reference/isa/unpriv/zicsr.html)
(20260120 ISA library), atomic CSRRC[I] read/clear and CSRRS[I] set semantics;
rev-v1.3 TRM mstatus p.68–69 defines MIE bit3. Local critical sections retain
only the old MIE bit and restore it, including nested already-disabled callers.
Compiler memory clobbers bracket accesses. This is single-core exclusion,
not an SMP/DMA/cache synchronization claim. No new MMIO register is introduced.
Dedicated project-owned delay/exit ECALL labels let dispatch commit the state
transition after the trap frame is saved, while IRQs are disabled. The raw
cause/return validation and real timer accounting remain unchanged.


## Controlled fatal traps — 2026-11-11

Primary references: [RISC-V unprivileged ISA, 20240411, C v2.0 sections26.5.4/26.5.6](https://docs.riscv.org/reference/isa/v20240411/_attachments/riscv-unprivileged.pdf)
define all-zero16-bit illegal and C.EBREAK; the already cited rev-v1.3 TRM
register1.13 p.73 defines illegal instruction cause2 and breakpoint cause3,
with mepc/mtval retaining trap details. The controlled32-bit fault writes the
read-only cycle CSR (`CSRRW x0,cycle,x0`), addressed in the ISA CSR-access rules
and the rev-v1.3 CSR summary (p.65: cycle 0xC00 is read-only). Fault-PC labels are deliberately halfword aligned.
The USB fatal sink uses only previously verified USB-Serial/JTAG registers,
with bounded polling/byte count and no display callbacks, heap or scheduler.
Old behavior on hardware prints FAULT_RESUMED after illegal16; the fixed path
reports raw cause/original halfword PC/value/sp and halts, verified by exact
PC comparison to the matching ELF symbol. See [fatal-trap report](../audits/native-fatal-traps.md)
for actual captures, expected failures, hardware scope and reset recovery.


## Mixed FS-state and scheduler diagnostic — 2026-11-11

Uses the previously cited rev-v1.3 mstatus.FS/CLIC references and verified
32-bit M-mode ecall path. No new MMIO or peripheral protocol. Test-only profile
`MAIA_NATIVE_PROBE_MIXED` combines two FP sentinel tasks with a higher-priority
integer task that clears FS, yields while ready, then sleeps two real ticks.
Expected invariants: no lower-priority work while high task yields ready; no
early wake; both lower tasks progress while it sleeps; FS remains off across
switch/ISR paths. Live FP/integer sentinels also span explicit ecalls. Real
IRQ/tick counters are still required; ecalls are reported separately.
Before correction, the probe reached250IRQs but failed with active MIL=0x3f
while a normal task was executing (`mcause=0x0800000b`). The previously cited
fresh-frame INT behavior also applies to saved ecall frames restored from a
timer ISR. The saved return frame now sets INT=1 while preserving saved MPIL;
the raw cause is passed separately to dispatch/statistics before normalization.
No synthetic IRQ/tick is recorded. Final hardware results are required below.


## RV32F context experiment — 2026-11-11

Primary source: [ESP32-P4 Chip Revision v1.3 TRM, Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
section 1.6.3, p.95: 32 single-precision FP registers and manual enablement via
mstatus.FS; FP instructions/CSRs fault with FS off. Register 1.38, p.82: fcsr
contains rounding mode and exception flags. mstatus FS descriptions are on p.68.
The opt-in RAM probe compiles RV32IMAFC/ILP32F (single-precision hardware ABI),
seeds all FP registers with task-distinct bit patterns and fcsr with distinct
legal rounding modes/flags, and checks them before calling C. A separate C
translation unit exercises actual float arguments/result (`2.0f+3.0f`).
Before adding FP context, the hardware test failed after three IRQs with f0
cross-task contamination (`reg=256 expected=1073742080 actual=1056964864`).
The independently authored 272-byte frame preserves all 32 registers plus fcsr.
Two final RV32F runs and an integer-profile regression each pass 250 real IRQs
and 251 switches without register errors. A RAM-only mutant that restores fcsr
as zero fails at `reg=512 expected=82 actual=0`; zero-initialized FP state and
ILP32F argument/result calls are checked too. No double-precision claim.
See [FP context report](../audits/native-fp-context.md) for exact scope and logs.


## Integer context register probe — 2026-11-11

Primary ABI reference: [RISC-V psABI](https://riscv-non-isa.github.io/riscv-elf-psabi-doc/),
version 2.0-rc0, November 04, 2026, sections 1.1 and 2.1: integer register
roles, gp/tp reserved for global/thread pointers, and 16-byte stack alignment.
The diagnostic is an explicitly authored assembly workload that seeds x1,
x4 and x5–x31 with distinct task/register patterns, holds them through a busy
window, then checks them before calling C. x2/sp and x3/gp retain their normal
values and are checked against stack snapshots. On the unchanged trap code the hardware probe failed after three IRQs:
`task=1 reg=4 expected=2986344452 actual=2701131784` (tp). This confirms
cross-task register contamination; interruptions can also occur between the two
instructions that materialize a sentinel constant. Fix under test: use the
existing reserved frame word at byte 124 for tp, zero for a fresh context.
It does not allocate TLS or claim a C/C++ TLS runtime. No new peripheral/MMIO facts are introduced.


## ESP32-P4 fresh-context return investigation — 2026-11-11

Primary source: [ESP32-P4 Chip Revision v1.3 TRM, Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
register 1.13 (`mcause`, p.73), register 1.17 (`mintstatus`, p.75), section
1.9.2.5 (p.113), register 1.84 (p.116). INT bit 31 selects interrupt versus
exception; MPIL is bits 23:16. Interrupt delivery requires a level greater than
both the active MIL and configured threshold. The revision-specific manual,
not the newer generic ESP32-P4 document, matches CSR mintstatus at 0x346.

Measured failing return: `mcause=0x08000000`, `mintstatus=0x3f000000`,
`last_mcause=0xb8000010`, `mstatus=0x88`, enabled pending source. The incoming
fresh task frame uses `mcause=0x38000000` (INT=0); active MIL stays at the
just-serviced level. Controlled experiment: initialize the fresh frame for an
interrupt return (INT=1), keeping MPIL=0 and all other state unchanged. Four corrected runs passed: each delivered 250 real IRQs/ticks and 251 switches,
with both busy tasks progressing and no ecalls/synchronous exceptions. Active
MIL is zero after return (`mintstatus=0`). The final probe also requires enabled
MIE, MIL zero and at least 250 switches. Returning to the former frame reproduces
the one-IRQ failure; the host frame regression rejects it with exit 52.
This silicon-specific behavior is measured, not inferred as a universal CLIC
rule. MIL is read-only and never written. No timer events are fabricated.
Raw captures and exact limits: [CLIC return report](../audits/native-clic-return.md).
Downloaded revision-specific PDF SHA-256:
`ae1fa2a411776760e03329adc3f9a7c13e98c440168cab208f06e3b2d5818830`.

## ESP32-P4 CLIC status diagnostic — 2026-11-11

Primary reference: ESP-IDF v5.4.2, commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`,
[interrupt_clic.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/riscv/include/esp_private/interrupt_clic.h),
ESP32P4 conditional and MINTSTATUS definitions (lines 49–86): this chip uses
CSR **0x346** for mintstatus, with MIL at bits 31:24; interrupt threshold is
memory-mapped. Read-only diagnostic accesses use that documented CSR, not
the different standard-CLIC CSR address.
[Vectors return](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/riscv/vectors.S)
lines 398–405 explain preserving mcause to restore the prior interrupt level
on mret. These sources are consulted, never included/copied/linked.


## Native SYSTIMER base address verification

Primary reference: Espressif ESP-IDF v5.4.2, commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`,
[ESP32-P4 peripheral base map](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/reg_base.h),
PERIPHERAL1 section, `DR_REG_HPPERIPH1_BASE = 0x500c0000` and
`DR_REG_SYSTIMER_BASE = DR_REG_HPPERIPH1_BASE + 0x22000` (line 115).
Thus SYSTIMER is at **0x500e2000**. The former native address `0x500c6000`
is I2S0, not SYSTIMER. Before correcting the base, the on-device RAM probe
read `conf=0 raw=0 ena=0 st=0`, despite initialization, while the interrupt
matrix route read back 16. Capture: `/tmp/maia-native-probe-diagnostics.log`.
The register contract test previously repeated the wrong address; it is being
corrected against the independent vendor map. No vendor runtime is linked.

## Native SYSTIMER routing verification

Primary reference: Espressif ESP-IDF v5.4.2, commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`,
[interrupt source enum](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/interrupts.h)
(`ETS_SYSTIMER_TARGET0_INTR_SOURCE`) and
[core-0 interrupt matrix register](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/interrupt_core0_reg.h)
(`INTERRUPT_CORE0_SYSTIMER_TARGET0_INT_MAP_REG`, offset `0xd4`). Both reference
files in the local SDK are unchanged from that commit. Compiling the source enum
with the host C compiler prints **53**. It is used only as specification evidence,
not included or linked into the native firmware.

The existing native constants used source 54 and offset `0xd8`. The new isolated
RAM diagnostic, before correcting them, observed `irq=0 ticks=0 ecall=0 sync=0
switches=1 workA=11962368 workB=0` and `MAIA_NATIVE_PREEMPT_FAIL`. It ran two
busy tasks without cooperative yields or synthetic ticks and reset back to the
preserved flash application. No flash write was performed. Raw evidence:
`/tmp/maia-native-probe-before-route.log`. Post-correction evidence is recorded
separately; these negative results are not a preemption claim.

## CST ownership fix — 2026-11-10 (limited device test)

On the one-core experimental firmware, the same small source that previously
faulted in the on-device compiler now compiles, validates and executes with
exit code 32. SciCalc/Alarm and background-job tests also pass over serial.
The change removes temporary CST shared control blocks; it does not change
hardware registers, claim native scheduling/SMP or remove the full foreign
runtime. Original application restored after testing. Exact artifacts, primary
runtime implementation references and limits:
[CST ownership report](../audits/maiascript-cst-ownership-2026-11-10.md).

## Runtime migration experiment — 2026-11-10 (limited scope)

The opt-in one-core legacy-driver profile was built and temporarily installed
on the connected Tab5 by app-only flashing. Serial tests passed scientific
calculations, timer fire/cancel, desktop return and shell background jobs. The
generated configuration selects `CONFIG_FREERTOS_UNICORE=y`; the ELF omits the
secondary-core startup and cross-core tick/IPC functions. It still uses FreeRTOS.
Primary software reference: [ESP-IDF v5.4.2, Single-Core Mode](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-reference/system/freertos_idf.html#single-core-mode).

The on-device compiler test failed, including a mutex assertion reproduced on
the original preserved firmware. The experiment was not promoted. The original
application was restored and verified (`2+3*4=14`, return to desktop); no
bootloader/partition-table replacement or new MMIO/pinout implementation occurred.
Exact image hashes, scope and raw-log locations:
[stage-one report](../audits/tab5-singlecore-stage-1-2026-11-10.md).
Historical hardware claims below do not certify this migration complete.

Record each validated hardware claim as:

| ID | Claim | Primary source URL | Revision / section / page | Confirmed on board? | Date |
|---|---|---|---|---|---|
| HW-001 | ESP32-P4 contains a dual-core High-Performance (HP) 32-bit RISC-V CPU system (up to 400 MHz, `RV32IMAFC` with `Zicsr`/`Zifencei`, `misa = 0x40901125`) and a single-core Low-Power (LP) RISC-V CPU, without a conventional virtual-memory MMU for POSIX process isolation | https://www.espressif.com/en/products/socs/esp32-p4 | ESP32-P4 Series Datasheet v0.7 / v1.2, Section "Features" & "CPU and Memory"; verified CSR `misa=0x40901125` and `mhartid=0x00000000` on Tab5 | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-002 | ESP32-P4 HP memory subsystem includes 768 KB internal HP L2MEM (`0x4FF00000..0x4FFC0000`, ROM reserved window `0x4FF296B8..0x4FF40000`), 128 KB HP ROM (`0x4FC00000`), 32 KB LP SRAM (`0x50108000`), and external Flash/PSRAM mapped via Cache/MMU | https://www.espressif.com/en/support/documents/technical-documents | ESP32-P4 TRM v0.7, Chapter "System and Memory" & `esp-idf/components/soc/esp32p4/include/soc/soc.h`; verified bare-metal execution in `L2MEM_LOW` (`0x4FF00000`) | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-003 | ESP32-P4 interrupt architecture uses the RISC-V Core-Local Interrupt Controller (CLIC, base `0x20800000`, `MTVEC` mode `3`, 64-byte aligned `_maia_trap_entry`) where `mret` restores `MPP[29:28]`, `MPIE[27]`, and `MPIL[23:16]` from `mcause` (`0x38000000`) | https://www.espressif.com/en/support/documents/technical-documents | ESP32-P4 TRM v0.7, Chapter "High-Performance CPU" & `esp-idf/components/riscv/vectors.S:398-403`; verified `ebreak` (`mcause=0x30000003`) and `ecall` (`mcause=0x3800000B`) context switching on Tab5 | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-004 | ESP32-P4 boot ROM evaluates strapping pins (GPIO34–GPIO38) on reset and provides direct `USB-Serial/JTAG` download/console on `/dev/cu.usbmodem1301` (`0x500D2000`, `EP1_REG=0x500D2000`, `EP1_CONF_REG=0x500D2004`) and `UART0` (`0x500CA000`) | https://www.espressif.com/en/products/socs/esp32-p4 | `esp-idf/components/soc/esp32p4/register/soc/usb_serial_jtag_reg.h` & `uart_reg.h`; verified interactive TX/RX over `/dev/cu.usbmodem1301` | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-005 | M5Stack Tab5 integrates an ESP32-P4 (`rev v1.3`, `ESP32-P4NRW32`, 32 MB PSRAM) with 16 MB XMC SPI Flash (`XM25QH128C`, Flash ID `0x184020`), USB-C routed to ESP32-P4 internal USB-Serial/JTAG (`303a:1001`), and ROM watchdogs (`TIMG0_WDT=0x500C2048`, `TIMG1_WDT=0x500C3048`, `LP_WDT=0x50116000`, `SWD=0x5011601C`, key `0x50D83AA1`) | https://docs.m5stack.com/en/core/Tab5 | M5Stack Tab5 Docs & `esp-idf/components/soc/esp32p4/register/soc/lp_wdt_reg.h`; verified continuous bare-metal uptime without watchdog reset | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-006 | M5Stack Tab5 physical keyboard accessory (`A164`) communicates over external I2C (`GPIO0`/`GPIO1`) at 7-bit slave address `0x6D` with STM32F030 coprocessor. Mode `0x02` (String/Character Mode in reg `0x10`) returns formatted ASCII string events via register `0x40` (queue length) and register `0x50` (modifier + characters), cleared via register `0x01` (`INT_STA = 0x00`) and register `0x02` (`EVENT_NUM = 0x00`). Named keys (`"backspace"`, `"enter"`, `"space"`, `"tab"`, `"esc"`, `"DEL"`) translated to control characters | https://docs.m5stack.com/en/core/Tab5 | `m5stack/M5Tab5-Keyboard-UserDemo` components/m5_tab5_keyboard_component/src/m5_tab5_keyboard.cpp; verified live key captures from physical keyboard into MaiaShell | YES (`A164 @ 0x6D`) | 2026-11-04 |
| HW-007 | ESP32-P4 clean-room bare-metal MIPI-DSI subsystem drives the M5Stack Tab5 5.0" panel (`720x1280` RGB565) using internal L2MEM SRAM (`0x4FF18000..0x4FF9F000`) without PSRAM dependency. Panel detection distinguishes ST7121 (touch `0x55` firmware `1`) which requires full 28-command DCS vendor init sequence (`0x60`, `0x78`, `0x79`, `0xA4`, `0xB7`, `0xB0`, `0xBF`, `0xA5`, `0xD7`, `0x90`, `0xA3`, `0xA6`, `0xA7`, `0xAC`, `0xAD`, `0xAE`, `0xB2`, `0xE8`, `0x75`, `0xE7`, `0xE1`, `0xEA`, `0xC8`, `0xC9`, `0x60`, `0x11`, `0x29`, `0x35`) and ST7121 DPI timing (`802x1524`, HSA=2, HBP=40, HFP=40, VSA=20, VBP=24, VFP=200). Backlight driven via LEDC PWM (1000 Hz, 10-bit) on GPIO22 | ESP32-P4 TRM Chapter "MIPI-DSI Host & Bridge", "DW_GDMA", "PMU", "LEDC" & `espressif/esp-iot-solution` `esp_lcd_st7121.c` | Verified continuous D-PHY lock, ST7121 vendor DCS sequencing, LEDC PWM backlight, and circular multi-block DMA frame streaming on M5Stack Tab5 hardware | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-008 | ESP32-P4 standalone Flash boot requirements: ESP-IDF 2nd-stage bootloader requires 256-byte `esp_app_desc_t` header (`magic_word = 0xABCD5432`, `min_efuse_blk_rev_full = 0`) at Segment 0 (`0x4FF00000`) and strictly asserts `rom_index == 2` in `unpack_load_app` (requiring two Flash MMU-mapped segments in `SOC_DROM_LOW..SOC_DROM_HIGH`, e.g. `0x40000000` and `0x40010000`). Once verified, bootloader loads `L2MEM_LOW` into SRAM and jumps directly to `0x4FF00100` (`_start`) | `esp-idf/components/bootloader_support/src/bootloader_utility.c:750-785` | Verified standalone cold flash boot from SPI Flash partition `0x10000` surviving USB disconnect and power cycles | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-009 | M5Stack Tab5 MicroSD Card slot connects to ESP32-P4 SDMMC host via dedicated 4-bit bus: `CLK=GPIO43`, `CMD=GPIO44`, `D0=GPIO39`, `D1=GPIO40`, `D2=GPIO41`, `D3=GPIO42` (card detection `DET=NC`) | https://github.com/m5stack/M5Tab5-Keyboard-UserDemo/blob/main/components/m5_tab5_component/include/m5tab5_pinmap.h | Verified from official `m5tab5_pinmap.h` and factory app symbol analysis | YES (Pinmap verified) | 2026-11-04 |
| HW-010 | M5Stack Tab5 power sequencing & hardware reset: PI4IOE 0x43 controls `LCD_RST` (Pin 4), `TP_RST` (Pin 5), `CAM_RST` (Pin 6), and `EXT5V_EN` (Pin 2). Requires `IO_DIR=0x7F`, `OUT_HI_Z=0x00`, reset pulse `OUT_STATE=0x44` (reset low, EXT5V high), followed by `OUT_STATE=0x74` (all resets released high, active push-pull). When `TP_RST=1` and `LCD_RST=1`, ST7121 panel and touch controller respond on `SysI2C` at address `0x55` (`fw=1`). LEDC LCD backlight on `GPIO22` operates at 5000 Hz, 12-bit PWM (`duty=4095`) via LEDC Channel 1 (`SIG_LEDC_LS_OUT1_IDX = 127`, timer divider `0x01F4`) | Tab5 factory firmware binary analysis & M5Stack Tab5 BSP | Verified cold boot detection: `[PANEL] Detected ST7121 (touch 0x55 fw=1)` and LEDC channel 1 backlight driving on physical hardware | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |

---

## Live Hardware Verification Log (M5Stack Tab5 — ESP32-P4 rev v1.3)

- **Date:** `2026-11-04`
- **Target Board:** M5Stack Tab5 (`ESP32-P4NRW32`, Chip Revision `v1.3`, MAC `30:ed:a0:e3:6c:2b`, 16 MB SPI Flash `0x184020`, 32 MB Octal PSRAM)
- **Serial Interface:** `/dev/cu.usbmodem1301` (ESP32-P4 built-in USB-Serial/JTAG controller at `0x500D2000`)
- **Cross-Compiler:** `riscv32-esp-elf-gcc (crosstool-NG esp-14.2.0_20241119) 14.2.0` (`-ffreestanding -nostdlib -march=rv32imafc_zicsr_zifencei -mabi=ilp32f`, SHA-256: `2bc6b93e97fbe3b5662655bafab65377a84c2850fff1f2abda97abeb91f5690b`)
- **Loader Tool:** `esptool.py v4.12.0` (`elf2image` + `load_ram` into `L2MEM_LOW` at `0x4FF00000`, SHA-256: `8ec1e4d5736e97248bc0eac713d94fba5a002b725d74d9d4c850d570e50ded5a`)
- **Runtime Dependencies:** Zero FreeRTOS, zero ESP-IDF runtime, zero Arduino, zero libc.

### Unedited Serial Capture (`/dev/cu.usbmodem1301`)

```text
esptool.py v4.12.0
Creating esp32p4 image...
Merged 1 ELF section
Successfully created esp32p4 image.
Connecting...
Detecting chip type... ESP32-P4
Connected to ESP32-P4 (revision v1.3)
RAM boot...
Downloading 12096 bytes at 4ff00000... done!
All segments done, executing at 4ff00000
=== CAPTURED SERIAL OUTPUT ===

========================================================
MaiaRTOS v0.1.0 — Bare-Metal Kernel & Shell Bring-Up
Target : M5Stack Tab5 (ESP32-P4 rev v1.3)
Runtime: ZERO FreeRTOS / ZERO ESP-IDF / ZERO Arduino
mhartid: 0x00000000  misa: 0x40901125  sp: 0x4FF13620
========================================================
[M1] Trap vector verified (ebreak): sync_exceptions=1 last_mcause=0x30000003 systimer_us=1221096
[M2] Created 3 RTOS tasks (idle=0, telemetry=1, maia_shell=2) — starting scheduler...
[M2/M5] Telemetry task queue verified: rx_seq=3

========================================================
 MaiaRTOS v0.1.0 + MaiaShell Native Execution MVP (N2)
 Board  : M5Stack Tab5 (ESP32-P4 rev v1.3, 40MHz XTAL)
 Kernel : Clean-Room C11/RV32IMAFC (Zero FreeRTOS/IDF)
 Type 'help' for built-in RTOS introspection commands.
========================================================
[AUTO-SELFTEST] Running: uname; ticks; ps; mem; alloc 256; qtest
MaiaRTOS 0.1.0 riscv32-esp32p4 (M5Stack Tab5) [Zero-FreeRTOS/Zero-IDF]
tick_ms=60  systimer_us=1297744  ctx_switches=8
traps: total=6493 timer_irq=0 ecall=6492 last_mcause=0x3800000B
TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME
  0     0  READY  4096      6488     60  idle
  1     2  SLEEP  4096      3     0  telemetry
  2     2  RUN    8192      2     0  maia_shell
L2MEM Region Allocator:
  total=32748B  used=0B  free=32748B  peak=0B  allocs=0  frees=0
alloc OK ptr=0x4FF076A8 size=256 (verified & freed)
qtest PASS: mutex + sem + queue verified (0xCAFEBABE)
msh> help
MaiaShell Native Commands:
  help          - Show this command summary
  uname         - Show MaiaRTOS kernel & SoC architecture info
  uptime        - Show SYSTIMER microseconds & RTOS tick uptime
  ticks         - Show CLIC/SYSTIMER interrupt & trap counters
  ps            - List RTOS tasks (ID, prio, state, switches, ticks)
  mem           - Show L2MEM region allocator statistics
  alloc <bytes> - Allocate, verify & free a block from L2MEM heap
  qtest         - Run live mutex/semaphore/queue self-test
  pwd           - Print working directory
  set K=V       - Set shell variable (use $K in commands)
  env           - List shell variables
  echo ...      - Print arguments
  clear         - Clear terminal screen
msh> set BOARD_TEST=Tab5_Interactive_OK; echo $BOARD_TEST; uptime; ps
Tab5_Interactive_OK
tick_ms=9310  systimer_us=10565931  ctx_switches=9813
traps: total=1017004 timer_irq=0 ecall=1017003 last_mcause=0x3800000B
TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME
  0     0  READY  4096      1012004     9310  idle
  1     2  SLEEP  4096      373     0  telemetry
  2     2  RUN    8192      4627     0  maia_shell
msh> 
=== END CAPTURED OUTPUT (bytes: 2724 ) ===

---

### M5Stack Tab5 Clean-Room MIPI-DSI, Hardware Keyboard & Flash Boot Verification (2026-11-05)

- **Target:** M5Stack Tab5 (`ESP32-P4 rev v1.3`, Chip Revision `v1.3`, MAC `e8:f6:0a:e2:fd:4c`)
- **Firmware:** Clean-Room freestanding RV32IMAFC (Zero FreeRTOS / Zero ESP-IDF / Zero Arduino)
- **SPI Flash:** 16 MB flashed at partition `0x10000` via `esptool.py`

#### Cold Boot Output Capture (Standalone SPI Flash @ 0x10000)

```text
========================================================
MaiaRTOS v0.1.0 — Bare-Metal Kernel & Shell Bring-Up
Target : M5Stack Tab5 (ESP32-P4 rev v1.3)
Runtime: ZERO FreeRTOS / ZERO ESP-IDF / ZERO Arduino
mhartid: 0x00000000  misa: 0x40901125  sp: 0x4FF18B40
========================================================
[I2C] Scanning SysI2C(GPIO31/32): 0x10 0x28 0x32 0x36 0x40 0x41 0x43 0x44 0x55 0x68
[I2C] Scanning ExtI2C(GPIO0/1): 0x6D
[PANEL] Detected ST7121 (touch 0x55 fw=1)
[KBD] Initialized M5Stack Tab5 Keyboard (A164 @ 0x6D on GPIO0/1)
[DSI] D-PHY status: 0x00001529
[DISPLAY] Initializing ST7121 panel commands & timings...
[DISPLAY] Initialized MIPI-DSI 720x1280 (10x128 multi-block DMA in L2MEM)
[M1] Trap vector verified (ebreak): sync_exceptions=1 last_mcause=0x30000003 systimer_us=1786068
[M2] Created 3 RTOS tasks (idle=0, telemetry=1, maia_shell=2) — starting scheduler...
[M2/M5] Telemetry task queue verified: rx_seq=3

========================================================
 MaiaRTOS v0.1.0 + MaiaShell Native Execution MVP (N2)
 Board  : M5Stack Tab5 (ESP32-P4 rev v1.3, 40MHz XTAL)
 Kernel : Clean-Room C11/RV32IMAFC (Zero FreeRTOS/IDF)
 Type 'help' for built-in RTOS introspection commands.
========================================================
[AUTO-SELFTEST] Running: uname; ticks; ps; mem; alloc 256; qtest
MaiaRTOS 0.1.0 riscv32-esp32p4 (M5Stack Tab5) [Zero-FreeRTOS/Zero-IDF]
tick_ms=60  systimer_us=3914628  ctx_switches=8
traps: total=6242 timer_irq=0 ecall=6241 last_mcause=0x3800000B
TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME
  0     0  READY  4096      6237     60  idle
  1     2  SLEEP  4096      3     0  telemetry
  2     2  RUN    8192      2     0  maia_shell
L2MEM Region Allocator:
  total=32748B  used=0B  free=32748B  peak=0B  allocs=0  frees=0
alloc OK ptr=0x4FF0CBC8 size=256 (verified & freed)
qtest PASS: mutex + sem + queue verified (0xCAFEBABE)
msh> 
--> peek 0x500A0888
0x500A0888 = 0x000000A0  (DSI Bridge multiblk_num = 10, DMA active)
--> peek 0x50081018
0x50081018 = 0x00000001  (DW_GDMA Channel 0 enabled & streaming)
--> backlight 50
Backlight set to 50%
--> backlight 100
Backlight set to 100%
```
```

### Display baseline audit and corrections — 2026-11-11

See [report, primary references and exact sections](../audits/display-baseline-audit.md)
and [source/artifact manifest](../audits/display-baseline-manifest.json). M5Stack
[Tab5 documentation](https://docs.m5stack.com/en/core/Tab5), unversioned, accessed
2026-11-11, PinMap/Tab5 Screen Reset/Version Change: system I2C GPIO31/32,
ST712x touch address0x55, reset P4 high via input pull-up. Requested clocks,
porches and initialization are firmware configuration facts pinned to bf044d3,
not measured waveforms. ESP-IDF f5c3654 ROM linker table + cache.h verify native
writeback symbol address/signature/map; DPI driver/startup explain the legacy
DMA/PSRAM dependencies. Reference files match pinned SDK HEAD.
Actual boot selects ST7121 (FW1), reports32MiB PSRAM/200MHz, CPU360MHz and known
ELF prefix. Canonical host comparison finds30 matching ST7121 table entries.
Source inspection finds extra command-delay differences, native80/960 versus
legacy70/965 requests, and unreserved SRAM bands conflicting with the diagnostic
memory contract. No native display test or runtime import. Legacy shell/SciCalc14
and desktop reverified through maia.py. Logs and limitations are in the report.
This supersedes old blanket display-success claims as evidence of full parity,
and corrects initial shorthand: panel constructors remain in ELF, lv_init absent.

### Boot-only native system-I2C prerequisites — 2026-11-11

[Touch identification source ledger](../audits/native-touch-identify.md) records
primary URLs, revisions and pages: user-supplied Tab5.pdf update2026-11-05 p12
(SDA31/SCL32/address0x55), embedded schematics pp8–10 visually inspected; Sitronix
ST7123 interface protocol v01.11 2023-12-25 pp9/10/15 (16-bit pointer, STOP/START,
timing, firmware/status readiness). ESP32-P4 rev1.3 TRM Pre-release v0.4 ch9,
p552 and ch15 pp1041/1044, plus pinned v5.4.2 GPIO register definitions establish
IO_MUX offsets, GPIO input/output-enable registers and OEN_SEL bit10. The new
boot transport uses low latch + release-by-output-disable; SYSTIMER with bounded
snapshot/progress waits. No voltage, clock waveform, cold panel startup or
multi-task I2C functionality is inferred from those references.

Target result (same report): status4/Idle, firmware1 stable16 reads plus recovery
read; actual0x56 NACK and software-sampled SCL-low TIMEOUT, errors0. No physical
stuck-wire/frozen-timer/cold-panel injection. First strict-status-gate failures
are preserved; final policy accepts Normal/Idle only for read-only firmware ID.
Native ELF gate PASS, host19/19/sanitized fixture/C++98/six mutants PASS. After
RAM-only probe/reset, preserved shell/SciCalc14/desktop verified via maia.py.
No FreeRTOS/SDK runtime imports or flash/SD/power/reset/DMA writes.

### Native PI4IOE5V6408 port prerequisites — 2026-11-11

Primary [Diodes PI4IOE5V6408 datasheet](https://www.diodes.com/datasheet/download/PI4IOE5V6408.pdf),
DS40583 Rev3-5, September2022, pp8–10 register map/configuration and p13 Figures6/7
(single-register write and repeated-START read; page rendered/visually inspected).
OUT05 reads the latch, not physical pin voltage; DIR03 bit1 means output, and
HIGH_Z07/PULL_ENABLE0B/PULL_SELECT0D are plain configuration. ID/control01 and
interrupt/status registers have side effects, so the new subset excludes them.
The local M5Unified revision44952cb8e4ac88bf04ad23b9dfd6f4b05c788348,
PI4IOE5V6408_Class.inl/I2C_Class.inl, supplies the port's configuration/bit-update
model under MIT (notice retained in LICENSES/M5Stack-MIT.txt). Its calls delegate
to native transport; no M5Unified/Arduino/ESP-IDF/FreeRTOS runtime linkage.
Existing Tab5 PDF pinmap/schematic and BSP audit establish system bus/address43/44.
The forthcoming hardware probe mirrors only the current OUT05 latch byte and
checks unchanged configuration; this does not measure electrical levels or reset.

Actual target result: both expanders preserve five config bytes across16 rounds
after identical OUT05 writes (43: DIR0x6F/OUT0x66;44: DIR0xB9/OUT0x09). Native
transport/port has no foreign runtime symbols/archives. Actual absent-address
write NACK and touch firmware1 regression pass. No changed reset/power bits or
physical voltage/glitch measurement. Host20/20, two sanitizer fixtures, C++98 and
five driver mutants PASS; original flash shell/SciCalc14/desktop verified after
RAM reset. Raw evidence/provenance/limits in ../audits/native-expander-port.md.

### Native panel-reset port prerequisites — 2026-11-11

[Reset source ledger](../audits/native-panel-reset.md): Tab5.pdf update2026-11-05
p9 schematic bank43 P4/P5 and p16 reset electrical-mode rule; functional bf044d3
bsp_reset_tp100ms assertion +100ms stabilization; M5Unified Power_Class.inl
input/pull-up release, M5GFX tab5_power_on10/100ms distinguished from that baseline.
GPIO23 input preparation reuses pinned GPIO/IO_MUX constants and primary register
references in the touch audit. Apache-2.0 Espressif notice/full license retained
for the modified BSP reset port; MIT expander adaptation remains separately
attributed. No new MMIO addresses or CPU-frequency delay assumption introduced.

Actual reset target: stage9/changed1/preserved1, native SYSTIMER elapsed244643us,
GPIO23 input readback1; after reset touch status0/firmware1 sixteen reads plus
recovery, errors0. Both-bank snapshots compare final reset masks/unrelated bits.
This proves register sequencing/touch recovery on warm supplies, not electrical
voltages/waveforms, pixels or cold power-up. Host21/21, sanitizer/C++98/six mutants
PASS. Preserved shell/SciCalc14/desktop verified after runner reset; no flash/SD
write. Raw captures/artifact hashes/limits: ../audits/native-panel-reset.md.

### Read-only native VO3 calibration prerequisites — 2026-11-12

[Calibration source ledger](../audits/native-ldo3-calibration.md) pins primary
ESP-IDF f5c3654 ldo_ll.h conversion/unit mapping, efuse_reg.h revision/VO3 trims,
reg_base.h eFuse base5012D000 and pmu_reg.h P0_0P2A control/analog. Exact shadow
addresses5012D04C/5012D050/5012D1C0 are read only; no eFuse command/program register
is introduced. SDK model conversion is ported under retained Apache-2.0 notice.
The current native stage computes a2500mV plan and observes registers; it neither
claims the physical voltage nor acquires/enables the regulator. Actual power
programming and ownership/failure contracts remain a separate step.

Actual2026-11-12 RAM observation: chip1.3/block0.3, no version-disable or block1
RS errors. Trims55/33/33 ->model coefficients1030/-4/989 ->dref13/mul3, model
2504637uV/error4637uV, no default coefficients. Existing PMU enable/SW-selection
and codes already match; no writes were made.16 snapshots unchanged, errors0.
This validates reading/model arithmetic, not measured voltage or native power
initialization. Host22/22, sanitizer/C++98/six mutants and1048576-case exact SDK
differential PASS.

Preserved flash shell/SciCalc14/desktop verified after reset with paced keys;
initial unpaced3s capture failed to collect the result and is retained in the
VO3 report. No firmware fix or flash/SD write was made for this retest.

### Native VO3 acquisition prerequisites — 2026-11-12

[Acquisition source ledger](../audits/native-ldo3-acquire.md) pins SDK f5c3654
esp_ldo_regulator.c/ldo_ll.h acquire and VO3 mapping, pmu_reg.h control4F80 and
analogF7800000 masks. Current-limit/delay/other fields are preserved. Pinned
hp_sys_clkrst_reg.h +18 mask1C00 and +3C mask93 guard CSI/DSI functional clocks;
API/diagnostic do not gate or program these clocks. LCD MIPI example identifies
VO3 as VDD_MIPI_DPHY supply. Native API is boot-exclusive; the controlled enable
probe may clear only VO3 XPD before invoking acquire, after checking existing
calibration/source and all guarded clocks. This is not voltage/discharge proof.

Target finding: retained clock words0x0C0D5678/0x2B3 keep guarded MIPI enables
active across ROM RAM entry. Initial guard refused before writes; final native
policy adopts matching active VO3 without writing. Actual stage5/adopted1/changed0,
clock/PMU-slot preservation1 and duplicate BUSY. Inactive configure/enable is
host-tested only; physical cycle NOT RUN until ordered consumer/DMA quiesce.
Host23/23, sanitizer/C++98/six mutants PASS. Paced shell/SciCalc14/desktop verified
after reset; no flash/SD or target regulator write. See ../audits/native-ldo3-acquire.md.

The earlier panel-reset probe did not establish consumer/DMA quiescence. Its
report is now qualified and its diagnostic rejects active MIPI clocks before
reset; guarded probe compile passes, physical rerun remains pending quiesce.

### Read-only inherited MIPI/DW-GDMA state — 2026-11-12

[Source/register ledger and actual values](../audits/native-mipi-state.md)
pins ESP-IDF f5c3654 revision/sections and primary URLs. In particular,
[peripheral linker symbols](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/ld/esp32p4.peripherals.ld)
verify DW_GDMA50081000 and CSI/DSI host/bridge bases. Pinned hp_sys_clkrst_reg.h
SOC_CLK_CTRL0+14/CTRL1+18 bus gates control diagnostic access; DW-GDMA requires
both CPU13/SYS5 per dw_gdma_ll_is_bus_clock_enabled. Native reads only listed
non-clearing config/status registers; never reads memory through DMA pointers.

Tab5 eight complete sequential samples: CPU/SYS/PHY E6DFB7FF/0C0D5678/2B3,
valid0D. DSI bridge/DPI enable0, host POWER/PHY_RSTZ0; CSI bridge0. CSI host
skipped because bus clock off. DW-GDMA CFG/CHEN0/CHEN1 all0, four channel
FIFO/status/address/LLP0. Clock enables persist, but controller enables were0;
active DMA is not established. Snapshot cannot certify AXI drain/PHY stop or
cold initialization. Host24/24, ASan/UBSan and negative checks PASS. Preserved
flash shell/SciCalc14/desktop verified after reset; no flash/SD write. This
checkpoint does not remove VO3/panel-reset guards or port native shutdown yet.

### Native VDMA completion contract — 2026-11-12

Primary silicon source: [ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
section5.5.7 pp337–338 (early channel disable), register5.3 p349 (global EN
readback stays1 until channel activity terminates), register5.4 p350 (EN_WE),
register5.5 p351 (RESET), section5.8 pp344–347 (register offsets). SDK f5c3654
[dw_gdma_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
channel-disable strobes/global completion/both bus clocks cross-check the same
contract. Addresses remain DW_GDMA50081000, CFG+10, CHEN+18, ABORT+1C, RESET+58,
channel FIFO+134/+234/+334/+434; no new interrupt mappings or DMA memory claims.
Exact PDF/source hashes, local extraction and source sections are retained in
[the stop audit/manifest](../audits/native-vdma-stop.md).

Tab5 actual empty-controller test: CFG write1/read1 with all channels disabled,
then native global stop0/read0 in one poll, no transfers. stage3/armed1/changed1/
requested0/preserved1/duplicate_busy1; all sampled MIPI/clock/channel words
preserved. This is not active DMA drain evidence; active path/failures are host
25/25 +sanitizer tested. No clock/power/reset/IRQ clear/descriptor/flash/SD writes;
shell/SciCalc14/desktop verified after RAM reset. PHY/bridge, DMA-visible memory
and active-transfer tests remain separate prerequisites for full native display.

### Native DMA-visible SRAM and active stop — 2026-11-12

Primary source [ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
7.3.1 pp451–454 (CPU NC alias8FF and L2MEM4FF/DMA access),5.7.4 p343 (single
block),5.5.7 pp337–338 and CFG0 p349 (stop/completion). Pinned SDK f5c3654
[cache alias definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/cache_ll.h),
ext_mem_defs.h offset40000000, dw_gdma_ll.h/register layout confirm memory-master1/
width8/increment/burst1. ROM cache.h signature/L1D map10 and esp32p4.rom.ld
4FC003FC/4FC003C8 are explicit immutable-ROM calls, not an IDF runtime link.
Register INTSIGNAL writable maskF83F7FFB preserves observed07C08004 reserved/RO
bits; the original zero-readback assumption was refused before channel enable.
Exact source revisions/hashes/sections in [the fixture audit](../audits/native-vdma-transfer.md).

Actual reserved SRAM4FF03500–4FF13700, CPU NC alias8FF03500, L1D64B/range purge
success. Full32768B M2M verified; second transfer stopped with initialCFG1/CHEN1/
requested1, two polls, finalglobal/channel/abort0, correct1152B prefix+zero tail,
source/guards intact and eight full NC scans stable. Sampled clocks/MIPI/other
channels unchanged. Status/common0 (no reported errors); no DMA IRQ claim.
Host26/26/sanitizer/negative checks PASS. Final capture complete; earlier truncated
banner retained/rejected. Shell/SciCalc14/desktop verified after reset; no flash/
SD/power/reset/clock writes. Peripheral handshake/PHY stop and PSRAM remain open.

### Native boot/reset MIPI parking — 2026-11-12

Primary [ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
38.7.2 p1974 identifies DMA completion/ISP reset/CSI host reset for live RX stop.
This is not implemented by the smaller boot parking API. Pinned SDK f5c3654
[mipi_csi_hal.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_csi_hal.c)
init asserts shutdown/reset controls low; DSI/CSI LL functions and register
headers pin bridge500A0804/5009F804 bit0, DSI PHY500A00A0 bits1..3 (bit0 shutdown
release must already0), CSI PHY5009F044 bit0 reset release, host controls500A0004/
5009F008/5009F040. hp_sys_clkrst_reg.h SYS+18 bit11/mipi_csi_ll host-bus helper
allow CSI host inspection; no PHY clock/power change. Exact source hashes/sections
and remaining contracts: [parking audit](../audits/native-mipi-park.md).

Actual Tab5 digital-arm/park RAM test PASS. SYS0C0D5678->0C0D5E78 enables only CSI
host access; shutdown/host controls remain low, DSI RSTZ2/CSI RSTZ1 and bridges1
are armed, then native stage4/attemptF/four reported control words0. Sampled
clocks after preparation and VDMA/channel/other config unchanged. DSI PHY clock-
lane/forcePLL dirty cases host-tested only. Host27/27/sanitizer/header/negative
checks PASS. Shell/SciCalc14/desktop verified after reset; no flash/SD/LDO/analog
power writes. Shared ISP/live-stream quiesce and physical settling remain open
before treating this as unconditional permission for clock/power gating.

### Shared ISP prerequisite and native clock gating — 2026-11-12

[ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
38.7.2 p1974 requires VDMA completion/ISP reset/CSI host reset; 34.7.5–7 pp1574–1575
separates input-source/input-enable from ISP global enable. Pinned SDK f5c3654
[isp_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/isp_ll.h)
module clock/reset and input source helpers, isp_reg.h CNTL+8 source bits28:27/
MIPI_DATA_EN0 and CAM_CNTL+114 enable/update/reset bits0..2, plus mipi_csi_ll.h/
mipi_dsi_ll.h clock/reset primitives pin the native policy. hp_sys_clkrst_reg.h
RCC500E6000+A8 ISP clock25, +C0 reset26..29 mask3C000000, +CC reset overrides11..14
mask7800, +3C PHY93 and +18 SYS1C00; preserve other fields. Exact sources/hashes and
restrictions: [gate audit](../audits/native-mipi-gate.md).

Target initially refused active ISP clock02E82000 before gate writes; clocked
inspection measured CNTL40002442/CAM4: CSI source, no MIPI data input, DVP held
reset/disabled. Native park claim +fresh input/VDMA/RCC guards then hold reset
40->3C000040, PHY2B3->220, SYS0C0D5E78->0C0D4278; stage5/attempt7/preserved1/
duplicate_busy1 PASS. All guarded MIPI clock bits0; ISP clock unchanged/module reset
held, CPU/VDMA retained. No MIPI/ISP MMIO after reset, no LDO/analog power/flash/SD
writes. Host28/28/sanitizer/header/negative checks PASS; shell/SciCalc14/desktop
verified after RAM reset. Electrical settling and live shared-ISP shutdown not proven.

### Guarded VO3 off and native inactive-rail acquire — 2026-11-12

Primary sources: ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[regulator](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/ldo/esp_ldo_regulator.c),
[ldo_ll](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/ldo_ll.h),
[CSI constructor](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_driver_cam/csi/src/esp_cam_ctlr_csi.c)
and mipi_csi_ll.h/RCC/PMU/CSI register headers pinned in
[manifest](../audits/native-vo3-cycle-manifest.json). TRM chip1.3 Pre-release v0.4
38.7.2 p1974/34.7.5–7 pp1574–1575/VDMA CFG0 p349 remain park/gate prerequisites;
[primary TRM](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf).
SDK enable/disable is an XPD write, with no explicit voltage-ready/discharge wait.
Native supply port attribution unchanged; no SDK runtime/refcounts imported.

[Actual capture/audit](../audits/native-vo3-cycle.md): CSI clock/reset preparation
38000040 ->20000040 before host MMIO; native park/gate held3C000040 and all MIPI
clock masks0. CTRL40200180 ->40200080 ->40200180/ANA D5800000 retained, calibrated
dref13/mul3/defaults0. Existing native acquire stage5/adopted0/changed1; sixteen
samples each phase/other ten LDO words/guards preserved/duplicate BUSY. Host windows
20.946/21.124ms are observation only, no electrical measurement. Analog/source
writes skipped because already matching. Early clock refusal/access fault logged,
no VO3 writes in either; final native trap vector and complete prerequisites pass.
Host30/30, ASan/UBSan, four compiled mutants/invalid profile PASS. Native ELF runtime
gate clean, SRAM-only image; preserved shell/SciCalc14/desktop verified after reset.
No flash/SD writes; no native cold display, voltage/ramp/load or live-stream proof.

### Native DSI register/config access — 2026-11-12

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[DSI bus constructor](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c)
`esp_lcd_new_dsi_bus` and
[mipi_dsi_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_ll.h)
pin RCC SYS12, PHY config0/reference1/DPI7 and reset26; source selection separate.
Pinned RCC/DSI register headers define host version+0/power+4/PHY_RSTZ+A0/status+B0,
bridge+4/DPI+40/FIFO+14. No guessed registers/timings. Earlier TRM chip1.3 v0.4
38.7.2 p1974/34.7.5–7 pp1574–1575/VDMA CFG0 p349 remain park/gate prerequisites.
Primary [TRM](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf).

[Audit/capture/manifest](../audits/native-dsi-access.md): native gated/reset-held
access preparation after guarded VO3 cycle passes actual SYS0C0D4278->0C0D5278,
PHY220->221, RESET3C000040->38000040. VO3 CTRL40200180/ANA D5800000 unchanged.
VERSION3134312A/POWER0/PHY_RSTZ0/BRIDGE0/DPI19D0(enable0)/FIFO0/STATUS1528;
status is observation, not PLL/lanes ready proof. All CSI/ISP resets remain held,
reference/DPI clocks off. Only three RCC writes by new API, stage5/attempt7,
duplicate BUSY and final RCC/VDMA/eFuse/PMU guards pass. No source/PLL/test-port/
analog power/transmit/panel/video/PSRAM writes. Host31/31/sanitizer/C++98/four compiled
mutants/invalid profile PASS. SRAM image dependency gate clean; installed baseline
shell/SciCalc14/desktop recovered; no flash/SD writes. Cold native display remains
pending. Exact pinned source hashes in manifest; original native code, no SDK runtime.

### Native nominal20MHz DSI reference routing — 2026-11-13

Primary [chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf):
10.2.2 p616 clock tree SPLL480MHz; registers10.11 p651 REF_CLK_CTRL1/+28,
10.12 p652 REF_CLK_CTRL2/+2C,10.15 pp657–658 PERI_CLK_CTRL02/+38,
10.16 p659 PERI_CLK_CTRL03/+3C. DSI source31:30 drives BOTH config/reference;
0=PLL_F20M,1=RC_FAST,2=PLL_F25M,3=invalid. Gate config around mux changes.
Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[clk_tree_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_tree_ll.h)
F20 divider=getfield+1,
[esp_clk_tree.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/port/esp32p4/esp_clk_tree.c)
nominal480MHz/div24,
[clk_gate_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_gate_ll.h)
REF20 provider bit8. Pinned
[lp_clkrst_reg.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/lp_clkrst_reg.h)
HP_CLK_CTRL50111040 XTAL40bit25/SPLL480bit27 sampled read-only, not lock proof.

[Audit/capture/manifest](../audits/native-dsi-reference.md): actual RAM stage7/
mask10/16 stable samples PASS. DIV58170503, LP1FFCFFFD, provider115, source38862600
already match; retained unchanged. PHY221->223 only actual write, reference on,
config retained. No physical frequency measurement: reported20MHz is model only.
DSI analog/digital/host/bridge controls0, DPI19D0(enable0), FIFO0/STATUS1528 observed,
no PLL-ready inference. VO3/eFuse/SYS/reset preserved; CSI/ISP resets held. Other
provider/mux programming branches host-tested only. Host32/32, sanitizer/C++98/four
compiled mutants/invalid profile PASS; native runtime gate clean; desktop recovery
shell/SciCalc14 PASS, no flash/SD writes. Native cold display/PSRAM/PHY PLL pending.

### Native DSI PHY PLL lock and lane stop — 2026-11-13

Primary ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[mipi_dsi_hal.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c)
init/PLL/test-byte functions,
[PHY LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_phy_ll.h),
[range table](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/mipi_dsi_periph.c)
and host register header: PHY_STATUS+B0 lockbit0/stopbits2,4,7; PHY_RSTZ+A0 bits0..3;
PHY_IF_CFG+A4 lanes0..1; LPCLK+94 HS requests0..1; test+B4/B8 writable masks3/100FF,
RO TESTDOUT15:8. Register facts from pinned SDK where TRM omits PHY internals.
[TRM chip1.3 Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
10.2.2 p616/regs10.11–10.16 pp651–659 remain clock prerequisites. No guessed timing.

Working local M5Stack-derived BSP ST7123/ST7121 constructor requests2lanes/965Mbps;
SDK chooses M48/N1 from20MHz, modeled960Mbps/hsfreq1A. BSP hash and pinned source
hashes in [audit/manifest](../audits/native-dsi-phy.md), not claimed unmodified upstream.
Adapted test-port sequence carries Apache-2.0 attribution, no runtime/task dependencies.

Actual native RAM PHY stage11 PASS:40 stores/five completed bytes/7polls,
POWER1/PHY_RSTZF/lane word1, STATUS15BD(lock+all-stop95),16 stable later observations,
clock/reset/VO3/eFuse guards preserved, duplicate BUSY. Initially status1528/lock0,
so no stale-lock adoption. Physical bitrate/timing/settling unmeasured. Bridge/DPI
remain off/FIFO0, no HS/packets/DCS/video/PSRAM. PHY powered/LP idle possible.
Host33/33/sanitizer/C++98/four compiled mutants/invalid profile PASS.41 dropped-store
positions tested,4 identical clock-low no-ops correctly tolerated. Original shell/
SciCalc14/desktop recovery passes, no flash/SD writes. Native cold display pending.

### Native DSI command-host setup — 2026-11-13

Primary ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[DSI bus constructor](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c),
[host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
[PHY LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_phy_ll.h)
and host register header pin MODE+34, CLKMGR+8 (escape low8/timeout8..15),
DATA_TMR+9C/CLK_TMR+98(10bit LP/HS fields), PCKHDL+2C(mask39), timeout registers
+78..8C, MAX_RD+F4(low15), PHY_IF+A4(stop-wait8..15), LPCLK+94(low2). Generic
CMD_PKT_STATUS+74 low7 empty0/2/4, full1/3/5, readbusy6; require15. SDK switch
counts50/104/46/128, maxread6000/stop63, timeout counts0. At nominal960Mbps,
byteclk120MHz/div6=20MHz escape and/div12=10MHz timeout, not measured clocks.
Earlier [TRM chip1.3 Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
10.2.2 p616/regs10.11–10.16 pp651–659 remain reference routing prerequisites.
No guessed timing; complete parameter masks/source hashes in
[audit/manifest](../audits/native-dsi-command.md).

Actual native RAM command stage16/mask181E: six writes. CLKMGR0C06/DATA00320068/
CLK002E0080/PCK19/MAXRD1770/PHY_IF3F01, MODE1 already retained. STATUS15BD and
FIFO00050015(low7=15),16 stable later observations/clock/reset/supply/PHY guards
preserved, duplicate BUSY. No header/payload or panel/HS/video/DMA/PSRAM writes;
HW timeout counts0, transaction completion API still pending. Host34/34/sanitizer/
C++98/four compiled mutants/invalid profile PASS. Native runtime gate clean,
preserved shell/SciCalc14/desktop verified; no flash/SD writes. No full native
panel/display, physical timing or packet acceptance proof claimed.

### Native bounded LP short packet publication — 2026-11-13

Primary SDK f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[HAL DCS short cases](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c),
[host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
[type definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/include/hal/mipi_dsi_types.h)
and host register header: DT05/15, CMD_MODE_CFG+68 LP16/17/ACK1/TE0, GEN_HDR+6C
parameter16..23/command8..15/VC6..7/DT0..5. RX VC untouched; no BTA/read. INT_ST0+BC
bits0..20 mask1FFFFF, INT_ST1+C0 bits0..12/19 mask81FFF. Status reads may consume
diagnostics: exclusive ownership/OR accumulation/immediate failure, no blind clear.
Native packing helper Apache-2.0 attributed; no SDK runtime imports.

Primary local [ST7123 SPEC V0.5](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/ST7123_SPEC_Preliminary_V0.5.pdf)
2021/09 section6.2.1 p140 defines NOP00 with no display effect, usable in sleep-in/
sleep-out. Source URL catalog/local SHA in [audit/manifest](../audits/native-dsi-short.md).
Earlier chip1.3 TRM v0.4 ch10/p616/regs10.11–10.16 pp651–659 clock facts unchanged.

Actual native Tab5 RAM16 NOPs PASS: one attempted header05 each, stage5, each2polls
and busy witnessed before drain/stop. LP selector0->10000 once, STATUS15BD/
FIFO50015/errors0, all prior config/root/supply/PHY guards retained. Explicit ACK0:
no panel response/reception/initialization proof. Hardware DCS1/nonzeroVC and faults
not exercised. Host35/35/sanitizer/C++98/five compiled mutants/invalid profile PASS;
GEN_HDR dropped-store ambiguity modeled/documented, not falsely detected. Runtime
gate clean, original shell/SciCalc14/desktop recovered, no flash/SD writes. Native
long writes/ACK/read/panel/video/PSRAM/full desktop remain pending.

### Native LP long framing and internal-buffer completion — 2026-11-14

Primary SDK f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[HAL long path](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c),
[host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
[register header](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_host_reg.h):
GEN_PLD+70, GEN_HDR+6C WC8..23, DCS39/LPbit19, CMD_PKT_STATUS+74 low interface
bits0..6 plus GEN_BUFF_CMD_EMPTY/FULL16/17, GEN_BUFF_PLD_EMPTY/FULL18/19. Require
both levels idle (F007F=50015); space checks mask80008, pre-header command30003=
10001. No capacity inferred. Native bytewise packing avoids SDK unaligned/tail
32-bit overread, Apache-2.0 port attribution retained.
[ST7123 V0.5](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/ST7123_SPEC_Preliminary_V0.5.pdf)
2021/09 sections5.4.2.3.1 p92/typep99/WCp102/payload103,109/NOP6.2.1p140 support
framing/content. DT39 is not payload command39. No ACK/panel acceptance proof.
Earlier TRM chip1.3 v0.4 ch10/p616/regs10.11–10.16 pp651–659 clock facts unchanged.

[Final capture/audit/manifest](../audits/native-dsi-long.md): preliminary capture
revealed10015 after staging despite front-empty; preserved and guard strengthened.
Revised Tab5 RAM4 one-byte NOP long frames stage6/header139/oneword/onebyte/4polls/
2space/busy1, pre10015->50015/PHY15BD/errors0, mode10000->90000 once, preserved1/
panel_ack0. Short NOP followup succeeds. Multiword/otherVC/buffer-depth unverified
on target. Host36/36/ASan+UBSan/C++98/seven compiled mutants/invalid profile PASS.
Native runtime gate clean, old shell/SciCalc14/desktop recovered. No flash/SD/
parser/video/PSRAM/panel configuration writes. Recovery carries both candidates.

### Native RDDID/BTA request and PHY refusal — 2026-11-14

Primary pinned SDK f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[HAL read path](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c),
[host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
[registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_host_reg.h):
MRPS DT37/read DT06, LP bits24/18, BTA PCKHDLbit2/RXVC low2; RXempty4/readbusy6,
PHYdirection1; GEN_PLD+70 pop. ST0bit20 named DPHY_ERRORS_4, detailed meaning NA.
[ST7123 SPEC V0.5](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/ST7123_SPEC_Preliminary_V0.5.pdf)
2021/09 section6.2.3 p142 specifies DCS04 and3 ID bytes/defaultFF in sleep-in/out;
p99 packet types. Does not prove this device's response or identity.

[Read audit/captures/manifest](../audits/native-dsi-read.md): two actual RAM loads
return IO at stage6/polls6/headers3, RXready FIFO50005/PHY15BD/ST0=00100000.
No payload pop/ID/restoration accepted; halt+reset preserved baseline. Working
shell/SciCalc14/desktop verified twice. Native predecessors pass, runtime gate
clean;37/37 host/sanitizer/C++98/seven mutants/invalid profile PASS. Success path
HOST-TESTED ONLY. Error meaning/cause unresolved, no mask/IRQ-status writes.
Microchip section6.5.42 labels an identically named bit LP1 contention; this is a
research clue, not P4 confirmation (primary link and limits in audit). No DSI
erratum in Espressif summary consulted2026-11-14; does not prove error benign.
No flash/SD/parser/production changes. Read/initialization/video/PSRAM gate open.

### DSI escape divider correction and bounded responses — 2026-11-14

Primary official ESP-IDF [bus initialization](https://github.com/espressif/esp-idf/blob/cebb3dd4d8704a5aa1c2b3a23c4c4b9b26e71194/components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c)
revision cebb3dd4d8704a5aa1c2b3a23c4c4b9b26e71194, esp_lcd_new_dsi_bus,
MIPI_DSI_DEFAULT_ESCAPE_CLOCK_FREQ_MHZ=18, TxClkEsc2..20MHz comment and rounded
lane_Mbps/8/18. Native fixed960Mbps selects7, replacing original pinned f5c3654
integer-truncated6. Register field CLKMGR+08 TX_ESC_CLK_DIVISION low8 is from
[original LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h).
No new addresses/PHY analog timing assumed. CLKMGR0C07 => nominal120/7MHz escape,
timeout unchanged10MHz; report17142857Hz. Not physically measured. Attribution
Apache-2.0 retained; no foreign runtime/headers/roundf function imported.

[Final audit/captures/manifest](../audits/native-dsi-bta-escape.md): two final RAM
loads pass4 bounded reads each, stage10/header attempts3/oneword00FBA080/bytes
80 A0 FB/polls7/error0=error1=0/PHY15BD/FIFO50015/restored mask5/mode90000,
stable1/preserved1. Earlier2 divider6 loads rejected ST0bit20; no acceptance rule
relaxed. Intermediate divider7 captures with stale report retained separately.
Host37/37/four ASan+UBSan fixtures/C++98/two targeted mutants PASS, runtime gate
clean. Full predecessor checks pass; shell/SciCalc14/desktop recovered. No flash/SD
writes. Response word does not prove exact length/model. Electrical error cause
and detailed P4bit20 meaning remain unresolved; native cold panel init/video/
framebuffer/PSRAM/full desktop still pending. Default firmware still FreeRTOS.

### Quiet native reset integrated before DSI — 2026-11-14

Primary [Tab5 PDF](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/core/Tab5.pdf)
update2026-11-05 schematicp9/P4 LCD_RST/P5 TP_RST, p12/GPIO23 and p16 reset-release
input/pull-up; unchanged native100/100ms port documented in native-panel-reset.md.
[Preserved working BSP](https://github.com/souzamonteiro/maia-rtos/blob/d0886c3/platforms/tab5/components/m5stack_tab5/m5stack_tab5.c)
bsp_detect_display_type maps touch firmware1->ST7121,3->ST7123; this device reports1.
This is BSP variant policy, not independent die identification. No mapping from
DSI00FBA080 to an assumed model. Native guarded clock/DMA/supply facts unchanged
from earlier pinned TRM/SDK references; no new MMIO/pin/timing fact introduced.

[Integrated reset audit/captures/manifest](../audits/native-dsi-reset.md) records two
final RAM loads: MIPI sys0C0D4278/phy220/reset3C000040/DMAquiet1/supplyready1;
reset result0/stage9/changed1/elapsed3BD55 and3B5FEus hex, firmware1/status0,
banks/clocks/supply/GPIO input preserved1, followed by four bounded stable00FBA080
RDDID words each/errors0/restored controls. Earlier standalone reset had no proven
gate; this closes that limited integration gate. No cold power/electrical pulse/
visual/panel-startup proof. Host38/38, two sanitized fixtures/four mutants/invalid
profile PASS, native runtime gate clean; original shell/SciCalc14/desktop restored.
No flash/SD/parser/foreign-runtime changes. Next selected ST7121 command sequence;
video/framebuffer/PSRAM/full native desktop remain pending.

### Native ST7121 command program and generic FIFO space — 2026-11-14

Primary preserved [ST7121 driver](https://github.com/souzamonteiro/maia-rtos/blob/883e9a6/platforms/tab5/components/esp_lcd_st7121/esp_lcd_st7121.c)
883e9a6, panel_st7121_reset/reset_gpio=-1, vendor_specific_init_default,
panel_st7121_init:01/120ms then30 table entries with source delays+5ms. Port exact
31 packets/461 bytes/1150ms, max61bytes. Apache-2.0 source/attribution retained;
no foreign runtime/DPI code imported. BSP firmware1 policy selects this program.
No new vendor-register semantics guessed; data is the known-working board recipe.

Primary pinnedSDK f5c3654 [host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h)
gen_is_write_fifo_full uses GEN_PLD_W_FULL/bit3;
[HAL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c)
checks it before each payload write. Actual initial90015/internal-full bit19 with
front bit3clear blocked old conservative mask after8bytes/before header. Corrected
space predicate tobit3, retained both-level empty/full completion F007F=50015 and
all native guards/errors/budgets. No FIFO capacity or streaming inferred.

[Audit/captures/manifest](../audits/native-st7121-program.md): two final RAM loads
pass31 packets/all waits, A7/61bytes stage6/23polls/errors0, then four stable
00FBA080 responses each/stage10/PHY15BD/FIFO50015/modeB0000 restored/mask5/errors0.
Host39/39/two sanitized fixtures/C++98/five mutants/invalid profile/table equality
PASS; runtime gate clean. Original shell/SciCalc14/desktop restored, no flash/SD.
No register/state/visual readback yet; next bounded panel-state verification,
then DPI/video/framebuffer/PSRAM. Default full desktop still FreeRTOS-based.

### Bounded RDDPM and reported SleepOut/DisplayOn transition — 2026-11-14

Primary pinnedSDK [commands](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/include/esp_lcd_panel_commands.h)
LCD_CMD_RDDPM0A. [Linuxv6.12 DCS constants](https://github.com/torvalds/linux/blob/v6.12/include/drm/drm_mipi_dsi.h)
corroborate bit2 Display/bit4 Sleep positions; facts only, no source imported.
Supplier-hosted [Sitronix ST7121P V1.0](https://www.display-lcd.com/data/upload/admin/202509/68b935053144a.pdf)
2024/01 section6.1 p95/6.2.6 p103:0A/one byte, SleepOut1/DisplayOn1, available
in sleep-in/out. Actual P suffix unproven; used only to corroborate standard DCS,
not identify silicon. Independent standard definition cross-checks heading typo.
All SoC packet/register/guard facts unchanged from native-dsi-read pinned HAL/LL.

[Power audit/captures/manifest](../audits/native-panel-power.md): two final RAM loads
observe4 stable08 before and4 stable9C after selected program each, all stage10/
requestA06/headers3/polls6/oneword/PHY15BD/FIFO50015/errors0; host mode restored
90000/B0000/mask5. Bits14h change0->14h; no full-byte hardcoding. Confirms reported
power-state transition, not pixels/color format/all vendor registers. Host39/39,
ASan+UBSan/C++98/five mutants/invalid profile PASS, runtime gate clean.
Original shell/SciCalc14/desktop recovered; no flash/SD/parser/table/foreign-runtime
changes. Next DPI format/timing/clock before video/framebuffer/PSRAM/full desktop.

### Inactive native DPI register staging — 2026-11-14

Primary pinned SDKf5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[clock enums](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/clk_tree_defs.h)
DPIdefaultPLL240;
[HAL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c)
integer divider and per-duration lane-byte conversion;
[bus LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_ll.h)
HP+3C sourcebits6:5/divbits15:8/gate7;
[host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
[bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h)
and matching register headers define masks/encoding. Working BSPc046903 ST7121
usesRGB565/720x1280/H2,40,40/V20,24,200/request70MHz =>240/div3=modeled80MHz;
PHY960/8=120MHz ->ratio3/2. No measured pixel frequency; clock stays gated.
Apache-2.0 attribution retained for adapted layout, no SDK runtime imported.

[Audit/captures/manifest](../audits/native-dpi-stage.md): two actual RAM loads pass
stage22/attemptFFF30(14stores),19 checked host/bridge config words, route223 retained;
HSA3/HBP60/HLINE1203/V20,24,200,1280; bridge totals802/1524/rawRGB5652.
MODE1/bridge0/DPI19D0/clockgateoff/update0/DMAquiet1/preserved1/duplicateBUSY1.
Four subsequent power9C reads each/error0=error1=0/controls restored. Host40/40/
sanitizer/C++98/five mutants/invalid profile PASS; runtime gate clean. Shell/SciCalc
14/desktop recovered, no flash/SD/parser changes. Route switching branch host-only,
no live latch/provider startup/active clock/output/pixels proof. Panel format still
to verify before activation. No framebuffer/PSRAM/full native desktop claim.

### Reported format selection and inactive DPI clock gate — 2026-11-14

Primary pinnedSDKf5c3654 [commands](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/include/esp_lcd_panel_commands.h)
0C read color/3A select; [clk_gate_ll](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_gate_ll.h)
REF240bit30 HP+28; [DSI LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_ll.h)
DPIgate7 HP+3C. Shared reference divider unchanged except allowed provider bit.
Primary M5GFX28d47949 [Panel_DSI](https://github.com/m5stack/M5GFX/blob/28d47949bdc84f30881623476f555398e2194313/src/lgfx/v1/platforms/esp32p4/Panel_DSI.inl)
RGB565 setColorDepth/init_dpi and COLMOD55 init_panel; no vendor runtime imported.
Supplier ST7121P V1.0/2024/01 p105 corroborates DBI low3=5(16bpp), initial00;
P suffix/wire-type applicability unproven, tablep56 listsRGB888. See audit for
qualification; don't equate reported DBI mode with actual pixel/wire acceptance.

[Audit/captures/manifest](../audits/native-dpi-clock.md): first00 read halted before
write; final two RAM loads select3A55/00->05, verify four reads, then clock223->2A3/
stage4/mask2/provider58170503 unchanged. MODE1/bridge0/DPI19D0/update0/DMAquiet1;
four post-clock color05 and power9C responses each/errors0/restored controls.
40/40 host tests/two sanitized fixtures/C++98/six mutants/invalid profile PASS,
runtime gate clean; shell/SciCalc14/desktop recovered. No flash/SD/parser/table
changes. Logical gates/model80MHz only, provider-off branch host-only; no measured
frequency/live latch/pixels/DMA/framebuffer/PSRAM/full native desktop proof.

### Native host VPG, illumination and command recovery — 2026-11-14

Primary pinned SDKf5c3654 [DSI host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h)
`dpi_set_pattern_type`, `set_clock_lane_state`, `enable_video_mode` and
[host register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_host_reg.h):
VPG16/mode20/orientation24 at+38, AUTOclockbits1:0 at+94, command/video0 at+34,
shadow controls+100. [SDK pattern selector](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c)
uses host VPG with bridge DPI output off. [Tab5 PDF](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/core/Tab5.pdf)
update2026-11-05 p12 G22/LEDA, preserved BSPe92305a DC HIGH100%; pinned
[GPIO fields](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/gpio_reg.h)
output256/OEN10/invert9,11/OD2 and [mux](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/io_mux_reg.h)
MCU_SEL14:12=1. New native transition/light code original MIT, parameter facts only.

[Audit/captures/owner observation/manifest](../audits/native-dpi-pattern.md) record
first black screen and final stable bars confirmed by owner after temporary native
GPIO22 illumination. Final one RAM load, five samples/errors0, no bridge/update/
DMA, exact stopped command/LP/VPG/GPIO restoration and four9C replies. This proves
visible native host-pattern output on this board, not bridge/DMA/framebuffer
pixels, physical80MHz, colorimetry or cold standalone PLL startup. Host41/41 and
negative/ASan/UBSan checks PASS; original flash firmware preserved/restored.

### Pattern orientation correction — 2026-11-14

Owner clarified the stable native VPG bars appeared HORIZONTAL. The previous
“vertical” description came from the SDK enum/question wording, not a measured
screen orientation. Captured VPG_ORIENTATION24=0 remains unchanged. Production
[HAL at58e8f47](https://github.com/souzamonteiro/maia-rtos/blob/58e8f47/platforms/tab5/main/hal/hal_esp32.cpp#L104)
selects `LV_DISPLAY_ROTATION_90` and software rotation. Native panel720x1280 and
logical desktop1280x720 are separate coordinate spaces. No firmware change or
hardware rerun; see [correction](../audits/native-dpi-pattern-orientation.md).

### Native bridge enable and WT update publication — 2026-11-14

Pinned SDKf5c3654 [bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h)
`enable/update_dpi_config` and [register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_bridge_reg.h):
EN+04 bit0 R/W, DPI_UPDATE+44 bit0 WT (write1 updates DPI_*), output+40 bit0,
INT_ENA+50/INT_RAW+58 underrun0. [DPI setup](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c)
enables bridge before update. Native original MIT guarded policy uses these facts,
no runtime imports. No completion ACK exists in the cited WT interface; a read0
or an issued store is not proof the live latch applied. No trigger read/RMW/poll.

[Bridge audit/captures/manifest](../audits/native-dpi-bridge.md) records final RAM
EN0->1/stage4/mask3/request1, cached config unchanged, MODE1/LP0/DPI19D0/DMAquiet,
IRQ_RAW0/PHY15BD/FIFO50015/errors0, four subsequent power9C replies with restored
controls. Host41/41/negative/sanitizer checks PASS. No pixel/output/latch-completion
claim; original firmware restored. No new pin/timing/rotation/PLL fact introduced.

### Native SRAM tile/LLI construction, DMA inactive — 2026-11-15

Pinned SDKf5c3654 [LLI layout/setters](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h),
[register types](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_struct.h),
[enums](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/include/hal/dw_gdma_types.h),
[map](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/reg_base.h),
[DPI initializer](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c):
64-byte item layout,64-bit source/master1 increment and DSI/master0 fixed50105000,
LLP physical/master1, count-minus1, control width/burst fields, valid/last31/30.
Original native MIT encoder; host-only SDK types oracle confirms fields/map.
Memory/cache ROM facts reuse TRMchip1.3v0.4 §7.3.1 pp451–454 and pinned ROM bindings
from [earlier DMA proof](../audits/native-vdma-transfer.md); no new cache API fact.

[Audit/captures/manifest](../audits/native-dpi-dma-tile.md) records one RAM load:
22,272byte owned NOLOAD reservation and NC-only fill/readback of11,520byte tile plus
160LLIs, hashes/guards match independent host model, peripheral snapshots unchanged,
DMA inactive and four power9C/error-free replies. Host42/42 plus sanitizers/mutants/
SDK oracle PASS. No engine descriptor fetch/transfer/live latch/pixel/PSRAM claim.

### Inactive solid channel/bridge staging and silicon observations — 2026-11-15

Pinned SDKf5c3654 [DMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
channel flow/handshake/burst/priority/outstanding setters, [register types](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_struct.h),
[bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h),
[bridge registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_bridge_reg.h):
contiguous fixed-source64-bit/M2P/DMAC/HW DSI0 config and bridge count7:4=1,
controller0, multiblock28off, threshold768/burst256. A4-bit direct count cannot
encode160; do not activate the earlier tile list without a verified new contract.
[Chip1.3 TRMv0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
register5.12 pp357–358 describes AWLEN range; final field15 verified. Exact cause
of16->15 readback is open. Host SDK types oracle checks register encoding only.

[Audit/captures/manifest](../audits/native-dpi-dma-stage.md) preserve a read fault
at optional status-fetch address50081148 despite SDK RW description (excluded now,
fetch/writeback off), strict AWLEN16 refusal/read15 and final stage18/maskD975
with source4FF10E80/CTL01C1B51/CTL1F8000/CFG1A020001/globalCFG0/channel0/outputoff,
unchanged memory/peripherals and four9C/error-free replies. Host42/42, sanitizers,
seven runtime mutants PASS. No activation/transfer/pixels or physical burst claim.

### Finite DSI DMA completion and qualified raw counter — 2026-11-15

Pinned SDKf5c3654 [DPI init](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c),
[host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
[bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h),
[DMA LL/registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h),
[TRMchip1.3v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
§5.5.7 pp337–338/§5.7.4 and reg5.27 p375: activation/disable/drain and documented
item-count interpretation. Actual independent SRAM byte-copy proof at widths8/64
and counts8/256/4096 yields rawN-1, contradicting literal wording; scope qualified
on this board, root cause open. No universal counter convention assumed.

[Frame audit/captures/manifest](../audits/native-dpi-dma-frame.md) preserves initial
count refusal and qualification's HOST FAIL (later byte-proof reevaluation), then
final active/natural-done/raw383FF/items38400, FIFO0/latched errors0, restored
command/LP/output/global/channel/GPIO and four9C replies.967 polls/13550us is one
diagnostic sample, not WCET/frequency. DMA-done still required for display. Raw
underrun at existing413-line discard threshold and no independent visual-red
confirmation: no all-pixel/continuous-refresh acceptance. Host42/42 + sanitizers/
negative checks PASS; no FreeRTOS runtime imported; original flash preserved.

## Bounded native DMA refresh and completion rearm — 2026-11-15

Pinned SDKf5c3654 [DW-GDMA registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h),
DMAC_CH1_INTCLEAR0_REG +198, WO completion/disabled bits0,1,3,4,30;
mask4000001B excludes errors. DSI/timing/counter sources unchanged from
[finite-frame audit](../audits/native-dpi-dma-frame.md), chip1.3 TRM v0.4
§5.5.7 pp337–338 and §5.27 p375. [Refresh audit](../audits/native-dpi-dma-refresh.md)
records400 finite DMA frames/399 checked rearms/92160000 items/errors0/recovery
and four9C replies; user explicitly ready before upload and confirmed stable red.
Single elapsed6109505us sample, not WCET/frequency/all-pixel coverage. Existing
underrun discard threshold413 unchanged. No foreign runtime; original flash kept.

## Two-block SRAM list fetch, writeback and source AXI length — 2026-11-15

Pinned [DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
and [registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h),
CFG0 LIST3/LLP64-byte/master1, CTL0 widths/source/destination increment controls,
CTL1 ARLEN_EN bit6/ARLEN14:7; [TRM chip1.3 v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
§5.5.4.2 pp334–336 LLI update/writeback/VALID clear, register5.12 p357 burst length,
§5.5.7 pp337–338 drain. [List audit](../audits/native-vdma-list.md) preserves automatic
ARLEN data refusal/actual32byte groups and exact successful copy after override
EN1/field0. SRAM LLIs receive count/status/VALID-clear writes even with peripheral
status-fetch bits0. Exact32768-byte proof+writeback/guard checks, no display or
universal burst-encoding inference. No SDK headers/runtime in target.

## Native five-node spatial display chain — 2026-11-15

Pinned SDKf5c3654 [bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h)
`set_multi_block_number`, [bridge registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_bridge_reg.h):
count5 at+88[7:4]/DMAC flow and multi-block+6C[28].
[DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
LLI/LIST3 and fixed/increment/source burst controls; [TRM chip1.3 v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
§5.5.4.2 pp334–336 LLI writeback/VALID clear/completion and §5.5.7 pp337–338 drain;
register5.12 p357 ARLEN. [Spatial audit](../audits/native-dpi-dma-spatial.md) records
400 finite five-block frames, exact per-node count/status writeback, rawE0FF,
guards/preserved source/tail/restored GPIO/commands and four9C replies. User
confirmed four stable large color bands after readiness; narrow eight-row strip
not identified. No all-pixel visual acceptance. No new runtime/header dependency,
no timing/rotation/flash/SD changes; elapsed6109911us is one diagnostic sample.

## Central32-row incremental-source tile — 2026-11-15

[Central audit](../audits/native-dpi-dma-center.md) records separate46848-byte NC
reservation, source720x32/list5, marker native rows624–655. Geometry/software-only
change uses previous [spatial primary sources](../audits/native-dpi-dma-spatial.md):
[SDKf5c3654 DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h),
[bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h),
[TRM chip1.3 v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
§5.5.4.2 pp334–336, §5.5.7 pp337–338, reg5.12 p357, §7.3.1 pp451–454.
No new MMIO/timing fact. Actual400 frames/complete five-node writeback/rawE0FF/
drain/errors0/recovery/preserved source4B6A57C5/guards/power9C PASS. Owner clarified
initial fault/other answer: stable aligned colored segments in center, as intended;
same image/run, no repaired-defect or repeat-test claim. Per-pixel/PSRAM/native
desktop not accepted. Elapsed6109902us sample only. No foreign runtime imported.

## P4 HEX/X16 PSRAM route and read-only controller state — 2026-11-15

[PSRAM audit](../audits/native-psram-state.md) pins SDKf5c3654
[HEX implementation](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c),
[P4 Kconfig](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/esp32p4/Kconfig.spiram),
[controller LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h),
register files reg_base/hp_sys_clkrst/spi_mem_s/spi1_mem_s/pmu/efuse and ldo_ll
unit mapping. Named register sections/fields detailed in audit; host oracle compares
all36 addresses/gate masks to unchanged pinned headers. Working boot says HEX/X16,
not Octal. Native read-only54 observations stable, inherited AXI_REQ_EN0; no array,
MR identity or cold initialization proof. No SDK headers/runtime in target. Host47/
47, sanitizers and failure guards PASS; original desktop recovered after RAM test.

## Native inherited HEX device identity — 2026-11-15

[Identity audit](../audits/native-psram-identity.md), SDKf5c3654
[HEX source s_get_psram_mode_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c),
[P4 command-port registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi1_mem_s_reg.h),
[controller LL common transaction](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h):
4040/16bit command/32bit MR address/read16 or8bits/dummy12 for preserved200M mode,
OPI_DTR/CS1. Native code bounded direct reads, no ROM driver call. SDK data-macro
oracle checks all ten addresses/masks/startbit. Actual8D30/A0DF/20/4B repeated,
AP/density7/32MiB/gen4/KGD6/X16 and exact controller/root restoration PASS. Busy
edge unobserved; poisoned receive value changed to valid repeatable device data.
No cold init/array/mapping/frequency or timing proof. Host48/48 and failure checks
PASS; original firmware recovered. No SDK runtime imported or device-mode writes.

## PSRAM warm XTAL clock qualification and VO2 bit correction — 2026-11-15

Primary register sources, pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[psram_ctrlr_ll](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h),
`_select_clk_source` XTAL0/MPLL1, `_set_core_clock_div` divide-minus1,
`_enable_core_clock`, `set_bus_clock` N/H/L encoding;
[hp_sys_clkrst_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/hp_sys_clkrst_reg.h),
PERI_CLK_CTRL00+30 source[13:12]/core-enable15/core-divider[23:16];
[spi_mem_s_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi_mem_s_reg.h), SRAM_CLK+50,
and [spi1_mem_s_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi1_mem_s_reg.h), CLOCK+14:
divide2=10001, divide4=30103. Native policy gates core before changing source,
selects XTAL with existing divide4, enables core, then sets both divide2. Restore
both divide4 before gating/restoring original source. This is native policy,
not a vendor guarantee about unmeasured clock transitions. Controller state is
not accessed while core gated. Qualified Tab5 ROM esptool XTAL40MHz supplies
nominal20MHz interpretation; no physical frequency measurement is claimed.

[ldo_ll](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/ldo_ll.h),
voltage_to_dref_mul unit1/1900 override, set_owner and adjust_voltage;
[pmu_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/pmu_reg.h),
EXT_LDO_P1_0P1A+1D0 / ANA+1D4;
[efuse_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/efuse_reg.h),
MAC_SYS2 VO2 DREF[31:28], MAC_SYS3 MUL[5:3]. **Correction:** VO2 software owner
bit7 is not XPD; XPD is bit8. Earlier identity code/audit described bit7 as power.
Actual previous target control40200180 had both enabled, so prior observed reads
remain valid. Native mode guard now checks XPD8; host negative rejects owner-only80.
Clock profile additionally requires owner1, select0, bypass0, nonzero calibration
pair matching analog DREF/MUL and qualified rev1.3/eFuse schema/error checks.
No VO2 programming, voltage measurement, cold power-up, PSRAM mode writes or
array/MMU/cache readiness are inferred. Source/mask oracle is host-only;
native build imports no vendor headers or runtime.

## Ported HEX PSRAM training-array transfers — 2026-11-15

Primary implementation [ESP-IDF mspi_timing_by_dqs.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/mspi_timing_by_dqs.c),
pinned f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0, Apache-2.0,
`mspi_timing_config_psram_write_data/read_data`: split into <=64byte FIFO chunks,
byte offset advances by chunk size; SYNC_WRITE8080 / SYNC_READ0000, command16 /
address32.200M configured device profile uses read26/write12 dummy bits. Source
[HEX implementation](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c)
mode fields fix MR0 lt1/readlatency4/drive0, MR4 write latency1, MR8 X16/burst2048.
[Controller LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h)
common_transaction selects OPI_DTR/CS1 on command port3. Adaptation retains that
algorithm/protocol and replaces SDK/ROM command engine with bounded native code.
[spi1_mem_s_reg.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi1_mem_s_reg.h)
USER MOSIbit27/MISObit28, MOSI_DLEN+24/MISO_DLEN+28 bits[9:0] length-minus1,
W0..W15+58..94 plainR/W FIFO. Host SDK oracle checks these facts; target imports
no SDK headers. No claim of a PSRAM device ACK from an idle command alone;
independent patterns/readback must establish actual data transfer. First4096
physical bytes are an explicit boot-exclusive training reservation, not an
inherited SDK heap allocation or assumed CPU virtual address. Warm powered modes,
DQS/DLL and nominal20MHz clock profile remain preconditions, not cold-init proof.

Correction during actual array testing: raw byte-valued command addresses while
MR8=4B (X16) alias CA10. Two refused images/captures retained. Samples show
read0..127 equals expected1024..1151, not random signal corruption. The small SDK
DQS-training algorithm is not a general linear-byte allocator for inheritedX16.
Primary [AP Memory APS256XXN-OBx9 datasheet](https://www.apmemory.com/en/downloadFiles/0325021716tb786912),
rev1.2 Jan3,2025, PDF p11 §§7.1–7.4/table: X8 byte vs X16 word addressing,
CA10 ignored in X16; p16 §7.7 Figures12/13: mode reads/writes use DQ7..0 even inX16,
MR8 is8bits and write latency1 independent of MR4; p21 Table19: MR8bit6 selects
X8/X16 and switching after power-up is supported. PDF downloaded from primary URL,
text extracted and pages11/16/21 rendered/visually inspected with macOS PDFKit
(Poppler absent). This is a protocol-family reference, not identification of exact
package part number. Native adaptation follows SDK CONFIG_SPIRAM_USE_8LINE_MODE:
claim exactMR8=4B, write0B via C0C0/address8/one-byte data/no dummy, verify readback,
then byte-addressed training only with private X8 claim/profile0B. Restore4B by
checked write/readback before restoring clock. No guessed address transformation,
reset opcode, clock delay, or adjacent-byte reads beyond the one-byte mode value.
Initial raw-offset tests do not prove a contiguous4KiB physical aperture or saved
contents restoration; they failed before restore and were reset. All PSRAM was
boot-exclusive, no live external-memory clients/flash/SD writes. Successful X8
mode+data+restore acceptance must come from subsequent actual device capture.

## PSRAM AXI guard register correction — 2026-11-15

Pinned [spi_mem_s_reg.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi_mem_s_reg.h),
CACHE_FCTRL+3C: AXI_REQ_ENbit0 / CLOSE_AXI_INF_ENbit31. Companion
[spi_mem_s_struct.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi_mem_s_struct.h)
mem_cache_fctrl offset3C vs mem_ctrl1 offset0C. Previous native mode/clock/array
code incorrectly used CTRL1bit0 as an AXI exclusion guard. Earlier documented
AXI-off inference was therefore not register-verified; actual byte tests remain
valid evidence, with caller-enforced boot-exclusive/no-live-client preconditions.
Current helper reads actual5008E03C and requires(mask80000001)==80000000, both
initially and during ownership checks, only with accessible clocks. Old CTRL1
bit0 restriction remains an additional warm-profile clock-mode restriction,
not an AXI claim. Snapshot's27 words unchanged; dedicated AXI before/final records
provide extra evidence. New actual full X8 pattern/mode/clock recovery run reports
C0000000 before/after and hostPASS; this validates actual closure for this run,
not a retroactive measurement of old captures. Host SDK address/offsetof/mask
oracle and negative actualAXI-enabled/open fixtures prevent repeating the mistake.

## Native PSRAM CPU-window port prerequisites — 2026-11-15

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[HEX config](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c),
s_config_mspi_for_psram/s_set_psram_cs_timing; [controller LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h):
command16/addr32, read26/write12 dummy for known200M latency mode, DDR/variable
dummy/no swap, OCT command/address/data, HEX data disabled for X8, AXI and splice;
CS setup4/hold4/ECC4/hold-delay3 (stored-minus1). [MSPI timing LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mspi_timing_tuning_ll.h)
pin_drv_set2 and enable_dqs true: twenty PSRAM pads only, no flash pads. Companion
[iomux_mspi_pin_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/iomux_mspi_pin_reg.h)
base500E1200, regular drive[13:12], DQS drive[16:15]/XPD0, phase/delay preserved.
[spi_mem_s_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi_mem_s_reg.h)
CTRL1+0C splice25/26, CACHE_SCTRL+40 lengths/phases, SRAM_CMD+44 OCT/HEX/dummy,
commands+48/+4C, DDR+D8, SMEM_AC+1A0, real CACHE_FCTRL+3C AXI enable/close.
FIFO+178 all six empty flags[31:26] must reachFC000000 under bounded polls.
PMS attrs+130..13C require RW3/ECC0, default PMS0 base0/+140 and size4096/+150;
SDK ECC source documents default region0..256MB. ECC_CTRL+174 address conversion
bit20 must be0. No unit guessed from register names; only exact qualified default.

[MMU HAL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mmu_hal.c)
map-region algorithm and [MMU LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mmu_ll.h),
[ext_mem_defs](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/ext_mem_defs.h):
64KiB pages, per-PSRAM table INDEX5008E380/CONTENT5008E37C, value physical-page |
PSRAM-valid bit11 | access-PSRAM bit10. Proposed fixed native reservation physical
[0,2MiB) maps VADDR4B800000..4BA00000 via slots896..927; this geometry is native
policy, not an inherited SDK heap address. [eFuse](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/efuse_reg.h)
REPEAT_DATA1+34 SPI_BOOT_CRYPT_CNT[20:18], odd popcount enabled per efuse_ll/MMU LL;
unimplemented encrypted path refused, no sensitive-bit mapping assumption.

[MMU/cache HAL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/cache_hal.c),
[ROM cache declarations](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_rom/esp32p4/include/esp32p4/rom/cache.h)
and [ROM bindings](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_rom/esp32p4/ld/esp32p4.rom.ld):
ROM_Boot_Cache_Init4FC003DC as RAM-app mmu_hal_init does; L1D enable4FC004F4 and
L2 enable4FC00504, autoload0, Get_Mode4FC003D0. Get_Mode struct size/line/ways/type
layout and cache-type5 query checked from header. WB+invalidate4FC003FC and
invalidate4FC003E4 use map30 (L1D bit4/L2 bit5) on ONLY owned window. These are
immutable chip ROM services, not SDK/FreeRTOS runtime; no ROM SPI helpers/callback
interfaces. Global cache/flash-MMU bootstrap is a boot-lifetime ownership change,
not undone by window_stop; no flash storage write. ROM internal wait WCET is not
established; CPU bus faults/stalls need actual target diagnostics/reset recovery.
[SDK linker layout](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_system/ld/esp32p4/memory.ld.in)
keeps lowSRAM4FF00000..4FF2CBD0 independent of cache size, ROM reserve below
4FF40000, high SRAM ends4FFC0000 minus cache size. Thus supported128/256/512KiB
L2 cache layouts exclude the qualified native low160KiB window; do not infer
S3-style cache allocation at lowSRAM start. No extension of RAM-load window.

Controller/MMU/cache integration is now HARDWARE-TESTED in the exclusive warm
boot profile: all524288words over2MiB matched FNV2E4E04FE; direct SPI read
after cache sync independently matched4096bytes. See [window audit](../audits/native-psram-window.md)
for exact restoration boundaries, actual capture and remaining startup/speed gates.
No generic nocache alias for PSRAM inferred from L2MEM-only cache_ll macros.

## PSRAM DMA source qualification — 2026-11-15

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
master1 supports L2MEM/ROM/MSPI Flash/PSRAM, ordinary mapped address used as SAR;
source/destination increment, width64 enum3, source MSIZE1/4/16 enum0/1/3.
[registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h)
CTL0+18 widths[10:8]/[13:11], sourceMSIZE[17:14], CTL1+1C ARLEN_EN6/
ARLEN[14:7]. Explicit candidate fields0/3/15, NOT automatic ARLEN; encoding is
qualification input, not yet assumed hardware proof. Native test requests4096B
from three offsets0/65408/1FF000 within the owned2MiB window, after window sync;
known deterministic full-window pattern provides independent byte oracle. Middle
source crosses a64KiB page. Dedicated4352B SRAM aligned128,128B guards; ROM sync
only this owned internal region then existing L2MEM NC alias+40000000. No PSRAM
nocache alias inferred. Save/restore12 channel configuration words; raw/status
progress and intentional status clears are not restored. MIPI clocks/peripherals/
other channels preserved; disable/drain before releasing source/destination.
Boot-only/all clients stopped, no IRQ/task/SMP integration. Actual9x4096B target
copies all matched, including page crossing/tail; fields0/3/15 with MSIZE1/4/16
are now qualified for these memory transfers. See [DMA audit](../audits/native-psram-dma.md)
for capture, restoration and performance limits.

## Native operational PSRAM clock port — 2026-11-16

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[analog-I2C patch](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_rom/patches/esp_rom_regi2c_esp32p4.c)
regi2c_enable_block/read_impl, Apache2 port: analogmaster0, slave63/address2,
WR_CNTL24=0/read, BUSY25, data[23:16]; routing conf1/2 masks00FFFFFF, selectMSPIbit9.
Native driver requires existing clock/resetidle, saves/restores both routing words,
checks both mastersidle, bounded waits/two repeated reads, no analog write command.
[regi2c_mpll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/regi2c_mpll.h)
DIV[7:3]/REFDIV[2:0]; [clock LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_tree_ll.h)
clk_ll_mpll_get_freq_mhz formula40*(div+1)/(refdiv+1), only reliable powered.
Power check PMU_RF_PWC+15C MSPI_PHY_XPD24, LP_CLKRST_HP_CLK_CTRL+40 MPLL_EN28;
HP_SYS_CLKRST_ANA_PLL_CTRL0+BC MSPI_CAL_END8/STOP9 required, never programmed.
[register map](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/reg_base.h)
LPAON50110000, LPPERIPH50120000; analogI2C50124000, conf1+1C/conf2+20;
LPPERIclock+0 I2CMST27/reset+8 I2CMST26, PMU50115000, LP_CLKRST50111000.

[PSRAM controller LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h)
_select_clk_source MPLL=1/XTAL=0, corediv storedminus1, busclock div2=10001 /
div4=30103. Native policy: genuineXTAL20/X8claim and closedAXI; accept only existing
MPLL400, use corediv1 and busdiv4=100MHz or2=200MHz. Gatecoreoff before mux; slower
source before lowering divisor on return. No PLLpower/config/calibration or flash
clock changes. These are calculated nominalrates, not measuredclock frequencies.
[HEX device setup](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c)
200M latency profile readLC14/regLC7/writeLC7 already validated by privateX8claim;
window port retains DQS/padphase inherited from warmdevice. Actual CPU/cache/raw
and DMA data qualification required at newrate; no cold/DQS-tuning claim yet.

Operational-clock qualification found two retained refusals: DQS pads must be
prepared before clock selection (SDK order), then phase0 at200MHz still produced
persistent wrong bits during16-item DMA, despite fullCPUdata match. Samples show
bit1/2 differences with correctCPUreads, not merely late destination visibility.
Now adapt [DQS phase algorithm](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/mspi_timing_by_dqs.c)
get_psram_tuning_phases/select_best_tuning_phase and
[LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mspi_timing_tuning_ll.h):
four phases67.5/78.75/90/101.25degrees encoded0..3, both DQS pinsPHASE[2:1].
Native sweep initializes/verifies reference2MiB at20MHz first, then read-onlyDMA
9address/burst cases at each phase, requires safe completion/config/windowrestore
before moving to next candidate. Byte mismatch is training evidence only; control/
guard/stop errors remain fatal. Select first phase of longest passing consecutive
run, no zero-pass fallback. Final fullCPUwrite/read/DMA/raw recovery must pass.
This is phase qualification, not full SDK31-point delayline calibration, coldboot,
temperature/voltage margin or productionconcurrency acceptance. PrivateX8claim
already validates profile at20MHz; fast mapping uses that exclusive claim rather
than issuing mode commands at deliberately unqualified training phases.

Four-phase sweep initially found no complete passing phase (all6/9copies).
Review of psram_ctrlr_ll_enable_dll revealed another required SDK startup step:
MSPI2 AXI uses SPIMEM2.smem_timing_cali+190bit5; MSPI3 command DLL uses
SPIMEM2.mem_timing_cali+180bit5, **not** only MSPI3 ownD4 register. Both actual
warm+180/+190 words were1 (DLL disabled), so padphase changes alone were not full
sampling setup. Fast clock ownership now enables both bit5 and restores exact
saved words after XTAL return. Snapshot model updated only after guarded readback;
window saves/restores the enabled DLL profile, then clock stop restores originals.
Independent pinned SDK struct-offset/bit oracle and dropped-store tests cover both.
No guessed wait/lock bit, PLL adjustment or PSRAM mode write added.

Final actual200MHz qualification now PASS: bothDLLwords1->21->1, MPLL99 twice/
400MHz, four phasecandidates each9/9copies, final2MiB CPU hash2E4E04FE and9/9DMA
copies. All45copies byte-exact, guarded cleanup/mode/clocks/window restored and
desktop verified. See [operational audit](../audits/native-psram-fast.md) for
four retained refusals, source hashes, full capture and remaining warm-only limits.

## Full native PSRAM framebuffer transfer — 2026-11-16

Pinned SDK f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
master1 reads MSPI PSRAM, master0 writes DSI FIFO50105000. Contiguous single block,
64bit/increment source, sourceMSIZE16 enum3, explicit ARLEN_EN/field15 (qualified
in native200MHz M2M). Destination fixed256items/AWLEN15 retains earlier display
path. Register plan CTL0=1CDB41 / CTL1=F87C0,230400items/full1843200B frame. No LLP.
[registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h),
sourceSAR can advance only within owned aligned frame span; before each rearm,
verify previous recovery/idle, clear only qualified status flags, rewind SAR.
Bridge single-block controls, status/DMA-done/rawN-1/drain/recovery unchanged from
finite solid-frame path; no relaxed display completion or new unverified status.

Fixed native policy: framebuffer4B800080..+1843200 in owned2MiB window,128B prefix
and all unused tailA5 guards; cache writeback/invalidate before private DMA claim.
Window sync/stop refuses while DMA owns source; release requires live memory guard,
MMU table and actualDMA clock/disabled/drained checks. Caller immutability/exclusive
boot still mandatory; no thread/SMP/ISR/allocator safety claim. Public legacy solid
API keeps SRAM-only bounds; framebuffer path is separate private diagnostic.

Production reference `platforms/tab5/main/hal/hal_esp32.cpp` sets software
LV_DISPLAY_ROTATION_90. Vendored [LVGL draw rotation](https://github.com/lvgl/lvgl/blob/v9.2.2/src/draw/sw/lv_draw_sw.c)
rotate90_rgb565 maps logical(x,y) to native(y,width-1-x); actual local source and
version checked during implementation. Test picture uses inverse logicalx=1279-
nativey/logicaly=nativex, with four large numbered red/green/blue/yellow corners,
white grid/border and central six-color rectangle. Native720x1280/logical1280x720;
no panel MADCTL/production timing change. Host corner/mapping/hash tests precede
hardware/visual proof. Electronic4frame run and visual1000frame run are distinct;
visual requires fresh owner readiness. New integration pending actual target.

First actual electronic integration completed all4 full frames with hashE3D71BEF
unchanged, guards retained, raw383FF/items921600/DMAstatus12/errors0/drain/release1,
3checked SARrewinds, total65383us. Nine preliminary PSRAM-to-SRAM copies also
byte-exact. Native memory/mode/clock restoration and fourRDDPM9C replies passed.
Initial host evaluator incorrectly required whole DPI control register0; actual
inherited/returned disabled value19D0 preserves configuration with enablebit0clear.
Evaluator now compares exact originalDPI_BRIDGE_IDLE word and checks enable0.
Same recorded device output before host reset/status footer re-evaluatesPASS;
original raw log/hostFAIL retained, no synthetic log or hardware rerun. Visual
acceptance remains pending fresh owner readiness; no visible-picture claim yet.

Actual prolonged1000frame run now strictPASS:230400000items/999rearms/15282355us,
all errors0/sourcehashE3D71BEF unchanged, guards/memory/mode/clocks/MMU recovered.
Fresh owner readiness then qualitative complete/stable/correct orientation
confirmed. See [framebuffer audit](../audits/native-framebuffer.md) for exact
logs, original electronic verifier refusal, source/runtime gates and limits.

## Native DW-GDMA completion interrupt — 2026-11-16

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[interrupt sources](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/interrupts.h):
ETS_DW_GDMA_INTR_SOURCE=24, ETS_MAX_INTR_SOURCE=128. [Core0 matrix](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/interrupt_core0_reg.h)
GDMA_INT_MAP=500D6060,6bit CPUline;128 map words contiguous. Native single-owner
policy selects the first free CLIC17..47 after scanning every other producer and confirming
line disabled/nonpending and DMAidle; restores exact previous route/control.
CLIC register layout/level/nonvectoredM-mode uses existing verified timer-port
facts; [soc_caps](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/soc_caps.h)
provides32 CPUinterrupts. CLIC16 remains the native timer; selection17..47 is native policy, not an SDK allocated handle.
[DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h):
globalCFG bit1 INT_EN, channel INTSTATUS_ENABLE0+80 generation versus
INTSIGNAL_ENABLE0+90 propagation. Native ISR masks propagation before waking
worker; completion/source release still require actualcount/done/drain guards.
No SDK IRQ allocator/runtime linked. Task/IRQ/stop target validation is recorded in the native LCD stream audit.

Native async LCD engine/service: start publishes one hardware frame under local
IRQ exclusion, returns immediately; DW-GDMA ISR masks globalINT_EN bit1, latches
status predicate and notifies native owner. Worker verifies actual done/raw383FF/
naturaldisable/FIFOs and qualified recovery before rearm, with IRQs enabled during
hardware wait. Residual drain uses bounded-deadline1tick sleeps, not a frame-long
spin. Unknown/unclaimed externalIRQ remains fatal; ownedIRQ uses existing trap
context and scheduler epilogue without advancing timer ticks. Service uses native
coalescing notifications plus predicate,100tick frame timeout and32poll error
cleanup. Source remains immutable/claimed until guarded actual stop/release.
No heap/printing/cache maintenance in ISR. Single-session warm native service;
source updates/reinitialization/SMP/margin qualification are separate open gates.


Native LCD publication/reuse (software ownership extension, no new MMIO facts):
CPU edit lease freezes window sync/stop/release while the owner draws with IRQs
and scheduling enabled. Begin requires the already qualified channel/controller
idle and MMU/table guards; publish uses the existing pinned ROM cache writeback
binding and bounded MSPI drain before returning the lease to DMA. Cache ABI,
range/line and MMU facts remain those cited for the native PSRAM window. Warm
prepare after clean stop reuses the checked DMA configuration, restores its IRQ
signal mask and reacquires the proven-free matrix/CLIC route; it does not reset
or cold-initialize the panel/PSRAM. Prior failed or uncertain state stays refused.
Evidence: [LCD lifecycle audit](../audits/native-lcd-lifecycle.md).


Native ST712x contacts / concurrent I2C — 2026-11-16:
[Sitronix TDDI Interface Protocol A v01.11,2023-12-25](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/ST7123-TDDI-Interface-Protocol-V01.11.pdf.pdf),
local `ST7123-TDDI-Interface-Protocol-V01.11.pdf.pdf`: p9 section5.3.2 sequential
16bit high/low-pointer read with STOP before data START; pp12-13 report map
(maximumX0005/6,maximumY0007/8,slots0009,advanced0010,7byte slots0014..0059),
p18 section6.6 reading the final supported coordinate register acknowledges the
report; no-coordinate bit means no fresh coordinate report, not finger release.
Valid flag Xhighbit7, X/Yhigh6bits, stable ID is slot index; p20 XY valid/area,
p21 section6.7 read-only advertised maxima/capacity. Proximity, gesture/checksum
modes are outside this native subset; reset/proximity-data flags are refused.
Read all advertised slots (1..10), never just first active finger. Geometry maps
native(x,y) to logical(1279-y,x) for90degrees, inverse of qualified framebuffer.
Out-of-raster coordinates are rejected, never clipped into a false edge touch.

Adapted primary implementation:
[Espressif ST7123 driver](https://github.com/espressif/esp-iot-solution/blob/1f855ac00e3fdb33773c8ba5893d4f98f1d9b576/components/display/lcd_touch/esp_lcd_touch_st7123/esp_lcd_touch_st7123.c),
Apache-2.0,2023-2024 attribution preserved in `drivers/input/st712x.c`; local
component1.0.0 pins that revision. Ported report/capability algorithm only: no
FreeRTOS locks/delays, SDK APIs, bitfield ABI, allocation or foreign runtime.
M5GFX `Touch_ST7123.inl` at28d47949bdc84f30881623476f555398e2194313 was reviewed
as corroboration, no code copied from it. ST7121 FW1 sharing remains limited to
the supplied BSP and actual device qualification, not a claim of all ST7123
silicon modes. Register/document hashes recorded with recovery evidence.

Native bus adoption keeps existing GPIO31/32/reset/power state. Core0 service
serializes callers with short IRQ-protected try-ownership; wire reads run with
IRQs enabled. Only UNIT0 capture is locally IRQ-excluded to avoid interleaved
snapshots by native timer/time clients; no timer-clock reconfiguration in tasks.
Existing GPIO/timer facts from the touch-identification audit apply. Concurrent
ISR/SMP/multi-master access and automatic recovery pulses are not supported.
Latest-state input snapshots preserve held contacts on no fresh report, invalidate
on errors/stale polling and recover only from a complete fresh report. They are
explicitly not a lossless gesture/event queue. Target integration evidence:
[touch/LCD audit](../audits/native-touch-input.md).


Touch/LCD physical checkpoint2026-11-16: [actual capture/owner confirmation](../audits/native-touch-input.md)
qualifies ST7121/fw1 on this Tab5:10slot/70byte reports under native preemption,
1148polls/826reports/1974bus transactions, ten downs/ten releases/96moves/four
corners, no bus/decode/controller errors.827image publications,1211actualDMAIRQs
and20048timerIRQs. Owner confirms cross alignment/following/release disappearance.
This supersedes the prior NOT RUN physical interaction status, not cold/SMP/
multi-finger/all-controller/latency/tear-free/full-native-desktop acceptance.


Native input/bus warm lifecycle2026-11-16 (software-only ownership extension,
no new MMIO facts): three touch stop/capability revalidation/restart and bus
close/reopen cycles alongside102LCDframes pass. Closed bus/poll refusal and
snapshot invalidation are verified; no hardware reset/pin/clock power-down
implied. [Audit, inherited startup inventory and primary algorithm comparison](../audits/native-input-lifecycle.md).
[Pinned SDKf5c3654 PSRAM reference](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c#L368)
`esp_psram_impl_enable` (Apache-2.0) includes MPLL start/frequency ownership, full
mode configuration and tuning beyond the existing native warm-qualified subset.
Native cold startup remains open; these facts do not authorize guessed register
values, warm-guard relaxation or SDK runtime/header linkage.


## Native MPLL programming and power cycle — 2026-11-16

Primary SDK revision f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[rtc_clk.c MPLL configure](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/port/esp32p4/rtc_clk.c#L597)
starts calibration, applies config, waits for end, stops calibration.
[clk_tree_ll MPLL power/config](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_tree_ll.h#L154):
PMU_MSPI_PHY_XPD bit24 at PMU_RF_PWC5011515C powers the MPLL circuit; LP HP MPLL
gate bit28 at50111040. Disable/enable circuit is distinct from PSRAM LDO2 supply.
Same file lines445–459: slave63/reg3 DHREF bits5:4 OR3, reg1 RSTB bit5 low/high,
reg2 divisor (div<<3)|refdiv, fixed400MHz/40MHz gives99.
[regi2c_mpll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/regi2c_mpll.h)
confirms fields; [regi2c_ctrl_ll calibration](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/regi2c_ctrl_ll.h#L81)
uses HP_SYS_CLKRST+BC CAL_STOP9/CAL_END8. Native waits are bounded rather than
vendor unbounded loops. [Analog patch](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_rom/patches/esp_rom_regi2c_esp32p4.c#L157)
encodes slave7:0/address15:8/data23:16/WR24/BUSY25, matched by native readback.
[Clock definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/clk_tree_defs.h)
identify MPLL PSRAM/F25/F50 consumers: all must be quiescent during the boot port;
this does not provide a runtime shared-clock manager. Native DSI uses qualified
SPLL-derived20MHz reference, not F25, in this diagnostic.
Apache2 source attribution retained. Independent official-header oracle verifies
addresses/masks; source byte hashes pinned in recovery. [Actual calibration,
refusals and downstream PSRAM/DMA/LCD evidence](../audits/native-mpll-initialize.md).
This qualifies native powered-domain circuit restart/programming, not cold
PSRAM supply/device startup or a measured clock waveform.


## Native PSRAM mode profile — 2026-11-16

[AP HEX implementation at pinned f5c3654](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c#L138),
Apache-2.0, s_init_psram_mode_reg plus the field struct/constants at lines21–112:
200M fixed read latency4/drive0 gives MR0 masked3F/value30; write latency1 gives
MR4 maskedE0/value20; burstlen3/type0/rbx1/X16 gives MR8=4B (X8=0B). Read command
4040, writeC0C0,16bit commands/32bit address, register writes without dummy.
Vendor uses16bit MR0 and MR4 writes and16bits starting at its byte MR8 object;
native port uses explicit2byte MR0/MR4 buffers and the previously qualified
one-byte MR8 writer to avoid an object over-read. It obtains MR4/MR8 bytes with
separate qualified reads, without assuming paired-read upper-byte behavior.
Masked fields and unowned MR0/MR4 bits preserved; no arbitrary register API.
The oracle compiles the exact pinned vendor struct and confirms30/20/4B/0B.
[Actual programming/readback, original-profile recovery and downstream DMA/LCD](../audits/native-psram-profile.md)
pass on this powered AP32MiB/X16 entry; initial fields already match the target,
so differing/default-field startup remains host-only evidence. Supply/reset/cold
entry is not inferred from same-value device readback. No SDK runtime/header.

## Native PSRAM VO2 configuration — 2026-11-16

Primary ESP-IDF revision f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0, Apache-2.0:
[Kconfig.ldo](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/port/esp32p4/Kconfig.ldo)
reserves channel2/1900mV for PSRAM **and MPLL**. This supply must not be treated
as a PSRAM-only reset line. [ldo_ll.h, voltage/owner/enable/ripple helpers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/ldo_ll.h#L70)
maps channel2 (unit1) to PMU.ext_ldo[3]; uses dedicated nonzero VO2 DREF/MUL
eFuse calibration for1900mV. Native implementation refuses absent calibration
instead of importing the SDK nominal-coefficient fallback. The SDK acquisition
[sequence](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/ldo/esp_ldo_regulator.c)
programs voltage, owner, ripple suppression and enable. No runtime/locks imported.
[PMU register layout](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/pmu_struct.h#L700)
places VO2 control/analog at PMU+1D0/+1D4: force7/XPD8/source11:9/bypass14;
analog MUL25:23/VDET26/DREF31:28. Masks4F80/F7800000 preserve all other fields.
[eFuse layout](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/efuse_struct.h#L618)
has VO2 DREF MAC_SYS2[31:28], MUL MAC_SYS3[5:3]. Existing rev1.3/schema/RS guard
retained. Off-supply configuration requires both PSRAM core/PLL gates off and
MPLL circuit/output off using previously pinned clock/power facts above. Digital
readback does not establish voltage/discharge/settling or device reset/defaults.
No power-off primitive or guessed time delay is introduced by this port.


## Native PSRAM/MPLL consumer parking — 2026-11-16

Primary pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[pmu_sleep_start](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/port/esp32p4/pmu_sleep.c#L372)
waits for memory clients, selects XTAL, gates PSRAM core/module and disables MPLL
before the deep-sleep VO2 power-off path. Native scope stops short of VO2 writes.
[psram_ctrlr_ll](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h#L292)
maps core gate to PERI_CLK_CTRL00 bit15, PLL gate bit14. Native changes those
only, retaining the shared register-bus gate SOC_CLK_CTRL0 bit31. No controller
access with core gated. [clk_ll_mpll_enable](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_tree_ll.h#L154)
sets circuit power then output gate; the existing RF_PWC bit24/LP_HP_CLK_CTRL
bit28 definitions above are used with unrelated fields preserved. Native park
adds explicit output gating before circuit off, reverses on resume, restores
XTAL core last, then invokes native MPLL initialization/calibration before use.
Power readback alone is not PLL-ready or PSRAM retention evidence.

[Official Tab5 schematic](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/Tab5_Schematics_PDF.pdf),
5 sheets, inspected sheet1 on2026-11-16: ESP_LDO2 connects U1 pin72 VO2 and pins
59/67 VDDPST, with C15=1uF and C18/C19=1uF/100nF to GND. Flash uses separate
ESP_LDO1. The owner's docs/hardware/Tab5.pdf is a17-page product documentation
export linking this schematic; it is not itself the5-sheet circuit drawing.
[Local ESP32-P4 datasheet v0.5](datasheets/esp32-p4_datasheet_en.pdf), printed
pages29-30/sections2.6.1-2.6.2, identifies59/67 as PSRAM supply and72 as output,
VO2 nominal1.9V. These documents do not establish the fitted in-package AP
memory discharge/initialization timing or a readable rail-off/ready indicator.
Generic AP Octal device timing is not assumed valid for this HEX/SIP part.

## Native off-supply boot composition — 2026-11-16 (HOST-TESTED)

Same pinned SDK f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0, Apache-2.0 sources;
native source notices retained, no vendor headers/runtime linked. Existing VO2,
PSRAM gates/dividers and MPLL facts above are reused. New analog-master setup:
[regi2c_ctrl_ll.h lines24-50](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/regi2c_ctrl_ll.h#L24)
enables LPPERI.CLK_EN.CK_EN_LP_I2CMST, releases RESET_EN.RST_EN_LP_I2CMST and
selects I2C_ANA_MST.CLK160M.CLK_I2C_MST_SEL_160M. The
[P4 bootloader hardware setup](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/bootloader_support/src/esp32p4/bootloader_esp32p4.c#L96)
calls master enable/configure before analog programming. Native releases an
already held reset, without pulsing a live shared master; all analog clients
must be excluded. [LPPERI definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/lpperi_reg.h)
give50120000 bit27 clock enable and50120008 bit26 reset.
[I2C analog master definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/i2c_ana_mst_reg.h#L268)
give CLK160M at50124034/bit0, initial default0; native selects1 and preserves all
other bits. It reads this gated peripheral only after clock/reset access exists.
No claim is made to initialize the whole CPU/CPLL clock tree from no ROM support.

Pinned [AP HEX s_init_psram_mode_reg](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c#L138)
reads/writes modes and selects the requested MR8.x16 field; native boot ownership
therefore permits a readable X8 starting mode before selecting targetX16. Warm
entry retains its prior X16 requirement. Unknown MR8 fields/identity/protocol are
still refused. MPLL configure writes divisor99 without requiring it already99;
only the private boot claim admits that transition, retaining strict warm recovery.
These are source/host-qualified adaptations, not physical cold/default proofs.
Seven source hashes and sanitizer/mutation checks preserved with
[native boot audit](../audits/native-psram-native-boot.md). Operator-synchronized
power/button acceptance remains NOT RUN by owner instruction; no guessed timing.

## Native PSRAM order correction and warm refusal — 2026-11-19

The full RAM-loaded diagnostic now attempts native supply/clock/MPLL/device
initialization before cache bootstrap. Host tests verify this ordering and
reject an inverted capture. CMake keeps the `load_ram` probe on `linker.ld`
even when the product-XIP option is enabled. The final default-option image
(97,760 bytes; SHA-256
`903234a96dbbb00a6458490e399c3fb52850786f016e45f820e0ba9cb67a0ecc`) passed
RAM-window validation and the no-foreign-runtime-symbol gate.

On the Tab5, the automatic warm-reset capture passed MIPI/VO3, DSI, ST7121/panel
and DPI bridge setup. At `DPI_FB_READY`, native boot returned
`MAIA_PSRAM_BUSY` (`-5`, stage0, zero writes). No cache-bootstrap marker or
downstream memory/DMA/LCD/touch acceptance was reached. The runner returned
`FAIL / NOT DEMONSTRATED` and invoked reset to the preserved flash application.
No flash image was written. Because this was not a physical power cycle and no
call-time PSRAM register snapshot was captured, the result is only evidence of a
safe refusal in the tested warm state; it is not cold-start evidence.

Exact image, ELF, linker map, CMake cache and capture:
[`default-product-option/`](../../../maia-rtos-recovery/native-psram-native-boot-order-20261119/default-product-option/).

## Native PSRAM entry-guard diagnostic — 2026-11-20

The boot report and RAM probe now preserve/print the snapshot used by the
entry guards and identify failed predicates. Automated host coverage exercises
all hardware-entry guards, software clock/X8 claims, duplicate/prior failure,
and the conditional MPLL-busy refusal. Host CTest passed 86/86. The RV32
diagnostic passed the native runtime-symbol gate; esptool 4.12.0 packaged the
RAM image, and runner `--validate-only` confirmed all load segments and the
entry point are within L2MEM_LOW.

The automatic 30-second Tab5 warm-reset run reached `DPI_FB_READY`, then
returned `MAIA_PSRAM_BUSY` (`-5`), stage 0, zero boot writes. Its stable
read-only snapshot reports `guards=00000500`: `VO2_ENABLED` (`0x100`) and
`DIVIDER_NOT_XTAL` (`0x400`). Corresponding observed values were VO2
`0x40200180` and PSRAM root word 1 `0x0000D00C` (native guard mask
`0x00FF3000`). AXI was `0xC0000000` and passed the quiet predicate; sampled
controller and eFuse predicates passed. Register naming/layout follows the
pinned ESP-IDF `f5c3654` sources documented in the
[native PSRAM state audit](../audits/native-psram-state.md).

No cache bootstrap, PSRAM array transfer, DMA, LCD stream or touch validation
followed. The runner issued a reset to the preserved flash application; it did
not write flash. This was not a physical power cycle and is not cold-start
acceptance. Image (99,008 bytes) and actual UART capture:
[`native-psram-entry-diagnostics-20261120/`](../../../maia-rtos-recovery/native-psram-entry-diagnostics-20261120/).
Image SHA-256:
`67ccae80de64d55fc5ec9340ac71d4248c2f78045700271e3f735b36178e06ac`;
capture SHA-256:
`f2dcd2494f89227e6c96486534cd17bf10b1eaf56b1ad624c7694ee198391607`.

Validation record: macOS host build with AppleClang 21 and Python 3.14.5 used
`cmake --build /tmp/maia-rtos-diagnostic-build -j4` and
`ctest --test-dir /tmp/maia-rtos-diagnostic-build --output-on-failure`
(86/86 passed); the focused `psram_boot_test` also passed. The public header
compiled with the ESP32-P4 GCC 14.2 C++98 cross-compiler. From the repository
root, the device probe build command was
`cmake --build ../maia-rtos-recovery/native-psram-entry-diagnostics-20261120/build --target maia_sched_probe_ram -j4`;
its post-link runtime gate passed. esptool 4.12.0 packaged the image, and the
runner's image/entry-point `--validate-only` check passed. From the recovery
directory, the target run was
`run_native_scheduler_probe.py --port /dev/cu.usbmodem1301 --image maia_sched_probe_ram.bin --log native-psram-entry-hardware.log --capture-seconds 30 --psram-native-boot`;
it returned the expected `FAIL / NOT DEMONSTRATED` because the native
initializer safely refused this warm entry.

## F1 Newlib/gthread runtime smoke — 2026-11-20

The separate `maia_f1_runtime_probe_image` target uses the Maia scheduler,
1 kHz SYSTIMER, console and Newlib/gthread runtime; it does not initialize the
display, touch or PSRAM. Two worker tasks each run eight rounds. They verify
task-local `errno` against distinct `__getreent()` pointers, call
`clock_gettime(CLOCK_MONOTONIC)` and `nanosleep`, contend on `std::mutex` while
delaying inside the critical section, exercise `calloc/realloc/free`, and
set/get separate `pthread_key_t` values. Key destructors are checked on task
return. The report task verifies that an empty libc `read(STDIN_FILENO)` returns
`EAGAIN`, then emits the result using libc `write`.

The 102,752-byte P4 image (entry `0x40000020`) passed esptool 4.12.0 packaging
and the image structural/SRAM-window validator (`0x4FF00000..0x4FF28000`).
The linked-symbol scan found the expected Newlib/gthread/time functions and no
FreeRTOS symbols. Image SHA-256:
`a7c0048c6edcebf7a787a893f68a36dcd164cc3884f368b27807fbbe5e9e42f8`.

The initial Tab5 boot and a separate three-boot repeat all emitted
`MAIA_F1_RUNTIME_PASS`; none had a fatal/stack-fault marker. Both runs restored
and verified the original app backup. Its SHA-256 is
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`; the
restore capture reached `msh>` and loaded `desktop.mobj`. The repeat report,
image, captures and backup are in
[the recovery directory](../../../maia-rtos-recovery/native-f1-runtime-probe-repeat-20261120/);
see its [report](../../../maia-rtos-recovery/native-f1-runtime-probe-repeat-20261120/report.json)
and [restore capture](../../../maia-rtos-recovery/native-f1-runtime-probe-repeat-20261120/restore-boot.log).
The initial single-boot artifacts are in
[the initial recovery directory](../../../maia-rtos-recovery/native-f1-runtime-probe-20261120/).

Host `newlib_syscalls_test`, `newlib_reent_test` and `gthread_test` passed (3/3).
Both `maia_f1_runtime_probe_image` and the existing `maia_f0_flash_probe_image`
packaged and passed structural validation. `flash_boot_probe.ld` now defines
explicit flash/SRAM program headers and collects C++ exception tables into
DROM, needed for this C++ runtime probe.

Limits: hardware exercised only the no-input `_read` case; successful input
was not tested on the device, and filesystem-related syscall stubs and broader
syscall error paths remain unvalidated. This does not
validate compiler `thread_local`/ELF TLS, C++ static constructors or guard
variables, `std::thread`, or reent slot reuse after task-ID recycling. An
earlier exploratory variant that included the probe-only smoke source in the
product ELF stopped at the linker assertion
`PSRAM cache adoption must execute from internal SRAM`; the guard was not
changed. The canonical product target with its normal runtime sources now
links and packages successfully, as recorded below; product device boot, cold
boot, display, touch and PSRAM acceptance remain open.

Build command from the repository root:

```sh
cmake --build /tmp/maia-rtos-f1-probe \
  --target maia_f1_runtime_probe_image maia_f0_flash_probe_image -j4
```

## Canonical product XIP link with F1 runtime — 2026-11-20

The normal product target, whose source list includes `newlib_locks`,
`newlib_heap`, `newlib_reent`, `newlib_syscalls` and `gthread`, now builds and
packages successfully:

```sh
cmake --build /tmp/maia-rtos-f1-runtime --target maia_m0_image -j4
```

`maia_m0.elf` linked with `MAIA_USE_FLASH_PRODUCT_LINKER=ON`; the
`maia_m0_image` target passed esptool 4.12.0 and
`f0_flash_image_test.py`. The image is 804,144 bytes, entry `0x40000020`,
five segments, app descriptor `0x400B0020`, valid checksum/hash, validation
hash `99a22f67d9697c839ac949daeafb326391d4ccf29227620b67489a5fe821d650`.
Image SHA-256:
`e9615cf79c2f1c910bbc5581896b8bd7f4db832a2f223081554850e44ca97f3d`.
ELF inspection confirms `__getreent`, `_read`, `_write`, `clock_gettime`,
`nanosleep` and `pthread_mutex_lock`; the guarded
`maia_esp32p4_psram_cache_adopt` symbol is at `0x4ff03b1c` in internal SRAM.
The product ELF symbol audit found zero matches for the common FreeRTOS API
families (`xTask`, `vTask`, `uxTask`, `xQueue`, `vQueue`, `xSemaphore`,
`vSemaphore`, `xTimer`, `vTimer`, `pvPortMalloc`, `vPortMalloc`,
`xPortStartScheduler`, `portYIELD`, `heap_2/3/4`) and runtime symbols
(`esp_timer`, `esp_event`, `esp_log`, `esp_system`, `esp_heap_caps`, `esp_cache`,
`esp_task_wdt`, `esp_pthread`, `esp_newlib`, `esp_pm`). `nm -u` reports zero
undefined symbols. The linker map has no matching FreeRTOS/ESP-IDF runtime
archive paths. This is a symbol-name/link-input audit, not the broader final
review that rules out all embedded vendor code; that F10 audit remains open.
ELF SHA-256 is
`5148207b3c81bd298885735ccb92c200c99cc2c4271d035b451bceb6f76dcb89`;
linker-map SHA-256 is
`84f9785aa61199acaeceec772ef88ef14ba84622507c7a3b11b317caf6c310b8`.

The audited [product ELF](../../../maia-rtos-recovery/native-product-xip-link-20261120/maia_m0.elf),
[image](../../../maia-rtos-recovery/native-product-xip-link-20261120/maia_m0.bin),
[linker map](../../../maia-rtos-recovery/native-product-xip-link-20261120/maia_m0.map),
and [build/audit report](../../../maia-rtos-recovery/native-product-xip-link-20261120/report.json)
are preserved in the recovery directory.
The link-only run did not flash the image; its subsequent controlled warm-reset
boot is recorded below. An earlier exploratory variant that added the probe-only
smoke source to the product ELF hit the
`PSRAM cache adoption must execute from internal SRAM` assertion; the
canonical product source list now links without changing that guard.

## Canonical product XIP warm-reset boot — 2026-11-20

The preserved 804,144-byte product image was run once through
`tests/hardware/run_f0_flash_probe.py` on `/dev/cu.usbmodem1301`, using marker
`msh>`. The runner read and validated the original factory app, confirmed
coverage before writing at `0x10000`, captured 20 seconds, then restored the
backup and verified its hash. The report says `probe_pass=true`,
`restore_verified=true`, one boot with marker and no fatal, and original app
SHA-256 `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.

The warm-reset UART log reached the native cache/PSRAM sequence:
`cache_adopt`, snapshot, `xtal20_start`, `x8_mode`, `fast_start`,
`window_start` and `window_sync` all returned 0. It then reported the native
LCD service ready on its PSRAM framebuffer and the ST7121 touch service ready.
The scheduler started its three tasks; shell automatic self-test passed
`qtest` (mutex, semaphore and queue), then reached `msh>`. No
`MAIA_NATIVE_FATAL` or `MAIA_STACK_FAULT` was present.

The restore capture returned to the original app, reached `msh>`, loaded
`/sd/desktop.mobj` and applied `/sd/wallpaper.raw`. Report, original backup,
product capture and restore capture are in
[the recovery directory](../../../maia-rtos-recovery/native-product-xip-smoke-20261120-1218/);
see the [report](../../../maia-rtos-recovery/native-product-xip-smoke-20261120-1218/report.json),
[product boot log](../../../maia-rtos-recovery/native-product-xip-smoke-20261120-1218/probe-boot-1.log)
and [restore log](../../../maia-rtos-recovery/native-product-xip-smoke-20261120-1218/restore-boot.log).

This first pass was one electronic warm-reset boot; it did not include visual
observation or touch input. A second warm-reset boot with a 60-second visual
observation is recorded next. The first pass establishes that the canonical
product's current native cache/PSRAM initialization path reaches the shell on
this warm-reset state; the separate RAM-only probe's guarded refusal remains
valid for its different entry snapshot.

### Owner visual observation — 60 seconds

A second run used the same image and recovery runner with
`--observe-seconds 60`. The owner reported the screen was correct and stable,
with no cyan screen, flicker or visible artifacts throughout the observation.
Serial capture again reached `msh>` without fatal/stack-fault output. The runner
restored the original app and verified the same backup SHA-256. Report and
captures are in
[the visual-run directory](../../../maia-rtos-recovery/native-product-xip-visual-20261120-1220/);
see the [report](../../../maia-rtos-recovery/native-product-xip-visual-20261120-1220/report.json)
and [product log](../../../maia-rtos-recovery/native-product-xip-visual-20261120-1220/probe-boot-1.log).
This remains a warm-reset visual check only; touch input was tested in the
following run, while cold boot was not tested.

### Touch interaction — owner confirmation

A third run kept the same product image active for a 60-second observation.
The owner confirmed the white cross appeared, followed the finger, and
disappeared on release. The product log reached `msh>` with the ST7121 service
ready and no fatal/stack-fault marker; the runner restored the original app
and verified its hash. The UART does not emit per-contact coordinates, so the
movement/release result is the owner's visual observation. Report and capture:
[touch-run directory](../../../maia-rtos-recovery/native-product-xip-touch-20261120-1233/),
[report](../../../maia-rtos-recovery/native-product-xip-touch-20261120-1233/report.json),
[product log](../../../maia-rtos-recovery/native-product-xip-touch-20261120-1233/probe-boot-1.log).
This validates touch on the warm-reset product path only; cold-start touch
remains untested.

## F1 C++ static initialization and guards — 2026-11-20

The Newlib toolchain defines `PTHREAD_ONCE_INIT` as `{1, 0}`. The Maia
gthread backend had treated `is_initialized == 1` as an in-progress state,
so a static once object could wait forever. The backend now accepts both the
Newlib initializer and a zero-initialized object, uses state `2` while the
initializer is running, and publishes completion as state `1` with
`init_executed` set. The host regression checks the Newlib initializer,
zero-initialized object and single execution.

The RISC-V startup now calls `__libc_init_array` after SoC early initialization
and before `maia_boot_main`; product and F1 C++ targets enable this path. All
three Tab5 linker scripts retain and delimit `.preinit_array` and `.init_array`.
The F1 device probe checks a global constructor and a function-local static
guard contended by two tasks. Its initial boot and three repeats passed; the
original app was restored and its hash verified. The artifacts and reports are
in the [four-boot recovery directory](../../../maia-rtos-recovery/native-f1-cxx-init-repeat-20261120/)
and [initial run](../../../maia-rtos-recovery/native-f1-cxx-init-fixed-20261120/).

The canonical `maia_m0_image` product also built and passed structural image
validation: 806,592 bytes, five segments, entry `0x40000020`, app descriptor
`0x400B0020`, image SHA-256
`3cb55a05b31776e31fffbe54b78aa6a9eab86166830094b7657583751f3281ac`.
The ELF has no undefined symbols; its map shows `__libc_init_array` and
`.init_array` entries, including the shell and libstdc++ constructors. The
controlled warm-reset run reached `msh>` with no fatal marker
(`probe_pass=true`); the original 2,093,696-byte app was restored and its
SHA-256 verified as
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
The [product report](../../../maia-rtos-recovery/native-product-xip-cxx-init-20261120/report.json),
[boot log](../../../maia-rtos-recovery/native-product-xip-cxx-init-20261120/probe-boot-1.log)
and [restore log](../../../maia-rtos-recovery/native-product-xip-cxx-init-20261120/restore-boot.log)
preserve the device result.

This confirms C++ initialization on the tested warm-reset path, not cold boot.
Compiler TLS (`thread_local`/ELF TLS) and `std::thread` remain unvalidated.

## F1 successful USB serial input — 2026-11-20

Extended the F1 runtime probe to first verify empty `read(STDIN_FILENO)` returns
`EAGAIN`, announce `MAIA_F1_RUNTIME_INPUT_READY`, then read and compare the
four-byte payload `Maia`. The flash runner gained optional `--input-marker` and
`--input-hex` arguments; it transmits once when the marker appears and fails
explicitly if the marker is never seen or the serial write is short. Existing
captures are unchanged when these options are omitted.

The host `f0_flash_runner_test` covers marker-triggered transmission,
missing-marker failure and argument validation; `gthread_test` also passed
(2/2 focused CTest cases). The 108,640-byte P4 probe image passed structural
validation (four segments, entry `0x40000020`) with SHA-256
`da13ba02c73fde4a8f4b707c9e5aecbfff20fa3135b102aa0d168b9990af4de0`.
On the Tab5, four warm-reset boots each received the payload, emitted
`MAIA_F1_RUNTIME_PASS`, and had no fatal/stack-fault marker. The runner restored
the original app and verified SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
The [report](../../../maia-rtos-recovery/native-f1-positive-read-20261120/report.json),
[host/device transcript](../../../maia-rtos-recovery/native-f1-positive-read-20261120/run.log),
and [boot captures](../../../maia-rtos-recovery/native-f1-positive-read-20261120/)
preserve the result.

This validates successful USB Serial/JTAG input through Newlib `_read`; it
does not validate UART0 input or the board keyboard path. Cold boot remains
separate.

## F1 Newlib `reent` slot reuse — 2026-11-20

The F1 probe now waits until both workers return, delays two ticks for the
trampoline to reclaim their runtime state, then creates a replacement task.
The replacement must receive the first worker's reused task ID, the same
`__getreent()` storage address with `_errno == 0`, and a cleared
`pthread_getspecific()` value. These assertions run before the replacement
exits, exercising the scheduler's real exited-slot reuse and Newlib/gthread
cleanup path.

The expanded 108,944-byte probe image passed structural validation (four
segments, entry `0x40000020`), SHA-256
`3d2225cbf8925734a5c7748dbec14f232488005087f452c4fe4f2d645e3999fd`. Four
warm-reset Tab5 boots emitted `MAIA_F1_RUNTIME_PASS`, with no fatal marker;
each also repeated the successful USB input check. The runner restored the
original app and verified SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Focused host tests `newlib_reent_test`, `gthread_test` and
`f0_flash_runner_test` passed (3/3). The [report](../../../maia-rtos-recovery/native-f1-reent-reuse-20261120/report.json),
[transcript](../../../maia-rtos-recovery/native-f1-reent-reuse-20261120/run.log)
and [boot captures](../../../maia-rtos-recovery/native-f1-reent-reuse-20261120/)
retain the evidence.

This checks one recycled slot and its errno/key reset; repeated high-churn
slot cycling, compiler TLS and `std::thread` remain open.

## Static compiler TLS per task — 2026-11-20

Added static local-exec ELF TLS support for the boot thread, idle context and
scheduler tasks. The two flash-XIP linker scripts delimit `.tdata`/`.tbss` and
expose a `PT_TLS` segment; the RISC-V context saves/restores `tp`, and new task
frames start with their own TLS block. The F1 probe checks initialized and
zero-filled TLS in the boot/report/reused task contexts, and verifies that two
workers keep different values across preemption. The host `tls_runtime_test`
checks template copying, zero-fill, alignment, isolation, reclamation and slot
reuse.

The F1 image (110,048 bytes; SHA-256
`dfda87ca4484f4dbd4b573cafa87812243e36db0eeb58fd95568174a1f9ee655`) passed
structural validation. Its ELF has a writable `PT_TLS` segment with 16-byte
`.tdata` and `.tbss`; the worker uses TP-relative accesses. The complete host
suite passed (87/87). Four warm-reset Tab5 boots emitted
`MAIA_F1_RUNTIME_PASS` without fatal/stack-fault markers; each also passed the
serial-input assertion. The runner restored the original 2,093,696-byte app
and verified SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
The [report](../../../maia-rtos-recovery/native-f1-compiler-tls-20261120/report.json),
[transcript](../../../maia-rtos-recovery/native-f1-compiler-tls-20261120/run.log)
and [boot captures](../../../maia-rtos-recovery/native-f1-compiler-tls-20261120/)
preserve the device result.

Support is limited to statically linked local-exec TLS with constant-initialized
trivial variables. Dynamic TLS modules and non-trivial `thread_local`
initialization/destruction are not implemented; `std::thread`, cold boot and
high-churn task-slot stress remain open.

## TLS 64-byte alignment and product warm boot — 2026-11-20

Extended the F1 probe with a volatile 64-byte-aligned `thread_local` variable.
Boot, worker, report and recycled task contexts assert the alignment and zero
initialization; workers also verify isolation across scheduling. The ELF has
`PT_TLS` alignment 64, `.tdata` at `0x4ff00140`, and the aligned variable at
TLS offset `0x40`. The 110,160-byte image passed structural validation and four
warm-reset boots emitted `MAIA_F1_RUNTIME_PASS` without fatal/stack-fault
markers. Its SHA-256 is
`71e2d1bab2ce613b64f0c5c094d181c04b286694441152c05bca7b7dcaf990d0`.
Host `tls_runtime_test`, `gthread_test`, `kernel_mvp_test` and
`kernel_fp_frame_test` passed (4/4). The
[probe artifacts](../../../maia-rtos-recovery/native-f1-tls-align64-20261120/)
include the image, ELF, map, report and logs.

The canonical XIP product with TLS runtime integrated also reached `msh>` in
one warm-reset boot, with `qtest PASS` and LCD/touch service-ready messages;
no new visual or touch interaction was performed. The 807,328-byte image
passed structural validation and has SHA-256
`42fc5b74b12a775da14a06dd075f0d6f501f8697cc59a1df3e587030a9a0edf9`.
For both runs the runner restored the original app and verified SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
The [product artifacts](../../../maia-rtos-recovery/native-product-xip-tls-20261120/)
include the image, ELF, map, report and logs. These results do not qualify cold
boot.

## Basic `std::thread` on Tab5 — 2026-11-20

The pthread runtime supplies the libstdc++-required create/join/detach path,
stable thread handles separate from reusable scheduler IDs, basic stack
attributes, and return values. Owned stacks are reclaimed only after the
scheduler marks the task `EXITED` and the exit trap has switched off that
stack. Host `pthread_runtime_test` covers join, detach, explicit exit, external
stacks, allocation failure, stack release and reuse of a scheduler task ID
while an earlier pthread remains joinable. The F1 C++ probe checks
`std::thread` creation, `get_id` against `this_thread::get_id`, join, detach
and detached worker completion.

`maia_f1_runtime_probe_image` passed image validation (113,216 bytes, SHA-256
`3c520339e84114fbb03653a8f36be2db1d5a0ec100e1784f94c6e63ca6b30065`). Two
warm-reset Tab5 boots emitted `MAIA_F1_RUNTIME_PASS`; both also passed the
serial input check. The original 2,093,696-byte app was restored and its SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`
verified. The full host suite passed 88/88.

The product XIP image with pthread linked passed structural validation
(807,824 bytes, SHA-256
`f35554a45aa7c6d9bee5295743f6bb8cbfbfe8d78971f22ad7375c55f685ccbc`) and one
warm-reset boot reached `qtest PASS` and `msh>`, with native LCD and ST7121
touch services reporting ready. No new visual/touch acceptance was performed.
The original app restoration/hash check also passed, and the restored boot
reached `msh>` and loaded `desktop.mobj`. Probe and product reports, captures,
backups, ELF files, maps and images are preserved in
[F1 evidence](../../../maia-rtos-recovery/native-f1-std-thread-20261120-with-input-detach/)
and [product evidence](../../../maia-rtos-recovery/native-product-xip-pthread-20261120/).

The current product ELF has zero undefined symbols; its symbol table and link
map contain no matches for the scanned common FreeRTOS/ESP-IDF runtime names.
This targeted regression check does not replace the broader F10 source audit.

Scope: this is the basic single-core libstdc++ thread subset exercised above,
not full POSIX or SMP support. Dynamic TLS and non-trivial `thread_local`
initialization/destruction, cold boot, and visual/touch regression remain
separate work.

A preliminary probe invocation omitted the positive serial-input injection and
therefore failed its existing input assertion after emitting the ready marker;
the runner still restored and verified the original app. That diagnostic run
is preserved separately in
`../../../maia-rtos-recovery/native-f1-std-thread-20261120/`. The accepted
two-boot result above sends the required `Maia` input.

## `std::thread` sequential lifecycle churn — 2026-11-20

Extended the F1 probe to create 16 sequential C++ threads per boot, alternating
eight `join` and eight `detach` lifecycles. Joined workers verify stable
`thread::id` and return data; detached workers must run to completion. The
detached path waits after its result so the exit hook can reclaim the stack and
record before the next iteration reuses scheduler resources.

The 113,744-byte image passed structural validation (SHA-256
`7c48c895f4949b6004fabf2f60b6074978543d78a7cb339d90808dd553d6fe39`). Both
warm-reset Tab5 boots emitted `MAIA_F1_RUNTIME_PASS` without fatal markers and
passed the required USB serial input check. The runner restored the original
app and verified SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
The [probe artifacts](../../../maia-rtos-recovery/native-f1-std-thread-churn-20261120/)
include the image, ELF, map, report, captures and restoration logs.

This is bounded single-core lifecycle churn, not prolonged/high-volume stress
or SMP validation. Cold boot and dynamic/non-trivial TLS remain untested.

## SDHC read-only protocol groundwork — 2026-11-20

References:

- Espressif ESP-IDF v5.4.2
  [SDMMC host documentation](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-reference/peripherals/sdmmc_host.html),
  sections Overview and Configuring Voltage Level, documents the P4 host slots
  and the SD VDDPST_5 power requirement.
- Pinned ESP-IDF revision `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:
  [`sdmmc_sd.c`, `sdmmc_decode_csd`](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/sdmmc/sdmmc_sd.c)
  and [`sd_protocol_defs.h`, SD R2 CSD definitions and `MMC_RSP_BITS`](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/sdmmc/include/sd_protocol_defs.h);
  [`sdmmc_transaction.c`, response handling](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_driver_sdmmc/src/sdmmc_transaction.c)
  copies the four long-response registers into the CSD response array.
  The port retains the original permissive copyright and license in the source;
  no ESP-IDF or FreeRTOS headers or runtime are linked.
- The project BSP snapshot at revision `1775c0dab0fac2b1658c68f017bd62182631554b`
  uses slot 0, GPIO 43/44 for CLK/CMD, GPIO 39–42 for D0–D3, and requests LDO
  channel 4 in `bsp_sdcard_init`. This is a source inventory, not independent
  proof of the electrical circuit or a safe native power sequence.
  [M5Stack Tab5 documentation](https://docs.m5stack.com/en/core/Tab5),
  specification section, confirms a microSD slot.

The independent `maia_sd_read` library decodes a validated SDHC/SDXC CSD v2
and checks block address bounds for a single-block read callback. It is not yet
connected to the P4 controller or the product image. CSD v1/SDSC, SDUC,
initialization, physical transport and writing are not implemented. The host
fixture is synthetic; it uses the block count from an earlier legacy firmware
boot and is not a captured raw CSD.

Validation on macOS, AppleClang 21: full host CTest 89/89; `sd_read_test`
passed ASan/UBSan; public header compiled with C++98 and warnings as errors.
`maia_sd_read` cross-compiled with Espressif GCC 14.2 and the existing product
image target built. Hardware **NOT RUN**: no serial access, flash/reset, SD
commands or USB operations were performed for this step.

Commands executed with the existing configured build directories:

```sh
cmake --build /tmp/maia-rtos-host -j4
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir /tmp/maia-rtos-host --output-on-failure
cmake --build /tmp/maia-sd-sanitized --target sd_read_test -j4
ctest --test-dir /tmp/maia-sd-sanitized --output-on-failure -R '^sd_read_test$'
cmake --build /tmp/maia-rtos-f1-runtime --target maia_sd_read maia_m0_image -j4
printf '#include <maia/drivers/sd_read.h>\n' | c++ -std=c++98 -Wall -Wextra -Wpedantic -Werror -I /Volumes/External_SSD/Documentos/Projects/maia-rtos/include -x c++ -fsyntax-only -
```

The sanitizer configuration used
`-fsanitize=address,undefined -fno-omit-frame-pointer` for C.

The earlier
[restored boot](../../../maia-rtos-recovery/native-f1-std-thread-churn-20261120/restore-boot.log)
reports an SDHC card and `/sd` mounted, then automatically synchronizes files
onto the card. That is evidence of the legacy application, not the native
driver. The owner clarified on 2026-11-20 that this microSD is test media.
Restoration may therefore perform its normal sync; do not format it. Once a
native transport exists, send initialization and `CMD17` reads, compare known
sector contents against an independent reference, include timeout/removal/error
tests, and capture actual UART results. Do not mark the SDMMC or protocol
checklist items done until those tests pass.

## SDMMC and VO4 warm-reset state — 2026-11-20

Sources, all pinned to ESP-IDF commit `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:

- [`reg_base.h`, peripheral bases](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/reg_base.h),
  [`sdmmc_reg.h`, controller registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/sdmmc_reg.h),
  [`hp_sys_clkrst_reg.h`, SOC_CLK_CTRL1/PERI_CLK_CTRL01/02](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/hp_sys_clkrst_reg.h)
  and [`lp_clkrst_reg.h`, HP_SDMMC_EMAC_RST_CTRL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/lp_clkrst_reg.h).
- [`io_mux_reg.h`, GPIO39–44 and MCU_SEL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/io_mux_reg.h)
  and [`sdmmc_pins.h`, slot 0 pins/function](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/sdmmc_pins.h).
- [`ldo_ll.h`, channel mapping and bypass](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/ldo_ll.h)
  and [`pmu_struct.h`, ext_ldo field layout](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/pmu_struct.h):
  VO4 maps to `ext_ldo[4]`; `tieh=1` bypasses the regulator to its input.
  The BSP uses LDO4 and the [SDMMC host default](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_driver_sdmmc/include/driver/sdmmc_default_configs.h)
  specifies 3.3 V. This is a requested nominal setting, not a voltage measurement.

The private RAM probe reads an explicit whitelist, skips SDHOST when its bus
clock is gated or reset is asserted, and rechecks clocks/reset/VO4 for changes.
It never reads the data FIFO, writes SDMMC/IO-mux/VO4 registers, or issues SD
commands. The ordinary probe startup still initializes console/watchdog state.
The host test checks the whitelist, all clock/reset combinations, skipped
registers and changed-state rejection; the Python capture validator rejects
missing, duplicate, inconsistent and malformed records.

Two automatic ROM-loader/warm-reset observations on the connected Tab5 passed,
each with eight stable samples. Observed: bus clock enabled, reset deasserted,
VO4 control `0x40204180` (software selection, enabled, bypass), host clock
enable `0`, and GPIO39–44 IO-mux values `0x1800` (`MCU_SEL=1`, not SDMMC
function 0). The SDHOST version was `0x5342270a`. These observations do not
establish physical voltage, electrical routing, card state or successful SD
communication; no cold boot was tested.

No flash writes were made by the RAM runner. After the second observation, an
additional normal reset was captured using `run_f0_flash_probe.capture`:
the legacy firmware mounted `/sd`, synchronized files, loaded
`/sd/desktop.mobj` and displayed its wallpaper. The USB pendrive was not
accessed by the diagnostic. Image, ELF, map and real UART logs are at
`../../../maia-rtos-recovery/native-sdmmc-state-20261120/`; image SHA-256
`9276a07be9b6c963dd7853173d55707d576563e07d0be002d00e927b15d44dfc`.

Validation on macOS/AppleClang 21: full host CTest 90/90; SD tests 2/2 with
ASan/UBSan. The RISC-V GCC 14.2 build passed the native-runtime and RAM-window
gates. Conflicting probe build/runner options were rejected before device
access. Commands:

```sh
cmake -S . -B /tmp/maia-native-sdmmc-state-20261120 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_SDMMC_STATE=ON
cmake --build /tmp/maia-native-sdmmc-state-20261120 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-sdmmc-state-20261120/maia_sched_probe_ram.elf
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --port /dev/cu.usbmodem1301 --image ../maia-rtos-recovery/native-sdmmc-state-20261120/maia_sched_probe_ram.bin --log ../maia-rtos-recovery/native-sdmmc-state-20261120/probe-boot2.log --sdmmc-state --capture-seconds 8
cmake --build /tmp/maia-rtos-host -j4
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir /tmp/maia-rtos-host --output-on-failure
cmake --build /tmp/maia-sd-sanitized --target sdmmc_snapshot_test sd_read_test -j4
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir /tmp/maia-sd-sanitized --output-on-failure -R '^(sdmmc_snapshot_test|sd_read_test)$'
```

## SDMMC boot-only CMD0/CMD8 diagnostic — 2026-11-20

Primary sources and provenance:

- Espressif's [ESP32-P4 Technical Reference Manual, section 59.9](https://documentation.espressif.com/esp32-p4_technical_reference_manual_en.pdf#page=3421)
  describes SDHOST register initialization and command submission. This is the
  online edition; the register definitions used by this diagnostic are pinned
  to the SDK revision below.
- ESP-IDF revision `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:
  [`sdmmc_host.c`, slot/clock setup](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_driver_sdmmc/src/sdmmc_host.c),
  [`sdmmc_transaction.c`, command and response handling](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_driver_sdmmc/src/sdmmc_transaction.c),
  [`sdmmc_ll.h`, controller operations](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/sdmmc_ll.h),
  [`sdmmc_reg.h`, SDHOST register fields](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/sdmmc_reg.h),
  [`sd_protocol_defs.h`, command and response definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/sdmmc/include/sd_protocol_defs.h),
  and [`sdmmc_common.h`, idle delay](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/sdmmc/include/esp_private/sdmmc_common.h).
  Pin, clock and VO4 register sources and the Tab5 BSP pin inventory are in the
  two preceding sections. The board-specific source retains the Apache-2.0
  attribution for adapted Espressif material; no ESP-IDF/FreeRTOS runtime is
  linked into the diagnostic.
- Espressif's [ESP-IDF v5.4.2 SDMMC host documentation, Configuring Voltage Level](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-reference/peripherals/sdmmc_host.html#configuring-voltage-level)
  states the VDDPST_5 IO-voltage requirement. It does not establish this
  particular board's physical voltage.

The private boot-only routine requires core 0, interrupts disabled, core 1
stopped and exclusive ownership of SDHOST, GPIO39/42/43/44, VO4 and SYSTIMER
UNIT0. Guards check the inherited software-selected, enabled, bypassed VO4 and
an idle, clocked host. They neither configure the supply nor measure it. It
configures slot 0 as 1-bit `CLK=GPIO43`, `CMD=GPIO44`, `D0=GPIO39`, drives
`D3=GPIO42` high and samples its input. D1/D2 are not used; card detect is
forced present, not physically observed. The nominal divider is
`PLL160M / 10 / (2 * 20) = 400 kHz`, not a measured frequency.

The controller sends only `CMD0` with the initial clocks and `CMD8` with
argument `0x1aa`, with a 20 ms delay between them. Polling, timer and handshake
waits are bounded. The code checks the R7 response index, voltage/pattern and
controller status/CRC; `RINTSTS` is write-one-to-clear and residual status is
checked. A failure after configuration begins is sticky until reset, and a
successful call also retains ownership until reset. No teardown, retry, sector
transfer, filesystem or runtime-safe/SMP use is provided.

The initial unsynchronized [capture](../../../maia-rtos-recovery/native-sdmmc-cmd8-20261120/probe-boot1.log)
lost the beginning of its schema header. It showed controller completion and
`0x1aa`, but the validator rejected it because the record was incomplete.
The host and probe were changed to exchange a ready byte before output or
SDHOST access. Two subsequent warm-reset captures passed the unchanged strict
record validation:

| Capture | CMD0 raw | CMD8 raw | R7 | Host status |
|---|---:|---:|---:|---:|
| [boot 2](../../../maia-rtos-recovery/native-sdmmc-cmd8-20261120/probe-boot2.log) | `0x4` | `0x4` | `0x1aa` | `0x41e6` |
| [boot 3](../../../maia-rtos-recovery/native-sdmmc-cmd8-20261120/probe-boot3.log) | `0x4` | `0x4` | `0x1aa` | `0x41e6` |

`CMD0` has no response: `0x4` is completion reported by the controller.
`CMD8` is an actual response consistent with the requested R7 pattern, but does
not prove SDHC capacity or establish that a complete card initialization or
read occurred. Captured settings included VO4 `0x40204180`, peripheral divider
`0x38929200`, `CLKDIV=0x14` and `CLKENA=1`. These are warm-reset observations,
not cold-boot, waveform, frequency or voltage measurements.

The runner used RAM loading and did not write flash; the probe did not read or
write card sectors and did not access the USB pendrive. After the tests, a
separately captured normal reset of the existing firmware showed `/sd` mounted,
files synchronized, desktop loaded and wallpaper applied. Legacy firmware's
normal synchronization can write to the test card; do not format it. No fresh
flash-hash comparison was performed, and this result makes no claim about
legacy firmware USB enumeration.

The [evidence directory](../../../maia-rtos-recovery/native-sdmmc-cmd8-20261120/)
contains both images, ELFs, maps and all serial logs. The synchronized image
SHA-256 is `17f57479346caba3cdd6d774e1519c4d4461b27f2403f106014b1c12dace8b99`;
the rejected first image is
`164093bfa76276bfcc21c6361ca91e940ad89a18947562d4a11b89ecccf9cdfb`.
The original firmware's [restored boot](../../../maia-rtos-recovery/native-sdmmc-cmd8-20261120/restore-boot.log)
also reports an SDHC card, but that is legacy evidence, not native
identification.

Host CTest passed 92/92 on macOS/AppleClang 21; the three SD tests passed with
ASan/UBSan. Tests cover register guards, bounded failures, status/response
errors, corrupted readbacks and invalid/incomplete/duplicate serial captures.
The public header compiled as C++98. The RISC-V GCC Espressif 14.2 build passed
the native-runtime and RAM-window gates. Conflicting probe options were rejected
before opening the device. Representative commands:

```sh
cmake -S . -B /tmp/maia-native-sdmmc-cmd8-20261120 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_SDMMC_CMD8=ON
cmake --build /tmp/maia-native-sdmmc-cmd8-20261120 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-sdmmc-cmd8-20261120/maia_sched_probe_ram.elf
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --port /dev/cu.usbmodem1301 --image ../maia-rtos-recovery/native-sdmmc-cmd8-20261120/maia_sched_probe_ram.bin --log ../maia-rtos-recovery/native-sdmmc-cmd8-20261120/probe-boot2.log --sdmmc-cmd8 --capture-seconds 8
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --port /dev/cu.usbmodem1301 --image ../maia-rtos-recovery/native-sdmmc-cmd8-20261120/maia_sched_probe_ram.bin --log ../maia-rtos-recovery/native-sdmmc-cmd8-20261120/probe-boot3.log --sdmmc-cmd8 --capture-seconds 8
cmake --build /tmp/maia-rtos-host -j4
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir /tmp/maia-rtos-host --output-on-failure
cmake --build /tmp/maia-sd-sanitized --target tab5_sdmmc_boot_test sdmmc_snapshot_test sd_read_test -j4
PYTHONDONTWRITEBYTECODE=1 ctest --test-dir /tmp/maia-sd-sanitized --output-on-failure -R '^(tab5_sdmmc_boot_test|sdmmc_snapshot_test|sd_read_test)$'
```

### Native SDHC identification and PIO block-0 attempt — 2026-11-20

The separate RAM probe advanced past `CMD8`: OCR was `0xc0ff8000`; CID was
`220197df 80599084 43363447 03534453`; RCA was `0xaaaa`; CSD
`0a4040df dbd37f80 5b590001 400e0032` decoded to `124735488` blocks, matching
the legacy-reported capacity. `CMD7` selection and `CMD13` ready/transfer-state
checks passed.

`CMD17` for block 0 returned R1 `0x900`; raw status `0x0c` contained
`CMD_DONE|DATA_OVER` and no controller data-error bits. This is not sufficient
to accept data. The PIO probe made 128 32-bit reads from `SDHOST_BUFFIFO` at
`SDHOST_BASE+0x200`; all returned zero. Before the first read the observed FIFO
count was 1; status immediately after the first and last reads remained
`0x00028d00` (count 1, not empty). Final status after `DATA_OVER` was
`0x01008900` (count 128, not empty). The emitted block was all zeroes and did
not contain the expected sector trailer, so the strict runner rejected the
capture. No native block was accepted and there was no independent data
comparison. Do not mark the read driver complete. The ESP32-P4 TRM chapter 59
defines `STATUS.FIFO_COUNT` at bits 29:17 as filled FIFO locations and
`BUFFIFO` at offset `0x200` as the software data port; the observed non-decrement
and zero data are not explained by those register definitions.

Captures:

- [retry 1](../../../maia-rtos-recovery/native-sdmmc-block0-20261120/probe-block0-retry1.log):
  initial FIFO check failed after identification.
- [retry 2](../../../maia-rtos-recovery/native-sdmmc-block0-20261120/probe-block0-retry2.log):
  PIO data/count instrumentation; 128 zero words.
- [retry 3](../../../maia-rtos-recovery/native-sdmmc-block0-20261120/probe-block0-retry3.log):
  status sampled after first and last read; FIFO count stayed at 1.
- [retry 4](../../../maia-rtos-recovery/native-sdmmc-block0-20261120/probe-block0-retry4.log):
  explicit I/O fences made no observed difference; the experimental fences
  were removed from the source.

All runs used RAM loading, no flash-write command, and no card write command.
The runner reset to the preserved flash application after capture; the original
firmware may synchronize files to `/sd`. The test medium was not formatted.
The focused host tests and complete 93/93 host suite passed; the SDMMC C test
passed ASan/UBSan with LeakSanitizer disabled because it is unsupported on this
platform. Native-runtime/RAM-window gates also passed. The immediate blocker
is understanding why reads from the documented FIFO port return zero without
reducing its reported count. Outstanding: valid `CMD17` data and an independent
comparison, failure/removal behavior, physical power/cold boot, DMA/cache
coherence, writes, FatFs/VFS and product integration.

### RX-watermark follow-up — 2026-11-20

The next RAM-only run lowered `FIFOTH.RX_WMARK` from the inherited value 511
(`FIFOTH=0x01ff0000`) to 0 and set `INTMASK` to RXDR only (`0x20`), with global
interrupts still disabled. This made RXDR observable: raw status during the
first and last FIFO reads was `0x24` (`CMD_DONE|RXDR`). It did not fix the
transfer. `CMD17` returned R1 `0x900`; 128 reads still returned zero. The
reported FIFO count stayed at 1 after the first and last accesses
(`STATUS=0x00028d01`), then was 128 at transfer completion
(`STATUS=0x01008901`); the final result was `MAIA_SDMMC_BOOT_IO`. The strict
runner rejected the capture. Lowering the watermark therefore confirms RXDR
activity but does not explain the read port or FIFO-count behavior, and does
not validate any sector.

The image was loaded at `0x4ff00000` using esptool `load_ram`; no flash-write
or card-write command was used. The runner reset to the preserved flash app
after capture; its legacy boot may synchronize files to `/sd`. The test medium
was not formatted. Preserved image, ELF, map and UART capture are in
`../../../maia-rtos-recovery/native-sdmmc-block0-rx-watermark-20261120/`;
the RAM image SHA-256 is
`9b3f31dcfbf63d962e73a28cba71fdd9cd0b04c0a96fdb06c03dd85a3db36a2f`.

The focused C test passed with ASan/UBSan, the runner test passed, full host
CTest passed 93/93, and the RISC-V probe passed its native-runtime and
L2MEM_LOW gates. The image passed runner `--validate-only` before the single
hardware run. Reproduction commands:

```sh
cmake -S . -B /tmp/maia-sdmmc-block0-rx-20261120 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_SDMMC_BLOCK0=ON
cmake --build /tmp/maia-sdmmc-block0-rx-20261120 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image --output /tmp/maia-sdmmc-block0-rx-20261120/maia_sched_probe_ram.bin /tmp/maia-sdmmc-block0-rx-20261120/maia_sched_probe_ram.elf
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --port /dev/cu.usbmodem1301 --image /tmp/maia-sdmmc-block0-rx-20261120/maia_sched_probe_ram.bin --log /tmp/maia-sdmmc-block0-rx-20261120/probe-rx-watermark.log --sdmmc-block0 --expected-sd-block-count 124735488 --capture-seconds 8
cmake --build /tmp/maia-rtos-host -j4
ctest --test-dir /tmp/maia-rtos-host --output-on-failure
```

### RXDR acknowledgement and FIFO telemetry — 2026-11-20

A further read-only run acknowledged RXDR by writing its W1C bit after servicing
the FIFO. The immediate post-ack raw status still read `0x24` (`CMD_DONE|RXDR`),
and the transfer remained unsuccessful: all 128 reads were zero, the FIFO
count stayed 1 after the first and last reads (`STATUS=0x00028d01`), then was
128 at completion (`STATUS=0x01008901`). The final result was
`MAIA_SDMMC_BOOT_IO`; R1 was still `0x900` and `CMD17` raw status was `0x2c`
without data-error bits. The W1C acknowledgement did not resolve the problem.

Additional post-transfer observations were `CTRL=0`, `BMOD=0`, `TCBCNT=0x200`,
`TBBCNT=0`, and `GPIO_IN1=0x1cc0`. These narrow the diagnostic state but do not
explain why CPU reads from `BUFFIFO` return zero or why `FIFO_COUNT` does not
decrease. No sector was accepted or compared to an independent reference.
The image, ELF, map and UART capture are preserved in
`../../../maia-rtos-recovery/native-sdmmc-block0-fifo-diag-20261120/`; image
SHA-256:
`06e31c9470446866eb562529caad0efb17b11b26ab495560e483558c80573c41`.

The probe again used RAM loading only, with no flash-write or card-write
command. The runner reset to the preserved flash app, whose legacy boot may
synchronize files to `/sd`; the card was not formatted. The CTest suite passed
93/93, the focused C test passed ASan/UBSan, and the RISC-V native-runtime and
L2MEM_LOW gates passed after adding the W1C acknowledgement and telemetry.

### DTO-gated FIFO drain follow-up — 2026-11-20

The TRM/SDK cross-check found no PIO receive routine to port from the pinned
ESP-IDF revision (`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`): its SDMMC data
path is DMA-only. TRM chapter 59 says software reads received data from
`SDHOST_BUFFIFO_REG`; its RX watermark note assigns the final partial drain to
`DATA_OVER`, but does not define a complete PIO receive loop. This experiment
is therefore a bounded TRM-based diagnostic, not a reproduction of an
ESP-IDF PIO sequence.

To test whether the earlier FIFO reads were simply too early, a new diagnostic
left `RX_WMARK` at its inherited value 511, set `INTMASK=0`, waited for
`DATA_OVER`, and only then read the reported 128 FIFO words. `CMD17` still
returned R1 `0x900`; `RINTSTS=0x0c` reported `CMD_DONE|DATA_OVER` without data
errors. All 128 reads returned zero, and `STATUS` remained `0x01008900` after
the first and last reads (count 128, not empty). `TCBCNT=0x200`, `TBBCNT=0`,
`CTRL=0`, and `BMOD=0` were observed. The probe returned
`MAIA_SDMMC_BOOT_IO`; no block was accepted or compared to an independent
reference. Waiting for DTO therefore did not resolve FIFO visibility or
consumption.

Disassembly of this exact probe confirms the FIFO load targets
`0x50083200` (`SDHOST_BASE+0x200`) using a 32-bit `lw`. This rules out the
specific hypothesis that the earlier reads happened only before DTO, and
confirms the generated load address/width, but does not explain why the
controller reports 128 words remaining after all 128 reads.

The RAM-loaded image, ELF, map, and UART log are preserved in
`../../../maia-rtos-recovery/native-sdmmc-block0-dto-drain-20261120/`; image
SHA-256:
`9a806fb0ee7b4f5d7373957f8b88f4727abc3227dac9240dbfb3e0bbc4328fe1`.
The image passed `--validate-only` and the P4 native-runtime/L2MEM_LOW build
gates. The focused SDMMC C test passed with ASan/UBSan and the runner test
passed. The single hardware attempt used RAM loading only; no flash/card write
command was issued and the card was not formatted. The runner reset the device
to its preserved flash application after the capture; that legacy boot may
synchronize files to `/sd`.

### Explicit CARDTHRCTL setup follow-up — 2026-11-20

The TRM's [command procedure, §59.9.2](https://documentation.espressif.com/esp32-p4_technical_reference_manual_en.pdf#page=3421)
explicitly includes `CARDTHRCTL` among the data-transfer registers to configure.
Register 59.21 defines `CARDRDTHREN` at bit 0 and a byte threshold in bits
31:16, valid only when enabled and required to be less than 512
([register description](https://documentation.espressif.com/esp32-p4_technical_reference_manual_en.pdf#page=3443)).
The probe now clears those configuration fields before `CMD17` and reports the
register value before and after. On this run both values were already zero;
the transfer still returned R1 `0x900`, `RINTSTS=0x0c`, 128 zero words, and
`STATUS=0x01008900` after the first and last FIFO reads. The result remains
`MAIA_SDMMC_BOOT_IO`. This explicit TRM-required setup is now covered, but the
threshold register was not the cause of the FIFO failure.

The image, ELF, map, and UART log are preserved in
`../../../maia-rtos-recovery/native-sdmmc-block0-cardthrctl-20261120/`; image
SHA-256:
`0abf8e09f5dba77fe59b7505dd64dc86dc6e732daf10a500a6a5421fbd6b63f2`.
The P4 native-runtime/L2MEM_LOW gates and image `--validate-only` passed; the
focused SDMMC C and runner tests passed. The one device attempt used RAM
loading only, issued no flash/card write commands, and did not format the
card. The runner reset the device to the preserved flash application.

### Native one-block IDMAC read — 2026-11-20

The PIO investigation did not produce a usable receive path. The pinned
ESP-IDF implementation and TRM §59 instead support an internal-IDMAC receive
sequence. The boot-only probe now prepares a single 64-byte descriptor and a
512-byte buffer in a separate 640-byte `NOLOAD` L2MEM reservation, aligned to
128 bytes and kept outside startup-cleared BSS and the stack. It purges old
L1D aliases through the P4 ROM writeback/invalidate entry, then accesses the
reservation only through the NC alias; the IDMAC receives the physical
`0x4ff...` addresses. The pinned ROM cache line query returned 64 bytes.
Descriptor ownership, DMA completion, controller DTO, FIFO-empty state and
error flags are all checked; DMA and its interrupt state are stopped/cleared
before the probe returns.

Two earlier RAM-only device runs completed SDHC identification and `CMD17`
block 0.
`RINTSTS=0x2c` contained `CMD_DONE|DATA_OVER|RXDR` without data-error bits;
IDMAC status was `0x102` (`RI|NIS`), and the completed descriptor returned to
the host with `DES0=0x1c`, `DES1=512`, `DES2=0x4ff03d00`. The emitted 512 bytes
had the `0x55aa` MBR trailer and a nonzero first partition entry; its SHA-256
was `32a28faf07d55fa077b3b188b06085d4d7ee8091fa1c6f672f1ddd864c75fae5` in
both runs, with all 512 bytes matching exactly. The runner checked the
previously observed capacity of `124735488` blocks and accepted both
structural captures. This establishes repeatability for one diagnostic LBA,
not yet a production read driver or an independent byte-for-byte comparison
against another reader.

The first image, ELF, map, and UART log are preserved in
`../../../maia-rtos-recovery/native-sdmmc-block0-idmac-20261120/`; the RAM
image SHA-256 is
`c96bfdefbd89cdad131845bc7d9f795b70ff812a8bd02af52e885646db01b0be`.
The focused host C and capture tests passed, the P4 native-runtime/L2MEM_LOW
gate passed, and runner `--validate-only` accepted that image before its
hardware run. After limiting `IDINTEN` writes to documented bits, the host C
and capture tests passed again, the P4 gate passed, and the rebuilt image
passed `--validate-only` before the repeat hardware run. That exact image, ELF,
map, and second UART log are preserved in
`../../../maia-rtos-recovery/native-sdmmc-block0-idmac-repeat-20261120/`; its
RAM image SHA-256 is
`9aa09ae9922e07e98089c42cb60f8a2d6a0a2ec55a8c873c58b21a3f146f8e84`.
Both block-0 runs used `load_ram`, issued no flash-write or card-write command,
and did not format the card. The runner reset to the preserved flash
application afterward; its legacy boot may synchronize files to `/sd`.

The probe and boot-only API now accept a selected SDHC LBA. The API rejects
addresses above `UINT32_MAX` before device access and rejects LBAs outside the
decoded CSD capacity before selecting the card. Host tests cover LBA 1 data,
argument propagation, capacity bounds and sticky failures; the runner accepts
generic one-block captures and preserves `--sdmmc-block0` as an LBA-0 alias.
The RISC-V native-runtime/L2MEM_LOW gate and `--validate-only` passed for the
LBA-32768 image.

RAM-only `CMD17` reads then passed at LBA 1 and LBA 32768. LBA 1 returned 512
zero bytes (SHA-256
`076a27c79e5ace2a3d47f9dd2e83e4ff6ea8872b3c2218f66c92b89b55f36560`). The
captured MBR identifies the first partition start at LBA 32768 with 124702720
sectors. At that LBA the read returned a FAT32 boot sector (`EB 58 90`, 512
bytes/sector, `55 AA` trailer); its BPB sector count matched the MBR partition
size. Two separate RAM-only runs of LBA 32768 produced identical 512-byte
payloads, SHA-256
`ebdfb90905827cbc8e0cd6f8b851c51552e7915ff0e27810834fe03b6d92978a`.
Images and UART captures are preserved in
`../../../maia-rtos-recovery/native-sdmmc-lba1-20261120/` and
`../../../maia-rtos-recovery/native-sdmmc-lba32768-20261120/`; the latter RAM
image SHA-256 is
`0a1db2e2f3b13a1371b4081038c023b72269e90a56f6f2f869aea56a6cd87d08`.
These checks establish bounded, repeatable reads at multiple LBAs and
consistency between the on-card MBR and partition boot record, not an
independent byte-for-byte comparison with another reader or production-driver
readiness. No run wrote to flash or card or formatted the card. Still pending:
an independent data comparison, broader LBA coverage, failure/removal behavior,
driver integration, writes, FatFs/VFS, and cold power sequencing.

### IDMAC byte-count checks and FAT32 backup — 2026-11-20

The one-block boot diagnostic now requires both `SDHOST_TCBCNT_REG` and
`SDHOST_TBBCNT_REG` to equal 512 after DTO, IDMAC RX and ownership return,
before reporting success. The P4 TRM §59.13 identifies these registers as the
CIU/card and BIU/DMA byte counters, respectively. The block is copied to the
report only after these checks and successful DMA shutdown. Host tests inject
0, 511 and 513 bytes for each counter, verify an explicit I/O failure, DMA
cleanup, no published block and failure-sticky behavior. The capture runner
also rejects post-shutdown counter values other than 512. This check is for
the current single-transfer boot probe; counter behavior across consecutive
transfers has not yet been qualified.

The primary FAT32 boot sector previously captured at LBA 32768 identifies a
backup boot-sector offset of 6, within the 32 reserved sectors. A new RAM-only
image using the byte-count checks read LBA 32774. The strict runner accepted
the transfer, with `TCBCNT=TBBCNT=0x200`, and all 512 bytes matched the earlier
primary boot-sector capture; both SHA-256 values are
`ebdfb90905827cbc8e0cd6f8b851c51552e7915ff0e27810834fe03b6d92978a`.
This compares two redundant sectors of the same medium through the same
native path, not an independent reader. A second image built from the same
source read LBA 0; its 512 bytes matched the earlier capture, SHA-256
`32a28faf07d55fa077b3b188b06085d4d7ee8091fa1c6f672f1ddd864c75fae5`.

The images, ELF files, maps and logs are preserved in
`../../../maia-rtos-recovery/native-sdmmc-fat32-backup-20261120/` and
`../../../maia-rtos-recovery/native-sdmmc-counts-lba0-20261120/`. Their image
SHA-256 values are
`dc1030c46fe01796ec5a2bc206b1a563a7b0281042e795cb8fe84a3bd0769e30` and
`bba7882502a76d2128d9aa95b6e2fac202e76cd96c1284fb1a849dad9132fac4`.
Both passed the native-runtime and L2MEM_LOW checks and `--validate-only` before
execution. Each run used `load_ram`, sent no card or flash write commands and
reset to the preserved application; its legacy boot may synchronize `/sd`.
Consecutive transfers are qualified below. Independent comparison, broader
LBA coverage, removal/error recovery and product integration remain open.

### Consecutive CMD17 transfers in one boot — 2026-11-20

The RAM-only probe now initializes the card once and reads LBA 32768 followed
by LBA 32774 in the same boot. On the Tab5, both reads completed through IDMAC;
the API checked `TCBCNT=TBBCNT=512` for each transfer, and the report recorded
two successful reads. The two 512-byte payloads matched exactly, each with
SHA-256
`ebdfb90905827cbc8e0cd6f8b851c51552e7915ff0e27810834fe03b6d92978a`.
The report's final DMA state was idle with ownership returned, and the final
host byte counters were both `0x200`. The strict schema-6 runner accepted the
capture.

The second transfer begins with `FIFOTH=0` because the IDMAC preparation sets
the RX threshold to zero; the schema-6 runner distinguishes this from the
single-transfer probe's inherited pre-transfer threshold. The successful
payload and byte-count checks establish consecutive-transfer behavior for
these two LBAs, not broader SD-card error recovery or production integration.
This is still a same-medium comparison of redundant FAT32 sectors, not an
independent-reader comparison.

The image, ELF, map and UART capture are preserved in
`../../../maia-rtos-recovery/native-sdmmc-consecutive-20261120/`; the RAM image
SHA-256 is
`e5c04541cd742a8a21f4752be0db7a0a2d34ed6275d1a00818e72be1efb3cbe9`.
Build and `--validate-only` passed before the device run. The test used
`load_ram`, sent no card or flash write commands and did not format the card.
The runner reset to the preserved flash application afterward; its legacy
boot may synchronize files to `/sd`. Software-independent comparison is
qualified below. Broader LBA coverage, removal/error handling, product driver
integration, writes, FatFs and VFS remain unqualified.

Reproduction used the isolated RAM linker and the two tested LBAs:

```sh
cmake -S . -B /tmp/maia-sdmmc-repeat-build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_USE_FLASH_PRODUCT_LINKER=OFF -DMAIA_NATIVE_PROBE_SDMMC_BLOCK0=ON -DMAIA_NATIVE_PROBE_SDMMC_REPEAT=ON -DMAIA_NATIVE_SDMMC_READ_LBA=32768 -DMAIA_NATIVE_SDMMC_REPEAT_LBA=32774
cmake --build /tmp/maia-sdmmc-repeat-build --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image --output /tmp/maia-sdmmc-repeat-build/maia_sched_probe_ram.bin /tmp/maia-sdmmc-repeat-build/maia_sched_probe_ram.elf
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --port /dev/cu.usbmodem1301 --image /tmp/maia-sdmmc-repeat-build/maia_sched_probe_ram.bin --log /tmp/maia-sdmmc-repeat-build/probe-validated.log --sdmmc-read --sdmmc-lba 32768 --sdmmc-repeat-read --sdmmc-second-lba 32774 --sdmmc-identical-blocks --expected-sd-block-count 124735488 --capture-seconds 8
```

### Native CMD24 write path — host-tested only — 2026-11-21

Added `maia_tab5_sdmmc_boot_write_block`, a boot-only, single-block `CMD24`
prototype. It stages the caller's 512 bytes in the reserved SRAM DMA buffer
through the non-cacheable alias, enables IDMAC transmit completion (`TI`), and
requires `DATA_OVER`, returned descriptor ownership, and both `TCBCNT` and
`TBBCNT` to equal 512. After stopping DMA it polls `CMD13` until the card
reports `READY_FOR_DATA` and state `TRAN`. A timeout after data transfer does
not prove that the card remained unchanged; the report counts only writes
confirmed ready by the final status.

The command-direction bit follows Espressif ESP-IDF v5.5.1: its command builder
sets `rw=1` when the data command is not marked `SCF_CMD_READ`, and its
single-block write issues CMD24 without that read flag. The pinned source maps
IDMAC transmit/receive completion to `TI`/`RI` in
[`sdmmc_ll.h`](https://github.com/espressif/esp-idf/blob/v5.5.1/components/hal/esp32p4/include/hal/sdmmc_ll.h);
the command builder is in
[`sdmmc_transaction.c`](https://github.com/espressif/esp-idf/blob/v5.5.1/components/esp_driver_sdmmc/src/sdmmc_transaction.c),
and the CMD24 invocation is in
[`sdmmc_cmd.c`](https://github.com/espressif/esp-idf/blob/v5.5.1/components/sdmmc/sdmmc_cmd.c).
The byte-count interpretation remains as documented from ESP32-P4 TRM §59.13.

The host mock validates the bytes staged for TX, CMD24 direction, `TI`
completion, descriptor return, exact counters, response errors, DMA errors,
cleanup, and failure to reach the ready/transfer state. The focused
`tab5_sdmmc_boot_test` passes. **No physical CMD24 was issued and the card was
not written by this implementation.** This does not qualify hardware writes or
complete the SD driver. Before device testing, select and preserve a safe test
LBA and agree on write/readback/restore verification.

### Vendor-software reference and verified flash restoration — 2026-11-21

The owner authorized a temporary reference firmware instead of removing the
SD card from the keyboard-equipped Tab5. The owner's example at
`src/MicroSD/MicroSD.ino` in the neighboring `m5stack-tab5-examples` repository
(base revision `8ab8b1d6b8b29406f1702211e417eeccb775b19a`, Apache-2.0) informed a
separate `src/MicroSDReference/MicroSDReference.ino`. Unlike the file-writing
example, the reference calls `sdmmc_card_init` and `sdmmc_read_sectors` directly;
it does not mount FatFs, open files, write sectors or format the card.

It was built with installed M5Stack Arduino core 3.3.10; the application image
reports ESP-IDF v5.5.4. The resolved board configuration selects pre-v3 P4,
PSRAM enabled and hardware USB CDC/JTAG. Slot 0 is configured for 4 bits at
20 MHz, with on-chip LDO4. Primary configuration source is the
[M5Stack 3.3.10 package](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/arduino/m5stack-3.3.10.zip):
`libraries/SD_MMC/src/SD_MMC.cpp`, `SDMMCFS::begin`, lines 233-284, and
`variants/m5stack_tab5/pins_arduino.h`, lines 58-61. These are external test
dependencies, not native MaiaRTOS dependencies.

The real UART capture reports capacity 124735488 sectors, sector size 512 and
OCR `0xc0ff8000`. The host verified exactly 32 ordered 16-byte data records for
each requested LBA, one terminal PASS, and no reference FAIL. It revalidated
the stored native captures with the current strict runner before comparing
every byte. All four sectors matched:

| LBA | Compared bytes | SHA-256 of both native/reference data |
|---|---:|---|
| 0 | 512 | `32a28faf07d55fa077b3b188b06085d4d7ee8091fa1c6f672f1ddd864c75fae5` |
| 1 | 512 | `076a27c79e5ace2a3d47f9dd2e83e4ff6ea8872b3c2218f66c92b89b55f36560` |
| 32768 | 512 | `ebdfb90905827cbc8e0cd6f8b851c51552e7915ff0e27810834fe03b6d92978a` |
| 32774 | 512 | `ebdfb90905827cbc8e0cd6f8b851c51552e7915ff0e27810834fe03b6d92978a` |

There were three distinct sector payloads, excluding the all-reads-return-one-
buffer hypothesis for this sample. This is an independent software reference
using the same controller/card, not another physical reader or production SD
driver qualification. Broader LBA coverage, errors/removal, power sequencing
and product integration remain open. The auxiliary firmware's failure branches
were not physically fault-injected.

Before any flash write, security information showed secure boot and flash
encryption disabled, and `flash_id` reported 16 MiB. Two complete flash reads
using the esptool v4.12.0 RAM stub and `--after no_reset` matched byte for byte.
A first ROM-only read failed during packet transfer and was not accepted as
a backup. The reference was flashed only after the full backup was verified.
After its capture, `write_flash --flash_mode keep --flash_freq keep
--flash_size keep 0 flash-before.bin` restored the entire original flash.
A complete readback matched all 16777216 bytes before allowing the application
to boot. Both backup and restored snapshot SHA-256:
`674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`.

RTS reset did not resume the restored legacy firmware, and the owner initially
reported a blank display. The official esptool `--after watchdog_reset chip_id`
then resumed it; a serial port reopened with DTR/RTS initially deasserted
captured display/keyboard initialization and desktop startup. The owner
confirmed the desktop returned and the keyboard responded. Reset source:
[esptool v4.12.0 P4 target](https://github.com/espressif/esptool/blob/v4.12.0/esptool/targets/esp32p4.py),
`ESP32P4ROM.watchdog_reset`. Do not equate flash equality or an RTS pulse alone
with successful application recovery. The restored legacy boot visibly
logged synchronization of files to `/sd`; no-card-write applies to the raw
reference firmware, not that legacy boot.

Artifacts are preserved in
`../../../maia-rtos-recovery/native-sdmmc-vendor-reference-20261120/`:
reference source/images/ELF, `reference-uart-reopened.log`, `comparison.json`,
full flash snapshots and backup/restore/boot logs. The reference application
SHA-256 is
`59d37470d6a8703c64f35a8e8fa78cf3bc9045dd8744886151d48925e2a7983c`.
Backups are outside both Git repositories. No native MaiaRTOS source was
linked to Arduino, ESP-IDF or FreeRTOS for this test.

### Native CMD24 write/read/restore test — 2026-11-21

The read-only scanner found FAT32 cluster 97 free in both mirrored FATs and
reported its first data-sector LBA as 69318 (64 sectors per cluster). The
original 512-byte sector was saved before testing; its SHA-256 is
`32aefed26b737027376dbc961157ae63cfaf6f34577c39d6b8aed51e4700a3c0`. A
one-operation-per-boot MaiaRTOS RAM probe was built with
`MAIA_NATIVE_PROBE_SDMMC_WRITE=ON`. One initial invocation passed the ELF rather
than its `elf2image` output; esptool rejected the `0x7f` ELF magic before the
probe ran, issued no SD command, and the full-flash restore/readback still
matched. The corrected ESP image is 11792 bytes, SHA-256
`b4177576cccdc23644c845bab376845eb2e3ea3287fb5bf7539d8b0b2178353f`.

On the successful run, the probe's precondition read matched all 512 saved
bytes (`result=0`, `completed=0x0d`, `reads=1`, `matches=1`). CMD24 then
returned success with one write confirmed ready by CMD13. A separate boot read
matched the entire temporary pattern (SHA-256
`1234b103e02d0b3fa8c0ad5b298bf9bf7218398d2be5aa1dce98ca4980a72f2e`).
The runner wrote the preserved original sector and a subsequent CMD17
comparison again matched all 512 bytes; restoration passed on its first
attempt. The pattern changed only the first sector of the already-free cluster;
no filesystem was mounted, no FAT entry was changed, and the card was not
formatted.
The USB pendrive reserved for TinyUSB host testing was not accessed.

Two full 16 MiB flash captures before the successful run matched the previously
verified image (SHA-256
`674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`); after the
test the complete image was restored and read back byte-for-byte. The restored
MaiaRTOS boot capture contains `msh>`. Probe image, operation captures, flash
snapshots and report are preserved in
`../../../maia-rtos-recovery/native-sdmmc-write-20261121-valid-image/`.

The focused host test passed, and CMake rejected an out-of-range write LBA.
Reproduction commands (use a new empty `--out` directory for another run):

```sh
cmake -S . -B /tmp/maia-sdmmc-write-probe-20261121 \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake \
  -DMAIA_USE_FLASH_PRODUCT_LINKER=OFF \
  -DMAIA_NATIVE_PROBE_SDMMC_WRITE=ON \
  -DMAIA_NATIVE_SDMMC_WRITE_LBA=69318
cmake --build /tmp/maia-sdmmc-write-probe-20261121 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool \
  --chip esp32p4 elf2image \
  --output /tmp/maia-sdmmc-write-probe-20261121/maia_sched_probe_ram.bin \
  /tmp/maia-sdmmc-write-probe-20261121/maia_sched_probe_ram.elf
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python \
  tests/hardware/run_sdmmc_write_probe.py \
  --image /tmp/maia-sdmmc-write-probe-20261121/maia_sched_probe_ram.bin \
  --original-sector /Volumes/External_SSD/Documentos/Projects/maia-rtos-recovery/native-sdmmc-fat-scan-20261121/free-sector-original.bin \
  --restore-flash /Volumes/External_SSD/Documentos/Projects/maia-rtos-recovery/native-sdmmc-fat-scan-20261121/flash-before-a.bin \
  --out /Volumes/External_SSD/Documentos/Projects/maia-rtos-recovery/native-sdmmc-write-20261121-valid-image \
  --lba 69318 --port /dev/cu.usbmodem1301
```

This is hardware evidence for the boot-only single-block CMD24 path, not a
general storage-driver qualification. Multi-block I/O, runtime ownership,
concurrency, removal/error recovery, power interruption, FatFs, VFS and product
integration remain untested. The auxiliary Arduino/ESP-IDF scanner was used
only to identify the free cluster and preserve its original bytes; none of
those runtime dependencies were linked into MaiaRTOS.

### Portable SD block-device request contract — 2026-11-21

`include/maia/drivers/sd_read.h` and `drivers/storage/sd_read.c` now define a
stationary block-device descriptor and whole-request read/write callbacks.
Requests validate nonzero count, 512-byte buffer-size arithmetic, capacity and
32-bit SDHC address bounds before dispatch. The callback reports the number of
fully completed blocks; partial progress is preserved on error, while success
with incomplete or over-reported progress is rejected. `BUSY` and `NO_MEDIA`
are represented in the portable result set. The callback contract requires
the transport to serialize each full request and to return in bounded time.

On macOS with AppleClang 21:

```sh
cmake -S . -B /tmp/maia-sd-block-host -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/maia-sd-block-host --target sd_read_test tab5_sdmmc_boot_test -j4
ctest --test-dir /tmp/maia-sd-block-host --output-on-failure -R '^(sd_read_test|tab5_sdmmc_boot_test)$'
cmake -S . -B /tmp/maia-sd-block-asan -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_C_FLAGS='-fsanitize=address,undefined -fno-omit-frame-pointer' \
  -DCMAKE_EXE_LINKER_FLAGS='-fsanitize=address,undefined'
cmake --build /tmp/maia-sd-block-asan --target sd_read_test -j4
ctest --test-dir /tmp/maia-sd-block-asan --output-on-failure -R '^sd_read_test$'
```

Results at the contract-only step: focused CTest 2/2; ASan/UBSan 1/1; public
header compiled as C++98 with warnings-as-errors. The P4 `maia_m0.elf` target
also built, compiling `sd_read.c`; that step had no product caller and made no
card access. The following addendum records the subsequent runtime backend
integration; device execution and real card-removal detection remain open.

### SDMMC native runtime service — 2026-11-21

The Tab5 product now links `boards/m5stack/tab5/sdmmc_boot.c` as a runtime
block service. Core 0 initializes and selects the card before the scheduler;
the product reserves its 640-byte, 128-byte-aligned IDMAC buffer from the
existing internal heap. The driver validates the SRAM range, uses the
non-cacheable alias for descriptors/data, serializes access with a
single-core IRQ-protected busy state, and performs each block of a range with
the already qualified CMD17/CMD24 path. Completed-block counts are preserved,
write completion still requires CMD13 READY/TRAN, and hardware failures are
sticky because controller/card recovery is not established.

The runtime cache adapter uses the same pinned P4 ROM entry points already
used by the isolated probes and `soc_psram_window.c`; it performs only a
range-scoped L1D writeback/invalidate and does not initialize or reconfigure
global cache state. Initialization runs before `maia_soc_systimer_init`; later
waits use `mcycle` so the driver does not reuse UNIT0 after the kernel claims
it. Product initialization errors are logged and do not stop the rest of boot.
If initialization succeeds, the shell task attempts a read-only LBA0 transfer.

The Tab5 pin map has no connected card-detect input (`DET=NC`), so the backend
does not claim to detect physical removal or report a trustworthy
`MAIA_SD_NO_MEDIA`; communication failures map to timeout/I/O/not-ready, then
leave the service faulted until reset.

Validation on 2026-11-21:

- The focused `sd_read_test` and `tab5_sdmmc_boot_test` passed (2/2). The latter
  now covers runtime service initialization, two-block read/write callbacks,
  sticky initialization failure, out-of-range rejection, partial progress on a
  later-block timeout and sticky failure after that timeout.
- The same two tests passed with ASan/UBSan (2/2); the complete host CTest suite
  passed (93/93).
- The P4 `maia_m0.elf` linked, and `maia_m0_image` produced an 814,096-byte
  five-segment XIP image with valid checksum/hash. Linker symbols show BSS
  ending at `0x4FF1E060` and the 16 KiB boot stack ending at `0x4FF22060`,
  within the configured `0x4FF28000` SRAM limit.
- The legacy `maia_sched_probe_ram` SDMMC block-0 target also built and passed
  the native runtime gate. The linker reported an RWX LOAD-segment warning;
  that diagnostic was not loaded onto the device.
- No device was flashed and no SD card I/O was performed in this step. Runtime
  LBA0 read under the scheduler, runtime multi-sector writing/restoration,
  physical fault injection and FatFs/VFS remain unqualified.

## SDMMC runtime-service task read/write qualification — 2026-11-21

The new RAM-only `MAIA_NATIVE_PROBE_SDMMC_RUNTIME` target initialized the
runtime service with interrupts disabled before scheduler startup, then called
the public block-device API from a MaiaRTOS task after the SYSTIMER scheduler
was active. The task read two contiguous sectors beginning at LBA 69318,
wrote a temporary two-sector pattern through one range request, and read it
back byte-for-byte. The backend issued CMD24 once per sector; both sectors
were confirmed ready before the request reported two completed blocks.

The target LBA was the start of FAT32 cluster 97, already identified as free in
both FAT copies. Before writing, the runner saved both original sectors and
confirmed the first against the independently preserved 512-byte reference
(SHA-256 `32aefed26b737027376dbc961157ae63cfaf6f34577c39d6b8aed51e4700a3c0`).
After the pattern readback, the runner wrote the saved 1024 bytes back and
verified both sectors byte-for-byte. No filesystem was mounted or modified.

The runner captured the full 16 MiB flash twice before testing; both captures
matched the supplied restore image. It restored and read back the complete
flash with SHA-256
`674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`, then
captured the restored MaiaRTOS boot through `msh>`. The test report, exact
sector backup, test pattern, operation captures and flash snapshots are in
`../../../maia-rtos-recovery/native-sdmmc-runtime-20261121/`.

The host runner parser test passed, and the complete host CTest suite passed
94/94. The host mock also triggered an overlapping request while a transfer was
active; it returned `MAIA_SD_BUSY` without additional MMIO. The focused test
passed with ASan/UBSan. The RAM image was validated inside the qualified
`0x4FF00000..0x4FF28000` window before loading; it was not written to flash.
Runtime fault injection, physical concurrency/stress, recovery after power
loss, FatFs and VFS remain open.

## Full XIP product boot with runtime SDMMC LBA0 read — 2026-11-21

The current product XIP image built as an 814,096-byte ESP32-P4 image,
entry `0x40000020`, five segments, valid checksum and validation hash; SHA-256:
`bf889774e0e2b5a11a12792f3f5c8c70e6de54f84a6ac5d1236d97519ba34659`.
Before the test, a fresh 16 MiB flash read matched the verified recovery image
byte-for-byte (SHA-256
`674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`).

`run_f0_flash_probe.py` backed up and validated the factory app, temporarily
wrote the product at `0x10000`, and captured one boot. The serial log contains
`[F2] Native SDMMC block service ready`, `[F2] Runtime SDMMC read LBA0 passed`,
the shell self-test, and `msh>`; no fatal marker was observed. This run only
read the card. The runner restored the original 2,093,696-byte app and verified
its hash. A second full 16 MiB flash read matched the pre-test capture exactly
with the same SHA-256.

The product image, both full-flash captures, factory app backup, boot logs and
runner report are in
`../../../maia-rtos-recovery/native-product-sdmmc-lba0-20261121/`. The SDMMC
initialization-failure/nonfatal path was not induced; physical card-removal
testing is unavailable through the board's `DET=NC` pin map.

## Product boot after an injected SDMMC error — 2026-11-21

With the explicit, default-OFF CMake option
`MAIA_DIAGNOSTIC_SDMMC_CMD8_TIMEOUT=ON`, the SDMMC backend returns the normal
timeout error in software immediately after CMD8 actually completes and before
its response is processed. The diagnostic banner identifies this injection.
The host test confirms one CMD0 and CMD8, reported timeout and CMD0 stage,
no published block device, and a sticky refusal without further MMIO writes.
This tests the product's failure propagation after host ownership is taken; it
does not simulate a controller still waiting for a real response, a physical
transport timeout, card removal, or DMA fault.

The diagnostic product image, SHA-256
`7af164556e9d59705ee447df8b22188a79c2ee6c978117963a4b690fb1d5fe35`, booted
once on the ESP32-P4 v1.3. UART captured the diagnostic banner,
`SDMMC service unavailable (error=-7, stage=6); boot continues`,
`qtest PASS`, and `msh>` without a fatal marker. No data-transfer command was
issued by this diagnostic. The recovery runner restored and verified the app;
16 MiB full-flash captures before and after the test match byte-for-byte, with
SHA-256 `674ce7e2efe406637f1534388bf4f70136a55c82a82824977a32268916e02e7c`.
The original app's boot capture also reached `msh>`. Image, logs, report and
flash captures are preserved in
`../../../maia-rtos-recovery/native-product-sdmmc-cmd8-fault-20261121/`.

The host default and injected tests passed 2/2, including ASan/UBSan. Full
host CTest passed 95/95. A default product rebuild with injection OFF was
byte-for-byte identical to the previously verified product image, SHA-256
`bf889774e0e2b5a11a12792f3f5c8c70e6de54f84a6ac5d1236d97519ba34659`.
Physical transport faults, concurrent physical requests, stress and power-loss
recovery are not qualified.

Reproduction (the flashing runner writes and restores the factory app):

```sh
cmake -S . -B /tmp/maia-product-sdmmc-cmd8-fault \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake \
  -DMAIA_USE_FLASH_PRODUCT_LINKER=ON \
  -DMAIA_DIAGNOSTIC_SDMMC_CMD8_TIMEOUT=ON \
  -DMAIA_ESPTOOL_EXECUTABLE=/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/esptool.py
cmake --build /tmp/maia-product-sdmmc-cmd8-fault --target maia_m0_image -j4
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python \
  tests/hardware/run_f0_flash_probe.py \
  --image /tmp/maia-product-sdmmc-cmd8-fault/maia_m0.bin \
  --out /path/to/new/recovery-directory \
  --port /dev/cu.usbmodem1301 --capture-seconds 45 --observe-seconds 2 \
  --marker 'msh>'
```

Separately capture and compare full flash before and after; check the diagnostic
and error lines in the probe log rather than relying solely on the runner's
`msh>` marker.

## FatFs native mount and file I/O — 2026-11-21

FatFs R0.15 with Espressif patch2 was taken from ESP-IDF v5.4.2 commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`, path
`components/fatfs/src`. The imported core retains ChaN's copyright and
redistribution notices. MaiaRTOS uses only `ff.c`, `ff.h`, `ffunicode.c`, and
`diskio.h`; it supplies its own `ffconf.h` and `diskio` adapter. ESP-IDF VFS,
FreeRTOS synchronization, `sdkconfig.h`, and ESP-IDF disk glue are excluded.
`FF_USE_MKFS=0`; exFAT is disabled; long filenames use UTF-8. FatFs is not
reentrant in this configuration; the MaiaVFS layer below serializes its calls.

Host validation used a synthetic FAT12 image in RAM. The invalid-image mount
returned `MAIA_FATFS_NO_FILESYSTEM` with zero writes. Mounting the valid image
also made zero writes. The test then created a long-name file, wrote/synced 700
bytes, read and compared it, updated one byte, verified the updated contents,
and removed the file. Repeating `FA_CREATE_NEW` for the existing path returned
`FR_EXIST` without a write. The focused test passed under ASan/UBSan; full CTest
passed 96/96.

The normal P4 product image was 816,864 bytes (SHA-256
`51aa400b501e6350c8ca8c362f2d3c1ac4f3c85aefd055a79cba8bc96970e5fc`).
The diagnostic image, built with default-OFF option
`MAIA_DIAGNOSTIC_FATFS_RW=ON`, was 831,840 bytes and passed image validation
(SHA-256
`a98fbe45dccca135bc3c4e651d69c6171a7bc5c38eaf2b006ba6952957440d83`).

On the Tab5, the diagnostic reported:

```text
[F2] Runtime SDMMC read LBA0 passed
[F2] FatFs mounted; media not formatted
[F2] MAIA_FATFS_RW_PASS create/update/read/remove
```

The diagnostic uses `FA_CREATE_NEW` for
`0:/MaiaRTOS FatFs test.txt`; if that name already exists, it exits without
changing or deleting it. It wrote and synchronized 700 bytes, read and
compared them, updated byte 19, read and compared again, and removed the file
before reporting PASS. The runner report records `probe_pass=true`,
`restore_verified=true`, and no fatal marker. It restored the factory app and
verified its app hash
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`; the
restored capture shows the legacy app mounting `/sd`. The test file was
removed, but normal FAT metadata sectors were updated; no byte-for-byte
restoration of the complete SD card was made. No full-flash comparison was
made in this run.

Image, probe/restore logs, app backup and report:
`../../../maia-rtos-recovery/fatfs-tab5-20261121/`. This qualifies the tested
single-task FatFs path, not multi-task stress, transport-failure recovery,
power-loss recovery, or file transfer through `maia.py`. No format command is
enabled or called.

## MaiaVFS `/sd` file API — 2026-11-21

The new `maia_vfs` library wraps FatFs with a native Maia C API for `open`,
`read`, `write`, `seek`, `sync`, `close`, and `unlink`. It accepts files only
under `/sd`; invalid mount prefixes, dot/parent components, backslashes and
drive-colon paths are rejected. It uses eight fixed file slots with
generation-bearing handles and sets `FF_FS_LOCK=8` to prevent incompatible
simultaneous opens. No heap or FreeRTOS/ESP-IDF runtime glue is used.

FatFs is configured with `FF_FS_REENTRANT=0`. Each VFS call uses the kernel's
nonblocking `maia_mutex_trylock`; concurrent callers receive `MAIA_VFS_BUSY`
without internal polling or yielding. Calls are thread-context-only;
mount/unmount require a quiescent system. A failed close preserves its handle
for retry. Directory enumeration and shell/Newlib adapters are not implemented.

Host validation extends `fatfs_test` over a synthetic FAT12 image. It covers
no-format mount, path confinement, invalid/missing paths, create-new and
non-truncating create, read/update/append/unlink, partial EOF, stale and
exhausted handles, file-sharing conflicts, injected lock/context errors, and
retry after a simulated disk-write failure. The focused test passed under
ASan/UBSan; CTest passed **96/96**; public headers compile with C++98 and
warnings-as-errors.

The normal P4 image is 818,704 bytes (SHA-256
`b4eb03d4e068185888d169d240e0eab1c7b167989aa6317f63d9721ce443a1a3`). The
default-OFF VFS diagnostic image is 838,880 bytes (SHA-256
`d233e1640fd5830158e245ea9307fd1b702e07a4485e740e4e1f3acb154a3e1b`); both
have five validated XIP segments and entry `0x40000020`.

On the Tab5, the final diagnostic emitted:

```text
[F2] FatFs mounted; media not formatted
[F2] MAIA_VFS_RW_PASS create/update/read/remove
```

The runner report records `probe_pass=true`, `restore_verified=true`, no fatal
marker, and a restored app SHA-256 of
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
The restored boot returned to `msh>`. The test file was created with
`CREATE_NEW`, removed before PASS, and never formatted the card. FAT metadata
sectors changed; the entire card was not backed up/restored, and no full-flash
comparison was made.

Reproduction:

```sh
cmake -S . -B /tmp/maia-vfs-p4-diag \
  -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake \
  -DMAIA_USE_FLASH_PRODUCT_LINKER=ON \
  -DMAIA_DIAGNOSTIC_FATFS_RW=ON \
  -DMAIA_ESPTOOL_EXECUTABLE=/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/esptool.py
cmake --build /tmp/maia-vfs-p4-diag --target maia_m0_image -j4
PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python \
  tests/hardware/run_f0_flash_probe.py \
  --image /tmp/maia-vfs-p4-diag/maia_m0.bin \
  --out ../../../maia-rtos-recovery/maia-vfs-tab5-final-20261121 \
  --port /dev/cu.usbmodem1301 --capture-seconds 45 \
  --marker MAIA_VFS_RW_PASS
```

Logs and report:
`../../../maia-rtos-recovery/maia-vfs-tab5-final-20261121/`. This is a
single-task hardware qualification; it does not prove concurrent stress,
directory operations, shell integration, transport-failure recovery, or
power-loss recovery.

## Native shell SD files on LCD, no desktop — 2026-11-21

This extends the preceding file-only qualification with MaiaVFS stat,
mkdir/rmdir and fixed opendir/readdir/closedir handles plus a Newlib adapter.
The existing native product already runs its shell on the native LCD service
and accepts USB serial/A164 keyboard input; no desktop or foreign runtime was
added. Default-OFF `MAIA_DIAGNOSTIC_VFS_SHELL=ON` starts in `/sd` and prints
instructions, without automatic SD writes. The host supplies the demo commands.

Host acceptance: CTest 96/96; `fatfs_test`, `newlib_locks_test`,
`newlib_heap_test` and `shell_expr_control_test` pass with ASan/UBSan.
Tests cover directories/EOF/stale handles/capacity, errno mapping, file
seek/append/truncate, invalid paths/flags, contention/context failures, failed
close retry, actual host shell commands, staged-output growth and exhaustion
without truncating an existing destination. Public VFS/FatFs headers compile
with `c++ -std=c++98 -Wall -Wextra -Wpedantic -Werror -I include -fsyntax-only`.
Host adapter tests call hooks directly; only the P4 test exercises Newlib stdio.

The first electronic attempt
(`../../../maia-rtos-recovery/vfs-shell-electronic-20261121/`) had a
`MAIA_NATIVE_FATAL cause=4c4f434b value=0000000e`. Disassembly of this compiler's
Newlib `_fclose_r` and `__sfp` confirmed close frees a dynamic FILE lock without
clearing its pointer and init later reuses that pointer. The host lock test
reproduced SIGTRAP before the native init fix. Reinitialization now allocates
a new dynamic lock rather than resetting released memory.

Two intermediate runs (`vfs-shell-lock-fix-20261121/` and
`vfs-shell-final-20261121/`) reached the host completion marker without fatal,
but **did not pass storage acceptance**: echo text went to the console,
cat reported a missing file and ls found zero entries. Their `probe_pass=true`
means only completion-marker acceptance. The shell's 64 KiB redirection
allocation exceeded the 32 KiB heap and failed silently. Shared pipe/redirection
staging now starts at 1 KiB, grows within the previous 64 KiB bound, and reports
allocation/limit errors without opening/truncating the destination.

The corrected electronic run
(`../../../maia-rtos-recovery/vfs-shell-rw-20261121/`) used this input:

```sh
if mkdir poc-20261121-1830; then
  cd poc-20261121-1830
  echo NATIVE_SD > poc.txt
  echo APPEND >> poc.txt
  cat poc.txt
  ls
  rm poc.txt
  cd /sd
  rmdir poc-20261121-1830
  echo MAIA_SHELL_SD_END
fi
```

The actual input was one semicolon-separated line ending in CR (existing shell
if syntax, not a parser change). mkdir failure skips the test, protecting
pre-existing data. The output now contains:

```text
NATIVE_SD
APPEND
Directory of /sd/poc-20261121-1830:
  [FILE] poc.txt (17 bytes)
Total 1 entries
MAIA_SHELL_SD_END
msh>
```

No cat/ls/rm/rmdir failure or fatal marker occurred. The 17-byte size matches
`NATIVE_SD\nAPPEND\n`; it is not acceptance based solely on a marker. The runner
restored the factory app, verified its hash
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`,
and captured its boot. The earlier empty test directories were also removed
by their own shell cleanup; no card format or full-card backup/restore occurred.

The final normal image is 847,072 bytes, SHA-256
`dbc358c040b135e32d62b219825a1353ef6d1cef848e59db8f38319f54f0e73c`;
the final POC image is 847,152 bytes, SHA-256
`2b8f37660fe0a8fc901ad9e390b335bddde4992aad4bbd673efe6ff5b863f284`.
Both package/validate with five XIP segments and entry `0x40000020`.

The final POC repeated the demo in `poc-20261121-visual`, then kept the native
prompt available for 120 seconds using the existing flash runner's
`--observe-seconds 120`. The owner reported **"Funcionou."** during this window,
confirming the visible shell-only POC. Evidence:
`../../../maia-rtos-recovery/vfs-shell-visual-20261121/`.
The final report records marker=true, fatal=false, 10,535 captured bytes and
restore_verified=true with the same backed-up app hash. The restored capture
returned to `msh>`. After the host demo, interactive `ls`, `ps`, `pwd` and
`ls lib` were captured; `/sd` had 60 existing entries and no leftover demo
directory. `ls ..`/`ls ../` were rejected by the existing dot/parent-component
confinement rule, not silently resolved as POSIX paths.

Reproduction: configure the normal P4 toolchain build with
`-DMAIA_USE_FLASH_PRODUCT_LINKER=ON -DMAIA_DIAGNOSTIC_VFS_SHELL=ON`, build
`maia_m0_image`, then use `tests/hardware/run_f0_flash_probe.py` with the POC
image, an exclusive recovery output directory, `--input-marker 'msh> '`,
the demo as `--input-hex`, marker CRLF + `MAIA_SHELL_SD_END` + CRLF, and
`--observe-seconds 120`. Do not use automatic FATFS_RW simultaneously.

Limits: fail-fast BUSY, not a blocking VFS API; eight file slots and four
directory slots also share FatFs's eight object locks. Newlib descriptors are
private to the adapter; it holds adapter → VFS locks for compound position
operations. Metadata/offsets above INT32_MAX return EOVERFLOW. O_TRUNC without
O_CREAT, read-only creation and unsupported flags return ENOTSUP. No rename,
actual POSIX permissions/timestamps, complete POSIX or full shell/apps/compiler
qualification. A failed native close keeps its descriptor; callers using
fclose must retain the raw fd if they need retry. Shell recovery after real
transport/close failures, power-loss recovery and cold/SMP storage remain open.
FAT metadata changed; no complete-card or full-flash comparison is claimed.

## MaiaVFS multi-task stress and injected I/O recovery — 2026-11-21

The default-OFF `MAIA_DIAGNOSTIC_VFS_STRESS=ON` test is limited to the Tab5 P4
and requires the flash-XIP product. It refuses to use a pre-existing test
directory and never formats the card. Two MaiaRTOS workers each completed 16
iterations of 512-byte write, sync, close, reopen, read/compare and EOF checks.
The test handles fail-fast `MAIA_VFS_BUSY` with a bounded 5,000-attempt,
one-tick retry; the passing run reported 6,173 retries. Each worker used its
own file. Successful completion removed both worker files and the test
directory; the separate temporary fault-test file was also unlinked.

The same opt-in diagnostic injects a one-shot write error before the real
disk-write callback, observed the sync error and then successfully retried sync.
It injects a read error before the real disk-read callback, closes the FatFs
object with its sticky read error, reopens the file and verifies all 512 bytes.
An explicit clear-pending-fault API and host regression ensure an unconsumed
injection cannot leak into a later I/O operation. This tests software error
propagation and recovery only; it is not a real SD transport fault, card-removal,
power-loss or media-integrity test.

Host CTest passed 96/96; the targeted FatFs fault test passed with
ASan/UBSan. Normal and stress-enabled P4 images built. The final recovery-wrapped
Tab5 run captured `MAIA_VFS_STRESS_WORKERS_PASS workers=2 iterations=32`,
`MAIA_VFS_FAULT_RECOVERY_PASS` and
`MAIA_VFS_STRESS_PASS ... files_removed=2`; no fatal marker occurred.
`report.json` records `probe_pass=true` and `restore_verified=true`. The runner
restored the factory app with hash
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`; the
restored boot loaded the desktop. Logs and report:
`../../../maia-rtos-recovery/native-vfs-stress-20261121-final/`.

Several earlier bounded diagnostic attempts failed before the final pass and
left their uniquely named test directories/files on `/sd` for inspection; no
format or full-card restore was performed. The successful run removed only its
own test files. The retry bound and coordinator stack were increased after
hardware observations; the final coordinator stack is 8 KiB. This is
single-core task contention, not SMP stress. Real transport/removal and
power-loss recovery remain open.

The host regression now also writes and syncs a 512-byte file, closes it,
unmounts/remounts FatFs on the same memory-backed block device, and verifies
the bytes after remount. It passes both normally and with ASan/UBSan. This is
an in-process host persistence check only; it is not a hardware reboot,
cold-boot, transport-failure or power-loss result.

## MaiaVFS persistence after clean Tab5 reset — 2026-11-21

The opt-in P4 diagnostic created a new file at
`/sd/MaiaVFSReboot-20261121/persist.bin`, wrote a deterministic 512-byte
record, called `sync`, and closed it. After a clean USB RTS reset, the next
boot mounted FatFs, reopened the file, compared all 512 bytes, confirmed EOF,
and emitted `MAIA_VFS_REBOOT_PERSIST_PASS`. The first boot emitted
`MAIA_VFS_REBOOT_ARMED`; both boots also passed the two-worker stress and
injected diskio-fault recovery markers. No fatal/stack-fault marker occurred.

The runner read the 3 MiB factory app region twice before flashing and required
identical backups. It restored that region after the test, verified the
readback byte-for-byte, and captured 25,735 bytes from the restored boot.
The restored boot log shows MaiaOS Desktop loading from `desktop.mobj`.
`report.json` records both boot stages and `flash_restore_verified=true`.
Evidence is in
`../../../maia-rtos-recovery/native-vfs-reboot-clean-20261121/`.

This proves persistence across a clean warm reset only. It is not a cold boot,
physical transport/card-removal, or power-loss test. The uniquely named
512-byte test file and directory were intentionally left on the test SD card
for inspection; no format or card-wide restore was performed.

## Simulated runtime transport faults — 2026-11-21

The owner cannot remove the card with the keyboard installed, so physical
removal is outside the current plan and remains untested. DET is not connected;
the Tab5 service cannot distinguish removal from another transport fault or
report it as `MAIA_SD_NO_MEDIA`.

The host MMIO mock now times out on the second block of both a two-block read
and a two-block write. Each returns `MAIA_SD_TIMEOUT` with one completed block;
the faulted service rejects subsequent reads and writes with
`MAIA_SD_NOT_READY`, zero completed blocks and no further MMIO. A separate
memory-backed block callback injects `MAIA_SD_TIMEOUT` and `MAIA_SD_NOT_READY`
through diskio and MaiaVFS, including failed `sync` and `close`. The mock is
manually restored for cleanup, not as evidence that the actual faulted service
can recover without reset. The three targeted host tests and two ASan/UBSan
tests passed.

Neither mock reproduces an electrical card removal, establishes the physical
state of a partially written block, or qualifies power-loss recovery. No
device access, removal or power interruption was performed for these tests.

## MaiaKV snapshots and native flash guard — host/build baseline

The native C MaiaKV service uses two 8 KiB banks in the reserved Tab5 range
`0xC74000..0xC77FFF`. Each update erases and rebuilds the inactive bank,
writes its payload and checksums, then writes a commit marker last. The previous
bank is left intact until a later update. Host tests inject failures in each
flash callback and reinitialize the service to verify recovery to either the
old committed value or the fully committed new value. Normal CTest and
ASan/UBSan passed. If both banks lack a valid commit, initialization returns
`MAIA_KV_RECOVERED_EMPTY` instead of silently reporting a normal empty store.

The ESP32-P4 product image linked and packaged with five segments. The
`flash_transaction` routine is in internal SRAM at `0x4FF00000`; its
disassembly calls ROM cache and raw SPI-flash routines without references into
the XIP text/rodata sections. At the time this baseline was recorded, no physical flash operation had been
run. The functional read/erase/write/reboot follow-up below supersedes that
limitation, but does not measure scheduler latency or cover power-loss
recovery. The native Tab5 profile registers the MaiaKV host imports for
MaiaScript; the compiler/emulator covers `get/set/remove/contains` (33/33
compiler cases).

### MaiaScript parser token-capacity fix and clean-image check — 2026-11-22

The parser now sizes its token vector from the expanded source length plus the
EOF token, capped at 32,768 entries, instead of reserving at least 1,024 tokens
for every input. `make -C maiascript test` passed all 33 compiler/emulator
cases and the AST-lifetime/repeated-parse checks.

The clean P4 image
(`30c44c1b5ddda2486fd7db887d0f30e0889ad2ed27e8bb7ef5173a6428aed0d5`) booted
on the Tab5. A minimal script, `i32 main() { return 42; }`, compiled through
`maiasc` to a 416-byte ELF32. The protected runner verified restoration of the
original application (SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`).
Capture and restore report:
`../../../maia-rtos-recovery/maia-maiasc-final-simple-20261122/`.

This initial attempt stopped after include expansion and before tokenization.
The later parser/CST, codegen and assembler memory reductions changed that
result; see the follow-up below.

### MaiaKV compilation and native module-run follow-up — 2026-11-22

The firmware heap was set to 44 KiB after a 48 KiB linker configuration
overflowed the validated SRAM window. The parser event log/CST were compacted,
the codegen optimizer avoids per-line string allocations, and the assembler
stores `string_view` source-line references and reparses each line in pass 2.
`make -C assembler test` passed 21/21 disassembly cases and assembled all 6/6
positive test files. `make -C maiascript test` passed 33/33 compiler/emulator
cases plus AST-lifetime and repeated-parse checks.

The protected Tab5 probe compiled a source importing `maia.kv` to a 2,224-byte
ELF32 and emitted `[SUCCESS] Compiled`. The instrumented image hash was
`9155df828390f7271d3d508e4066022a567d1d79119ff66da00144feecef9497`.
The runner restored the original application and verified SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Capture and restore report:
`../../../maia-rtos-recovery/maia-kv-assembler-pass2-consistent-name/`.

Execution is not yet demonstrated. `maiasc --run` compiled the plain
`i32 main(){return 42;}` source to 416 bytes, and the loader logged
`Executing maia_module_main`, but the probe did not observe a module-return or
success marker. This reproduces the run blocker without MaiaKV imports, so it
does not yet implicate the KV backend. A slightly larger KV source reached
grammar-parse success but did not reach the next CST diagnostic; its cause is
also unresolved. No KV read/write/persistence result is claimed.

Temporary parser, codegen, shell and assembler diagnostics were removed after
the probe. The rebuilt clean image
(`e9ec4b1879c6c8ebadf3a2d67c002b0c566b7f50c855d75228db53b15d880865`) was
then tested with the same import and compiled to a 2,236-byte ELF32. The probe
removed its scratch source/module files, and the protected runner restored the
original firmware with the same verified SHA-256. Capture and restore report:
`../../../maia-rtos-recovery/maia-kv-clean-final/`.

### Native ELF entry/return trace — follow-up

The device-generated assembly for `i32 main(){return 42;}` is an eight-
instruction, 32-byte function with a normal stack frame, local `jal` to its
epilogue, and `ret`. The host compiler/emulator reports 42; all 33 MaiaScript
compiler/emulator cases and the assembler's 21 disassembly plus 6 assembly
tests pass.

The protected Tab5 probe also ran a separately compiled C control module whose
entire `.text` is `addi a0, zero, 42; ret` (8 bytes). Both it and the
MaiaScript module reached `Loader: Executing maia_module_main`, but neither
produced the loader's return message or the following shell marker. A
MaiaScript probe that called `println` before returning produced no print
either. This localizes the blocker below MaiaScript parsing/code generation;
the loader/hardware execution path is still unresolved, and the absent fatal
marker does not prove that no trap occurred.

The minimal C ELF was allocated at `0x4ff133d8`; the clean-image map places
the general heap pool at `0x4ff12a90` within the linker-defined executable
`SRAM_LOW` range. This is an observation, not a confirmed cause. The protected
runner restored the original application after each probe and verified its
SHA-256 as
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Temporary diagnostic files were removed from the device SD card. Reports:
`../../../maia-rtos-recovery/maia-ret-trace-chunked/`,
`../../../maia-rtos-recovery/maia-hello-control-run/`,
`../../../maia-rtos-recovery/maia-ret42-c-control/`, and
`../../../maia-rtos-recovery/maia-module-debug-cleanup/`.

### Native ELF execution — cache coherency fix

Root cause of the missing return: the native loader only issued `fence.i`
after writing the module, whereas the ESP-IDF build calls
`esp_cache_msync(C2M|DATA)` and `esp_cache_msync(M2C|INST)`. On the ESP32-P4
those map to the ROM calls `Cache_WriteBack_Addr(CACHE_MAP_L1_DCACHE)` and
`Cache_Invalidate_Addr(CACHE_MAP_L1_ICACHE_MASK)` (ESP-IDF v5.4
`esp32p4.rom.ld`: `0x4FC003F4` and `0x4FC003E4`). The native loader now calls
`maia_esp32p4_cache_sync_code()` with the same operations over the line-expanded
module range.

Comparison with the working FreeRTOS build: `platforms/tab5/main/CMakeLists.txt`
compiles the same `components/exec/elf_loader.c` under `ESP_PLATFORM`, so
that build takes the `esp_cache_msync` branch. The FreeRTOS kernel sources
(`esp-idf/components/freertos`) contain no cache maintenance; the flush comes
from ESP-IDF `esp_mm/esp_cache.c` (`cache_hal_writeback_addr` /
`cache_hal_invalidate_addr`). The scheduler swap was therefore not the
difference. The native build compiled the same loader without `ESP_PLATFORM`,
so it skipped that branch.

With only that change (image
`4b22ef1f8727e69fe4002b556af44e10338ca38b58de3d37541ac9acfb15f3ea`), a
MaiaScript module returned `42`, but its `println` emitted only `M`: newlib
`_write` returned a partial write on the first busy USB IN endpoint. `_write`
now retries with the console's bounded spin; `newlib_syscalls_test` covers a
transient busy endpoint. With image
`1caec394e6ee787278e815177ebacb3417572b64b09dbdcf817474ec484da87a`, the probe
printed `MAIA_ENTRY_REACHED`, logged `Loader: Module returned 42 (0x2A)`,
removed its SD files, and the runner restored and verified the original
application SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Reports: `../../../maia-rtos-recovery/maia-exec-cache-sync/` and
`../../../maia-rtos-recovery/maia-exec-stdout-retry/`.

### MaiaKV Tab5 functional test — 2026-11-22

The current native product image (`1caec394e6ee787278e815177ebacb3417572b64b09dbdcf817474ec484da87a`)
was exercised on the Tab5 over USB with the protected flash runner. A
read-only call to `host_kv_contains_text("copilot_probe")` returned 0, so that
key was absent and was not modified.

The test then confirmed `mkvp_221758` was absent, called the registered
`host_kv_set_text("mkvp_221758", "ok")` and received return 0. On a subsequent
runner boot, `host_kv_get_text` returned `"ok"` and the module returned 42
after comparing both bytes. A third boot called `host_kv_delete_text`, checked
`contains == 0`, and confirmed `get` returned `MAIA_KV_NOT_FOUND` (-3). A
fourth boot again observed `contains == 0`, proving deletion survived reset.
These operations exercised the inactive-bank erase, write, verification, and
commit path. Removal is logical snapshot deletion, not secure erasure of the
older inactive bank.

All successful runner reports record `probe_pass=true` and
`restore_verified=true`; each restored the original application with SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
The temporary source/module files were removed from the SD card. Reports:
`../../../maia-rtos-recovery/maia-kv-device-host-contains-20261122/`,
`../../../maia-rtos-recovery/maia-kv-device-set-20261122/`,
`../../../maia-rtos-recovery/maia-kv-device-get-20261122/`,
`../../../maia-rtos-recovery/maia-kv-device-remove-20261122/`,
`../../../maia-rtos-recovery/maia-kv-device-check-removed-20261122/`, and
`../../../maia-rtos-recovery/maia-kv-device-temp-cleanup-20261122/`.

The initial wrapper probe stopped during code generation, before either
`[SUCCESS] Compiled` or `Wrote assembly` appeared. A direct call to the
registered C symbol compiled and ran. The follow-up below resolved this
specific wrapper path; this does not qualify arbitrary larger modules.
No power-loss, latency, or secure-erase qualification is claimed. The initial
compile-only capture is
`../../../maia-rtos-recovery/maia-kv-device-wrapper-assembly-20261122/`.
The minimal control and operation sources are in `../../tests/hardware/`.

### MaiaKV wrapper compile and run — clean Tab5 image, 2026-11-22

The on-device stall was reproduced after parsing and at the final function
output: the assembly stream had reached 4,082 bytes, with 22.5 KiB total heap
free but no available 8 KiB contiguous block. The shell stack had over 11 KiB
of remaining guard margin, so increasing stack was not the fix. Redistributing
the static L2 pool/task-stack budget to a 49 KiB heap and 16 KiB shell stack
provided the required contiguous allocation; the small-shell/telemetry boot
self-test and the actual wrapper run both passed.

On the Tab5 over USB, the original
`maia_kv_wrapper_compile_probe.ms` (`import maia.kv`, `kv.contains`,
`isError()`, and `return 42 + r.value()`) compiled, assembled, and executed.
The loader reported `Module returned 42 (0x2A)`. This was repeated on the
uninstrumented image with SHA-256
`1d1f83ca2ed12e1fb969f1ff6e1a3e99aea8323e4fa2c3605afd4ab8f38b439c`.
`make -C maiascript test` passed 33/33 compiler/emulator cases and the
AST-lifetime/repeated-parse checks; `make -C assembler test` passed 21/21
disassembly tests and assembled all 6/6 positive sources.

The protected runner reported `probe_pass=true` and `restore_verified=true`,
restored the original application with SHA-256
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`, and
the shell removed `/sd/kv_device_probe.ms` and its `.mobj`. Capture and restore
report: `../../../maia-rtos-recovery/maia-kv-device-original-clean-heap49-20261122/`.
This resolves UQ-018 for the tested wrapper; broader module-size limits,
power-loss recovery, latency, and secure erase remain unqualified.

The library path `/sd/lib/maia/kv.ms` was then read through the
`MaiaSerialClient` transfer implementation in `tools/maia-cli/maia.py`. The
file already existed, so it was not overwritten. A full 1,259-byte pullback
matched the workspace source exactly (SHA-256
`2ac4734b6bb1cea998fa5b49f1f742d0dee04ab341f80dba388ad2a35e8e60e0`).
The protected session restored and hash-verified the original app; report:
`../../../maia-rtos-recovery/maia-kv-push-library-client-heap49-retry-20261122/`.

### Native desktop stack correction and visual startup — 2026-11-25

The P4 product image was rebuilt with the native loader fix and passed the
structural image validator (five segments, 889,216 bytes). Three focused host
tests passed: `legacy_host_compat_test`, `elf_loader_context_test`, and
`elf_loader_context_no_tls_test`.

The USB-only visual runner uploaded the rebuilt
`desktop.mobj` (27,180 bytes; SHA-256
`c6818372711b42925266ac5d3881e41c9a0a8c79ca7d063a5b2ab6ec1c21b7e4`) to a
temporary SD path and ran it for 60 seconds. The final report has
`probe_pass=true`, `fatal=false`, and `restore_verified=true`. The owner
confirmed the desktop elements were visible and stable. The temporary module
was removed; the restored application SHA-256 is
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Product image SHA-256:
`6c329335bd2dd0ae6a743b5842129ccc838be400c633ba8402856a4ee3493d6c`.

The first attempt after making the legacy scope conditional still faulted in
`send_command`; its capture is retained and restoration was verified. The
final change also removes one 256-byte local section-offset table by reusing
the address table between layout and relocation. Disassembly of the final ELF
shows a 1,648-byte loader stack frame; this is not a measurement of total
remaining stack margin or high-water usage. The final capture is at
`../../../maia-rtos-recovery/native-tab5-desktop-native-visual-probe-20261125-05/`;
the earlier fault is at
`../../../maia-rtos-recovery/native-tab5-desktop-native-visual-probe-20261125-04/`.
### Desktop app launch and return — 2026-11-25

A second USB-only probe uploaded `desktop.mobj` to the temporary path
`/sd/desktop-interaction-probe-20261125-06.mobj` and ran it for 120 seconds.
The report records `probe_pass=true`, `fatal=false`, `runelf_started=true`,
`active_until_reset=true`, and `restore_verified=true`. The host transcript
records removal of the temporary module and successful probe completion. The
restored app hash is again
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.

During the observation window, the owner confirmed that touching the Banner
icon opened the app and that it returned to the desktop. This validates the
touch-to-launch-and-return flow for Banner only, on the tested warm boot path.
Other app launches, keyboard shortcuts, cold boot, and stack high-water margin
remain unverified. Capture:
`../../../maia-rtos-recovery/native-tab5-desktop-interaction-probe-20261125-06/`.

### Files input, VFS listing, and loader timing — 2026-11-26

The product shell now normalizes the A164 keyboard's ANSI arrow sequences for
MNM modules while preserving raw shell input. Focused host tests
(`native_shell_display_test`, `elf_loader_context_test`) passed, and the Files
module compiled as a RV32IMAF relocatable ELF.

On the Tab5, direct execution of `/sd/apps/files.mobj` loaded a 19,708-byte
module. Diagnostic timing was 187,434 us for validation and 16,745,886 us for
load/relocation. The MaiaVFS direct-directory path listed `/sd` (74 entries)
in 130–137 ms and `/sd/DCIM` (2 entries) in 59,338 us. The earlier `/sd`
listing through per-entry `stat` took about 4.35 s. The owner confirmed that
the up/down arrows did not close Files and that X closed it normally; the
module returned code 0. The runner reports `probe_pass=false` for this
interaction because the app returned before the requested 120-second window,
but it observed no fatal and verified restoration.
The owner described the observed response time as similar to the reference
desktop app and later confirmed that response time was normal and the arrows
were working.

The first diagnostic image kept a 512-byte read cache in `cmd_runelf`'s task
stack and produced `MAIA_STACK_FAULT reason=1` with `sp=0x4ff0ca30`, below the
shell stack base at `0x4ff0cb10`. The cache was moved to static storage. The
rebuilt diagnostic image kept the desktop alive for 120 seconds without a
fatal. Its desktop loader still measured 25,051,883 us; the Files loader and
remaining module-load latency therefore still need optimization. During that
diagnostic desktop probe, the owner could not open Files before the runner's
automatic reset; the later production probe confirmed that launch path.

The final diagnostic image SHA-256 was
`c8b1b52bcec92b6354f2be7d74a4b234144574c789c2546de4aaa5000a5090db`.
All probes restored the original app; the verified app SHA-256 was
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Captures:
[desktop stack-fault attempt](../../../maia-rtos-recovery/native-tab5-desktop-files-fastinput-probe-02/),
[desktop stable run](../../../maia-rtos-recovery/native-tab5-desktop-files-fastinput-probe-03/),
[Files direct baseline](../../../maia-rtos-recovery/native-tab5-files-direct-probe-01/),
[Files direct interaction](../../../maia-rtos-recovery/native-tab5-files-direct-probe-02/).

After the device probes, the production image was rebuilt with
`MAIA_DIAGNOSTIC_MODULE_TIMING=OFF`. Structural validation passed with five
segments; the 889,888-byte image SHA-256 is
`b0337655064a6a6271cdb12a2fed73dac25446317f9facb6dbe25fc6ddb2a7ee`.

A [USB-only production probe](../../../maia-rtos-recovery/native-tab5-desktop-files-production-20261126-01/report.json)
ran the installed desktop with this image. The desktop stayed active until
reset, no fatal marker was reported, and the original factory app was restored
with the same verified SHA-256 (`probe_pass=true`, `restore_verified=true`).
The owner confirmed that Files opened from its desktop icon in about 15 s.
After opening, entering `/sd/apps` was nearly instantaneous, up/down arrows
worked, and clicking X returned immediately to the desktop. These are the
owner's observations; the non-instrumented serial log confirms desktop
execution but does not record the navigation or break down launch time.

### HP core 1 startup reference — 2026-11-26

The isolated probe does not enable the single-core scheduler/trap metadata on
core 1. The source protocol and subsequent target evidence are recorded below.
Pinned primary source: ESP-IDF v5.4.2 local HEAD
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` (the files below have no local diff).

- [cpu_start.c, start_other_core](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_system/port/cpu_start.c#L293):
  unstall core 1, enable its clock, release global reset, then publish the entry
  via `ets_set_appcpu_boot_addr`. `call_start_cpu1` clears that boot address.
- [cpu_utility_ll.h, unstall](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/cpu_utility_ll.h#L43):
  PMU core-1 stall code `0xff`, with core-stalled status checked before proceeding.
- [hp_sys_clkrst_reg.h, SOC_CLK_CTRL0 and HP_RST_EN0](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/hp_sys_clkrst_reg.h#L144):
  clock register `0x500e6014`, core-1 clock bit 4; reset register `0x500e60c0`,
  core-1 global-reset bit 8.
- [pmu_reg.h, CPU_SW_STALL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/pmu_reg.h#L4718):
  core-1 stall code is bits 23:16 at PMU + `0x200` (`0x50115200`).
- [hp_system_reg.h, CPU_CORESTALLED_ST](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/hp_system_reg.h#L359):
  core-1 stalled status is bit 1 at HP_SYS + `0x64` (`0x500e5064`).
- [lp_system_reg.h, BOOT_ADDR_HP_CORE1](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/lp_system_reg.h#L827):
  full-width boot mailbox at LP_SYS + `0x164` (`0x50110164`).
  Base addresses come from the same revision's
  [reg_base.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/reg_base.h).
  The local official `esp-rom-elfs/20241011/esp32p4_rev0_rom.elf` independently
  disassembles `ets_set_appcpu_boot_addr` at `0x4fc05640` as a store to this
  address (ROM jump-table entry `0x4fc000a8`). This checks the published API;
  actual rev-v1.3 startup still requires target evidence.

The probe adapts only that register protocol, preserving Apache-2.0 attribution;
no vendor header, FreeRTOS scheduler, or ESP-IDF runtime is linked. The secondary
entry and worker must reside in the already qualified low-SRAM window so the
probe does not assume core-1 flash/PSRAM cache setup. Shared aligned 32-bit words
use release/acquire ordering; the existing single-core drivers stay unused.

The first protected flash run returned `MAIA_CORE1_BUSY` before writes:
`clock=e6dfb7ff reset=38000040 stall=00ff0000 boot=00000000`.
It reported no fatal and restored the original app with hash verification.
The initial diagnostic guard incorrectly rejected an already clocked,
reset-released secondary; the vendor startup explicitly permits that state.
The corrected guard rejects a nonzero boot mailbox and restricts use to initial
boot while ROM still owns the secondary, not runtime takeover. Capture:
`../../../maia-rtos-recovery/native-tab5-core1-startup-20261126-01/`.

The corrected isolated image ran twice on the rev-v1.3 Tab5 over the existing
USB cable. Each boot produced:

```text
CORE1 result=00000000 completed=00000005 hart=00000001 sp=4ff00510 rounds=00000400 cause=00000000 pc=00000000
CORE1_INITIAL clock=e6dfb7ff reset=38000040 stall=00ff0000 boot=00000000
MAIA_NATIVE_CORE1_PASS
```

Each round publishes a distinct payload using a release command, checks the
core-1-transformed result after an acquire acknowledgement, then proceeds.
The final stop acknowledgement is accepted only with state 2 and intact stack
guard. The dedicated secondary code (including its fault sink) occupies a
256-byte SRAM section at `0x4ff00000`; the stack is at `0x4ff00110` and the
mailbox at `0x4ff00510`. The worker does not call any function or access
flash/PSRAM and does not use floating-point instructions. No core-1 interrupt,
scheduler, TLS, cache behavior or driver concurrency is qualified by this test.
The stack pointer and guard are checked; this assembly worker has no C call
frames, so it does not qualify future task-stack requirements.

The [protected runner report](../../../maia-rtos-recovery/native-tab5-core1-startup-20261126-02/report.json)
records two boots with `marker=true`, `fatal=false`, `probe_pass=true` and
`restore_verified=true`. Probe SHA-256:
`e699a73d179343c42b0454cbc374fc9598ee4b069a65a76a7a76d6061bff99a3`.
Restored original app SHA-256:
`08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
Only the app region was temporarily flashed; bootloader, partitions and SD
contents were not written. Procedure:

```sh
cmake --build <p4-build> --target maia_core1_probe_image maia_f0_flash_probe_image -j 8
<idf-python> tests/hardware/run_f0_flash_probe.py \
  --image <p4-build>/maia_core1_probe.bin --out <new-recovery-directory> \
  --port /dev/cu.usbmodem1301 --marker MAIA_NATIVE_CORE1_PASS \
  --capture-seconds 15 --boots 2
```

Host validation on macOS: build `core1_start_test`, then
`ctest --test-dir <host-build> --output-on-failure -R '^(core1_start_test|native_probe_gate|f0_flash_runner_test)$'`
passed 3/3. The new host test covers wrong context, occupied boot mailbox,
already-clocked startup, dropped writes, stuck unstall, boot/round/stop
timeouts, fault reporting, wrong hart/SP, uncleared ROM mailbox, corrupt
results, wrong stop state, guard damage, and rejection of retries.
The same C test passed under host ASan/UBSan, and the public header passed a
C++11 syntax check. The isolated ELF has no undefined symbols and no symbols
matching the checked `FreeRTOS|xTask|vTask|esp_` runtime-name patterns; this
supports, but is not by itself a complete dependency audit.
Both isolated flash images passed the existing image validator. Rebuilding the
product with the pre-existing working-tree Files changes retained its exact
pre-probe SHA-256
`b0337655064a6a6271cdb12a2fed73dac25446317f9facb6dbe25fc6ddb2a7ee`.
No product performance improvement is claimed; IPI and per-core interrupt
state are the next independent gate.

## HP core 1 online in the product firmware — 2026-11-26

`maia_m0` (flash-XIP product, `MAIA_PRODUCT_CORE1=1`) calls
`maia_esp32p4_core1_start()` from `maia_boot_main` before any IRQ is enabled.
It reuses the probe's register protocol and SRAM worker; after 64 verified
exchanges the worker stays online, increments `heartbeat` and services the
mailbox (no park). `maia_esp32p4_core1_ping()` performs one bounded exchange
from any core-0 task, rejects concurrent callers and marks core 1 offline on
timeout/fault. Worker code lives in `.maia_core1` inside the `flash_iram`
segment (5 LOAD segments retained); the unused worker stack was reduced to
64 bytes so SRAM ends exactly at `__stack_top = 0x4ff28000` (zero margin).

Image SHA-256: `c591af64680bb191bbfedc7f43967b08fcf75f6ab56c840f5777c90aa667d381`.

Host: `core1_start_test` (probe + start/ping, timeout/fault/verify/busy paths)
passed natively and under ASan/UBSan; focused CTest 5/5.

Tab5 rev-v1.3 over USB only:
- `native-tab5-core1-product-20261126-01` (`run_f0_flash_probe.py --boots 2
  --marker "Cores: 2 online" --input-marker "msh> "`, input `smp` ×3 +
  `cpuinfo`): both boots printed `[F9] MAIA_CORE1_ONLINE hart=1
  sp=0x4FF1DA50 rounds=64`; every `smp` returned `result=0` with heartbeat
  advancing (e.g. 988174 → 1161297 → 1346157); `probe_pass=true`, no fatal,
  `restore_verified=true`, restored app SHA-256
  `08cee753c0c7c1bd8c69ea4bcaebf41dea5e8c19d1136f7838a8e36ca96abc06`.
- `native-tab5-core1-desktop-20261126-01` (`run_mnm_visual_app_probe_usb.py
  --app /sd/desktop.mobj --observe-seconds 120`): core 1 online, desktop
  stayed up 120 s, `probe_pass=true`, no fatal, `restore_verified=true`.

Limits: core 1 runs no scheduler, libc, TLS, drivers, flash or PSRAM code and
there is no IPI, so no performance improvement is claimed.

## Product heap expanded to ~256 KiB (high L2MEM + reclaimed bootloader SRAM) — 2026-11-26

`maia_m0` with `MAIA_PRODUCT_HIGH_SRAM=ON` (default): shell/LCD/terminal/touch
stacks live in NOLOAD `.maia_high_bss` (`0x4FF40000`, 46 KiB, zeroed by
`start.S` after checking `CACHE_L2_CACHE_CACHESIZE_CONF_REG 0x3FF10278` is
128 or 256 KiB). Heap = low pool 78,848 B + reclaimed 2nd-stage bootloader
area `0x4FF28000..0x4FF3AFC0` (77,760 B) + `.maia_high_heap`
`0x4FF4B800..0x4FF65800` (106,496 B) = 263,044 B. The image keeps 5 LOAD
segments; high sections are `:NONE`.

Before adding the high region the boot writes `li a0,0x5A; ret`, calls
`maia_esp32p4_cache_sync_code`, `fence.i`, and executes it. Without the L1D
writeback the first attempt (`native-tab5-sram-heap256-20261126-01`) trapped
with Illegal instruction at `PC=0x4ff4b802`, `MCAUSE=0x30000002`, so the
bootloader PMP does allow execution there and the cause was cache coherence.

Image SHA-256: `6b7ed1442f2d5a07f4e2f86aa65b68ff0fa4cef4bd093e269a90b849f10e1587`.

Tab5 over USB only (outputs in `maia-rtos-runs/`):
- `native-tab5-sram-heap256-20261126-02` (`--boots 2`, input `mem`, `smp`,
  `cpuinfo`): both boots printed `[F1] Heap += bootloader SRAM 0x4FF28000
  bytes=77760` and `[F1] Heap += high SRAM 0x4FF4B800 bytes=106496`; `mem`
  total=263044 B; core 1 online, `smp` result=0; `probe_pass=true`,
  `restore_verified=true`.
- `native-tab5-sram-heap256-desktop-20261126-01` (desktop 120 s):
  `probe_pass=true`, no fatal, `restore_verified=true`.
- Files alone 60 s passed on the intermediate image (reclaim only).
- Control `native-tab5-control-files-desktop-20261126-01` (build with
  `MAIA_PRODUCT_HIGH_SRAM=OFF`, SHA `4d3e23e5…06ac`): Files then desktop in the
  same session fails with `Native graphics transaction failed`, the same as the new
  image, so this is pre-existing. Restored and verified.

Incident: an earlier run (`native-tab5-sram-expand-desktop-20261126-03`) died
because the host disk was full (`ENOSPC`) and left the probe image on the
device. The factory app was restored manually with `esptool write_flash
0x10000` and verified by a full readback (SHA-256 `08cee753…bc06`). Run outputs
now go to the external SSD.

Host: focused CTest 4/4 passed.

## Native PSRAM X8 adoption after warm reset — 2026-11-26

Root cause of the former "Files then desktop" failure: the visual runner resets
by RTS between apps; the powered AP HEX device keeps MR8=0x0B (X8) from the
previous native boot, and `maia_esp32p4_psram_x8_mode(0)` required 0x4B (X16,
IDF state), returning BUSY. Diagnostic run `native-tab5-psram-warm-diag-20261126-03`:
boot 1 (after IDF) passed; boot 2 read MR0=0x8D30, MR2=0xA0DF, MR4=0x20,
MR8=0x0B and failed x8_mode with 0xFFFFFFFB.

Fix: `x8_mode(0)` adopts an already-X8 device only if the full X8 profile
matches and an MR8 reread is 0x0B (`stage=4`, no writes); otherwise BUSY as
before. The product prints MR0/2/4/8 on failure. Host test `psram_array_test`
covers adoption and rejection of a mismatched profile (PSRAM CTest 18/18).

Tab5 over USB only:
- `native-tab5-psram-warm-fix-20261126-01` (`--boots 4`): all four boots
  reached `Native LCD service ready`; boot 1 x8 stage=3, boots 2–4 stage=4;
  `probe_pass=true`, `restore_verified=true`.
- `native-tab5-files-desktop-psramfix-20261126-01` (image SHA
  `be305dd4…f7d1c`): `/sd/apps/files.mobj` then `/sd/desktop.mobj` in the
  same session, 40 s each: both `APP_STAGE_PASS`; restored and verified.

Cold boot still pending.

## Fast `.mobj` loading (whole-file read + reader release hook) — 2026-11-26

Timing build (`MAIA_DIAGNOSTIC_MODULE_TIMING=ON`, `maia-rtos-runs/build-p4-timing`):
- Before, `native-tab5-loader-timing-20261126-01`: Files validation=183481 us,
  load-relocate=16755724 us; desktop validation=206019 us,
  load-relocate=24941228 us.
- After, `native-tab5-loader-timing-20261126-02`: Files validation=565 us,
  load-relocate=9189 us; desktop validation=588 us, load-relocate=12402 us.

Cause: `runelf` streamed through a single 512-byte page cache while relocation
reads headers, symbols and strings at random offsets. Fix: `runelf` reads the
whole module into the heap (falling back to the paged reader on allocation
failure); new optional `elf_reader_t.release` is called by the loader after
relocation and before execution, freeing the buffer while the module runs.
Host: loader/shell CTest 7/7 (`elf_loader_context_test` asserts no release on
a failed load).

Product image (SHA `69b8b471…8f23`), `native-tab5-desktop-fastload-20261126-02`:
desktop 120 s, `probe_pass=true`, `restore_verified=true`; owner opened Files
and other apps from the desktop: fast and working. MaiaScript calculator
drawing still slow.


## Dirty-box framebuffer preserve and MaiaScript fs imports — 2026-11-26

- `native-tab5-desktop-dirtybox-20261126-01` (SHA `3b06688f…1e91`): desktop 120 s, `probe_pass=true`, `restore_verified=true`;
  owner: calculator fast as reference; Notes crashed to cyan.
- `native-tab5-notes-direct-20261126-01`: loader warnings `Unresolved external symbol 'host_fs_*'` — root cause.
- `native-tab5-notes-fsimports-20261126-01` (SHA `e8d79691…ac85`): Notes loads with no unresolved symbols.
- `native-tab5-desktop-fsimports-20261126-01`: desktop 120 s; owner: Notes, calculator, Files and Terminal all working.

## SMP scheduler on both HP cores — 2026-11-26

Image built from `build-p4-threads` with `MAIA_PRODUCT_SMP=ON` and shell affinity 2. Runs are in `maia-rtos-runs/`.

- `smp-2-181407`: `smp` reports `scheduler cores=0x3` and `shell runs on core 1`. `ps` shows `lcd` RUN on CPU 0 and `maia_shell` RUN on CPU 1 at the same time. `ipi1` grows at about 1 kHz.
- `smp-3-181542`: a KV module on core 1 returned `IO_ERROR`, because `valid_buffer` only accepted low SRAM (task stacks are now in high SRAM). Fixed by widening the range to `0x4FF00000..0x4FF80000`.
- `smp-4-181718` and `smp-5-*`: the KV module (8× set/contains, then delete) returns 0 twice. Counters show `freezes=234 migrations=234`, and the shell is back on core 1 afterwards.
- `smp-desktop-182005` and `smp-desktop2-200302`: the USB-Serial/JTAG link dropped during `write_flash`, which left a partially written app (black screen). The reference was rewritten manually with hash `08cee753…` verified. `run_f0_flash_probe.py` now retries `write_flash` up to 3 times.
- `smp-desktop3-200439`: desktop ran 120 s with `probe_pass=true` and `restore_verified=true`. Owner checked Files, Notes and the calculator: all working, but MaiaScript apps are "a bit slower" than the reference.
- Host CTest: 112/112, including the new `scheduler_smp_test`.

## Direct scanout drawing — 2026-11-26

- Image SHA `c8186f46…5836` (`build-p4-threads`, SMP on, shell affinity 2).
- `direct-lcd-202118`: boot OK, `probe_pass=true`, `restore_verified=true`. Nobody was watching the screen.
- `direct-lcd-211624`: desktop ran 120 s, `probe_pass=true`, `restore_verified=true`. Owner: "Está excelente! Parece mais rápido para carregar os apps." The calculator buttons, Files scrolling and the other apps now match the reference.
- Host CTest: 112/112.

## Ten-minute SMP driver stress — 2026-11-26

- `smp-driver-stress-20261126-retry-212721`, image SHA
  `c8186f46…5836`: `probe_pass=true`, `restore_verified=true`, no fatal boot.
- Native ST7121 service reported ready. MaiaScript ran on the shell-affined
  core 1 for 600 s while LCD/DMA scanout continued; each iteration exercised
  read-only KV lookup, SD stat, direct RGB565 drawing/flush, and a 1 ms yield;
  `/sd` directory enumeration ran every 16 iterations.
- 3,703 iterations, 19 progress checkpoints, `MAIA_SMP_STRESS_PASS`, module
  exit code 0. Temporary module removed. No KV/SD writes were performed.
- Initial probe incorrectly treated legacy module `touch_poll` returning `-1`
  as a driver failure; the native shell host does not expose that legacy input
  API. The corrected probe checks the native ST7121-ready boot marker instead.
- Host runner test passed; full CTest: 112/112.
