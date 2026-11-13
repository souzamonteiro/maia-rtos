# Native reset-state DSI configuration access — 2026-11-12

**HARDWARE-TESTED, LIMITED.** One boot-only native SoC operation opens the DSI
register/configuration interface after gated reset-state MIPI and active calibrated
VO3. This is deliberately separate from reference-clock selection/PLL programming,
analog power-up and lane/video transmission. Production firmware remains unchanged.

## Primary sources and sequence

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0:
[DSI bus constructor](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c)
enables APB/register clock, pulses DSI bridge reset, configures PHY source,
configuration/reference clocks, then calls HAL init and programs PLL. SDK
[mipi_dsi_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_ll.h)
pins DSI SYS bus bit12, PHY configuration bit0, reference bit1, DPI bit7, bridge
reset bit26. Source selection lives separately in RCC peri_clk_ctrl02; this unit
does not select or enable a PLL clock source. Reset defaults/control masks come
from pinned mipi_dsi_host_reg.h/mipi_dsi_bridge_reg.h; addresses from reg_base.h.

[Chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
38.7.2 p1974/34.7.5–7 pp1574–1575/VDMA CFG0 p349 underpin earlier native park/gate
prerequisites, not an invented analog-settling delay. Exact per-file source hashes
and complete test image are retained in the manifest/recovery checkpoint.

The new original MIT native policy consumes a stricter held-reset/gated starting
state than SDK construction. It enables DSI bus and configuration clocks while
reset remains held, then releases only DSI bridge reset. No DSI register access
before full access prerequisites. No SDK imports/headers/runtime/shim, no copied
SDK lock/allocation/task/polling code. Existing Apache-2.0 LDO calibration port is
used unchanged; its attribution/license remain intact.

## Contract and failure handling

Public API: `include/maia/soc/esp32p4_dsi_access.h`, implementation
`soc/espressif/esp32p4/soc_dsi_access.c`, private MMIO seam
`dsi_access_internal.h`. Boot-exclusive chip1.3/core0/IRQs-off, before scheduler;
caller owns DSI/CSI/ISP/RCC/VDMA/PMU and all producers. Successful native VO3
acquisition/stable supply is a caller prerequisite; matching register state is a
guard, not proof of native lifetime ownership, physical voltage or settling.

Requires all MIPI SYS1C00 and PHY93 bits0, shared reset3C000000 held, relevant
reset overrides7800 clear, both DMA clocks on and VDMA global/channels/abort/
reset/FIFOs0. Validates ROM-populated eFuse calibration and active VO3 SW-owned/
source0/bypass0 state with matching dref/mul/VDET. Unmasked analog/current-limit
fields remain unchanged. No ISP/CSI peripheral MMIO behind held resets.

Reads complete initial RCC words; fresh full RCC/VDMA/eFuse/PMU checks precede
each of three masked writes: SYS|1000, PHY|1, reset&~04000000. Full readbacks.
Only after DSI reset release and another guard, samples host version, power+4,
PHY reset+A0, status+B0, bridge+4, DPI+40, FIFO+14. Requires power bit0, PHY bits0..3,
bridge/DPI enable0 and FIFOlow14=0; status/version are observations, with no
invented success constant or PLL-ready claim. Final guard verifies supply and
RCC/VDMA again. Reference/DPI/CSI clocks stay gated; CSI/ISP resets stay held.

Null report INVALID; wrong context CONTEXT without MMIO; incompatible initial
state BUSY; unstable initial fresh snapshot VERIFY/write-free/retryable. After
entering programming, failure sticky FAILED until reset, no rollback/retry. Last
completed stage/conservative attempted write mask returned. Success stage5;
duplicate BUSY without MMIO. No public release, live reconfiguration or runtime
concurrency implementation. No change to existing park/gate/LDO state machines.

## Validation commands and scope

Host macOS AppleClang21/Python3.14.5; target GCC14.2 RV32IMAC/ILP32.

```sh
cmake -S . -B /tmp/maia-dsi-access-host-20261112 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-dsi-access-host-20261112 -j4
ctest --test-dir /tmp/maia-dsi-access-host-20261112 --output-on-failure
cmake -S . -B /tmp/maia-native-dsi-access-20261112 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON -DMAIA_NATIVE_PROBE_VO3_CYCLE=ON -DMAIA_NATIVE_PROBE_DSI_ACCESS=ON
cmake --build /tmp/maia-native-dsi-access-20261112 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-access-20261112/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-access-20261112/maia_sched_probe_ram.bin --log /tmp/maia-dsi-access-hardware.log --dsi-access --capture-seconds 10
```

Opt-in DSI diagnostic requires VO3 cycle profile. Extends actual native park/gate/
VO3 off-on evidence, then calls DSI API and verifies duplicate BUSY. Default
production image/probe profiles unchanged. Host fixture executes real calibration
planner/snapshot, rejects MMIO before required clocks/reset release, verifies
exact three writes and preserved unrelated bits, every fresh guard change,
dropped-write stages/sticky failure, unexpected transmit/FIFO state and duplicate
no-I/O behavior. Capture validator checks exact previous gate/supply state,
transitions, complete records, mode separation and final PASS.

Still pending: native PLL clock-provider/source preparation, PHY test-port/PLL
configuration and bounded lock/stop-state waits; panel commands/video/framebuffer/
PSRAM/full native desktop. No parser/EBNF/MaiaCC changes needed in this unit.

## Actual results and limits — 2026-11-12

[Hardware capture](native-dsi-access-hardware.txt),
[desktop recovery](native-dsi-access-desktop.txt) and
[hash manifest](native-dsi-access-manifest.json). Tab5 ESP32-P4 rev1.3/40MHz,
ROM `load_ram` only; no flash/SD writes. Tracked captures normalize line endings
and trailing whitespace; original raw logs retained in recovery.

Existing park/gate/VO3 cycle completes first. VO3 control40200180/ANA D5800000
restored; host off/on observation24.802/25.022ms has the same non-electrical limits
as the earlier supply audit. Native DSI operation passes stage5/attempt7:

| Register | Before | After | Meaning |
|---|---|---|---|
| RCC SYS | 0C0D4278 | 0C0D5278 | DSI bus only enabled |
| RCC PHY | 00000220 | 00000221 | DSI config only enabled |
| RCC RESET | 3C000040 | 38000040 | Only DSI bridge reset released |
| VO3 CTRL / ANA | 40200180 / D5800000 | unchanged | Register guards retained |

Actual DSI VERSION3134312A, POWER0, PHY_RSTZ0, BRIDGE0, DPI000019D0 (enable0),
FIFO0, PHY_STATUS00001528. Unmasked DPI fields need not be zero. Status/version
are observations only: PHY remains in shutdown/digital reset, so lane/PLL status
must not be interpreted as an initialized link. No PLL/reference/source/test-port/
analog/lane/video/panel writes occurred. CSI/ISP clocks gated and resets held.
Duplicate returns BUSY; fresh final guards confirm complete expected RCC/VDMA and
supply state. No generic clock-release or runtime ownership API is implied.

Host31/31 PASS. New fixture passes Clang ASan/UBSan, including changes in each of
19 RCC/DMA/eFuse/PMU guard words after each of three writes; failure is sticky and
no next write occurs. Four compiled mutants (enable reference clock, release ISP
reset, read host before clocks/reset release, retry failed claim) abort as expected.
Invalid profile without VO3 rejects; public header passes strict C++98. Native
ELF runtime gate PASS (project objects +libgcc, no foreign/unresolved symbols);
image/entry confined to L2_LOW4FF00000..4FF28000, below ROM reservation.
No new parser changes. One local build initially found a reused diagnostic variable
name; corrected before target load. No failed hardware candidate in this unit.

Paced maia.py recovery passes shell echo, SciCalc `2+3*4=14`, module exit0 and
desktop left active. Functional desktop remains the preserved FreeRTOS baseline;
this diagnostic is wholly native but is not a replacement desktop firmware.

Recovery: `../maia-rtos-recovery/native-dsi-access-20261112/` with final source,
image/ELF/map, host/raw target logs, negative scripts/results and pinned references.
Next small task: native DSI PHY reference-source/clock-provider preparation with
verified source frequency before PLL/test-port programming and bounded lock/lane
waits. Cold display/PSRAM/full native desktop and electrical settling remain open.
