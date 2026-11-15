# Native inactive DPI bridge enable/update publication — 2026-11-14

**HARDWARE-TESTED, LIMITED.** Native code enables bridge EN0 with verified readback,
issues a DPI configuration-update trigger and retains command/power reads. Pixel
output, video, VPG, HS clock request and DMA remain inactive. This verifies bridge
ownership, configuration preservation and command usability, **not completion of
the internal timing latch**: the update register has no readable completion ACK.

## Primary reference and publication semantics

Pinned ESP-IDF `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:

- [Bridge LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_brg_ll.h),
  `mipi_dsi_brg_ll_enable`: bridge+04 EN0; `update_dpi_config`: bridge+44 trigger1.
- [Register definitions](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/mipi_dsi_bridge_reg.h):
  EN0 is R/W; DPI_CONFIG_UPDATE0 is **WT**, writing1 updates DPI_* configuration.
  DPI_MISC_CONFIG+40 bit0 controls pixel output independently. INT_ENA+50 bit0
  enables underrun interrupt; INT_RAW+58 bit0 is raw underrun (not cleared here).
- [DPI setup](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c)
  enables bridge then issues update after configuring the timings. Native code uses
  that ordering with previously verified19 staged fields; it does not import DMA/
  framebuffer allocation, interrupt installation, semaphores or SDK runtime.

No WT readback, RMW, polling, automatic retry or inferred acknowledgment. The
reported `update_value=1` and attemptmask3 mean the CPU issued the trigger store,
not that internal application was observed. A deliberately dropped trigger in
the host fixture remains undetectable; this limit is tested explicitly, not hidden
by a fake self-clearing/completion model. Actual DMA-fed pixels are a later gate.
New checked ownership/publication code is original MaiaRTOS MIT; register/parameter
facts consulted, no SDK/Arduino/FreeRTOS headers/runtime/control flow imported.
Prior staging retains its Apache-2.0 attribution and unchanged timing/table data.

## Interface and failure behavior

`maia_esp32p4_dsi_dpi_bridge_prepare(report)` is boot/core0/IRQs-off/exclusive.
It requires actual private stage+clock ownership, unchanged cached19 config/root/
supply/PHY/command controls, command mode, LP clock, stopped/locked and empty TX,
VPG/frameACK/shadow controls off, bridge/output/DMA inactive, bridge IRQ enables0
and no raw underrun/DSI errors. No external bridge adoption.

Two stores only: bridge+04 OR1 preserves other bits, checked after fresh root
access guard; then literal1 to +44. Report stages1 guarded/2 enabled/3 published/
4 verified, attemptbits0 enable/1 update. Guards and accumulated error checks
surround each step. Bridge remains enabled while output and DMA stay off. No
reset/PLL/HS/video/IRQ/framebuffer/memory operations. Work is finite without a
polling loop; no elapsed-time or WCET guarantee.

The shared PHY guard follows only the private expected bridge bit, initialized0
and changed to1 after checked native enable. Existing LP short/long/read transport
can therefore work with the owned bridge on; outside enable or loss is refused.
Initial reference acquisition still strictly requires bridge off. VPG start
refuses bridge ownership, preventing accidental reuse of its bridge-off contract.
Default production profile is unchanged. The isolated bridge diagnostic requires
DPI_CLOCK and excludes the VPG profile; it does not rerun the visual bars test.

Duplicate prepare returns BUSY without MMIO. Preflight DSI/underrun errors and any
partial failure poison bridge and transfers until reboot. No rollback/retry or
bridge disable/teardown API. A late BUSY result is still a failed partial claim;
subsequent bridge/transfers return FAILED. Successful prepare releases transport,
retaining stage/clock/bridge ownership. Required reports are initialized.

## Tests and target evidence

Host41/41 CTest PASS. Actual driver fixture verifies private ownership, full EN
word preservation, one enable and one WT store, **zero WT reads**, duplicate/no I/O,
active-VPG exclusion, post-enable read, externally enabled/lost bridge, dropped
EN write, bus-clock loss after either store, DSI error/underrun at either step,
shadow/config/IRQ/output drift, stopped-lane refusal and shared sticky failure.
Dropped WT publication is explicitly NOT detectable by this API. Capture tests
require complete prior format/clock chain, exact request/idle/reply ordering,
preserved bits, DMA/output off, no ACK claim and stable power responses.

ASan+UBSan actual driver fixture PASS; C++98 public header PASS. Six compiled runtime
mutants rejected: wrong bridge-enable bit, wrong update value, WT read-as-ACK,
ignored underrun, ignored shadow and foreign-bridge adoption. Invalid standalone
bridge profile rejected. GCC14.2 RV32IMAC/ILP32 runtime gate PASS: project+libgcc,
no unresolved/foreign symbols. Existing RWX/removed empty flash-segment warnings.

Tab5 chip1.3/40MHz/cu.usbmodem1301, one final RAM load:

- Full native supply/reset/ST7121 program/power08->9C/format00->05/staging/clock PASS.
- Request stage4/mask3: EN0->1/update_value1/IRQ_RAW0, PHY15BD/FIFO50015/errors0.
- Idle EN1/DPI19D0/MODE1/LP0/DMAquiet1/preserved1/duplicateBUSY1/VPGbusy1,
  latch_verified0. No pixels or lighting test in this unit.
- Four post-prepare power9C words, stage10/headers3/polls6/one word/errors0,
  commandB0000 restored/mask5. Runner PASS/reset to original flash firmware.
- An earlier candidate also passed on hardware. Review restored the original
  strict bridge-off reference guard before the final build/test; both images and
  actual captures are retained separately, not described as identical binaries.

Final SHA256 `213a32b2288b58ebe4ea8363d267a3b38c5d4cfbfc15a29ee5ac6999b476e10d`.
Entry/segments checked inside4FF00000..4FF28000. Preserved original firmware
hashes remain unchanged. Shell/SciCalc `2+3*4=14`/desktop recovery is recorded in
[desktop capture](native-dpi-bridge-desktop.txt).

[Final hardware capture](native-dpi-bridge-hardware.txt),
[first candidate](native-dpi-bridge-first-hardware.txt),
[manifest](native-dpi-bridge-manifest.json). Recovery
`../maia-rtos-recovery/native-dpi-bridge-20261114/` includes source, final/first
images, logs, pinned SDK references and original firmware hashes. No flash/SD,
parser/EBNF, panel command-table, timing or production rotation change. Native
panel720x1280/logical desktop1280x720/LV_DISPLAY_ROTATION_90 remain separate; the
owner's prior visible bars were horizontal. Installed full desktop still uses
FreeRTOS; this native diagnostic does not. POSIX remains DESIGN.

## Reproduction and next small task

Configure all DPI_CLOCK prerequisites plus `MAIA_NATIVE_PROBE_DPI_BRIDGE=ON`,
`MAIA_NATIVE_PROBE_DPI_PATTERN=OFF`.

```sh
cmake --build /tmp/maia-native-dpi-bridge-20261114 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dpi-bridge-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-bridge-20261114/maia_sched_probe_ram.bin --log /tmp/maia-dpi-bridge-final-hardware.log --dpi-bridge --capture-seconds 15
python3 /tmp/maia-dpi-bridge-checks.py
```

Next: specify a bounded DMA descriptor/source-memory/bridge flow-control contract
for owned SRAM, stage those fields with output/DMA inactive, then prove pixels in
an isolated transfer. A full720x1280 RGB565 frame needs1,843,200 bytes, exceeding
the currently tested RAM-image window; do not silently allocate it there or assume
PSRAM works. Evaluate verified internal SRAM capacity or a documented repeating
source strategy before choosing a first pixel test. PSRAM, cache/DMA ownership,
rotation/input mapping, native graphics/apps and cold PLL startup remain pending.
