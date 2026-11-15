# Native warm HEX PSRAM XTAL clock qualification — 2026-11-15

**HARDWARE-TESTED, LIMITED.** Native code selects XTAL/core-div1/bus-div2,
reads four mode-register groups twice, restores the original clock profile and
checks all nine roots/27 controller words. Nominal20MHz follows the actual ROM
host report of40MHz XTAL and programmed divisors; no physical frequency or supply
voltage measurement. This is not cold initialization or usable array/framebuffer.

## Sources and implementation

[Hardware evidence](../hardware/EVIDENCE.md#psram-warm-xtal-clock-qualification-and-vo2-bit-correction--2026-11-15)
records primary pinned SDKf5c3654 sources/field sections: psram_ctrlr_ll clock
selection/core divider/gate and bus N/H/L encoding; hp_sys_clkrst_reg PERI+30;
spi_mem_s_reg SRAM_CLK+50 and spi1_mem_s_reg CLOCK+14; ldo_ll1900mV nonzero
VO2 calibration pair; PMU/eFuse fields. Host-only independent register oracle
checks addresses/masks/encodings. Native implementation is original MIT policy;
no SDK headers/runtime, PLL/ROM SPI helper, FreeRTOS or Arduino linkage.

`soc_psram_clock.c` adds boot/core0/IRQs-off/exclusive start/stop with private
claim and sticky failure state. Requires all code/data internal SRAM, no external
memory clients, both FSMs idle, AXI_REQ_EN0, verified warm MPLL/core-div1 and exact
bus-div4 profile30103 on both controllers. Supply must already be software-owned,
XPD1/select0/bypass0 with analog DREF/MUL matching nonzero eFuse1900mV calibration;
qualified rev1.3/schema/error checks. No voltage writes/adoption of arbitrary
profiles, power-cycle/reset, pad/DQS/DLL/device-mode or memory access.

Start: core gate off; sourceXTAL/core-div1 while gated; core gate on; each bus
divider2. Stop: both original bus-div4; core off; original source/divider while
gated; core on. Preserve unrelated RCC fields, including flash clock bits. Each
write requires fresh ownership checks and readback. Controller snapshots only
with core enabled; root checks while gated. Private expected roots/controller
state detects unexpected changes; no caller report acts as token. All loops
bounded, no sleeps/retries/automatic recovery. Any post-claim failure becomes
sticky until reboot; no uncertain rollback. Fully restored stop allows repeat.
Reports retain attempted stages/words. A gate readback is not an independently
measured physical clock-edge/settling acknowledgment.

Public API/header, isolated opt-in CLOCK+ID+STATE profile, native probe/runner,
host clock test/capture regressions and mode guard are changed. Existing complete
platform/default app/panel tables/rotation/parsers remain untouched.

### VO2 guard correction

Previous native identity guard incorrectly treated VO2 control bit7 as XPD.
Pinned PMU+1D0 actually defines software-owner7 and **XPD8**. Corrected guard and
negative owner-only80 fixture; old-bit7 runtime mutant is rejected. Earlier
actual control40200180 had both bits set, so observed identity reads remain valid;
this correction does not rewrite that old source contract. New clock profile
also refuses rail bypass/alternate select/missing calibration. No claim that
programmed calibration proves measured output voltage.

## Actual tests and hardware results

- Host49/49 CTest PASS. Clock/mode fixtures ASan+UBSan PASS, C++98 public header
  PASS, invalid CLOCK-without-ID profile rejected. Six compiled clock mutants
  rejected: ignoreAXI, wrongXPD, omitmux, omitrestore, retryfault, ignoreownership;
  additional old-mode-bit7 mutant rejected. Tests cover thirteen refused profiles,
  duplicate/no-claim no-I/O, ten changed-store drops, loss after every write,
  controller change while gated detected after enable, active command at stop,
  exact restoration and two cycles. No physical fault injection claimed.
- Native GCC14.2 RV32IMAC/ILP32 runtime-symbol/archive gate PASS (project+libgcc).
  Existing RWX/empty-flash-segment linker warnings retained; no isolation claim.
- First image `2d77851895748048dc4cbd7dd6255d54b25026a0260c8671266eaa2ef0eeffbd`
  produced successful native clock/read/restore records, but capture lost the
  beginning of PSRAM_STATE. Strict host result FAIL, preserved with exact image.
  No data reconstructed and no acceptance condition relaxed.
- Added bounded host-ready byteC before any PSRAM access in CLOCK probe only;
  runner sends after esptool returns. Timeout fails with no PSRAM programming.
  Capture tests rerun PASS after handshake and later stricter record-order check;
  actual final capture re-evaluated PASS, original partial capture remains FAIL.
- Intermediate synchronized image3a168acec8a6d9a055b64df83de9f87e50a6a9f067298bc0144616f1b001371f
  passed, followed by verified desktop recovery. Final review added a fresh root
  check before controller readback after a store, so injected clock loss cannot
  cause a gated readback. Host49/49 and sanitizer/mutant checks rerun PASS; final
  target rerun below. Intermediate image/sources/capture/desktop preserved in
  `../maia-rtos-recovery/native-psram-clock-intermediate-pass-20261115/`.
- Final RAM image `561588ba9a1301294c8c252f2c218224bea52f88fd1462ec1dc3daa5940114e2`,
  entry4FF00100/load4FF00000/13152bytes, one final upload. HOST_READY and full
  54-observation initial snapshot captured. Start result0/stage5/attempts1F;
  RCC PERI D00C->C00C, both clock30103->10001. Eight mode reads result0/stage5,
  setup+restore3FF, polls1/busy0/command0/restored1. MR0/1=8D30, MR2/3=A0DF,
  MR4=20, MR8=4B twice; same AP/32MiB/X16 device. No sampled busy-edge claim.
- Stop result0/stage5/attempts1F/restored1; originalD00C and both30103 restored.
  Final nine roots and27 controller words equal initial snapshot; strict runner
  PASS. Neither device mode nor PSRAM array touched. Exact intermediate hardware
  clock edges/undershoot not measured. This qualifies only the stated warm path.
- Reset then maia.py shell echo/SciCalc2+3*4=14/module exit0/desktop PASS. Original
  preserved bin/ELF hashes unchanged. No flash/SD writes, no visual test needed.

Build with toolchain `cmake/toolchains/riscv32-esp32p4-elf.cmake`, all three
`MAIA_NATIVE_PROBE_PSRAM_STATE/ID/CLOCK=ON`, target `maia_sched_probe_ram`, then
standalone esptool `elf2image`. Actual invocation:

```
python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-psram-clock-20261115/maia_sched_probe_ram.bin --log /tmp/maia-psram-clock-hardware-final.log --psram-clock --capture-seconds 10
```

[Final capture](native-psram-clock-hardware.txt), [first capture refusal](native-psram-clock-capture-refusal.txt),
[desktop](native-psram-clock-desktop.txt), [manifest](native-psram-clock-manifest.json).
Recovery `../maia-rtos-recovery/native-psram-clock-20261115/`; first image also in
`native-psram-clock-capture-refusal-20261115/`. Full sources/build/check scripts
and hashes preserved outside Git; tested native code matches final source copies.

## Remaining work

Own VO2 startup and device mode programming, qualify bounded read/write in a
reserved physical array range, then MMU/cache/capability and full framebuffer.
Warm inherited device modes/DQS/DLL remain dependencies.20MHz is a command
qualification profile, not established full-frame display bandwidth. Cold boot,
continuous LCD service/task/IRQ ownership and touch events remain open in the
[delivery plan](../plans/LCD_TOUCH_NATIVE.md). Installed complete desktop remains
original ESP-IDF/FreeRTOS firmware, not evidence of native completion.
