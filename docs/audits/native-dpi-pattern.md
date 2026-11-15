# Native DSI host color bars and command recovery — 2026-11-14

**HARDWARE-TESTED, LIMITED; VISIBLE BARS CONFIRMED BY OWNER.** The isolated native
RAM diagnostic displays stable color bars, observed horizontally by the owner,
without FreeRTOS, DMA,
framebuffer or PSRAM. It then restores command mode, LP clock control, the VPG
word and GPIO22 configuration; four panel power responses remain9C/error-free.
This is a host-generated test pattern, not application graphics or a native desktop.

## Orientation correction — owner clarification

The owner subsequently clarified: **“As barras estavam horizontais”**. This
supersedes the visual orientation inferred from the SDK pattern name and the
leading wording of the earlier questions. Visible stable bars were confirmed;
their observed orientation was horizontal. VPG_ORIENTATION24=0 remains the actual
captured configuration, named BAR_VERTICAL by the SDK. No firmware change or new
hardware test was made for this documentation correction.

The production baseline `platforms/tab5/main/hal/hal_esp32.cpp:104` explicitly calls
`lv_display_set_rotation(lvDisp, LV_DISPLAY_ROTATION_90)` with `sw_rotate=true`.
Keep native panel geometry720x1280 distinct from logical desktop1280x720. The
native VPG bypasses LVGL and its software rotation. Preserve the production
orientation and corresponding input-coordinate mapping when native framebuffer
presentation is implemented; do not infer a clockwise/counterclockwise physical
view or change hardware timings from the generator's enum name. Verify the final
mapping with an asymmetric labeled pattern. No native framebuffer rotation is
implemented by the VPG test. See [correction record](native-dpi-pattern-orientation.md).

## Evidence and source facts

Build toolchain remains GCC14.2 RV32IMAC/ILP32. Pinned ESP-IDF revision
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` provides register/parameter facts:

- [Host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h),
  `dpi_set_pattern_type`: vertical color bars use VID_MODE_CFG VPG_EN16=1,
  VPG_MODE20=0, VPG_ORIENTATION24=0. `set_clock_lane_state(AUTO)` sets LPCLK_CTRL
  bits1:0=3; `enable_video_mode(true)` clears MODE_CFG bit0.
- [Register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_host_reg.h):
  host+38 pattern, +94 clock, +34 mode; +100 shadow controls default0.
- [DPI panel implementation](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c),
  `esp_lcd_dpi_panel_set_pattern`: host generator replaces bridge DPI stream;
  bridge output is disabled for patterns. The native test establishes that the
  host VPG operates with bridge/output/latch and DMA left inactive, rather than
  importing SDK framebuffer/DMA initialization or its runtime.
- [Tab5 PDF](https://m5stack.oss-cn-shenzhen.aliyuncs.com/resource/docs/static/pdf/static/en/core/Tab5.pdf),
  user-supplied update2026-11-05 p12: G22/LEDA for LCD.
  [Preserved BSP](https://github.com/souzamonteiro/maia-rtos/blob/e92305a/soc/espressif/esp32p4/soc_board_io.c)
  `maia_tab5_backlight_set_duty(4095)` uses DC HIGH for100% brightness.
- Pinned [GPIO register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/gpio_reg.h)
  GPIO22 output matrix bits8:0=256, invert9/11=0, OEN_SEL10=1; PAD_DRIVER bit2=0
  for push-pull. [IO mux](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/io_mux_reg.h)
  MCU_SEL14:12=1 for GPIO. Existing native addresses reused, no guessed addresses.

Only register/parameter facts consulted; the new checked transitions, ownership,
reports and private light fixture are original MIT code. No SDK/Arduino/FreeRTOS
headers, runtime or implementation control flow imported. Existing attribution
for prior staging and board initialization remains unchanged.

## Interface and invariants

`maia_esp32p4_dsi_dpi_pattern_start/stop` are boot/core0/IRQs-off/exclusive APIs.
Start requires real private native stage+clock claims and BSP-verified panel format.
It checks the cached19 fields/root/supply/PHY/command controls, locked/stopped TX,
empty buffers, shadow controls0, bridge/output/DMA/pattern/frameACK off. No adoption
of an externally configured generator or video stream.

Start makes three individually guarded/read-back stores: SDK vertical VPG mode,
automatic HS clock request, then video mode. Report stages1 guarded/2 generator/
3 HS clock/4 video, attemptmask7. The transport is reserved while active: read,
short and long calls return BUSY. Duplicate start is BUSY without MMIO.

Stop accepts only the private active claim: checked command mode, LP clock, bounded
polls for stopped/empty TX, then restoration of the exact original VPG word. It
checks full snapshots and errors before/after stores; root-clock guards precede
peripheral readback if bus access is lost. Stages1 guarded/2 command/3 LP/4 idle/
5 VPG restored, mask7. Budget1..100000 bounds shared polls, not a WCET guarantee.
A single successful cycle per boot; duplicate stop/restart BUSY without MMIO.
Partial start/any valid stop failure poisons pattern/transfers until reboot, with
no rollback/retry. An observed preflight DSI error is sticky too. Successful stop
releases transport while retaining the existing native DPI clock claim.

No bridge enable/output/update, framebuffer, DMA, IRQ, PLL retune or panel-table
change. The host VPG bypasses the bridge pixel path, so this does not validate the
bridge timing latch or DMA-fed RGB565 pixels. Format0C readback05 and the visible
pattern strengthen link evidence but do not prove every framebuffer color format.

The private diagnostic temporarily configures GPIO22 as DC HIGH. It saves output/
enable/pad/mux/matrix, disables the pin before rerouting, preloads HIGH, changes
only required pin fields and verifies every write. After stopping VPG it restores
all saved values, using W1TS/W1TC for just GPIO22, before post-stop panel reads.
This is a scoped test fixture, not a production backlight/PWM driver. It owns boot
GPIO access exclusively; partial failure halts and the host resets to preserved
flash. No concurrent GPIO task or general-purpose teardown contract is implied.

## Actual hardware observations

First RAM image SHA256
`7a902b357a2fd4c02e03e17c71e4d9d08a4ba9d0ca13ef44e3cabba88a67214a`
started/held/stopped the VPG and returned four9C words without errors. Owner
reported **black/no bars**. That diagnostic had not configured the backlight after
ROM RAM loading. Its actual capture is preserved, not relabeled as visible success.
The first image binary itself was replaced by the final build; only its recorded
hash/capture and construction scripts are retained for that attempt.

Final RAM image SHA256
`22fa5cedf860437a2f1ec5291119654cecab8008a6d0d452e3f8c1a84e4a91b2`:

- Full prior native supply/reset/ST7121 program/power/format/staging/clock chain PASS.
- GPIO22 initially OUT0, ENABLE0D40, PAD0, MUX800, MATRIX100. It was not configured
  as an enabled GPIO HIGH output. Temporary OUT400000/ENABLE400D40/PAD0/MUX1800/
  MATRIX500 read back correctly. The observation does not measure LED current.
- Start stage4/mask7, VID0001BF02/MODE0/LPCLK3/shadow0; all error flags0.
- Five bounded >=1000ms waits and samples: same controls, PHY1529 or15B9 (clock/
  lanes may be moving), FIFO50015, errors0, bridge/output/update/DMA inactive,
  duplicate start and command read BUSY. Lock maintained; stopped lanes are not
  required while transmitting. This is sampled evidence, not continuous monitoring.
- Owner replied **“Sim, barras estáveis”** to the second visual check. No camera/
  screenshot capture, colorimetry or electrical frequency measurement is claimed.
- Stop stage5/mask7/one poll, VIDBF02/MODE1/LPCLK0/PHY15BD/FIFO50015/errors0.
  GPIO22 restored and verified. Four power9C words/stage10/one word/headers3/
  polls6/errors0/control restoremask5. Runner PASS and reset to preserved flash.

[Initial black-screen capture](native-dpi-pattern-initial-hardware.txt),
[final visible-pattern capture](native-dpi-pattern-hardware.txt),
[owner observation record](native-dpi-pattern-observation.txt),
[desktop recovery](native-dpi-pattern-desktop.txt),
[manifest](native-dpi-pattern-manifest.json).

## Validation, scope and next small task

Host41/41 CTest PASS. Actual SoC driver fixture covers three start and three stop
dropped stores, cached timing/source/shadow drift, exclusive/duplicate claims,
post-stop reads, DSI errors, six clock-loss points, bounded stop timeout and sticky
failure. Light fixture covers four original latch/enable combinations, unrelated
pin preservation and12 deliberately dropped stores. Matching writes have no
hardware side effect and cannot detect a dropped identical write in general.
Capture tests reject malformed/reordered/missing phases, wrong VPG/clock/mode/
shadow/light fields, lost exclusivity, active DMA and missing restoration.

ASan+UBSan driver/light fixtures PASS. Six compiled runtime mutants rejected:
wrong generator bit, wrong HS request, retained command mode, ignored shadow,
skipped stop idle and prematurely released transport. Public C++98 header PASS;
invalid CMake pattern-without-clock profile rejects. Native runtime gate PASS,
project+libgcc only, no unresolved/foreign symbols. Known RWX/removed empty flash
segment linker warnings remain. Entry/segments all inside4FF00000..4FF28000.
No parser/EBNF, board command table, timing/default production profile, flash or
SD writes. Installed complete desktop remains the preserved FreeRTOS firmware;
the native diagnostic has no FreeRTOS dependency. Cold PLL startup, continuous
video recovery, framebuffer/PSRAM, native app graphics and POSIX remain pending.

```sh
cmake --build /tmp/maia-native-dpi-pattern-20261114 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dpi-pattern-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-pattern-20261114/maia_sched_probe_ram.bin --log /tmp/maia-dpi-pattern-light-hardware.log --dpi-pattern --capture-seconds 20
python3 /tmp/maia-dpi-pattern-checks.py
```

Configure all prior DPI_CLOCK prerequisites plus `MAIA_NATIVE_PROBE_DPI_PATTERN=ON`.
Recovery `../maia-rtos-recovery/native-dpi-pattern-20261114/` retains final image,
source, logs, references, check scripts and original firmware hashes. Next small
task: guarded bridge/DPI configuration latch with output and DMA inactive, before
an owned SRAM framebuffer/DMA transfer and later PSRAM. The host VPG bypass means
its visible success does not establish that separate bridge path.
