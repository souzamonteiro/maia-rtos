# Native touch contacts and LCD feedback — 2026-11-16

HARDWARE-TESTED electronic idle path; physical gestures pending. No production
touch acceptance yet.
Ported Espressif ST7123 capability/report algorithms into a bounded native reader,
with ten-slot fixed storage and explicit masks. ST7121/fw1 requires device evidence.
Source revision, protocol sections and license are in EVIDENCE.md.

Changes: bounded16bit block read with ACK intermediates/NACK last and atomic
output after final STOP; native core0 I2C task serialization; controller decoder;
latest-state touch service/checked rotations; separate touch+LCD RAM harness.
No parser/grammar/generated sources, flash or SD writes. Complete installed
original-runtime desktop is preserved as fallback.

The input service holds state on no fresh report. Fresh empty reports indicate
release; wire/decoder errors invalidate input and require local gesture cancel,
not a fabricated UP. A complete report recovers valid state. Snapshots expire
when polling stops for more than100ticks and may coalesce intermediate reports.
This is not a lossless tap/gesture queue. Coordinates outside the actual raster
are errors, even if device-advertised maxima include an endpoint beyond the raster.

The bus is prepared exclusively at boot, adopted before scheduling, then accessed
only through the native task service. Try-lock contention returns BUSY; no hidden
retry. GPIO phases run with IRQs enabled; bounded UNIT0 capture masks local IRQs
briefly to serialize the shared timer snapshot. No task reconfigures timer clocks.
CPU1 is parked, no SMP/multi-master/general ISR bus access is claimed.

Host64/64 PASS; wire/controller+service/bus ASan+UBSan PASS; six negative mutants
rejected: NACK intermediate, read only one slot, convert no-data to release,
wrong rotation, missing bus serialization, preserve validity after error.
C++98 public headers PASS. Full1..70byte read coverage and every injected pause
failure verify ACK sequence, output commit and line release, plus all prior bus
regressions. Driver bounds, slot9, fresh/empty/stale/error recovery, rotations,
wrong owner/context and nested-call refusal are covered. Native runtime gate
PASS with project objects+libgcc only. Tests/mocks do not establish physical touch.


Electronic [raw UART](native-touch-lcd-electronic-hardware.txt), native RAM image
cf6d6fcf84f1051d2e982d17576d39f937a121929e3e66a0a8c12d6fa9d1ca88/92976bytes.
Actual capabilities: firmware1, maximumX720, maximumY1280,10slots.295polls with
no fresh coordinate report (no synthetic release),295native bus calls, errors0,
contention0;101LCDframes/realIRQs/posts,0timeouts/spurious/errors. One explicit
cleanup publication/presentation.1548sessionticks,1549timerIRQs,1444heartbeats,
CPU-load040CDDF4. All four other tasks exited before teardown. Source hashE3D71BEF,
guards, saved4KiB/mode4B/clocks/MMU and fourRDDPM9C replies restored. Native/strict
host PASS. No physical touch functionality is inferred from the idle result.


Final status-first image2482663ab6063b6ee76e97477858e3f5795efda43139c34194d15ae180f77fda/
93040bytes, [final electronic UART](native-touch-lcd-final-electronic-hardware.txt).
Status must be Normal0/Idle4 before capability registers are accessed; an Init
state returns BUSY without reading other registers. The same status is checked
again in the capability block. Final electronic run again295polls/101LCDframes,
295bus calls/0errors/contention,0fresh reports, source guards/restoration PASS.
1548ticks/1549timerIRQs/1443heartbeats; native/strict host PASS. Initial electronic
image and source evidence retained, not overwritten. The packaged source driver,
idf_component.yml and Apache license match CHECKSUMS.json (source SHA256
f2d6123929f3a2fd8e291cba703047375b93b82df9e10a207c6f186e6ddaf0d7).

## Physical acceptance — NOT RUN / awaiting owner readiness

A single grouped20second test is prepared: tap/lift numbered corners1red,2green,
3blue,4yellow, then drag through center and lift. A white cross should follow the
finger, then disappear on release. Runner requires corner maskF, at least4downs/
4ups/3moves/6completed image publications, actual DMA IRQs, task progress, zero
bus/decode/DSI errors, final source restoration and return to preserved firmware.
No gestures, contact accuracy, multi-finger behavior or visible cursor alignment
have been observed yet. Polling70byte reports under preemption remains to be
qualified by this test; the idle electronic path is not a substitute.
Owner presence was requested asynchronously and has not yet been confirmed.
Do not launch visual/touch mode without a fresh readiness response.

```sh
PYTHONDONTWRITEBYTECODE=1 <idf-python> tests/hardware/run_native_scheduler_probe.py \
  --port /dev/cu.usbmodem1301 --image <native-build>/maia_sched_probe_ram.bin \
  --log <capture> --touch-lcd --framebuffer-visual --capture-seconds 30
```

Hardware profile `MAIA_NATIVE_PROBE_TOUCH_LCD=ON` requires existing framebuffer/
stream prerequisites; `MAIA_NATIVE_PROBE_LCD_LIFECYCLE=OFF`. Build only standalone
`maia_sched_probe_ram`; use esptool elf2image then load_ram, never flash this probe.
Recovery snapshots/images/map/config/checks/logs are in
`../maia-rtos-recovery/native-touch-input-electronic-20261116/`. Final reset desktop
verification is recorded there: responsive shell, SciCalc2+3*4=14, moduleexit0,
desktop left active. No flash/SD/parser writes. Complete installed firmware still
uses its original runtime; new native diagnostic imports none.

Implementation scope is HOST-TESTED contacts/service and HARDWARE-TESTED LIMITED
native bus/capability/idle polling concurrent with LCD. Physical contacts remain
NOT RUN. Cold startup, bus teardown/physical fault injection, multi-master/SMP,
proximity/gestures/checksum validation, lossless event queue, other clients and
full native desktop integration are separate open gates. Do not mark native
production touch complete from electronic idle evidence.
