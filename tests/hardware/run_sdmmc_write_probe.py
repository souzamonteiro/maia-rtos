#!/usr/bin/env python3
"""Verify one native SDMMC sector write, then restore both the sector and flash.

The probe runs from RAM. It performs one controller operation per boot because
the boot-only driver retains controller ownership until reset. The original
sector and complete flash image must be supplied from verified local backups.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

import esptool
import serial

FLASH_BYTES = 0x1000000
BOOTED_MARKER = b"MAIA_SDMMC_WRITE_BOOTED schema=1"
READY_MARKER = b"MAIA_SDMMC_WRITE_READY schema=1"
PASS_MARKER = b"MAIA_NATIVE_SDMMC_WRITE_PASS"
FAIL_MARKER = b"MAIA_NATIVE_SDMMC_WRITE_FAIL"


class ProbeError(RuntimeError):
    pass


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def run_esptool(port, log_path, *arguments):
    command = [
        sys.executable, "-m", "esptool", "--chip", "esp32p4",
        "--port", port, *arguments,
    ]
    try:
        result = subprocess.run(command, capture_output=True, text=True, timeout=600)
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
        if chunk:
            output.extend(chunk)
            if any(marker in output for marker in markers):
                return
    raise ProbeError("serial capture timed out waiting for " +
                     ", ".join(marker.decode("ascii") for marker in markers))


def run_operation(image, port_name, lba, mode, payload, output_path):
    output = bytearray()
    esp = None
    try:
        esp = esptool.get_default_connected_device(
            [port_name], port=port_name, connect_attempts=3,
            initial_baud=115200, chip="esp32p4", before="default_reset")
        esptool.main(
            ["--chip", "esp32p4", "--no-stub", "load_ram", str(image)],
            esp=esp)
        port = esp._port
        port.timeout = 0.1
        read_until(port, output, (BOOTED_MARKER,), 10)
        if port.write(b"B") != 1:
            raise ProbeError("short host-ready command")
        port.flush()
        read_until(port, output, (READY_MARKER,), 10)
        request = mode + payload
        if port.write(request) != len(request):
            raise ProbeError("short SDMMC operation payload")
        port.flush()
        read_until(port, output, (PASS_MARKER, FAIL_MARKER), 30)
    except (esptool.FatalError, serial.SerialException, OSError, SystemExit) as error:
        raise ProbeError(f"probe communication failed: {error}") from error
    finally:
        output_path.write_bytes(output)
        if esp is not None:
            try:
                esp.hard_reset()
            except (esptool.FatalError, serial.SerialException, OSError) as error:
                raise ProbeError(f"could not reset after probe: {error}") from error
            finally:
                esp._port.close()

    text = output.decode("ascii", errors="replace").replace("\r", "")
    op = re.search(
        r"^SDMMC_WRITE_OP mode=(compare|write) result=([0-9a-f]{8}) "
        r"completed=([0-9a-f]{8}) lba=([0-9a-f]{8}) "
        r"(?:reads=([0-9a-f]{8}) writes=([0-9a-f]{8}) "
        r"matches=([0-9a-f]{8}) mismatch=([0-9a-f]{8})|"
        r"writes=([0-9a-f]{8}))$",
        text, re.MULTILINE)
    if PASS_MARKER.decode() not in text or FAIL_MARKER.decode() in text or op is None:
        return False, "probe did not report a valid pass record"
    values = op.groups()
    if values[0] != ("compare" if mode == b"C" else "write"):
        return False, "operation mode did not match request"
    if int(values[1], 16) != 0 or int(values[2], 16) != 13:
        return False, "SDMMC operation did not complete successfully"
    if int(values[3], 16) != lba:
        return False, "probe reported an unexpected LBA"
    if mode == b"C":
        if (int(values[4], 16) != 1 or int(values[5], 16) != 0 or
                int(values[6], 16) != 1 or int(values[7], 16) != 512):
            return False, "readback did not exactly match the expected sector"
    elif int(values[8], 16) != 1:
        return False, "CMD24 was not confirmed ready by CMD13"
    return True, "PASS"


def test_pattern(lba, original):
    pattern = bytes(
        ((index * 73 + 19) ^ ((lba >> ((index % 4) * 8)) & 0xFF)) & 0xFF
        for index in range(512)
    )
    if pattern == original:
        raise ProbeError("generated test pattern unexpectedly equals original sector")
    return pattern


def restore_sector(image, port, lba, original, out, report):
    attempts = []
    for attempt in range(1, 4):
        try:
            written = run_operation(
                image, port, lba, b"W", original,
                out / f"restore-write-{attempt}.log")
            verified = run_operation(
                image, port, lba, b"C", original,
                out / f"restore-verify-{attempt}.log")
            attempts.append({"attempt": attempt, "write_pass": written[0],
                             "verify_pass": verified[0], "detail": verified[1]})
            if verified[0]:
                report["sector_restore"] = {"verified": True, "attempts": attempts}
                return True
        except ProbeError as error:
            attempts.append({"attempt": attempt, "error": str(error)})
    report["sector_restore"] = {"verified": False, "attempts": attempts}
    return False


def restore_flash(port, restore_image, out, report):
    write_output = run_esptool(
        port, out / "flash-restore-esptool.log",
        "--after", "hard_reset", "write_flash", "0x0", str(restore_image))
    if "Hash of data verified." not in write_output:
        raise ProbeError("esptool did not verify the restored full-flash image")

    verified_path = out / "flash-after-restore.bin"
    run_esptool(
        port, out / "flash-verify-esptool.log",
        "--after", "no_reset", "read_flash", "0x0", hex(FLASH_BYTES),
        str(verified_path))
    restored_hash = sha256(restore_image.read_bytes())
    actual_hash = sha256(verified_path.read_bytes())
    report["flash_restore"] = {
        "verified": actual_hash == restored_hash,
        "expected_sha256": restored_hash,
        "actual_sha256": actual_hash,
    }
    if actual_hash != restored_hash:
        raise ProbeError("full-flash readback differs from the verified restore image")


def capture_restored_boot(port_name, out):
    output = bytearray()
    last_error = None
    for attempt in range(3):
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
            deadline = time.monotonic() + 15
            while time.monotonic() < deadline:
                chunk = device.read(4096)
                if chunk:
                    output.extend(chunk)
            break
        except serial.SerialException as error:
            last_error = error
            output.clear()
            if attempt < 2:
                time.sleep(1)
        finally:
            if device.is_open:
                device.close()
    else:
        raise ProbeError(f"could not capture restored application boot: {last_error}")
    (out / "restored-boot.log").write_bytes(output)
    text = output.decode("utf-8", errors="replace")
    return "msh>" in text or "MaiaRTOS" in text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, type=Path,
                        help="esptool elf2image output maia_sched_probe_ram.bin")
    parser.add_argument("--original-sector", required=True, type=Path)
    parser.add_argument("--restore-flash", required=True, type=Path,
                        help="verified 16 MiB flash image to restore at completion")
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--lba", required=True, type=lambda value: int(value, 0))
    parser.add_argument("--port", default="/dev/cu.usbmodem1301")
    args = parser.parse_args()

    if not 0 <= args.lba <= 0xFFFFFFFF:
        parser.error("--lba must fit the SDHC 32-bit block address")
    if not args.image.is_file():
        parser.error("--image does not exist")
    image_bytes = args.image.read_bytes()
    if len(image_bytes) < 24 or image_bytes[0] != 0xE9:
        parser.error("--image must be an esptool elf2image binary, not an ELF file")
    if not args.original_sector.is_file() or args.original_sector.stat().st_size != 512:
        parser.error("--original-sector must be exactly 512 bytes")
    if not args.restore_flash.is_file() or args.restore_flash.stat().st_size != FLASH_BYTES:
        parser.error("--restore-flash must be a complete 16 MiB flash image")
    if args.out.exists() and any(args.out.iterdir()):
        parser.error("--out must be a new or empty directory")
    args.out.mkdir(parents=True, exist_ok=True)
    probe_image = args.out / "maia_sched_probe_ram.bin"
    shutil.copyfile(args.image, probe_image)

    original = args.original_sector.read_bytes()
    if len(original) != 512:
        parser.error("could not read a complete original sector")
    report = {
        "lba": args.lba,
        "probe_image_sha256": sha256(image_bytes),
        "original_sector_sha256": sha256(original),
        "restore_flash_sha256": sha256(args.restore_flash.read_bytes()),
        "operations": [],
        "sector_restore": {"verified": False},
        "flash_restore": {"verified": False},
    }
    touched = False
    failure = None
    try:
        current_a = args.out / "flash-before-test-a.bin"
        current_b = args.out / "flash-before-test-b.bin"
        run_esptool(args.port, args.out / "flash-backup-esptool.log",
                    "--after", "no_reset", "read_flash", "0x0",
                    hex(FLASH_BYTES), str(current_a))
        run_esptool(args.port, args.out / "flash-backup-esptool.log",
                    "--after", "no_reset", "read_flash", "0x0",
                    hex(FLASH_BYTES), str(current_b))
        if current_a.read_bytes() != current_b.read_bytes():
            raise ProbeError("the two pre-test full-flash captures differ")
        report["current_flash_backup_sha256"] = sha256(current_a.read_bytes())

        before = run_operation(
            probe_image, args.port, args.lba, b"C", original,
            args.out / "compare-original-before.log")
        report["operations"].append({"stage": "compare-original-before",
                                     "passed": before[0], "detail": before[1]})
        if not before[0]:
            raise ProbeError("precondition failed: candidate sector differs from saved original")

        pattern = test_pattern(args.lba, original)
        (args.out / "test-pattern.bin").write_bytes(pattern)
        report["test_pattern_sha256"] = sha256(pattern)
        touched = True
        written = run_operation(
            probe_image, args.port, args.lba, b"W", pattern,
            args.out / "write-test-pattern.log")
        report["operations"].append({"stage": "write-test-pattern",
                                     "passed": written[0], "detail": written[1]})
        if not written[0]:
            raise ProbeError("native CMD24 test-pattern write was not confirmed")

        verified = run_operation(
            probe_image, args.port, args.lba, b"C", pattern,
            args.out / "verify-test-pattern.log")
        report["operations"].append({"stage": "verify-test-pattern",
                                     "passed": verified[0], "detail": verified[1]})
        if not verified[0]:
            raise ProbeError("native CMD17 readback did not match the test pattern")
        report["write_readback_pass"] = True
    except ProbeError as error:
        failure = str(error)
    finally:
        if touched:
            try:
                if not restore_sector(probe_image, args.port, args.lba, original,
                                      args.out, report):
                    failure = failure or "could not verify restoration of original sector"
            except ProbeError as error:
                failure = failure or str(error)
        try:
            restore_flash(args.port, args.restore_flash, args.out, report)
        except ProbeError as error:
            failure = failure or str(error)
        if report["flash_restore"].get("verified"):
            try:
                report["restored_boot_ok"] = capture_restored_boot(args.port, args.out)
            except ProbeError as error:
                report["restored_boot_error"] = str(error)
        report["failure"] = failure
        (args.out / "report.json").write_text(json.dumps(report, indent=2) + "\n")

    passed = (
        failure is None and report.get("write_readback_pass", False) and
        report["sector_restore"].get("verified", False) and
        report["flash_restore"].get("verified", False) and
        report.get("restored_boot_ok", False)
    )
    print(json.dumps(report, indent=2))
    return 0 if passed else 1


if __name__ == "__main__":
    sys.exit(main())
