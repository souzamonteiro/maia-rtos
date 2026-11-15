# Native bounded HEX PSRAM mode-register identity — 2026-11-15

**HARDWARE-TESTED, LIMITED: inherited powered/timed device identified.** Native
code reads MR0/1, MR2/3, MR4 and MR8 twice, with bounded direct controller waits
and exact temporary configuration restore. Actual AP vendor0D/density7/32MiB,
device field3 (SDK calls generation4), KGD6 and X16 confirmed. No native cold
initialization, PSRAM array, cache/MMU mapping or allocation proof yet.

## Sources, direct implementation and scope

Pinned SDK `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:

- [HEX implementation](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_psram/device/esp_psram_impl_ap_hex.c),
  `s_get_psram_mode_reg`: command4040/16bits, address32bits, MR0/2 read16bits,
  MR4/8 read8bits, preserved200M fixed-register protocol uses12 dummy bits.
  Mode-register layouts and density mapping define interpretation. This is the
  inherited configured-device profile, not a universal cold-default protocol.
- [Controller LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/psram_ctrlr_ll.h),
  `common_transaction` selects PSRAM command controller3, OPI_DTR and CS1. Native
  implementation uses bounded register operations, not this driver or SPI ROM
  helpers. No clock/VO2/mode-register programming is performed here.
- [PSRAM command-port registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/spi1_mem_s_reg.h):
  CTRL+08 FASTRD/OCT, USER+18 phases/low FIFO, USER1+1C address/dummy length,
  USER2+20 command length/value, ADDR+04, MISO_DLEN+28, MISC+34 chip-select/
  keep-active, CACHE_FCTRL+3C address4byte, DDR+D4 DDR_EN/CMD_DIS and W0+58.
  Fields converted to programmed length-minus1. Literal CMD+00 USRbit18 starts
  only the4040 read. No program/erase trigger, PSRAM mode writeC0C0 or array
  write8080. Register names inherited from generic SPI documentation do not mean
  flash controller access; base is dedicated PSRAM MSPI1=5008F000.
- [SPI ROM bindings](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_rom/esp32p4/ld/esp32p4.rom.ld)
  and [API declarations](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_rom/esp32p4/include/esp32p4/rom/opi_flash.h)
  were reviewed. Local shipped `esp-rom-elfs/20241011/esp32p4_rev0_rom.elf`
  inspection shows cmd_config begins an unbounded command-idle wait and
  set_op_mode can invoke a mutable callback. That ELF is a reference, not proof
  of the current board ROM implementation. No SPI ROM call/code/header is used
  in this target. Published register fields are the implementation facts;
  native policy/error guards are original MIT, no ROM implementation copied.

Host-only SDK register oracle checks all ten addresses/masks plus start bit.
Headers/HEX source match pinned revision. Native has only project objects+libgcc,
no SDK/FreeRTOS/Arduino runtime or compatibility API.

## Ownership and failure behavior

Boot/core0/IRQs-off/exclusive, all code/data internal SRAM, no external-memory
clients. Initial native snapshot must be stable/accessible, both controller CMDs
idle, memory-side AXI_REQ_EN0, VO2 XPD1, supported phase/DQS conditions. This uses
inherited power/clocks/device modes; no independent cold claim. Root/voltage/
eFuse, untouched cache-side configuration and command clock/calibration are
checked repeatedly. Cache-side arbitration status may change in low8 CMD bits;
no cache-side user command is allowed. Before setup/restore both CMDs must be0.

Save ten command-port words, masked RMW/readback to OPI_DTR/read-only phases,
16-bit4040 command/32-bit MR address/12 dummy bits, CS1 only and keep-active0.
Receive W0 is poisoned A55A5AA5 before starting. Start field records an attempt,
not an ACK. Bounded1..100000 polls require both controllers idle, reject foreign
command bits and compare owned configuration. Only then sample response; stale
poison or non-AP vendor on MR0 is refused. Source changes after guard are excluded
by required producer exclusivity; sequential snapshots are not atomic tokens.

Restore all ten words in reverse order only when idle and ownership matches.
Successful fully restored reads can repeat. Any partial store, timeout, ownership
loss or bad response is sticky until reboot. Timeout does not restore an active
controller, abort/reset/repair it or retry. Bad response restores guarded idle
configuration but remains failed. Caller reports are not adopted as ownership.
Null/invalid/context/prior FAILED no MMIO. No array/cache/MMU/LDO/clock/IRQ writes.

Host initial dropped-store test incorrectly required detection of restoring a W0
value equal to the old ID. Corrected before target: same-value stores, including
ADDR0 and unchanged W0 restore, have no readable ACK. Changed stores still must
refuse, and stale plausible ID cannot pass a dropped trigger because poisoning
and vendor/response checks remain strict. No target failure hidden by this test
correction. Independent capture oracle checks configuration, response classes,
repeatability and complete restoration; initial GATED snapshot cannot pass ID.

## Actual target result

One RAM upload, SHA256
`95cec29d4b5fbd6ff0b635c2bf420a9eafc37be8e3c6509ae0f0c3421f8d8471`.
Entry4FF00100/load4FF00000/10448bytes, inside qualified SRAM window. Initial
snapshot54 observations stable. Eight reads, indices0..7, addresses0/2/4/8 twice:

| Mode registers | Actual raw response | Read bits |
|---|---|---:|
| MR0/MR1 | 8D30 | 16 |
| MR2/MR3 | A0DF | 16 |
| MR4 | 20 | 8 |
| MR8 | 4B | 8 |

Each result0/stage5/setup3FF/restore3FF/start-attempt1/polls1/busy0/CMD0/restored1.
No busy edge was sampled: transaction completed before first command observation;
poison-replacement/real vendor and repeated mode values establish the response.
Full responses retained, including MR1 upper flag and MR3 upper fields; no forced
match to host synthetic sample values. All80 configuration records match the
independent mask/value model. Native post-snapshot matches original roots and27
controller words; summary vendorD/density7/device3/KGD6/X16=1. HOST ID PASS.
Reset then shell/SciCalc `2+3*4 = 14`/desktop PASS. No flash/SD writes.

Host48/48 CTest PASS, actual mode driver ASan/UBSan PASS, C++98 header PASS,
invalid profile rejected; six compiled runtime mutants rejected (ignore AXI owner,
response, timeout, omit start, omit restore, permit retry after failure). Tests
cover no-MMIO errors, inherited AXI busy, gates, four mode widths, repeated exact
restore, fast completion, dropped changed stores, stale receive word/bad vendor,
active timeout and root loss with no unsafe restore. SDK oracle PASS. Native
GCC14.2 RV32IMAC/ILP32/runtime gate clean. No hard WCET/clock measurement claim.

```
python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-psram-id-20261115/maia_sched_probe_ram.bin --log /tmp/maia-psram-id-hardware.log --psram-id --capture-seconds 10
```

Enable PSRAM_STATE+PSRAM_ID only. [Hardware](native-psram-identity-hardware.txt),
[desktop](native-psram-identity-desktop.txt), [manifest](native-psram-identity-manifest.json).
Recovery `../maia-rtos-recovery/native-psram-identity-20261115/`; tested code matches
prepared snapshot. Original firmware digests retained. Next native checked VO2/
clock/mode setup and owned memory test, then MMU/cache/framebuffer; reading device
identity does not establish those. LCD continuous-service and touch event goals
remain in [delivery plan](../plans/LCD_TOUCH_NATIVE.md).
