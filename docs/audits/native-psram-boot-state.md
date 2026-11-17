# PSRAM boot-state observation preparation — 2026-11-16

Later checkpoint: [armed connection and actual unconfigured-supply snapshot](native-psram-boot-armed.md).
The preparation/procedure below is retained as history; the first connection
failed, then idle-before-open/held-port transport succeeded. Physical cold
provenance is still unconfirmed; no further manual preparation is pending.

HOST-TESTED / PREPARED. Actual no-reset target observation NOT RUN; operator
power/download-mode preparation requested and pending. No cold initialization,
supply power cycle or new memory-ready acceptance is claimed.

## Reason and change

Native consumer stop/resume is qualified, but an applicable discharge/reset/
initialization specification for the fitted AP32MiB HEX/SIP part has not been
established. Generic Octal timing cannot define a new VO2 software power pulse.
Instead, observe the existing controller/eFuse/supply state after a manually
prepared board start, before intentionally running the preserved desktop.

`run_native_scheduler_probe.py --psram-boot-state` selects the existing isolated
PSRAM state-only profile and requests esptool `before=no_reset`. Other profiles
retain explicit `before=default_reset`, matching prior behavior. Boot observation
refuses identity transactions, array/cache/clock/mode/power/LCD programming and
fatal profiles before connecting. It logs that power history comes from the
operator, not register inference. Existing snapshot evaluation accepts either
accessible matching controller observations or gated roots with zero controller
reads; it does not equate either with usable PSRAM.

Only the host runner, its CLI regression and CMake registration change. Existing
native snapshot code and startup remain unchanged. The image is PSRAM-read-only:
native early startup still configures UART/watchdogs/USB-reset control, and ROM
load writes internal SRAM. This is not a whole-SoC read-only operation. CLI flags
are not binary certification; use the exact state-only image recorded below.

## Prepared image and checks

Fresh isolated build `/tmp/maia-psram-boot-state`, standalone GCC14.2 RV32IMAC/
ILP32, `MAIA_NATIVE_PROBE_PSRAM_STATE=ON`, target `maia_sched_probe_ram`.
esptool4.12 elf2image, image6704bytes, SHA256
**aff430345ea1781429ed2541c7dcd40bd3a735dcbe9270a0d62cbc1f2f3960b7**.
Native gate PASS: project objects+libgcc, no unresolved/foreign runtime symbols.
`--psram-boot-state --validate-only` confirms the original low-SRAM image/entry
bounds without opening a port. No flash/SD/parser/EBNF modifications.

Host73/73 PASS. New regression drives the actual runner `main`/CLI with a fake
image and connection spy that stops before serial: default versus no-reset
selection, validate-only with no connect, out-of-range image, and15 prohibited
profile combinations. This is host validation, not a physical capture.

Actual installed esptool4.12.0 `ESPLoader._connect_attempt` tested with fake serial:
no-reset calls no reset strategy on success or any of five injected failed sync
attempts; ordinary default-reset still calls it. Loader source SHA256
6c5f0c4a9d2047adb1c9164aee66208018789a0f6531922204f6b280168c80e8.
No serial device was opened for those checks. The request suppresses the tool's
reset strategy; it is not a measured USB/control-pin waveform guarantee.

Image/ELFs/map/config/source snapshot are preserved in
`../maia-rtos-recovery/native-psram-boot-state-prepared-20261116/`.
Logs, source pins and transport checks additionally preserved in
`../maia-rtos-recovery/native-psram-boot-state-20261116/`.

## Physical procedure — PENDING

The operator must prepare the device; never assume elapsed time or lack of a
visible desktop proves this preparation occurred. Record the exact operator
report, power sources disconnected, power-off interval and how download mode
was entered. [M5Stack's official instructions](https://docs.m5stack.com/en/core/Tab5#download-mode)
describe Reset/Boot download entry; its power-supply notes require a5second wait
after direct power disconnection for IMU behavior. That board interval is not
used as an undocumented PSRAM timing or a software-controlled VO2 pulse.

1. Shut down the board, remove USB/other power and battery if fitted, wait at
   least5seconds, then reconnect directly in ROM download mode without knowingly
   starting the desktop. If the desktop appears, record it and do not label the
   state as untouched startup. Absence of a visible desktop alone does not prove
   that no earlier application instruction ran; retain that provenance limit.
2. After the operator confirms USB download mode, verify the prepared image hash
   and run once with the IDF Python/esptool environment:

   ```sh
   PYTHONDONTWRITEBYTECODE=1 /Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python \
     tests/hardware/run_native_scheduler_probe.py \
     --image ../maia-rtos-recovery/native-psram-boot-state-prepared-20261116/maia_sched_probe_ram.bin \
     --log /tmp/maia-psram-boot-state-hardware.log \
     --psram-boot-state --capture-seconds 8
   ```

3. On sync failure, do not silently switch to reset mode or an active profile.
   Preserve failure and re-establish physical entry. On success, preserve raw
   output and operator provenance, compare roots/controllers with prior warm
   snapshots, and derive only the next native initialization prerequisites.
4. Runner resets to the unchanged flash application after capture. Verify the
   original shell/SciCalc2+3*4=14/moduleexit0/desktop with the existing recovery
   script. No flash/SD writes are part of this diagnostic.

ROM itself may initialize some chip resources. This observes the state presented
to a native SRAM program after ROM download entry, not an analog rail measurement
or an assertion about the memory's exact factory defaults. Native supply-loss
startup, full capacity/margins/SMP and complete native desktop remain open.
