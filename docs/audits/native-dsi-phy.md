# Native fixed960Mbps DSI PHY bring-up — 2026-11-13

**HARDWARE-TESTED, LIMITED.** One exclusive boot operation extends native
DSI configuration/reference setup with host/PHY power-up, fixed two-lane PLL
programming and bounded lock/stop-state observation. No video, panel commands,
framebuffer, DMA/PSRAM changes or native desktop completion claim.

## Primary source and port attribution

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[mipi_dsi_hal.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c):
`mipi_dsi_hal_init`, `mipi_dsi_hal_configure_phy_pll`,
`mipi_dsi_hal_phy_write_register`;
[PHY LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_phy_ll.h)
control/status/test-port definitions;
[PLL range table](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/mipi_dsi_periph.c)
950..999Mbps ->hsfreq1A. Pinned host register header offsets PHY_RSTZ+A0,
PHY_IF_CFG+A4, PHY_STATUS+B0, TST_CTRL0+B4, TST_CTRL1+B8, LPCLK_CTRL+94;
read-only TESTDOUT[15:8] must not be compared as writable control bits.

Working M5Stack-derived BSP `platforms/tab5/components/m5stack_tab5/m5stack_tab5.c`,
`bsp_display_new_with_handles_to_st7123` requests two lanes/965Mbps/default PHY
source for ST7123/ST7121. SDK integer algorithm with ref20MHz chooses even M48/N1,
realizing960Mbps (5Mbps below request); range1A still covers it. Native API makes
that realized nominal rate explicit, avoiding a misleading965MHz declaration.
Local BSP hash recorded with evidence, not represented as an unmodified upstream
revision. Reference20 remains modeled, not physically measured.

Adapted byte programming/sequence in `dsi_phy_program.h` preserves Espressif
copyright/Apache-2.0 and license `LICENSES/Apache-2.0.txt`. Modified for fixed exact
960Mbps, masked/readback MMIO, native guard/failure policy. Original native state/
guard/poll code stays in `soc_dsi_access.c`; no SDK headers/allocation/task/lock
code imported. No foreign runtime/API/shim linked. SDK unbounded `vTaskDelay`
lock/stop loops are replaced by caller-bounded native observations.

Existing [TRM chip1.3 Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
10.2.2 p616 and registers10.11/10.12/10.15/10.16 pp651–659 underpin unchanged
reference routing. Prior park/gate/VO3 guards remain intact. No invented timing,
PLL-ready flag or reference frequency measurement.

## API, sequence and limits

`maia_esp32p4_dsi_phy960(budget,report)` in existing DSI module/header requires
successful internal native reference claim (not a fabricated caller report).
Same chip1.3/core0/IRQs-off/exclusive pre-scheduler ownership and stable supply/
inherited SPLL assumptions. Initial RCC/access/reference pattern must match,
CSI/ISP resets held, DMA inactive, supported active calibrated VO3, host/PHY off,
bridge/DPI/FIFO0, no HS clock request and initial PLL lock low. No stale-lock adoption.
Full stable RCC/divider/provider/source/LP/eFuse/PMU/DSI controls checked freshly.

Native SDK-based order: set lanes low2bits=1; host power1; PHY shutdown-release1,
digital reset-release2, clock enable4, forcePLL8. Initial digital reset already
held, so redundant clear is skipped; release is checked. Preserve all other fields.
Program bytes44=34,19=30,17=00,18=0F,18=81: M48/N1/hsfreq1A. Each byte seven test-
interface writes: clock low/clear0, address+TESTEN, clock high/low, data/TESTEN0,
clock high/low. Address latch falling edge; data latch rising edge, as SDK.
Mask/verify only TST_CTRL0low2/TST_CTRL1mask100FF; retain unrelated bits, ignore
read-only TESTDOUT for readback. Control readbacks do not read hidden PLL registers.
Fresh full native guard before every setup step/byte and every poll. No heap/FPU.

Report stages0..11: none/guarded/lanes/host/analog/reset/clock/force/programmed/
lock/stop/verified. Writes conservative on failed byte (seven reserved attempts),
programmed counts completed bytes. Successful writes40 or41 depending whether
lanes already equal1. Duplicate success BUSY without I/O. Null/invalid budget
INVALID, context CONTEXT; incompatible initial state BUSY, initial unstable guard
VERIFY/write-free. Once guarded: sticky FAILED on any failure/timeout until reset,
no rollback/retry. Failure may leave analog power, PLL and test controls changed.
Runner resets preserved flash; this is not a runtime teardown/power manager.

Budget1..100000 bounds status observations. Lock bit0 then clock/data0/data1 stop
mask94 must be simultaneously set. Stop timeout after lock reports stage9;
lock timeout stage8. No undocumented microsecond delay; budget is not an elapsed-
time limit or analog settling measurement. PHY may drive LP idle levels; caller
owns attached lanes. No HS/packet request, DCS/DBI/video/DPI or panel configuration.
PLL lock/stop evidence does not prove link bitrate accuracy or functioning display.

## Validation procedure

Host AppleClang21/Python3.14.5; native GCC14.2 RV32IMAC/ILP32. New host fixture
executes real access/reference/calibration and models seven-edge test protocol,
programmed address/data, read-only TESTDOUT bits, delayed lock/stop, bounded
negative waits, preconditions/fresh guard drift and duplicate/sticky semantics.
41 dropped-store positions:37 significant failures stop immediately;4 repeated
clock-low stores are harmless no-ops (positions14/21/28/35). Tests distinguish
that behavior instead of claiming every dropped identical store is observable.

```sh
cmake -S . -B /tmp/maia-dsi-phy-host-20261113 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-dsi-phy-host-20261113 -j4
ctest --test-dir /tmp/maia-dsi-phy-host-20261113 --output-on-failure
cmake -S . -B /tmp/maia-native-dsi-phy-20261113 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON -DMAIA_NATIVE_PROBE_VO3_CYCLE=ON -DMAIA_NATIVE_PROBE_DSI_ACCESS=ON -DMAIA_NATIVE_PROBE_DSI_REFERENCE=ON -DMAIA_NATIVE_PROBE_DSI_PHY=ON
cmake --build /tmp/maia-native-dsi-phy-20261113 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-phy-20261113/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-phy-20261113/maia_sched_probe_ram.bin --log /tmp/maia-dsi-phy-hardware.log --dsi-phy --capture-seconds 15
```

Opt-in profile requires reference/access/VO3/park/gate. Diagnostic uses100000 poll
budget, then samples16 locked/stopped/controls/supply states, duplicate BUSY,
resets in runner finally and paced maia.py checks original shell/SciCalc14/desktop.
No flash/SD writes. On failure diagnostic reports stage/count/status and stops,
no compensating hardware writes. Capture requires all predecessor profiles,
exact programmed count/nominal rate/lanes, bounded polls/lock+all stops and final
preserved samples. Production/default firmware remains untouched.

## Actual hardware validation — 2026-11-13

[Hardware capture](native-dsi-phy-hardware.txt),
[desktop recovery](native-dsi-phy-desktop.txt),
[manifest](native-dsi-phy-manifest.json). Tab5 ESP32-P4 rev1.3/40MHz; native RAM
image confined to L2_LOW. Tracked captures normalize CR/trailing whitespace; raw
logs retained in recovery. No failed hardware candidate in this unit.

All native park/gate/VO3/access/reference predecessor profiles pass. Initial PHY
status1528/POWER0/PHY_RSTZ0, native initial guard rejects preexisting lock. Actual
PHY stage11, five programmed bytes, **40 stores** (lanes already configured2):

- PHY_IF_CFG00000001 ->00000001 (skip same lane-count write).
- Host POWER00000001, PHY_RSTZ0000000F after native ordered initialization.
- PLL/status000015BD: lock0-bit set, clock/data0/data1 stop mask94 set.
- **Seven bounded observations**, not seven milliseconds or a measured timing bound.
- Model960Mbps from nominal ref20/M48/N1; no physical frequency measurement.
- Sixteen later observations remain locked/stopped. Host/PHY/lane words, clock/
  reset/supply/eFuse guards stable; duplicate BUSY, preserved1/samples16.
- DSI bridge/DPI output/FIFO stay off/empty, no HS clock/packet request. No panel
  commands, video/DMA/PSRAM writes. Analog PHY is now powered and may drive LP idle;
  this is stronger evidence than prior reference-only register setup.

Prior VO3 host observation windows30.015/20.759ms remain observation-only, not
rail discharge/settling proof. Hidden PHY-register readback not available in this
API: actual control sequence plus observed lock/stop validates this workload, not
all rates, tolerance, signal integrity, panel initialization or cold SPLL startup.
No sleep/task/FreeRTOS/ESP-IDF runtime imported; standalone vendor tools only.

Host33/33 PASS; ASan/UBSan fixture PASS; strict C++98 header PASS. Four compiled
mutants reject missing stop check, wrong test-clock edge, reordered power/lanes,
and missing sticky PHY failure. Invalid PHY-without-reference CMake rejects.
Capture tests reject missing program bytes, wrong lanes/rate, unlocked/partial-
stop status, unbounded/zero polls, duplicate or malformed output. Native runtime
gate PASS: project objects+libgcc, no unresolved/foreign symbols. Existing linker
RWX/removed-empty-flash-segment warnings remain, no new project warnings.

Paced maia.py verifies shell echo, SciCalc `2+3*4=14`, module exit0, desktop left
active. Installed desktop remains preserved FreeRTOS baseline, not this native
firmware. No flash/SD writes. Source/build/raw logs, mutants/sanitizer commands,
per-file pinned SDK refs, port license and working BSP hash preserved under
`../maia-rtos-recovery/native-dsi-phy-20261113/`.

Implemented: fixed native two-lane960Mbps PLL/PHY initialization, checked test-port
port, finite lock/stop wait, host/device regression and failure evidence. Still not
implemented: DSI command-mode clocks/timing/FIFO service, panel-reset integration/
DCS startup, video/PSRAM/framebuffer/full native desktop, cold root SPLL startup,
physical bitrate/settling/eye-diagram measurement, runtime teardown and ownership.
Next small task: configure/check native command mode and escape/timeout clocks,
then bounded LP command transfers, before panel startup and image streaming.
