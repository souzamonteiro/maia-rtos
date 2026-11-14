# Bounded native VC0 RDDID/BTA read — 2026-11-14

**HOST-TESTED API; HARDWARE-TESTED refusal, successful read NOT DEMONSTRATED.** Fixed read-only DCS04 request with maximum return
size3, guarded one-word RX pop, shared timeout/fault policy and restoration of
host packet/RXVC/LP settings after verified idle. No model/response-length guess,
panel reset/startup/video/PSRAM or full native desktop claim.

## Primary sources

Pinned ESP-IDF f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0
[HAL read path](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/mipi_dsi_hal.c)
`host_gen_read_short_packet`/`host_gen_read_dcs_command`: send max return size,
enable BTA/set matching RX VC, publish read header, wait/read payload. Native
implementation replaces all unbounded waits and blind draining, uses one fixed
command/response word and restores temporary host settings only on success.
[Host LL](https://github.com/espressif/esp-idf/blob/f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0/components/hal/esp32p4/include/hal/mipi_dsi_host_ll.h)
and host register header: CMD_MODE_CFG+68 MRPS LPbit24/DCS-read LPbit18,
PCKHDL+2C BTA bit2, GEN_VCID+30 RXlow2, GEN_HDR+6C, GEN_PLD+70 RX pop;
CMD_PKT_STATUS+74 RX-empty4/readbusy6, PHY_STATUS+B0 direction1 (RX1/TX0).
Constants/header definitions in `dsi_read_packet.h` retain Espressif Apache-2.0
attribution and license; native lifecycle/error/budget policy original MIT.
No SDK headers/runtime/tasks/locks or IRQ-status/mask writes imported.

[ST7123 V0.5](https://m5stack-doc.oss-cn-shenzhen.aliyuncs.com/1132/ST7123_SPEC_Preliminary_V0.5.pdf)
2021/09 section6.2.3 p142 defines RDDID04 with three identification bytes, available
in sleep-in/out. FF defaults mean unprogrammed fields; FF/zero are not rejected or
used as a guessed silicon model. Datatypes37(max return size)/06(DCS read) follow
p99 and pinned SDK types. Local source hashes/selected text retained in recovery.
Prior TRM chip1.3 v0.4 ch10/p616/regs10.11–10.16 pp651–659 clock prerequisites
unchanged; [TRM](https://documentation.espressif.com/esp32-p4-chip-revision-v1.3_technical_reference_manual_en.pdf).

## Narrow API contract

`maia_esp32p4_dsi_read_id(budget,report)` requires existing internal native command
claim, same chip1.3/core0/IRQs-off/exclusive boot/root/supply/VDMA/DSI/status access.
Initial interface+internal buffers empty, PHY locked/stopped/TX direction, TE/ACK/
BTA off, cached14 settings unchanged. No stale RX discard or queue adoption.
Validate budget1..100000/null/context before MMIO. Initial clean busy returns BUSY,
fresh drift VERIFY/write-free. Preexisting error sticky IO. Shared transfer fault
blocks short/long/read until reset; no retry/rollback/flush after partial operation.

Temporarily set MRPS/read LP bits (01040000), preserve other mode fields; header
00000337 requests MRPS3. Wait bounded host drain. Then set BTA, RX VC0 preserving
other fields, publish header00000406. MRPS changes the panel's protocol return
limit, not image configuration; it remains3 after success. There are no payload
writes or visual DCS changes. ACK request stays off; a DCS read intrinsically
requests a response/BTA, rather than an ACK to a previous write.

A single aggregate budget covers MRPS drain, response availability and RX idle
loops, plus fixed preflight/restoration/final checks. Check RCC before peripheral
MMIO; complete table/root/supply/PHY controls with temporary PCKHDL override before
polls. PHY lock required; direction/stops may change during RX. Read data ONLY
when readbusy0 and RX-empty0. Pop at most one word, record low3 byte view. Hardware
strips response framing; FIFO exposes word availability, not precise received
length or datatype. MRPS3 is a maximum, so one word/low3 view does not prove all
three bytes were supplied or identify a panel family. Excess available data is IO,
never silently drain. Error flags OR-accumulated and may be consumed by reading.

After pop, wait for all buffers idle, readbusy0, PHY stop and direction TX0
(mask97=95). Only then restore complete original PCKHDL (BTA off), RXVC and mode,
with fresh idle/errors/guards/readback before/after steps. Successful repeat reads
allowed. Failure may leave BTA/RX/LP settings altered and a packet outstanding;
no compensating restore while RX possibly active. Runner reset restores baseline.

Report stages0:none/1:guarded/2:LP/3:MRPS attempted/4:drained/5:BTA configured/
6:read attempted/7:RX word popped/8:RX idle TX direction/9:host restored/10:verified.
Header flags0/1 set BEFORE MRPS/read stores; one received word recorded only after
actual pop. Raw word and byte views are observations, not a verified model. Exact
original host words and restore-attempt mask reported. Mode after means last
observed/restored mode; successful result requires full restoration. No real-time
latency/voltage/signal-integrity promise, generic read API, large RX or runtime
concurrent ownership/teardown.

## Verification procedure

Host AppleClang21/Python3.14.5, native GCC14.2 RV32IMAC/ILP32. Fixture first executes
all real native init APIs. Simulates MRPS delay, readbusy clearing before data,
response delay/one RX pop, direction RX after FIFO empty, extra word, no response,
error/read-clear flags, guard/config write failure and failed restoration. Each
failure is sticky across write/read, no repeat headers or blind extra data pops.
No unreadable queue/config access; RX pop only valid when readable/nonempty.

```sh
cmake -S . -B /tmp/maia-dsi-read-host-20261114 -DMAIA_BUILD_HOST_TESTS=ON
cmake --build /tmp/maia-dsi-read-host-20261114 -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
cmake -S . -B /tmp/maia-native-dsi-read-20261114 -DCMAKE_TOOLCHAIN_FILE=cmake/toolchains/riscv32-esp32p4-elf.cmake -DMAIA_NATIVE_PROBE_MIPI=ON -DMAIA_NATIVE_PROBE_MIPI_PARK=ON -DMAIA_NATIVE_PROBE_MIPI_GATE=ON -DMAIA_NATIVE_PROBE_VO3_CYCLE=ON -DMAIA_NATIVE_PROBE_DSI_ACCESS=ON -DMAIA_NATIVE_PROBE_DSI_REFERENCE=ON -DMAIA_NATIVE_PROBE_DSI_PHY=ON -DMAIA_NATIVE_PROBE_DSI_COMMAND=ON -DMAIA_NATIVE_PROBE_DSI_SHORT=ON -DMAIA_NATIVE_PROBE_DSI_LONG=ON -DMAIA_NATIVE_PROBE_DSI_READ=ON
cmake --build /tmp/maia-native-dsi-read-20261114 --target maia_sched_probe_ram -j4
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-read-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-read-20261114/maia_sched_probe_ram.bin --log /tmp/maia-dsi-read-hardware.log --dsi-read --capture-seconds 15
```

Opt-in read diagnostic includes preceding short/long tests, requests4 VC0 identity
responses/budget10000 each, checks stable low24-bit views and preserved native
configuration/root/supply. No hardcoded identity or exact-response-length claim.
Failure prints report and halts, no cleanup writes; runner resets preserved flash
in finally. Paced maia.py recovery verifies shell/SciCalc14/desktop. No flash/SD/
parser/default production changes. Working desktop still uses FreeRTOS baseline.

## Actual results and open gate

37/37 host tests PASS, ASan/UBSan PASS, C++98 public header PASS, seven compiled
mutants rejected, invalid native profile rejected. The actual driver is linked
into the fixtures. Added a regression for RX-ready with ST0bit20 set: reject
before the payload pop, preserve the error and poison subsequent transfers.
Capture evaluator rejects that refusal and a fabricated success with bit20 set.
Native GCC14.2 runtime gate PASS (project objects/libgcc only), no foreign runtime.

Two independent RAM loads of the same image on Tab5 reproduced:

```text
result=fffffff9 stage=00000006 headers=00000003 words=00000000
polls=00000006 before=00090000 after=010d0000 restored=00000000
status=000015bd fifo=00050005 error0=00100000 error1=00000000
```

Result is IO after publication of both MRPS/read headers. RX-empty is clear and
readbusy clear, but ST0 DPHY_ERRORS_4 is set. Therefore no payload is popped or ID
accepted; word/id fields zero mean NOT READ, not panel identity zero. Mode/BTA may
remain temporary; fixture halts, runner resets the board to preserved flash.
Both runs successfully exercised preceding native PHY/config/16short/4long and
short followup. No successful read/restoration/repetition is claimed on target.
The runner correctly returned exit1/FAIL in both cases. These are failure captures,
not successful hardware acceptance logs.

The pinned P4 register header names ST0bit20 DPHY_ERRORS_4 but supplies no detailed
meaning. A primary Microchip controller reference
[DSI ISR0 section6.5.42, online revision en-US-3](https://onlinedocs.microchip.com/oxy/GUID-51CE727C-126C-4670-A2BA-189781F25924-en-US-3/GUID-158FA27F-33B6-4747-8EE7-2B16772DC3BF.html)
labels the identically named bit as Lane0 LP1 contention. This is a cross-controller
clue, NOT confirmation of P4 semantics/cause. No bit has been masked or ignored.
[Espressif errata summary](https://docs.espressif.com/projects/esp-chip-errata/en/latest/esp32p4/02-errata-summary/index.html)
consulted 2026-11-14 lists no DSI erratum; absence does not establish harmlessness.
SDK read helper does not inspect these flags; that alone cannot justify ignoring
an error in the native driver. Physical contention, historical-latch effects,
turnaround timing and panel retained state have not been distinguished.

Next small task: establish P4-specific DPHY_ERRORS_4 semantics and compare native
BTA/turnaround controls with primary vendor/working-panel evidence. Correct only
supported settings, preserve this failing reference, and require four bounded
error-free responses with restored host state before closing the read gate. No
panel-specific initialization/video milestone should rely on this rejected reply.

Raw initial/repeat hardware and desktop captures are adjacent `native-dsi-read-*.txt`
files. Both resets restored shell/SciCalc `2+3*4=14`/desktop. No flash or SD writes,
parser changes or default production changes. Recovery:
`../maia-rtos-recovery/native-dsi-read-20261114/`; manifest records the tested image,
source, pinned SDK and baseline hashes, including the first refusal snapshot.
