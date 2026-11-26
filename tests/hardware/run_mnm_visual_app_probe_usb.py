#!/usr/bin/env python3
"""Flash native P4 firmware, observe MNM applications over USB, then restore."""
import importlib.util
import io
import json
from pathlib import Path
import sys
import time
import traceback

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OUT = ROOT.parent / "maia-rtos-recovery" / "native-tab5-mnm-visual-app-probe"
DEFAULT_APPS = ("/sd/desktop.mobj", "/sd/apps/scicalc.mobj")


def load_module(name, path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Unable to load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


def main():
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--image", required=True, type=Path)
    parser.add_argument("--out", type=Path, default=DEFAULT_OUT)
    parser.add_argument("--port", default="/dev/cu.usbmodem1301")
    parser.add_argument("--observe-seconds", type=float, default=20.0)
    parser.add_argument("--app", action="append", dest="apps",
                        help="VFS path of a module to display; may be repeated")
    parser.add_argument("--upload-module", type=Path,
                        help="Upload a local module before probing")
    parser.add_argument("--upload-path",
                        help="Temporary VFS path for --upload-module; removed after probing")
    args = parser.parse_args()
    apps = args.apps or list(DEFAULT_APPS)

    if not args.image.is_file():
        parser.error(f"image not found: {args.image}")
    if (args.upload_module is None) != (args.upload_path is None):
        parser.error("--upload-module and --upload-path must be used together")
    if args.upload_module is not None and not args.upload_module.is_file():
        parser.error(f"module not found: {args.upload_module}")
    if args.upload_path is not None and apps.count(args.upload_path) != 1:
        parser.error("--upload-path must appear exactly once as an --app")
    if args.out.exists():
        parser.error(f"output directory already exists: {args.out}")
    if not 1.0 <= args.observe_seconds <= 120.0:
        parser.error("--observe-seconds must be between 1 and 120")

    runner = load_module("f0_flash_probe", ROOT / "tests/hardware/run_f0_flash_probe.py")
    cli = load_module("maia_cli", ROOT / "tools/maia-cli/maia.py")
    for app_path in apps:
        cli._validate_remote_path(app_path)
    upload_chunk_size = 46
    if args.upload_path is not None:
        upload_chunk_size = min(
            upload_chunk_size,
            cli._max_push_chunk_size(args.upload_path),
        )
        if upload_chunk_size < 1:
            parser.error("--upload-path is too long for a hexwrite command")
    original_capture = runner.capture
    app_results = []

    class TracedClient(cli.MaiaSerialClient):
        def __init__(self, ser, transcript):
            self.ser = ser
            self.transcript = transcript

        def execute_command(self, cmd, timeout=3.0):
            self.transcript.write(f"$ {cmd}\n")
            output = super().execute_command(cmd, timeout)
            self.transcript.write(f"{output}\n")
            return output

    def read_until_prompt(ser, seconds, log, phase):
        text = ""
        deadline = time.monotonic() + seconds
        while time.monotonic() < deadline:
            try:
                chunk = ser.read(4096)
            except runner.serial.SerialException:
                ser.close()
                time.sleep(0.3)
                ser.open()
                continue
            if not chunk:
                continue
            decoded = chunk.decode("utf-8", "replace")
            text += decoded
            log.write(decoded)
            log.flush()
            if "MAIA_NATIVE_FATAL" in text or "MAIA_STACK_FAULT" in text:
                raise RuntimeError(f"Native fatal marker during {phase}")
            if "msh> " in text:
                return text
        raise TimeoutError(f"Native shell prompt not reached during {phase}")

    def reset_to_shell(ser, log, phase):
        ser.rts = True
        time.sleep(0.2)
        ser.rts = False
        return read_until_prompt(ser, 45.0, log, phase)

    def capture_apps(port, seconds, log, marker=None, observe_seconds=0.0,
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
        transcript = io.StringIO()
        text = ""
        client = None
        shell_ready = False
        upload_attempted = False

        def cleanup_uploaded_module():
            nonlocal shell_ready, text, upload_attempted
            if not upload_attempted or client is None:
                return
            if not shell_ready:
                text += reset_to_shell(ser, log, "cleanup uploaded module")
                shell_ready = True
            try:
                client._read_remote_bytes(args.upload_path, 0, 1)
            except FileNotFoundError:
                upload_attempted = False
                return
            client.execute_command(f"rm {args.upload_path}")
            try:
                client._read_remote_bytes(args.upload_path, 0, 1)
            except FileNotFoundError:
                upload_attempted = False
                return
            else:
                raise RuntimeError(f"Unable to remove temporary module: {args.upload_path}")

        try:
            ser.open()
            text += reset_to_shell(ser, log, "native boot")
            shell_ready = True
            if "Native module graphics integrated with LCD service" not in text:
                raise RuntimeError("Native LCD graphics integration was not reported")

            client = TracedClient(ser, transcript)
            if args.upload_module is not None:
                cli._validate_remote_path(args.upload_path)
                try:
                    client._read_remote_bytes(args.upload_path, 0, 1)
                except FileNotFoundError:
                    pass
                else:
                    raise RuntimeError(
                        f"Temporary module path already exists: {args.upload_path}")
                upload_attempted = True
                client.push_file(str(args.upload_module), args.upload_path,
                                 chunk_size=upload_chunk_size,
                                 verify_each_chunk=False)
            for app_path in apps:
                # runelf reads ELF metadata and sections on demand from the file.
                app_text = ""
                deadline = time.monotonic() + args.observe_seconds
                started = False
                announced = False
                command_sent = False
                try:
                    command = f"runelf {app_path}\n".encode("ascii")
                    written = ser.write(command)
                    if written != len(command):
                        raise RuntimeError(
                            f"Short command write: {written}/{len(command)} bytes")
                    ser.flush()
                    command_sent = True
                    shell_ready = False

                    while time.monotonic() < deadline:
                        chunk = ser.read(4096)
                        if not chunk:
                            continue
                        decoded = chunk.decode("utf-8", "replace")
                        app_text += decoded
                        transcript.write(decoded)
                        transcript.flush()
                        if any(marker_text in app_text for marker_text in (
                                "MAIA_NATIVE_FATAL", "MAIA_STACK_FAULT",
                                "Guru Meditation", "task_wdt", "assert failed:",
                                "Loader Error:", "Native graphics transaction failed")):
                            raise RuntimeError(f"Native app error while running {app_path}")
                        if "Loader: Executing maia_module_main at " in app_text:
                            started = True
                        if "msh> " in app_text:
                            raise RuntimeError(
                                f"Module returned before observation: {app_path}")
                        if started and not announced:
                            message = (
                                f"[HOST] VISUAL_OBSERVATION_STARTED app={app_path} "
                                f"seconds={args.observe_seconds:g}\n")
                            transcript.write(message)
                            transcript.flush()
                            print(message.strip(), flush=True)
                            announced = True
                    if not started:
                        raise RuntimeError(
                            f"Module did not enter runelf: {app_path}\n{app_text}")
                except Exception:
                    if command_sent:
                        try:
                            text += app_text + reset_to_shell(
                                ser, log, f"recovery after {app_path}")
                            shell_ready = True
                        except Exception as reset_error:
                            transcript.write(
                                f"[HOST] Recovery reset failed: {reset_error}\n")
                    raise

                reboot_log = reset_to_shell(ser, log, f"reset after {app_path}")
                shell_ready = True
                text += app_text + reboot_log
                app_results.append({
                    "path": app_path,
                    "runelf_started": True,
                    "active_until_reset": True,
                    "observe_seconds": args.observe_seconds,
                    "shell_recovered": True,
                })
                transcript.write(f"[HOST] APP_STAGE_PASS path={app_path}\n")

            cleanup_uploaded_module()
            transcript.write("MNM_VISUAL_APP_PROBE_PASS\n")
        except Exception:
            transcript.write(traceback.format_exc())
            raise
        finally:
            active_exception = sys.exc_info()[0] is not None
            cleanup_error = None
            try:
                cleanup_uploaded_module()
            except Exception as error:
                cleanup_error = error
                transcript.write(f"[HOST] Temporary module cleanup failed: {error}\n")
            if client is not None:
                client.close()
            elif ser.is_open:
                ser.close()
            trace = transcript.getvalue()
            log.write("\n[HOST] MNM visual app transcript\n" + trace)
            log.flush()
            (args.out / "interactive.log").write_text(trace)
            if cleanup_error is not None and not active_exception:
                raise RuntimeError(
                    f"Temporary module cleanup failed: {cleanup_error}") from cleanup_error
        return text + "\n" + trace

    runner.capture = capture_apps
    sys.argv = [
        "run_f0_flash_probe.py",
        "--image", str(args.image),
        "--out", str(args.out),
        "--port", args.port,
        "--marker", "msh>",
        "--capture-seconds", "45",
    ]
    try:
        return runner.main()
    finally:
        if args.out.exists():
            (args.out / "application-report.json").write_text(
                json.dumps({"apps": app_results}, indent=2) + "\n")


if __name__ == "__main__":
    raise SystemExit(main())
