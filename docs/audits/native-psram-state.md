# Native PSRAM controller state and HEX initialization audit — 2026-11-15

**HARDWARE-TESTED, LIMITED: read-only controller/root observation.** Native RAM
probe observed nine stable roots and27 whitelisted controller words twice, with
no PSRAM/control/clock/cache/MMU writes or array access. This does not initialize,
identify, allocate or test PSRAM memory. LCD/touch completion remains the priority
before other drivers and resuming MaiaScript; see [delivery plan](../plans/LCD_TOUCH_NATIVE.md).

## Correct device path and sources

Earlier documents called this PSRAM Octal. The preserved `sdkconfig` selects
SPIRAM_MODE_HEX with no8-line override; SDK Kconfig calls that16-Line-Mode. Actual
preserved boot says hex_psram/vendor0D/AP/gen4/density7/256Mbit/X16/Burst2048bytes,
32MB200MHz, then PSRAM XIP/memory test. These are legacy observations, not native
cold-start settings. Do not port the S3 Octal implementation to this P4 path.

Primary implementation sources pinned to ESP-IDF
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:

- [HEX implementation](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c),
  `esp_psram_impl_enable`, `s_init_psram_mode_reg`, `s_get_psram_mode_reg`,
  `s_config_mspi_for_psram`: acquire MPLL/set400MHz, PSRAM clock/reset, DQS/pad,
  CS timing, two bus dividers/DLL, MR setup/identity/density, cache transactions,
  timing tuning. MR register commands4040/C0C0 with16-bit command/32-bit address;
  native mode-reg command/latency handling not yet implemented. High-level code
  depends on SDK clocks/critical sections/tuning/logs; do not link those runtimes.
- [P4 Kconfig](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/esp32p4/Kconfig.spiram),
  SPIRAM_MODE_HEX and20M/experimental200M options. The fallback latency branch
  must not be treated as a verified cold-device specification by itself.
- [Controller LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h),
  enable/reset module, select source, set core/bus divider, DLL/common transaction.
  PSRAM memory controllers are SDK MSPI2/3 at PSRAM_MSPI0/1 base5008E000/5008F000;
  these are separate from flash controller bases. No register names containing
  'FLASH' authorize flash operations.
- [Register bases](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/reg_base.h),
  [RCC](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/hp_sys_clkrst_reg.h)
  SOC+14 PSRAM_SYSbit31, PERI00+30 PSRAM_PLL/core bits14/15, source13:12,
  dual MSPI reset+ C0 bits23/25; force-norst+CC observed without interpretation.
- [PSRAM MSPI0 registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi_mem_s_reg.h),
  CMD/CTRL/CTRL1/CACHE_SCTRL/SRAM_CMD/DRD/DWR/CLK/DDR/calibration/delay/gate/PMS0;
  [MSPI1 registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi1_mem_s_reg.h),
  CMD/CTRL/CTRL1/CTRL2/CLOCK/USER/USER1/USER2/DDR/calibration/gate. Whitelist omits
  FIFO/status-clear/trigger/MMU selector/table reads; CMD read is status only.
- [LDO LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/ldo_ll.h),
  unit index map0,3,1,4: VO2(unit1) is PMU.ext_ldo[3],
  [PMU](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/pmu_reg.h)
  P1_0P1A+1D0/1D4. Existing eFuse SYS2/3 and RS_ERR0(+1C0) are recorded;
  no calibration or measured-voltage claim. SDK1.9V special eFuse pair requires
  an independently checked native acquisition contract, separate from VO3.

Reference headers match the pinned SDK. Host-only oracle compiles data macros
and compares all36 addresses and gate/reset masks; no SDK header/runtime enters
native source. New whitelist/API/tests/probe are original MIT.

## API and safety limits

`maia_esp32p4_psram_snapshot` is boot/core0/IRQs-off/exclusive, not runtime/ISR-safe.
Report initialized; null/context no MMIO. Conservative policy requires SYS/PLL/core
clock bits on and dual resets off before controller reads; it is an access policy,
not a complete physical accessibility proof. Otherwise return GATED with roots,
zero controller reads. Before EVERY controller read, compare the three RCC words
against original values. Recheck all roots and compare two controller passes.
CHANGED retains partial evidence and stops; no retries/repairs. No unbounded wait,
heap, arbitrary read API, memory mapping, power command or PSRAM array access.
Sequential guards require exclusive producers; cannot close a hardware race.
OK means matching observations, not device/readiness/capacity/coherency proof.

## Actual target and checks

Host47/47 CTest PASS, actual C ASan/UBSan PASS, C++98 public header PASS, invalid
mixed profile rejected. Tests: no MMIO null/context, each policy-gating condition,
root loss after each partial read, all27 controller drifts and final PMU drift;
four compiled runtime mutants rejected (ignore gates/live clocks/data/root drift).
Independent SDK oracle PASS. A typo in the oracle's FORCE_NORST macro name was
corrected before target execution; actual target addressCC was already correct.
Capture-validator eFuse error address corrected to1C0 before target (host synthetic
capture used same former wrong literal17C); SDK oracle independently confirms1C0.
No initial hardware-failure image or retry is concealed by these host corrections.

Native GCC14.2 RV32IMAC/ILP32/runtime gate PASS, project+libgcc only. Image entry
4FF00100, load segment4FF00000/6608bytes, inside tested SRAM window. SHA256
`a059fe491269d2cec18e4402a2edb45f441dcf32dd30f16d47744c7929bc29e1`.
One RAM upload: result0/root_valid1/samples36hex=54, all nine roots/27 words match.
RCC SYS E6DFB7FF/PERI D00C/reset38000040/norst0, VO2control40200180/analog76800000,
eFuseSYS2 7B054313/SYS3 3A184DEA/RSerror0. Both controller CMD words0, bus-clock
words30103, MSPI0 CTRL1 28E00000 (AXI_REQ_EN0), read/write command registers0,
DDR3020; MSPI1 DDR20. These inherited observations are not a usable-memory claim
or clock-frequency measurement. No array read attempted. HOST OBSERVATION PASS.
Reset shell/SciCalc `2+3*4 = 14`/desktop PASS. No flash/SD writes.

```
cmake --build /tmp/maia-native-psram-state-20261115 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-psram-state-20261115/maia_sched_probe_ram.bin --log /tmp/maia-psram-state-hardware.log --psram-state --capture-seconds 8
```

[Hardware](native-psram-state-hardware.txt), [desktop](native-psram-state-desktop.txt),
[manifest](native-psram-state-manifest.json); recovery
`../maia-rtos-recovery/native-psram-state-20261115/`. Original firmware digests kept.
Next bounded native HEX mode-register identification/restore with actual vendor
response, then checked VO2/clock/mode setup, MMU/cache and owned memory test. Do not
reuse 48100000 or200MHz assumptions. Cold initialization/full framebuffer/native
desktop remain unimplemented. Touch coordinate/event driver still pending.
