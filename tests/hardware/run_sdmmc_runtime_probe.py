#!/usr/bin/env python3
"""Exercise two-sector runtime SDMMC I/O and restore SD sectors and flash."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys
import time

FLASH_BYTES = 0x1000000
BLOCK_BYTES = 512
BLOCK_COUNT = 2
BOOTED_MARKER = b"MAIA_SDMMC_RUNTIME_BOOTED schema=1"
READY_MARKER = b"MAIA_SDMMC_RUNTIME_READY schema=1"
PASS_MARKER = b"MAIA_NATIVE_SDMMC_RUNTIME_PASS"
FAIL_MARKER = b"MAIA_NATIVE_SDMMC_RUNTIME_FAIL"
RESULT_RE = re.compile(
    r"^SDMMC_RUNTIME_OP mode=(read|write|compare) result=([0-9a-f]{8}) "
    r"completed=([0-9a-f]{8}) lba=([0-9a-f]{8}) "
    r"matches=([0-9a-f]{8}) mismatch=([0-9a-f]{8})$",
    re.MULTILINE,
)
DATA_RE = re.compile(
    r"^SDMMC_RUNTIME_DATA block=([0-9a-f]{8}) "
    r"offset=([0-9a-f]{8}) data=([0-9a-f]{32})$",
    re.MULTILINE,
)


class ProbeError(RuntimeError):
    pass


def sha256(data):
    return hashlib.sha256(data).hexdigest()


def parse_blocks(output, count=BLOCK_COUNT):
    blocks = [bytearray(BLOCK_BYTES) for _ in range(count)]
    seen = set()
    text = output.decode("ascii", errors="replace").replace("\r", "")
    for match in DATA_RE.finditer(text):
        block, offset = (int(value, 16) for value in match.groups()[:2])
        chunk = bytes.fromhex(match.group(3))
        if block >= count or offset % 16 or offset + len(chunk) > BLOCK_BYTES:
            raise ProbeError("runtime read emitted an out-of-range data record")
        key = (block, offset)
        if key in seen:
            raise ProbeError("runtime read emitted a duplicate data record")
        seen.add(key)
        blocks[block][offset:offset + len(chunk)] = chunk
    expected = {(block, offset) for block in range(count)
                for offset in range(0, BLOCK_BYTES, 16)}
    if seen != expected:
        raise ProbeError("runtime read did not emit every byte of both sectors")
    return b"".join(blocks)


def evaluate_operation(output, mode, lba, payload=None):
    text = output.decode("ascii", errors="replace").replace("\r", "")
    match = RESULT_RE.search(text)
    if (PASS_MARKER.decode() not in text or FAIL_MARKER.decode() in text or
            match is None):
        return False, "probe did not report a valid pass record", None
    values = match.groups()
    expected_mode = {"R": "read", "W": "write", "C": "compare"}[mode]
    if values[0] != expected_mode:
        return False, "operation mode did not match request", None
    result, completed, actual_lba, matches, mismatch = (
        int(value, 16) for value in values[1:])
    if result != 0 or completed != BLOCK_COUNT:
        return False, "SDMMC operation did not complete both sectors", None
    if actual_lba != lba:
        return False, "probe reported an unexpected LBA", None
    if matches != 1 or mismatch != BLOCK_COUNT * BLOCK_BYTES:
        return False, "operation did not report exact completion", None
    data = None
    if mode == "R":
        try:
            data = parse_blocks(output)
        except ProbeError as error:
            return False, str(error), None
    elif mode == "C" and payload is not None:
        if len(payload) != BLOCK_COUNT * BLOCK_BYTES:
            return False, "comparison payload has the wrong length", None
    return True, "PASS", data


def test_pattern(first_lba, original):
    pattern = bytes(
        ((index * 73 + 19) ^
         (((first_lba + index // BLOCK_BYTES) >> ((index % 4) * 8)) & 0xFF)) & 0xFF
        for index in range(BLOCK_COUNT * BLOCK_BYTES)
    )
    if len(original) != len(pattern) or pattern == original:
        raise ProbeError("invalid original sectors or repeated test pattern")
    return pattern


def validate_image(image_path):
    import esptool.bin_image

    image = esptool.bin_image.LoadFirmwareImage("esp32p4", str(image_path))
    if not image.segments or not all(
        0x4FF00000 <= segment.addr < segment.addr + len(segment.data) <= 0x4FF28000
        for segment in image.segments
    ) or not any(
        segment.addr <= image.entrypoint < segment.addr + len(segment.data)
        for segment in image.segments
    ):
        raise ProbeError("RAM image is not confined to the qualified L2MEM_LOW window")
    return image


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
    import esptool
    import serial

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
        read_until(port, output, (BOOTED_MARKER,), 15)
        if port.write(b"B") != 1:
            raise ProbeError("short host-ready command")
        port.flush()
        read_until(port, output, (READY_MARKER,), 15)
        request = mode.encode("ascii") + payload
        if port.write(request) != len(request):
            raise ProbeError("short runtime operation payload")
        port.flush()
        read_until(port, output, (PASS_MARKER, FAIL_MARKER), 40)
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

    return evaluate_operation(output, mode, lba, payload)


def restore_sectors(image, port, lba, original, out, report):
    attempts = []
    for attempt in range(1, 4):
        try:
            written = run_operation(
                image, port, lba, "W", original,
                out / f"restore-write-{attempt}.log")
            verified = run_operation(
                image, port, lba, "C", original,
                out / f"restore-verify-{attempt}.log")
            attempts.append({
                "attempt": attempt, "write_pass": written[0],
                "verify_pass": verified[0], "detail": verified[1],
            })
            if written[0] and verified[0]:
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
    import serial

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
    parser.add_argument("--known-first-sector", required=True, type=Path,
                        help="previously verified original 512-byte sector")
    parser.add_argument("--restore-flash", required=True, type=Path,
                        help="verified 16 MiB flash image matching the current device")
    parser.add_argument("--out", required=True, type=Path)
    parser.add_argument("--lba", required=True, type=lambda value: int(value, 0))
    parser.add_argument("--port", default="/dev/cu.usbmodem1301")
    parser.add_argument("--validate-only", action="store_true",
                        help="validate the RAM image and arguments without device access")
    args = parser.parse_args()

    if not 0 <= args.lba <= 0xFFFFFFFD:
        parser.error("--lba must leave room for two SDHC block addresses")
    if not args.image.is_file():
        parser.error("--image does not exist")
    image_bytes = args.image.read_bytes()
    if len(image_bytes) < 24 or image_bytes[0] != 0xE9:
        parser.error("--image must be an esptool elf2image binary, not an ELF file")
    try:
        validate_image(args.image)
    except (ImportError, OSError, ProbeError, ValueError) as error:
        parser.error(str(error))
    if (not args.known_first_sector.is_file() or
            args.known_first_sector.stat().st_size != BLOCK_BYTES):
        parser.error("--known-first-sector must be exactly 512 bytes")
    if (not args.restore_flash.is_file() or
            args.restore_flash.stat().st_size != FLASH_BYTES):
        parser.error("--restore-flash must be a complete 16 MiB flash image")
    if args.validate_only:
        print("PASS: RAM image and inputs validated; no device accessed")
        return 0
    if args.out.exists() and any(args.out.iterdir()):
        parser.error("--out must be a new or empty directory")
    args.out.mkdir(parents=True, exist_ok=True)
    probe_image = args.out / "maia_sched_probe_ram.bin"
    shutil.copyfile(args.image, probe_image)

    known_first = args.known_first_sector.read_bytes()
    report = {
        "first_lba": args.lba,
        "sector_count": BLOCK_COUNT,
        "probe_image_sha256": sha256(image_bytes),
        "known_first_sector_sha256": sha256(known_first),
        "restore_flash_sha256": sha256(args.restore_flash.read_bytes()),
        "operations": [],
        "sector_restore": {"verified": False},
        "flash_restore": {"verified": False},
    }
    original = None
    touched = False
    flash_restore_safe = False
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
        current_hash = sha256(current_a.read_bytes())
        report["current_flash_backup_sha256"] = current_hash
        if current_hash != report["restore_flash_sha256"]:
            raise ProbeError("current flash does not match the supplied restore image")
        flash_restore_safe = True

        snapshot = run_operation(
            probe_image, args.port, args.lba, "R", b"",
            args.out / "snapshot-original-sectors.log")
        report["operations"].append({
            "stage": "snapshot-original-sectors",
            "passed": snapshot[0], "detail": snapshot[1],
        })
        if not snapshot[0] or snapshot[2] is None:
            raise ProbeError("runtime read of original sectors failed")
        original = snapshot[2]
        if original[:BLOCK_BYTES] != known_first:
            raise ProbeError("runtime first sector differs from the verified saved original")
        (args.out / "original-sectors.bin").write_bytes(original)
        report["original_sectors_sha256"] = sha256(original)
        report["second_sector_preserved"] = True

        pattern = test_pattern(args.lba, original)
        (args.out / "test-pattern.bin").write_bytes(pattern)
        report["test_pattern_sha256"] = sha256(pattern)
        touched = True
        written = run_operation(
            probe_image, args.port, args.lba, "W", pattern,
            args.out / "write-test-pattern.log")
        report["operations"].append({
            "stage": "write-two-sector-pattern",
            "passed": written[0], "detail": written[1],
        })
        if not written[0]:
            raise ProbeError("runtime two-sector write did not complete")

        verified = run_operation(
            probe_image, args.port, args.lba, "C", pattern,
            args.out / "verify-test-pattern.log")
        report["operations"].append({
            "stage": "verify-two-sector-pattern",
            "passed": verified[0], "detail": verified[1],
        })
        if not verified[0]:
            raise ProbeError("runtime readback did not match both test sectors")
        report["write_readback_pass"] = True
    except ProbeError as error:
        failure = str(error)
    finally:
        if touched and original is not None:
            try:
                if not restore_sectors(probe_image, args.port, args.lba, original,
                                       args.out, report):
                    failure = failure or "could not verify restoration of original sectors"
            except ProbeError as error:
                failure = failure or str(error)
        if flash_restore_safe:
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
