# PSRAM ROM-entry observation and armed host connection — 2026-11-16

HARDWARE-TESTED LIMITED: read-only PSRAM snapshot from ROM entry with VO2 enable
clear/unconfigured trim, using the existing native SRAM image. Not cold physical
power-loss provenance, native supply/device initialization or usable-memory proof.

## Failed first attempt and transport correction

Operator observed the green LED blinking, repeated the button action and said
"Pronto". The original `before=no_reset` connection received no serial data;
[original failure](native-psram-boot-no-sync.txt) retained. No RAM image was sent,
no runner reset executed. USB remained303A:1001/MAC E8:F6:0A:E2:FD:4C on1301;
no competing serial owner. Operator then reported "Desktop visível". This does
not establish uninterrupted power-off-to-ROM entry; visible pixels alone do not
prove the CPU's present state or that no application code previously executed.

Inspection of installed esptool4.12.0 `loader.py` showed its string-port opener
configures DTR/RTS false before opening only on Windows. Its `reset.py` USB JTAG
sequence defines false/false as idle. New host-only `connect_boot_state` creates
an unopened serial object, sets both false, opens it and passes that object to
the pinned chip loader. No automatic reset strategy is requested. This removes
implicit asserted defaults on macOS; it does not establish that those defaults
caused the first failure or prove physical pin waveforms/reset-free operation.

Optional `--psram-boot-wait 0..120` keeps that port open while waiting for manual
ROM entry. The pinned `_connect_attempt` returns failed-sync status without
closing the port; a callback refuses any reset-strategy invocation. After sync,
normal no-reset chip/security checks run. USB I/O/re-enumeration failures reopen
only the same port with idle signals; elapsed wait/error closes owned resources,
never falls back to reset mode or active firmware. Version other than4.12.0 is
refused before opening. CLI restricts wait to the isolated read-only boot profile.
Other runner modes retain the prior default-reset connector.

In this run ROM sync succeeded promptly after arming; no further button request
was sent. The operator was told not to repeat the action. Exact power history/
whether any extra action occurred during tooling work remains unconfirmed.

## Actual observation

[Unedited capture](native-psram-boot-armed-hardware.txt), P4rev1.3,40MHz crystal,
MAC e8:f6:0a:e2:fd:4c. Existing isolated state-only RAM image6704bytes/SHA256
**aff430345ea1781429ed2541c7dcd40bd3a735dcbe9270a0d62cbc1f2f3960b7**,
payload6624bytes, entry4FF00100, GCC14.2 RV32IMAC/ILP32/esptool4.12.0.
No native source change, PSRAM power/clock/device/array writes or foreign runtime.
Early startup still configures UART/watchdogs/USB-reset control; host RAM load
writes SRAM. "Read-only" applies to PSRAM state, not the entire SoC.

Snapshot result0, roots_valid1,54controller samples, all nine roots and27controller
words identical before/after. New observation versus prior warmed reference:

| Word | Observed ROM-entry state | Warm reference |
| --- | --- | --- |
| SYS clock root | E6DF97AF | E6DFB7FF |
| PERI PSRAM root | 0000C00C | 0000D00C |
| reset root | 00000100 | 38000040 |
| VO2 control | 40200000 | 40200180 |
| VO2 analog | A0000000 | 76800000 |
| both bus dividers | 00030103 | 00030103 |

VO2 XPD8/software-owner7 are clear, trim DREF10/MUL0/VDET0 differs from the
dedicated eFuse pair7/5. The three eFuse/error roots remain7B054313/3A184DEA/0.
PSRAM core/PLL gates are enabled and source is XTAL, while supply enable is
clear: the old native warm clock handoff correctly refuses this entry. Accessible
controller registers do not imply an accessible/powered PSRAM array. Pin/mask
definitions remain those in the existing [evidence ledger](../hardware/EVIDENCE.md).
No array access or mode-register read was attempted; memory identity/defaults,
actual voltage/discharge and boot-ROM effects are not inferred.

This supplies a concrete unconfigured-supply boot entry for the next native
startup implementation, without inventing a software rail power-off pulse.
Next: qualify native consumer gating and calibrated VO2 enable from this observed
state, establish MPLL reset/configuration prerequisites, then mode/data/DMA/LCD.
Do not relax the separate warm-entry guards to accept it.

## Recovery and automated validation

Runner requested its normal final reset, but immediate shell check received no
response. Explicit standard esptool default-reset/chip_id/hard-reset recovery
succeeded without flash writes. Two subsequent short script checks overlapped
fallback startup and missed the echo; their original logs remain. With the same
idle-signal connection held open10seconds before the existing checks, shell echo,
SciCalc2+3*4=14/moduleexit0/desktop PASS. Full fallback boot logs retained. Cause
of every USB/reset transition is not claimed measured. A transient approval
service "Bad Request" prevented the first recovery invocation; the identical
action passed approval on retry. No approval bypass occurred.

Host75/75 PASS: actual CLI profile/wait/image validation before serial, fake-port
idle-before-open/held waiting/re-enumeration/timeouts/version/cleanup, and actual
snapshot integrity/duplicates/failed-capture rejection. Powered snapshots also
remain valid observations: validation never claims cold or ready memory. Previous
native gate permits project objects+libgcc only; exact image unchanged. No flash
or SD writes were commanded by these tools; ordinary preserved-firmware startup
performs its existing SD initialization. No parser/EBNF/OS implementation changes.

All image/source/commands/logs, failed attempts, official demo GIF/source pins and
recovery checks are in `../maia-rtos-recovery/native-psram-boot-armed-20261116/`.
Existing native power-off API remains absent. Full cold/32MiB/margins/SMP/shared
consumers and native desktop acceptance remain open. No further operator button
action is currently pending for this completed observation.

Owner clarification2026-11-16: proceed with manually synchronized physical
power/button acceptance NOT RUN; no further repetition requests. The subsequent
[native boot implementation](native-psram-native-boot.md) has host/build evidence
only. This preserves the actual snapshot above without asserting cold provenance.
