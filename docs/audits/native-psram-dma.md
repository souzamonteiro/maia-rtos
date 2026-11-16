# Native PSRAM source DMA: all nine burst/address cases pass — 2026-11-15

**HARDWARE-TESTED, LIMITED.** Extends the native mapped-memory diagnostic with
actual DW-GDMA channel0 reads from the synchronized2MiB PSRAM window into a
private4352byte aligned SRAM reservation (4096data+128byte guard each side).
Uses ordinary mappedPSRAM address and only qualified internalSRAM NC alias.
No display, PLL, power, mode, DMA IRQ or other-channel programming. Source facts
and independent SDK oracle: [evidence](../hardware/EVIDENCE.md#psram-dma-source-qualification--2026-11-15).
Existing licensed PSRAM setup remains native; DMA diagnostic/plan originalMIT.

Grouped9 cases: source offsets0,65408(crosses64KiB page),1FF000(last4KiB ofwindow),
each with source MSIZE1/4/16 items and explicit ARLEN_EN1/fields0/3/15.64bit,
increment source/destination, memory master1 both sides, softwareM2M, one block,
no descriptors. This deliberately avoids the earlier automaticARLEN SRAM defect.
Source is immutable during each copy; window sync precedes first transfer. Cache
WB/invalidate owns only newSRAM region; CPU accesses it through existingNC alias.
Private boot exclusivity/all clients stopped required; this is not a general DMA
allocation/service API or a concurrency proof. Native loops bounded; ROM cache
helper WCET/bus-stall bound remains unestablished.

Actual ONE target RAM upload, SHA
686f26dead6cee2033c6cec32250670e27d837a86e2fd273275ee89acd6d84dd,
load4FF00000/27152bytes/entry4FF00100. All9x4096bytes matched independent full-window
word pattern, total36864bytes; allguards intact, natural channel disable and
raw511=512items-minus1 each, errors0/common0. SoftwareM2M status0 accepted because
actual byte proof/natural stop establish completion; no display contract relaxed.
4KiB sampled times:1-item1179us,4-item373us,16-item170–171us. These include timer/
software overhead and bounded polling; they are not sustained display throughput,
latency guarantees or speed qualification at another PSRAM clock.16-item roughly
24MB/s here, still below the existing finiteDPI consumption (~120MB/s). Operational
clock and directPSRAM-to-DSI path remain to qualify before full-frame image.

Native helper checks warm MIPI/DMA roots/reset/idle/FIFO, stages12 channel words
with readback, bounds completion/stop/drain, verifies bytes/guards and restores12
config words only when safely idle. Progress/status/intentional W1C clears are
not reversible. Root loss/uncertain stop exits without releasing window/buffer;
probe halts and host resets, no blind retry. Final controller/channels/FIFO0,
config restored1 and DSI/CSI/otherchannel state preserved1. Original full2MiB CPU
and4KiB raw/cache/mode/clock recovery tests rerunPASS; strictnew runnerPASS.

Host52/52PASS. Plan bounds/no-output-on-refusal and independent pinned SDK
width/MSIZE/ARLEN oracle ASan/UBSanPASS; C++98/invalidisolatedprofilePASS. Three
compiled mutants rejected (automaticARLEN,wrongburst,badsourcebound). Capture
regressions reject CPU-only evidence, wrongbytecount/guards/burst/offset/raw/
error/poll/restoration and duplicates. Post-target additions affect only host
registration/fixtures, not native source/image; manifest records delta. Native
GCC14.2 dependency gatePASS (project+libgcc only). Initial default all-target build
hit the known legacymaia_m0 missingriscv_asm.hpp; explicitly selecting the isolated
maia_sched_probe_ram target passed. Initial runner attempts with missingbin
failed before opening serial; one actual upload only. Build/logs retained.

[UART](native-psram-dma-hardware.txt), [desktop](native-psram-dma-desktop.txt),
[manifest](native-psram-dma-manifest.json). Recovery
`../maia-rtos-recovery/native-psram-dma-20261115/`. Reset shell/SciCalc2+3*4=14/
exit0/desktopPASS; no flash/SD/parser/rotation changes. Installed full firmware
still originalESP-IDF/FreeRTOS fallback, independent native image imports none.
Next adapt bounded analogMPLL divisor read and checked operational PSRAM clock,
qualify data/bursts there, then fullframePSRAM->DSI visual delivery. No PLL
programming is implemented by this unit. Coldstartup/fullcapacity/continuousLCD/
touch/complete desktop migration remain open; avoid per-substep approval stops.
