# Native finite five-block spatial DMA diagnostic — 2026-11-15

**HARDWARE-TESTED, LIMITED.** Native five-node DMA chain completed400 frames,
with all per-node writeback/count/guard/recovery checks passing. Owner confirmed
four large stable color bands; the narrow incremental-source strip was NOT
identified visually. No complete spatial-pixel/native desktop/framebuffer/PSRAM
acceptance is implied.

## Prerequisite evidence and primary sources

[Two-block SRAM qualification](native-vdma-list.md) demonstrated descriptor fetch,
source increment/fixed data, exact LLI writeback and explicit source AXI length
before this profile was built. [Bounded red refresh](native-dpi-dma-refresh.md)
confirmed solid red visually, but not spatial rendering. Prior160-LLI representation
was CPU memory-only and must not be activated through the four-bit bridge count.

Pinned SDK `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` primary sources:

- [Bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h),
  `set_multi_block_number`, `set_flow_controller`, `enable_dpi_output`,
  `update_dpi_config`: direct count5, multi-block-enable for count>1, DMAC flow,
  output/WT update. [Bridge registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_bridge_reg.h)
  flow+88 count7:4/controller bit0, interval+6C multi-block bit28.
- [DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
  and [registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h):
  CFG0 source/destination LIST3, LLP master1/64-byte addresses, memory master1/
  DSI master0,64-bit widths, source burst1/destination256, source increment/fixed,
  source ARLEN_EN1/field0, destination AWLEN_EN1/field15, no peripheral-status
  fetch or block interrupt. These explicit source controls are based on the
  actual SRAM byte proof, not a claim of universal burst encoding.
- [TRM chip1.3 Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
  §5.5.4.2 pp334–336: LLI fetch/LAST, writable status record, VALID clear, block/
  DMA completion flags published last; §5.5.7 pp337–338 disable/drain,
  register5.12 p357 ARLEN_EN/ARLEN. CPU channel status can be0 after qualified
  SRAM list completion; this path requires exact completion writeback from ALL
  five nodes, final DMA-done flag, natural disable, last raw count and empty FIFOs.
  Old contiguous-frame path still requires its original channel DMA-done flag.
- All unchanged panel/color/clock/timing/PHY/cache/ROM/GPIO facts and licenses:
  [frame audit](native-dpi-dma-frame.md), [SRAM tile](native-dpi-dma-tile.md),
  [SRAM transfer](native-vdma-transfer.md). Native destination50105000. No SDK
  headers/runtime or FreeRTOS/Arduino layer imported. New composition is MIT.

## Fixed geometry and ownership

Private `dsi_dma_spatial_probe.h` API is compiled only under the opt-in diagnostic
macro, not a production ABI. Boot/core0/IRQs off, controller exclusively owned;
source11520 bytes and writable list320 bytes disjoint128-byte-aligned physical
SRAM spans within4FF00000..4FF28000. Numeric checks are not allocation proof.
Preceding CPU-only tile fixture supplies actual reserved memory/cache handoff;
CPU uses NC alias only. Original160-item memory list is never activated: after
its memory-only checks, first320bytes are replaced by the separate five-node list.
Untouched tail hash and guard regions must remain unchanged.

| Node | Native rows | Source behavior | Bytes | Items64 | Raw final count |
|---|---|---|---:|---:|---:|
| 0 | 0–7 | Increment over720x8 tile; four180px RGB565 bands per row | 11520 | 1440 | 1439 |
| 1 | 8–319 | Fixed red sample at source+0 | 449280 | 56160 | 56159 |
| 2 | 320–639 | Fixed green sample at source+360 | 460800 | 57600 | 57599 |
| 3 | 640–959 | Fixed blue sample at source+720 | 460800 | 57600 | 57599 |
| 4 | 960–1279 | Fixed yellow sample at source+1080; LAST/terminal LLP0 | 460800 | 57600 | 57599 |

Total1843200 bytes,23040064-bit items/frame. Four large color bands plus a narrow
crosswise eight-row tile strip. This is a spatial diagnostic, not an allocated
full framebuffer. Native geometry720x1280; production software rotation90/logical
1280x720 unchanged. Exact user-view direction requires observation; do not silently
relabel source x/y as production horizontal/vertical.

Staging uses existing masked16-word guarded transaction, count5/multi-block-enable,
source ARLEN override, CFG0F and LLP head. No activation before stage succeeds.
Live guards account for legitimate fetched source/CTL/LLP changes but retain
root/supply/PHY/timing/config/error checks; source address must remain in owned
span, LLP within finite five-node set/terminal, widths/burst/status-fetch/IRQ bits
must remain exact. These sequential checks are not atomic or protection against
another producer; exclusivity is mandatory.

Each frame uses existing bounded activation/cleanup protocol. Success requires
observed channel active, natural disable, terminal rawE0FF, all80 exact expected
post-writeback words including raw counts and final DMA-done flag, drained DMA/
bridge, no recorded DSI/channel/common/underrun errors. Normalized items230400
reported only after this combined proof. Per-node expected writeback clears only
VALID and writes known LLP_STATUS fields; no broad mutation mask. DMA owns LLIs
while active; source remains immutable. Before each rearm, verify recovered idle
configuration/FIFOs and prior writeback, clear only completion/disabled4000001B,
require status0, rebuild/verify list through NC, restore exact channel setup while
idle, then consume private frame claim. Failure is sticky/no retry/reboot.

One bounded batch1..400, per-frame budget1..100000 and existing separate recovery
budgets; aggregate counters bounded. No unbounded loop/IRQ/heap/producer refill.
A completed batch cannot be called again or consumed by old standalone API.
Temporary backlight spans batch then saved GPIO state restored. Four panel power
reads follow success; runner resets to untouched installed firmware afterward.

## Prepared image and host checks

Native GCC14.2 RV32IMAC/ILP32 runtime gate PASS (project+libgcc only). Host44/44
CTest PASS, actual driver ASan/UBSan PASS, C++98 private header PASS, invalid
spatial-without-tile profile rejected. Four compiled runtime mutants rejected:
ignoring done writeback, ignoring incomplete rearm readback, zeroing LLI rearm,
and exceeding frame bound. Tests cover malformed/overlapping spans, malformed
initial list, multi-frame/max400/duplicate, corrupted completion writeback,
dropped rearm, timeout and underrun. Capture validator independently reconstructs
all five before/after descriptors and rejects wrong count/flags/errors/guards.

Prepared RAM image `/tmp/maia-native-dpi-spatial-20261115/maia_sched_probe_ram.bin`:
SHA256 `b2ae7a91e8f42a93283d39349cd639ca58df4b76db58a6f810b415f3e83a7648`,
entry4FF00100, load segment4FF00000/66272 bytes; RAM bounds verified. Procedure,
ONLY after fresh owner readiness:

```
python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-spatial-20261115/maia_sched_probe_ram.bin --log /tmp/maia-dpi-spatial-hardware.log --dpi-dma-spatial --capture-seconds 30
```

## Actual target run and limited visual acceptance

Owner confirmed readiness before upload: “Pode iniciar agora; estou olhando”.
The first CLI invocation requested45s and was rejected by argument validation
(limit1..30) before accessing serial. Retried immediately with30s, same image;
this was one actual RAM upload, not a failed target transaction. The ephemeral
desktop-check script had been removed from `/tmp`; its preserved recovery copy
was used successfully, with no diagnostic firmware change or second upload.

Final runner HOST PASS: stage18/frame-stage9/attemptFFF,400 frames,399 rearms,
414370 aggregate polls,92160000 normalized64-bit items,6109911us elapsed sample
(about6.1s, not WCET/frequency). Last frame active1/done1/rawE0FF/status12,
common/underrun/DMA FIFO/bridge FIFO/DSI errors0, transfer/recovery results0 and
recovered1. Source4FF12B80/list4FF15980, all80 initial/post-writeback words match
five-node oracle, including final LAST/terminal0/DMA-done status. Source hash
E18C2E45 and unused list tail retained, guards1. GPIO/output/global/channel/abort
restored/off, commandMODE1/LP0/DPI19D0, unrelated snapshots retained, duplicate
BUSY; four panel power9C replies still pass. Reset then shell/SciCalc14/desktop
PASS. Image SHA unchanged from preparation. No flash/SD/parser/timing/rotation
writes in this native unit.

Owner response: “Quatro faixas grandes estáveis; não identifiquei a faixa estreita”.
Record stable four-band visible output only. Descriptor proof and prior SRAM byte
copy validate transfer mechanics; they do not independently confirm every pixel
of the eight-row strip on glass. Do not count its absence from the observation
as a black-screen failure, or infer it was visible. Next small task: make the
incremental-source tile more visible, preferably at the center with greater
thickness within checked SRAM bounds, then obtain fresh readiness/visual report.
Full framebuffer/PSRAM/native desktop remain separate work.

[Hardware capture](native-dpi-dma-spatial-hardware.txt),
[desktop capture](native-dpi-dma-spatial-desktop.txt),
[user observation](native-dpi-dma-spatial-observation.txt),
[manifest](native-dpi-dma-spatial-manifest.json). Recovery
`../maia-rtos-recovery/native-dpi-spatial-20261115/` preserves tested images,
sources/build/checks/logs/pinned references and original firmware digests.

 Existing underrun discard threshold413 still limits raw0 meaning;
WT publication has no readable latch ACK. No hard WCET/refresh frequency claim.
Installed desktop remains original runtime. Full native graphics/PSRAM/production
scheduler/rotation-input acceptance remain separate work.
