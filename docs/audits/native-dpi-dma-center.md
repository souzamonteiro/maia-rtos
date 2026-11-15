# Native central32-row spatial DMA diagnostic — 2026-11-15

**HARDWARE-TESTED, LIMITED; central marker visually confirmed.** After one RAM
upload with fresh readiness, owner confirmed four colored segments stable and
aligned in the center. Initial ambiguous visual answer and clarification are
retained below; no intervening code change or repeat visual test. Previous [five-node test](native-dpi-dma-spatial.md)
confirmed four large stable bands; its eight-row edge strip was not identified.
This opt-in profile moves the incremental-source tile to native rows624–655 and
increases its thickness to32rows. Geometry720x1280, production rotation90,
panel commands/timings/clocks, DMA controller ownership and completion/cleanup
protocol remain as previously qualified. No FreeRTOS/SDK runtime imported.

## Layout and transfer

| Node | Native rows | Source | Bytes | Items64 |
|---|---|---|---:|---:|
| 0 | 0–319 | Fixed red at tile+0 | 460800 | 57600 |
| 1 | 320–623 | Fixed green at tile+360 | 437760 | 54720 |
| 2 | 624–655 | Increment over720x32 RGB565 tile | 46080 | 5760 |
| 3 | 656–959 | Fixed blue at tile+720 | 437760 | 54720 |
| 4 | 960–1279 | Fixed yellow at tile+1080, LAST/terminal0 | 460800 | 57600 |

Total1843200 bytes/23040064-bit items; five nodes within existing bridge count.
Each source row contains four180px red/green/blue/yellow bands. Marker centered
between green/blue, with greater thickness for observation; orientation on glass
is not assumed from native x/y. Large bands reuse four exact8-byte samples from
that same tile, using previously qualified ARLEN_EN1/field0. No complete physical
framebuffer allocated. Increment is on node2; old edge profile remains node0.

Separate NOLOAD reservation46848 bytes,128-aligned, wholly within tested
4FF00000..4FF28000 window. Source at reservation+128,46080 bytes;128-byte gap;
list at+46336, capacity384 bytes (320used/64unused), followed by128-byte guard.
BSS/cache line/stack bounds verified before CPU writes. ROM cache handoff covers
only this reservation; all CPU buffer accesses use NC alias. Memory preparation
checks every source byte/unused byte, then unchanged clocks/DSI/CSI/all-channel
snapshots and inactive DMA. During transfer, existing private ownership/recovery
and exact80-word writeback checks apply; source hash, unused64-byte list tail,
guards and unrelated snapshots must remain unchanged. No160-item list is built
or published in this profile. Legacy22272-byte reservation/profile untouched.

No new hardware register/address fact: primary sources and licenses are those
in [spatial audit](native-dpi-dma-spatial.md) and [SRAM list proof](native-vdma-list.md),
SDKf5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h),
[bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h),
[chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
§5.5.4.2 pp334–336 LLI writeback/completion, §5.5.7 pp337–338 drain,
register5.12 p357 ARLEN; NC/cache/ROM facts in [tile proof](native-dpi-dma-tile.md),
TRM§7.3.1 pp451–454. New profile/geometry/reservation is original MIT software.

## Host/build evidence and pending procedure

46/46 CTest PASS. Both legacy and central actual-driver fake-MMIO regressions
run, plus independent descriptor geometry/source-offset/width/burst/completion
checks. Central marker boundaries624/656, total1280rows/1843200bytes, enlarged
source bounds, overlap/alias/high-end refusal checked. Capture oracle independently
checks new buffer/hash/source span and all80 before/after descriptor words; rejects
legacy/central capture confusion, bad increment/count/guards/writeback/overlap.
Actual central driver ASan/UBSan PASS; four compiled runtime mutants rejected
(ignore done, ignore rearm, zero LLI rearm, exceed frame bound); central C++98
header PASS, invalid center-without-spatial/bridge build rejected.

Native GCC14.2 RV32IMAC/ILP32/runtime gate PASS, project+libgcc only. Final map:
BSS end/reservation start4FF12580, reservation end/stack bottom4FF1DC80,
stack top4FF21C80 <4FF28000; full primary stack16384bytes retained. RAM entry
4FF00100, load segment4FF00000/64944bytes. Prepared image:
`/tmp/maia-native-dpi-center-20261115/maia_sched_probe_ram.bin`, SHA256
`834ce950de16f5a39b6aca27bf0c7c136ee7d6f554621b00549bff1e16e02435`.

ONLY after fresh readiness:

```
python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-center-20261115/maia_sched_probe_ram.bin --log /tmp/maia-dpi-center-hardware.log --dpi-dma-center --capture-seconds 30
```

Configure DPI_DMA_CENTER+SPATIAL+BRIDGE and all bridge prerequisites; legacy
DPI_DMA_TILE/solid-stage/frame profiles excluded. Sequence400 finite frames, per
frame100000 completion budget plus existing separate recovery budgets. Elapsed
duration/visual marker are not established by compilation. No flash/SD writes;
runner resets to preserved installed firmware; verify shell/SciCalc14/desktop.

## Actual target result and clarified visual report

One RAM upload after “Pode iniciar agora; estou olhando”, unchanged prepared SHA.
Native buffer layout matches map: start4FF12580/end4FF1DC80/alias8FF12580,
source4FF12600/list4FF1DA80, sourceB400/listcapacity180/spanB700hex/cacheline40.
Every source/unused byte verified during preparation, guard/snapshot/inert1.
Source hash4B6A57C5 matches independent host construction and is retained after DMA.

Runner HOST PASS: stage18/frame9/attemptFFF,400 frames,399 rearms,414268 aggregate
polls,92160000 normalized64-bit items,6109902us sample (about6.1s; no WCET/frequency
inference). Last active1/done1/rawE0FF/status12, common/underrun/DMA and bridge FIFO/
DSI errors0, transfer/recovery results0/recovered1. All80 initial and completed LLI
words match independently reconstructed counts/control/status/terminal words.
Source/unused64-byte list tail/guards preserved. Output/global/channel/abort off,
MODE1/LP0/DPI19D0, GPIO/unrelated snapshots restored, duplicate BUSY. Four native
panel power reads9C still succeed. Reset then shell/SciCalc `2+3*4 = 14`/desktop
PASS, preserved original flash/ELF digests unchanged. No flash/SD/parser/panel
commands/timing/production-rotation changes.

Initial answer selected “A faixa central apareceu com falhas ou outro resultado”.
Description then was “Pedaços bem pequenos de faixas, uma de dada cor, no centro.”
After explaining that each of the four intended pieces is32x180pixels in native
coordinates, asked whether the pieces were stable/aligned in one thin central
strip. Owner confirmed “Sim, estáveis e alinhados no centro”. This accepts the
qualitative central segmented-marker appearance. It was clarification of the SAME
run, not a repaired defect or second visual test. Initial answer remains preserved.
Do not infer per-pixel correctness, production rotation/input or tear-free timing.

[Hardware capture](native-dpi-dma-center-hardware.txt),
[desktop](native-dpi-dma-center-desktop.txt),
[owner report](native-dpi-dma-center-observation.txt),
[manifest](native-dpi-dma-center-manifest.json). Recovery
`../maia-rtos-recovery/native-dpi-center-20261115/` includes tested source/build,
checks and pinned references; preparation and first-observation copies retained.

## Limits and next small task

No full framebuffer/PSRAM/native desktop acceptance; boot IRQs off/exclusive
diagnostic, not production scheduling or streaming. Existing WT no-ACK/underrun
threshold413/counter-doc limitations remain. Prior automatic-ARLEN source footprint
limitation in old solid API remains separate; spatial profiles explicitly control
source length. Next audit native PSRAM initialization/cache/DMA ownership and
budget for a full framebuffer, using pinned primary sources before register writes.
Installed complete desktop remains the original runtime; native diagnostics have
no foreign OS/runtime dependency.

