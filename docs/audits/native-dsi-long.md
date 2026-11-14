# Bounded native LP DCS long framing — 2026-11-13–14

**HARDWARE-TESTED, LIMITED; final validation 2026-11-14.** Add payload staging before one DCS39 header,
byte-safe final-word handling and a shared aggregate polling budget. Transport
failures poison both long/short paths until reset so staged data cannot leak into
another packet. No panel initialization/ACK/read/video/PSRAM completion claim.

## Primary references and port

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[HAL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c)
`mipi_dsi_hal_host_gen_write_long_packet`/DCS long branch stage payload then publish
WC header; [host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h)
GEN_PLD+70/GEN_HDR+6C/CMD_MODE_CFG+68 DCS long LPbit19.
[Packet types](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/include/hal/mipi_dsi_types.h)
DCS_LONG_WRITE39. WC is byte length INCLUDING command, encoded bits8..23; VC6..7.
Only DCS supported here, no arbitrary generic datatype. Controller supplies packet
framing/ECC/checksum per vendor programming model; software does not append them
to payload or WC.

Supplied [ST7123 SPEC V0.5](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/ST7123_SPEC_Preliminary_V0.5.pdf)
2021/09 section5.4.2.3.1 p92, datatype p99, WC p102, payload pp103/109 describe long
framing; NOP00 section6.2.1 p140 has no display effect. Test uses only one logical
payload byte00 and WC1, never padded NOP parameters or a configuration command.
DT39 in the HEADER is a packet type, not a payload command39 (Idle Mode On).
Long-encoded NOP acceptance by the panel is not inferred; no ACK/readback exists.
The SDK raw long function accepts explicit sizes, unlike its automatic short/long
DCS selector. Choosing explicit long framing does not prove panel support.

`dsi_long_packet.h` carries Espressif Apache-2.0 attribution/license for adapted
packing path; bytewise packing removes SDK aligned32-bit casts and final partial
word overread. No unbounded SDK waits/runtime/headers imported. Native guards and
budget/failure policy original MIT. Existing chip1.3 TRM v0.4 ch10/p616/regs10.11–
10.16 pp651–659 clock facts unchanged;
[primary TRM](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf).

## Contract and failure semantics

`maia_esp32p4_dsi_dcs_long(vc,bytes,length,budget,report)` requires internal native
command-host claim and same exclusive chip1.3/core0/IRQs-off/boot context, stable
supplies/inherited SPLL, quiet VDMA/bridge/DPI, locked/stopped PHY and idle generic
FIFOs. ACK/TE/BTA disabled; LPbit19 only, other fields preserved. Validate vc0..3,
readable immutable ordinary RAM buffer, length1..64 including command, report not
aliasing buffer, budget1..100000. Bad inputs/context without MMIO. Length64 is a
**policy cap, not verified physical FIFO depth or guaranteed capacity**. Larger
requests reject, never truncate/split silently. No DMA/memory allocation/PSRAM.

Short and long use the same private transfer claim; shared guard helper names
clarify that reuse. Short API semantics unchanged except a failed long attempt
also blocks it. Command-host cached14 settings remain fully checked with fresh
RCC/reference/provider/divider/LP/supply/eFuse/PHY controls. No external token can
substitute for internal boot ownership. No runtime/concurrent queue/teardown.

Stage all words before header, checking interface payload-full bit3 AND internal buffer-full bit19 and a total observation
budget. Pack at most4 bytes little-endian from valid length, zero unused high bytes
in the last word, no read outside the provided buffer or unaligned32-bit access.
Increment payload word/byte ATTEMPTS before GEN_PLD store. No payload readback.
After all bytes staged, check command FIFO idle/PHY stopped/errors afresh and
publish header once with exact WC. A full payload FIFO at this point must NOT
block header (it can drain only after publication). Never publish a partial WC.

Budget shared among all payload-space/header/completion observations; not reset
per word, no multiplication by buffer size. Extra finite preflight/final checks
remain outside that loop budget. Pre-header PHY must stay locked/stopped and
command FIFO idle; read FIFO empty/not busy throughout. After header allow payload
full/nonempty and lane-stop transitions while lock stays set. Require final FIFO
maskF007F==50015 (interface and internal buffers empty/not full)/PHY95==95 plus guards/errors. No real-time/voltage/physical-rate promise.

Report embeds short-style observation data but completed0:none/1:guarded/2:LP/
3:payload attempted/4:header attempted/5:drain-stop observed/6:verified; plus payload
word/byte attempts, space-observation count and pre-header FIFO. Attempt counters
are not confirmed payload acceptance. Any failure after initial guards, including
partially staged/no-header timeout, sticky FAILED across BOTH APIs until reset.
No rollback, flush, automatic header/packet retry or compensating controller reset.
Initial clean busy is write-free; preexisting error causes sticky IO. Error/status
read-consumption semantics and no-ACK/fast/ignored-publication limitations from
[short transport](native-dsi-short.md) still apply. OK is host observation only.

## Verification

Host AppleClang21/Python3.14.5, native GCC14.2 RV32IMAC/ILP32. Tests lengths1..64,
VC0..3, buffers unaligned and ending exactly at allocation boundary under ASan,
all final-group sizes, exact header/WC/payload words, temporary/full FIFO, exact-
capacity final FIFO, aggregate budget exhausted before header, partial error/data
and clock-loss failure, and short/long interoperation/shared poisoning both ways.
Policy cap and unknown hardware depth remain explicit open verification items.

```sh
cmake -S . -B /tmp/maia-dsi-long-host-20261113 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-dsi-long-host-20261113 -j4
ctest --test-dir /tmp/maia-dsi-long-host-20261113 --output-on-failure
cmake -S . -B /tmp/maia-native-dsi-long-20261113 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON -DMAIA_NATIVE_PROBE_VO3_CYCLE=ON -DMAIA_NATIVE_PROBE_DSI_ACCESS=ON -DMAIA_NATIVE_PROBE_DSI_REFERENCE=ON -DMAIA_NATIVE_PROBE_DSI_PHY=ON -DMAIA_NATIVE_PROBE_DSI_COMMAND=ON -DMAIA_NATIVE_PROBE_DSI_SHORT=ON -DMAIA_NATIVE_PROBE_DSI_LONG=ON
cmake --build /tmp/maia-native-dsi-long-20261113 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-long-20261113/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-long-20261113/maia_sched_probe_ram.bin --log /tmp/maia-dsi-long-hardware.log --dsi-long --capture-seconds 15
```

Opt-in long profile requires short profile and all native prerequisites. After16
short NOPs, attempt4 explicit-long VC0/WC1/NOP payloads, budget10000 each. Capture
attempted bytes/words/space+total polls/mode/error/FIFO/busy and check cached config,
root/supply/PHY after each. Then short NOP followup proves the host can be reused
without staged leftovers. No panel ACK claimed. Payload lengths2..64 and other VCs
remain host-only until separately target-tested. Runner resets preserved flash,
then paced maia.py recovery. No flash/SD/parser/default firmware change.

## Internal-buffer finding and final target evidence — 2026-11-14

[Final hardware capture](native-dsi-long-hardware.txt),
[desktop check](native-dsi-long-desktop.txt),
[manifest](native-dsi-long-manifest.json). Initial RAM run on2026-11-13 passed the
interface-FIFO predicate but revealed pre-header status00010015: interface low7
already looked empty, while internal GEN_BUFF_PLD_EMPTY18 was low. This does NOT
justify equating front FIFO empty with all queued data consumed. That preliminary
image/ELF/source/raw capture is retained under recovery `earlier/first-observation/`.

Pinned host register header additionally defines GEN_BUFF_CMD_EMPTY/FULL16/17 and
GEN_BUFF_PLD_EMPTY/FULL18/19. Strengthened long API initial/completion predicate to
maskF007F==50015, command staging predicate30003==10001 and payload-room mask80008.
Final header still allowed when payload buffer is full after ALL words staged.
Host tests now hold internal buffer nonempty after interface empties/PHY stops;
API must wait or timeout. Seventh mutant ignoring internal buffers is rejected.
No assumption about physical FIFO depth is derived from these status bits.

Final revised image retested on actual Tab5 ESP32-P4 rev1.3/40MHz, no flash/SD
writes. Native predecessor16 short NOPs pass. Four explicit long VC0 frames each
contain exactly one payload byte00 (NOP), header00000139/WC1, one attempted
payload word/byte and one header attempt. LP mode10000->90000 once; stage6 PASS.
Each has4 total loop observations,2 space checks,busy1. Before header00010015;
after transfer00050015, PHY_STATUS15BD, error0/error1 both0. The internal payload
buffer transition is observed explicitly, not inferred from front FIFO bits.
All cached14 config/root/supply/PHY samples retained, summary4/busy4/change1/
preserved1/panel_ack0. Subsequent short NOP stage5/polls2/busy1 succeeds with no
staged leftovers. This remains host progress, not external panel ACK or wire proof.

Hardware-tested scope: explicit long framing with ONE-byte content/one FIFO word
and short interoperation. Lengths2..64, multiple words, nonzero VC and injected
errors/timeouts are HOST-TESTED only. No padded NOP payload or configuration data
sent; no panel reception/compatibility guarantee for forced-long NOP encoding.
Prior VO3 host observation20.802/25.032ms is not voltage/discharge/settling proof.
Nominal PHY/reference clocks remain modeled, not physically measured.

Host36/36 PASS. Clang ASan/UBSan PASS, including exact-end unaligned buffers and
all final-word counts. Seven compiled mutants execute and fail as expected: wrong
WC, rounded-up tail read (ASan heap-buffer-overflow), payload sent to header port,
per-word budget reset, blocking final header on full payload FIFO, ignoring
internal buffers, ignoring shared failed state. Invalid long-without-short profile
rejects; strict C++98 public header PASS. Capture rejects incomplete/duplicate/
malformed rows, wrong WC/length/LP masks/budgets/buffer state and missing short
followup. Native runtime gate PASS: project objects+libgcc only, no unresolved or
foreign runtime. Image and entry confined to L2_LOW/under ROM reservation.
Known linker RWX/removed-empty-flash-segment warnings unchanged.

Paced maia.py verifies original shell echo, SciCalc `2+3*4=14`, module exit0 and
desktop left active. Installed functional desktop still uses preserved FreeRTOS
baseline; native diagnostic is not complete replacement firmware. No parser,
flash/SD/default production changes. Recovery
`../maia-rtos-recovery/native-dsi-long-20261114/` includes final source/build/raw
logs, first observation, mutants/sanitizer commands, licenses and pinned sources.
Build directories keep20261113 (work started then); final checkpoint date is14.
SDK bytes compared with pinned commit before source hashing.

Implemented: bounded1..64-byte LP long publication, safe byte packing/zero tail,
aggregate space/header/completion budget, explicit partial attempts, interface+
internal buffer completion, cross-short/long sticky failure and target framing
capture. Not implemented: verified maximum FIFO capacity/streaming beyond64,
BTA/ACK/read responses, panel reset/DCS startup integration, video/framebuffer/
PSRAM/full native desktop, runtime ownership/teardown or physical signal metrics.
Next small task: bounded read/BTA to obtain a panel response/identity before
claiming external reception or integrating panel-specific startup commands.
