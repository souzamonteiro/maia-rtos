# Native panel format readback and inactive DPI clock enable — 2026-11-14

**HARDWARE-TESTED, LIMITED.** Two final RAM loads read initial color00, select
COLMOD55, verify color05, enable the DPI gate223->2A3 and retain stable color05/
power9C responses. Mode remains command1, bridge/output/DMA and update latch off.
This establishes reported16-bit format and logical clock gating, not measured
pixel frequency, actual wire-format acceptance, pixels or native video output.

## Primary references and format policy

Pinned SDK f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[panel commands](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/include/esp_lcd_panel_commands.h)
RDD_COLMOD0C/COLMOD3A. One-byte fixed read uses the same HAL framing/guards as
[RDDPM](native-panel-power.md): MRPS137, DCS requestC06. No new arbitrary public
read command/length interface, no response-framing/exact-length claim. Only low
byte (word&FF/id[0]) is requested data; other word bytes must not be decoded.

Primary M5GFX revision28d47949bdc84f30881623476f555398e2194313
[Panel_DSI init_panel/setColorDepth/init_dpi](https://github.com/m5stack/M5GFX/blob/28d47949bdc84f30881623476f555398e2194313/src/lgfx/v1/platforms/esp32p4/Panel_DSI.inl),
[Panel_DSI default command-mode initialization](https://github.com/m5stack/M5GFX/blob/28d47949bdc84f30881623476f555398e2194313/src/lgfx/v1/platforms/esp32p4/Panel_DSI.hpp),
[ST7121 subclass](https://github.com/m5stack/M5GFX/blob/28d47949bdc84f30881623476f555398e2194313/src/lgfx/v1/platforms/esp32p4/Panel_ST7121.hpp):
RGB565 depth/output and explicit COLMOD55 before DPI activation. Protocol parameter/
selection facts consulted; no M5GFX/FreeRTOS runtime or implementation imported.
Existing M5Stack MIT license retained; reference source snapshots match pinned Git.
The original31-packet board table is unchanged; this private BSP diagnostic adds
explicit format selection after table/staging and before enabling the clock.

Supplier-hosted Sitronix ST7121P V1.0,2024/01 section6.2.8 p105 corroborates RDDCOLM
and DBI low3 codes5=16,6=18,7=24bits/pixel, with documented initial00. PDF command
headings contain typos; SDK0C definition and M5GFX3A55 usage are independent checks.
P suffix is not identified on this device: do not treat supplierP timing/type tables
as exact silicon proof. Its pixel datatype table lists RGB888, so DBI readback05
alone is not evidence for RGB565 video packets. Working BSP/M5GFX use RGB565;
physical native pixel/wire compatibility must be tested before claiming output.
No vendor register meaning beyond these corroborated facts is assumed.

Initial diagnostic strictly rejected00 before any COLMOD/clock write. That image/
source/capture is preserved. Since00 is a documented initial state and explicit
selection is supported by M5GFX, the final diagnostic permits initial0/5/6/7,
rejects reserved1..4, writes3A55 only if low3 !=5, waits bounded>=5ms as fixture
policy, then REQUIRES four stable low3=5 reads. Five-ms wait is not advertised as
an electrical minimum from a datasheet. It never accepts initial00 as16-bit proof
or ignores errors to pass. Already-selected5 skips the write (host/capture-tested
branch only). Full initial/returned bytes are observations, not hardcoded values.

## Native ownership and clock gate implementation

New fixed `read_color_mode` wrapper shares existing guarded one-word read helper,
MRPS1 and lifecycle with power/identity/short/long. Existing root/supply/PHY/FIFO,
error-mask/sticky/budget/restoration semantics unchanged; MRPS1 remains protocol
state after success. Successive color/power/identity requests remain usable.

The prior stage now retains a PRIVATE copy of its verified19 words, root/supply,
PHY controls and command LP mode. No caller report can grant clock ownership.
New boot/core0/IRQs-off/exclusive `dpi_clock_enable` requires that real stage claim
and exact cached snapshot, command mode, locked/stopped TX PHY, empty buffers,
bridge/output/pattern/frame ACK off. It requires inherited XTAL/SPLL logical enable;
no cold PLL calibration, source retuning or measured frequency claim.

Primary pinned SDK [clock tree](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/port/esp32p4/esp_clk_tree.c)
PLL_F240M source enable calls
[clk_gate_ll_ref_240m_clk_en](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/clk_gate_ll.h):
REF_CLK_CTRL1/HP+28 bit30. This shares the native20MHz divider word: preserve all
other bits. Then [DSI LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_ll.h)
DPI gatebit7 in HP+3C. Require already staged PLL240/div3 route, do not change it.
Only two possible checked masked-OR stores, skip matching provider; guard all19
words/root/supply/VDMA/PHY around each. No config update, video-mode/HS request,
bridge/output/DMA/IRQ/pixel writes. Fixed finite work, no unbounded wait/WCET claim.

Report stages1 guarded/2 provider/3 DPI gate/4 verified; attemptbits0 provider,
1 DPI gate. Success reserves clock, duplicate BUSY without I/O. Partial failure
sticks FAILED across stage/clock/transfers until reboot, no rollback/retry. The
cached root follows verified provider/gate changes only. Native LP short/long/read
preflights now accept a clock-on state ONLY if internally owned, route240/div3,
provider240 enabled and existing inactive-output guards hold. External gate enable,
route/provider loss or dropped gate is refused; this is not stream-mode commands
or a runtime clock manager. Other cached LP/timing changes require explicit design.

## Verification and actual evidence

Host40/40 CTest PASS. Actual stage/clock driver fixtures cover provider off/on,
missing stage/invalid context/null report, cached timing drift, external gate,
both dropped stores, error and bus-clock loss after provider write, sticky/duplicate
claims, post-enable identity reads and provider/route/gate loss. Shared read fixture
covers exact color command/MRPS1, raw low-byte view and sticky error. Capture tests
require full format/readback/clock/idle/post-response ordering and field masks;
wrong format/header/provider/gate/phase or omitted evidence fails, including skip
selection and initial00 positive branches.

ASan/UBSan actual clock and read fixtures PASS, C++98 header PASS. Six compiled
runtime mutants rejected: missing provider, wrong DPI gate, adopt foreign clock,
ignore staged config, wrong color command, wrong color MRPS. Invalid profile
rejected. GCC14.2 RV32IMAC/ILP32 build/runtime gate PASS, project+libgcc only/no
foreign/unresolved symbols. Existing linker RWX/empty removed-segment warnings.

Two final RAM loads, Tab5 chip1.3/40MHz/cu.usbmodem1301:

- Prior native gate/VO3/reset/ST7121 program/power transition/DPI staging PASS.
- Four color reads00, one COLMOD55 header00553A15, four color reads05.
- Clock stage4/mask2: provider58170503 alreadyenabled/unchanged, clock223->2A3,
  nominal80MHz. Only actual gate store in target; provider-off branch HOST-TESTED.
- MODE1/bridge0/DPI misc19D0/update0/DMAquiet1/preserved1/duplicateBUSY1.
- Four post-clock color05 reads and four power9C reads, all stage10/headers3/
  polls6/oneword/PHY15BD/FIFO50015/errors0; command modeB0000 restored/mask5.
- Both runner exits0/PASS; installed flash shell/SciCalc `2+3*4=14`/module exit0/
  desktop recovered. No flash/SD writes. COLMOD changes volatile panel state only.

This is logical gate/registered-format evidence, not physical80MHz measurement,
active timing latch, pattern generation, visible pixels, DMA/framebuffer/PSRAM,
full cold boot or native desktop equivalence. Default full desktop still FreeRTOS.

## Files, reproduction, recovery and next task

Changed SoC read/preflight/cache/clock API, new private BSP fixture, CMake option,
runner and actual driver/capture tests. Existing board table/timing unchanged.
Use full DPI_STAGE configuration plus `-DMAIA_NATIVE_PROBE_DPI_CLOCK=ON` in
`/tmp/maia-native-dpi-clock-20261114`.

```sh
cmake --build /tmp/maia-native-dpi-clock-20261114 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dpi-clock-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-clock-20261114/maia_sched_probe_ram.bin --log /tmp/maia-dpi-clock-format-hardware.log --dpi-clock --capture-seconds 15
python3 /tmp/maia-dpi-clock-checks.py
```

[Final capture](native-dpi-clock-hardware.txt), [repeat](native-dpi-clock-repeat-hardware.txt),
[initial refusal](native-dpi-clock-initial-color.txt), [desktop](native-dpi-clock-desktop.txt),
[manifest](native-dpi-clock-manifest.json). Recovery
`../maia-rtos-recovery/native-dpi-clock-20261114/` retains final/initial images,
source, actual logs/checks, qualified pinned references and original firmware hashes.
Next: guarded configuration-latch/output strategy and native test pattern without
DMA to establish actual pixels/wire compatibility before framebuffer/PSRAM. No
physical visual claim from register readback alone; no POSIX implementation yet.
