# Native scheduled LCD and real DMA interrupts — 2026-11-16

HARDWARE-TESTED, LIMITED: a native owner task refreshes the complete immutable
RGB565 framebuffer until another task requests stop. DW-GDMA interrupts wake the
owner through MaiaRTOS notifications. Timer preemption, heartbeat and CPU-load
tasks progress throughout the session. No FreeRTOS/Arduino/SDK runtime or headers
are linked to this diagnostic. The installed complete desktop remains the
preserved original runtime; this does not claim a completed native desktop.

## Implementation and ownership

`services/lcd_output.c` owns one warm, single-core, single-session display.
`dsi_lcd_async_impl.h` splits the qualified frame sequence into start, actual IRQ,
finish and guarded stop. The ISR masks global DMA INT_EN, records a completion
predicate and notifies the owner; it performs no logging, cache maintenance,
allocation or wait. The owner blocks with interrupts enabled and verifies done,
raw item count, natural disable, FIFO drain and error words before rearming.
Residual drain sleeps one tick; frame timeout is100ticks, cancel cleanup32polls.
The source lease remains held until actual hardware quiescence and ownership
checks permit release. These bounds are not measured worst-case latency claims.

The dedicated core0 IRQ route scans candidate CLIC17..47, excluding native timer16.
It requires disabled/nonpending control and no other producer among128 matrix
sources. Source24 and the selected control are restored exactly (pending bit
is dynamic). The existing trap/scheduler epilogue handles owned external IRQs
without advancing timer ticks; unknown IRQs remain fatal. Hardware facts and
primary source revision are in [EVIDENCE](../hardware/EVIDENCE.md).

## Target evidence

[Unedited electronic UART](native-lcd-stream-hardware.txt), ESP32-P4rev1.3 Tab5,
USB `/dev/cu.usbmodem1301`; standalone GCC14.2 RV32IMAC/ILP32, esptool4.12.
RAM image SHA256 **f62e30839ff5ac3b7ba7ad63d5f0927fd9c4569701b1414ead30659db00e8c38**,
87008bytes. Load/run/reset only; no flash/SD writes. Same qualified200MHz native
PSRAM/frame image and software90degree rotation as the preceding framebuffer unit.

- Six init results0, engine stage5/router stage6; actual selected CLIC18. CLIC17
  was already enabled and was preserved. Previous route0 and control1FC00000 restored.
- 99frames,100notification waits,99actual DMA IRQs,99posts and99kernel external
  IRQs; no spurious IRQ, timeout or reported error.99residual-drain retries.
- Stop requested while channel active; current frame completes, source released,
  done/recovered1, raw383FF (230400items), status12; common/bridge/DSI errors0.
- 1516sessionticks,1517timerIRQs,1619ecalls,1414heartbeat iterations, CPU-load
  counter04F9618A. All three other tasks exited before diagnostic memory teardown.
- Source hashE3D71BEF and guards unchanged; preliminary9x4KiB copies matched;
  original saved4KiB/mode4B/clocks/MMU restored and four panelRDDPM replies9C.
  Full2MiB scratch contents beyond saved4KiB are deliberately not restored.
- Native PASS and strict host evaluator PASS. This session is electronic; no new
  visual observation is claimed. Previous complete/stable/orientation confirmation
  remains recorded in [framebuffer audit](native-framebuffer.md).
- After reset: preserved desktop shell responsive, SciCalc2+3*4=14, module exit0,
  desktop left active. Original firmware SHA256 remains
  21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd.

Two earlier RAM attempts safely refused initialization. The second diagnostic
exposed fixedCLIC17 already enabled (3FC00100); neither started the async frame
service. Both failures and prepared images remain in recovery directories
`native-lcd-stream-init-refusal-20261116` and `native-lcd-stream-irq-busy-20261116`.
Selection was corrected to find a proven free line, without relaxing ownership.

## Reproduction and automated checks

Build the standalone `maia_sched_probe_ram` target with its existing bridge/
framebuffer prerequisites and `MAIA_NATIVE_PROBE_LCD_STREAM=ON`; convert its RAM
ELF using standalone esptool `--chip esp32p4 elf2image`. CMakeCache, source snapshots,
ELFs/map, command logs, checks and hashes are preserved in
`../maia-rtos-recovery/native-lcd-stream-20261116/`.

```sh
PYTHONDONTWRITEBYTECODE=1 <idf-python> tests/hardware/run_native_scheduler_probe.py \
  --port /dev/cu.usbmodem1301 --image <build>/maia_sched_probe_ram.bin \
  --log <capture> --lcd-stream --capture-seconds 30
```

Host CMake/CTest60/60 PASS, including legacy finite framebuffer and real-capture
validator positives/negative init, task, IRQ, ownership, corruption and duplicate
records. Actual router/service/engine ASan+UBSan PASS; four mutants rejected:
skip route scan, omit timeout cancel, leave IRQ enabled, accept wrong item count.
Public service/engine headers compile as C++98. Readonly SDK oracle confirms
source24, matrix address,32externalCPUlines/offset16, CLIC masks and INT_ENbit1
against pinned f5c3654. Native linker runtime gate PASS: project objects+libgcc,
no unresolved/foreign symbols. Late changes only strengthen the host evaluator,
add tests/docs/header comments; uploaded native implementation is unchanged.

## Remaining work

Source publication/update API, repeated initialization and brightness lifecycle,
cold PSRAM/panel boot, concurrent clients, fault recovery on target, SMP and
operating-margin qualification remain open. Current session holds one immutable
full buffer; it does not accept application drawing. Native timeout/cancel guards
are host-tested; this target run proves graceful active-stop recovery only.
Next grouped unit: safe image publication/reuse and LCD lifecycle, followed by
native touch contacts/rotation/service integration. Preserve complete fallback
until production integration and acceptance pass. No parser/EBNF changes.
