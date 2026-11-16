# Native operational PSRAM clock and DMA qualified — 2026-11-16

**HARDWARE-TESTED, LIMITED: nominal200MHz warm boot diagnostic.** Native adaptation
of Espressif analog-I2C read, clock dividers, DQS pin/DLL setup and four-phase
selection algorithms. Source revision/license/facts: [evidence](../hardware/EVIDENCE.md#native-operational-psram-clock-port--2026-11-16).
Analog-I2C module and phase helper retain Apache2 notices. No SDK/FreeRTOS headers,
linked runtime, scheduler shim or ROM analog/SPI helper. Independent native image;
installed full firmware remains the original ESP-IDF/FreeRTOS fallback.

## Implementation and ownership

Boot/core0/IRQs-off, all PSRAM/cache/analog-I2C/calibration clients stopped, code/
buffers in internalSRAM/ROM. Requires native XTAL20/X8 claims; device latency
profile checked by X8 claim at20MHz. Read existing powered/calibrated MPLL register2
(slave63) twice via native bounded analogmaster0, restore routing words. Existing
LP clock/reset, MPLL power/enable/calibration guards; no PLL retuning/power/reset
writes or analog write commands. Accept only calculatedMPLL400 with known40MHz
XTAL. This is not a physical oscillator/clock measurement or cold PLL startup.

Fast start enables DLL timing-calibration bit5 in **both MSPI0+180 and+190**, stages
all20 PSRAM pad drive/DQS enable words, then gates core clock before source switch.
MPLL/corediv1 and busdiv2 gives nominal200MHz (100MHz/div4 also HOST-TESTED only).
No flash pads or flash clock programming. Stop requires closed/restored window,
returns toXTAL20, restores20 pads and both DLL words, then xtal20_stop restores
original firmware dividers/mux. Private claims/sticky faults prohibit forged-token
cleanup/retry; all modified controller words checked. Clock report attempt mask
covers its five clock stages; pad/DLL ownership is additional native guarded state.

Mapped window now accepts the genuine private fast profile. At deliberately
unqualified phases it relies on the X8 profile already checked at20MHz, rather
than issuing mode commands and poisoning the command engine on expected bad data.
No foreign device-mode writer may run under this exclusive contract. Ordinary20MHz
window retains fresh MR8 read. Configure/restore/controller/cache/MMU contracts
remain in [window audit](native-psram-window.md). MMIO poll loops bounded; immutable
ROM cache waits/bus stalls have no measured worst-case bound. No thread/ISR/SMP
memory service or allocator capability is established.

Reference2MiB is initialized/synchronized/read-verified at20MHz first. Four DQS
phases0..3 (67.5/78.75/90/101.25degrees) each run read-only9x4KiB DMA address/burst
checks. Expected data mismatches may inform training **only after** natural stop,
guards, config and window restoration succeed; control/stop/ownership errors remain
fatal. Choose first phase of longest consecutive passing run, no zero-pass fallback.
Then repeat the full2MiB CPU write/read,9 DMA cases and low-clock direct-device
read/recovery. This ports phase selection with stronger DMA data checks; it does
not implement the SDK31-point delayline sweep or temperature/voltage margin testing.

## Actual target evidence

Final one RAM upload SHA
85168103973a0ca39828eb8ee955289e11f1b8ece72139ab72370fab3c845af1,
33056bytes at4FF00000/entry4FF00100. MPLL raw99 twice => calculated400MHz,
power09000000/clock1FFcfffd/cal34C,6 analog polls/routing restored. DLL+180/+190
actual1->21->1, fastPERID00C/buses10001, restoredXTALPERIC00C and ultimately
originalD00C/buses30103. All nine roots/27 snapshot words restored; actualAXI
C0000000 before/final, deviceMR8 restored4B/X16, all32 MMU pages restored.

Baseline524288/524288words at20MHz. All four candidate phases passed9/9 copies,
36864/36864bytes each, maskF/selected0. Final200MHz CPU524288/524288words/FNV2E4E04FE,
all final9x4096 DMA bytes/guards correct, raw511/naturalstop/errors0/common0 each.
Total45 DMA copies/184320 matched bytes over training plus final qualification.
Final4KiB copy samples: single188–189us,4items55–56us,16items22–23us, versus prior
20MHz16-item170–171us. These samples include software/timer overhead and are not
sustained PSRAM->DSI throughput, hard latency guarantees or other-clock proof.

Post-sync directSPI4096bytes match expected source at20MHz; saved first4KiB restored
and reread4096/4096. Rest of reserved2MiB deliberately overwritten and **not data-
restored**, as earlier mapped diagnostic. Global volatile cache/flash-MMU boot
state remains owned until reset; no flash storage write. All five DMA batches
restored12 channel words with DSI/CSI/otherchannels preserved; progress/status/W1C
history is not reversible. Full grouped prior four-pattern command test alsoPASS.
Strict host evaluatorPASS, then reset shell/SciCalc2+3*4=14/moduleexit0/desktopPASS.

## Refusals retained and resolved

1. [First refusal](native-psram-fast-first-refusal.txt), imagecbe78113: fast register
   selection worked but MR8 read refused IO. Vendor prepares DQS before raising
   clock; native sequence had prepared pads only later in window setup.
2. [Prepared pads](native-psram-fast-burst-refusal.txt), image8cba12a3: CPU2MiB passed,
  16-item DMA matched4085/4096; not accepted or described as successful.
3. [Samples](native-psram-fast-samples-refusal.txt), image726e99c7:4083/4096, persistent
   bit1/2 differences with correctCPUsource reads; delayed reread remained4083.
   This distinguishes the observed corruption from merely late destination data.
4. [Phase-only sweep](native-psram-fast-phase-refusal.txt), image1333f0fc: all four
   phases only6/9 passing cases, mask0; correctly refused with no phase fallback.
   Review of vendor psram_ctrlr_ll_enable_dll showed both DLL paths were still
   disabled in RAM-loader snapshot(+180/+190 both1); enabling them made all four
   candidate phases and final full checks pass. No guessed timing delay was used.

Each failed source/image/log is preserved in corresponding recovery sibling
`native-psram-fast200-{first,burst,samples,phase}-refusal-20261116/`. Exactly five
actual uploads in this unit, all reset to preserved application; no visual claim.

## Validation and remaining work

Host53/53PASS. Actual MPLL/clock driver ASan+UBSanPASS; six runtime mutants rejected
(PLL write, missing routing restore, ignored repeat, omittedDLL, wrong busdivider,
skipped fast-stop mux). All16 phase masks/zero-pass refusal tested; fast pad/DLL/
clock dropped writes and sticky errors, fast window100/200 geometry and negative
profiles, malformed/duplicate/missing/data-error capture testsPASS. Independent
pinned SDK register/struct-offset/DQS/MMU clock oracle and15 source hashesPASS.
C++98 public headerPASS. Last extra DLL assertion is host-only after target and
was rerun with focused test/sanitizers/mutants; native source/image unchanged.
Standalone GCC14.2 strict build/runtime dependency gatePASS (project+libgcc only).
No parser/lexer/EBNF, flash/SD or display timing/rotation changes.

Build/profile: `cmake ... -DMAIA_NATIVE_PROBE_PSRAM_FAST=200` with STATE/ID/CLOCK/
ARRAY/WINDOW/DMA allON; `cmake --build ... --target maia_sched_probe_ram`; standalone
`python -m esptool --chip esp32p4 elf2image .../maia_sched_probe_ram.elf`.
Runner: `tests/hardware/run_native_scheduler_probe.py --port /dev/cu.usbmodem1301
--image .../maia_sched_probe_ram.bin --log ... --psram-fast 200 --capture-seconds 30`.

[UART](native-psram-fast-hardware.txt), [desktop](native-psram-fast-desktop.txt),
[manifest](native-psram-fast-manifest.json). Final archive/sources/checks at
`../maia-rtos-recovery/native-psram-fast200-20261116/`. Preserved fallbackbin/ELF
hashes unchanged. Next grouped delivery: owned full1843200byte RGB565 framebuffer
PSRAM->DSI and prepared orientation-aware visual test. Cold power/device startup,
full32MiB capacity, full delayline/margin qualification, sustained/concurrent LCD,
recovery acceptance and touch integration remain incomplete. No permission stop
is needed for internal implementation/electronic tests under owner's instruction.
