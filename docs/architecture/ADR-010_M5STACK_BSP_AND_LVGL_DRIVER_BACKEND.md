# ADR-010: Adoption of M5Stack Vendor BSP (M5GFX/M5Unified) and LVGL Backend for Graphics & Drivers

- **Status**: ACCEPTED (Decided on 2026-11-05)
- **Context**: MaiaRTOS on M5Stack Tab5 (ESP32-P4 v1.3)
- **Scope**: Hardware driver backend (Display MIPI-DSI ST7121, Touch, Physical Keyboard, Backlight, PSRAM) and Graphics Subsystem (LVGL). Kernel core (Scheduler, IPC, Tasks, Memory allocator) remains clean-room freestanding C.

---

## 1. Context and Problem Statement

Bringing up low-level display and peripheral drivers on modern complex SoCs like the **ESP32-P4** involves multi-million transistor peripherals:
- **Synopsys MIPI-DSI Host & D-PHY** requiring dynamic PLL calibration, lane deskew, and strict clock ratio alignment.
- **DSI Bridge FIFO with 2D-DMA / DW-GDMA** requiring bus arbitration across the AXI interconnect.
- **PSRAM (32 MB Octal / Quad SPI)** caching and cache coherence (`esp_cache_msync`).
- **Physical STM32 A164 keyboard & capacitive touch** over multiple I2C buses.

Writing isolated register-poking drivers from scratch without vendor HAL in a pure freestanding environment led to severe hardware constraints:
- Framebuffer was constrained to L2 Internal SRAM (using 128-line band tiling hacks due to absence of PSRAM initialization).
- Minor sub-cycle timing drifts caused DSI Bridge FIFO underruns (turning the screen light cyan/blue).
- Duplicating the vendor's display training and timing logic introduces long-term maintenance overhead.

The downstream user products (`MaiaOS` and `MaiaShell`) already utilize **LVGL with C++** for UI rendering.

---

## 2. Decision Drivers

1. **Standard Industry Practice**: Modern operating systems (e.g. Linux BSPs, NuttX, Zephyr RTOS) utilize manufacturer SDKs / HALs for complex high-bandwidth peripherals (MIPI-DSI, Wi-Fi, BLE, 2D-DMA, Power Management).
2. **Full Hardware Capability**: M5Stack's official BSP (`M5Unified` and `M5GFX`) already incorporates full 32 MB PSRAM support, tearing-free double buffering, hardware 2D-DMA (`esp_async_fbcpy`), and precise ST7121 timing parameters.
3. **Ecosystem Synergy**: Adopting LVGL as the graphical rendering engine allows direct code reuse from `MaiaShell` and `MaiaOS`.
4. **Architectural Separation**: The core microkernel (scheduler, task management, synchronization, IPC) remains pure, clean, and portable C11 without vendor pollution.

---

## 3. Considered Options

* **Option A**: Continue reverse-engineering bare-metal DSI bridge, PSRAM, and cache controller registers directly in pure C. *(Rejected: High fragility, maintenance burden, lack of PSRAM bandwidth for high-res graphics)*.
* **Option B**: Adopt M5Stack's official BSP (`M5Unified` / `M5GFX`) and LVGL as the driver and graphics backend for board support, preserving the MaiaRTOS microkernel core. *(Accepted)*.

---

## 4. Architectural Layers

- **UI Layer**: `MaiaShell` (LVGL / C++)
- **Kernel Core**: `MaiaRTOS` (Freestanding C11: preemptive scheduler, tasks, IPC)
- **HAL Adapter**: Clean C API wrapping display flush, keyboard poll, touch poll
- **Driver Backend**: M5Unified + M5GFX + ESP-IDF hardware HAL

