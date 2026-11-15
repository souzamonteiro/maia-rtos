# Inactive native DPI format/clock-route/timing staging — 2026-11-14

**HARDWARE-TESTED, LIMITED.** Two final RAM loads stage/check19 host/bridge words
for the working ST7121 timing and RGB565, while DPI clock, bridge/output, video,
pattern generator and DMA remain inactive. Four subsequent power reads per load
still return9C/error-free with restored host controls. No active clock, latch,
pixel output, framebuffer/PSRAM or native desktop completion claim.

## Primary references and chosen contract

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[clock enums](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/clk_tree_defs.h)
default DPI sourcePLL_F240M;
[HAL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c)
calculate_divider uses integer source_MHz/request_MHz; horizontal timing converts
individual pixel durations to lane-byte cycles, then sums converted durations.
[Bus LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_ll.h)
source1=PLL240, divider register=div-1, DPI gatebit7 in HP peri_clk_ctrl03/+3C.
[Host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
[bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h)
and matching register headers define field masks/offsets and encoding below.
[SDK DPI setup](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c)
provides the parameter translation reference only. Apache-2.0 attribution retained
in new dsi_dpi_stage.h; no SDK types/headers/runtime/allocation/DMA code imported.

[Working BSP](https://github.com/souzamonteiro/maia-rtos/blob/c046903/platforms/tab5/components/m5stack_tab5/m5stack_tab5.c)
ST7121 variant uses720x1280, Hsync/back/front2/40/40, Vsync/back/front20/24/200,
RGB565 and requested70MHz. With source240MHz the pinned divider is3 and resulting
**modeled80MHz**, not70. The PHY's native960Mbps gives120MHz lane-byte clock;
conversion ratio120/80=3/2. Source/divider is staged only, gate stays off, frequency
not measured. Do not advertise actual running pixel clock or refresh rate.

Geometry stays in the BSP diagnostic, passed to a SoC timing structure; no board
resolution/pin data enters generic kernel code. SoC policy is fixedRGB565/VC0,
PLL240/div3/PHY960/two lanes. Clock source/provider startup and panel-format
readback/compatibility remain separate pre-activation gates. RGB565 follows the
working host/bridge reference; lcd_dev_config.bits_per_pixel24 alone does not
establish the transport format (SDK DPI config selects16-bit memory/output here).
No format command/register in the panel is written or inferred in this unit.

## Register plan and safety boundary

Stage route with masked FF60->220 in HP PHY clock word, preserving DPIgate7=0 and
all CSI/reference/config fields. On this target it already reads223 and needs no
write; nonmatching route programming is HOST-TESTED only. Do not enable shared
provider/PLL or any clock as a side effect. Nominal report80MHz is a model only.

Host offsets: VCID0C=0, COLOR10=0(RGB565 config1/loosely18off), POL14=0;
VID38 low fieldsBF02 (burst2, LP timing/commands), packet3C=720/chunks40=0/null44=0;
HSA48=3/HBP4C=60/HLINE50=1203; VSA54=20/VBP58=24/VFP5C=200/VACTIVE60=1280.
Bridge: PIXEL18 rawtype2/RGB/config0; V_CFG0/30=050005F4 (1280,total1524),
V_CFG1/34=00140018, H_CFG0/38=02D00322 (720,total802), H_CFG1/3C=00020028.
All writes preserve unmasked fields and verify full readback.

Frame BTA ACKbit14 stays0 and patternEN16 must already0; ACK policy is deliberately
deferred rather than importing SDK active-video frame ACK behavior. MODE remains
command1, LPCLK/HS request unchanged, bridge EN and DPI_EN remain0. No CONFIG_UPDATE
latch write, raw transfer count, flow/burst/credit/DMA/IRQ setup, framebuffer or
panel command is issued. Bridge misc/underrun defaults remain unchanged. Registers
are prepared/read back; the internal live timing latch has not been activated.

New native stage API requires internal command claim and exclusive CPU0/IRQs-off
boot, stable calibrated supply/root/VDMA/reference/PHY/config state, empty front+
internal FIFOs, TX/lock/all-stop, bridge/output/clock off and TE/ACK/BTA off.
Validate dimensions before I/O: nonzero active/sync/front, each field within both
host/bridge limits, bounded sums/products, strict positive porches. Use exact
integer3/2 conversions matching individual SDK truncations, no FP runtime.

Before every operation check complete command/root/supply/VDMA/PHY and saved19
DPI words. Recheck clock access BEFORE peripheral readback after a write; host
injection exposed and verified that safety point. Fixed finite steps, no wait
loops or hard-latency claim. Report stage1 guard/2 route/3..21 words/22 verified;
attempt maskbit0 route andbits1..19 words. A matching word skips its store.
Failure after guarded claim poisons stage AND all transfers until SoC reboot;
no rollback/retry/clock repair. Success reserves stage; duplicate BUSY without I/O,
while normal bounded panel reads remain usable. No concurrent runtime ownership.

## Verification and actual target result

Host40/40 CTest PASS. New fixture executes real native init+stage APIs, exact
80MHz/RGB565/host+bridge timing, all20 dropped-store failures, clock drift/error,
active clock/output/FIFO/pattern refusal, invalid dimensions/context, duplicate,
sticky transfer refusal, fully matching no-op profile and post-stage read reuse.
Capture negatives reject wrong timing/color/clock/nominal report, active output/
DMA/latch, missing preservation/duplicate evidence and malformed stage data.

ASan+UBSan actual driver fixture PASS, C++98 header PASS. Five compiled runtime
mutants rejected: wrong divisor, enable DPI clock, wrong pixel format, wrong line
conversion and ignored readback. Invalid native profile rejected. GCC14.2
RV32IMAC/ILP32 runtime gate PASS (project+libgcc; no foreign/unresolved symbols).
Existing RWX/empty removed-segment warnings unchanged.

Two final Tab5 chip1.3/40MHz RAM loads:

- Stage result0/stage22/attemptmaskFFF30 =14 actual config stores. Route223->223
  retained, modeled80MHz. All19 field comparisons/unrelated bits pass.
- Final MODE1/bridge0/DPI misc19D0, clock223 with DPI gateoff, update0, DMAquiet1,
  preserved1/duplicateBUSY1. Full earlier native reset/program/power checks pass.
- Four subsequent power words9C each, stage10/6polls/errors0/PHY15BD/FIFO50015,
  modeB0000 restored/mask5. Runner exits0/PASS twice.
- Preserved flash shell/SciCalc `2+3*4=14`/module exit0/desktop recovered.
  No flash/SD writes or default production/parser changes.

This closes only inactive register staging for this profile. Other geometries,
physical timing/frequency, provider startup, panel-format verification, update
latching, DPI output/HS/video, memory/cache/DMA/framebuffer/PSRAM/cold power and
full native desktop remain unproven. Working installed desktop still FreeRTOS.

## Files, reproduction, recovery and next task

Changed public SoC timing/report API, private stage implementation, existing claim
reset hook, native probe/CMake profile and host/capture fixtures. Timing parameters
reside in tests/hardware/native_dpi_stage.h for this BSP fixture, not generic kernel.
Full PANEL_POWER configuration plus `-DMAIA_NATIVE_PROBE_DPI_STAGE=ON`, build
`/tmp/maia-native-dpi-stage-20261114`.

```sh
cmake --build /tmp/maia-native-dpi-stage-20261114 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dpi-stage-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-stage-20261114/maia_sched_probe_ram.bin --log /tmp/maia-dpi-stage-hardware.log --dpi-stage --capture-seconds 15
python3 /tmp/maia-dpi-stage-checks.py
```

[Capture](native-dpi-stage-hardware.txt), [repeat](native-dpi-stage-repeat-hardware.txt),
[desktop](native-dpi-stage-desktop.txt), [manifest](native-dpi-stage-manifest.json).
Recovery `../maia-rtos-recovery/native-dpi-stage-20261114/` preserves tested source,
image, actual logs/checks, pinned SDK/BSP facts and original firmware hashes.
Next: verify panel-format contract and native DPI source/provider/clock enable
under inactive-output guards, then latch/output strategy and framebuffer/PSRAM.
No pixels, full native desktop or POSIX implementation claimed.
