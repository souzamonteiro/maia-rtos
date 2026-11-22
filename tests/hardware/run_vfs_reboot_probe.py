#!/usr/bin/env python3
"""Verify synced MaiaVFS file data after a clean Tab5 reset; restore the app."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
import time

FLASH_OFFSET = 0x10000
BACKUP_BYTES = 0x300000
ARMED_MARKER = b"MAIA_VFS_REBOOT_ARMED bytes=512"
PASS_MARKER = b"MAIA_VFS_REBOOT_PERSIST_PASS bytes=512"
FAIL_MARKER = b"MAIA_VFS_REBOOT_PERSIST_FAIL"
STRESS_MARKERS = (
    b"MAIA_VFS_STRESS_WORKERS_PASS workers=2 iterations=32",
    b"MAIA_VFS_FAULT_RECOVERY_PASS",
    b"MAIA_VFS_STRESS_PASS workers=2 iterations=32",
)
FATAL_MARKERS = (
    b"MAIA_NATIVE_FATAL",
    b"MAIA_STACK_FAULT",
    b"MAIA_VFS_STRESS_FAIL",
    b"MAIA_VFS_FAULT_RECOVERY_FAIL",
)


class ProbeError(RuntimeError):
    pass


def esptool_executable(explicit=None):
    if explicit is not None:
        if not explicit.is_file():
            raise ProbeError(f"esptool executable does not exist: {explicit}")
        return str(explicit)
    candidate = Path(sys.executable).with_name("esptool.py")
    if candidate.is_file():
        return str(candidate)
    candidate = shutil.which("esptool.py")
    if candidate:
        return candidate
    raise ProbeError("esptool.py is not available in PATH or beside Python")


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def validate_image(path, esptool_path=None):
    if not path.is_file():
        raise ProbeError(f"image does not exist: {path}")
    if not 24 <= path.stat().st_size <= BACKUP_BYTES:
        raise ProbeError("image size is outside the backed-up factory app region")
    try:
        result = subprocess.run(
            [esptool_executable(esptool_path), "--chip", "esp32p4",
             "image_info", str(path)],
            capture_output=True, text=True, timeout=30)
    except (OSError, subprocess.TimeoutExpired, ProbeError) as error:
        raise ProbeError(f"could not validate ESP32-P4 image: {error}") from error
    if result.returncode or "Validation Hash:" not in result.stdout:
        raise ProbeError(
            "esptool rejected the ESP32-P4 image: " +
            f"{result.stdout[-500:]}{result.stderr[-500:]}")


def run_esptool(port, log_path, esptool_path, *arguments):
    command = [
        esptool_executable(esptool_path), "--chip", "esp32p4",
        "--port", port, *arguments,
    ]
    try:
        result = subprocess.run(
            command, capture_output=True, text=True, timeout=600)
    except (OSError, subprocess.TimeoutExpired) as error:
        raise ProbeError(f"esptool invocation failed: {error}") from error
    with log_path.open("a", encoding="utf-8") as log:
        log.write("$ " + " ".join(command) + "\n")
        log.write(result.stdout)
        log.write(result.stderr)
    if result.returncode:
        raise ProbeError(
            f"esptool failed ({result.returncode}): "
            f"{result.stdout[-1000:]}{result.stderr[-1000:]}"
        )
    return result.stdout + result.stderr


def read_until(port, output, markers, timeout):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        chunk = port.read(4096)
        if not chunk:
            continue
        output.extend(chunk)
        if any(marker in output for marker in markers):
            return
    names = ", ".join(marker.decode("ascii") for marker in markers)
    raise ProbeError(f"serial capture timed out waiting for {names}")


def capture_boot(port_name, markers, timeout, output_path):
    import serial

    output = bytearray()
    device = serial.Serial()
    device.port = port_name
    device.baudrate = 115200
    device.timeout = 0.2
    device.dtr = False
    device.rts = False
    try:
        device.open()
        device.rts = True
        time.sleep(0.2)
        device.rts = False
        if markers:
            read_until(device, output, markers, timeout)
        else:
            deadline = time.monotonic() + timeout
            while time.monotonic() < deadline:
                chunk = device.read(4096)
                if chunk:
                    output.extend(chunk)
    except (serial.SerialException, OSError) as error:
        raise ProbeError(f"serial boot capture failed: {error}") from error
    finally:
        if device.is_open:
            device.close()
        output_path.write_bytes(output)
    return bytes(output)


def validate_boot(output, required_markers, forbidden_markers):
    missing = [marker for marker in required_markers if marker not in output]
    if missing:
        raise ProbeError(
            "boot did not report required marker(s): " +
            ", ".join(marker.decode("ascii") for marker in missing))
    forbidden = [marker for marker in forbidden_markers if marker in output]
    if forbidden:
        raise ProbeError(
            "boot contained failure marker(s): " +
            ", ".join(marker.decode("ascii") for marker in forbidden))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, type=Path,
                        help="ESP32-P4 MaiaRTOS diagnostic flash image")
    parser.add_argument("--port", default="/dev/cu.usbmodem1301")
    parser.add_argument("--esptool", type=Path,
                        help="esptool.py path; defaults to PATH or the active Python environment")
    parser.add_argument("--out", required=True, type=Path,
                        help="new or empty directory for logs and flash backup")
    parser.add_argument("--validate-only", action="store_true",
                        help="validate the image without accessing the device")
    args = parser.parse_args()

    try:
        validate_image(args.image, args.esptool)
    except ProbeError as error:
        parser.error(str(error))
    if args.validate_only:
        print("PASS: ESP32-P4 image validated; no device accessed")
        return 0
    if args.out.exists() and any(args.out.iterdir()):
        parser.error("--out must be a new or empty directory")
    args.out.mkdir(parents=True, exist_ok=True)

    report = {
        "image": str(args.image),
        "image_sha256": sha256(args.image.read_bytes()),
        "flash_backup_verified": False,
        "armed_boot_pass": False,
        "post_reset_persistence_pass": False,
        "flash_restore_verified": False,
        "restore_boot_bytes": 0,
    }
    backup_a = args.out / "factory-app-region-a.bin"
    backup_b = args.out / "factory-app-region-b.bin"
    backup = args.out / "factory-app-region.bin"
    image_changed = False
    failure = None

    try:
        run_esptool(
            args.port, args.out / "flash-backup.log", args.esptool,
            "--after", "no_reset", "read_flash", hex(FLASH_OFFSET),
            hex(BACKUP_BYTES), str(backup_a))
        run_esptool(
            args.port, args.out / "flash-backup.log", args.esptool,
            "--after", "no_reset", "read_flash", hex(FLASH_OFFSET),
            hex(BACKUP_BYTES), str(backup_b))
        if backup_a.read_bytes() != backup_b.read_bytes():
            raise ProbeError("two app-region flash captures differ")
        backup.write_bytes(backup_a.read_bytes())
        report["flash_backup_verified"] = True
        report["factory_region_sha256"] = sha256(backup.read_bytes())

        image_changed = True
        output = run_esptool(
            args.port, args.out / "flash-test-image.log", args.esptool,
            "--after", "no_reset", "write_flash",
            hex(FLASH_OFFSET), str(args.image))
        if "Hash of data verified." not in output:
            raise ProbeError("esptool did not verify the diagnostic image")

        first = capture_boot(
            args.port,
            (ARMED_MARKER, PASS_MARKER, FAIL_MARKER, *FATAL_MARKERS), 90,
            args.out / "armed-boot.log")
        validate_boot(first, (*STRESS_MARKERS, ARMED_MARKER),
                      (FAIL_MARKER, *FATAL_MARKERS))
        report["armed_boot_pass"] = True

        second = capture_boot(
            args.port, (PASS_MARKER, FAIL_MARKER, *FATAL_MARKERS), 90,
            args.out / "persistence-boot.log")
        validate_boot(second, (*STRESS_MARKERS, PASS_MARKER),
                      (FAIL_MARKER, *FATAL_MARKERS))
        report["post_reset_persistence_pass"] = True
    except ProbeError as error:
        failure = str(error)
    finally:
        if image_changed and report["flash_backup_verified"]:
            try:
                output = run_esptool(
                    args.port, args.out / "flash-restore.log", args.esptool,
                    "--after", "no_reset", "write_flash",
                    hex(FLASH_OFFSET), str(backup))
                if "Hash of data verified." not in output:
                    raise ProbeError(
                        "esptool did not verify the factory app-region restore")
                verify_path = args.out / "factory-app-region-after-restore.bin"
                run_esptool(
                    args.port, args.out / "flash-restore.log", args.esptool,
                    "--after", "no_reset", "read_flash", hex(FLASH_OFFSET),
                    hex(BACKUP_BYTES), str(verify_path))
                report["flash_restore_verified"] = (
                    sha256(verify_path.read_bytes()) ==
                    report["factory_region_sha256"])
                if not report["flash_restore_verified"]:
                    raise ProbeError(
                        "restored app-region readback differs from its backup")
            except ProbeError as error:
                failure = (f"{failure}; flash restore failed: {error}"
                           if failure else str(error))

        if report["flash_restore_verified"]:
            try:
                restored = capture_boot(
                    args.port, (), 15, args.out / "restore-boot.log")
                report["restore_boot_bytes"] = len(restored)
                if not restored:
                    failure = failure or "restored firmware emitted no boot output"
            except (OSError, ProbeError) as error:
                restore_error = f"restored boot capture failed: {error}"
                failure = (f"{failure}; {restore_error}"
                           if failure else restore_error)

        report["failure"] = failure
        (args.out / "report.json").write_text(
            json.dumps(report, indent=2) + "\n", encoding="utf-8")

    if failure:
        print(f"FAIL: {failure}", file=sys.stderr)
        return 1
    if not (report["armed_boot_pass"] and
            report["post_reset_persistence_pass"] and
            report["flash_restore_verified"] and report["restore_boot_bytes"]):
        print("FAIL: one or more reboot probe stages did not pass", file=sys.stderr)
        return 1
    print("PASS: synced file survived a clean reset; app-region backup restored")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
