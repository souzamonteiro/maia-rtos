# Inherited MIPI/DW-GDMA state — 2026-11-12

Read-only prerequisite to native shutdown; **HARDWARE-TESTED, LIMITED**. This is
an opt-in RAM diagnostic, not a display driver, shutdown or idle certificate.
No default/production firmware path changes. No parser or FreeRTOS API imports.

## Reviewed defects before implementation

1. ROM RAM entry leaves MIPI clock enables set; it does not establish peripheral
   shutdown. The previous VO3 probe proved this, not active DMA transactions.
2. Power/reset requires consumer quiescence. Clock bits alone do not identify
   DMA channels, FIFO residue, outstanding transactions or PHY lane state.
3. SDK deletion routines depend on live SDK allocation/interrupt/lock objects.
   Those objects cannot be reused from the overwritten former application.
4. Raw abort has an unbounded wait in the reference LL. It must not be copied
   into a bounded native shutdown implementation.

## Pinned primary sources and observed contracts

Reference revision: ESP-IDF **f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0** (local
owner tree). Exact file hashes/HEAD equality will be retained in the manifest.
Primary URLs use this immutable revision under
https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/ .

- [esp_lcd_panel_dpi.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c): `dpi_panel_create_dma_link`
  allocates a memory -> DSI DW-GDMA channel (hardware handshake); callbacks
  re-arm linked lists. `dpi_panel_del` disables DPI clock and bridge then deletes
  DMA resources; not a raw inherited-state drain protocol.
- [esp_lcd_mipi_dsi_bus.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c): `esp_lcd_del_dsi_bus` disables
  PHY reference/config clocks and DSI bus clock separately. It does not perform
  DPI panel/DMA deletion or prove lane stop.
- [dw_gdma.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_hw_support/dma/dw_gdma.c): `channel_destroy`,
  `channel_unregister_from_group`, `dw_gdma_release_group_handle` free/release
  resources; the last group deinitializes HAL/gates shared DMA clocks. Therefore
  a channel deletion is not a standalone hardware idle/drain proof.
- [dw_gdma_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h): bus clock check requires
  **both** CPU bit13 and SYS bit5; four channels; HW handshake DSI0/CSI1/ISP2;
  suspend does not guarantee transaction completion; abort polls without bound.
- [esp32p4.peripherals.ld](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/ld/esp32p4.peripherals.ld): DW_GDMA **0x50081000**,
  CSI host **0x5009F000**, bridge **0x5009F800**, DSI host **0x500A0000**,
  bridge **0x500A0800**. Use linker symbol as DMA base authority: the generated
  `dw_gdma_reg.h` references DR_REG_DMAC_BASE not defined by this SDK's reg_base.
- [hp_sys_clkrst_reg.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/hp_sys_clkrst_reg.h): SOC_CLK_CTRL0+14,
  SOC_CLK_CTRL1+18, PERI_CLK_CTRL03+3C. SYS bits12/11/10 gate DSI/CSI host/bridge.
- Register headers `mipi_dsi_bridge_reg.h` EN+04, DPI_MISC+40, FIFO_STATUS+14;
  `mipi_dsi_host_reg.h` PWR+04, MODE+34, PHY_RSTZ+A0, PHY_STATUS+B0;
  `mipi_csi_host_reg.h` RESETN+08, SHUTDOWNZ+40, DPHY_RSTZ+44, STOPSTATE+4C;
  `mipi_csi_bridge_reg.h` CSI_EN+04. These reads do not acknowledge/clear IRQs.
- `dw_gdma_reg.h`: CFG+10, CHEN0+18, CHEN1+1C; four channels at+100/+200/
  +300/+400, CFG0+20, CFG1+24, STATUS0+30 (completed data), STATUS1+34 (FIFO),
  SAR0+00, DAR0+08, LLP0+28. Reads do not follow SAR/DAR/LLP addresses.
  `dw_gdma_struct.h`/LL confirm live layout. CHEN bit n is enable; bit16+n
  suspend; CHEN1 bit n abort. Status1 bits14:0 FIFO; CFG1 bits8:7 src handshake,
  bits13:12 dst handshake, bit3/4 software handshake selection. These fields
  alone do not establish channel ownership or actual bus idle.

This implementation is original MIT diagnostic code using register facts; no
vendor function bodies, vendor headers/runtime or SDK object handles imported.
SDK references retain their Espressif Apache-2.0 notices in recovery snapshots.

## Diagnostic contract and validation

`MAIA_NATIVE_PROBE_MIPI=ON` is separate from every existing diagnostic workload.
Boot core0/MIE-off check, eight sequential samples, only bounded read loops.
Each sample reads clocks first; each peripheral domain is read only when its bus
clock enable is observed. Both DMA gates required. Skipped fields remain zero,
with validity bits distinguishing absence from measured zero. Full clock words
are rechecked after each sample. Changed clocks fail, but this cannot detect an
ABA change or make sequential register observations atomic. Exclusive stable
boot access is required. No API for runtime concurrent clients is introduced.

Output is strict schema1: eight SAMPLE/DSI/CSI/DMA records and32 CH records.
Runner checks completeness, exact keys/values, duplicate/missing/malformed data,
validity/gate consistency, skipped fields zero and clock recheck. PASS means
complete observations with stable clock words, **never** that the display/DMA is
inactive or it is safe to power cycle. No register, clock, LDO, reset, descriptor,
PSRAM, flash or SD writes by the diagnostic. Existing RAM loader/startup/serial
I/O still use their own established register/memory accesses.

Host fixture supplies only a read callback, checks a fixed address whitelist,
each domain/half-enabled-DMA combination, all four channel addresses, no repeats
and changed-clock rejection. Native gate must link only project code + libgcc.
Host24/24 CTest and ASan/UBSan fixture PASS. Three compiled mutations are
rejected (missing DMA CPU gate, wrong channel stride, ignored clock recheck);
two mixed CMake profiles reject. Standalone GCC14.2 build and runtime gate PASS
(project objects + libgcc only), image6848B entirely L2MEM_LOW, entry4FF00100.
Existing linker RWX/removed-empty-segment warnings remain; not changed here.

## Actual Tab5 observation and limits

ESP32-P4 rev1.3 /40MHz XTAL, `/dev/cu.usbmodem1301`, RAM-only run PASS. All eight
clock words and observations were identical across samples:

| Measurement | Observed value / interpretation |
|---|---|
| CPU/SYS/PHY clock words | E6DFB7FF /0C0D5678 /000002B3 |
| Validity | 0D: DSI, CSI bridge and DW-GDMA read; CSI host skipped |
| DSI bridge EN /DPI config /FIFO | 0 /19D0 (dpi_en bit0 clear) /0 |
| DSI host POWER /MODE /PHY_RSTZ /PHY_STATUS | 0 /1 (command) /0 /1528 |
| CSI bridge EN | 0 |
| DW-GDMA CFG /CHEN0 /CHEN1 | 0 /0 /0 |
| Four channels CFG0 | all0 |
| Channel0..3 CFG1 | 6001B /4001B /2001B /1B |
| Four channels STATUS0 /STATUS1 /SAR0 /DAR0 /LLP0 | all0 |

Thus active clock gates did **not** imply enabled DMA or an enabled DSI host/
bridge in this observed entry state. DMA global enable0 and channel-enable0,
FIFO counts0 and zero pointers are consistent with a reset/disabled controller,
but do not prove absence of outstanding AXI traffic or an ordered shutdown.
No inherited active-channel ownership can be inferred from these reset-like
CFG values. DSI PHY reset/shutdown controls are0: do not interpret lane-status
bits as a live, valid stop-state handshake in that condition. CSI host bus clock
bit11 is0: its zero output fields are **unmeasured**, not asserted reset/idle.
CSI bridge bus bit10 and DSI bit12 remain1. Do not clear them solely from these
snapshots; actual reset/AXI/PHY completion policy still needs a native contract.
No active-stream shutdown or cold-display claim is made.

After automatic reset, paced maia.py verifies shell echo, SciCalc `2+3*4=14`,
module return0 and desktop active: PASS. No flash/SD, LDO, panel reset or MIPI/DMA
register write by the diagnostic. Eight samples are sequential, without claimed
timing/latency bounds. Physical waveform/pixels were not inspected.

[Target capture](native-mipi-state-hardware.txt),
[desktop check](native-mipi-state-desktop.txt),
[artifact/reference manifest](native-mipi-state-manifest.json). Full source,
build, test logs, mutation evidence and reference snapshots are retained at
`../maia-rtos-recovery/native-mipi-state-20261112/`.

```sh
cmake -S . -B /tmp/maia-native-mipi-20261112 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON
cmake --build /tmp/maia-native-mipi-20261112 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-mipi-20261112/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-mipi-20261112/maia_sched_probe_ram.bin --log /tmp/maia-mipi-hardware.log --mipi-state --capture-seconds 12
cmake -S . -B /tmp/maia-mipi-host-20261112
cmake --build /tmp/maia-mipi-host-20261112 -j4
ctest --test-dir /tmp/maia-mipi-host-20261112 --output-on-failure
```

## Next gate

Interpret actual channel/bridge/host observations against the pinned sources.
Then design bounded ordered shutdown with explicit partial-failure behavior and
proof of completion; determine shared-channel ownership, DMA/AXI outstanding
status and CSI/DSI PHY stop conditions before clock gating. Do not equate clearing
CHEN, FIFO zero or a static SAR with completed in-flight transactions. Do not
turn off other consumers or clear clocks merely to pass VO3 guards. Native
physical VO3 cycle and guarded panel reset remain NOT RUN until then.
