# Native calibrated PSRAM supply configuration — 2026-11-16

HARDWARE-TESTED LIMITED: native adoption of the already enabled VO2/1900mV
supply followed by the native PSRAM/MPLL/LCD/input pipeline. Disabled-supply
programming is HOST-TESTED only. No supply power cycle/cold startup acceptance.

## Findings and implementation

The previous clock handoff checked calibrated VO2 but could not configure it.
Three issues determine the port: VO2 also powers MPLL, missing dedicated trim
must not become a guessed voltage, and digital enable readback cannot establish
electrical stabilization or PSRAM power-on defaults. Primary pinned sources and
field masks are in [EVIDENCE](../hardware/EVIDENCE.md#native-psram-vo2-configuration--2026-11-16).

`soc_psram_supply.c` adapts Apache-2.0 vendor LDO voltage/owner/ripple/enable
operations to a fixed native boot-lifetime claim. No foreign headers/runtime,
FreeRTOS locks, heap, adjustable-voltage interface or reference-count service.
The public report/API lives in `esp32p4_psram.h`. Requires boot/core0/IRQs-off,
CPU1 parked, internal SRAM code/data, exclusive supply and all consumers, before
native clock claims. Rev1.3/schema and RS checks plus nonzero dedicated1900mV
DREF/MUL are mandatory. Reports never grant ownership.

An enabled supply is adopted only with exact calibrated SW-owned/non-bypass/
ripple-suppressed fields, with no writes. A disabled supply can be programmed
only with both PSRAM core/PLL gates and MPLL circuit/output disabled. No gated
controller reads occur. Analog trim/ripple, software routing and enable stores
preserve every unrelated field. Fixed eFuse/consumer/control snapshots are checked
before and after stores; ignored stores or foreign changes fail closed. Any
attempted-programming failure remains sticky until reset, without blind rollback.
Duplicate claims/context errors refuse without MMIO. The API does not disable
the supply, wait for electrical stabilization or claim memory readiness. A future
off-supply caller must independently qualify startup before enabling consumers.

## Actual target evidence

[Unedited UART](native-psram-supply-hardware.txt), Tab5 ESP32-P4rev1.3,
MAC e8:f6:0a:e2:fd:4c, `/dev/cu.usbmodem1301`; standalone GCC14.2 RV32IMAC/ILP32,
esptool4.12. Image SHA256
**119322f50a2443dfcadabc0fdc2e2e36c078683c6565f27c01ffbdd715db1138**,
100304bytes. RAM load/run/reset only; no flash/SD writes or parser changes.

- Supply result0/stage4/adopted1/writes0: DREF7/MUL5, control40200180 and
  analog76800000 unchanged. This proves checked adoption, not a voltage change,
  rail measurement or transition from an unpowered device.
- Native MPLL circuit cycle/program/calibration END0->1/400MHz, mode profile
  programming/restoration8D30/20/4B, PSRAM200/DQS/bothDLL,1843200byte full image
  hashE3D71BEF/guards and nine byte-exact4KiB DMA cases pass.
- 102LCDframes/real DMAIRQs/posts, controller errors/timeouts/spurious0; four
  image publications. Three touch/bus restart cycles plus final stop/close,
  295polls/301bus calls/errors/contentions0. No touch stimulus/new visual claim.
  1568ticks/1569timerIRQs/1460heartbeats/CPU-load040D6DDE, four other tasks exited.
- Saved4KiB, mode/clock/controller/MMU/IRQ routing restored, four panelRDDPM9C
  replies. Scratch data beyond saved4KiB is deliberately not restored. Original
  desktop shell/SciCalc2+3*4=14/moduleexit0/desktop PASS after RAM reset.

## Automated validation / reproduction

Host70/70 PASS, including real capture and negative calibration/write/owner/
pipeline records. Actual supply implementation ASan+UBSan PASS: enabled adoption,
disabled programming/order/preserved fields, missing trim/revision/RS, each live
consumer, foreign snapshot changes, every ignored store and sticky refusal.
Five non-equivalent mutants rejected: allow missing calibration, ignore active
consumers, ignore foreign clocks, discard unrelated analog fields, clear sticky
fault. An initial redundant-readback mutant survived because the following whole
snapshot check still refused it; the initial harness log is preserved and the
redundant mutant replaced. This was a test-harness expectation failure, not a
target failure or a relaxed acceptance guard. Original pinned PMU/eFuse bitfield
oracle and source byte pins PASS; C++98 public header PASS. Native link runtime
gate permits project objects+libgcc only, with no unresolved/foreign symbols.

Build existing profile prerequisites with `MAIA_NATIVE_PROBE_PSRAM_SUPPLY=ON`,
target `maia_sched_probe_ram`, convert RAM ELF using esptool, then runner
`--psram-supply --capture-seconds 30`. This profile explicitly refuses anything
except unchanged powered-supply adoption; it cannot silently turn into a cold
test. Full source/image/config/checks/commands/UART/desktop evidence preserved in
`../maia-rtos-recovery/native-psram-supply-20261116/`.

Remaining: documented discharge/settling/reset and native consumer sequencing
from genuine power-on state, physical differing-mode/default qualification,
full32MiB/margins/SMP/shared consumers/native desktop. Next grouped task is that
cold-supply/device entry; retain the working fallback and existing strict warm
guards, rather than using this powered run as evidence of cold completion.
