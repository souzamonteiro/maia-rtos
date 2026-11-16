# Ported PSRAM mode and array data path — 2026-11-15

**HARDWARE-TESTED, LIMITED.** Port of Espressif's FIFO read/write algorithm,
with checked native X8 mode selection, passes four byte-exact patterns over a
reserved4KiB on Tab5. Saved bytes, original X16 mode and clocks restored before
reset; desktop/SciCalc14 recovered. Warm boot-exclusive command path only, not
cold startup, ordinary mapped memory, full capacity or continuous LCD.

## Port, sources and ownership

[Primary evidence](../hardware/EVIDENCE.md#ported-hex-psram-training-array-transfers--2026-11-15)
records pinned SDKf5c3654 `mspi_timing_by_dqs.c`, read_data/write_data algorithm:
FIFO chunks<=64bytes, offset/pointer advance; SYNC0000/8080, command16/address32,
200M configured-latency profile read26/write12 dummy. `psram_ctrlr_ll` selects
OPI_DTR/CS1; HEX source CONFIG_SPIRAM_USE_8LINE_MODE supports device X8 selection.
`soc_psram_array.c` is a licensed Apache-2.0 adaptation with original copyright,
revision/modification notice and existing LICENSES/Apache-2.0.txt. Native bounded
command engine replaces SDK/ROM transaction calls, critical sections and runtime.
Host-only SDK oracle verifies MOSI/MISO phases/lengths and FIFO addresses; target
imports only project objects+libgcc. No FreeRTOS/Arduino/SDK headers or runtime.

AP Memory [APS256XXN-OBx9](https://www.apmemory.com/en/downloadFiles/0325021716tb786912)
rev1.2 Jan3,2025, p11 §§7.1–7.4/address table, p16 §7.7 Figures12/13, p21 Table19:
X16 is word-addressed and ignoresCA10; mode access uses low8lines even inX16;
MR8bit6 can switch X8/X16 after power-up. One-byte MR8 write uses no dummy phase,
consistent with SDK mode-write transaction. Reference is protocol-family evidence,
not proof of exact in-package PN. PDF downloaded from primary site, SHA in
manifest, p11/16/21 rendered/visually inspected using macOS PDFKit after Poppler
was unavailable. PDF/text/renders retained in recovery, not redistributed in Git.

Native public APIs in esp32p4_psram.h: X8 mode claim/restore and bounded training
transfer. Boot/core0/IRQs-off, all code/data/report/buffers internal SRAM, exclusive
whole PSRAM/no live cache/AXI clients. Caller data/report disjoint. Clock must be
qualified XTAL nominal20MHz; temperature/high-speed margins not inferred.
Checked vendor/32MiB/device/KGD/latency/burst profile via mode reads before data
access. X8 start requires exact4B, writes0B via C0C0/reg8/8bits, verifies readback;
private claim required for array access. X8 stop requires exact0B, writes/readback
4B. Unknown/already-X8 device is not silently adopted. No arbitrary mode writer,
reset, LDO or clock operations exposed by this port. Device-mode errors sticky.

Data aperture physical[0,4096) applies ONLY AFTER verifiedX8 selection.4byte
aligned offset/length, subtraction-based overflow/bounds checks, length<=4096,
read/write0/1, per-command budget1..100000. Whole transfer splits into64byte
chunks. Save ten config words plus16FIFO words; verify masked native setup,
program phases/lengths/CS1/DDR and byte-packed FIFO; bounded literalUSR command.
Only idle+ownership permits reverse exact configuration/FIFO restore. Read bytes
committed after chunk restore; reports retain completed prefix and command polls.
No active timeout cleanup, hidden retry or uncertain rollback. First post-claim
error sticky until reboot. Controller/roots guards prevent reads after gate loss.

Command idle/start-write attempt is not a device ACK; dropped read can return
poison and dropped write can leave data unchanged. Host models exercise both,
and target acceptance requires independent byte-pattern readback. FIFO after
engine execution is engine-produced in either direction; configuration remains
strictly guarded. No software claim of an observed busy edge for fast completion.
Successful writes may have modified array even if later chunks fail. Probe
restores saved array only after all verified operations; uncertain failure resets.

## Actual failures retained and resolved

Initial image4182748f20727c542b08a234d4e9bb3bcb5cdf0bb9594452e428a281ad61adca
ported small SDK training transactions while inheritingX16. Zero/FF/AA55 patterns
passed but random pattern matched2060/4096, hashA4DEE15D vsE4B8E234: native/host
FAIL, no original-content restoration claimed. Sample-only second image
d637522bd20a5e0491a23ec6b3c0fa216ce8ac0c3b74a02d790503ccbed7e1e4
shows read0..127 equals expected1024..1151. Primary address table then identifies
CA10 alias in X16. No unstable timing diagnosis or guessed address remapping.
The training-only reference byte-offset loop is insufficient to establish a
linear byte aperture under inheritedX16. All PSRAM was exclusive/volatile and
no flash/SD/live external-memory client was touched. Both captures/images kept;
runner resets to preserved flash after each. Only final recovery was separately
verified with shell/SciCalc. Correct adaptation includes explicit X8 mode switch
and refuses array use without private X8 claim; tests require switching back.

During development a mutant helper initially failed to compile due parentheses;
corrected before counting rejected runtime mutants. Native compiler also caught
unsigned-vs-uint32_t public definition mismatch (RV32 uint32_t is unsigned long),
fixed before final target image. Neither was represented as hardware evidence.

## Final validation

- Host **50/50 CTest PASS**. Actual port ASan+UBSan PASS; C++98 public header,
  invalid ARRAY-without-CLOCK profile rejection and pinned register oracle PASS.
  Six compiled mutants rejected: ignorebounds, wrongreadlatency, omitreadcopy,
  omitrestore, ignoretimeout, retryfault. Model checks full4KiB and partial chunks,
  overflow/endpoints, all53 data setup/start/restore drop positions (unchanged
  stores need no detectable ACK), completed64byte prefix then active timeout,
  root loss without gated readback, profile/claim refusal, duplicate/no-claim
  mode requests and dropped mode trigger rejected by device readback.
- Native GCC14.2 RV32IMAC/ILP32 strict build/runtime gate PASS. Existing linker
  RWX/empty-flash-segment warnings unchanged; no memory-isolation assertion.
- Final image SHA256830fd7d2a29e102a7a2a522aa9ad340851b4ecddf94f74a5e8288f15d56d32a3,
  entry4FF00100/load4FF00000/18320bytes, internal static buffers outside image
  BSS/stack correctly reserved by linker. One final RAM upload with host-readyC;
  initial identity/clock gates pass. Mode4B->0B readback stage3/result0.
- **Eleven4096byte transfers /704chunks /45056bytes issued-or-read**, every call
  result0/completed1000/chunks40/polls40/command0/restored1. Write-attempt flags
  only on writes. Four patterns each4096/4096 matched; FNV32 hashes:
  zero76EFDDC5, FF34E76DC5, AA550F3A4DC5, seededLCGE4B8E234. Independent host
  generator matches all bytes/hashes; no weakened acceptance of first failure.
- Saved original bytes restored/read back4096/4096, hashD3D6E224 before/after;
  internal guards kept. Mode0B->4B exact readback stage3/result0, then clock
  stop restores D00C/30103/30103; nine roots/27 controller words unchanged.
  Strict host ARRAY evaluator PASS, both refusal captures still evaluateFAIL.
- Reset then maia.py shell echo/SciCalc2+3*4=14/module exit0/desktop PASS.
  Preserved firmware bin/ELF hashes unchanged; no flash/SD or parser writes.
  No owner observation/approval needed for these electronic tests.

```
cmake -S . -B /tmp/maia-native-array -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_PSRAM_STATE=ON -DMAIA_NATIVE_PROBE_PSRAM_ID=ON -DMAIA_NATIVE_PROBE_PSRAM_CLOCK=ON -DMAIA_NATIVE_PROBE_PSRAM_ARRAY=ON
cmake --build /tmp/maia-native-array --target maia_sched_probe_ram -j4
python -m esptool --chip esp32p4 elf2image /tmp/maia-native-array/maia_sched_probe_ram.elf
python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-array/maia_sched_probe_ram.bin --log /tmp/maia-array-x8-hardware.log --psram-array --capture-seconds 20
```

[Final hardware](native-psram-array-hardware.txt), [first refusal](native-psram-array-first-refusal.txt),
[samples refusal](native-psram-array-samples-refusal.txt), [desktop](native-psram-array-desktop.txt),
[manifest](native-psram-array-manifest.json). Full sources/build/check scripts/PDF
and hashes in `../maia-rtos-recovery/native-psram-array-20261115/`; first/samples
images in sibling `native-psram-array-first-refusal-20261115/` and
`native-psram-array-samples-20261115/`. Tested native sources exactly match archive.

## Delivery boundary and next implementation

Implemented: adapted mode/data streaming path, reservation/bounds/partial-transfer
contracts, actual byte-pattern/restoration proof and grouped electronic tests.
Not implemented: cold native supply/latency/DQS startup, AXI/cache/MMU/capability
mapping, full32MiB stress, operational-speed/bandwidth qualification, full-frame
LCD/continuous tasks/IRQ/touch or native desktop. This4KiB result cannot substitute
for framebuffer ownership or prove full capacity. Installed complete desktop
still original ESP-IDF/FreeRTOS fallback. Next port SDK controller/memory startup
and mapping, then full framebuffer per [four LCD gates](../plans/LCD_TOUCH_NATIVE.md).
Owner now explicitly prefers grouped functional deliveries, existing licensed
algorithms and autonomous internal tests; per-substep permission stops removed.
