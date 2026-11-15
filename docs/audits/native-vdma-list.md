# Native two-block SRAM DMA list qualification — 2026-11-15

**HARDWARE-TESTED, LIMITED.** A native, finite two-descriptor DMA transaction
copied all32768 expected bytes into an owned SRAM destination, naturally stopped,
and preserved guards/source/unrelated peripheral state. No display/frame output,
PSRAM, IRQ or production driver API added. This qualifies descriptor fetching,
source increment, fixed-source behavior under explicit AXI length, and descriptor
writeback before the next spatial pixel test.

## Sources and contract

Pinned SDK `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0`:
[DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h)
and [registers](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_reg.h),
LLI64-byte format, source/destination master1, CFG0 source/destination LIST3,
LLP master1, width64, source/destination burst items1. No SDK headers or runtime.
New fixture/probe and checks are original MIT.
[Chip1.3 TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
§5.5.4.2 pp334–336 defines LLI fetching/terminal LAST and writeback clearing VALID,
CTL/LLP_STATUS writeback with status flags, separate SSTAT/DSTAT fetch controls;
register5.12 p357 defines ARLEN_EN and ARLEN. §5.5.7 pp337–338 disable/drain.
Prior NC/cache/ROM/NOLOAD ownership facts: [SRAM transfer proof](native-vdma-transfer.md).
TRM wording for source length does not establish raw AXI encoding; the explicit
field0/enable1 behavior below is qualified by actual bytes on this board.

The probe uses existing66048-byte reserved/guarded memory, boot/core0/IRQs off,
all channels/controller initially off and FIFOs empty, after ROM cache handoff,
then NC CPU accesses only. First32768-byte region contains a11520-byte RGB565 tile
(720x8, red/green/blue/yellow180px bands), disjoint8-byte solid8410 source at+12288,
and disjoint128-byte list at+16384. Second region is32768-byte destination.
All unused source bytes and four boundary guards remain A5. First descriptor
increments source/destination for11520 bytes. Second fixes source and increments
destination for21248 bytes; only second LAST, terminal LLP0. No circular list.
Expected bytes are computed from coordinates/colors, not copied from source for
comparison. Every source/unused/guard byte and all32 descriptor words checked.
Descriptor memory is writable engine-owned until completion/drain; never treat
VALID-clearing/status writes as corruption or refill while DMA owns the list.

## Refusals and correction

The initial image was strictly refused (matched16832/32768), with no errors,
all guards/source retained and natural stop. This original refusal is preserved.
A sampling image confirmed solid destination repeated8 correct bytes followed
by24 A5 bytes per32-byte group when ARLEN override was disabled. The programmed
source fixed bit alone did not establish an8-byte repeated-read footprint.
Initial descriptors also changed VALID to0 and wrote per-block count/status even
with SSTAT/DSTAT fetch bits0. This is consistent with TRM's LLI status writeback
layout; those fetch controls do not justify treating descriptors as read-only.

Final second descriptor CTL1=C0000040: ARLEN_EN1, ARLENfield0, VALID/LAST1,
no SSTAT/DSTAT fetch, no block interrupt. First CTL1=80000000, CTL0=1B05;
second CTL0=1B15. Exact post-writeback values required: clear VALID only in each
CTL1, word12=1439/word13=40000000, word28=2655/word29=C0000000; all other words
unchanged. No broad accept-mask or weakening of data/guards. All32768 bytes then
matched, including the8410 solid across the entire second block; samples64bytes
all84108410. Raw counterA5F, FIFO0/status0/common0, 4170 polls, natural stop1.
Peripheral CPU interrupt status0 is not the completion proof: exact data,
natural disable, drain and independent LLI block/DMA completion writeback are.
Do not infer a universal fixed-read granularity from the first refusal.

The preceding solid-display stage still leaves ARLEN override disabled. Its
stable-red observation remains valid (the containing owned tile starts with many
red pixels), but its claimed8-byte read footprint is not established. Before
reusing fixed-source output, qualify explicit source-length programming there.
The current change does not silently alter that previously tested driver/image.

## Actual checks, recovery and next task

Host43/43 CTest PASS; encoder actual C ASan/UBSan PASS. Invalid base alignment,
range/high bound/null rejected without output mutation; independent control/address/
span decoding and captured-data/guard/status/completion mutations rejected.
Native GCC14.2 RV32IMAC/ILP32 runtime gate PASS, project+libgcc, no foreign symbols.
RAM entry4FF00100 and segment bounds verified inside4FF00000..4FF28000.
Final image SHA256 `c5a06dd084445025f0e314024b8cff93447da9e34ae33b09994dde2750affcc7`.
Runner HOST PASS; reset preserved flash, shell/SciCalc `2+3*4 = 14`/desktop PASS.
No flash/SD/parser/panel timing/rotation change. No visual test performed here.

```
cmake --build /tmp/maia-native-vdma-list-20261115 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-vdma-list-20261115/maia_sched_probe_ram.bin --log /tmp/maia-vdma-list-arlen-hardware.log --vdma-list --capture-seconds 8
```

Enable MIPI/VDMA_STOP/VDMA_TRANSFER/VDMA_LIST; count/park profiles excluded.
[Final capture](native-vdma-list-hardware.txt), [initial refusal](native-vdma-list-first-refusal.txt),
[sampling refusal](native-vdma-list-sample-refusal.txt), [desktop](native-vdma-list-desktop.txt),
[manifest](native-vdma-list-manifest.json). Recovery
`../maia-rtos-recovery/native-vdma-list-20261115/` preserves both refusal images,
final image, sources, pinned references, logs and hashes.
Next: use bounded <=15 descriptors for native display spatial color test, with
explicit source length, owned writable descriptor status, guarded publication,
completion/drain/recovery and fresh visual readiness. Prior160-LLI plan cannot be
activated directly. Full framebuffer/PSRAM/native desktop remain unimplemented.
