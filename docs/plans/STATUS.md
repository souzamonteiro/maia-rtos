# Project status

Date: 2026-11-04

| Area | Status | Evidence |
|---|---|---|
| Architecture documents | DESIGN | Repository documents |
| M0-A / N0 ESP32-P4 & Tab5 hardware/memory audit | HARDWARE-TESTED | `docs/audits/esp32p4-execution-memory.md`, `docs/hardware/EVIDENCE.md`, `docs/hardware/OPEN_QUESTIONS.md` |
| N1 MaiaShell boundary & reentrancy audit | DESIGN | `docs/audits/shell-boundary.md` (inspected against `maiaos/apps/shell`) |
| Host build/test scaffold (`maia_contract_test`, `esp32p4_regs_test`, `kernel_mvp_test`) | HOST-TESTED | CMake/CTest (`100% tests passed out of 4` in `build_host`, plus C++98 header checks) |
| N2 MaiaShellHost v1 contract & Interactive Native Shell (`maia_shell_mvp.c`) | HARDWARE-TESTED | Host CTest (`tests/host/shell_host_contract_test.c`, `tests/host/kernel_mvp_test.c`) + live interactive session on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M0 ESP32-P4 bare-metal boot (`start.S`, `soc_early_init.c`, `linker.ld`) | HARDWARE-TESTED | Booted directly in `L2MEM_LOW` (`0x4FF00000`) on M5Stack Tab5 (`ESP32-P4 rev v1.3`) with zero FreeRTOS/ESP-IDF/Arduino runtime (`docs/hardware/EVIDENCE.md`) |
| M0 Console driver (`soc_console.c`: USB-Serial/JTAG + UART0) | HARDWARE-TESTED | Bidirectional TX/RX verified on M5Stack Tab5 `/dev/cu.usbmodem1301` (`docs/hardware/EVIDENCE.md`) |
| M1 RISC-V Trap Vector & Timebase (`trap_context.S`, `soc_timer.c`) | HARDWARE-TESTED | 64-byte aligned CLIC `_maia_trap_entry`, `ebreak` exception trap (`mcause=0x30000003`), `ecall` trap (`mcause=0x3800000B`), CLIC `mcause` `MPP`/`MPIE`/`MPIL` save/restore, and microsecond hardware timebase verified on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M2 Multitasking Scheduler (`kernel/ksched.c`) | HARDWARE-TESTED | Priority + round-robin task scheduler (`idle`, `telemetry`, `maia_shell`), tick sleep/wakeup (`maia_task_delay_ticks`), and task introspection (`ps`, `ticks`, `uptime`) verified on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M4 Deterministic Region Memory Allocator (`kernel/kmem.c`) | HARDWARE-TESTED | 16-byte aligned region allocator with free-block coalescing (`mem`, `alloc 256`) verified on host CTest and on M5Stack Tab5 `L2MEM` (`0x4FF076A8`) (`docs/hardware/EVIDENCE.md`) |
| M5 Synchronization & IPC (`kernel/ksync.c`) | HARDWARE-TESTED | Mutex (`maia_mutex_t`), counting semaphore (`maia_sem_t`), and bounded message queue (`maia_queue_t`) verified across `telemetry` and `maia_shell` tasks and `qtest` on M5Stack Tab5 (`docs/hardware/EVIDENCE.md`) |
| M3 SMP (Dual-Core HP Core 0 + Core 1) | NOT IMPLEMENTED | Core 1 parked safely in `start.S`; bring-up tracked in `UQ-006` |
| PSRAM external heap region | NOT IMPLEMENTED | Internal 32 KiB `L2MEM` pool active; Octal PSRAM MSPI init tracked in `UQ-007` |
| Board display driver (720x1280 MIPI-DSI ST7121 with LEDC backlight) & physical keyboard (A164 @ 0x6D) | HARDWARE-TESTED | Clean-room MIPI-DSI 800Mbps D-PHY + ST7121 28-cmd vendor DCS init + LEDC 5000Hz 12-bit PWM channel 1 backlight + PI4IOE 0x43/0x44 push-pull reset sequencing + DW_GDMA continuous 10-block ring in L2MEM + A164 STM32 keyboard String mode driver verified on Tab5 (`docs/hardware/EVIDENCE.md`) |
| Permanent Standalone Flash Boot (0x10000) | HARDWARE-TESTED | ESP-IDF 2nd-stage bootloader dual-segment Flash MMU mapping (`0x40000000`/`0x40010000`) and clean-room `esp_app_desc_t` (`0xABCD5432`) at Segment 0 (`0x4FF00000`), booting standalone directly from SPI Flash without host computer (`docs/hardware/EVIDENCE.md`) |
| MaiaScript Native Compiler (RV32IMAF ILP32F) & MaiaASM | IMPLEMENTED / HARDWARE-TESTED | Zero-GC AOT/JIT native compiler with 30/30 regression tests passing (Sections 01-32: Basics, OO, UI, Canvas, LVGL, Wi-Fi, Storage, Audio, I2C), EBNF-driven via MaiaCC (`make parser`), zero FreeRTOS dependencies, and live GUI execution verified on Tab5 hardware (`t_gui.mobj`) |
| Board touch/audio/SD drivers | NOT IMPLEMENTED | MicroSD pinout verified (GPIO39-44); driver tracked in `UQ-008` (M6) |
| MaiaWASMVM integration | OUT OF SCOPE for core | Optional user-space workload per `README.md` |

Do not change statuses without attached evidence.

## Scoped update — 2026-11-10

MaiaScript SciCalc/Alarm panel newlines: HOST-TESTED pixel positions and
panel bounds; updated firmware built/flashed and app operation
HARDWARE-TESTED over serial. Physical display appearance still requires
visual confirmation. See [test results and scope](../audits/maiascript-panel-text.md).
This update concerns the user's restored working firmware baseline and
does not revalidate the historical kernel/milestone claims above.
