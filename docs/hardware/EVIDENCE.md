# Hardware evidence register

## Notification-driven native TX worker — 2026-11-11

Reuses the verified queue, notification/ECALL, SYSTIMER/CLIC and USB TX paths;
no new MMIO, pinout or display initialization fact. Rev-v1.3 RAM evidence:
80 intact payload records/3840 bytes, 302 real IRQs, stable TX polls/run_ticks
through 20 initial and 21 final quiet IRQ observations, then one actual record
published in IRQ context wakes the owner and is captured by the host. BUSY
uses finite retries (240 timeouts); empty queue uses an indefinite kernel wait.
This does not establish a USB TX-ready IRQ, SMP, power/WCET bounds, reconnect
or panel/DMA correctness. [Exact logs and limits](../audits/native-tx-worker-notifications.md).

## Native task notification waits — 2026-11-11

Uses the already verified MIE exclusion, M-mode ECALL/context restore and
SYSTIMER/CLIC IRQ. No new MMIO, route, pinout, clock or display configuration.
Kernel stores completion in TCB and writes a0 only after validating the saved
frame. Rev-v1.3 RAM workload passes 40 rounds/282 waits/80 timeouts, 750 real IRQs,
520 blocked-state observations and 750 rejected IRQ-context waits; idle runs.
FP/mixed lifecycle and exact-PC illegal16 regressions pass. This does not prove
SMP, arbitrary driver IRQ routing, power/WCET bounds or panel/DMA correctness.
[Raw logs, API, race ordering and limits](../audits/native-task-notifications.md).

## Native serial output ownership — 2026-11-11

Reuses EP1/EP1_CONF WR_DONE[0] and SERIAL_IN_EP_DATA_FREE[1], plus UART0
STATUS TXFIFO_CNT[23:16]. Primary SDK register definitions, commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` (v5.4.2 checkout), sections
[USB_SERIAL_JTAG_EP1_CONF_REG / WR_DONE / SERIAL_IN_EP_DATA_FREE](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/usb_serial_jtag_reg.h)
and [UART_STATUS_REG / TXFIFO_CNT](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/uart_reg.h).
These definitions were read locally, not compiled/imported. No new MMIO or pins.
32-byte USB watermark is software policy; UART threshold 120 is inherited
conservative policy. Actual rev-v1.3 RAM USB capture: 80 intact ordered records,
3840 bytes, 260 real IRQs, 344 FULL and 239 BUSY events with correct resume.
This does not establish physical reconnect/UART, SMP or a time bound.
[Exact captures, reproduction and limitations](../audits/native-serial-output.md).

## Native serial input ownership — 2026-11-11

RX reuses existing USB-Serial/JTAG EP1 availability bit2 and data register,
then UART0 STATUS RXFIFO_CNT[7:0] and FIFO. Primary register definitions at SDK
commit `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` (v5.4.2 checkout), sections
[USB_SERIAL_JTAG_EP1_CONF_REG / SERIAL_OUT_EP_DATA_AVAIL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/usb_serial_jtag_reg.h)
and [UART_STATUS_REG / RXFIFO_CNT](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/uart_reg.h).
Local reference headers match this commit (no diff); read only, no SDK header
or runtime linked. Software ownership/batching does not introduce new MMIO.
Actual rev-v1.3 USB RAM test: all 256 byte values in order, full queue observed,
317 real IRQs and zero errors. UART physical wiring/traffic is not revalidated;
its fallback is host-model evidence only. [Capture, initial test failure and
limits](../audits/native-serial-input.md).

## Native queue task/IRQ exclusion — 2026-11-11

Uses the existing single-core CSR MIE exclusion and validated SYSTIMER/CLIC path;
no new MMIO, pin, clock or boot claim. References remain the local-exclusion and
timer/context sections below. The RAM-only diagnostic checks real C IRQ-stack
SP and MIE preservation while a timer-IRQ producer shares a copying queue with
task producers/consumer. Final result: 253 real IRQs, 688 intact ordered packets,
zero errors. This is not an SMP, blocking-wait, driver-equivalence or latency
bound claim. [Exact workload, initial failure, logs and recovery](../audits/native-driver-queues.md).

## Native stack entry protection — 2026-11-11

Primary references: [ESP32-P4 rev-v1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
register 1.11 p.72 (`mscratch`, CSR 0x340 R/W), and
[RISC-V Privileged ISA 20240411, Machine Scratch Register](https://docs.riscv.org/reference/isa/v20240411/_attachments/riscv-privileged.pdf),
section 3.1.13. Scratch CSR swapping provides initial working registers without
writing through the interrupted SP. All metadata layouts, guard patterns and
IRQ/emergency buffers here are project software, not invented MMIO addresses.
Boot bounds come from the existing linker __stack_bottom/__stack_top symbols.
Single-core/non-nested only; this does not establish SMP or memory isolation.
Actual rev-v1.3 RAM evidence: seven bad-stack injections stop on the emergency
stack with exact ELF cause/PC and expected guard/SP; normal lifecycle/integer
and idle handlers independently observe their C SP within the IRQ buffer.
See [stack protection report and raw logs](../audits/native-stack-protection.md)
for final counts, negative tests, recovery and limitations.


## Internal idle and WFI diagnostic — 2026-11-11

Uses the already verified timer/CLIC and M-mode context-return paths. Standard
instruction reference: [RISC-V privileged ISA,20240411, Machine ISA: Wait for Interrupt](https://docs.riscv.org/reference/isa/v20240411/_attachments/riscv-privileged.pdf).
The private idle context keeps MIE enabled, FS off, and uses WFI rather than
reviving blocked/exited applications. The architecture wait hook is instrumented
only in the RAM diagnostic to inspect coherent idle/task/IRQ snapshots before
the same WFI instruction. No interrupt, tick or application wake is synthesized.
No power-saving/latency claim is inferred: WFI may return spuriously.


## Scheduler local exclusion and transactional task traps — 2026-11-11

Primary ISA reference: [Zicsr v2.0, CSR Instructions](https://docs.riscv.org/reference/isa/unpriv/zicsr.html)
(20260120 ISA library), atomic CSRRC[I] read/clear and CSRRS[I] set semantics;
rev-v1.3 TRM mstatus p.68–69 defines MIE bit3. Local critical sections retain
only the old MIE bit and restore it, including nested already-disabled callers.
Compiler memory clobbers bracket accesses. This is single-core exclusion,
not an SMP/DMA/cache synchronization claim. No new MMIO register is introduced.
Dedicated project-owned delay/exit ECALL labels let dispatch commit the state
transition after the trap frame is saved, while IRQs are disabled. The raw
cause/return validation and real timer accounting remain unchanged.


## Controlled fatal traps — 2026-11-11

Primary references: [RISC-V unprivileged ISA, 20240411, C v2.0 sections26.5.4/26.5.6](https://docs.riscv.org/reference/isa/v20240411/_attachments/riscv-unprivileged.pdf)
define all-zero16-bit illegal and C.EBREAK; the already cited rev-v1.3 TRM
register1.13 p.73 defines illegal instruction cause2 and breakpoint cause3,
with mepc/mtval retaining trap details. The controlled32-bit fault writes the
read-only cycle CSR (`CSRRW x0,cycle,x0`), addressed in the ISA CSR-access rules
and the rev-v1.3 CSR summary (p.65: cycle 0xC00 is read-only). Fault-PC labels are deliberately halfword aligned.
The USB fatal sink uses only previously verified USB-Serial/JTAG registers,
with bounded polling/byte count and no display callbacks, heap or scheduler.
Old behavior on hardware prints FAULT_RESUMED after illegal16; the fixed path
reports raw cause/original halfword PC/value/sp and halts, verified by exact
PC comparison to the matching ELF symbol. See [fatal-trap report](../audits/native-fatal-traps.md)
for actual captures, expected failures, hardware scope and reset recovery.


## Mixed FS-state and scheduler diagnostic — 2026-11-11

Uses the previously cited rev-v1.3 mstatus.FS/CLIC references and verified
32-bit M-mode ecall path. No new MMIO or peripheral protocol. Test-only profile
`MAIA_NATIVE_PROBE_MIXED` combines two FP sentinel tasks with a higher-priority
integer task that clears FS, yields while ready, then sleeps two real ticks.
Expected invariants: no lower-priority work while high task yields ready; no
early wake; both lower tasks progress while it sleeps; FS remains off across
switch/ISR paths. Live FP/integer sentinels also span explicit ecalls. Real
IRQ/tick counters are still required; ecalls are reported separately.
Before correction, the probe reached250IRQs but failed with active MIL=0x3f
while a normal task was executing (`mcause=0x0800000b`). The previously cited
fresh-frame INT behavior also applies to saved ecall frames restored from a
timer ISR. The saved return frame now sets INT=1 while preserving saved MPIL;
the raw cause is passed separately to dispatch/statistics before normalization.
No synthetic IRQ/tick is recorded. Final hardware results are required below.


## RV32F context experiment — 2026-11-11

Primary source: [ESP32-P4 Chip Revision v1.3 TRM, Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
section 1.6.3, p.95: 32 single-precision FP registers and manual enablement via
mstatus.FS; FP instructions/CSRs fault with FS off. Register 1.38, p.82: fcsr
contains rounding mode and exception flags. mstatus FS descriptions are on p.68.
The opt-in RAM probe compiles RV32IMAFC/ILP32F (single-precision hardware ABI),
seeds all FP registers with task-distinct bit patterns and fcsr with distinct
legal rounding modes/flags, and checks them before calling C. A separate C
translation unit exercises actual float arguments/result (`2.0f+3.0f`).
Before adding FP context, the hardware test failed after three IRQs with f0
cross-task contamination (`reg=256 expected=1073742080 actual=1056964864`).
The independently authored 272-byte frame preserves all 32 registers plus fcsr.
Two final RV32F runs and an integer-profile regression each pass 250 real IRQs
and 251 switches without register errors. A RAM-only mutant that restores fcsr
as zero fails at `reg=512 expected=82 actual=0`; zero-initialized FP state and
ILP32F argument/result calls are checked too. No double-precision claim.
See [FP context report](../audits/native-fp-context.md) for exact scope and logs.


## Integer context register probe — 2026-11-11

Primary ABI reference: [RISC-V psABI](https://riscv-non-isa.github.io/riscv-elf-psabi-doc/),
version 2.0-rc0, November 04, 2026, sections 1.1 and 2.1: integer register
roles, gp/tp reserved for global/thread pointers, and 16-byte stack alignment.
The diagnostic is an explicitly authored assembly workload that seeds x1,
x4 and x5–x31 with distinct task/register patterns, holds them through a busy
window, then checks them before calling C. x2/sp and x3/gp retain their normal
values and are checked against stack snapshots. On the unchanged trap code the hardware probe failed after three IRQs:
`task=1 reg=4 expected=2986344452 actual=2701131784` (tp). This confirms
cross-task register contamination; interruptions can also occur between the two
instructions that materialize a sentinel constant. Fix under test: use the
existing reserved frame word at byte 124 for tp, zero for a fresh context.
It does not allocate TLS or claim a C/C++ TLS runtime. No new peripheral/MMIO facts are introduced.


## ESP32-P4 fresh-context return investigation — 2026-11-11

Primary source: [ESP32-P4 Chip Revision v1.3 TRM, Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
register 1.13 (`mcause`, p.73), register 1.17 (`mintstatus`, p.75), section
1.9.2.5 (p.113), register 1.84 (p.116). INT bit 31 selects interrupt versus
exception; MPIL is bits 23:16. Interrupt delivery requires a level greater than
both the active MIL and configured threshold. The revision-specific manual,
not the newer generic ESP32-P4 document, matches CSR mintstatus at 0x346.

Measured failing return: `mcause=0x08000000`, `mintstatus=0x3f000000`,
`last_mcause=0xb8000010`, `mstatus=0x88`, enabled pending source. The incoming
fresh task frame uses `mcause=0x38000000` (INT=0); active MIL stays at the
just-serviced level. Controlled experiment: initialize the fresh frame for an
interrupt return (INT=1), keeping MPIL=0 and all other state unchanged. Four corrected runs passed: each delivered 250 real IRQs/ticks and 251 switches,
with both busy tasks progressing and no ecalls/synchronous exceptions. Active
MIL is zero after return (`mintstatus=0`). The final probe also requires enabled
MIE, MIL zero and at least 250 switches. Returning to the former frame reproduces
the one-IRQ failure; the host frame regression rejects it with exit 52.
This silicon-specific behavior is measured, not inferred as a universal CLIC
rule. MIL is read-only and never written. No timer events are fabricated.
Raw captures and exact limits: [CLIC return report](../audits/native-clic-return.md).
Downloaded revision-specific PDF SHA-256:
`ae1fa2a411776760e03329adc3f9a7c13e98c440168cab208f06e3b2d5818830`.

## ESP32-P4 CLIC status diagnostic — 2026-11-11

Primary reference: ESP-IDF v5.4.2, commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`,
[interrupt_clic.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/riscv/include/esp_private/interrupt_clic.h),
ESP32P4 conditional and MINTSTATUS definitions (lines 49–86): this chip uses
CSR **0x346** for mintstatus, with MIL at bits 31:24; interrupt threshold is
memory-mapped. Read-only diagnostic accesses use that documented CSR, not
the different standard-CLIC CSR address.
[Vectors return](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/riscv/vectors.S)
lines 398–405 explain preserving mcause to restore the prior interrupt level
on mret. These sources are consulted, never included/copied/linked.


## Native SYSTIMER base address verification

Primary reference: Espressif ESP-IDF v5.4.2, commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`,
[ESP32-P4 peripheral base map](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/reg_base.h),
PERIPHERAL1 section, `DR_REG_HPPERIPH1_BASE = 0x500c0000` and
`DR_REG_SYSTIMER_BASE = DR_REG_HPPERIPH1_BASE + 0x22000` (line 115).
Thus SYSTIMER is at **0x500e2000**. The former native address `0x500c6000`
is I2S0, not SYSTIMER. Before correcting the base, the on-device RAM probe
read `conf=0 raw=0 ena=0 st=0`, despite initialization, while the interrupt
matrix route read back 16. Capture: `/tmp/maia-native-probe-diagnostics.log`.
The register contract test previously repeated the wrong address; it is being
corrected against the independent vendor map. No vendor runtime is linked.

## Native SYSTIMER routing verification

Primary reference: Espressif ESP-IDF v5.4.2, commit
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`,
[interrupt source enum](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/include/soc/interrupts.h)
(`ETS_SYSTIMER_TARGET0_INTR_SOURCE`) and
[core-0 interrupt matrix register](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/interrupt_core0_reg.h)
(`INTERRUPT_CORE0_SYSTIMER_TARGET0_INT_MAP_REG`, offset `0xd4`). Both reference
files in the local SDK are unchanged from that commit. Compiling the source enum
with the host C compiler prints **53**. It is used only as specification evidence,
not included or linked into the native firmware.

The existing native constants used source 54 and offset `0xd8`. The new isolated
RAM diagnostic, before correcting them, observed `irq=0 ticks=0 ecall=0 sync=0
switches=1 workA=11962368 workB=0` and `MAIA_NATIVE_PREEMPT_FAIL`. It ran two
busy tasks without cooperative yields or synthetic ticks and reset back to the
preserved flash application. No flash write was performed. Raw evidence:
`/tmp/maia-native-probe-before-route.log`. Post-correction evidence is recorded
separately; these negative results are not a preemption claim.

## CST ownership fix — 2026-11-10 (limited device test)

On the one-core experimental firmware, the same small source that previously
faulted in the on-device compiler now compiles, validates and executes with
exit code 32. SciCalc/Alarm and background-job tests also pass over serial.
The change removes temporary CST shared control blocks; it does not change
hardware registers, claim native scheduling/SMP or remove the full foreign
runtime. Original application restored after testing. Exact artifacts, primary
runtime implementation references and limits:
[CST ownership report](../audits/maiascript-cst-ownership-2026-11-10.md).

## Runtime migration experiment — 2026-11-10 (limited scope)

The opt-in one-core legacy-driver profile was built and temporarily installed
on the connected Tab5 by app-only flashing. Serial tests passed scientific
calculations, timer fire/cancel, desktop return and shell background jobs. The
generated configuration selects `CONFIG_FREERTOS_UNICORE=y`; the ELF omits the
secondary-core startup and cross-core tick/IPC functions. It still uses FreeRTOS.
Primary software reference: [ESP-IDF v5.4.2, Single-Core Mode](https://docs.espressif.com/projects/esp-idf/en/v5.4.2/esp32p4/api-reference/system/freertos_idf.html#single-core-mode).

The on-device compiler test failed, including a mutex assertion reproduced on
the original preserved firmware. The experiment was not promoted. The original
application was restored and verified (`2+3*4=14`, return to desktop); no
bootloader/partition-table replacement or new MMIO/pinout implementation occurred.
Exact image hashes, scope and raw-log locations:
[stage-one report](../audits/tab5-singlecore-stage-1-2026-11-10.md).
Historical hardware claims below do not certify this migration complete.

Record each validated hardware claim as:

| ID | Claim | Primary source URL | Revision / section / page | Confirmed on board? | Date |
|---|---|---|---|---|---|
| HW-001 | ESP32-P4 contains a dual-core High-Performance (HP) 32-bit RISC-V CPU system (up to 400 MHz, `RV32IMAFC` with `Zicsr`/`Zifencei`, `misa = 0x40901125`) and a single-core Low-Power (LP) RISC-V CPU, without a conventional virtual-memory MMU for POSIX process isolation | https://www.espressif.com/en/products/socs/esp32-p4 | ESP32-P4 Series Datasheet v0.7 / v1.2, Section "Features" & "CPU and Memory"; verified CSR `misa=0x40901125` and `mhartid=0x00000000` on Tab5 | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-002 | ESP32-P4 HP memory subsystem includes 768 KB internal HP L2MEM (`0x4FF00000..0x4FFC0000`, ROM reserved window `0x4FF296B8..0x4FF40000`), 128 KB HP ROM (`0x4FC00000`), 32 KB LP SRAM (`0x50108000`), and external Flash/PSRAM mapped via Cache/MMU | https://www.espressif.com/en/support/documents/technical-documents | ESP32-P4 TRM v0.7, Chapter "System and Memory" & `esp-idf/components/soc/esp32p4/include/soc/soc.h`; verified bare-metal execution in `L2MEM_LOW` (`0x4FF00000`) | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-003 | ESP32-P4 interrupt architecture uses the RISC-V Core-Local Interrupt Controller (CLIC, base `0x20800000`, `MTVEC` mode `3`, 64-byte aligned `_maia_trap_entry`) where `mret` restores `MPP[29:28]`, `MPIE[27]`, and `MPIL[23:16]` from `mcause` (`0x38000000`) | https://www.espressif.com/en/support/documents/technical-documents | ESP32-P4 TRM v0.7, Chapter "High-Performance CPU" & `esp-idf/components/riscv/vectors.S:398-403`; verified `ebreak` (`mcause=0x30000003`) and `ecall` (`mcause=0x3800000B`) context switching on Tab5 | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-004 | ESP32-P4 boot ROM evaluates strapping pins (GPIO34–GPIO38) on reset and provides direct `USB-Serial/JTAG` download/console on `/dev/cu.usbmodem1301` (`0x500D2000`, `EP1_REG=0x500D2000`, `EP1_CONF_REG=0x500D2004`) and `UART0` (`0x500CA000`) | https://www.espressif.com/en/products/socs/esp32-p4 | `esp-idf/components/soc/esp32p4/register/soc/usb_serial_jtag_reg.h` & `uart_reg.h`; verified interactive TX/RX over `/dev/cu.usbmodem1301` | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-005 | M5Stack Tab5 integrates an ESP32-P4 (`rev v1.3`, `ESP32-P4NRW32`, 32 MB PSRAM) with 16 MB XMC SPI Flash (`XM25QH128C`, Flash ID `0x184020`), USB-C routed to ESP32-P4 internal USB-Serial/JTAG (`303a:1001`), and ROM watchdogs (`TIMG0_WDT=0x500C2048`, `TIMG1_WDT=0x500C3048`, `LP_WDT=0x50116000`, `SWD=0x5011601C`, key `0x50D83AA1`) | https://docs.m5stack.com/en/core/Tab5 | M5Stack Tab5 Docs & `esp-idf/components/soc/esp32p4/register/soc/lp_wdt_reg.h`; verified continuous bare-metal uptime without watchdog reset | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-006 | M5Stack Tab5 physical keyboard accessory (`A164`) communicates over external I2C (`GPIO0`/`GPIO1`) at 7-bit slave address `0x6D` with STM32F030 coprocessor. Mode `0x02` (String/Character Mode in reg `0x10`) returns formatted ASCII string events via register `0x40` (queue length) and register `0x50` (modifier + characters), cleared via register `0x01` (`INT_STA = 0x00`) and register `0x02` (`EVENT_NUM = 0x00`). Named keys (`"backspace"`, `"enter"`, `"space"`, `"tab"`, `"esc"`, `"DEL"`) translated to control characters | https://docs.m5stack.com/en/core/Tab5 | `m5stack/M5Tab5-Keyboard-UserDemo` components/m5_tab5_keyboard_component/src/m5_tab5_keyboard.cpp; verified live key captures from physical keyboard into MaiaShell | YES (`A164 @ 0x6D`) | 2026-11-04 |
| HW-007 | ESP32-P4 clean-room bare-metal MIPI-DSI subsystem drives the M5Stack Tab5 5.0" panel (`720x1280` RGB565) using internal L2MEM SRAM (`0x4FF18000..0x4FF9F000`) without PSRAM dependency. Panel detection distinguishes ST7121 (touch `0x55` firmware `1`) which requires full 28-command DCS vendor init sequence (`0x60`, `0x78`, `0x79`, `0xA4`, `0xB7`, `0xB0`, `0xBF`, `0xA5`, `0xD7`, `0x90`, `0xA3`, `0xA6`, `0xA7`, `0xAC`, `0xAD`, `0xAE`, `0xB2`, `0xE8`, `0x75`, `0xE7`, `0xE1`, `0xEA`, `0xC8`, `0xC9`, `0x60`, `0x11`, `0x29`, `0x35`) and ST7121 DPI timing (`802x1524`, HSA=2, HBP=40, HFP=40, VSA=20, VBP=24, VFP=200). Backlight driven via LEDC PWM (1000 Hz, 10-bit) on GPIO22 | ESP32-P4 TRM Chapter "MIPI-DSI Host & Bridge", "DW_GDMA", "PMU", "LEDC" & `espressif/esp-iot-solution` `esp_lcd_st7121.c` | Verified continuous D-PHY lock, ST7121 vendor DCS sequencing, LEDC PWM backlight, and circular multi-block DMA frame streaming on M5Stack Tab5 hardware | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-008 | ESP32-P4 standalone Flash boot requirements: ESP-IDF 2nd-stage bootloader requires 256-byte `esp_app_desc_t` header (`magic_word = 0xABCD5432`, `min_efuse_blk_rev_full = 0`) at Segment 0 (`0x4FF00000`) and strictly asserts `rom_index == 2` in `unpack_load_app` (requiring two Flash MMU-mapped segments in `SOC_DROM_LOW..SOC_DROM_HIGH`, e.g. `0x40000000` and `0x40010000`). Once verified, bootloader loads `L2MEM_LOW` into SRAM and jumps directly to `0x4FF00100` (`_start`) | `esp-idf/components/bootloader_support/src/bootloader_utility.c:750-785` | Verified standalone cold flash boot from SPI Flash partition `0x10000` surviving USB disconnect and power cycles | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |
| HW-009 | M5Stack Tab5 MicroSD Card slot connects to ESP32-P4 SDMMC host via dedicated 4-bit bus: `CLK=GPIO43`, `CMD=GPIO44`, `D0=GPIO39`, `D1=GPIO40`, `D2=GPIO41`, `D3=GPIO42` (card detection `DET=NC`) | https://github.com/m5stack/M5Tab5-Keyboard-UserDemo/blob/main/components/m5_tab5_component/include/m5tab5_pinmap.h | Verified from official `m5tab5_pinmap.h` and factory app symbol analysis | YES (Pinmap verified) | 2026-11-04 |
| HW-010 | M5Stack Tab5 power sequencing & hardware reset: PI4IOE 0x43 controls `LCD_RST` (Pin 4), `TP_RST` (Pin 5), `CAM_RST` (Pin 6), and `EXT5V_EN` (Pin 2). Requires `IO_DIR=0x7F`, `OUT_HI_Z=0x00`, reset pulse `OUT_STATE=0x44` (reset low, EXT5V high), followed by `OUT_STATE=0x74` (all resets released high, active push-pull). When `TP_RST=1` and `LCD_RST=1`, ST7121 panel and touch controller respond on `SysI2C` at address `0x55` (`fw=1`). LEDC LCD backlight on `GPIO22` operates at 5000 Hz, 12-bit PWM (`duty=4095`) via LEDC Channel 1 (`SIG_LEDC_LS_OUT1_IDX = 127`, timer divider `0x01F4`) | Tab5 factory firmware binary analysis & M5Stack Tab5 BSP | Verified cold boot detection: `[PANEL] Detected ST7121 (touch 0x55 fw=1)` and LEDC channel 1 backlight driving on physical hardware | YES (`ESP32-P4 rev v1.3`) | 2026-11-04 |

---

## Live Hardware Verification Log (M5Stack Tab5 — ESP32-P4 rev v1.3)

- **Date:** `2026-11-04`
- **Target Board:** M5Stack Tab5 (`ESP32-P4NRW32`, Chip Revision `v1.3`, MAC `30:ed:a0:e3:6c:2b`, 16 MB SPI Flash `0x184020`, 32 MB Octal PSRAM)
- **Serial Interface:** `/dev/cu.usbmodem1301` (ESP32-P4 built-in USB-Serial/JTAG controller at `0x500D2000`)
- **Cross-Compiler:** `riscv32-esp-elf-gcc (crosstool-NG esp-14.2.0_20241119) 14.2.0` (`-ffreestanding -nostdlib -march=rv32imafc_zicsr_zifencei -mabi=ilp32f`, SHA-256: `2bc6b93e97fbe3b5662655bafab65377a84c2850fff1f2abda97abeb91f5690b`)
- **Loader Tool:** `esptool.py v4.12.0` (`elf2image` + `load_ram` into `L2MEM_LOW` at `0x4FF00000`, SHA-256: `8ec1e4d5736e97248bc0eac713d94fba5a002b725d74d9d4c850d570e50ded5a`)
- **Runtime Dependencies:** Zero FreeRTOS, zero ESP-IDF runtime, zero Arduino, zero libc.

### Unedited Serial Capture (`/dev/cu.usbmodem1301`)

```text
esptool.py v4.12.0
Creating esp32p4 image...
Merged 1 ELF section
Successfully created esp32p4 image.
Connecting...
Detecting chip type... ESP32-P4
Connected to ESP32-P4 (revision v1.3)
RAM boot...
Downloading 12096 bytes at 4ff00000... done!
All segments done, executing at 4ff00000
=== CAPTURED SERIAL OUTPUT ===

========================================================
MaiaRTOS v0.1.0 — Bare-Metal Kernel & Shell Bring-Up
Target : M5Stack Tab5 (ESP32-P4 rev v1.3)
Runtime: ZERO FreeRTOS / ZERO ESP-IDF / ZERO Arduino
mhartid: 0x00000000  misa: 0x40901125  sp: 0x4FF13620
========================================================
[M1] Trap vector verified (ebreak): sync_exceptions=1 last_mcause=0x30000003 systimer_us=1221096
[M2] Created 3 RTOS tasks (idle=0, telemetry=1, maia_shell=2) — starting scheduler...
[M2/M5] Telemetry task queue verified: rx_seq=3

========================================================
 MaiaRTOS v0.1.0 + MaiaShell Native Execution MVP (N2)
 Board  : M5Stack Tab5 (ESP32-P4 rev v1.3, 40MHz XTAL)
 Kernel : Clean-Room C11/RV32IMAFC (Zero FreeRTOS/IDF)
 Type 'help' for built-in RTOS introspection commands.
========================================================
[AUTO-SELFTEST] Running: uname; ticks; ps; mem; alloc 256; qtest
MaiaRTOS 0.1.0 riscv32-esp32p4 (M5Stack Tab5) [Zero-FreeRTOS/Zero-IDF]
tick_ms=60  systimer_us=1297744  ctx_switches=8
traps: total=6493 timer_irq=0 ecall=6492 last_mcause=0x3800000B
TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME
  0     0  READY  4096      6488     60  idle
  1     2  SLEEP  4096      3     0  telemetry
  2     2  RUN    8192      2     0  maia_shell
L2MEM Region Allocator:
  total=32748B  used=0B  free=32748B  peak=0B  allocs=0  frees=0
alloc OK ptr=0x4FF076A8 size=256 (verified & freed)
qtest PASS: mutex + sem + queue verified (0xCAFEBABE)
msh> help
MaiaShell Native Commands:
  help          - Show this command summary
  uname         - Show MaiaRTOS kernel & SoC architecture info
  uptime        - Show SYSTIMER microseconds & RTOS tick uptime
  ticks         - Show CLIC/SYSTIMER interrupt & trap counters
  ps            - List RTOS tasks (ID, prio, state, switches, ticks)
  mem           - Show L2MEM region allocator statistics
  alloc <bytes> - Allocate, verify & free a block from L2MEM heap
  qtest         - Run live mutex/semaphore/queue self-test
  pwd           - Print working directory
  set K=V       - Set shell variable (use $K in commands)
  env           - List shell variables
  echo ...      - Print arguments
  clear         - Clear terminal screen
msh> set BOARD_TEST=Tab5_Interactive_OK; echo $BOARD_TEST; uptime; ps
Tab5_Interactive_OK
tick_ms=9310  systimer_us=10565931  ctx_switches=9813
traps: total=1017004 timer_irq=0 ecall=1017003 last_mcause=0x3800000B
TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME
  0     0  READY  4096      1012004     9310  idle
  1     2  SLEEP  4096      373     0  telemetry
  2     2  RUN    8192      4627     0  maia_shell
msh> 
=== END CAPTURED OUTPUT (bytes: 2724 ) ===

---

### M5Stack Tab5 Clean-Room MIPI-DSI, Hardware Keyboard & Flash Boot Verification (2026-11-05)

- **Target:** M5Stack Tab5 (`ESP32-P4 rev v1.3`, Chip Revision `v1.3`, MAC `e8:f6:0a:e2:fd:4c`)
- **Firmware:** Clean-Room freestanding RV32IMAFC (Zero FreeRTOS / Zero ESP-IDF / Zero Arduino)
- **SPI Flash:** 16 MB flashed at partition `0x10000` via `esptool.py`

#### Cold Boot Output Capture (Standalone SPI Flash @ 0x10000)

```text
========================================================
MaiaRTOS v0.1.0 — Bare-Metal Kernel & Shell Bring-Up
Target : M5Stack Tab5 (ESP32-P4 rev v1.3)
Runtime: ZERO FreeRTOS / ZERO ESP-IDF / ZERO Arduino
mhartid: 0x00000000  misa: 0x40901125  sp: 0x4FF18B40
========================================================
[I2C] Scanning SysI2C(GPIO31/32): 0x10 0x28 0x32 0x36 0x40 0x41 0x43 0x44 0x55 0x68
[I2C] Scanning ExtI2C(GPIO0/1): 0x6D
[PANEL] Detected ST7121 (touch 0x55 fw=1)
[KBD] Initialized M5Stack Tab5 Keyboard (A164 @ 0x6D on GPIO0/1)
[DSI] D-PHY status: 0x00001529
[DISPLAY] Initializing ST7121 panel commands & timings...
[DISPLAY] Initialized MIPI-DSI 720x1280 (10x128 multi-block DMA in L2MEM)
[M1] Trap vector verified (ebreak): sync_exceptions=1 last_mcause=0x30000003 systimer_us=1786068
[M2] Created 3 RTOS tasks (idle=0, telemetry=1, maia_shell=2) — starting scheduler...
[M2/M5] Telemetry task queue verified: rx_seq=3

========================================================
 MaiaRTOS v0.1.0 + MaiaShell Native Execution MVP (N2)
 Board  : M5Stack Tab5 (ESP32-P4 rev v1.3, 40MHz XTAL)
 Kernel : Clean-Room C11/RV32IMAFC (Zero FreeRTOS/IDF)
 Type 'help' for built-in RTOS introspection commands.
========================================================
[AUTO-SELFTEST] Running: uname; ticks; ps; mem; alloc 256; qtest
MaiaRTOS 0.1.0 riscv32-esp32p4 (M5Stack Tab5) [Zero-FreeRTOS/Zero-IDF]
tick_ms=60  systimer_us=3914628  ctx_switches=8
traps: total=6242 timer_irq=0 ecall=6241 last_mcause=0x3800000B
TID  PRIO  STATE  STACK   SWITCHES  TICKS  NAME
  0     0  READY  4096      6237     60  idle
  1     2  SLEEP  4096      3     0  telemetry
  2     2  RUN    8192      2     0  maia_shell
L2MEM Region Allocator:
  total=32748B  used=0B  free=32748B  peak=0B  allocs=0  frees=0
alloc OK ptr=0x4FF0CBC8 size=256 (verified & freed)
qtest PASS: mutex + sem + queue verified (0xCAFEBABE)
msh> 
--> peek 0x500A0888
0x500A0888 = 0x000000A0  (DSI Bridge multiblk_num = 10, DMA active)
--> peek 0x50081018
0x50081018 = 0x00000001  (DW_GDMA Channel 0 enabled & streaming)
--> backlight 50
Backlight set to 50%
--> backlight 100
Backlight set to 100%
```
```

### Display baseline audit and corrections — 2026-11-11

See [report, primary references and exact sections](../audits/display-baseline-audit.md)
and [source/artifact manifest](../audits/display-baseline-manifest.json). M5Stack
[Tab5 documentation](https://docs.m5stack.com/en/core/Tab5), unversioned, accessed
2026-11-11, PinMap/Tab5 Screen Reset/Version Change: system I2C GPIO31/32,
ST712x touch address0x55, reset P4 high via input pull-up. Requested clocks,
porches and initialization are firmware configuration facts pinned to bf044d3,
not measured waveforms. ESP-IDF f5c3654 ROM linker table + cache.h verify native
writeback symbol address/signature/map; DPI driver/startup explain the legacy
DMA/PSRAM dependencies. Reference files match pinned SDK HEAD.
Actual boot selects ST7121 (FW1), reports32MiB PSRAM/200MHz, CPU360MHz and known
ELF prefix. Canonical host comparison finds30 matching ST7121 table entries.
Source inspection finds extra command-delay differences, native80/960 versus
legacy70/965 requests, and unreserved SRAM bands conflicting with the diagnostic
memory contract. No native display test or runtime import. Legacy shell/SciCalc14
and desktop reverified through maia.py. Logs and limitations are in the report.
This supersedes old blanket display-success claims as evidence of full parity,
and corrects initial shorthand: panel constructors remain in ELF, lv_init absent.

### Boot-only native system-I2C prerequisites — 2026-11-11

[Touch identification source ledger](../audits/native-touch-identify.md) records
primary URLs, revisions and pages: user-supplied Tab5.pdf update2026-11-05 p12
(SDA31/SCL32/address0x55), embedded schematics pp8–10 visually inspected; Sitronix
ST7123 interface protocol v01.11 2023-12-25 pp9/10/15 (16-bit pointer, STOP/START,
timing, firmware/status readiness). ESP32-P4 rev1.3 TRM Pre-release v0.4 ch9,
p552 and ch15 pp1041/1044, plus pinned v5.4.2 GPIO register definitions establish
IO_MUX offsets, GPIO input/output-enable registers and OEN_SEL bit10. The new
boot transport uses low latch + release-by-output-disable; SYSTIMER with bounded
snapshot/progress waits. No voltage, clock waveform, cold panel startup or
multi-task I2C functionality is inferred from those references.

Target result (same report): status4/Idle, firmware1 stable16 reads plus recovery
read; actual0x56 NACK and software-sampled SCL-low TIMEOUT, errors0. No physical
stuck-wire/frozen-timer/cold-panel injection. First strict-status-gate failures
are preserved; final policy accepts Normal/Idle only for read-only firmware ID.
Native ELF gate PASS, host19/19/sanitized fixture/C++98/six mutants PASS. After
RAM-only probe/reset, preserved shell/SciCalc14/desktop verified via maia.py.
No FreeRTOS/SDK runtime imports or flash/SD/power/reset/DMA writes.

### Native PI4IOE5V6408 port prerequisites — 2026-11-11

Primary [Diodes PI4IOE5V6408 datasheet](https://www.diodes.com/datasheet/download/PI4IOE5V6408.pdf),
DS40583 Rev3-5, September2022, pp8–10 register map/configuration and p13 Figures6/7
(single-register write and repeated-START read; page rendered/visually inspected).
OUT05 reads the latch, not physical pin voltage; DIR03 bit1 means output, and
HIGH_Z07/PULL_ENABLE0B/PULL_SELECT0D are plain configuration. ID/control01 and
interrupt/status registers have side effects, so the new subset excludes them.
The local M5Unified revision44952cb8e4ac88bf04ad23b9dfd6f4b05c788348,
PI4IOE5V6408_Class.inl/I2C_Class.inl, supplies the port's configuration/bit-update
model under MIT (notice retained in LICENSES/M5Stack-MIT.txt). Its calls delegate
to native transport; no M5Unified/Arduino/ESP-IDF/FreeRTOS runtime linkage.
Existing Tab5 PDF pinmap/schematic and BSP audit establish system bus/address43/44.
The forthcoming hardware probe mirrors only the current OUT05 latch byte and
checks unchanged configuration; this does not measure electrical levels or reset.

Actual target result: both expanders preserve five config bytes across16 rounds
after identical OUT05 writes (43: DIR0x6F/OUT0x66;44: DIR0xB9/OUT0x09). Native
transport/port has no foreign runtime symbols/archives. Actual absent-address
write NACK and touch firmware1 regression pass. No changed reset/power bits or
physical voltage/glitch measurement. Host20/20, two sanitizer fixtures, C++98 and
five driver mutants PASS; original flash shell/SciCalc14/desktop verified after
RAM reset. Raw evidence/provenance/limits in ../audits/native-expander-port.md.

### Native panel-reset port prerequisites — 2026-11-11

[Reset source ledger](../audits/native-panel-reset.md): Tab5.pdf update2026-11-05
p9 schematic bank43 P4/P5 and p16 reset electrical-mode rule; functional bf044d3
bsp_reset_tp100ms assertion +100ms stabilization; M5Unified Power_Class.inl
input/pull-up release, M5GFX tab5_power_on10/100ms distinguished from that baseline.
GPIO23 input preparation reuses pinned GPIO/IO_MUX constants and primary register
references in the touch audit. Apache-2.0 Espressif notice/full license retained
for the modified BSP reset port; MIT expander adaptation remains separately
attributed. No new MMIO addresses or CPU-frequency delay assumption introduced.

Actual reset target: stage9/changed1/preserved1, native SYSTIMER elapsed244643us,
GPIO23 input readback1; after reset touch status0/firmware1 sixteen reads plus
recovery, errors0. Both-bank snapshots compare final reset masks/unrelated bits.
This proves register sequencing/touch recovery on warm supplies, not electrical
voltages/waveforms, pixels or cold power-up. Host21/21, sanitizer/C++98/six mutants
PASS. Preserved shell/SciCalc14/desktop verified after runner reset; no flash/SD
write. Raw captures/artifact hashes/limits: ../audits/native-panel-reset.md.

### Read-only native VO3 calibration prerequisites — 2026-11-12

[Calibration source ledger](../audits/native-ldo3-calibration.md) pins primary
ESP-IDF f5c3654 ldo_ll.h conversion/unit mapping, efuse_reg.h revision/VO3 trims,
reg_base.h eFuse base5012D000 and pmu_reg.h P0_0P2A control/analog. Exact shadow
addresses5012D04C/5012D050/5012D1C0 are read only; no eFuse command/program register
is introduced. SDK model conversion is ported under retained Apache-2.0 notice.
The current native stage computes a2500mV plan and observes registers; it neither
claims the physical voltage nor acquires/enables the regulator. Actual power
programming and ownership/failure contracts remain a separate step.

Actual2026-11-12 RAM observation: chip1.3/block0.3, no version-disable or block1
RS errors. Trims55/33/33 ->model coefficients1030/-4/989 ->dref13/mul3, model
2504637uV/error4637uV, no default coefficients. Existing PMU enable/SW-selection
and codes already match; no writes were made.16 snapshots unchanged, errors0.
This validates reading/model arithmetic, not measured voltage or native power
initialization. Host22/22, sanitizer/C++98/six mutants and1048576-case exact SDK
differential PASS.

Preserved flash shell/SciCalc14/desktop verified after reset with paced keys;
initial unpaced3s capture failed to collect the result and is retained in the
VO3 report. No firmware fix or flash/SD write was made for this retest.

### Native VO3 acquisition prerequisites — 2026-11-12

[Acquisition source ledger](../audits/native-ldo3-acquire.md) pins SDK f5c3654
esp_ldo_regulator.c/ldo_ll.h acquire and VO3 mapping, pmu_reg.h control4F80 and
analogF7800000 masks. Current-limit/delay/other fields are preserved. Pinned
hp_sys_clkrst_reg.h +18 mask1C00 and +3C mask93 guard CSI/DSI functional clocks;
API/diagnostic do not gate or program these clocks. LCD MIPI example identifies
VO3 as VDD_MIPI_DPHY supply. Native API is boot-exclusive; the controlled enable
probe may clear only VO3 XPD before invoking acquire, after checking existing
calibration/source and all guarded clocks. This is not voltage/discharge proof.

Target finding: retained clock words0x0C0D5678/0x2B3 keep guarded MIPI enables
active across ROM RAM entry. Initial guard refused before writes; final native
policy adopts matching active VO3 without writing. Actual stage5/adopted1/changed0,
clock/PMU-slot preservation1 and duplicate BUSY. Inactive configure/enable is
host-tested only; physical cycle NOT RUN until ordered consumer/DMA quiesce.
Host23/23, sanitizer/C++98/six mutants PASS. Paced shell/SciCalc14/desktop verified
after reset; no flash/SD or target regulator write. See ../audits/native-ldo3-acquire.md.

The earlier panel-reset probe did not establish consumer/DMA quiescence. Its
report is now qualified and its diagnostic rejects active MIPI clocks before
reset; guarded probe compile passes, physical rerun remains pending quiesce.

### Read-only inherited MIPI/DW-GDMA state — 2026-11-12

[Source/register ledger and actual values](../audits/native-mipi-state.md)
pins ESP-IDF f5c3654 revision/sections and primary URLs. In particular,
[peripheral linker symbols](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/ld/esp32p4.peripherals.ld)
verify DW_GDMA50081000 and CSI/DSI host/bridge bases. Pinned hp_sys_clkrst_reg.h
SOC_CLK_CTRL0+14/CTRL1+18 bus gates control diagnostic access; DW-GDMA requires
both CPU13/SYS5 per dw_gdma_ll_is_bus_clock_enabled. Native reads only listed
non-clearing config/status registers; never reads memory through DMA pointers.

Tab5 eight complete sequential samples: CPU/SYS/PHY E6DFB7FF/0C0D5678/2B3,
valid0D. DSI bridge/DPI enable0, host POWER/PHY_RSTZ0; CSI bridge0. CSI host
skipped because bus clock off. DW-GDMA CFG/CHEN0/CHEN1 all0, four channel
FIFO/status/address/LLP0. Clock enables persist, but controller enables were0;
active DMA is not established. Snapshot cannot certify AXI drain/PHY stop or
cold initialization. Host24/24, ASan/UBSan and negative checks PASS. Preserved
flash shell/SciCalc14/desktop verified after reset; no flash/SD write. This
checkpoint does not remove VO3/panel-reset guards or port native shutdown yet.

### Native VDMA completion contract — 2026-11-12

Primary silicon source: [ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
section5.5.7 pp337–338 (early channel disable), register5.3 p349 (global EN
readback stays1 until channel activity terminates), register5.4 p350 (EN_WE),
register5.5 p351 (RESET), section5.8 pp344–347 (register offsets). SDK f5c3654
[dw_gdma_ll.h](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
channel-disable strobes/global completion/both bus clocks cross-check the same
contract. Addresses remain DW_GDMA50081000, CFG+10, CHEN+18, ABORT+1C, RESET+58,
channel FIFO+134/+234/+334/+434; no new interrupt mappings or DMA memory claims.
Exact PDF/source hashes, local extraction and source sections are retained in
[the stop audit/manifest](../audits/native-vdma-stop.md).

Tab5 actual empty-controller test: CFG write1/read1 with all channels disabled,
then native global stop0/read0 in one poll, no transfers. stage3/armed1/changed1/
requested0/preserved1/duplicate_busy1; all sampled MIPI/clock/channel words
preserved. This is not active DMA drain evidence; active path/failures are host
25/25 +sanitizer tested. No clock/power/reset/IRQ clear/descriptor/flash/SD writes;
shell/SciCalc14/desktop verified after RAM reset. PHY/bridge, DMA-visible memory
and active-transfer tests remain separate prerequisites for full native display.

### Native DMA-visible SRAM and active stop — 2026-11-12

Primary source [ESP32-P4 chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf),
7.3.1 pp451–454 (CPU NC alias8FF and L2MEM4FF/DMA access),5.7.4 p343 (single
block),5.5.7 pp337–338 and CFG0 p349 (stop/completion). Pinned SDK f5c3654
[cache alias definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/cache_ll.h),
ext_mem_defs.h offset40000000, dw_gdma_ll.h/register layout confirm memory-master1/
width8/increment/burst1. ROM cache.h signature/L1D map10 and esp32p4.rom.ld
4FC003FC/4FC003C8 are explicit immutable-ROM calls, not an IDF runtime link.
Register INTSIGNAL writable maskF83F7FFB preserves observed07C08004 reserved/RO
bits; the original zero-readback assumption was refused before channel enable.
Exact source revisions/hashes/sections in [the fixture audit](../audits/native-vdma-transfer.md).

Actual reserved SRAM4FF03500–4FF13700, CPU NC alias8FF03500, L1D64B/range purge
success. Full32768B M2M verified; second transfer stopped with initialCFG1/CHEN1/
requested1, two polls, finalglobal/channel/abort0, correct1152B prefix+zero tail,
source/guards intact and eight full NC scans stable. Sampled clocks/MIPI/other
channels unchanged. Status/common0 (no reported errors); no DMA IRQ claim.
Host26/26/sanitizer/negative checks PASS. Final capture complete; earlier truncated
banner retained/rejected. Shell/SciCalc14/desktop verified after reset; no flash/
SD/power/reset/clock writes. Peripheral handshake/PHY stop and PSRAM remain open.
