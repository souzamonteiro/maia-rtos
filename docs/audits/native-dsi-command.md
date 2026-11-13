# Native DSI command-host configuration — 2026-11-13

**HARDWARE-TESTED, LIMITED.** Configure the already-native960Mbps/two-lane host
for command mode with checked SDK timing parameters. No packet/header/payload,
panel DCS command, framebuffer/video/PSRAM operation or native desktop claim.

## Primary sources and attribution

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[esp_lcd_mipi_dsi_bus.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c)
`esp_lcd_new_dsi_bus` command-mode setup after PLL lock/stop; constants timeout10MHz,
escape target18MHz (integer divisor yields20MHz for actual byte clock120MHz).
[Host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h)
mode/clock/ECC/CRC/EoTp/divider/count helpers and
[PHY LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_phy_ll.h)
switch/read/stop timing helpers.
[Host register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_host_reg.h)
provide offsets/masks, including CMD_PKT_STATUS+74 empty bits0/2/4, full bits1/3/5,
read busy6. Require low7==15 before configuration and every step.

The [chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
10.2.2 p616/regs10.11–10.16 pp651–659 remain previous reference clock prerequisites.
Host timing facts come from pinned SDK register/LL definitions where TRM omits
these host fields. No guessed addresses, readiness flags or timing guarantees.

`dsi_command_program.h` ports SDK parameters with Espressif Apache-2.0 attribution
and `LICENSES/Apache-2.0.txt`. Native guard/state/report code original MIT. No SDK
headers/runtime/task/locks or compatibility layer. Native ordering sets dividers
early under proved idle/no-transaction conditions; LP clock request already0 and
is preserved/checked. The table is not a copied SDK runtime initialization call.

## Contract and parameter table

`maia_esp32p4_dsi_command_config` in existing SoC DSI module requires successful
internal native PHY claim, chip1.3/core0/IRQs-off/exclusive pre-scheduler ownership,
stable board supplies/inherited SPLL, unchanged reference routing and calibrated
VO3, clocked inactive VDMA, powered PHY two-lane/lock+all-stop, no HS request,
bridge/DPI/FIFOs empty. No externally fabricated report authorizes this operation.
Guard core context before MMIO, root/access state before peripheral MMIO.

| Index | Host offset | Mask | Value | Purpose |
|---|---|---|---|---|
|0|34|1|1|Command mode|
|1|08|FFFF|0C06|Timeout divisor12, escape divisor6|
|2|9C|03FF03FF|00320068|Data HS->LP50/LP->HS104 byte cycles|
|3|98|03FF03FF|002E0080|Clock HS->LP46/LP->HS128 byte cycles|
|4|2C|39|19|RX CRC/ECC, HS-EoTp on, LP-EoTp off|
|5|78|FFFFFFFF|0|HS TX/LP RX timeout counts|
|6..10|7C/80/84/88/8C|FFFF|0|HS/LP read/write and BTA timeout counts|
|11|F4|7FFF|1770|Max read time6000|
|12|A4|FF00|3F00|Stop-wait63, lane count preserved|
|13|94|3|0|Keep LP clock/no HS request|

Offsets/masks/values hexadecimal unless explicitly described in decimal. Nominal
byte clock960/8=120MHz ->escape20MHz/timeout10MHz. These are computed from previous
nominal PHY configuration, not physically measured frequencies. SDK counts0 disable
hardware timeout mechanisms; read-time6000 must not be advertised as a bounded
read-response guarantee. No transaction/read API or unbounded wait is introduced.
Future transfers must add separately bounded native completion/failure handling.
CMD_MODE_CFG packet-specific LP/HS selection is left for that transfer API.

Read before words, verify all14 complete words and fresh RCC/provider/divider/LP/
eFuse/PMU/VDMA/PHY controls/lock+stop/FIFO guard before each step. Mask preserving
unrelated bits, skip matching words, full readback, update expected PHY_IF word
when stop-wait changes. Fresh final guard plus status/FIFO capture. No packet,
IRQ acknowledgment, root/power/PLL/PHY test-port/bridge/DPI/DMA or panel writes.

Report0:none/1:guarded/2..15:completed table steps/16:verified, conservative attempted
mask (bits0..13), before/after full words, status/FIFO and modeled rates. Null
INVALID/context CONTEXT; missing claim/incompatible initial state BUSY; unstable
fresh guard VERIFY/write-free. Once guarded, failure sticky FAILED until reset,
no rollback/retry. Duplicate success BUSY without MMIO. All loops finite14-element
checks; no waiting. Not a runtime reconfiguration/teardown or ownership manager.

## Verification procedure

Host AppleClang21/Python3.14.5, native GCC14.2 RV32IMAC/ILP32. New fixture executes
real native access/reference/PHY first, then checks all exact masks/preserved bits,
13 significant writes (LPCLK already0), every dropped write, all-matching/no-op,
fresh root/supply/table drift, PLL loss and each FIFO full/empty/read-busy bit.
Capture checks full ordered14 rows, exact preserved masks/dividers/timers/rates,
attempted mask derived from actual changes, prior native lane word and FIFO state.

```sh
cmake -S . -B /tmp/maia-dsi-command-host-20261113 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-dsi-command-host-20261113 -j4
ctest --test-dir /tmp/maia-dsi-command-host-20261113 --output-on-failure
cmake -S . -B /tmp/maia-native-dsi-command-20261113 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON -DMAIA_NATIVE_PROBE_VO3_CYCLE=ON -DMAIA_NATIVE_PROBE_DSI_ACCESS=ON -DMAIA_NATIVE_PROBE_DSI_REFERENCE=ON -DMAIA_NATIVE_PROBE_DSI_PHY=ON -DMAIA_NATIVE_PROBE_DSI_COMMAND=ON
cmake --build /tmp/maia-native-dsi-command-20261113 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-command-20261113/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-command-20261113/maia_sched_probe_ram.bin --log /tmp/maia-dsi-command-hardware.log --dsi-command --capture-seconds 15
```

Opt-in command profile requires native PHY/reference/access/VO3/park/gate. Device
probe samples16 stable configured words/PLL/FIFOs/supply/root state and duplicate
BUSY. Runner resets original flash in finally; paced maia.py verifies preserved
shell/SciCalc14/desktop. No flash/SD write, parser edit or default production change.

## Actual results — 2026-11-13

[Hardware capture](native-dsi-command-hardware.txt),
[desktop check](native-dsi-command-desktop.txt),
[manifest](native-dsi-command-manifest.json). Tab5 ESP32-P4 rev1.3/40MHz; ROM RAM
load followed by reset to preserved flash. Tracked logs normalize line endings/
trailing whitespace; raw logs remain in recovery. No failed hardware candidate.

All native predecessor park/gate/VO3/access/reference/PHY profiles pass. Command
config stage16/mask181E: **six actual writes**. MODE already1, LPCLK0, timeout
counts0 unchanged. Changed CLKMGR0->0C06, DATA_TMR0->00320068, CLK_TMR0->002E0080,
PCKHDL0->19, MAX_RD0->1770, PHY_IF1->3F01 (lane count1 preserved). All14 full
before/after words captured; matching words skipped. Derived escape20MHz/timeout
10MHz are nominal software rates, not measured frequencies.

PHY_STATUS15BD retains lock/all-stop. CMD_PKT_STATUS00050015: low7==15 proves
generic command/write/read FIFOs empty, not full or read-busy. Upper flags are
observed but not interpreted as part of generic-interface acceptance. Sixteen
later samples preserve all14 settings, PHY power/reset/status, root clock/reset,
VO3/eFuse and disabled bridge/DPI/empty bridge FIFO; duplicate BUSY. No packet
header/payload, HS request or panel commands sent. Hardware timeout counts remain
zero by this setup contract; no transfer API or unbounded waiting exists here.
Prior VO3 host windows30.016/30.018ms remain observation-only, not voltage proof.

Host34/34 PASS; Clang ASan/UBSan PASS; strict C++98 public header PASS. Four compiled
mutants (wrong escape divisor, ignored read-busy flag, clobbered unmasked bits,
retry after programming failure) abort as expected. Invalid command-without-PHY
CMake profile rejects. Capture negatives cover malformed/duplicate/missing rows,
wrong attempted mask/rates/lanes/dividers and busy FIFOs. Native ELF runtime gate
PASS (project objects+libgcc only, no unresolved/foreign runtime); image/entry
confined to L2_LOW. Existing RWX/empty-removed-flash-segment linker warnings remain.

Paced maia.py verifies shell echo, SciCalc `2+3*4=14`, module exit0 and desktop
left active. Installed desktop remains preserved FreeRTOS baseline; this native
RAM diagnostic is not a replacement desktop firmware. No flash/SD writes.
Recovery `../maia-rtos-recovery/native-dsi-command-20261113/` contains source/image/
ELF/map, raw logs, sanitizer/mutation scripts/results, licenses and pinned primary
references; SDK files verified byte-for-byte against pinned revision before hashing.

Implemented: native command mode/dividers/timing and generic idle-FIFO guards,
checked writes/no-op handling, tests/real target evidence. Not implemented: packet-
specific LP selection/virtual-channel setup and bounded header/payload transfers,
BTA/read/ACK handling, panel reset/DCS startup, video/framebuffer/PSRAM/native
desktop, physical clock/PHY signal-integrity measurements or runtime teardown.
Next small task: bounded native LP short-write transfer with explicit error and
completion handling; distinguish host FIFO completion from panel acknowledgment.
