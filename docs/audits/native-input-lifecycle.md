# Native input/bus lifecycle with active LCD — 2026-11-16

HARDWARE-TESTED, LIMITED: three explicit input stop/restart and I2C close/reopen
cycles while the native LCD DMA, timer, heartbeat and load tasks remain active.
This extends the qualified warm touch/LCD stack. It is not cold hardware startup.

## Implementation and contracts

Touch stop is owner-thread only and refuses a poll/reentrant callback. It stops
wire reads and invalidates held contacts without creating a physical UP. Restart
rereads status first, then identity/capabilities with IRQs enabled. Only original
firmware/maxima/slot count are accepted. Any read/mismatch leaves stopped/invalid
state; explicit retry is allowed because this path does not reset hardware.
A fresh complete coordinate report is required before state becomes valid again.
Sequence and statistics remain cumulative; stop/restart counters are visible.

Bus close/reopen requires native core0 task context, no active wire call and both
lines sampled high. Reads while closed return INVALID without I/O/output changes.
The same stationary callbacks, pin reservation and configured timebase remain
owned; no power, GPIO mux, clock reset or physical recovery is implied. All
clients must be stopped before close. Read eligibility is checked in the same
IRQ-protected transaction as acquiring ownership, preventing close/read races.

Changed files: public touch/I2C headers, native services, host lifecycle negatives,
opt-in hardware profile/harness, capture evaluator and documentation. No kernel,
parser/EBNF/generated source, flash or SD writes. Hardware register facts remain
those already qualified in [touch audit](native-touch-input.md); no new MMIO or
unverified hardware reset protocol introduced.

## Actual target evidence

[Unedited capture](native-input-lifecycle-hardware.txt), Tab5 ESP32-P4rev1.3,
`/dev/cu.usbmodem1301`; standalone GCC14.2 RV32IMAC/ILP32, esptool4.12.
RAM image SHA2567204dc1076dfc79d12326a95a1b6f6108a8f943e9173084ec4037a2d7011bf90,
94896bytes. Load/run/reset only; preserved original firmware remains installed.

- Three restart cycles, four stops including final shutdown. Touch ended stopped
  and bus closed. Three refused polls without wire access, three closed-bus reads
  refused with sentinel unchanged, three monotonic sequence/invalidation checks.
- 296polls,302bus calls (six status/capability transactions for three restarts),
  zero bus/decode errors or contention. No fresh contact reports in this electronic
  session; prior physical corner/drag/release acceptance is in the touch audit.
- 102LCDframes/actualDMAIRQs/posts,107notification waits, four cache publications/
  presentations, no timeout/spurious/DMA/common/bridge/DSI errors. Active final
  stop, guarded source release and exact IRQ route/control restoration pass.
- 1568sessionticks/1569timerIRQs/1460heartbeat iterations, CPU-load041129BE;
  all four other tasks exited before quiescent diagnostic teardown.
- Source hashE3D71BEF/guards, saved4KiB/mode4B/clocks/MMU and fourRDDPM9C replies
  restored. Scratch data beyond saved4KiB deliberately not restored.
- Native PASS. Original host FAIL preserved: evaluator treated the intentional
  invalid-state sentinel after restart as a corrupt contact. Corrected profile
  accepts that sentinel only with zero coordinates in the explicit lifecycle
  profile and requires all restart/closed-bus/sequence counters. Re-evaluation of
  the same UART prefix before host reset PASS; no UART data or native acceptance
  relaxed. The original tool summary remains FAIL in the raw log.
- After reset: original desktop shell responsive, SciCalc2+3*4=14/moduleexit0,
  desktop left active. Baseline SHA256 remains
  21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd.

Host65/65 PASS, including previous physical capture and new lifecycle negatives.
Actual wire/touch/bus ASan+UBSan PASS; eight rejected mutants including read while
closed and accept changed firmware. C++98 public headers PASS; native runtime
linker gate permits project objects+libgcc only. Later capture rules/tests/docs
changed after target; uploaded native implementation unchanged.

Build existing framebuffer/stream/touch prerequisites with
`MAIA_NATIVE_PROBE_INPUT_LIFECYCLE=ON` (LCD lifecycle harness OFF). Run standalone
`maia_sched_probe_ram` via esptool/runner `--input-lifecycle --capture-seconds 30`.
The profile is electronic-only; do not combine visual mode. Exact source snapshots,
ELFs/map/config, command logs/checks and original FAIL/re-evaluation proof are in
`../maia-rtos-recovery/native-input-lifecycle-20261116/`.

## Remaining startup dependencies and next work

The native services now support clean warm lifecycle, but neither reinitializes
PSRAM silicon/supply or powers down board pins. CPU1 is parked. Physical held-line,
frozen-timebase, power loss, new clients and SMP remain separate gates.

| Component | Native qualification | Remaining inherited startup dependency |
| --- | --- | --- |
| LCD panel/DSI | Native reset, vendor commands, LDO3/PHY, RGB565 DMA and updates | Existing supply prerequisites, cold board boot and brightness lifecycle |
| Touch/I2C | Full report reads, rotation, physical feedback, stop/revalidate/restart | Prepared stationary GPIO/timebase; no cold supply or physical bus recovery |
| PSRAM device | Known AP identity, X16/X8 switching,2MiB mapped data/DMA | Powered device and known inherited latency/mode baseline |
| PSRAM clock | Native bounded MPLL divider read,200MHz routing/DQS/DLL/phase checks | MPLL already powered/calibrated, supply ready; no native PLL start/retuning |
| Cache/window | Native controller/MMU/ROM cache setup and owned publication | Boot-exclusive global cache state, fixed2MiB reservation; no general clients |

Source comparison: pinned ESP-IDFf5c3654
`components/esp_psram/device/esp_psram_impl_ap_hex.c:esp_psram_impl_enable` starts
MPLL ownership/frequency, controller clock/reset, pads/DQS/CS timing, full mode
register setup, identity/density, controller config and timing tuning. Current
native implementations cover verified subsets and explicitly refuse unmet roots;
calling warm restart does not replace the missing stages. Reference revision and
native contracts are already in EVIDENCE.md; no SDK runtime/header is linked.

Next grouped work is native PSRAM device/clock cold-start ownership, starting from
these primary algorithms and board supply evidence, with a separate native boot
acceptance. Do not clear warm guards merely to claim cold support. Brightness,
full memory capacity/margins, multi-finger, lossless input, full native desktop
and other drivers retain their independent acceptance gates.
