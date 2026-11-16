# Native PSRAM/MPLL consumer stop and resume — 2026-11-16

HARDWARE-TESTED LIMITED: native consumer gating, MPLL circuit power-down/resume,
then fresh native calibration and PSRAM/DMA/LCD/input qualification. VO2 remains
powered. This is a prerequisite for supply sequencing, not cold-device startup.

## Research and scope

The previous supply unit refused configuration with live consumers but supplied
no native operation to stop them. Pinned vendor sleep/clock algorithms provide
the order; ported with Apache-2.0 attribution, no SDK runtime/header/FreeRTOS.
[EVIDENCE](../hardware/EVIDENCE.md#native-psrammpll-consumer-parking--2026-11-16)
records revision, functions, gates and the actual Tab5 schematic.

The owner's `docs/hardware/Tab5.pdf` is a17-page product-documentation export.
Its official linked5-sheet schematic was downloaded and visually inspected:
ESP_LDO2 connects P4 pins72/59/67 and capacitors; flash uses separate LDO1. The
local P4 datasheet v0.5 identifies the PSRAM supply but does not specify the
fitted AP32MiB HEX/SIP memory discharge/initialization/reset requirements.
Generic Octal device timing is not substituted. Physical supply cycling remains
unimplemented until an applicable primary specification is established.

## Native contract and files

`soc_psram_clock.c` now owns `maia_esp32p4_psram_consumers_park(restore,report)`;
declaration/report/error/context contract in `esp32p4_psram.h`. Requires genuine
native XTAL20 claim, idle controllers/closed AXI, CPU1 parked, IRQs off, internal
SRAM execution/data, exclusive stopped PSRAM/cache/DMA/analog/MPLL/F25/F50 clients,
no X8/pad claim. This is private boot control, not a runtime shared-clock manager.

Park gates PSRAM core then its module PLL clock, gates MPLL output then circuit.
The shared register-bus clock is retained. All other register fields and VO2 stay
unchanged. No controller reads occur with the core gated. Clock reports0MHz while
parked and fast/stop operations refuse. Resume powers circuit then output,
restores module PLL and finally the XTAL core; original roots/controller words
must still match. Resume does not claim MPLL ready; diagnostic runs native
initialize400 before selecting MPLL. Attempted-store failures remain sticky,
with no unsafe retries/rollback. Invalid/context/duplicate/no-claim refuse without
MMIO. Fixed bounded operations, no allocation, no new delays or RT guarantees.

`native_dpi_framebuffer.h`/runner add explicit park profile before recalibration;
CMake requires the prior supply/profile/LCD pipeline. Host tests verify ordering,
unrelated fields, gated-read refusal, duplicate/context/clock exclusions, all
eight ignored stores and foreign changes, sticky failure and exact restoration.
Capture tests check actual rows and negative fields/duplicates/order/calibration.
No parser/EBNF/shell/MaiaScript changes.

## Actual Tab5 evidence

[Unedited UART](native-psram-park-hardware.txt), P4rev1.3,
MAC e8:f6:0a:e2:fd:4c, `/dev/cu.usbmodem1301`, standalone GCC14.2 RV32IMAC/ILP32,
esptool4.12. RAM image SHA256
**5c5e46958229260d1d01ab316a02e23417fde572cde3ded616e683f1653023b2**,
101072bytes. RAM load/run/reset only; no flash/SD writes.

- Park/result0/stage4/writes4: PERI C00C->000C, RF_PWC09000000->08000000,
  MPLL output1FFCFFFD->0FFCFFFD. Resume/result0/stage4/writes4/restored1 returns
  all three words exactly. VO2 adoption unchanged40200180/76800000, trim7/5.
- Subsequent native MPLL initializer observes CAL024C->034C, END0->1,
  divider99/400MHz and register3=50->70, four analog writes/29polls. Native mode
  programming/restoration8D30/20/4B, PSRAM200MHz/DQS/bothDLL, full1843200byte
  image hashE3D71BEF/guards and nine byte-exact4KiB DMA cases pass.
- 102LCDframes/actualDMAIRQs/posts, no errors/timeouts/spurious. Three touch/bus
  restart cycles plus final stop/close,295polls/301bus calls/errors0, four image
  publications.1568ticks/1569timerIRQs/1460heartbeats/CPU-load040D0CCA; all four
  other tasks exited. Electronic test only; no new physical-touch/visual claim.
- Saved4KiB/mode/clock/controller/MMU/IRQ route restored, four panelRDDPM9C
  replies. Scratch beyond saved4KiB is not data-restored. Preserved desktop
  shell/SciCalc2+3*4=14/moduleexit0/desktop verification PASS after RAM reset.

## Build and automated validation

Host72/72 PASS. Actual park/clock implementation ASan+UBSan with size optimization
PASS. Five mutants rejected: omit module PLL gate, wrong power-off order, restore
core too early, clobber unrelated power fields, read gated controllers. Six
pinned vendor source byte checks and C++98 public header PASS. Native runtime
gate permits project objects+libgcc only, no unresolved/foreign symbols.

Initial native link refused low-SRAM overflow416bytes; no new image was executed.
Clock source uses `-Os` only for this diagnostic profile. This initially emitted
unresolved compiler-generated `memcpy` for structure assignment, also refused at
link. An explicit bounded boot snapshot copy removes that dependency; no libc
was linked. Checked-store helper remains a single body. Final link/runtime gate
passes with unchanged160KiB window and16KiB boot stack, BSS end4FF23FA0/stack
top4FF27FA0. Task stacks/guards unchanged and target checks pass. Build refusals
are retained; old RAM files accidentally copied during failed preparation are
explicitly marked NOT this implementation/NOT RUN and are not test evidence.

Reproduce existing prerequisites plus `MAIA_NATIVE_PROBE_PSRAM_PARK=ON`, build
`maia_sched_probe_ram`, esptool elf2image, runner `--psram-park --capture-seconds 30`.
Evidence/source/image/config/checks/desktop and schematic in
`../maia-rtos-recovery/native-psram-park-20261116/`.

Remaining/next: exact fitted memory supply/reset timing and native device entry
after genuine supply loss; do not infer it from MPLL circuit power-off. Full32MiB,
margin testing, SMP/shared consumers and complete native desktop remain open.
