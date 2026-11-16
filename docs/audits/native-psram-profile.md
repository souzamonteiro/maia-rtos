# Native PSRAM latency and mode programming — 2026-11-16

HARDWARE-TESTED, LIMITED: native MR0/MR4/MR8 programming/readback and restoration,
followed by the qualified PSRAM200MHz/framebuffer/DMA/LCD/input pipeline. This is
a powered, readable-device entry, not a completed cold supply/device boot.

## Port and contract

Adapted `s_init_psram_mode_reg` in pinned ESP-IDFf5c3654
`components/esp_psram/device/esp_psram_impl_ap_hex.c`, Apache-2.0; original notices
retained. Register fields/source oracle are in [EVIDENCE](../hardware/EVIDENCE.md#native-psram-mode-profile--2026-11-16).
No SDK/FreeRTOS headers, ROM SPI helper, allocation or foreign runtime linked.

Boot/core0/IRQs-off, code/data in internal SRAM, genuine native XTAL20 clock claim,
closed AXI, no memory/cache/DMA clients. Requires readable AP32MiB HEX identity
and X16 entry with recognized MR8 fields. It does not require the old MR0/MR4
latency fields already match200M. Unknown identity/reserved/interface states are
refused. Read-register protocol availability is still an entry prerequisite.

Initialization saves MR0/1 word, MR4 byte and MR8 byte. It sets MR0 drive0/read
latency4/fixed1 (mask3F/value30), MR4 write latency1 (maskE0/value20), and native
X16 burst profile MR8=4B, retaining other MR0/MR4 fields. MR0 and MR4 use vendor
16bit writes. MR4 upper byte is explicitly formed from the known MR8 byte;
separate validated reads avoid assuming an unqualified paired MR4/MR8 response.
MR8 uses the already qualified one-byte write instead of copying a16bit access
past the vendor's one-byte MR8 object. No arbitrary mode-write API is exposed.

The existing separately verified X8 claim then switches4B->0B for byte-safe
array/window transfers. Restore requires all users stopped, fast clock back to
XTAL20 and X8 claim restored to X16. It verifies current profile ownership, writes
original values and rereads them. Unexpected mode changes poison the shared
profile state; legacy array/X8 entry cannot bypass it. Any attempted programming
failure remains sticky, with no blind rollback while hardware state is uncertain.
Individual waits budgeted1..100000; not measured WCET/device ACK guarantees.

## Actual Tab5 evidence

[Final unedited UART](native-psram-profile-hardware.txt), ESP32-P4rev1.3,
`/dev/cu.usbmodem1301`; standalone GCC14.2 RV32IMAC/ILP32, esptool4.12.
Final RAM image SHA256 **57927950a3d722e5295b3a47a6e8b682a490cba8adb2f61612f872359515f1c3**,
99168bytes. Load/run/reset only; no flash/SD writes.

- Init result0/stage3/write-attempt mask7/10polls; MR0/1=8D30, MR4=20, MR8=4B
  before and after. Device was already in the requested latency/burst profile.
  This proves native transaction/readback/functional integration on that entry,
  not a physical transition from factory defaults or an ACK for idempotent writes.
- Native MPLL circuit restart/program/calibration END0->1/400MHz and subsequent
  PSRAM200/DQS/bothDLL setup pass. Full1843200byte image hashE3D71BEF and surrounding
  2MiB-window guards pass, nine byte-exact4KiB DMA address/burst cases match.
- 102LCDframes/actualDMAIRQs/posts, no timeout/spurious/controller errors; four
  cache publications/presentations. Input/bus three restart cycles plus final
  stop/close pass,295polls/301bus calls/errors/contentions0.1568ticks/1569timerIRQs/
  1460heartbeats/CPU-load041155DB, all four other tasks exited before teardown.
- Restore result0/stage3/mask7/9polls/restored1, exact original8D30/20/4B; saved
  4KiB/mode/clocks/MMU/IRQ route and four panelRDDPM9C replies restored. Source
  guards/hash unchanged. Scratch data beyond saved4KiB is deliberately not restored.
- Native/strict host PASS. Original desktop shell/SciCalc2+3*4=14/moduleexit0/
  desktop verification PASS after both actual RAM runs; original firmware kept.

Initial image3000f4de6b669b821e1b05f7500bb256b13f9f8444f414cdb25c7030697d6ce2 also
passed the same pipeline. A later failure-path correction poisons the profile on
unexpected changes during restore; final image reran the electronic test above.
Initial source/image/capture are preserved, not overwritten. No new visual
observation claimed; prior physical LCD/touch confirmation remains qualified.

## Automated checks / reproduction

Host68/68 PASS. Actual array/profile initializer+restore ASan+UBSan PASS. Host
starts from differing latency/write/burst fields and verifies200M configuration,
X8 use/restore and exact original profile recovery. Tests cover both modeled MR4
upper-byte side effects, ignored device writes/readback failure, timeout, wrong
clock, duplicate/no-claim, unexpected foreign mode change and shared sticky guard.
Five mutants rejected: wrong latency, skip MR4, ignore readback, skip original
restoration, allow continued use after foreign profile change. C++98 header PASS.
Oracle uses the exact pinned vendor field struct to verify30/20/4B/0B constants;
source SHA2563c2f7bdd3224bf33e08d8fb65729c9b08e02f8e086f4c091c7e1e3f033cca1ac.
Native strict runtime gate permits project objects+libgcc only. Late comments,
host capture tests/docs changed after target; native implementation unchanged.

Build existing MPLL/input-lifecycle/framebuffer prerequisites plus
`MAIA_NATIVE_PROBE_PSRAM_PROFILE=ON`, standalone target `maia_sched_probe_ram`.
Convert RAM ELF with esptool and run the hardware runner with
`--psram-profile --capture-seconds 30`. This electronic profile requires no owner
visual observation. Full artifacts/source hashes/config/commands/checks and desktop
logs are in `../maia-rtos-recovery/native-psram-profile-20261116/`.

Next grouped work: native LDO2/supply sequencing and PSRAM reset/power-on entry,
then exercise this initializer against actual device defaults. Current private
XTAL20 handoff and register-read protocol still assume a powered/readable device.
Cold entry, full32MiB capacity, voltage/temperature/delayline margins, SMP, other
clients and complete native desktop acceptance remain open. Differing mode entry
is HOST-TESTED only; never describe the already-matching physical run as cold boot.
