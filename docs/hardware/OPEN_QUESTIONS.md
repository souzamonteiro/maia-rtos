# Hardware unknowns — status register

- [x] **UQ-001 (Silicon & PCB Revision — RESOLVED on hardware):** Physical M5Stack Tab5 unit confirmed via `esptool.py flash_id` as **ESP32-P4 revision v1.3** (`ESP32-P4NRW32`, 32 MB in-package Octal PSRAM, MAC `30:ed:a0:e3:6c:2b`, 16 MB SPI Flash `0x184020`, USB VID:PID `303a:1001` on `/dev/cu.usbmodem1301`).
- [ ] **UQ-002 (Register and CLIC audit reopened):** Previous blanket verification was incorrect: native SYSTIMER base was I2S0 (`0x500C6000`), and target-0 interrupt routing was off by one. Official v5.4.2 map establishes SYSTIMER `0x500E2000` and source 53/map `0x500D60D4`. Correcting both initially delivered one real IRQ. The subsequent fresh-frame INT correction restores MIL=0 and passes four 250-IRQ/251-switch RAM runs; see `../audits/native-clic-return.md`. Full context ABI, FP and SMP still require validation; do not restore this blanket verification checkbox. See `EVIDENCE.md` and `../audits/native-scheduler-probe.md`. Other historical peripheral/CSR claims need individual evidence, not this closed checkbox.
- [x] **UQ-003 (ROM Image Format & Direct RAM/Flash Load — RESOLVED & HARDWARE-TESTED):** Confirmed `esptool.py --chip esp32p4 elf2image` (`chip_id = 0x0012`) + `esptool.py load_ram` loads single-segment bare-metal ELF images directly into `L2MEM_LOW` (`0x4FF00000..0x4FF28000`, below the ROM reserved window `0x4FF296B8..0x4FF40000`) and jumps to `_start` (`0x4FF00000`) without touching SPI Flash.
- [x] **UQ-004 (Console & Debug Routing on Tab5 — RESOLVED & HARDWARE-TESTED):** Confirmed M5Stack Tab5 USB Type-C port connects directly to the ESP32-P4 internal `USB-Serial/JTAG` controller (`0x500D2000`, `EP1_REG=0x500D2000`, `EP1_CONF_REG=0x500D2004`). Dual-sink/source driver in `soc/espressif/esp32p4/soc_console.c` verified with bidirectional interactive `MaiaShell` I/O on `/dev/cu.usbmodem1301`.
- [ ] **UQ-005 (Executable Memory Matrix):** Exact instruction-fetch (`X`) permissions and cache mapping windows for mapped SPI Flash (XIP) and 32 MB Octal PSRAM, plus required `fence.i` and L1/L2 cache writeback/invalidation instructions before executing dynamically loaded `.mobj` code (`L2MEM_LOW` execution at `0x4FF00000` is now `HARDWARE-TESTED`).
- [ ] **UQ-006 (Secondary Core Bring-Up & IPI):** Exact register sequence in `HP_SYS_CLKRST` (`HP_SYS_CLKRST_SOC_CLK_CTRL0_REG` bit 4 `CORE1_CPU_CLK_EN`, `HP_SYS_CLKRST_HP_RST_EN0_REG` bit 8 `RST_EN_CORE1_GLOBAL`) and `ets_set_appcpu_boot_addr` to boot HP Core 1 and trigger cross-core Software Interrupts (IPI) via CLIC.
- [ ] **UQ-007 (Cache, PSRAM & DMA Coherency):** Octal PSRAM initialization sequence (MSPI controller + cache MMU mapping) and DMA alignment/cacheline constraints. Preserved sdkconfig sets L2 lines to128B (L1 64B); native cache configuration must be established independently. ROM writeback address/signature/mask verified, coherency policy still open; see ../audits/display-baseline-audit.md.
- [ ] **UQ-008 (Tab5 Peripheral Power & Reset Sequencing):** Exact `PI4IOE5V6408` I2C0 (GPIO31/GPIO32) expander I2C addresses and bit masks for LCD backlight, MIPI-DSI bridge (`ILI9881C` / `ST7123`), touch (`GT911` / `ST7123`), audio (`ES8388` / `ES7210`), and SD card slot power rails.
- [x] **UQ-009 (Toolchain Installation & Verification — RESOLVED):** Verified `riscv32-esp-elf-gcc` (`esp-14.2.0_20241119`, `14.2.0`, SHA-256 `2bc6b93e97fbe3b5662655bafab65377a84c2850fff1f2abda97abeb91f5690b`) and `esptool.py` (`v4.12.0`, SHA-256 `8ec1e4d5736e97248bc0eac713d94fba5a002b725d74d9d4c850d570e50ded5a`) in `/Users/roberto/.espressif` and configured `cmake/toolchains/riscv32-esp32p4-elf.cmake`.
- [x] **UQ-010 (Hardware Recovery Path — RESOLVED):** Verified non-destructive RAM execution (`esptool.py load_ram`) and automatic USB-Serial/JTAG reset (`default_reset`) on `/dev/cu.usbmodem1301`, plus early watchdog disarm (`TIMG0_WDT`, `TIMG1_WDT`, `LP_WDT`, `SWD`) in `soc/espressif/esp32p4/soc_early_init.c`.

- [ ] **UQ-011 (Native display parity):** Legacy boot selects ST7121 from touch firmware1; safe reset-release rule and active requested timings are documented in `../audits/display-baseline-audit.md`. Independently verify native bounded I2C identification/timebase, LDO/calibration, reset stages, cold PSRAM initialization, DMA-visible allocation/reservations, cache publication, refresh ownership and initialization failures. Native display remains NOT RUN in this audit; do not reuse fixed bands overlapping the diagnostic ROM-reserved interval.

- [ ] **UQ-012 (Native system-I2C runtime/cold-state contract):** Boot-exclusive GPIO/SYSTIMER transport and warm-reset status4/firmware1 reads are HARDWARE-TESTED, LIMITED; see `../audits/native-touch-identify.md`. Define runtime owner/locking and timer snapshot serialization before task use; prove cold supply/reset readiness and checked bit-preserving expander writes separately. Physical held-line and frozen-timebase tests are NOT RUN. Read-only identification at Idle is device-tested, not permission for arbitrary display commands or a complete ST7121 electrical specification.

- [ ] **UQ-013 (Native board reset sequence):** M5Unified PI4IO configuration model port and bounded8-bit-pointer I2C are verified for boot-only config reads and same-value OUT05 writes on both banks; see `../audits/native-expander-port.md`. Superseding checkpoint: `../audits/native-panel-reset.md` validates warm-boot P4/P5 register sequence, safe LCD direction/pull-up configuration and post-reset touch firmware1. Intermediate/failure states are host-tested. Physical waveform/voltage and cold supply sequencing remain open before a full display claim; LDO/DSI/PSRAM/DMA are separate gates.

- [ ] **UQ-014 (Native VO3 acquisition and power behavior):** Read-only chip1.3/block0.3 trims and native2500mV planner are verified; `../audits/native-ldo3-calibration.md`. Actual codes13/3 match an already-enabled inherited rail; no native power-write/ownership proof. Follow-up `../audits/native-ldo3-acquire.md` implements boot reservation/conflict rejection/masked configure and sticky failures; active matching adoption is hardware-tested, off programming/failures host-tested. Physical off->on is pending consumer quiesce (UQ-015). Physical voltage/settling/current-load/cold-boot behavior remain unmeasured; protect other rails/PMU fields.

- [ ] **UQ-015 (Inherited active MIPI/DMA state):** Native acquisition probe reads active MIPI clock enables after ROM load_ram: system0x0C0D5678, PHY0x2B3. RAM entry does not establish cold peripheral state or stopped DMA. Port ordered DSI/CSI/DMA quiesce with bounded failure/idle checks from primary SDK references before clearing clocks or toggling VO3. Do not reuse stale SDK object handles or infer native display initialization from a retained image. Native LDO active adoption is write-free; off->on hardware test remains pending this gate.

Follow-up to UQ-015: [native read-only MIPI state](../audits/native-mipi-state.md)
observes disabled DW-GDMA/global/channels, DSI host/bridge/DPI despite active
clocks on this ROM entry. CSI host skipped (bus gate off). This narrows the
unknown; it does not resolve ordered shutdown/AXI drain/PHY completion or justify
clock/power writes. No active-stream stop test has run.

Follow-up to UQ-015: [native VDMA stop](../audits/native-vdma-stop.md) uses TRM
v0.4 DMAC_CFG0 global EN completion (p349), with bounded channel-disable requests
and no clock/reset/abort writes. Target empty-controller arm/stop passes;
active-transfer stop is HOST-TESTED only. Need native DMA-visible memory/cache
fixture and actual traffic test, then PHY/bridge completion before clock gating.
This does not establish the whole MIPI shutdown or retire VO3/panel-reset guards.

Follow-up UQ-015: [active SRAM VDMA stop](../audits/native-vdma-transfer.md) now
hardware-tests channel0 single-block M2M: full32KiB copy, then genuine active
stop with1152B partial result, intact boundaries and stable post-stop memory.
Native memory/cache handling is fixture-specific (NOLOAD reservation/NC alias/
pinned ROM range operation), not a general allocator or PSRAM ownership solution.
DSI/CSI PHY/bridge stop, peripheral handshakes and clock/power gating remain open.

- [ ] **UQ-016 (Live MIPI/ISP shutdown vs boot parking):** TRM chip1.3 v0.4
  38.7.2 p1974 requires DMA completion, ISP reset and CSI host reset to stop RX.
  Shared ISP ownership/reset and active DSI/CSI/PHY drain/settling remain open.
  [Native boot reset parking](../audits/native-mipi-park.md) only accepts already-
  shutdown hosts/analog PHYs and inactive VDMA, with fresh checks and four digital
  control clears. Actual digital arm/park PASS; no live stream/analog shutdown
  test. CSI host clock was temporarily enabled for real inspection; no longer
  infer its reset state from unmeasured zeros. Verify boot ISP/consumer state
  before checked clock gating/power tests; do not relax existing VO3/reset guards.

Follow-up UQ-015/UQ-016: [native reset-state clock gate](../audits/native-mipi-gate.md)
PASS holds shared ISP/CSI/DSI resets and clears all guarded MIPI clocks after native
park/VDMA guards. Actual ISP clock is on, but source/input controls are disconnected
(CNTL40002442/CAM4); no clock-on=idle inference. ISP function clock remains on with
reset held. No MIPI/ISP reads after reset/gating. Live-stream/shared runtime ownership,
internal/electrical reset/analog settling and PSRAM remain open. Existing VO3/reset
guards were not bypassed; compose only the verified boot path for subsequent tests.

Follow-up UQ-014/UQ-015/UQ-016: [guarded VO3 cycle](../audits/native-vo3-cycle.md)
now passes actual off-control/native non-adopting acquire on Tab5 after verified
park/gate. This closes only the previously pending enable-path hardware test.
No voltage/discharge/stabilization/load measurement; matching analog/source
steps were no-ops. Cold DSI/PHY/panel/PSRAM and live stream shutdown remain open.
Inherited CSI bus/config clocks and reset bits vary after warm entry; establish
all access/reset prerequisites before reads. An earlier preparation faulted on
CSI RESETN before VO3 writes. Final guarded CSI reset/clock preparation and native
trap vector pass, preserving unrelated ISP reset. Do not assume cold defaults or
claim reset restores every unused peripheral; desktop recovery is functional proof.

Follow-up UQ-014/UQ-015/UQ-016: [native DSI config access](../audits/native-dsi-access.md)
now enables only DSI register/config clocks and releases only DSI bridge reset after
held-reset/gated MIPI and guarded VO3 acquisition. Actual shutdown controls verified;
STATUS1528 is not treated as lane/PLL readiness while PHY reset/shutdown is active.
Need native clock-provider/reference source and verified frequency before PHY PLL,
then bounded lock/stop waits, panel commands and framebuffer/PSRAM. No inference of
cold/native display success or electrical settling from readable host registers.
