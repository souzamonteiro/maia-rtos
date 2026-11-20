# Native PSRAM supply-to-mode boot path — 2026-11-16

HOST-TESTED / NATIVE-BUILT. New supply/device startup and integrated LCD/touch
execution **NOT RUN on target**. Owner explicitly instructed proceeding with the
manually synchronized power/button test marked untested. No manual timing request,
serial connection, firmware load/reset, flash or SD operation was performed here.
Do not promote this path to cold-start or complete driver acceptance.

## Result and ownership

`maia_esp32p4_psram_boot_initialize200` now composes native supply, clock, MPLL
calibration and AP HEX mode programming. It starts from the observed ROM-entry
class: accessible idle controllers, closed AXI, XTAL core-div1/bus-div4 and VO2
enable clear, rev1.3/valid dedicated1900mV eFuse pair. The prior ROM snapshot is
input evidence, not a claim about uninterrupted physical power loss. Other roots
and model-only initial analog/divider values are not invented factory defaults.

The sequence checks exclusive boot/core0/IRQs-off/internal SRAM ownership, CPU1
parked and all memory/cache/DMA/analog/MPLL/F25/F50 clients stopped. It gates
PSRAM core/PLL then MPLL output/circuit; configures/enables calibrated VO2; enables
the analog master clock, releases held reset, selects its vendor160M clock;
stops calibration, enables MPLL circuit/output, restores XTAL core and bus-div2
for nominal20MHz. No gated controller/master accesses occur. It then runs native
DHREF/RSTB/divisor400M calibration with observed END0->1 and initializes/readbacks
MR0/MR4/MR8 for200M/X16. Register fields/source pins are in
[EVIDENCE](../hardware/EVIDENCE.md#native-off-supply-boot-composition--2026-11-16-host-tested).

This is boot-lifetime ownership. It never restores a disabled supply or returns
to old-firmware clocks. Warm XTAL handoff/adoption remain distinct. Only a private
boot claim permits an initially different MPLL divisor or a readable X8 device
mode; warm guards still demand the known400M/X16 recovery profile. No caller
report/boolean can manufacture the claim. Fast MPLL selection is blocked before
native calibration; parking the consumers invalidates that readiness again.

All stores are guarded/read back, unrelated fields retained, shared-root changes
refused. Supply handoff accepts only its two intended root changes. Attempted
programming failure stays sticky in the owning layer; later-phase failure also
poisons the composed initializer. Invalid/context/duplicate calls refuse without
MMIO. No unsafe rollback, supply-off/reset pulse, guessed discharge/settling delay,
allocation or hard real-time/WCET promise. Readable device protocol and the wider
ROM/CPU clock/cache prerequisites remain necessary. Digital readback/calibration
alone does not establish voltage, electrical margins or readable memory.

## Files and integrated path

- `soc_psram_boot.c` exposes the composed initializer/report in the public SoC
  header. `soc_psram_clock.c` owns boot gating/clock state; `soc_psram_supply.c`
  reuses the shared strict trim validator; `soc_psram_mpll.c` and
  `soc_psram_array.c` distinguish native boot from warm recovery.
- `native_dpi_framebuffer.h`/CMake add an explicit boot profile preceding the
  existing PSRAM200/window/DMA/LCD/input pipeline. Bootstrap mode is retained;
  teardown releases users and restores the saved4KiB/native controller/window
  baseline, keeping supply, XTAL20 and configured device modes owned until reset.
  It does not claim to restore undefined pre-power-on contents or off-supply state.
- Runner `--psram-native-boot` has a separate strict boot validator, refuses warm
  supply/profile/park combinations and retains all downstream integrity/IRQ/task
  gates. Early phase failure is reported without fabricating MPLL progress.
- `AGENTS.md`, status/handoff/open questions record the owner's manual-test waiver.
  Parser/EBNF/MaiaCC, app API, fallback firmware and storage remain unchanged.

## Automated validation

Host77/77 PASS. New boot integration links actual snapshot/clock/supply/MPLL and
composition, with profile completion stubbed; the separate real array/profile
fixture covers actual native mode transactions including X8->X16 boot entry and
strict warm refusal. Closed AXI, other analog roots/dividers and device contents
in these fixtures are models, not hardware measurements. Tests cover both
disabled/enabled master/circuit/output entry states, unknown native versus warm
divider, missing trim/revision/error, active controller/AXI/incorrect source,
every modeled value-changing dropped store, foreign root changes at every store,
mode failure, bounded timeout, duplicate/context guards and sticky refusal.

Actual boot, array/profile and MPLL implementations pass ASan+UBSan under `-Os`.
Six mutants rejected: omit core gate, wrong trim, skip analog-source setup,
allow fast clock before calibration, relax warm divider, skip mode initialization.
C++98 public header and seven exact pinned primary-source hashes PASS.
Future-output envelope tests are explicitly synthetic host models using prior
warm downstream output; they are never saved/presented as native boot UART data.

Both native boot and old warm diagnostic profiles build with runtime gate PASS:
project objects+libgcc, no unresolved/foreign runtime symbols. Size optimization
initially exposed compiler-generated memcpy/memset for automatic const arrays/
zero-initialized reports. Native tables now have static storage and transfer
reports use bounded explicit clearing; no libc was linked. Warm build initially
exceeded SRAM by32bytes; size optimization of its MPLL code resolves this without
changing any memory boundary/stack size. Failure logs retained. Final cold-profile
BSS4FF23B20/stacktop4FF27B20; warm BSS4FF23E20/stacktop4FF27E20,160KiB window and
16KiB boot stack unchanged. Bounds validation ran with `--validate-only`, no port.

Prepared native boot RAM image99840bytes, SHA256
**45a2589c643d1118519a0330e4c8ad26ed04aa654524bf634571bf2e61bd187c**.
Standalone GCC14.2 RV32IMAC/ILP32, esptool4.12. Source/image/ELFs/map/config/commands/
host/sanitizer/mutant/source-pin/build-failure evidence in
`../maia-rtos-recovery/native-psram-native-boot-20261116/`.

## Unexecuted physical acceptance and continuation

The manual power/button timing test remains NOT RUN, explicitly waived as a
development blocker. New native power/device startup and its complete PSRAM/LCD/
touch pipeline also remain NOT RUN. No elapsed time or host result substitutes
for those facts. Existing prior physical LCD/touch evidence remains about its
original warm profile/image.

On 2026-11-18, the current native boot + framebuffer/LCD/touch RAM image
(SHA-256 `bac487ed97e925944e8c7a5c9077f7bf3b839ee98235890e73a24c73b0b58ff5`)
reached the cache bootstrap, which passed, then `boot_initialize200` returned
`MAIA_PSRAM_BUSY` at stage0 with zero boot writes. The framebuffer path did not
run; the runner reset to flash and the owner confirmed the original desktop.
This is a safe refusal, not an initialization pass.

A separate read-only 54-sample snapshot after a normal reset reported VO2
register `0x501151d0 = 0x40200180`, with enable bit `0x100` set. That is
incompatible with the native ROM-entry guard, which requires VO2 disabled, and
is consistent with the refusal. The exact register snapshot at the failed call
was not printed, so this is not claimed as a complete call-time diagnosis. The
reset was not a physical power cycle and proves no cold-start behavior.
Evidence and both images/logs are preserved in
`../../../maia-rtos-recovery/native-psram-product-startup-refusal-20261118/`.

For a future automatic target run from a suitable ROM state, configure the prior
LCD/input/profile prerequisites plus `MAIA_NATIVE_PROBE_PSRAM_BOOT=ON` and
`MAIA_NATIVE_PROBE_PSRAM_PARK=OFF`, build `maia_sched_probe_ram`/elf2image, then
runner `--psram-native-boot --capture-seconds 30`. It must refuse a powered warm
supply, not cycle VO2 to manufacture the entry. Keep original firmware recovery;
do not request another chat-synchronized button sequence. An unsuitable state
is an unexecuted/failed acceptance case, never a reason to weaken guards.

Continue remaining memory/driver work with this path explicitly unqualified.
Full32MiB capacity, electrical/timing margins, fully independent CPU/cache startup,
SMP/shared clients, native desktop and the rest of the drivers remain open.

## Order correction and target result — 2026-11-19

The diagnostic now calls `maia_esp32p4_psram_boot_initialize200` before
`maia_esp32p4_psram_cache_boot`; the host capture validator requires that order.
The RAM-loaded probe now always selects `linker.ld`, independently of the
product XIP linker option. With the default product-XIP option enabled, the
97,760-byte ESP32-P4 image passed `--validate-only` and the native runtime-symbol
gate (project objects + libgcc; no foreign or unresolved symbols). The
diagnostic translation unit uses `-Os` to fit the 160 KiB RAM-load window; a
test-only freestanding `memcpy` resolves compiler-generated copies without
linking libc.

The 2026-11-19 warm-reset run on the Tab5 passed the preceding MIPI/VO3, DSI,
ST7121/panel and DPI bridge checks, reached `DPI_FB_READY`, then printed
`PSRAM_BOOT result=fffffffb stage=00000000 writes=00000000`
(`MAIA_PSRAM_BUSY`, `-5`). It emitted no `DPI_FB_CACHE` marker and did not reach
PSRAM memory transfer, DMA, LCD streaming or touch validation. The runner
returned `FAIL / NOT DEMONSTRATED` and invoked its reset to the preserved flash
application. This is a safe entry-guard refusal, not a cold-start pass.

The run used a normal warm reset, not a physical power cycle, and did not print
a call-time register snapshot. The separate prior warm-reset VO2 observation is
consistent with this refusal but does not prove which predicate was decisive at
the call. Keep the guards intact; cold PSRAM startup and the integrated
framebuffer pipeline remain unqualified. Final default-option image SHA-256:
`903234a96dbbb00a6458490e399c3fb52850786f016e45f820e0ba9cb67a0ecc`;
capture SHA-256:
`09d01cb73c0280f171a5aa567e10e814d219d39a51c9d46a1df6fe18fe865c21`.
The final ELF, RAM image, map, CMake cache and UART capture are preserved in
`../../../maia-rtos-recovery/native-psram-native-boot-order-20261119/default-product-option/`.

## Entry guard diagnosis — 2026-11-20

The boot report now retains the read-only entry snapshot, the AXI word used by
the entry predicate, the conditional MPLL busy sample, and a named
`failed_guards` bitmask. The runner prints these observations on a separate
`PSRAM_BOOT_ENTRY` line; early invalid/context/duplicate paths still perform no
MMIO. Host tests exercise every hardware-entry guard, both software ownership
guards, the prior-boot guard and the MPLL-busy guard.

The updated 99,008-byte RAM image passed the native runtime-symbol gate and
runner `--validate-only`. On the Tab5, the automatic warm-reset run again
returned `MAIA_PSRAM_BUSY` (`-5`), stage 0, zero boot writes. This capture
identifies two rejected predicates: `VO2_ENABLED` (`0x100`) and
`DIVIDER_NOT_XTAL` (`0x400`), combined as `guards=0x500`. The stable sampled
values were VO2 `0x40200180` and `root1=0x0000D00C`; the recorded divider mask
was `0x00FF3000`. AXI quiet, controller idle/configuration, and eFuse predicates
did not fail. No cache, PSRAM data, DMA, LCD stream or touch work followed.

This proves why this warm-reset entry is refused; it does not establish a
successful cold start or justify adopting this powered state as a cold boot.
No flash image was written, no physical power cycle was performed, and the
runner reset to the preserved flash application. The image and raw UART capture
are preserved in
`../../../maia-rtos-recovery/native-psram-entry-diagnostics-20261120/`.
Image SHA-256:
`67ccae80de64d55fc5ec9340ac71d4248c2f78045700271e3f735b36178e06ac`;
capture SHA-256:
`f2dcd2494f89227e6c96486534cd17bf10b1eaf56b1ad624c7694ee198391607`.
