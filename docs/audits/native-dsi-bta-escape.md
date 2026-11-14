# Native DSI LP escape divider and successful RDDID — 2026-11-14

**HARDWARE-TESTED, LIMITED.** Change the fixed escape divider from6 to7 and derive
its reported nominal frequency from the checked register. Two final RAM loads
return four stable error-free response words each, with temporary controls
restored. No error mask or BTA/read acceptance rule was relaxed. This supersedes
the successful-read NOT DEMONSTRATED status at the prior `ede1698` checkpoint;
its [failure audit](native-dsi-read.md) and captures remain historical evidence.

## Primary source and narrow change

The original native command table port followed ESP-IDF
`f5c3654a1c2d2a01f7f67def7a0dc48e691f63c0` integer truncation of
`lane_Mbps / 8 / 18`:960Mbps gives divider6 and modeled20MHz.
The newer official [bus initialization](https://github.com/espressif/esp-idf/blob/cebb3dd4d8704a5aa1c2b3a23c4c4b9b26e71194/components/esp_lcd/dsi/esp_lcd_mipi_dsi_bus.c)
revision `cebb3dd4d8704a5aa1c2b3a23c4c4b9b26e71194`,
`esp_lcd_new_dsi_bus`, defines18MHz target, documents2..20MHz permitted TxClkEsc
and rounds `lane_Mbps / 8.0 / 18`. At the native fixed960Mbps this gives7.
The source snapshot, API revision metadata and SHA256 are preserved in recovery.
This is a parameter-policy adaptation, not a port of its runtime/clock/APLL code.
Existing Apache-2.0 attribution/license retained. No SDK includes or floating-point
library/roundf call added: the fixed table encodes7 directly.

Native CLKMGR+08 changes0C06->0C07: timeout divider12 stays unchanged, escape low8
becomes7. Nominal byte clock960/8=120MHz; escape120/7=17.142857...MHz. Report stores
floor(120000000/7)=17142857Hz (01059449), derived from the verified after[1] low8.
PHY/reference/lane bitrate, supply, timing tables, packet flags and protocol
headers remain the prior settings. No new turnaround/test-port/analog register
was guessed or changed. Read errors, including ST0bit20, remain fatal/sticky.

The former20MHz setting was at the documented upper limit, not proven out of
spec. Changing only this functional parameter removed the reproduced error in
these warm-entry tests. We have not measured waveform margins, physical bitrate,
contention or the exact electrical mechanism. Detailed P4 DPHY_ERRORS_4 semantics
remain an open documentation question; it has not been declared benign.

## Verification and observed results

Host37/37 CTest PASS with actual native driver fixtures; command/short/long/read
ASan+UBSan PASS, public header C++98 PASS. Two compiled mutants rejected: restore
divider6; leave stale20MHz report. Capture tests reject the previous divisor,
stale nominal frequency and any ST0bit20 response even with success markers.
GCC14.2 RV32IMAC/ILP32 native RAM build/runtime gate PASS: project objects/libgcc
only, no unresolved or foreign runtime symbols. Existing linker RWX/empty removed
segment warnings unchanged. All image segments/entry remain in authorized SRAM.

Two initial candidate runs with divider7 passed four responses each but still
printed the old nominal20MHz report. They are preserved as intermediate evidence,
not final reporting. The final image corrects the report and was loaded twice:

- Each load passes all existing park/gate/VO3/access/reference/PHY/config stages,
  sixteen short NOPs, four one-byte long NOP frames and short followup.
- CLKMGR after0C07, escape_hz01059449, timeout_hz00989680; configuration/root/supply
  checks retained and16 stable configuration samples.
- Each of four RDDID requests per load: result0, stage10, headers3, words1,
  raw word00FBA080, low byte views80/A0/FB, polls7, error0=error1=0.
- Final PHY15BD/FIFO50015, packet original19/RXVC original0, mode90000 restored,
  restore-attempt mask5 (packet and LP mode); summary stable1/preserved1.
- Both runners exit0/PASS and reset to installed flash. Paced maia.py verifies
  shell echo, SciCalc `2+3*4=14`, module exit0 and desktop left active.

These are actual bounded panel-response observations, stronger than prior host
FIFO-drain evidence. The FIFO strips response framing: one word/MRPS3 does not
establish exact received length/datatype or identify the panel model. Do not map
80/A0/FB to a guessed ST712x variant. No native panel reset/DCS initialization,
HS video/pixels/framebuffer/PSRAM/cold boot or physical timing proof in this unit.
Default full desktop still uses the preserved FreeRTOS firmware.

## Files, reproduction and recovery

Changed fixed command table and modeled report in `soc_dsi_access.c`, corresponding
public comment, four host fixtures, capture evaluator and its positive/negative
fixtures. No read state-machine/parser/EBNF/production firmware changes.

Use the full native read configuration from [read audit](native-dsi-read.md), with
build directory `/tmp/maia-native-dsi-bta-escape-20261114`; option names unchanged.

```sh
cmake --build /tmp/maia-native-dsi-bta-escape-20261114 --target maia_sched_probe_ram -j4
ctest --test-dir /tmp/maia-dsi-read-host-20261114 --output-on-failure
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python -m esptool --chip esp32p4 elf2image /tmp/maia-native-dsi-bta-escape-20261114/maia_sched_probe_ram.elf
/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python tests/hardware/run_native_scheduler_probe.py --image /tmp/maia-native-dsi-bta-escape-20261114/maia_sched_probe_ram.bin --log /tmp/maia-dsi-bta-escape-final-hardware.log --dsi-read --capture-seconds 15
python3 /tmp/maia-dsi-bta-escape-checks.py
```

[Final capture](native-dsi-bta-escape-hardware.txt),
[repeat](native-dsi-bta-escape-repeat-hardware.txt),
[desktop recovery](native-dsi-bta-escape-desktop.txt),
[manifest](native-dsi-bta-escape-manifest.json).
Recovery `../maia-rtos-recovery/native-dsi-bta-escape-20261114/` preserves source,
final/intermediate binaries, captures, checks, pinned references and original
firmware hashes. The earlier refusal bundle remains separate. No flash/SD writes.

Next small task: integrate verified native panel reset under proven quiet MIPI,
then the board-specific DCS initialization sequence with bounded command/delay
failure handling. Establish actual panel variant before variant-specific setup;
keep video/framebuffer/PSRAM as subsequent gates. No POSIX implementation yet.
