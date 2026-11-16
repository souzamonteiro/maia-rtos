# Native powered-domain MPLL400 initialization — 2026-11-16

HARDWARE-TESTED, LIMITED. MaiaRTOS now cycles the MPLL circuit power, programs
its fixed400MHz profile through native analog-I2C, observes not-done then done,
and restores routing before qualifying PSRAM/DMA/LCD. No FreeRTOS/SDK runtime,
headers, PLL helper, allocator or scheduler shim is linked. This does not yet
initialize the PSRAM supply/device from arbitrary power-on state.

## Port and ownership

Native subset of pinned ESP-IDFf5c3654 `rtc_clk_mpll_configure`,
`clk_ll_mpll_set_config`, `clk_ll_mpll_enable/disable`, calibration LL and analog
read/write patch. Apache-2.0 attribution retained. [Primary source ledger and
register facts](../hardware/EVIDENCE.md#native-mpll-programming-and-power-cycle--2026-11-16).

The boot-only function requires genuine native XTAL20 ownership, actual matching
PSRAM clock/mux words, both controllers idle and AXI closed; all code/data internal
SRAM and other analog/MPLL consumers stopped (including F25/F50 derived clients).
VO2 supply, analog-master clock and initial known400MHz recovery reference must
already be valid. No supply-voltage/PSRAM-device-mode or flash write occurs.

Native sequence snapshots analog regs1/2/3, cycles only PMU_MSPI_PHY_XPD, rereads
power-on analog configuration, clears CAL_STOP, writes DHREF mask30, pulses RSTB
bit5 low/high and writes divisor99. It verifies command echo, relevant readback,
not-done/done transition, other roots/controller configuration, CAL_STOP and exact
routing restoration. Unknown reference or any post-claim failure remains sticky;
there is no blind retry or unsafe rollback while ownership is uncertain. Shared
failure state also prevents the older read-only MPLL path from bypassing refusal.
Individual waits are bounded by caller budget1..100000, not measured WCET.

## Target result

[Unedited successful UART](native-mpll-initialize-hardware.txt), Tab5
ESP32-P4rev1.3 `/dev/cu.usbmodem1301`, standalone GCC14.2 RV32IMAC/ILP32/esptool4.12.
Image SHA256 **f27fcb3fae416f5ca07a0b68625047a260e128a017c4d78cd34d159b47973da1**,
97776bytes. Native RAM load/run/reset only, no flash/SD writes.

- Init result0/stage5, four analog writes,29polls, routing restored1,
  saw_not_done1; calibration34C before/after. Analog1=31->31,2=99->99,
  post-power analog3=50->70. Calculated frequency400MHz, not a physical clock
  measurement. Actual power-cycle and register defaults distinguish this from
  merely reading an already-calibrated divider.
- Subsequent native PSRAM200MHz/DQS/bothDLL setup passes full1843200byte image
  hashE3D71BEF and surrounding2MiB-window guards;9byte-exact4KiB DMA address/burst
  cases pass.102LCDframes/actualDMAIRQs/posts, no controller/error/timeout/spurious
  flags, four cache publications/presentations.
- Native input/bus three stop/revalidate/restart cycles plus final stop/close pass:
  295polls/301bus calls, errors/contentions0, no physical interaction required.
 1568ticks/1569timerIRQs/1460heartbeat iterations and CPU-load0411F701; all four
  other tasks exited before diagnostic memory teardown.
- Actual active-stop/drain/source release/exact IRQ route, saved4KiB/mode4B/
  clocks/MMU and fourRDDPM9C replies restored. Scratch data beyond saved4KiB is
  deliberately not restored. Native/strict host PASS.
- Final reset original desktop responsive, shell echo/SciCalc2+3*4=14/moduleexit0,
  desktop left active. Baseline SHA256 remains
  21bfb79ab701bbab592f8f95e26dd7b47896c1ca483bf7892fdc1d5719fb72dd.

Two earlier attempts are preserved, not accepted. Both programmed/read back the
four analog writes but never observed END0 while the original MPLL remained
powered, so they refused at stage3 without releasing uncertain ownership. The
second added immediate CAL sampling and still refused. The successful change
ports the documented MPLL circuit disable/enable sequence, without switching off
the PSRAM LDO2 supply or relaxing the required END0->END1 gate. Recovery siblings:
`native-mpll-init-refusal-20261116` and
`native-mpll-init-immediate-refusal-20261116` contain exact images/patches/logs.

## Checks and limits

Host67/67 PASS; actual initializer ASan+UBSan PASS. Host covers both initial END
states, all23 dropped MMIO write positions, wrong source/rate/context, stale-high
END, stuck calibration/analog engine and sticky refusal. Five mutants rejected:
omit power-cycle, wrong divider, accept stale END, skip routing restore and skip
command echo. C++98 header PASS; independent official-header oracle and six
primary source byte hashes match pinned SDK. Actual target runtime gate permits
project objects+libgcc only. Late test/capture/documentation changes after target
leave native implementation/image unchanged.

The image initially exceeded the existing low-SRAM link window by496bytes and
was rejected. Only the MPLL diagnostic's two large task stacks were reduced from
8KiB to6KiB each; no reserved memory limit or context/stack guard was weakened.
The final target run passes normal native context/stack checks. Arbitrary client
callbacks still require their own stack sizing; no general stack-usage bound.

Reproduce using existing framebuffer/stream/touch/input-lifecycle prerequisites
plus `MAIA_NATIVE_PROBE_MPLL_INITIALIZE=ON`, build only `maia_sched_probe_ram`,
convert its RAM ELF with standalone esptool and run the hardware runner with
`--mpll-initialize --capture-seconds 30`. Source snapshots, images/map/config,
commands/checks/oracle and desktop logs are preserved in
`../maia-rtos-recovery/native-mpll-initialize-20261116/`.

Next grouped work: native PSRAM MR0/MR4/MR8 latency/interface configuration and
supply/device startup from documented defaults. Current entry still requires a
known powered PSRAM/supply and native XTAL20 handoff; it does not prove cold
board boot, full32MiB allocation, complete delayline/voltage/temperature margins,
SMP, other active MPLL clients or the complete native desktop. Prior physical
touch/LCD acceptance is retained; no new visual observation claimed here.
