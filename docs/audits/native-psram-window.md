# Native mapped PSRAM: 2 MiB CPU data qualified — 2026-11-15

**HARDWARE-TESTED, LIMITED.** Ports the pinned Espressif HEX controller setup,
PSRAM pad configuration, MMU mapping and cache startup algorithms to native
MaiaRTOS C. Apache-2.0 notices/source revision retained. No SDK headers/runtime,
FreeRTOS shim, allocator or scheduler dependency. Sources: [hardware evidence](../hardware/EVIDENCE.md#native-psram-cpu-window-port-prerequisites--2026-11-15).

Boot/core0/IRQs-off, all external-memory/cache clients stopped and all code/data
in qualified low SRAM/ROM. Requires genuine private XTAL20MHz and X8 claims,
known warm device latency, calibrated existing supply and encryption disabled.
Twenty PSRAM pads and eight controller words are checked; flash pads unchanged.
Maps physical[0,2MiB) at4B800000..4BA00000 with32x64KiB MMU pages, slots896..927.
AXI opens only after mapping/configuration/cache invalidation; sync writes back
and invalidates the owned range then drains controller FIFOs with bounded polls.
Stop synchronizes, closes AXI, restores all32 MMU entries/selector and28 words.
Failures are sticky and do not permit blind retries or unowned cleanup.

ROM cache bootstrap globally initializes volatile cache/flash-MMU state for this
exclusive boot, not reversed by window_stop; no flash storage write. ROM services
are immutable chip code, not linked SDK code. Native polling is bounded; ROM wait
WCET and bus stalls are not established. Actual cache256KiB/line64/ways8 keeps
qualified low160KiB RAM image separate from cache SRAM allocation.

Actual one RAM upload, image SHA
23c512c93d72fc111a417829246eb73b3b6dc511e733bbaeebbcf10a408253cf,
24304bytes/entry4FF00100. CPU wrote and reread all524288 words over2MiB after
cache synchronization, bytewiseFNV2E4E04FE matched independent expected hash.
Direct command-port read after AXI closure independently matched first4096 bytes,
confirming cache-to-device visibility. All MMU entries restored fromC00..C1F to0;
controller/pads restored, actual AXIC0000000, X8->X16 MR8=4B and original clocks
restored. First4KiB saved bytes restored and reread byte-exactly. Remaining2MiB
array data was deliberately overwritten as exclusive volatile test memory and is
**not restored**; reset reloads the preserved full application. Earlier complete
four-pattern4KiB grouped test also passed in this run. Strict runnerPASS.

Host51/51PASS; actual driver ASan/UBSanPASS; six runtime mutants rejected (missing
mapping, missing restore, ignored FIFO, omitted sync, retry after fault, ignored
ROM failure). C++98 header and invalid profilePASS. Independent pinned SDK oracle
checks all28 register/pad plans, MMU, FIFO and encryption facts;14 source hashes
match revision. Final host-only fixture additions test encryption parity and AXI
closure ordering/dropped close; focused test and sanitizers/mutants rerunPASS.
Native source/image unchanged after target; manifest distinguishes host-only delta.
Standalone GCC14.2 strict native build/runtime dependency gatePASS.

[UART](native-psram-window-hardware.txt), [desktop](native-psram-window-desktop.txt),
[manifest](native-psram-window-manifest.json). Artifacts/source/checks preserved at
`../maia-rtos-recovery/native-psram-window-20261115/`. Reset shell/SciCalc2+3*4=14/
module exit0/desktopPASS. No flash/SD/parser/display timing/rotation changes.

This proves ordinary CPU access to an owned2MiB window, enough capacity for one
1843200byte RGB565 framebuffer. It does not prove full32MiB capacity, DMA visibility,
LCD-rate throughput, cold PSRAM startup or production concurrent memory service.
At nominal20MHz, higher operating speed/burst qualification remains necessary for
existing DPI rate. Next qualify clock/DMA and present a full framebuffer, then
continuous driver/acceptance and touch. Full installed desktop remains the original
ESP-IDF/FreeRTOS fallback; independent diagnostic imports none of that runtime.
