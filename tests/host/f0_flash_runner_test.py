import contextlib
import importlib.util
import io
import json
import os
from pathlib import Path
import signal
import struct
import sys
import tempfile
import types
from unittest.mock import patch


sys.dont_write_bytecode = True
root = Path(__file__).resolve().parents[2]
serial = types.ModuleType("serial")
serial.SerialException = type("SerialException", (Exception,), {})
with patch.dict(sys.modules, {"serial": serial}):
    spec = importlib.util.spec_from_file_location(
        "f0_flash_runner", root / "tests/hardware/run_f0_flash_probe.py"
    )
    runner = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(runner)


class Clock:
    def __init__(self):
        self.now = 0.0

    def monotonic(self):
        return self.now

    def sleep(self, seconds):
        self.now += seconds


class Port:
    def __init__(self, clock, chunks):
        self.clock = clock
        self.chunks = list(chunks)
        self.closed = False
        self.writes = []
        self.flush_count = 0

    def open(self):
        self.closed = False

    def close(self):
        self.closed = True

    def read(self, size):
        self.clock.now += 0.1
        chunk = self.chunks.pop(0) if self.chunks else b""
        if isinstance(chunk, Exception):
            raise chunk
        return chunk

    def write(self, data):
        self.writes.append(data)
        return len(data)

    def flush(self):
        self.flush_count += 1


def run_capture(chunks, seconds=2, marker="msh> ", observe_seconds=0):
    clock = Clock()
    port = Port(clock, chunks)
    log = io.StringIO()
    with patch.object(runner.serial, "Serial", return_value=port, create=True), \
         patch.object(runner.time, "monotonic", clock.monotonic), \
         patch.object(runner.time, "sleep", clock.sleep), \
         contextlib.redirect_stdout(io.StringIO()):
        text = runner.capture("fake", seconds, log, marker, observe_seconds)
    assert port.closed
    return text, clock.now, log.getvalue()


text, elapsed, log = run_capture([b"boot\n", b"msh> ", b"later\n"])
assert text == "boot\nmsh> " and elapsed < 1 and "VISUAL_OBSERVATION_STARTED" not in log

text, elapsed, log = run_capture([b"boot\nmsh", b"> ", b"later\n"], observe_seconds=3)
assert text == "boot\nmsh> later\n" and 3.4 <= elapsed < 3.7
assert log.count("VISUAL_OBSERVATION_STARTED") == 1

clock = Clock()
port = Port(clock, [b"boot\nMAIA_F1_RUNTIME_INPUT_READY\n",
                    b"MAIA_F1_RUNTIME_PASS\n"])
log = io.StringIO()
with patch.object(runner.serial, "Serial", return_value=port, create=True), \
     patch.object(runner.time, "monotonic", clock.monotonic), \
     patch.object(runner.time, "sleep", clock.sleep):
    text = runner.capture(
        "fake", 2, log, "MAIA_F1_RUNTIME_PASS", 0,
        "MAIA_F1_RUNTIME_INPUT_READY", b"Maia"
    )
assert "MAIA_F1_RUNTIME_PASS" in text
assert port.writes == [b"Maia"] and port.flush_count == 1
assert "SENT_INPUT bytes=4 hex=4d616961" in log.getvalue()

clock = Clock()
port = Port(clock, [b"MAIA_F1_RUNTIME_PASS\n"])
with patch.object(runner.serial, "Serial", return_value=port, create=True), \
     patch.object(runner.time, "monotonic", clock.monotonic), \
     patch.object(runner.time, "sleep", clock.sleep):
    try:
        runner.capture(
            "fake", 2, io.StringIO(), "MAIA_F1_RUNTIME_PASS", 0,
            "MAIA_F1_RUNTIME_INPUT_READY", b"Maia"
        )
    except RuntimeError as error:
        assert "input marker" in str(error)
    else:
        raise AssertionError("missing input marker must not silently skip input")
assert port.writes == []

text, elapsed, log = run_capture([b"boot\n"], seconds=1, observe_seconds=3)
assert text == "boot\n" and 1.2 <= elapsed < 1.4
assert "VISUAL_OBSERVATION_STARTED" not in log

text, elapsed, log = run_capture([b"msh> ", b"MAIA_NATIVE_FATAL\n"], observe_seconds=3)
assert "MAIA_NATIVE_FATAL" in text and elapsed < 1
assert log.count("VISUAL_OBSERVATION_STARTED") == 1

clock = Clock()
port = Port(clock, [RuntimeError("read failed")])
with patch.object(runner.serial, "Serial", return_value=port, create=True), \
     patch.object(runner.time, "monotonic", clock.monotonic), \
     patch.object(runner.time, "sleep", clock.sleep):
    try:
        runner.capture("fake", 2, io.StringIO(), "msh> ")
    except RuntimeError as error:
        assert str(error) == "read failed"
    else:
        raise AssertionError("capture must propagate unexpected read failures")
assert port.closed


def backup_bytes():
    header = bytearray(24)
    header[0] = 0xE9
    header[1] = 1
    image = bytes(header) + struct.pack("<II", 0x40000020, 4) + b"test"
    return image + bytes(8192 - len(image))


with tempfile.TemporaryDirectory() as directory:
    directory = Path(directory)
    image = directory / "probe.bin"
    image.write_bytes(b"probe")
    writes = []
    capture_calls = []
    fatal = False
    fail_capture = False
    interrupt_write = False
    interrupt_restore = False
    restore_hash = True

    def esptool(port, *args, log):
        if "read_flash" in args:
            Path(args[-1]).write_bytes(backup_bytes())
        elif "image_info" in args:
            return "Checksum: (valid)"
        elif "write_flash" in args:
            writes.append(Path(args[-1]).name)
            if writes[-1] == "probe.bin" and interrupt_write:
                os.kill(os.getpid(), signal.SIGTERM)
            if writes[-1] == "factory-backup-app.bin":
                assert signal.getsignal(signal.SIGTERM) == signal.SIG_IGN
                if interrupt_restore:
                    os.kill(os.getpid(), signal.SIGTERM)
                return "Hash of data verified." if restore_hash else ""
        return ""

    def capture(port, seconds, log, marker=None, observe_seconds=0,
                input_marker=None, input_data=None, input_line_delay=0.0):
        capture_calls.append((marker, observe_seconds, input_marker, input_data, input_line_delay))
        if marker is not None:
            if fail_capture:
                raise RuntimeError("capture failed")
            return "msh> " + ("MAIA_NATIVE_FATAL\n" if fatal else "")
        return "restored\n"

    def run(arguments, name):
        argv = ["runner", "--image", str(image), "--out", str(directory / name), *arguments]
        with patch.object(sys, "argv", argv), patch.object(runner, "esptool", esptool), \
             patch.object(runner, "capture", capture), contextlib.redirect_stderr(io.StringIO()), \
             contextlib.redirect_stdout(io.StringIO()):
            try:
                return runner.main()
            except SystemExit as error:
                return error.code

    assert run(["--marker", "msh> ", "--observe-seconds", "3"], "success") == 0
    report = json.loads((directory / "success/report.json").read_text())
    assert report["probe_pass"] and report["restore_verified"]
    assert report["boots"][0]["marker"] and not report["boots"][0]["fatal"]
    assert capture_calls == [("msh> ", 3, None, None, 0), (None, 0, None, None, 0)]
    assert writes == ["probe.bin", "factory-backup-app.bin"]

    capture_calls.clear()
    assert run([
        "--marker", "msh> ", "--input-marker", "READY", "--input-hex", "4d616961",
        "--input-line-delay", "0.5"
    ], "send-input") == 0
    assert capture_calls == [("msh> ", 0, "READY", b"Maia", 0.5), (None, 0, None, None, 0)]

    fatal = True
    writes.clear()
    assert run(["--marker", "msh> "], "fatal") == 1
    report = json.loads((directory / "fatal/report.json").read_text())
    assert not report["probe_pass"] and report["boots"][0]["fatal"]
    assert writes == ["probe.bin", "factory-backup-app.bin"]

    fail_capture = True
    writes.clear()
    try:
        run(["--marker", "msh> "], "exception")
    except RuntimeError as error:
        assert str(error) == "capture failed"
    else:
        raise AssertionError("capture failure must propagate after restoration")
    assert writes == ["probe.bin", "factory-backup-app.bin"]
    report = json.loads((directory / "exception/report.json").read_text())
    assert not report["probe_pass"] and report["restore_verified"]

    fail_capture = False
    fatal = False
    interrupt_write = True
    old_handler = signal.getsignal(signal.SIGTERM)
    writes.clear()
    try:
        run(["--marker", "msh> "], "interrupt")
    except InterruptedError:
        pass
    else:
        raise AssertionError("interrupt must propagate after restoration")
    assert writes == ["probe.bin", "factory-backup-app.bin"]
    assert signal.getsignal(signal.SIGTERM) == old_handler
    report = json.loads((directory / "interrupt/report.json").read_text())
    assert not report["probe_pass"] and report["restore_verified"]

    interrupt_write = False
    interrupt_restore = True
    writes.clear()
    assert run(["--marker", "msh> "], "restore-signal") == 0
    assert writes == ["probe.bin", "factory-backup-app.bin"]
    assert signal.getsignal(signal.SIGTERM) == old_handler

    interrupt_restore = False
    restore_hash = False
    writes.clear()
    try:
        run(["--marker", "msh> "], "restore-unverified")
    except RuntimeError as error:
        assert "did not confirm" in str(error)
    else:
        raise AssertionError("unverified restoration must fail explicitly")
    report = json.loads((directory / "restore-unverified/report.json").read_text())
    assert not report["restore_verified"]
    restore_hash = True

    writes.clear()
    image.write_bytes(bytes(4097))
    result = run(["--marker", "msh> "], "uncovered")
    assert "nothing was written" in result and not writes

    for index, arguments in enumerate((
        ["--marker", ""],
        ["--capture-seconds", "nan"],
        ["--capture-seconds", "inf"],
        ["--observe-seconds", "nan"],
        ["--observe-seconds", "-1"],
        ["--observe-seconds", "661"],
        ["--input-marker", "READY"],
        ["--input-hex", "4d"],
        ["--input-marker", "READY", "--input-hex", "not-hex"],
        ["--input-marker", "", "--input-hex", "4d"],
        ["--input-marker", "READY", "--input-hex", ""],
    )):
        assert run(arguments, f"invalid-{index}") == 2
        assert not writes

print("PASS: bounded visual capture, fatal detection, backup coverage, and restoration")
