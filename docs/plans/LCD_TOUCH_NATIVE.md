# Native LCD and touch delivery — 2026-11-15

Owner priority: finish LCD and touch on native MaiaRTOS, then remaining drivers,
then resume MaiaScript. No FreeRTOS/Arduino/SDK runtime/shim. Existing functional
flash remains fallback, not evidence of native completion. Parsers retain EBNF +
MaiaCC ownership. Each completed verified unit gets separate commit/push.

- [x] Native panel selection/reset/power/DSI and finite SRAM DMA output; owner
  confirmed stable colored central32-row incremental-source marker. This is a
  boot diagnostic with IRQs off, not a production graphics service.
- [x] PSRAM route audited as HEX/X16, read-only native controller state sampled.
- [x] Native inherited HEX mode-register identity and exact command-port restore;
  two stable series confirm AP/32MiB/X16. Cold startup is not inferred.
- [x] Native warm XTAL/core-div1/bus-div2 clock qualification with repeated mode
  reads and exact restore. Nominal20MHz; cold startup remains open.
- [ ] Independent VO2/clock/mode startup, conservative
  speed qualification and bounds-checked owned read/write memory test.
- [ ] Cache/MMU/DMA ownership and full RGB565 framebuffer1843200bytes, then actual
  full-frame readout; no arbitrary PSRAM address or inherited SDK allocation.
- [ ] Continuous LCD service under native tasks/IRQ/synchronization, bounded stop/
  recovery/source lifetime, backlight brightness and repeated initialization.
- [ ] Native touch coordinates/contact lifecycle and orientation mapping; existing
  touch work only identifies firmware1 and verifies reset/I2C transport.
- [ ] Shared system-I2C ownership, task-safe timeouts/error propagation and recovery.
- [ ] Combined LCD/touch interaction, drawing/drag/release and concurrent task load,
  repeated native boot and failure cases. Preserve final production rotation.

Current next work: native VO2/device mode setup and a reserved physical array
read/write test, then memory/cache/framebuffer. Touch register protocol may be
studied alongside this work, with distinct tested commits. Kernel integration defects found along
the way are fixed natively; SMP and full system acceptance remain separate gates.
No schedule or overall percentage is established by counting these checkboxes.

Evidence: [latest display marker](../audits/native-dpi-dma-center.md),
[PSRAM state](../audits/native-psram-state.md),
[PSRAM warm clock](../audits/native-psram-clock.md),
[touch identification](../audits/native-touch-identify.md).
