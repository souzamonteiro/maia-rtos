# Native Tab5 LCD/touch reset — 2026-11-11

**HARDWARE-TESTED, LIMITED.** Final target evidence and limitations are below.
Scope: ST7121/ST7123 reset on bank43 P4/P5 only. No supply, LDO, DSI, PSRAM,
DMA, backlight or default production-firmware migration in this unit.

## Sources and port provenance

- User Tab5.pdf update2026-11-05, schematic PDF p9 bank43 LCD_RST/P4,
  TP_RST/P5, p12 touch GPIO23, p16 “Tab5 Screen Reset”: LCD reset low as output,
  release as input/pull-up. [Primary M5Stack PDF](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/core/Tab5.pdf).
- Working source bf044d37e43c3b8db2011d6cbf81c9df2d2e1649,
  `platforms/tab5/components/m5stack_tab5/m5stack_tab5.c`, bsp_reset_tp:
  clear P4/P5 latch, assert P4, delay100ms, release P5 then P4/input,
  delay100ms. Full-chip expander reset/setup from its earlier init is not ported.
  Its 2024 Espressif copyright/Apache-2.0 notice is retained in the modified
  native port; full license in LICENSES/Apache-2.0.txt. Calls now use native
  checked/masked access and bounded waits, not SDK/FreeRTOS functions.
- Local M5Unified44952cb8e4ac88bf04ad23b9dfd6f4b05c788348,
  Power_Class.inl Tab5 configuration: LCD_RST input/pull-up. Local
  M5GFX28d47949bdc84f30881623476f555398e2194313,
  board_detect/m5/pmic_ops.hpp tab5_power_on uses10ms low/100ms stabilization
  and also changes other board rails/GPIO23. That complete power table is not
  transplanted into this reset-only port. Chosen100/100ms waits preserve the
  known working BSP. Existing M5Stack-MIT notice covers the expander port.
- GPIO/IO_MUX and 16MHz SYSTIMER facts use the primary references already
  recorded in [touch transport evidence](native-touch-identify.md). Test setup
  makes TP_INT/GPIO23 an input with output disabled before resetting ST712x;
  it does not implement GT911 address strapping. Diodes DS40583 Rev3-5 pp8–10
  config semantics are recorded in [expander port](native-expander-port.md).

## Contract and intermediate states

New `boards/m5stack/tab5/panel_reset.c` / public header operate through the native
PI4IO driver. Required report records the last completed verified stage and a
conservative may_have_changed flag, set before the first update attempt. Initial
snapshot errors or initially driven-high LCD_RST return before any update.
Caller must identify ST712x, stabilize existing supplies, make TP_INT an input,
stop display/DMA activity and exclusively own the bus/reset pins. This is
pre-scheduler/core0 only; GPIO backend requires IRQs already off. No ISR/task/SMP.

| Stage completed | Checked operation / resulting reset bits |
|---|---|
| 1 CHECKED | Snapshot five bank43 config registers; reject P4 output-high |
| 2 PULLUP | P4 pull select=up and pull enable=on |
| 3 LATCH_LOW | P4/P5 output latches=0 before enabling either output |
| 4 DRIVE_ENABLED | Clear P4/P5 high-Z, enable their outputs; both low |
| 5 HELD_LOW | 20,000 successful pauses of at least5us (>=100ms) |
| 6 TOUCH_RELEASED | P5 latch high, P4 latch stays low |
| 7 LCD_RELEASED | P4 direction=input, released by pull-up; P5 remains output |
| 8 SETTLED | Another20,000 successful >=5us pauses (>=100ms) |
| 9 VERIFIED | Read five full config bytes and compare against expected result |

All updates use fresh masked RMW/readback. Allowed changes: P4/P5 in direction,
latch and high-Z, P4 only in pull select/enable. Unrelated bits and bank44 are not
rewritten. P4 latch remains zero at completion, making future low assertion safe.
Any transport/verify/pause failure stops immediately; no compensating write,
implicit retry, rollback or claim that hardware remains unchanged. A write can
be applied before a reported failure. Partial reset may leave the panel held
low; caller must abort bring-up and use a known board-reinitialization path.
The report describes completed work, not certainty about a failed in-flight step.

The pause callback contract is reused rather than adding a CPU-frequency delay.
The two loops are bounded in iterations and each SoC pause bounds both capture
and timer-progress polling. No maximum wall-clock latency or hard-real-time
claim follows. Long waits are deliberate boot-only busy waits with IRQs off;
runtime services need scheduling/ownership contracts before reusing this logic.

## Validation and actual target result

- Host macOS C11/Clang build and **21/21 CTest PASS**. New state-machine fixture
  links the real native expander driver over register-I/O fault injection. It
  checks each intermediate write: no newly driven-high LCD, unmasked bits
  preserved; verifies64 safe initial configurations,62 bus-failure cases
  (before/after write application), six delay failures including boundaries,
  seven update-verification and five final-readback mismatches. No accesses
  occur after an injected failure; report stage matches the completed prefix.
  Invalid arguments and initially driven-high state reject without writes.
- Same fixture **ASan/UBSan PASS**; public header **C++98 PASS**. Six compiled
  mutants rejected: wrong LCD mask, shortened wait, LCD driven high, LCD left
  output, ignored update failure, ignored final mismatch. Existing transport,
  expander, scheduler and capture tests pass. Invalid profile combination rejects.
- Standalone strict GCC14.2.0 build, symbol/archive gate PASS (project objects +
  libgcc only). esptool4.12.0 preflight confines image/entry to L2MEM_LOW. Existing
  RWX and removed empty flash-segment warnings remain; no isolation claim.
- Actual Tab5 ESP32-P4 rev1.3 /40MHz XTAL `/dev/cu.usbmodem1301`: RAM probe first
  checks known touch firmware1, configures GPIO23 input with output disabled,
  snapshots both banks, performs the checked reset, then rereads both banks.
  Report: **stage9, may_have_changed1, preserved1, elapsed244643us, GPIO23 input1**.
  Elapsed is measured through native SYSTIMER, not an oscilloscope measurement.
- After reset, touch status is **0 (Normal)**;16 firmware reads and a final
  recovery read return1, errors0. Existing same-value expander, actual0x56 NACK
  and sampled-SCL-low timeout checks also PASS. The timeout is a software input
  injection, not an electrical short. Register readback does not measure reset
  voltages/pulse edges, LCD ID, pixels or DMA behavior. Warm-reset supplies were
  already configured by the preserved image; cold power sequencing is untested.
- Runner resets to preserved flash; maia.py confirms shell echo,
  SciCalc `2+3*4=14`, module exit0 and desktop active. No flash/SD writes.

[Hardware capture](native-panel-reset-hardware.txt),
[desktop verification](native-panel-reset-desktop.txt),
[manifest](native-panel-reset-manifest.json). Complete artifacts, source and
reference snapshots, licenses, host/sanitizer/mutant logs are preserved in
`../maia-rtos-recovery/native-panel-reset-20261111/`.

```sh
cmake -S . -B /tmp/maia-native-panel-reset-20261111 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_TOUCH=ON -DMAIA_NATIVE_PROBE_EXPANDER=ON -DMAIA_NATIVE_PROBE_PANEL_RESET=ON
cmake --build /tmp/maia-native-panel-reset-20261111 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-panel-reset-20261111/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-panel-reset-20261111/maia_sched_probe_ram.bin --log /tmp/maia-panel-reset-hardware.log --panel-reset
cmake -S . -B /tmp/maia-panel-reset-host-20261111
cmake --build /tmp/maia-panel-reset-host-20261111 -j4
ctest --test-dir /tmp/maia-panel-reset-host-20261111 --output-on-failure
```

## Status and next task

**HARDWARE-TESTED, LIMITED:** warm-boot native register reset sequence and touch
recovery. Failure behavior is host-tested, not electrical fault-injection proof.
No general power-init/GT911/SMP/runtime driver, native pixels, PSRAM/cache/DMA,
LDO calibration or replacement of the legacy desktop. The complete functional
firmware still depends on FreeRTOS; POSIX remains DESIGN.

Next: audit and port the MIPI PHY's LDO/power-control acquisition, including
silicon/revision calibration, ownership and failure reporting, before touching
DSI clocks/commands. Preserve the70MHz/965Mbps working configuration as reference;
do not reuse the old fixed-register LDO/native DMA path without evidence.

## Later qualification — 2026-11-12

[VO3 acquisition audit](native-ldo3-acquire.md) observes retained active MIPI
clocks after ROM RAM entry. This earlier reset probe did not measure its
inactive-display/DMA precondition. Its register transitions/touch recovery remain
observations, not proof of quiescent reset or cold display initialization. The
probe now rejects active guarded MIPI clocks before resetting. A new physical
run under proven quiesce is pending; the original captured evidence is unchanged.
