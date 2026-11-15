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

Follow-up UQ-014/UQ-015/UQ-016: [native DSI reference](../audits/native-dsi-reference.md)
passes actual reference-gate enable under unchanged matching inherited SPLL divider24
and source0/provider on. Nominal20MHz is computed, not measured; LP gate bits do
not establish PLL lock/stability, which remains a caller boot prerequisite. Cold
SPLL startup/power/calibration and actual nonzero source switching are unproven on
target. Mux supplies both config/reference: new native path gates config around
source changes and avoids DSI reads while off (host-tested branch). Next PHY PLL/
test-port programming, bounded lock/lane waits; no full display/PSRAM proof yet.

Follow-up UQ-014/UQ-015/UQ-016: [native PHY960](../audits/native-dsi-phy.md) now
programs actual two-lane PLL/test interface and observes lock+clock/data0/data1 stop
on Tab5 (STATUS15BD,7 bounded observations,16 stable later samples). This closes
only boot PHY bring-up at modeled960Mbps/reference20MHz; not physical frequency,
signal integrity, all rates, cold SPLL startup or runtime teardown. No packet/HS/
video request or panel initialization yet. Next command-mode clocks/timing/bounded
LP FIFO transfer, then panel DCS/video/framebuffer/PSRAM. Desktop remains baseline.

Follow-up UQ-014/UQ-015/UQ-016: [native command-host config](../audits/native-dsi-command.md)
now sets/checks actual command mode, dividers and timing on Tab5 with locked/stopped
PHY and idle generic FIFOs. No packet sent; successful setup does not prove panel
acceptance or bounded response. Counts0 disable hardware timeouts; future transfer
API must provide bounded software completion/errors, LP/VC selection and explicit
ACK/read policy. Next native LP short writes, then panel reset/DCS startup and
video/framebuffer/PSRAM. Nominal clock arithmetic remains unmeasured physically.

Follow-up UQ-014/UQ-015/UQ-016: [native LP DCS short transport](../audits/native-dsi-short.md)
now attempts16 actual VC0 NOPs with busy witnessed, bounded drain/stop and zero
observed errors on Tab5. No ACK requested: this does not establish panel receipt,
read response or display initialization. Header publication has no safe readback;
fast completion and silently ignored store can look alike without stronger evidence.
Error status reads may consume latches; sole-owner OR accumulation/sticky failure
prevents blind clear-and-retry. Next long payload path then panel-specific startup;
read/BTA/ACK policy, cold panel identity/reset integration and physical waveforms
remain unverified. Do not infer successful panel configuration from host FIFO idle.

Follow-up UQ-014/UQ-015/UQ-016: [native long transport](../audits/native-dsi-long.md)
now stages one actual NOP word then WC1/DCS39 header on Tab5, records busy and
requires front+internal buffers empty. First10015 observation motivated stronger
F007F=50015 completion; final retest passes, no ACK/panel reception inferred.
Multiword/lengths2..64 remain host-tested only;64 is policy, not FIFO capacity.

- [ ] Verify generic interface/internal payload FIFO depths and maximum safe
  pre-header staging for this chip revision from primary documentation and a
  separately scoped target fixture. Never infer capacity from full/empty flags,
  silently split DCS payloads or import SDK's unbounded prefill waits. Current
  full-before-complete request times out/poisons shared transport before header.
- [ ] Add bounded BTA/read response handling and establish panel identity/response
  before panel-specific reset/DCS startup and full video/framebuffer/PSRAM claims.

Follow-up UQ-014/UQ-015/UQ-016: [bounded RDDID/BTA](../audits/native-dsi-read.md)
API is HOST-TESTED, but two target loads reject ST0bit20 DPHY_ERRORS_4 at stage6
with RX data ready (FIFO50005), PHY15BD, no RX pop. The panel ID is UNREAD.
- [ ] Verify bit20 meaning and latch semantics specifically for ESP32-P4 rev1.3.
  Another vendor's identically named bit suggests Lane0 LP1 contention only; this
  is not P4 evidence. No DSI entry found in consulted Espressif errata summary.
- [ ] Distinguish BTA/turnaround configuration, retained panel state and electrical
  conditions from historical/consuming error latches. Preserve captures; do not
  mask/clear/ignore flags to make a success. No blind FIFO drain or retry.
- [ ] Verify four bounded error-free RDDID responses and host-control restoration
  on target before closing read gate. MRPS3/one FIFO word does not prove exact
  response length or model. Earlier short/long evidence remains host-completion
  evidence only. Panel startup/video/framebuffer/PSRAM still pending.

Follow-up UQ-014/UQ-015/UQ-016: [escape divider correction](../audits/native-dsi-bta-escape.md)
closes the bounded error-free response/restoration gate in the tested warm boot
conditions: two final loads/four responses each,00FBA080/errors0/preserved1.
Divider6->7 is the only functional setting change, based on official rounded
18MHz-target policy. No error flag ignored. Physical mechanism/P4 error20 detailed
meaning remain open; no scope/frequency measurement. Exact RX length/model cannot
be inferred from the stripped FIFO word. Next native quiet reset plus verified
variant-specific DCS startup; video/framebuffer/PSRAM/cold power remain pending.

Follow-up UQ-014/UQ-015/UQ-016: [quiet reset+DSI](../audits/native-dsi-reset.md)
now proves warm-entry reset under native MIPI gate+VO3, touch recovery and bounded
error-free responses afterwards (two final loads). Older standalone reset evidence
remains historically qualified. No cold supplies/pulse-voltage/native pixels claim.
Observed touch firmware1 selects ST7121 in preserved working BSP; firmware3 selects
ST7123. Audit the selected ST7121 startup table/driver provenance before vendor
commands; never substitute the ST7123 custom table or guess from DSI raw response.
Next bounded startup/delays, then video/framebuffer/PSRAM. Full desktop still legacy.

Follow-up UQ-014/UQ-015/UQ-016: [ST7121 program](../audits/native-st7121-program.md)
now sends the selected board recipe natively after quiet reset, including actual
61byte long packet; two final loads/all31 commands+1150ms waits+postRDDID pass.
First90015 refusal showed internal full is not generic-interface no-space: pinned
SDK uses front fullbit3. Completion still requires both buffer levels empty. Max
FIFO capacity/other lengths remain unproven. Opaque recipe bytes retain original
source semantics/lengths/delays; no undocumented register meaning fabricated.
- [ ] Read bounded panel state/configuration to prove effects of sleep-out/display
  commands, rather than infer them from host drain or unchanged RDDID.
- [ ] Native DPI video/framebuffer/PSRAM/cold startup still pending; no native
  desktop equivalence. Firmware1->ST7121 selection stays BSP policy.

Follow-up UQ-014/UQ-015/UQ-016: [RDDPM power readback](../audits/native-panel-power.md)
closes the limited sleep/display-state evidence gate: two final loads/four stable
reads per phase,08->9C/bit4 SleepOut0->1/bit2 DisplayOn0->1 after native program.
No register writes added by this read diagnostic, no assumed exact silicon suffix.
Color format/vendor-register/image output still unverified. Next audit matched
panel/DPI format, clock/timing and inactive DMA before video/framebuffer/PSRAM.
Cold power/full native desktop remain pending. No exact RX framing/length proof.

Follow-up UQ-014/UQ-015/UQ-016: [inactive DPI stage](../audits/native-dpi-stage.md)
now verifies19 host/bridge format/timing words with DMA/output/clock inactive on
two loads, retaining bounded panel-power reads. BSPrequest70MHz produces modeled
80MHz via pinnedPLL240/div3; use actual divider arithmetic when computing timing.
This is not running-clock or active-video proof. Pending before activation:
- [ ] Verify panel-format/readback contract against working host RGB565; do not
  infer transport format from lcd_dev_config.bits_per_pixel24 alone.
- [ ] Verify PLL240 source/provider readiness and enable DPI clock under inactive
  bridge/output/DMA; current route223 was already matching (switch host-only).
- [ ] Establish configuration update/latch, active video/ACK policy, DMA/memory/
  cache/framebuffer/PSRAM and actual pixels; do not treat staged readback as output.

Follow-up UQ-014/UQ-015/UQ-016: [format and clock](../audits/native-dpi-clock.md)
now reads initial00, selects3A55 via M5GFX-supported policy, verifies DBI color05,
enables DPI gate under private staged claim, and retains color05/power9C after it
(two final loads). Source/provider logical gates checked; no physical clock measure
or cold PLL startup. Actual provider240 was alreadyon; enable-off branch host-only.
- [ ] Verify actual wire format/pixels with guarded native test pattern: reported
  DBI16bpp does not establish pixel packet acceptance. ExactP suffix unknown;
  supplierP datatype table lists24-bit, while working BSP/M5GFX useRGB565.
- [ ] Establish latch/output/ACK strategy while DMA inactive, then memory/cache/
  framebuffer/PSRAM. No register-only visual claim. Full desktop remains legacy.

### Host-generated bars visible; bridge path remains unproven — 2026-11-14

Owner confirmed stable vertical bars after the native RAM test explicitly drove
GPIO22 HIGH. First attempt without illumination was black despite correct host
controls. See [pattern audit](../audits/native-dpi-pattern.md). Host VPG bypasses
bridge; next verify guarded config latch with bridge/output/DMA inactive, then
owned SRAM DMA-fed pixels. Backlight fixture is temporary DC HIGH, not production
PWM. Wire RGB565 from framebuffer, PSRAM, cold clocks and full native desktop
remain separate gates. No measured frequency/color accuracy claim.

Orientation acceptance: owner saw horizontal bars. Production software selects
LV_DISPLAY_ROTATION_90; validate physical direction and input mapping with an
asymmetric labeled native framebuffer pattern before desktop acceptance. Do not
replace panel720x1280 timings with logical1280x720 solely to rotate the UI.

### Bridge update requested; live application and DMA-fed pixels pending — 2026-11-14

[Native bridge preparation](../audits/native-dpi-bridge.md) verifies EN1 with
output/DMA off and continuing power9C replies. DPI_UPDATE+44 is WT: publication has
no readable completion ACK and a dropped trigger cannot be detected here. Verify
live timing/pixel path by actual transfer later. First specify source memory,
cache/DMA ownership, descriptors and bridge flow controls; stage inactive first.
Full720x1280 RGB565 frame1,843,200bytes exceeds the tested RAM-image window. Do not
assume PSRAM or unverified SRAM. Consider verified region capacity or a documented
repeating source. Preserve logical1280x720 rotation/input contract and horizontal
bar observation. Full native desktop/cold PLL/POSIX remain pending.

### Finite repeated-tile descriptors built; fetch/flow still pending — 2026-11-15

[Tile/list audit](../audits/native-dpi-dma-tile.md) verifies CPU-only construction
and readback in owned SRAM. Next stage channel/LLP/peripheral handshakes/bridge
flow controls inactive, then actual fetch, finite stop and visible pixels. Confirm
engine valid/LAST/LLP termination semantics despite SDK marker-helper shadow-only
comment versus its use in DPI linked-list setup. Address destination50105000.
No engine has accessed the tile/list; writeback/lifetime/stop/cache ownership must
be established before enabling. Full framebuffer/PSRAM/rotation/input/native apps
remain separate. No source/header/runtime FreeRTOS dependency introduced.

### Channel staging verified; solid transfer and optional-register behavior open — 2026-11-15

[Stage audit](../audits/native-dpi-dma-stage.md): reading50081148 optional destination
status address causes load fault on this device; source/destination status-address
registers excluded and fetch/writeback disabled. Explain SDK RW versus silicon
behavior separately; no need to access these unused optional registers now.
AWLEN requested16 reads15; final explicit15 verified, root cause/actual bus beats
unmeasured. Earlier tile LLI encoding with field16 is not an executed transfer.
Bridge count4bits cannot encode160 by SDK direct-count contract: initial channel
plan is one full-frame contiguous block with fixed8-byte source, no LLP. Do not
activate160LLIs directly. Next design guarded activation/update/underrun/completion
and early stop with source lifetime, then prove solid color pixels. PSRAM, bandwidth,
full framebuffer and production rotation/input remain independent gates.

### DMA frame complete; visual refresh and counter documentation open — 2026-11-15

[Frame audit](../audits/native-dpi-dma-frame.md): independent byte-verified8/64-bit
SRAM copies show raw completion counterN-1 on this board despite TRM/SDK count
wording. Explain revision/documentation applicability before generalizing. Display
accepts only activity + natural disable + DMA-done + exact qualified raw count +
FIFO drain/no latched errors; exposes normalized items separately. Single frame
completed/recovered, visible red unconfirmed. Next bounded refresh/color/underrun
qualification (existing discard threshold413 masks some startup-line reporting),
then full framebuffer/PSRAM and rotation/input acceptance. WT publication has no
ACK; dropped off-update/already-off disable may be unobservable. Source remains
immutable and one-shot claim consumed; continuous streaming not implemented.

### Solid red confirmed; spatial framebuffer acceptance next — 2026-11-15

[Bounded refresh](../audits/native-dpi-dma-refresh.md) now confirms stable red
visually after readiness handshake and400 individually completed/recovered frames.
No negative visual conclusion from the earlier brief test: user was not in front.
Remaining: owned finite SRAM pixel tile with source increment/spatial-color check,
then PSRAM/full framebuffer and native desktop/rotation-input. Diagnostic batch
uses boot exclusivity/IRQs off, not production scheduling or a continuous stream.
Prior counter documentation discrepancy, WT no-ACK and underrun threshold413
limits remain open. Do not activate unused160LLIs as one bridge frame.

### Fixed-source footprint and writable LLI lifecycle qualified in SRAM — 2026-11-15

[List audit](../audits/native-vdma-list.md): automatic ARLEN and source-fixed64bit
read8correct+24A5 per32byte group; explicit ARLEN_EN1/field0 yields exact8-byte
repetition on this board. Do not assume source-fixed alone limits memory footprint.
Earlier solid stage/image still automatic ARLEN; next display use must explicitly
qualify this source-length field/owned span. Existing red observation remains valid,
containing tile was owned and first many pixels red. LLI engine writes VALID0,
per-block count/completion even with SSTAT/DSTAT fetch0; maintain writable/exclusive
list and verify exact permitted writes before bounded rearm. General burst encoding
and hardware revision applicability remain open. Spatial pixels/PSRAM/full native
desktop still untested. Existing160-LLI bridge count limit unchanged.

### Five-block display accepted; incremental strip visual proof pending — 2026-11-15

[Spatial audit](../audits/native-dpi-dma-spatial.md) confirms400 finite frames,
five exact writable LLI completion records/rawE0FF/drain, no recorded errors,
source/tail/guards/recovery intact. Owner saw four large stable bands, did not
identify the narrow strip at the native display edge. Do not infer all incremental
pixels were visible. Next move marker toward center and increase thickness within
checked owned SRAM, preserving timing/native geometry/production rotation, then
fresh readiness/visual test. PSRAM/full framebuffer/native desktop and production
scheduler acceptance remain separate. Automatic-ARLEN legacy source footprint,
threshold413/WT no-ACK and counter documentation discrepancy remain open; spatial
profile explicitly constrains source ARLEN and uses only five finite descriptors.

### Central marker visually confirmed after clarification — 2026-11-15

[Central audit](../audits/native-dpi-dma-center.md): wider32-row marker at624–655,
400 finite frames, source/tail/guards/LLIs/cleanup all PASS. Owner first chose fault/
other and described small pieces; after explaining expected32x180colored segments,
confirmed stable/aligned center. Preserve both answers, not a repaired defect.
Qualitative incremental marker accepted; no instrumented all-pixel/tear-free/
production rotation-input acceptance. Next audit native PSRAM initialization and
cache/DMA ownership for full framebuffer before any register writes. Actual full
framebuffer/PSRAM/native desktop remain pending. Existing WT no-ACK/threshold413/
counter-doc and legacy solid automatic-ARLEN footprint limitations remain.
