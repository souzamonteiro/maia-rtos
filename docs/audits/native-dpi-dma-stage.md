# Native inactive single-block DSI DMA channel staging — 2026-11-15

**HARDWARE-TESTED, LIMITED: configuration/readback, no transfer.** New native API
stages channel0 and DSI flow controls under actual private bridge ownership, leaving
controller/channel/video/output disabled. Four panel power reads still succeed.
No DMA fetch, pixel transfer, bandwidth, bridge live-latch or visible-image proof.

## Source facts and change from the tile-list plan

Pinned ESP-IDF `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:

- [DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h),
  channel setters for flow, width, fixed addresses, burst size, HW handshake,
  priority/outstanding requests and contiguous transfer; [types](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/include/hal/dw_gdma_types.h).
- [Register types](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_struct.h),
  `dmac_chn_ctl0/1`, `cfg0/1`, LLP0/1 and block count. Channel0 base GDMA+100,
  SAR0/1+0/4, DAR0/1+8/C, BLOCK_TS+10, CTL0/1+18/1C, CFG0/1+20/24, LLP0/1+28/2C.
  Signal-enable maskF83F7FFB reuses [prior native DMA proof](native-vdma-transfer.md).
- [Bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h)
  `set_flow_controller/set_multi_block_number/set_empty_threshold/set_burst_len`,
  [bridge register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_bridge_reg.h):
  +88 controller bit0=0 means DMAC, count bits7:4 hold1; +6C multiblock28=0;
  +8C empty threshold10:0=768; +8 burst11:0=256 (ignored when DMAC is flow controller).
- [SDK DPI initializer](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c)
  uses DMAC flow and one block per frame. Four-bit bridge count cannot encode160
  as used by its direct-number setter. The prior160-LLI tile encoding was only
  constructed/read back, never connected or transferred. It remains unchanged and
  **must not be activated directly as this bridge's frame plan** without a separately
  verified flow/termination design. No claim that all possible160-block strategies
  are impossible; only the simple direct mapping is unsupported by this contract.

This initial transfer plan instead uses **one contiguous frame-sized block** with
an eight-byte fixed source (four equal RGB565 pixels in the fixture), a fixed DSI
FIFO destination50105000, no LLP/list and no full framebuffer. Source increments
are disabled and source burst-items=1; source ARLEN override disabled. Destination
burst-items=256, AWLEN override enabled with field15,64-bit width for both, memory
master1/DSI master0. One frame1,843,200bytes=230400 items, BLOCK_TS383FF.
CTL0=001C1B51, CTL1=000F8000, CFG0 contiguous0, CFG1=0A020001:
M2P/DMAC flow, HW destination handshake/DSI0, priority1, source/destination outstanding
limits5/2. Source handshake ignored for memory, set HW0. No status fetch/writeback,
interrupt generation request or valid/LAST/list markers. No optional peripheral
status-address registers are accessed. Data transfer with this combination remains
unproven; performance and possible underrun must be observed in the next gate.

An independent host-only SDK register-types/enums/map oracle checks these controls.
No SDK headers/runtime/heap/task/semaphore/control flow imported into MaiaRTOS;
new guards, RMW, reporting and fixture are original MIT. Prior licensed code keeps
its attribution. No parser/lexer change.

## Hardware refusals preserved and resolved without weakening comparison

1. First candidate read optional destination-status-fetch address50081148 and
   trapped: `cause=30000005 pc=4FF0BF52 value=50081148`. The access occurred while
   snapshotting configuration, before channel-configuration stores/enable. Runner
   reset to preserved flash. SDK register types describe RW, but that does not
   establish readable implementation on this device. All four optional source/
   destination status-fetch address registers were removed from the whitelist;
   fetch/writeback enables remain0. Exact cause of the decode/read fault is open.
2. With optional registers excluded, CTL1 write requested00108000 (AWLEN field16),
   readback was000F8000 (field15). Strict comparison refused to continue. A first
   enhanced-diagnostic rebuild failed on misleading indentation, and its subsequent
   RAM attempt repeated the preceding binary; that attempt is not new evidence.
   The build was corrected and a new diagnostic explicitly recorded the actual
   word before returning VERIFY. These captures are retained separately.
3. Final code requests **15 explicitly** and still requires exact full-word
   readback. This is within the field's described range in chip1.3
   [TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
   register5.12 pp357–358. No mask/error relaxation or forced expected-value adoption.
   Why16 reads15 is not inferred from this observation; no electrical AXI beat
   count or measured throughput claim. The prior SDK-derived LLI encoding with
   field16 is also not certified for real DMA execution.

The fault/refusals are useful board-specific constraints, not successful transfer
tests. No candidate enabled the DMA/controller/video/output. Original flash was
not rewritten. Images/source/logs of refused candidates are archived privately.

## API, ownership and safety

`maia_esp32p4_dsi_dma_solid_stage(source, report)` is boot/core0/IRQs-off/exclusive.
Caller owns at least8 source bytes in the tested physical SRAM window, aligned128,
completes cache handoff and retains the source until reboot/future documented
completion. Numeric range checks do not prove ownership. The fixture supplies the
existing owned NC-prepared tile's first8 bytes and checks all four pixels are red.
It does not modify the tile or160-LLI buffer; that list is left unpublished/unused.

Actual native bridge/stage/clock claims are required. Cached timing/root/supply/
PHY/LP controls, command mode, stopped/empty TX, bridge EN1/output0/VPG0/shadow0,
bridge IRQ enables0/no underrun, and whole DMA controller/channels/reset/abort
inactive are checked. Channel0/common status must0; no status-clear writes.
Sixteen captured config words are guarded around each checked masked RMW/store,
preserving RO/reserved/unrelated bits. Root guards precede peripheral readback if
clock access is lost. Source/destination high words and LLP are normalized within
writable masks. No global/channel enable, IRQ-enable, WT, cache, source-memory,
PLL/reset or output/video/HS write. Bridge flow fields are staged, not proven live.

Report stage1 guarded, each field advances to i+2, final18 verified; attemptmask
records actual stores only. Failed readback records the actual observed word.
Private source/config cache and ownership are retained on success; duplicate BUSY
without MMIO. Partial failure and observed preflight DMA/DSI errors are sticky
FAILED across transfers until reboot. No retry, rollback, teardown or activation
API. Preflight non-error refusal before writes may be rechecked. Successful stage
releases LP transport with the channel/controller still disabled.

## Tests and final target evidence

Host42/42 CTest PASS. Actual driver fixture covers16 dropped stores and16 clock-loss
points, masked/full-word readbacks, no-op profile, invalid source/context/ownership,
DMA/DSI error, active controller refusal, shared sticky/duplicate lifecycle and
post-stage reads. Existing20 DPI-stage dropped-store cases retained unchanged.
Explicit fixture exclusion of optional status-address accesses prevents regression.
Capture tests require prior tile/bridge chain, all16 words/masks/attempts, correct
fixed source/count/handshake/flow, inactive output/global/channel, unchanged memory/
other peripherals and four subsequent power replies. ASan+UBSan actual driver PASS,
C++98 public header PASS, seven compiled mutants rejected (source increment, SW
handshake, wrong count/destination/bridge flow, optional status read, ignored DMA
errors), invalid profile rejected, independent host SDK oracle PASS.
GCC14.2 RV32IMAC/ILP32/runtime gate PASS, project+libgcc/no foreign or unresolved
runtime. Existing RWX/removed empty flash-segment warnings remain.

One final successful Tab5 RAM load, chip1.3/40MHz/cu.usbmodem1301:

- Full prior native supply/reset/ST7121/power/format/clock/bridge/tile checks PASS.
- Stage18/attemptD975 (10 actual writes), source4FF10E80, frame1C2000,
  channel/common status0, PHY15BD/FIFO50015/errors0.
- SAR4FF10E80/DAR50105000/BLOCK383FF/CTL0=1C1B51/CTL1=F8000/CFG0=0/
  CFG1=A020001/LLP0/1=0; signal wordFFFFFFFF->07C08004 preserves non-RW bits.
- Bridge flow11->10, frame interval20002409 unchanged, threshold200->300,
  burst80->100. MODE1/LP0/DPI19D0/globalCFG0/channels0/abort0.
- Tile/list hashesE18C2E45/33AD7F48 unchanged, other clocks/DSI/CSI/channels preserved,
  duplicateBUSY. Four power9C words/stage10/headers3/polls6/one word/errors0/
  modeB0000 restored/mask5. Runner PASS, reset to preserved firmware.

Final image SHA256 `20fe0dcdba0dc61ee1fbaf34163fc039a45a6788d408ff021795696a11d19287`;
entry/load segments remain inside4FF00000..4FF28000. Shell/SciCalc14/desktop recovery
is recorded separately. No flash/SD, production panel table/timing/rotation or
parser/EBNF changes. Native diagnostic has no FreeRTOS runtime; installed full
desktop still uses the preserved FreeRTOS firmware. No full native graphics/POSIX.

[Final capture](native-dpi-dma-stage-hardware.txt), [optional-register fault](native-dpi-dma-stage-refusal.txt),
[burst readback](native-dpi-dma-stage-burst.txt), [desktop](native-dpi-dma-stage-desktop.txt),
[manifest](native-dpi-dma-stage-manifest.json). Recovery
`../maia-rtos-recovery/native-dpi-dma-stage-20261115/` retains final/refused images,
sources/logs, pinned references, oracle/check scripts and original firmware hashes.

```sh
cmake --build /tmp/maia-native-dpi-dma-stage-20261115 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dpi-dma-stage-20261115/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-dma-stage-20261115/maia_sched_probe_ram.bin --log /tmp/maia-dpi-dma-stage-supported-hardware.log --dpi-dma-stage --capture-seconds 15
python3 /tmp/maia-dpi-dma-stage-checks.py
```

Configure all DPI_DMA_TILE prerequisites plus `MAIA_NATIVE_PROBE_DPI_DMA_STAGE=ON`.
Next: bounded single-frame fixed-source transfer with checked activation, DMA
completion/error/early-stop recovery and temporary light, then visible-color proof.
Before that activation design must cover bridge update/underrun policy and keep
source immutable. No160-LLI activation; pixels, bandwidth, framebuffer/PSRAM,
production90-degree rotation/input mapping and cold PLL startup remain pending.
