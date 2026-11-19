#!/usr/bin/env python3
"""F0 flash-boot probe runner for the Tab5 (ESP32-P4).

Sequence: read back the factory app currently on the device, validate it,
write the native probe at the factory offset, reset and capture the boot log,
then always write the backed-up app back (esptool verifies by MD5) and capture
its boot log. The probe must clear USB_UART_CHIP_RST_DIS so the host can reset.
"""
import argparse
import hashlib
import json
import math
import signal
import struct
import subprocess
import sys
import time
from pathlib import Path

import serial

ESPTOOL = "/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/esptool.py"
FACTORY_OFFSET = 0x10000
PROBE_MARKER = "MAIA_F0_FLASH_PROBE"


def esptool(port, *args, log):
    cmd = [ESPTOOL, "--chip", "esp32p4", "--port", port, *args]
    log.write("$ " + " ".join(cmd) + "\n")
    log.flush()
    result = subprocess.run(cmd, capture_output=True, text=True)
    log.write(result.stdout + result.stderr)
    log.flush()
    if result.returncode != 0:
        raise RuntimeError(f"esptool failed: {' '.join(args)}\n{result.stdout}{result.stderr}")
    return result.stdout


def image_length(data):
    """Length of an ESP app image (header, segments, checksum pad, SHA-256)."""
    if len(data) < 24 or data[0] != 0xE9:
        raise ValueError("backup does not start with an ESP image header")
    count = data[1]
    offset = 24
    for _ in range(count):
        _, length = struct.unpack_from("<II", data, offset)
        offset += 8 + length
    offset += 1
    offset = (offset + 15) & ~15
    if data[23] == 1:
        offset += 32
    if offset > len(data):
        raise ValueError("backup is shorter than the image it describes")
    return offset


def capture(port, seconds, log, marker=None, observe_seconds=0.0):
    ser = serial.Serial()
    ser.port = port
    ser.baudrate = 115200
    ser.timeout = 0.1
    ser.dtr = False
    ser.rts = False
    text = ""
    marker_seen = False
    try:
        ser.open()
        ser.rts = True
        time.sleep(0.2)
        ser.rts = False
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            try:
                chunk = ser.read(4096)
            except serial.SerialException:
                ser.close()
                time.sleep(0.3)
                try:
                    ser.open()
                except serial.SerialException:
                    pass
                continue
            if chunk:
                decoded = chunk.decode("utf-8", "replace")
                text += decoded
                log.write(decoded)
                log.flush()
                if marker and ("MAIA_NATIVE_FATAL" in text or "MAIA_STACK_FAULT" in text):
                    break
                if marker and not marker_seen and marker in text:
                    marker_seen = True
                    if observe_seconds == 0:
                        time.sleep(0.3)
                        break
                    message = f"[HOST] VISUAL_OBSERVATION_STARTED seconds={observe_seconds:g}"
                    log.write("\n" + message + "\n")
                    log.flush()
                    print(message, flush=True)
                    deadline = time.monotonic() + observe_seconds
    finally:
        ser.close()
    return text


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def interrupt_probe(signum, _frame):
    raise InterruptedError(f"probe interrupted by signal {signum}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, type=Path)
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--port", default="/dev/cu.usbmodem1301")
    parser.add_argument("--backup-size", type=lambda v: int(v, 0), default=0x300000)
    parser.add_argument("--capture-seconds", type=float, default=8.0)
    parser.add_argument("--boots", type=int, default=1, help="probe resets to capture")
    parser.add_argument("--marker", default=PROBE_MARKER,
                        help="success marker that ends each boot capture")
    parser.add_argument("--observe-seconds", type=float, default=0.0,
                        help="keep capturing for up to 120 seconds after the marker for visual inspection")
    args = parser.parse_args()

    if args.boots < 1:
        parser.error("--boots must be at least one")
    if not math.isfinite(args.capture_seconds) or args.capture_seconds <= 0:
        parser.error("--capture-seconds must be finite and positive")
    if not 0 <= args.observe_seconds <= 120:
        parser.error("--observe-seconds must be between 0 and 120")
    if not args.marker:
        parser.error("--marker must not be empty")

    args.out.mkdir(parents=True, exist_ok=True)
    log = (args.out / "run.log").open("w")
    report = {
        "image": str(args.image),
        "image_sha256": sha256(args.image),
        "boots": [],
        "probe_pass": False,
        "restore_verified": False,
    }

    backup = args.out / "factory-backup-raw.bin"
    esptool(args.port, "--after", "no_reset", "read_flash",
            hex(FACTORY_OFFSET), hex(args.backup_size), str(backup), log=log)
    raw = backup.read_bytes()
    app_len = image_length(raw)
    app_len_padded = (app_len + 0xFFF) & ~0xFFF
    image_size = args.image.stat().st_size
    image_size_padded = (image_size + 0xFFF) & ~0xFFF
    if app_len_padded > len(raw) or image_size == 0 or image_size_padded > app_len_padded:
        raise SystemExit("backup does not cover every flash sector the probe would overwrite; nothing was written")
    app = args.out / "factory-backup-app.bin"
    app.write_bytes(raw[:app_len_padded])
    info = esptool(args.port, "image_info", str(app), log=log)
    if "(valid)" not in info or "invalid" in info.lower():
        raise SystemExit("backup app image did not validate; nothing was written")
    report.update(backup_app_length=app_len, backup_app_sha256=sha256(app))

    guarded_signals = [signal.SIGINT, signal.SIGTERM]
    if hasattr(signal, "SIGHUP"):
        guarded_signals.append(signal.SIGHUP)
    previous_handlers = {sig: signal.getsignal(sig) for sig in guarded_signals}
    try:
        for sig in guarded_signals:
            signal.signal(sig, interrupt_probe)
        try:
            esptool(args.port, "--after", "no_reset", "write_flash",
                    hex(FACTORY_OFFSET), str(args.image), log=log)
            for boot in range(args.boots):
                log.write(f"\n[HOST] probe boot {boot + 1}\n")
                text = capture(args.port, args.capture_seconds, log, args.marker, args.observe_seconds)
                (args.out / f"probe-boot-{boot + 1}.log").write_text(text)
                fatal = "MAIA_NATIVE_FATAL" in text or "MAIA_STACK_FAULT" in text
                reached = args.marker in text
                report["boots"].append({"marker": reached, "fatal": fatal, "bytes": len(text)})
                if fatal or not reached:
                    break
            report["probe_pass"] = len(report["boots"]) == args.boots and all(
                b["marker"] and not b["fatal"] for b in report["boots"]
            )
        finally:
            for sig in guarded_signals:
                signal.signal(sig, signal.SIG_IGN)
            try:
                log.write("\n[HOST] restoring backed-up factory app\n")
                restored_output = esptool(args.port, "--after", "no_reset", "write_flash",
                                           hex(FACTORY_OFFSET), str(app), log=log)
                if "Hash of data verified." not in restored_output:
                    raise RuntimeError("esptool did not confirm the restored app hash")
                report["restore_verified"] = True
                restored = capture(args.port, 12.0, log)
                (args.out / "restore-boot.log").write_text(restored)
                report["restore_boot_bytes"] = len(restored)
            finally:
                (args.out / "report.json").write_text(json.dumps(report, indent=2) + "\n")
    finally:
        for sig, handler in previous_handlers.items():
            signal.signal(sig, handler)

    print(json.dumps(report, indent=2))
    return 0 if report["probe_pass"] else 1


if __name__ == "__main__":
    sys.exit(main())
