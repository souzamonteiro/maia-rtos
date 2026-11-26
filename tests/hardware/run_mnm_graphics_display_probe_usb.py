#!/usr/bin/env python3
"""Flash the native image, run an MNM graphics display probe over USB, and restore."""
import contextlib
import importlib.util
import io
from pathlib import Path
import sys
import time
import traceback

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUT = ROOT.parent / "maia-rtos-recovery" / "native-tab5-mnm-graphics-display-probe"
DEFAULT_MODULE = Path("/tmp/maia-mnm-graphics-display-probe.mobj")
REMOTE = "/sd/mnm_graphics_display_probe"


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def checked_marker(output, marker):
    if marker not in output.splitlines():
        raise RuntimeError(f"Expected {marker!r}, got {output!r}")


def main():
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, type=Path)
    parser.add_argument("--module", type=Path, default=DEFAULT_MODULE)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--port", default="/dev/cu.usbmodem1301")
    parser.add_argument("--capture-seconds", type=float, default=45.0)
    args = parser.parse_args()

    if not args.image.is_file():
        parser.error(f"image not found: {args.image}")
    if not args.module.is_file():
        parser.error(f"MNM module not found: {args.module}")
    if args.out.exists():
        parser.error(f"output directory already exists: {args.out}")

    runner = load_module("f0_flash_probe", ROOT / "tests/hardware/run_f0_flash_probe.py")
    cli = load_module("maia_cli", ROOT / "tools/maia-cli/maia.py")
    original_capture = runner.capture

    class TracedClient(cli.MaiaSerialClient):
        def __init__(self, ser, transcript):
            self.ser = ser
            self.transcript = transcript

        def execute_command(self, cmd, timeout=3.0):
            self.transcript.write(f"$ {cmd}\n")
            try:
                output = super().execute_command(cmd, timeout)
            except Exception:
                self.transcript.write(traceback.format_exc())
                raise
            self.transcript.write(f"{output}\n")
            return output

    def capture_with_module(port, seconds, log, marker=None, observe_seconds=0.0,
                            input_marker=None, input_data=None, input_line_delay=0.0):
        if marker != "msh>":
            return original_capture(port, seconds, log, marker, observe_seconds,
                                    input_marker, input_data, input_line_delay)

        ser = runner.serial.Serial()
        ser.port = port
        ser.baudrate = 115200
        ser.timeout = 0.1
        ser.dtr = False
        ser.rts = False
        text = ""
        transcript = io.StringIO()
        client = None
        created = False
        trace = ""
        try:
            ser.open()
            ser.rts = True
            time.sleep(0.2)
            ser.rts = False
            deadline = time.monotonic() + seconds
            while time.monotonic() < deadline and marker not in text:
                try:
                    chunk = ser.read(4096)
                except runner.serial.SerialException:
                    ser.close()
                    time.sleep(0.3)
                    try:
                        ser.open()
                    except runner.serial.SerialException:
                        pass
                    continue
                if chunk:
                    decoded = chunk.decode("utf-8", "replace")
                    text += decoded
                    log.write(decoded)
                    log.flush()
                    if "MAIA_NATIVE_FATAL" in text or "MAIA_STACK_FAULT" in text:
                        raise RuntimeError("Native boot reported a fatal error")
            if marker not in text:
                raise RuntimeError("The native shell prompt was not reached")
            if "Native module graphics integrated with LCD service" not in text:
                raise RuntimeError("Native LCD graphics integration was not reported")

            client = TracedClient(ser, transcript)
            result = client.execute_command(f"if mkdir {REMOTE};then;echo MNM_GRAPHICS_DIR_CREATED;fi")
            checked_marker(result, "MNM_GRAPHICS_DIR_CREATED")
            created = True
            client.execute_command(f"cd {REMOTE}")
            checked_marker(client.execute_command("pwd"), REMOTE)

            remote_module = f"{REMOTE}/display_probe.mobj"
            with contextlib.redirect_stdout(transcript):
                client.push_file(str(args.module), remote_module, chunk_size=32)
            result = client.execute_command(f"runelf {remote_module}", timeout=30.0)
            checked_marker(result, "MNM_GRAPHICS_INITIAL_FRAME")
            checked_marker(result, "MNM_GRAPHICS_PARTIAL_UPDATE")
            checked_marker(result, "MNM_GRAPHICS_RELEASED")
            checked_marker(result, "Loader: Module returned 0 (0x0)")
            checked_marker(result, "[SUCCESS] Module executed! Exit code: 0 (0x00)")
            checked_marker(client.execute_command("echo MNM_GRAPHICS_SHELL_ALIVE"),
                           "MNM_GRAPHICS_SHELL_ALIVE")
            transcript.write("MNM_GRAPHICS_DISPLAY_PROBE_PASS\n")
        except BaseException:
            transcript.write(traceback.format_exc())
            raise
        finally:
            try:
                if created:
                    client.execute_command(f"rm {REMOTE}/display_probe.mobj")
                    client.execute_command("cd /sd")
                    result = client.execute_command(
                        f"if rmdir {REMOTE};then;echo MNM_GRAPHICS_DIR_CLEAN;fi")
                    checked_marker(result, "MNM_GRAPHICS_DIR_CLEAN")
            finally:
                if client is not None:
                    client.close()
                elif ser.is_open:
                    ser.close()
                trace = transcript.getvalue()
                log.write("\n[HOST] MNM graphics display probe transcript\n" + trace)
                log.flush()
                (args.out / "interactive.log").write_text(trace)
        return text + "\n" + trace

    runner.capture = capture_with_module
    sys.argv = [
        "run_f0_flash_probe.py",
        "--image", str(args.image),
        "--out", str(args.out),
        "--port", args.port,
        "--marker", "msh>",
        "--capture-seconds", str(args.capture_seconds),
    ]
    return runner.main()


if __name__ == "__main__":
    raise SystemExit(main())
