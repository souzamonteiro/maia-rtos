# Native owned SRAM source tile and finite DSI DMA list — 2026-11-15

**HARDWARE-TESTED, LIMITED: CPU memory construction/readback only.** The isolated
RAM diagnostic constructs an eight-row RGB565 tile and160 finite linked-list
items in owned SRAM, verifies their contents/guards and retains panel commands.
DMA registers are not programmed, no list is published to the engine, and no DMA,
video or pixel output is enabled. Descriptor fetch/handshake/pixels remain untested.

## References and representation

Pinned ESP-IDF `f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` primary sources:

- [DW-GDMA LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/dw_gdma_ll.h),
  `dw_gdma_link_list_item_t` and `lli_set_*`:64-byte item, source/destination high
  words0, BLOCK_TS items-minus1, next pointer master bit0/physical address bits31:6,
  zero terminal pointer, control words at offsets20/24hex. LLI valid31/last30.
- [Register types](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/dw_gdma_struct.h)
  `dmac_chn_ctl0/1_reg_t`, and [enums](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/include/hal/dw_gdma_types.h):
  memory master1/DSI master0, incrementing source/fixed destination,64-bit widths3,
  source512/destination256 burst-item encodings8/7, ARLEN/AWLEN fields16 and enable
  bits set. CTL0=001E1B41, base CTL1=00108840; valid bit always, last bit final only.
  No interrupt/status-fetch/writeback flags. Reserved/status/high words start0.
- [Address map](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/soc/esp32p4/register/soc/reg_base.h)
  `MIPI_DSI_BRG_MEM_BASE` is **50105000**, not50100000.
- [DPI initialization](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/esp_lcd/dsi/esp_lcd_panel_dpi.c)
  `dpi_panel_init` supplies memory-to-fixed-DSI64-bit linked-list transfer and marker
  facts. Native encoder/validation/repeated-tile policy is original MIT, no SDK
  headers/runtime/allocator/driver control flow imported. The SDK's marker helper
  comment mentions shadow transfers while DPI setup applies it to LLIs; actual
  native list fetching/termination must be verified before claiming DMA support.

An independent **host-only** oracle compiles the pinned SDK register types/enums/
address map to compare encoded controls and destination. Those headers are not
part of any MaiaRTOS target source or runtime. Snapshots/hash checks establish the
SDK files match the pinned revision. No new third-party implementation imported.

Memory/ROM facts reuse [native DMA memory proof](native-vdma-transfer.md): chip1.3
[TRM Pre-release v0.4](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf)
section7.3.1 pp451–454, CPU NC8FF alias versus physical4FF; pinned ROM cache API/
entry bindings (4FC003FC writeback-invalidate,4FC003C8 line size, L1D mapbit4).
Immutable silicon ROM calls, no SDK libraries. No bounded-WCET claim for ROM code.

## Scope and memory ownership

Private SoC header `dsi_dma_descriptor.h` validates a diagnostic request and encodes
one16-word item; no MMIO/heap or public driver ABI. Inputs require bounded positive
width/height<=4095, width multiple4, nonzero tile rows dividing height,128-byte
aligned disjoint source/list capacity spans wholly inside4FF00000..4FF28000,
sufficient capacities, and a valid item index. Checks precede output writes.
Arithmetic is bounded before products/end sums. Caller owns output storage and
source/list capacity; numeric range validation is not proof of allocation rights.
No foreign buffers or generic production memory capability API is implied.

Fixture uses720x1280 native geometry, repeated8-row tile11,520bytes and160x64-byte
items10,240bytes. Every item reads the SAME tile start, source increments within
its block, fixed DSI destination;1440 64-bit items per block. This describes a
finite total1,843,200bytes without storing a full framebuffer. Tail LLP exactly0,
only tail LAST; no circular list or producer refill. No engine has fetched this
representation yet, so frame coverage is currently a plan/encoding result only.

Opt-in linker profile reserves22,272bytes NOLOAD,128-byte aligned, before stack,
after BSS, below4FF28000. Default linker/production image unchanged. There are four
128-byte guards: before/after tile and before/after list. Startup does not zero
this reservation through its cached alias. After clock/DMA-quiescence checks, ROM
writes back/invalidates ONLY this reservation, and CPU uses only NC alias for fill,
encode and readback. Supported cache-line sizes16/32/64/128. Fences publish stores
and precede observation. Core0/IRQs-off/exclusive ownership is required; no SMP
allocator/cache management contract is added.

Full tile uses little-endian red/green/blue/yellow180-pixel bands in each of8rows.
All160 items are read back against expected words. The host independently builds
the payload/list and verifies FNV-1a32 checksums; checksums are diagnostics, not
cryptographic integrity. First/middle/last item fields also printed. Four guard
regions and clock/DSI/CSI/DMA/channel snapshots must remain unchanged; native
post-memory power reads must succeed. Construction does not establish RAM retention
under concurrent DMA or actual pixel presentation/orientation.

## Actual validation and limitations

Host42/42 CTest PASS, including independent field decoding, exact aggregate span,
one-item/final-list handling, null/index/geometry/overflow/alias/alignment/capacity/
overlap rejection without output changes. ASan+UBSan encoder fixture PASS; C++98
private header PASS. Six compiled mutants rejected: wrong destination, wrong
source master, circular tail, missing LAST, wrong item count and accepted overlap.
Pinned SDK oracle PASS. Invalid profiles reject; the prior VDMA profile still builds with its original
66,048byte reservation/runtime gate. Tile profile excludes standalone
VDMA transfer. Native GCC14.2 RV32IMAC/ILP32/runtime gate PASS (project+libgcc only,
no unresolved/foreign runtime). Known linker RWX/removed empty segment warnings.

One final RAM load on Tab5 chip1.3/40MHz/cu.usbmodem1301:

- Complete native supply/reset/ST7121/power/format/stage/clock/bridge chain PASS.
- BSS end4FF0FC40, reservation4FF0FC80..4FF15380, stack starts4FF15380.
  CPU alias8FF0FC80, tile physical4FF0FD00, list physical4FF12B00, cacheline64.
- Plan source2D00/list2800/countA0/frame1C2000; samples indices0/50/9Fhex,
  source4FF0FD00/destination50105000/items5A0, next4FF12B41/4FF13F41/0,
  CTL0=1E1B41/CTL1=80108840, tailC0108840, other words0.
- Source hashE18C2E45/list hashF8CAF0B9 match independent host construction;
  guards1/preserved1/inert1/repeated_bytes1C2000. No DMA/channel/LLP publication.
- Four power9C words, stage10/headers3/polls6/one word/errors0/modeB0000 restored,
  mask5, PHY15BD/FIFO50015. Runner PASS/reset to preserved flash firmware.

Image SHA256 `dfc11fe6b6012e7411e3cfaac2175c80383ee53ca307ab9ccc18d5a85d69f57b`.
Entry/load segments inside tested RAM window. Subsequent CMake invalid-profile
exclusion rebuild was byte-identical; no additional hardware run needed. Preserved
flash shell/SciCalc `2+3*4=14`/desktop recovery is captured separately. No flash/SD,
parser/EBNF, panel table, timing, existing DSI MMIO driver or production rotation changes.
Earlier visible bars were horizontal; logical1280x720/production90-degree software
rotation remains separate from720x1280 native geometry. Complete installed desktop
still FreeRTOS-based; native diagnostic is independent. POSIX remains DESIGN.

[Hardware capture](native-dpi-dma-tile-hardware.txt), [desktop](native-dpi-dma-tile-desktop.txt),
[manifest](native-dpi-dma-tile-manifest.json). Recovery
`../maia-rtos-recovery/native-dpi-dma-tile-20261115/` retains tested image/map, source,
actual logs, SDK/TRM/ROM facts, oracle/check scripts and original firmware hashes.

```sh
cmake --build /tmp/maia-native-dpi-dma-tile-20261115 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dpi-dma-tile-20261115/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dpi-dma-tile-20261115/maia_sched_probe_ram.bin --log /tmp/maia-dpi-dma-tile-hardware.log --dpi-dma-tile --capture-seconds 15
python3 /tmp/maia-dpi-dma-tile-checks.py
```

Configure all DPI_BRIDGE prerequisites plus `MAIA_NATIVE_PROBE_DPI_DMA_TILE=ON`.
Next: checked channel/list/DSI handshake and bridge flow-control staging while
channel/controller/output remain off; then a bounded transfer and stop/recovery
with actual pixels, respecting source/list lifetime and possible engine writeback.
No descriptor fetch, transfer, bridge live latch, pixel output, framebuffer,
PSRAM, native desktop or production rotation/input mapping proof in this unit.
