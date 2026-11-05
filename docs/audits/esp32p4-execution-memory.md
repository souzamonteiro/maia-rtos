# Audit: ESP32-P4 / M5Stack Tab5 Boot, Toolchain, and Executable Memory (M0-A / N0)

**Date:** 2026-11-04  
**Status:** DESIGN / RESEARCH AUDIT (No hardware claims; `Confirmed on board? = NO`)  
**Scope:** Milestone M0-A (`docs/agents/ANTIGRAVITY.md`) and Work Package N0 (`docs/shell/MaiaRTOS_Native_Execution_Architecture.md` §14).

---

## 1. Primary Hardware Documents & Revisions

All architectural facts below are traced to official primary sources. Register-level programming remains gated until exact section/page offsets from the PDFs and physical Tab5 hardware logs are captured in [`docs/hardware/EVIDENCE.md`](../hardware/EVIDENCE.md).

| Document | Publisher | URL | Revision Identified | Status |
|---|---|---|---|---|
| ESP32-P4 Technical Reference Manual (TRM) | Espressif Systems | https://www.espressif.com/en/support/documents/technical-documents | `v0.7` | Primary reference (`HW-002`, `HW-003`) |
| ESP32-P4 Series Datasheet | Espressif Systems | https://www.espressif.com/en/products/socs/esp32-p4 | `v0.7` / `v1.2` (for silicon rev v1.3) | Primary reference (`HW-001`, `HW-004`) |
| ESP-IDF Programming Guide (ESP32-P4) | Espressif Systems | https://docs.espressif.com/projects/esp-idf/en/stable/esp32p4/ | Stable (research reference only; zero runtime linkage permitted) | Reference |
| M5Stack Tab5 Product Documentation | M5Stack | https://docs.m5stack.com/en/core/Tab5 | `v1.0` | Primary board reference (`HW-005`) |

---

## 2. CPU ISA, ABI, and Interrupt Architecture

### 2.1 Cited Facts
- **High-Performance (HP) Subsystem (`HW-001`):** Dual 32-bit RISC-V cores operating up to 360–400 MHz.
- **Baseline ISA (`HW-001`):** `RV32IMAFC` (`I` base integer, `M` multiply/divide, `A` atomics, `F` single-precision floating-point, `C` 16-bit compressed instructions) plus `Zicsr` and `Zifencei`.
- **Low-Power (LP) Subsystem (`HW-001`):** Single 32-bit RISC-V coprocessor (up to 40 MHz, `RV32IMAC`). Out of scope for M0–M3.
- **MMU / Privilege (`HW-001`):** Machine (`M`) and User (`U`) modes with Physical Memory Protection (PMP) / World Controller, **no SV32 virtual-memory MMU**. Native tasks and `.mobj` modules share a single physical/bus address space.
- **Interrupt Controller (`HW-003`):** RISC-V Core-Local Interrupt Controller (CLIC) + Core-Local Interruptor (CLINT), fed by the ESP32-P4 Interrupt Matrix.

### 2.2 Compiler & ABI Strategy (Design Proposal)
- **Toolchain binary:** Standalone `riscv32-esp-elf-gcc` (from Espressif's standalone GNU toolchain release; zero ESP-IDF runtime libraries).
- **Host audit result (`UQ-009`):** Neither `riscv32-esp-elf-gcc` nor `esptool.py` is currently installed in `PATH` on the build host (`/opt/homebrew/bin/cmake` and `/usr/bin/cc` AppleClang 21.0.0 are present).
- **Initial M0–M2 Kernel ABI (`DESIGN`):**
  - Compile M0–M2 bare-metal bring-up with `-march=rv32imac_zicsr_zifencei -mabi=ilp32 -ffreestanding -fno-builtin -nostdlib` so early trap handlers and context switches do not touch `f0..f31` or `fcsr` before FPU enablement (`mstatus.FS`) is verified on silicon.
  - Enable `-march=rv32imafc_zicsr_zifencei -mabi=ilp32f` only after M1/M2 trap frame layout explicitly saves/restores or lazily switches the 32 single-precision FP registers and `fcsr`.

---

## 3. Memory Map & Executable Memory Matrix

Understanding where instructions can execute is a hard prerequisite for both M0 startup and the N5 native ELF32 (`ET_REL` `.mobj`) loader.

| Memory Region | Size | Address Window (TRM v0.7) | Bus / Cache Path | Executable (`X`)? | DMA Capable? | Status / Gate |
|---|---|---|---|---|---|---|
| **HP ROM** | 128 KB | `0x4FC00000` (check exact end in TRM) | Internal HP Bus | YES (Immutable ROM) | NO | `FACT` (`HW-002`) |
| **HP L2MEM (Internal SRAM)** | 768 KB | `0x4FF00000` – `0x4FFBFFFF` | L1 I-Cache / D-Cache -> L2MEM | **YES** (Primary target for bare-metal kernel `.text`, `.data`, `.bss`, stacks, and initial `.mobj` code) | YES (via AXI/AHB GDMA) | `FACT` (`HW-002`); exact usable range below ROM stack requires verification (`UQ-003`, `UQ-005`) |
| **External SPI Flash (Tab5)** | 16 MB | Cache MMU mapped window | MSPI0/1 -> L2 Cache -> L1 I/D Cache | **YES (XIP)** once MSPI + Cache MMU are configured | NO | `FACT` (`HW-005`); MMU setup registers `UNVERIFIED` (`UQ-005`) |
| **External Octal PSRAM (Tab5)** | 32 MB | Cache MMU mapped window | MSPI -> L2 Cache -> L1 I/D Cache | **CONDITIONAL / UNVERIFIED** (requires Cache MMU `X` permission + explicit D-cache writeback & I-cache invalidate + `fence.i` across both cores) | Conditional (requires cache maintenance) | `UNVERIFIED` (`UQ-005`, `UQ-007`) |
| **LP SRAM** | 32 KB | `0x50108000` (verify in TRM) | LP Bus | YES (LP CPU / slow HP access) | NO | `UNVERIFIED` (`UQ-002`) |
| **Peripheral MMIO** | — | `0x50000000`+ window | AHB / APB | NO | N/A | `UNVERIFIED` exact offsets (`UQ-002`) |

### 3.1 Key Implication for Native Execution (MaiaExec / MNM-1)
1. **Internal HP L2MEM (`0x4FF00000` range, 768 KB)** is the safest first target for both M0 bare-metal firmware execution and initial N5 native module (`hello.mobj`) loading, because it does not depend on external Octal PSRAM MSPI timing initialization.
2. **Instruction/Data Cache Coherency Gate (`UQ-005`):** Even in internal `L2MEM`, the HP RISC-V cores fetch through L1 I-Cache and write through L1 D-Cache. Loading a relocatable `.mobj` into RAM requires:
   - Writing relocated `.text`/`.rodata`/`.data` and zeroing `.bss`,
   - Flushing/writing back D-Cache lines covering the module region,
   - Invalidating I-Cache lines covering the module region,
   - Executing `fence rw, rw` and `fence.i` on the loading core (and via IPI on the remote core before it can jump into the module).

---

## 4. Boot, Image Format, Flash Route, UART & Debug on M5Stack Tab5

### 4.1 ROM First-Stage Bootloader (`FACT` / `UQ-003`)
1. On power-on reset, HP Core 0 starts in **HP ROM** while HP Core 1 is held in reset/clock-gated state.
2. ROM samples strapping pins (`GPIO34`–`GPIO38`, `HW-004`):
   - **SPI Boot mode:** ROM reads the image header from SPI Flash (typically at offset `0x0` or `0x2000` depending on ESP32-P4 ROM layout — marked `UQ-003`), copies `LOAD` segments into internal `L2MEM` (`0x4FFxxxxx`), validates checksum/SHA-256 if enabled, and jumps to the image `entry_addr`.
   - **Download Boot mode:** ROM listens on `UART0` (`GPIO37` TX / `GPIO38` RX) and internal `USB Serial/JTAG` (`GPIO24` D- / `GPIO25` D+) for `esptool` framing commands (`load_ram` or flash programming).

### 4.2 Console & Debug Route on Tab5 (`UQ-004`)
- **Primary candidate:** Internal `USB Serial/JTAG` peripheral exposed directly on the Tab5 USB Type-C connector (`GPIO24`/`GPIO25`). This provides both a CDC-ACM serial console and a USB-JTAG debug interface (usable with OpenOCD) without an external UART bridge chip.
- **Secondary candidate:** Hardware `UART0` (`GPIO37`/`GPIO38`) or Tab5 expansion debug pins (`PC_TX = GPIO6`, `PC_RX = GPIO7`).
- **Verification rule:** Do not hardcode `UART0` or `USB_SERIAL_JTAG` FIFO registers until `UQ-002` and `UQ-004` are confirmed against the Tab5 schematic and TRM v0.7.

---

## 5. Proposed Minimal Independent Startup / Linker / Image Workflow (No ESP-IDF Runtime)

To satisfy `R-002` (zero FreeRTOS/Arduino/ESP-IDF runtime dependency), MaiaRTOS M0 will use a **direct RAM/Flash bare-metal image workflow**:

```text
arch/riscv/start.S          ─┐
soc/espressif/esp32p4/...   ─┼─> riscv32-esp-elf-gcc (-ffreestanding -nostdlib)
boards/m5stack/tab5/main.c  ─┘           │
boards/m5stack/tab5/linker.ld ───────────┤
                                         ▼
                                   maia_m0.elf
                                         │
                  esptool.py --chip esp32p4 elf2image
                                         │
                                         ▼
                                   maia_m0.bin
                                         │
            ┌────────────────────────────┴────────────────────────────┐
            ▼ (Development RAM test — safest)                         ▼ (Flash boot test)
esptool.py --chip esp32p4 load_ram maia_m0.bin        esptool.py --chip esp32p4 write_flash 0x... maia_m0.bin
```

### 5.1 Why `esptool.py load_ram` First?
- `esptool.py load_ram` loads segments directly into internal `L2MEM` (`0x4FFxxxxx`) via the ROM bootloader over USB-C and jumps to `_start` without modifying SPI Flash or requiring custom flash/MMU initialization.
- If the test image faults or hangs, a power cycle / reset immediately returns the Tab5 to its intact factory flash image.

### 5.2 Minimal Startup Sequence (`arch/riscv/start.S` — Design Only)
1. Disable interrupts (`csrw mstatus, zero`).
2. Read `mhartid`: if `mhartid != 0`, park secondary core in a `wfi` loop until M3.
3. Initialize `.option norelax`, set global pointer `gp` (`__global_pointer$`) and stack pointer `sp` (`__stack_top` in internal `L2MEM`, 16-byte aligned).
4. Zero `.bss` (`__bss_start` to `__bss_end`).
5. Call `maia_soc_early_init()` (disable/feed ROM watchdogs once `UQ-002` addresses are confirmed, init console).
6. Call `maia_boot_main()` -> print `"MaiaRTOS M0 bare-metal boot OK\r\n"` -> enter deterministic idle loop.

---

## 6. File-by-File Patch Plan for M0-B (After M0-A Review)

| File | Layer | Responsibility | Prerequisite |
|---|---|---|---|
| `cmake/toolchains/riscv32-esp-elf.cmake` | Build | Standalone cross-compiler CMake toolchain file (`-ffreestanding -nostdlib -fno-builtin`) | `UQ-009` |
| `boards/m5stack/tab5/linker.ld` | BSP | Link `.text`, `.rodata`, `.data`, `.bss`, and 16 KiB boot stack into verified `L2MEM` range | `UQ-003`, `UQ-005` |
| `arch/riscv/start.S` | Arch | `_start` entry, `mhartid` check, `gp`/`sp` init, `.bss` zeroing, jump to C | `UQ-002` |
| `soc/espressif/esp32p4/soc_console.c` | SoC HAL | Minimal polled TX output on verified console peripheral (`USB_SERIAL_JTAG` or `UART0`) | `UQ-002`, `UQ-004` |
| `boards/m5stack/tab5/boot_main.c` | BSP | Print M0 banner and halt cleanly | `UQ-001`..`UQ-004` |
| `tests/host/elf_dependency_audit.cmake` | Test | Verify via `riscv32-esp-elf-nm` / `readelf` that target ELF contains zero `vTask*`, `xQueue*`, `esp_*`, or `arduino*` symbols | `UQ-009` |

---

## 7. Minimal Acceptance Test for M0 Gate

1. **Host Static Audit:**
   - `riscv32-esp-elf-readelf -h -l -S maia_m0.elf` confirms `ELF32`, `EM_RISCV`, entry in `L2MEM`, and zero external shared libraries.
   - `riscv32-esp-elf-nm maia_m0.elf` confirms only `maia_*` and compiler intrinsic symbols exist (zero FreeRTOS/ESP-IDF runtime symbols).
2. **Target Hardware Proof (Required for M0 DONE):**
   - Connect M5Stack Tab5 via USB-C; run `esptool.py --chip esp32p4 chip_id` and record exact silicon revision in `docs/hardware/EVIDENCE.md`.
   - Execute `esptool.py --chip esp32p4 load_ram maia_m0.bin` and capture the raw, unedited serial output showing the MaiaRTOS M0 banner.

