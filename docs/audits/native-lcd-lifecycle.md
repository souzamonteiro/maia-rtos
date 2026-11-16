# Native LCD image publication and warm reuse — 2026-11-16

HARDWARE-TESTED, LIMITED. The native LCD owner accepts one queued drawing
callback, pauses at a completed frame boundary, transfers the framebuffer lease
to CPU editing, publishes the cache and resumes DMA. A clean stop permits warm
restart on the same owner, panel and reserved PSRAM window. No foreign OS runtime,
headers, allocator or IRQ service is used by the native image.

## Contract and changes

`services/lcd_output.c` and its public header define the request, restart and
per-session update/presented counters. Requests are nonblocking core0 task/ISR
operations. Duplicate pending requests return BUSY. The callback executes in the
owner thread with interrupts enabled, receives packed native-orientation RGB565,
and must return without retaining the pointer. Stop cancels untaken requests; an
already started callback publishes and completes one frame before stopping.
`updates` means cache publication; `presented` means a subsequent DMA frame
completed. Neither is a measured panel scanout/tear-free guarantee.

The SoC engine permits edits only in READY with actual DMA idle. The PSRAM CPU
edit state retains the reservation and rejects external sync/stop/acquire/release.
Publication writes back the cache, verifies MMU/controller ownership and bounded
FIFO drain, then restores DMA ownership. On uncertainty it retains a sticky fault
and cannot release memory. CPU1 remains parked and unrelated PSRAM/cache clients
are excluded; local IRQ exclusion does not provide SMP memory sharing.

Warm restart reclaims the checked source and IRQ route and reuses the owned DMA
configuration. It clears per-session counters; it cannot restart after an error,
uncertain cleanup or a failed publication. It is not cold panel/PSRAM startup.
The initial boot-only preparation contract remains unchanged. Existing finite
framebuffer and continuous immutable-image profiles remain regression tested.

Files: service/public header, SoC async engine and PSRAM lease, opt-in CMake
profile, lifecycle RAM harness, strict capture runner and host tests. Hardware
facts/licenses remain those cited in [EVIDENCE](../hardware/EVIDENCE.md); no new
register address or timing value was introduced.

## Electronic target evidence

[Raw UART](native-lcd-lifecycle-hardware.txt). Tab5 ESP32-P4 rev1.3,
`/dev/cu.usbmodem1301`; GCC14.2 RV32IMAC/ILP32, esptool4.12.
Initial image SHA256 **c9a494b87c32c4247bd8109e1c5b1a1e0b579004376dba0f53c618499d5f0f35**.
Three warm sessions, each 24 frames and two full image publications, with clean
active stop and release: 72 frames/72 actual DMA IRQs/72 notifications total.
No errors, timeouts or spurious IRQ. Same native200MHz PSRAM,720x1280 RGB565 source,
1280x720 logical orientation/software90degree mapping.

Each callback XORs all 921600 pixels, yielding the independently computed FNV
BCC4E003; the next restores original E3D71BEF. Six hashes alternate exactly.
Each edit takes68ticks in this diagnostic (including a full-source hash and
intentional3tick delay), with3 heartbeat iterations while the CPU lease is held.
This is liveness evidence, not a worst-case latency or rendering speed bound.
Session total1523ticks,1524timerIRQs,1409ecalls,1048heartbeat iterations; load
counter03A3DA81. All three other tasks exited before diagnostic teardown.

Source guards/hash,9 preliminary byte-exact4KiB mirror copies, saved4KiB/mode4B/
clocks/MMU restoration and four panel RDDPM9C replies pass. Matrix source24/control
return to exact previous values; CLIC18 used, occupied17 preserved. Full2MiB
scratch contents beyond saved4KiB are deliberately not restored. Native and strict
host evaluator PASS. Original installed full firmware is preserved, not replaced.

## Visual confirmation

[Unedited visual UART](native-lcd-lifecycle-visual-hardware.txt), same initial image:
253frames per session,759total DMA frames/IRQs/posts, three clean active stops and
six correct image publications.12022sessionticks,12023timerIRQs,10862heartbeat
iterations, no errors/timeouts/spurious IRQ, same source hash/guards/restoration.
Fresh owner readiness was obtained before running. Owner confirmed:
“Sim, alternou corretamente e ficou estável”. This supports whole-picture visible
updates in the tested warm configuration, not pixel-perfect/tear-free guarantees.

Final harness correction makes the duplicate-request negative atomic against task
preemption (otherwise the owner could legally consume the first request before
the second arrives). Driver/service code is identical to the visual run. The
final image and its separate electronic regression are recorded below; no new
visual observation of that harness-only change is claimed.

Final image SHA256 **3fda28fd6228a4bc40cd58527aa48dde2c66d4489c8ba6307c8089b16cd06f48**,
89792bytes. [Final electronic UART](native-lcd-lifecycle-final-hardware.txt):
three24frame sessions/72IRQs, six correct publications, clean active stops and
source release, no errors/timeouts.1523ticks/1524timerIRQs/1048heartbeats;
load03A3DCA1. Native/strict host PASS. Earlier and final images are both preserved.
After both visual and final electronic resets, shell/SciCalc2+3*4=14/moduleexit0/
desktop verification PASS.
No flash/SD writes; fallback SHA256 remains
21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd.

## Tests and reproduction

CMake/CTest61/61 PASS. Actual router/service/engine/window ASan+UBSan PASS;
six mutants rejected (omit cache publication, omit engine publish, skip route
scan, omit timeout cancel, leave IRQ enabled, wrong item count). Host negatives
cover active edits, duplicate requests, stop cancellation, failed publication,
wrong owner/context and error restart. C++98 service/engine headers and pinned
SDK IRQ/CLIC oracle pass. Native runtime gate permits project objects+libgcc
only, no unresolved/foreign symbols. Late changes to tests/header comments and
capture validation do not change the uploaded native implementation.

Configure existing framebuffer/stream prerequisites plus
`MAIA_NATIVE_PROBE_LCD_LIFECYCLE=ON`; build `maia_sched_probe_ram`, convert its RAM
ELF with standalone esptool. Captures use the runner's `--lcd-lifecycle`; add
`--framebuffer-visual` only with fresh owner readiness, `--capture-seconds 30`.
Source snapshots, CMakeCache, images/map/checksums, actual commands/checks and logs
are preserved in `../maia-rtos-recovery/native-lcd-lifecycle-20261116/`.

## Remaining work

This is one fixed warm single-buffer reservation. Callbacks and arguments require
explicit lifetime; callbacks must be bounded by their caller and cannot be
cancelled midway. Whole-window cache publication runs under local IRQ exclusion
and has no measured WCET. Brightness lifecycle, cold boot, target fault injection,
multiple PSRAM/cache clients, SMP, operating margins and native desktop integration
remain open. Touch contacts, rotation and native input service are next. No parser,
EBNF, flash or SD writes are part of this change.
