# Correct native PSRAM AXI exclusion guard — 2026-11-15

**HARDWARE-TESTED, LIMITED.** Corrects a native guard defect found while porting
memory mapping, before enabling AXI. Earlier mode/clock/array code tested CTRL1
+0Cbit0 (clock mode) instead of actual CACHE_FCTRL+3Cbit0. Existing byte-pattern
results remain real, but earlier AXI-off assertion was not register-verified;
boot-exclusive/all-internal/no-live-client preconditions were still required.

[Evidence](../hardware/EVIDENCE.md#psram-axi-guard-register-correction--2026-11-15)
cites pinned SDKf5c3654 reg/struct/LL facts. New shared psram_axi_quiet reads
5008E03C, requires AXI_REQ_EN0 and CLOSE_AXI_INF_EN1. Guards in mode/clock/array
check initially and during ownership verification. No controller read while core
gated; root checks precede guard. No register/clock/power/device-mode programming
added by this fix. Caller exclusivity remains mandatory; mask check is not an
atomic ownership mechanism or proof against every unobserved producer.

Host50/50PASS, actual array port ASan/UBSan and six compiled runtime mutantsPASS,
C++98 and invalid profilePASS. Independent pinned SDK oracle checks address,
struct offsets(CACHE_FCTRL3C vs CTRL1C), bit0/bit31 and direct helper responses
for C0000000/C0000001/40000000/0. Mode/clock fixtures separately reject enabled
and open AXI even when old CTRL1bit0 is clear. Native GCC14.2 runtime gatePASS.

Actual one RAM run SHA4d96fb7a71a3b79ba2f20e5d4c1dc8a055a1212da255545d03edd71b9a5567a2,
18592bytes/entry4FF00100. New PSRAM_AXI/FINAL records bothC0000000. Full grouped
X8 mode/data test repeated: four exact4KiB patterns, all eleven transfers, saved
content/mode4B/clocks/27 snapshot words restored, strict hostPASS. This establishes
actual quiet flags for this new run, not retroactive register observations.
Stronger evaluator requires both fields; prior captures retain their historical
runner version/results. No acceptance relaxation. Reset then maia.py shell/
SciCalc14/desktopPASS; no flash/SD/parser writes or mapped PSRAM activation.

[Hardware](native-psram-axi-hardware.txt), [desktop](native-psram-axi-desktop.txt),
[manifest](native-psram-axi-manifest.json). Sources/artifacts/checks preserved in
`../maia-rtos-recovery/native-psram-axi-20261115/`. Original array port provenance,
X16 addressing refusals and limits remain in [array audit](native-psram-array.md).
Next adapt controller/memory mapping, keeping corrected guard before activation.
Cold startup/cache/MMU/full-framebuffer/continuous LCD/touch remain incomplete;
installed full desktop still original runtime, independent native tests use none.
