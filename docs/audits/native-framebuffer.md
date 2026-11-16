# Full native PSRAM framebuffer on Tab5 — 2026-11-16

**HARDWARE-TESTED, LIMITED; OWNER CONFIRMED COMPLETE/STABLE/CORRECT ORIENTATION.**
One complete1843200byte RGB565 picture now transfers from native200MHz PSRAM to
DSI in bounded finite batches. This is an independent boot/core0/IRQs-off diagnostic,
not a continuously scheduled LCD service or native desktop replacement. Installed
complete firmware remains preserved original ESP-IDF/FreeRTOS fallback. No foreign
OS/runtime/header added to native image; project+libgcc dependency gatePASS.

## Implementation and lifetime

Reuses native ST7121 reset/program/power/DSI/DPI80MHz/bridge path and existing
DMA single-block completion/recovery, with a separate private framebuffer API.
Source4B800080..4B9C2080 in the owned2MiB window, prefix128bytes/all unused tailA5.
Immutable source720x1280 RGB565,23040064bititems, master1 PSRAM->master0 fixed
DSI FIFO50105000; sourceMSIZE16/explicitARLEN15, destination256items/AWLEN15,
CTL0=1CDB41/CTL1=F87C0. No descriptors or speculative160-LLI bridge configuration.
Legacy solid-source API keeps original SRAM-only contract.

New private memory claim syncs owned cache range before DMA and freezes window
sync/stop while the source is engine-owned. DMA/frame guards include actual PSRAM
root/pad/controller ownership and bounded incrementingSAR inside the exact frame.
Repeated frames verify prior completion/drain, clear only qualified status bits,
rewindSAR only while idle, then reuse existing start/done/rawN-1/stop/LP recovery.
Source release checks actual clock/disabled/drained DMA and unchanged MMU table/
memory guards. Lost ownership/uncertain recovery keeps source owned/faulted until
reset; no unsafe rollback or report-as-token. Caller exclusivity/immutability is
still required: no task/ISR/SMP allocator or concurrent cache-client capability.

PSRAM setup adapts the already qualified native clock/DQS/DLL path. Cache boot
uses immutable chipROM APIs, all code/buffers internalSRAM/ROM, no SDK runtime.
Ordinary full picture CPU write/read/hash plus nine actual4KiB sourceDMA copies
(current boot, different offsets/bursts including pagecross) precede display.
No ROM cache wait/bus-stall WCET established; native poll counts are bounded.

Pattern uses production software90 mapping from vendored LVGL9.2.2: logical(x,y)
->native(y,1279-x). Four160x128 numbered corners:1redTL/2greenTR/3blueBL/4yellowBR,
white grid/border, central six-color rectangle and varying background. Whole
frameFNV E3D71BEF, not a repeated tiny SRAM tile. No panelMADCTL/rotation/timing
change. [Primary facts/oracle](../hardware/EVIDENCE.md#full-native-psram-framebuffer-transfer--2026-11-16).

## Actual electronic and visual results

Same native image for BOTH uploads, SHA
68a8dfc0f8d9c9cb8bd3c2cdad7635a2d2c610bb703c07d129bc65c587e97d8f,
79488bytes at4FF00000/entry4FF00100. Full source/native dependency snapshots and
baselinecommit c54261e preserved in recovery. Only host-side capture checking/
registration changed between uploads, no target-image change.

ElectronicE:4frames/921600items/3rearms/1236polls/65383us. NativePASS, natural
stop/raw383FF/DMAdone/status12, common/bridge/DSIerror0/error1 all0, recovered1/
sourceReleased1. HashE3D71BEF unchanged/all guard bytes kept; nine preliminary
PSRAM DMA copies4096/4096 each. Original host result wasFAIL because evaluator
incorrectly demanded whole DPI register0 instead of disabled bit0. Real inherited
and restored DPI=19D0 contains persistent configuration. Fixed evaluator compares
exactDPI_BRIDGE_IDLE word and bit0clear. Same recorded device output re-evaluates
PASS; original full log/hostFAIL retained. Host reset/result footer is not target
UART data and is excluded exactly as runner's in-memory capture does. No hidden
hardware rerun or fabricated successful log. This was a verifier defect.

VisualV: after fresh owner readiness, ONE new RAM upload, temporary backlight,
1000frames/230400000items/999rearms/312827polls/15282355us. Strict runnerPASS;
last raw383FF/done1/status12/errors0/recovered1/released1. Source hashE3D71BEF/
guards intact; DMAglobal/channels0, DPIreturned19D0/mode1, duplicate requestBUSY.
Four RDDPM9C responses after memory teardown, command recovery/no errors. Owner:
“Pode iniciar agora; estou olhando”, then “Sim, imagem completa, estável e
orientação correta” in response to the complete picture/corner/color/number prompt.
This accepts qualitative full image, stability and orientation, not pixelwise CRC,
absolute tear-free output or every temperature/concurrency scenario.

Both runs restored saved first4KiB, X16MR8=4B, original clocks/DLL/pads/controller,
all nine roots/27 snapshot words and actualAXIC0000000. All32 MMU entries restored
by native window stop. Rest of reserved2MiB deliberately overwritten/not data-
restored, as prior PSRAM diagnostic. Global volatile cache/flash-MMU boot state
owned until reset; no flash storage write. Reset followed by maia.py shell/
SciCalc2+3*4=14/moduleexit0/desktop verified after EACH run. Preservedbin/ELF hashes
unchanged. No flash/SD/parser/EBNF/generated lexer modification.

## Tests, artifacts and limits

Host56/56PASS. Actual framebufferDSI and PSRAMwindow drivers plus picture test
ASan/UBSanPASS; four compiled mutants rejected (fixed source, noSARrewind,
premature source release, ignoring source lease during sync). Tests include1000
repeated frames, droppedSARrewind, transfer timeout, failed release and retained
ownership, duplicate/context/range contracts, exact full-source bounds, known
rotation/corner samples/hash and electronic/visual capture negatives. Ready-marker
stimulus sendsE/V only after completeDPI_FB_READY and only once, after two existing
VO3 acknowledgments. Independent pinned SDK field/burst/FIFO/span oraclePASS and
source revisions match. C++98 private header/picture planPASS. GCC14.2 strict native
runtime gatePASS. Initial compile warned conditional light snapshot might be
uninitialized; explicit zero initialization fixed before any target upload.

[Electronic/raw hostFAIL](native-framebuffer-electronic.txt),
[exact-data reevaluation](native-framebuffer-reevaluation.txt),
[visual capture](native-framebuffer-visual.txt), [desktop](native-framebuffer-desktop.txt),
[manifest](native-framebuffer-manifest.json). Recovery
`../maia-rtos-recovery/native-framebuffer-20261116/`; separate electronic and visual
prepared/archive directories retain their original runner/source versions.
Profile: all existingDPI_BRIDGE ancestorsON plusMAIA_NATIVE_PROBE_DPI_FRAMEBUFFER,
no SRAMtile/spatial/solid flags. Build targetmaia_sched_probe_ram; esptool elf2image;
runner `--dpi-framebuffer --capture-seconds 30` (electronic4), add
`--framebuffer-visual` only after fresh owner readiness (visual1000).

Next grouped delivery: continuous LCD service using native tasks/IRQ/completion,
source lifetime/repeated updates and stop/recovery, followed by native touch input.
Finite boot batch is not continuous production refresh, LVGL/desktop integration
or native coldpower/device startup. Full capacity/31-point delayline/margin/SMP
acceptance still open. Existing bridge discard threshold413 and WT-without-readable-
ACK limitations retained; zero error status/owner observation do not prove every
pixel/underrun case. No production timing/rotation edits or legacy contract fallback.
