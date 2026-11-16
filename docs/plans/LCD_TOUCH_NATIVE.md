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
   measured slow-clock limitation for the next full-image diagnostic; sustained
   PSRAM-to-DSI throughput is still to qualify.
2. **Full-frame image — HARDWARE-TESTED, LIMITED.** Complete1843200B RGB565
   framebuffer in owned200MHz PSRAM, cache/lifetime claim, direct single-block
   DMA toDSI. Electronic4frames and fresh-ready visual1000frames pass with source
   hash/guards/restoration; owner confirms complete/stable/correct orientation.
   Native720x1280/logical1280x720/software90 preserved. Finite boot diagnostic,
   not yet continuously scheduled production graphics service.
3. **Continuous driver — PENDING.** Native tasks/IRQ/synchronization, bounded
   stop/recovery/source lifetime, brightness and repeated initialization.
4. **LCD acceptance — PENDING.** Cold boot/reboot, repeated image updates,
   concurrent task load and failure recovery. Preserve final production rotation.

## Touch delivery

- Port native contact/coordinate reading and lifecycle from the existing driver,
  with orientation mapping. Current target evidence identifies firmware1 and
  reset/I2C transport, not full touch input.
- Integrate shared system-I2C ownership/timeouts/recovery and native input service;
  verify drawing/drag/release with LCD under concurrent task load/repeated boot.

Already qualified foundation: panel selection/reset/power/DSI and finite SRAM
DMA output; owner confirmed stable colored central32-row incremental-source
marker. PSRAM AP/32MiB/X16 identity and native warm XTAL nominal20MHz clock
change/restore qualified. Raw inherited-X16 byte transfers were refused for
address aliasing; corrected X8 mode/data/restoration path is now target-tested.
These are boot diagnostics, not continuous desktop equivalence. Installed full
firmware remains original ESP-IDF/FreeRTOS fallback until native integration passes.

Next implementation: integrate continuous nativeLCD task/IRQ completion,
repeated image updates and source lifetime/stop/recovery, then touch input. Continue through internal
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
[touch identification](../audits/native-touch-identify.md).
