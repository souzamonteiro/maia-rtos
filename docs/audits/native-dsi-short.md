# Bounded native LP DCS short writes — 2026-11-13

**HARDWARE-TESTED, LIMITED.** New synchronous boot-exclusive transfer API
publishes one DCS short header, observes bounded host drain/PHY stop, accumulates
errors and never automatically retransmits an ambiguous attempt. No long payload,
read/BTA/panel ACK support or complete panel/display initialization claim.

## Primary references and attribution

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[mipi_dsi_hal.c](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c)
`mipi_dsi_hal_host_gen_write_dcs_command` short cases and
[host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h)
`gen_set_packet_header`, `set_dcs_short_wr_speed_mode`, speed enum LP=1.
[Types](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/include/hal/mipi_dsi_types.h)
DT05=DCS0/DT15=DCS1. Header is parameter<<16|command<<8|VC<<6|DT at host+6C;
CMD_MODE_CFG+68 LP bits16/17 for DCS0/1, TE/ACK bits0/1. VC encoded in transmitted
header; RX GEN_VCID is not written because there is no BTA/read/ACK operation.

Pinned host register header CMD_PKT_STATUS+74 low7 empty0/2/4/full1/3/5/readbusy6;
INT_ST0+BC defined bits0..20 (mask1FFFFF), INT_ST1+C0 bits0..12/19 (mask81FFF).
Native code never writes IRQ status/masks or blindly clears flags. Status reads
may consume latched diagnostics; their side effects are not assumed lossless.
Observed bits are OR-accumulated and any defined error causes immediate failure,
including historical errors before publication. No read-then-clear-and-ignore.

User-supplied [ST7123 specification V0.5](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/ST7123_SPEC_Preliminary_V0.5.pdf),
2021/09, section6.2.1 p140 describes NOP00 as having no display effect and available
in sleep-in/sleep-out. Local copy at docs/hardware/datasheets, source URL in
`download.sh`, bytes/hash retained in recovery manifest. This is the sole hardware
test command; it does not prove panel acceptance. SDK `esp_lcd_panel_commands.h`
also defines LCD_CMD_NOP00. No panel reset/sleep/display/brightness command sent.

Adapted header packing/LP selector in `dsi_short_packet.h` retain Espressif
Apache-2.0/copyright and `LICENSES/Apache-2.0.txt`; validation/guards/bounded policy
are original MIT in existing native DSI module. No SDK includes/runtime/locks or
unbounded FIFO loops imported. Previous TRM chip1.3 Pre-release v0.4 10.2.2 p616/
regs10.11–10.16 pp651–659 remain reference/clock prerequisites;
[primary TRM](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf).

## Contract and completion semantics

`maia_esp32p4_dsi_dcs_short(vc,command,count,parameter,budget,report)` requires
successful internal native command-config claim, fixed960Mbps PHY/reference20MHz,
chip1.3/core0/IRQs-off/exclusive pre-scheduler RCC/PMU/VDMA/DSI/IRQ-status ownership,
stable supplies/inherited SPLL and no external producer. Not ISR/task/SMP/reentrant
or a runtime teardown service. Inputs vc0..3/byte command/parameter, count0..1,
unused parameter0, budget1..100000. Null report/invalid args/context reject without
MMIO. Missing claim/incompatible initial state BUSY, initial fresh drift VERIFY.

Initial host powered, PHY locked/all-stopped, no HS request, bridge/DPI/DMA quiet,
generic FIFOs empty/no readbusy, TE/ACK/BTA disabled. Full cached14 command-config
words must still match native setup, plus fresh RCC/provider/divider/LP/eFuse/PMU/
PHY controls. Select just the requested LP bit in CMD_MODE_CFG, preserve other
fields; skip matching configuration. Check bus clocks/reset BEFORE peripheral
readback, fresh state/error/idle check, then attempt exactly ONE GEN_HDR store.
No GEN_PLD+70 writes or reads; no GEN_HDR readback (it is publication, not a safe
configuration register). `header_attempted` is set BEFORE the store.

Polls first check RCC access, then FIFO/status/errors, then complete guard. Allow
lane-stop transitions during LP TX while requiring PLL lock; write/read payload
FIFOs remain empty and readbusy clear. Budget bounds loop observations, plus a
finite number of preflight/final observations; not an elapsed-time deadline.
Need FIFOlow7==15 and PHYstatus&95==95 simultaneously, then final fresh guard and
observation. Report stages0:none/1:guarded/2:LP configured/3:header attempted/
4:drain+stop observed/5:verified; OR errors, polling count and optional busy witness.

**OK means an attempted header and observed host FIFO drain/PHY stop. It does
not mean panel ACK, wire-complete proof or validated panel behavior.** A fast
packet can finish before busy is sampled. A silently ignored GEN_HDR store is
indistinguishable from that case without stronger hardware evidence; host fixture
explicitly demonstrates it and never claims detection. Actual bus faults invoke
the existing native trap policy. `saw_busy` records whether nonidle was observed.
No ACK requested, no RX read: panel success cannot be inferred from zero error bits.

Success permits the next exclusive call; LP selection remains set until reset.
Any failure after guards is sticky FAILED until reset (IO=-7/timeout=-6/etc), no
rollback/retry: packet may already have been accepted. Historical error also causes
sticky IO even before mode/header writes; flags may have been consumed by reading.
Before guards, clean busy/invalid state is write-free. This is a single-flight
boot transport, not a concurrent queue or lifecycle/ownership manager.

## Validation procedure

Host AppleClang21/Python3.14.5; native GCC14.2 RV32IMAC/ILP32. Fixture executes actual
native access/reference/PHY/command setup. Covers both DCS types/all VC values,
byte/unused/budget validation, full LP-mask preservation, repeated calls without
extra mode writes, one header only, no payload/GEN_HDR reads, delayed FIFO drain
and later lane stop, timeouts without retries, preexisting/read-clear/post-TX error
flags, root/supply/table drift and lost lock. Tests caught a missing fresh bus
check before mode readback under injected clock loss; fixed before target load.
They also exercise unobservable ignored publication vs fast completion explicitly.

```sh
cmake -S . -B /tmp/maia-dsi-short-host-20261113 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-dsi-short-host-20261113 -j4
ctest --test-dir /tmp/maia-dsi-short-host-20261113 --output-on-failure
cmake -S . -B /tmp/maia-native-dsi-short-20261113 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON -DMAIA_NATIVE_PROBE_VO3_CYCLE=ON -DMAIA_NATIVE_PROBE_DSI_ACCESS=ON -DMAIA_NATIVE_PROBE_DSI_REFERENCE=ON -DMAIA_NATIVE_PROBE_DSI_PHY=ON -DMAIA_NATIVE_PROBE_DSI_COMMAND=ON -DMAIA_NATIVE_PROBE_DSI_SHORT=ON
cmake --build /tmp/maia-native-dsi-short-20261113 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-short-20261113/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-short-20261113/maia_sched_probe_ram.bin --log /tmp/maia-dsi-short-hardware.log --dsi-short --capture-seconds 15
```

Opt-in hardware profile extends previous native command host; sends16 VC0/DCS0
NOPs, budget10000 each. Reports every publication/result/polls/LP state/errors and
busy witness; checks full previous configuration/root/PHY/supply after each call.
Summary explicitly panel_ack0. Complete ordered16-row capture required. Only NOP
is target-tested; parameterized/other-VC cases stay host-tested unless recorded.
Runner resets to preserved flash in finally, then paced maia.py recovery test.
No flash/SD/parser/default firmware changes in this unit.

## Actual target validation — 2026-11-13

[Hardware capture](native-dsi-short-hardware.txt),
[desktop recovery](native-dsi-short-desktop.txt),
[manifest](native-dsi-short-manifest.json). Tab5 ESP32-P4 rev1.3/40MHz,
ROM RAM load only; no failed hardware candidate. Tracked captures normalize line
endings/trailing whitespace, raw logs retained in recovery.

All native park/gate/VO3/access/reference/PHY/command predecessors pass. Sixteen
VC0/DCS0/NOP publications complete with stage5/header05/attempted1, each **two
poll observations** and busy1. Summary packets16/busy16/mode_changes1/preserved1/
panel_ack0. CMD_MODE_CFG initially0 ->10000 on first call, retained across all
later calls. Thus one LP selector change and sixteen GEN_HDR store attempts,
no payload/long/read/BTA/HS/video/panel-initialization packets. Hardware host busy
was actually witnessed in every call before drain/stop observations; no witness
was synthesized or made mandatory for hypothetical faster packets.

Final PHY_STATUS15BD and FIFO00050015 (generic low7==15) on each call. Error0/
error1 both0 throughout recorded observations. All previous14 config words, root
clocks/resets, PHY power/digital settings and full VO3/eFuse snapshots preserved.
No ACK requested, so no proof of panel reception/acceptance or functional display.
Actual byte-clock/LP waveform/bitrate, target electrical fault injection, ACK/read/
nonzero-VC/one-parameter writes are NOT hardware-tested by this unit. NOP's empty
command semantics come from the supplied ST7123 specification, not from a claim
that software could read back the panel after sending it.

Prior VO3 host windows25.017/25.019ms retain observation-only limits, not voltage/
discharge/settling proof. Hardware timeout counts remain0 from prior setup, while
this transfer's software poll limit is explicit. Polls are not milliseconds or
hard real-time worst-case bounds.

Host35/35 PASS; ASan/UBSan PASS; strict C++98 public header PASS. Five compiled
mutants rejected: wrong packet type, ignored error flags, skipped post-LP-write
bus guard, ignored lane-stop completion and retry after ambiguous publication.
Invalid short-without-command profile rejects. Capture tests require16 ordered
NOP rows/exact LP mask preservation/error masks/bounded polls/summary and explicit
panel_ack0. Complete-frame mode isolation and malformed/duplicate/missing output
reject. Native runtime gate PASS: project objects+libgcc, no foreign/unresolved
symbols. RAM image/entry validated below reserved ROM window. Existing linker
RWX/removed-empty-flash warnings unchanged; no new project warnings.

Paced maia.py recovery passes shell echo, SciCalc `2+3*4=14`, module exit0 and
desktop left active. Installed working desktop still uses preserved FreeRTOS
baseline, not the native RAM diagnostic. No flash or SD writes.

Implemented: reusable bounded boot LP DCS0/1 API, cached prior command table,
masked LP selection, one publication, accumulated error/failure/timeout reports,
no automatic retry, target NOP diagnostic and host/capture regression. Attributed
Apache-2.0 packing helper; no runtime/SDK headers, parser changes or blanket app
compatibility shim. Recovery: `../maia-rtos-recovery/native-dsi-short-20261113/`
with source/image/ELF/map, raw host/device logs, sanitizer/mutation scripts/results,
licenses, pinned SDK refs and local datasheet hashes/reference text.

Still unimplemented: long writes/payload streaming, BTA/ACK/read responses, panel
identification/reset/DCS startup integration, video/framebuffer/PSRAM/full native
desktop, runtime concurrent ownership/teardown and physical signal measurements.
Next small task: bounded LP long-write payload path needed by vendor panel startup,
with partial-publication ambiguity/failure accounting and safe target command;
then integrate verified panel-specific initialization rather than infer reception.
