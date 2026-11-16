# Native LCD and touch delivery — 2026-11-15

Owner priority: finish native LCD+touch, remaining drivers, then MaiaScript.
Adapt existing licensed SDK/M5Stack algorithms/drivers to MaiaRTOS services.
No foreign OS/runtime/shim. Group internal steps into functional deliveries;
existing authorization covers implementation/electronic tests without per-step
approval. Keep verified commits/push/recovery points; coordinate actual visual/
touch observations when needed. Parser ownership remains EBNF + MaiaCC.

## Four LCD delivery gates

1. **Native PSRAM startup and usable owned memory — IN PROGRESS.** A port of
   Espressif's FIFO read/write algorithm plus checked X8 mode switching now passes
   four byte-exact patterns over reserved4KiB on Tab5 and restores saved contents,
   X16 mode and clocks. This is a warm boot-exclusive command path. Own remaining
   supply/device startup, full capacity and operational-speed
   qualification before calling PSRAM ready for production clients. A native
   controller/pad/MMU/cache port now passes2MiB CPU write/read and independent
   post-sync direct-device checks. This provides space for one complete frame.
   Native DMA reads now pass all nine4KiB address/burst cases, including a page
   crossing. Operational200MHz clock/DQS/both DLL paths now pass2MiB CPU and
   all45 training/final DMA copies;4KiB16-item samples22–23us. This removes the
   measured slow-clock limitation; sustained full-image DMA now runs in later
   gates. Native powered-domain MPLL400 restart/programming with observed END0->1
   and downstreamPSRAM200/LCD also passes. PSRAM supply/device-mode cold entry,
   full capacity and operating margins remain open.
2. **Full-frame image — HARDWARE-TESTED, LIMITED.** Complete1843200B RGB565
   framebuffer in owned200MHz PSRAM, cache/lifetime claim, direct single-block
   DMA toDSI. Electronic4frames and fresh-ready visual1000frames pass with source
   hash/guards/restoration; owner confirms complete/stable/correct orientation.
   Native720x1280/logical1280x720/software90 preserved. Finite boot diagnostic,
   not yet continuously scheduled production graphics service.
3. **Continuous driver — HARDWARE-TESTED, PARTIAL.** Native owner task, real DMA
   IRQ notification,100tick deadline and guarded active stop/source release now
   pass99frames with heartbeat/CPU-load tasks progressing. Single warm session
   now extended to CPU edit/cache publication and three clean warm restarts.
   Six image updates per run/759visual frames passed; owner confirms stable
   whole-image alternation. Brightness lifecycle/cold initialization and concurrent
   PSRAM clients remain pending.
4. **LCD acceptance — HARDWARE-TESTED, PARTIAL.** Repeated complete image updates,
   warm reuse and concurrent SRAM tasks pass in the native harness. Cold boot,
   brightness lifecycle, unrelated PSRAM clients and target fault recovery remain
   pending. Preserve final production rotation.

## Touch delivery

- HARDWARE-TESTED, LIMITED: native70byte/full-slot reads, rotated latest-state
  input and LCD feedback pass corner taps, drag and release.1148polls/826reports,
  ten downs/ten ups/96moves,827image publications/1211DMAframes with no errors.
  Owner confirms correct cross position/following/disappearance. Single-finger
  warm test; multi-finger target/cold/full-production acceptance remain open.
- Native task bus ownership/timeouts and input service are integrated and tested
  with concurrent native LCD/timer/heartbeat/load. Physical bus recovery, additional
  bus clients and repeated cold boot remain open. Native warm input stop/restart
  and software bus close/reopen now pass three cycles with the LCD active; this
  retains prepared pins/timebase rather than powering them down.

Already qualified foundation: panel selection/reset/power/DSI and finite SRAM
DMA output; owner confirmed stable colored central32-row incremental-source
marker. PSRAM AP/32MiB/X16 identity and native warm XTAL nominal20MHz clock
change/restore qualified. Raw inherited-X16 byte transfers were refused for
address aliasing; corrected X8 mode/data/restoration path is now target-tested.
These are boot diagnostics, not continuous desktop equivalence. Installed full
firmware remains original ESP-IDF/FreeRTOS fallback until native integration passes.

Next implementation: native PSRAM mode/latency/interface and supply/device
cold-start ownership (powered MPLL restart now qualified), with the
remaining brightness/concurrency acceptance gates. Continue through internal
substeps autonomously; do not stop just to request permission for the next small
register operation. Fix integration defects in native layers. SMP and full-system
acceptance remain separate gates; no schedule/percentage follows checkbox counts.

Evidence: [SRAM marker](../audits/native-dpi-dma-center.md),
[PSRAM identity](../audits/native-psram-identity.md),
[warm clock](../audits/native-psram-clock.md),
[ported data path](../audits/native-psram-array.md),
[mapped2MiB window](../audits/native-psram-window.md),
[PSRAM DMA](../audits/native-psram-dma.md),
[operational200MHz](../audits/native-psram-fast.md),
[full framebuffer/owner observation](../audits/native-framebuffer.md),
[scheduled LCD/real IRQ](../audits/native-lcd-stream.md),
[image publication/warm reuse/visual acceptance](../audits/native-lcd-lifecycle.md),
[touch identification](../audits/native-touch-identify.md),
[touch physical qualification](../audits/native-touch-input.md),
[input lifecycle/startup dependency inventory](../audits/native-input-lifecycle.md),
[native MPLL startup](../audits/native-mpll-initialize.md).
