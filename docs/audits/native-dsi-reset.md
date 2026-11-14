# Native quiet panel reset followed by DSI responses — 2026-11-14

**HARDWARE-TESTED, LIMITED.** Integrate the existing native ST712x reset into the
opt-in DSI RAM diagnostic after proven MIPI parking/gating and VO3 acquisition,
before DSI access/reference/PHY setup. Two final RAM loads complete reset, recover
touch and pass four stable error-free RDDID response words each. No DCS startup
sequence, HS video, pixels, framebuffer, PSRAM or default firmware migration.

## Prerequisites and existing primary evidence

The unchanged [board reset port](native-panel-reset.md) documents the working BSP
bf044d3 bsp_reset_tp, Apache-2.0 attribution and MIT M5Stack configuration model.
Primary [Tab5 PDF](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/core/Tab5.pdf)
update2026-11-05: schematicp9 bank43 LCD_RST/P4 and TP_RST/P5; p12 TP_INT/GPIO23;
p16 reset release via input/pull-up. Pin/register/timebase facts remain those in
[native touch transport](native-touch-identify.md), reset100/100ms stays unchanged.
No new register address, pin, voltage, timing or undocumented reset sequence.

Actual panel-family selection follows the preserved project's
[bsp_detect_display_type](https://github.com/souzamonteiro/maia-rtos/blob/d0886c3/platforms/tab5/components/m5stack_tab5/m5stack_tab5.c):
firmware1 at I2C55 register0000 selects **ST7121**; firmware3 selects ST7123.
The observed device returns1 before and after reset. This is the working BSP's
selection policy, not independent die identification. Do not reverse this mapping
or infer a panel model from the DSI stripped response00FBA080. Subsequent startup
must audit the ST7121 driver table and its provenance, rather than blindly use the
ST7123-specific custom table. Firmware outside1/3 aborts before reset writes.

The old standalone reset capture predates proof of MIPI quiescence and was
qualified as such. This integrated diagnostic supplies the missing gate evidence;
it does not retroactively change that older capture or allow active-stream reset.

## Scope and ordering

New private `tests/hardware/native_dsi_reset.h` integrates existing drivers only.
No public driver/API/state-machine/parser/EBNF changes. New CMake option
MAIA_NATIVE_PROBE_DSI_RESET requires the full DSI_READ profile; separate old
TOUCH/EXPANDER/PANEL_RESET profiles remain mutually exclusive with MIPI. It links
native bitbang, GPIO transport, expander, touch identify and panel reset sources.

1. After native park/gate/VO3 succeeds, require exact saved RCC/ISP/reset/VDMA
   controls via existing vo3_cycle_guard. All MIPI guarded clocks off, all four
   reset bits held, DMA enabled channels/abort/FIFOs inactive. Check calibrated
   active VO3 against its eFuse plan and save the full supply snapshot.
2. Prepare native GPIO31/32 boot I2C and identify supported touch firmware. Set
   GPIO23 input/output-disabled using the existing verified mux/routing facts;
   read back complete written fields and output-disable state. No GT911 strap.
3. Snapshot five configuration bytes of both expander banks43/44. Recheck exact
   gate/supply/GPIO state immediately before calling the existing native reset.
4. Existing board driver asserts P4/P5 low, waits>=100ms, releases touch high and
   LCD as input/pull-up, waits>=100ms, verifies bank43. Report stage9/changed1.
5. Independently reread both banks: only allowed43 reset bits may differ and44
   stays identical. Require touch Normal/status0, same supported firmware, bus
   released, GPIO23 input, exact gate and full supply preserved before proceeding.
6. Then and only then open DSI and execute the existing native reference/PHY/
   command/short/long/read fixture. The runner requires reset evidence in this
   order between VO3 pass and DSI access, followed by all four valid responses.

Only RCC/still-clocked VDMA/PMU/GPIO/I2C accesses occur during reset. No reads of
DSI/CSI/bridge registers through gated clocks. CPU0/IRQs-off/exclusive boot only;
caller owns producers, bus and pins throughout. Guards are snapshots around the
sequence, not concurrent ownership monitoring or runtime clock/power management.
Boot-I2C enables its timebase, no timer interrupt or foreign runtime used.

Any failure halts before DSI open; a partial reset can leave panel held low. No
retry, rollback, blind reset release or compensating write. Runner resets to the
preserved installed firmware. Timing is measured with native SYSTIMER and has no
oscilloscope/voltage or maximum-latency guarantee. Supplies are warm/inherited;
this is not complete cold power sequencing.

## Verification

Host **38/38 CTest PASS**. New integration fixture executes the actual private
helper over fake boundary APIs: supported1/3 success, unsafe clock/DMA, unknown
variant, dropped GPIO stores, supply/root drift, partial reset, bank44 corruption,
failed touch recovery and GPIO output drift. It verifies refusal before reset and
no continuation after partial reset. It does not simulate electrical I2C or replace
the existing real board-driver fault tests; those continue to run separately.
Capture negatives reject incomplete/duplicate/out-of-order reset, active clocks,
wrong firmware, DMA/supply uncertainty, shortened wait and failed preservation.

ASan/UBSan PASS for integration and actual board-reset/expander fixtures. Four
compiled runtime mutants rejected: ignore quiet guard; permit unknown variant;
continue partial reset; ignore GPIO input check. Invalid profile rejected.
GCC14.2 RV32IMAC/ILP32 runtime gate PASS: project objects/libgcc only, no foreign
symbols/runtime/headers. Existing RWX/empty removed-segment warnings unchanged.

Two final image loads on Tab5 chip1.3/40MHz USB cu.usbmodem1301:

- Quiet sys0C0D4278/phy220/reset3C000040, DMA quiet1/supply ready1/firmware1.
- Reset result0/stage9/changed1, measured elapsed3BD55 and3B5FE microseconds
  (hex). Touch firmware1/status0, banks/clocks/supply preserved1, GPIO23 input1.
- All prior native DSI stages pass,16short/4long/followup pass, four RDDID words
  per load00FBA080, stage10/7polls/errors0, final PHY15BD/FIFO50015/restored5.
- Stable1/preserved1; both runner exits0. Desktop recovery verifies shell echo,
  SciCalc `2+3*4=14`, module exit0, desktop left active. No flash/SD writes.

One earlier integration load also passed before final GPIO rechecks were added;
its log is retained as intermediate evidence. The final two captures below are
from the manifest's final binary. Responses prove bounded reception after reset,
not exact RX length/datatype/model, visual configuration or working native pixels.
Default complete desktop remains FreeRTOS-based; no POSIX implementation.

## Reproduction, changed files and next gate

Changed CMake profile/host target, native MIPI probe include/call, new private
reset integration and its host fixture, runner/capture regressions and evidence.
Use the full read build configuration from [read audit](native-dsi-read.md), plus
`-DMAIA_NATIVE_PROBE_DSI_RESET=ON`, in `/tmp/maia-native-dsi-reset-20261114`.

```sh
cmake --build /tmp/maia-native-dsi-reset-20261114 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-reset-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-reset-20261114/maia_sched_probe_ram.bin --log /tmp/maia-dsi-reset-final-hardware.log --dsi-reset --capture-seconds 15
python3 /tmp/maia-dsi-reset-checks.py
```

[Final capture](native-dsi-reset-hardware.txt), [repeat](native-dsi-reset-repeat-hardware.txt),
[desktop](native-dsi-reset-desktop.txt), [manifest](native-dsi-reset-manifest.json).
Recovery `../maia-rtos-recovery/native-dsi-reset-20261114/` preserves final source,
binary, real logs, checks and pinned references/original firmware hashes.

Next: audit and port the selected ST7121 command table as a bounded native panel
startup sequence, with explicit delay/partial-failure policy and attribution.
Video/framebuffer/PSRAM follow separately; do not infer that reset plus RDDID is
full initialization or import the BSP's ESP-IDF runtime to advance that step.
