# Native finite solid-source DMA frame and command recovery — 2026-11-15

**HARDWARE-TESTED, LIMITED: one DMA frame completed and recovered.** Native code
observed channel0 active, natural disable/DMA-done, the calibrated full-frame count,
empty DMA/bridge FIFOs and restoration of output/command/LP/global/channel/light.
Four panel power reads still succeed. Visible red has not been independently
confirmed; this is not a framebuffer/desktop, sustained refresh or image-quality
acceptance. Source is one immutable8-byte red word, not the unused160-LLI list.

## Primary sources and sequence

Pinned ESP-IDF `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:

- [DPI initialization](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c),
  `dpi_panel_init`: DMA ready before video mode, automatic HS clock, DPI output
  enable/update. Native code imports these register-ordering facts, not ISR/heap/
  restart/semaphore/FreeRTOS runtime. No source/table/timing change.
- [Host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h)
  mode+34 bit0, LPCLK+94 bits1:0 AUTO3; [bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h)
  output+40 bit0 and WT update+44 (no ACK), FIFO depth+14, underrun raw+58.
- [DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h),
  global CFG0 bit0/IRQ bit1, CH0 enable strobes101/100, DMA-done status+188 bit1;
  channel error mask023F7FE0 as in [previous native proof](native-vdma-transfer.md).
  [Register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h)
  default status-generation enables at+180 and common+40 are checked, not written;
  CPU interrupt propagation remains off. Required channel mask errors|3, common
  mask001FFF8F. No status-clear, reset or abort write during frame transaction.
- [Chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
  section5.5.7 pp337–338 establishes disable/drain; §5.7.4 contiguous single block;
  register5.27 p375 describes STATUS0 as a transferred-item number. Actual raw
  counter behavior requires the qualification below; do not silently equate it
  to a normalized number of items.

New native transaction/guards/recovery and qualification code are original MIT.
No foreign OS/runtime/headers or vendor ISR/driver control flow imported.

## Counter qualification before accepting completion

Initial DMA frame signaled DMA-done, naturally disabled, FIFOs0/errors0 and recovered,
but STATUS0 was383FF while configured items38400. The initial strict count check
expected38400 and timed out; this refusal/image/capture is retained. It is not
relabeled as a passing frame.

A separate opt-in SRAM qualification profile uses the already verified NOLOAD/NC
reservation and M2M configuration. Four transfers independently verify every
requested destination byte, untouched zero tail, guards and inactive controller:

| Width | Programmed items | Verified bytes | Raw STATUS0 |
|---|---:|---:|---:|
| 8-bit | 8 | 8 | 7 |
| 64-bit | 8 | 64 | 7 |
| 8-bit | 256 | 256 | 255 |
| 64-bit | 4096 | 32768 | 4095 |

This qualifies **raw N−1 on this chip/conditions** independently of the display;
not an unmeasured addition to force a pass. It contradicts a literal count reading
of the cited TRM/SDK wording. Exact cause/documentation applicability remains open.
Do not generalize the convention to other chips/revisions without qualification.
The production API now exposes `count_raw` and `items` separately. It confirms
items only after observed activity, natural disable, DMA-done and raw equality
to the privately staged BLOCK_TS. For this frame raw383FF confirms38400 items,
1,843,200 bytes. A short/failed transfer cannot be accepted by adding1 blindly.

The qualification target reports status0 despite correct copied data. Its first
host validator unnecessarily required DMA-done and therefore printed HOST FAIL,
while target byte checks passed. That validator was corrected to require independent
byte/tail/guard proof, retaining the raw status as observation. Exact target rows
were reevaluated and passed; the original full capture, including HOST FAIL, is
retained without editing. This does **not** weaken the display transaction's
DMA-done requirement (actual final status12 includes DMA-done).

## API and bounded recovery contract

`maia_esp32p4_dsi_dma_solid_frame(budget,report)` is boot/core0/IRQs-off/exclusive;
it consumes the real private solid-stage claim. Source remains immutable, owned
and cache-prepared throughout. No list, heap, CPU IRQ, buffer writes or PLL changes.
Preflight checks complete idle staging, roots/supply, source binding and default
latched-event generation. Active guards check stable programmed RW fields, other
channels/roots/bridge ownership, but allow live channel0 FIFOs/progress. This
separates active-transfer guards from the old DMA-quiet predicate. Block progress
is read independently; completion can race sequential register observations, so
final status/count are refreshed after channel disable is seen.

Activation: globalCFG1 (IRQ bit1 stays0), channel101 with observed EN1, MODE0,
LPCLK3, DPI_EN1, literal WT1. No WT readback/RMW/retry or claimed update ACK.
Completion requires channel naturally off, exact calibrated count and DMA-done,
no documented channel/common/DSI/bridge latched errors, both FIFOs empty. The
bridge raw-underrun latch is observed at the existing discard threshold413
(DPI19D0); raw0 is not proof that every startup-line transient was detectable or
that all visible pixels were correct. No threshold masking change was introduced.

Budget1..100000 bounds completion/drain polls. Separate budget bounds DMA shutdown
and another bounds restored LP/PHY idle (recovery polls up to2*budget). No WCET or
hard-real-time guarantee; caller blocks with interrupts off by this boot contract.
Cleanup is attempted even after primary failure, provided root/ownership guards
still permit safe access: channel100, globalCFG0, bounded DMA FIFO idle, output
restore/WT1, command mode1/LP0, bounded stopped/empty PHY/bridge. It does not clear
errors or abort. Lost clocks/ownership can prevent cleanup; report says unrecovered
and requires reboot. Exact primary and recovery result codes are separate.

Successful cleanup proves register/idle recovery, not WT live-latch acknowledgment.
An already-off channel-disable strobe or a dropped off-update can be unobservable;
no fake ACK model is used. Report attempts12 writes(maskFFF), active/done/recovered,
raw/normalized count, flags and both poll totals. Success consumes the claim;
duplicate BUSY without MMIO, partial/error sticky FAILED/no retry. LP commands
are released only on complete success; otherwise reboot, even if cleanup succeeded.

## Validation and actual evidence

Host42/42 CTest PASS. Actual-driver fixture covers successful completion, a
completion/status race, reserved block-count bits, short natural completion refusal,
timeout with recovery, underrun with recovery, root loss without unsafe recovery,
missing event generation, failed writes and failed stop. Already-off CH strobe
and WT-off drops are explicitly excluded from guaranteed-detection claims; an
active-channel dropped stop is tested separately. Post-success LP read works.
Capture tests reject short counts/missing DMA-done/activity, errors, failed recovery,
active global/channel, missing post-reads and altered memory/phase order. Counter
qualification tests require byte-copy/tail/guard proof independently of raw values.

ASan+UBSan actual driver PASS, C++98 public header PASS, six compiled mutants rejected:
global IRQ enable, wrong channel strobe/count, ignored underrun, missing channel
stop and wrong restored mode. Invalid frame-without-stage profile rejected.
Native GCC14.2 RV32IMAC/ILP32 builds/runtime gates pass for both profiles,
project+libgcc/no foreign or unresolved runtime. Known RWX/removed empty-segment
warnings remain. Entry/load segments bounded inside4FF00000..4FF28000.

Final one-frame RAM load on Tab5 chip1.3/40MHz/cu.usbmodem1301:

- Full prior native supply/reset/ST7121/power/format/clock/bridge/tile/stage PASS.
- Temporary GPIO22 HIGH checked; active1, done1, count_raw383FF/items38400,
  channel_status12/common0/bridge_raw0/DMAfifo0/bridgefifo0/DSIerrors0.
- Stage9/attemptFFF,967 completion polls,2 recovery polls, both results0,
  recovered1; diagnostic elapsed13550us in this run (not a WCET/frequency claim).
- MODE1/LP0/DPI19D0/globalCFG0/channels0/abort0 and GPIO22 restored. Source/list
  hashesE18C2E45/3C9E517A unchanged; other clocks/DSI/CSI/channels preserved.
  Live PHY status is reported separately, not compared as an immutable snapshot.
- DuplicateBUSY and four post-transfer power9C replies/stage10/polls6/headers3/
  oneword/errors0/host controls restored. Runner PASS/reset to original flash.

Final image SHA256 `de826aeeb87f82a3a43d2e3e17bf4856b0d470525ffaebae56083924e2cc2e03`.
[Final capture](native-dpi-dma-frame-hardware.txt), [counter refusal](native-dpi-dma-frame-refusal.txt),
[SRAM qualification raw capture](native-dpi-dma-frame-count.txt),
[qualification reevaluation](native-dpi-dma-frame-count-validation.txt),
[desktop recovery](native-dpi-dma-frame-desktop.txt), [manifest](native-dpi-dma-frame-manifest.json).
Recovery `../maia-rtos-recovery/native-dpi-dma-frame-20261115/` retains final/refused/
qualification images, source, actual logs, checks, references and original firmware
hashes. No flash/SD, parser/EBNF, panel table/timing or production rotation changes.
Complete installed desktop is still the preserved FreeRTOS firmware; this native
transaction/qualification has no FreeRTOS runtime. Native full desktop/POSIX pending.

## Reproduction and next small task

Use prior DPI_DMA_STAGE prerequisites plus `MAIA_NATIVE_PROBE_DPI_DMA_FRAME=ON`.
Counter qualification is a separate VDMA_TRANSFER profile with VDMA_COUNT=ON,
MIPI=ON, VDMA_STOP=ON, MIPI_PARK=OFF.

```sh
cmake --build /tmp/maia-native-dpi-dma-frame-20261115 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dpi-dma-frame-20261115/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-dma-frame-20261115/maia_sched_probe_ram.bin --log /tmp/maia-dpi-dma-frame-final-hardware.log --dpi-dma-frame --capture-seconds 20
python3 /tmp/maia-dpi-dma-frame-checks.py
```

Visible red remains unconfirmed, and one frame is brief. Next qualify a bounded
refresh interval/visual color and underrun behavior before continuous output or
framebuffer/PSRAM integration. Do not enable the old160-LLI plan; single-block
fixed source is the verified path. Preserve native720x1280 versus logical1280x720
production90-degree rotation/input contract and prior horizontal-bar observation.
