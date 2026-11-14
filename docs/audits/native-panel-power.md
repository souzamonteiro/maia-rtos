# Native bounded panel power-state readback — 2026-11-14

**HARDWARE-TESTED, LIMITED.** Two RAM loads observe stable RDDPM08 before the
selected native ST7121 program and9C afterwards. Each phase has four error-free
responses; DCS SleepOut(bit4) and DisplayOn(bit2) change from0 to1. This establishes
reported panel power-state effects beyond host FIFO drain. It does not prove
pixels, backlight, all vendor registers, color format, video timing or native DPI.

## Primary facts and qualification

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[panel commands](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/include/esp_lcd_panel_commands.h)
LCD_CMD_RDDPM=0A. Existing HAL/LL packet fields and read protocol references remain
those in [RDDID/BTA audit](native-dsi-read.md): DT37/MRPS, DT06/DCS read, LPbits24/18,
BTA/RXVC controls, FIFO/readbusy/direction and checked restoration.
[Linux v6.12 DCS definitions](https://github.com/torvalds/linux/blob/v6.12/include/drm/drm_mipi_dsi.h)
MIPI_DSI_DCS_POWER_MODE_* corroborate Display bit2 and Sleep bit4 positions.
Only protocol facts consulted: no Linux headers, runtime or implementation copied.

Supplier-hosted [Sitronix ST7121P datasheet V1.0](https://www.display-lcd.com/data/upload/admin/202509/68b935053144a.pdf),
2024/01 section6.1 p95 and6.2.6 p103 corroborates RDDPM0A/one parameter,
availability during sleep-in/out and bit values: SleepOut1/DisplayOn1.
The device's P suffix is NOT independently established; this document corroborates
standard DCS semantics, not exact die/model identification or arbitrary vendor
register behavior. Working BSP still selects ST7121 from observed touch firmware1.
The datasheet table has a duplicated column heading; interpretation is cross-checked
with standard bit definitions, not guessed from that typo. Full snapshots/hash/
selected text stay in local recovery, not imported into the source tree.

## Narrow API and implementation

Add `maia_esp32p4_dsi_read_power_mode(budget,report)` with fixed VC0/RDDPM0A,
MRPS1/header137 and requestA06. Extract the already verified read implementation
into a PRIVATE helper receiving two fixed header words; public RDDID04/MRPS3
wrapper retains its original values and contract. No arbitrary public command,
response size, bus queue or runtime ownership API introduced. Guard, budget,
error, PHY direction, single-pop and success-only restoration code shared unchanged.

Power byte is low8 of raw `word`, also available as `id[0]` in the existing report.
Other word bytes/id[1..2] are not requested payload and must not be decoded.
At most one RX word after data-ready/readbusy0; excess data/error rejects/sticks
across identity/power/short/long until reset. Do not infer exact response framing/
length from FIFO availability. MRPS1 remains protocol state after success; later
RDDID explicitly requests3 again. Host controls PCKHDL/RXVC/LP restored only after
verified empty FIFOs/TX direction, with unchanged full root/supply/config guards.
No error masks/clear, payload writes, visual changes or retries in this read API.
Apache-2.0 packet-definition attribution retained; native state machine original MIT.

New opt-in PANEL_POWER diagnostic requires ST7121 profile and all reset/DSI
prerequisites. After initial RDDID validation, read power four times, execute the
unchanged31-packet program, validate its four RDDID responses, then read power four
times again. Each phase must be stable. Require `(before &14h)==0` and
`(after &14h)==14h`; unrelated bits and full08/9C values are NOT hardcoded gates.
A panel already reporting both bits before startup cannot satisfy transition proof.
No corrective display/sleep writes are added if the observed state is unexpected;
fixture halts and runner resets to preserved flash.

## Verification and actual evidence

Host39/39 CTest PASS. Actual shared ID/power driver fixture verifies exact MRPS1/
RDDPM headers, upper-word garbage allowed without extra-byte interpretation,
successful power->identity reuse, timeout/excess data/restoration failure/PHY error
and sticky rejection in both APIs. Existing identity fault/guard/budget tests run
through the same helper. Capture regressions reject wrong command, malformed/
missing/duplicate/out-of-order samples, failed restore, wrong byte view, missing
SleepOut/DisplayOn, already-on baseline and false success. A positive case varies
unrelated bits to prove acceptance is based on14h rather than exact08/9C.

ASan/UBSan shared read fixture PASS; C++98 public header PASS. Five compiled runtime
mutants rejected: wrong power command, MRPS3 instead of1, pop empty FIFO, leave BTA
on, ignore PHYbit20. Invalid native option combination rejects. Native GCC14.2
RV32IMAC/ILP32 build/runtime gate PASS (project+libgcc, no foreign/unresolved
symbols). Existing RWX/empty removed-segment warnings unchanged. No SDK/FreeRTOS
runtime imported; no parser/EBNF/default production changes.

Two final RAM loads, Tab5 chip1.3/40MHz/cu.usbmodem1301, sixteen total power reads:

- Each before phase: four raw words00000008, byte08, mode90000 restored.
- Each after phase: four raw words0000009C, byte9C, modeB0000 restored.
- Every RDDPM: result0/stage10/requestA06/headers3/polls6/oneword, restore mask5,
  PHY15BD/FIFO50015/error0=error1=0. Summary08->9C/SleepOut1/DisplayOn1.
- Prior native gate/VO3/reset/touchFW1/DSI/short/long/read and full selected program
  still PASS, including61-byte command and1150ms requested waits.
- Both runner exits0/PASS, reset to installed flash. Paced maia.py confirms shell
  echo, SciCalc `2+3*4=14`, module exit0 and desktop left active. No flash/SD writes.

Reported power bits are evidence from the panel; they do not establish physical
image output or every vendor-table effect. No physical voltage/frequency/WCET or
cold power sequencing guarantee. The complete working desktop still uses FreeRTOS.

## Reproduction, files and recovery

Full prior ST7121 configuration plus `-DMAIA_NATIVE_PROBE_PANEL_POWER=ON`, build
`/tmp/maia-native-panel-power-20261114`. Changed SoC read helper/fixed wrappers and
header/constants, private diagnostic integration, CMake option, runner and read/
capture fixtures; no board command table or delays modified.

```sh
cmake --build /tmp/maia-native-panel-power-20261114 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-panel-power-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-panel-power-20261114/maia_sched_probe_ram.bin --log /tmp/maia-panel-power-hardware.log --panel-power --capture-seconds 15
python3 /tmp/maia-panel-power-checks.py
```

[Capture](native-panel-power-hardware.txt), [repeat](native-panel-power-repeat-hardware.txt),
[desktop](native-panel-power-desktop.txt), [manifest](native-panel-power-manifest.json).
Recovery `../maia-rtos-recovery/native-panel-power-20261114/` preserves exact final
source/bin/captures/checks, pinned references and original firmware hashes.

Next small task: audit and configure native DPI format/clock/timing with bounded
failure and inactive DMA, establishing the matching panel-format contract before
video/framebuffer/PSRAM. No native pixels/full desktop or POSIX implementation yet.
