#!/usr/bin/env python3
"""
MaiaRTOS Device CLI (maia)
Command-line tool to manage, inspect, execute and transfer arbitrary
binary and text files to/from MaiaRTOS hardware over serial.

Usage:
  maia push <local_file> [remote_path]
  maia push-module <local.mobj> [remote_dir]
  maia ls [remote_dir]
  maia cat <remote_path>
  maia exec <command>
  maia info
"""

import sys
import os
import time
import glob
import argparse
import re

MAX_SHELL_SEGMENT_CHARS = 127  # MSH_SEG_LEN is 128 bytes including its NUL.

# Auto-reexec with ESP-IDF python if pyserial is not installed in current python
try:
    import serial
except ImportError:
    serial = None
    if __name__ == "__main__":
        candidate_pythons = [
            "/Users/roberto/.espressif/python_env/idf5.4_py3.14_env/bin/python",
            os.path.expanduser("~/.espressif/python_env/idf5.4_py3.14_env/bin/python"),
        ] + glob.glob(os.path.expanduser("~/.espressif/python_env/*/bin/python"))
        for py in candidate_pythons:
            if os.path.isfile(py) and os.access(py, os.X_OK):
                os.execv(py, [py] + sys.argv)
        print("Error: 'pyserial' is not installed in the active Python environment.", file=sys.stderr)
        print("Install it via: pip install pyserial", file=sys.stderr)
        sys.exit(1)


def _validate_remote_path(remote_path):
    if not remote_path.startswith("/") or len(remote_path) > 127:
        raise ValueError("Remote path must be absolute and contain at most 127 ASCII characters")
    if any(char not in "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-/" for char in remote_path):
        raise ValueError("Remote path contains unsupported characters")
    if "//" in remote_path or any(part in (".", "..") for part in remote_path.split("/")):
        raise ValueError("Remote path cannot contain empty, '.' or '..' components")
    if remote_path.endswith("/"):
        raise ValueError("Remote path must name a file, not a directory")
    return remote_path


def _validate_chunk_size(chunk_size):
    if not isinstance(chunk_size, int) or chunk_size < 1 or chunk_size > 256:
        raise ValueError("Chunk size must be between 1 and 256 bytes")


def _max_push_chunk_size(remote_path):
    remote_path = _validate_remote_path(remote_path)
    append_prefix = f"hexwrite -a {remote_path} "
    return (MAX_SHELL_SEGMENT_CHARS - len(append_prefix)) // 2


def find_default_port():
    """Auto-detect USB serial port connected to ESP32-P4 / MaiaRTOS device."""
    patterns = [
        "/dev/cu.usbmodem*",
        "/dev/ttyACM*",
        "/dev/ttyUSB*",
    ]
    for pat in patterns:
        matches = sorted(glob.glob(pat))
        if matches:
            return matches[0]
    return "/dev/cu.usbmodem1301"


class MaiaSerialClient:
    def __init__(self, port, baudrate=115200, timeout=0.1):
        self.port = port
        self.baudrate = baudrate
        self.timeout = timeout
        if serial is None:
            raise RuntimeError("pyserial is not installed")
        try:
            self.ser = serial.Serial()
            self.ser.port = port
            self.ser.baudrate = baudrate
            self.ser.timeout = timeout
            # Keep opening the USB port from asserting the ESP32 reset lines.
            self.ser.dtr = False
            self.ser.rts = False
            self.ser.open()
        except Exception as e:
            print(f"Error opening serial port '{port}': {e}", file=sys.stderr)
            sys.exit(1)
        time.sleep(0.1)
        self.flush_input()

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

    def flush_input(self):
        self.ser.reset_input_buffer()

    def wait_for_prompt(self, timeout=2.0):
        buf = ""
        t0 = time.time()
        while (time.time() - t0) < timeout:
            chunk = self.ser.read(128).decode("utf-8", errors="ignore")
            if chunk:
                buf += chunk
                if "msh> " in buf:
                    return buf
            else:
                time.sleep(0.005)
        return buf

    def execute_command(self, cmd, timeout=3.0):
        cmd = cmd.strip()
        if not cmd:
            raise ValueError("Cannot execute an empty shell command")
        self.flush_input()
        self.ser.write((cmd + "\n").encode("utf-8"))
        buf = ""
        t0 = time.time()
        while (time.time() - t0) < timeout:
            chunk = self.ser.read(256).decode("utf-8", errors="ignore")
            if chunk:
                buf += chunk
                if "msh> " in buf:
                    break
            else:
                time.sleep(0.01)
        if "msh> " not in buf:
            raise TimeoutError(f"Timed out waiting for shell prompt after: {cmd}")
        # Strip echo command and prompt from output
        lines = buf.splitlines()
        clean = []
        for line in lines:
            line_str = line.strip()
            if line_str == cmd:
                continue
            if line_str == "msh>":
                continue
            if line_str.startswith("msh>"):
                line_str = line_str[4:].strip()
            if line_str:
                clean.append(line_str)
        return "\n".join(clean)

    def push_file(self, local_path, remote_path, chunk_size=32,
                  verify_each_chunk=True):
        if not os.path.isfile(local_path):
            raise FileNotFoundError(f"Local file not found: {local_path}")
        remote_path = _validate_remote_path(remote_path)
        _validate_chunk_size(chunk_size)
        max_chunk_size = _max_push_chunk_size(remote_path)
        if chunk_size > max_chunk_size:
            raise ValueError(
                f"Chunk size {chunk_size} exceeds the command-line limit for {remote_path}"
            )

        with open(local_path, "rb") as f:
            data = f.read()
        file_size = len(data)

        print(f"[*] Uploading: {local_path} ({file_size} bytes)")
        print(f"[*] Target   : {remote_path}")

        t_start = time.time()

        if file_size == 0:
            output = self.execute_command(f"hexwrite {remote_path} --empty", timeout=4.0)
            if any(line.startswith("hexwrite:") for line in output.splitlines()):
                raise RuntimeError(f"Device failed to create empty file: {output}")
            if self._read_remote_bytes(remote_path, 0, 1) != b"":
                raise RuntimeError("Empty-file verification returned unexpected data")
            print("[+] Upload complete (empty file created)")
            return

        sent = 0
        while sent < file_size:
            chunk = data[sent:sent + chunk_size]
            append = "-a " if sent else ""
            cmd = f"hexwrite {append}{remote_path} {chunk.hex()}"
            # Never replay an append after a delayed prompt; verify its offset instead.
            try:
                output = self.execute_command(cmd, timeout=10.0)
            except TimeoutError as error:
                actual = self._read_remote_bytes(remote_path, sent, len(chunk))
                if actual != chunk:
                    raise RuntimeError(
                        f"Upload write timed out and readback mismatched at offset {sent}"
                    ) from error
                output = ""
            if any(line.startswith("hexwrite:") for line in output.splitlines()):
                raise RuntimeError(f"Device rejected upload at offset {sent}: {output}")
            if verify_each_chunk:
                actual = self._read_remote_bytes(remote_path, sent, len(chunk))
                if actual != chunk:
                    raise RuntimeError(f"Readback verification failed at offset {sent}")

            sent += len(chunk)

            pct = (sent / file_size) * 100.0
            elapsed = time.time() - t_start
            speed = (sent / 1024.0) / elapsed if elapsed > 0 else 0
            bar_len = 28
            filled = int(bar_len * (sent / file_size))
            bar = "=" * filled + ">" if filled < bar_len else "=" * bar_len
            bar = bar.ljust(bar_len)

            sys.stdout.write(f"\r[{bar}] {pct:5.1f}% ({sent}/{file_size} B) @ {speed:5.1f} KB/s")
            sys.stdout.flush()

        if not verify_each_chunk:
            for offset in range(0, file_size, 256):
                expected = data[offset:offset + 256]
                actual = self._read_remote_bytes(remote_path, offset, len(expected))
                if actual != expected:
                    raise RuntimeError(f"Readback verification failed at offset {offset}")

        elapsed = time.time() - t_start
        speed = file_size / max(elapsed, 0.01) / 1024
        print(f"\n[+] Successfully transferred and verified {file_size} bytes in {elapsed:.2f}s ({speed:.1f} KB/s)")

    def _read_remote_bytes(self, remote_path, offset, count):
        cmd = f"hexread {remote_path} {offset} {count}"
        try:
            output = self.execute_command(cmd, timeout=10.0)
        except TimeoutError:
            # hexread is idempotent; retry once after a delayed/missed prompt.
            output = self.execute_command(cmd, timeout=10.0)
        lines = [line.strip() for line in output.splitlines() if line.strip()]
        for line in lines:
            if line.startswith("hexread:"):
                raise FileNotFoundError(line)
        if not lines:
            return b""
        if len(lines) != 1 or re.fullmatch(r"[0-9a-fA-F]*", lines[0]) is None or len(lines[0]) % 2:
            raise RuntimeError(f"Invalid hexread response for {remote_path} at offset {offset}")
        data = bytes.fromhex(lines[0])
        if len(data) > count:
            raise RuntimeError(f"Device returned too many bytes for {remote_path} at offset {offset}")
        return data

    def pull_file(self, remote_path, local_path, chunk_size=64):
        remote_path = _validate_remote_path(remote_path)
        _validate_chunk_size(chunk_size)
        print(f"[*] Downloading: {remote_path}")
        print(f"[*] Local target: {local_path}")
        t_start = time.time()
        offset = 0
        received_data = bytearray()

        while True:
            chunk = self._read_remote_bytes(remote_path, offset, chunk_size)
            if len(chunk) == 0:
                break

            received_data.extend(chunk)
            offset += len(chunk)

            elapsed = time.time() - t_start
            speed = (offset / 1024.0) / elapsed if elapsed > 0 else 0
            sys.stdout.write(f"\r[*] Downloaded {offset} bytes @ {speed:5.1f} KB/s")
            sys.stdout.flush()

            if len(chunk) < chunk_size:
                break

        with open(local_path, "wb") as f:
            f.write(received_data)

        elapsed = time.time() - t_start
        print(f"\n[+] Successfully pulled {len(received_data)} bytes in {elapsed:.2f}s ({len(received_data) / (elapsed if elapsed > 0 else 0.01) / 1024:.1f} KB/s)")


def cmd_push(args):
    client = MaiaSerialClient(args.port, args.baud)
    try:
        remote = args.remote
        if not remote:
            remote = f"/sd/{os.path.basename(args.local)}"
        elif not remote.startswith("/"):
            remote = os.path.join("/sd", remote).replace("\\", "/")
        if remote.endswith("/") or remote == "/sd":
            remote = os.path.join(remote, os.path.basename(args.local)).replace("\\", "/")

        client.push_file(args.local, remote, chunk_size=args.chunk_size)
        
        # Verify size on device
        time.sleep(0.1)
        out = client.execute_command(f"ls {remote}", timeout=1.5)
        if out:
            print(f"[*] Remote status:\n{out}")
    finally:
        client.close()


def cmd_pull(args):
    client = MaiaSerialClient(args.port, args.baud)
    try:
        remote = args.remote if args.remote.startswith("/") else f"/sd/{args.remote}"
        local = args.local
        if not local:
            local = os.path.basename(remote)
        client.pull_file(remote, local, chunk_size=args.chunk_size)
    finally:
        client.close()


def cmd_push_module(args):
    local_mobj = args.module
    if not os.path.isfile(local_mobj):
        print(f"Error: module '{local_mobj}' not found", file=sys.stderr)
        sys.exit(1)

    base, ext = os.path.splitext(local_mobj)
    manifest_local = f"{base}.mnm.json"

    # Auto-pack/update manifest if mnm_pack tool is available
    pack_tool = os.path.join(os.path.dirname(__file__), "..", "mnm-pack", "mnm_pack.py")
    if os.path.isfile(pack_tool):
        import subprocess
        print(f"[*] Packaging and validating module manifest via mnm_pack.py...")
        subprocess.run([sys.executable, pack_tool, local_mobj], check=True)

    remote_dir = args.remote_dir.rstrip("/") if args.remote_dir else "/sd"
    if not remote_dir.startswith("/"):
        remote_dir = f"/sd/{remote_dir}"
    remote_mobj = f"{remote_dir}/{os.path.basename(local_mobj)}"
    remote_manifest = f"{remote_dir}/{os.path.basename(manifest_local)}"

    client = MaiaSerialClient(args.port, args.baud)
    try:
        # Dismiss any active exclusive GUI/desktop session before updating binaries
        client.ser.write(b"\x1b\x1bqqq\n")
        time.sleep(0.2)
        client.flush_input()
        client.wait_for_prompt(timeout=1.0)

        # Push module binary
        client.push_file(local_mobj, remote_mobj, chunk_size=args.chunk_size)

        # Push manifest if exists
        if os.path.isfile(manifest_local):
            client.push_file(manifest_local, remote_manifest, chunk_size=args.chunk_size)

        # Run on-device modinfo verification
        print("\n[*] Running on-device modinfo integrity verification...")
        time.sleep(0.2)
        res = client.execute_command(f"modinfo {remote_mobj}", timeout=3.0)
        print(res)
    finally:
        client.close()


def cmd_exec(args):
    client = MaiaSerialClient(args.port, args.baud)
    try:
        out = client.execute_command(args.command, timeout=args.timeout)
        print(out)
    finally:
        client.close()


def cmd_ls(args):
    client = MaiaSerialClient(args.port, args.baud)
    try:
        target = args.path if args.path else "/sd"
        out = client.execute_command(f"ls {target}", timeout=2.0)
        print(out)
    finally:
        client.close()


def cmd_cat(args):
    client = MaiaSerialClient(args.port, args.baud)
    try:
        out = client.execute_command(f"cat {args.path}", timeout=3.0)
        print(out)
    finally:
        client.close()


def cmd_info(args):
    client = MaiaSerialClient(args.port, args.baud)
    try:
        print("=== MaiaRTOS Device Telemetry ===")
        uname = client.execute_command("uname", timeout=1.5)
        print(uname)
        cpu = client.execute_command("cpuinfo", timeout=1.5)
        print(cpu)
        mem = client.execute_command("mem", timeout=1.5)
        print(mem)
    finally:
        client.close()


def main():
    default_port = find_default_port()
    parser = argparse.ArgumentParser(
        prog="maia",
        description="MaiaRTOS Device CLI: Upload arbitrary files, modules, inspect and execute commands."
    )
    parser.add_argument("-p", "--port", default=default_port, help=f"Serial port (default: {default_port})")
    parser.add_argument("-b", "--baud", type=int, default=115200, help="Baud rate (default: 115200)")

    sub = parser.add_subparsers(dest="subcommand", required=True)

    # push
    p_push = sub.add_parser("push", help="Upload arbitrary file (binary/audio/image/text) to device")
    p_push.add_argument("local", help="Path to local file")
    p_push.add_argument("remote", nargs="?", default="", help="Path on device (e.g. /sd/myfile.ext; relative paths use /sd)")
    p_push.add_argument("--chunk-size", type=int, default=32, help="Binary chunk size in bytes (default: 32)")
    p_push.set_defaults(func=cmd_push)

    # pull
    p_pull = sub.add_parser("pull", help="Download arbitrary file from device to host")
    p_pull.add_argument("remote", help="Path on device (e.g. /sd/myfile.ext; relative paths use /sd)")
    p_pull.add_argument("local", nargs="?", default="", help="Path on local machine")
    p_pull.add_argument("--chunk-size", type=int, default=64, help="Binary chunk size in bytes (default: 64)")
    p_pull.set_defaults(func=cmd_pull)

    # push-module
    p_mod = sub.add_parser("push-module", help="Package, upload .mobj + .mnm.json and verify with modinfo")
    p_mod.add_argument("module", help="Path to local .mobj file")
    p_mod.add_argument("remote_dir", nargs="?", default="/sd", help="Remote directory (default: /sd; relative paths use /sd)")
    p_mod.add_argument("--chunk-size", type=int, default=32, help="Binary chunk size in bytes (default: 32)")
    p_mod.set_defaults(func=cmd_push_module)

    # ls
    p_ls = sub.add_parser("ls", help="List directory contents on device")
    p_ls.add_argument("path", nargs="?", default="/sd", help="Directory path (default: /sd)")
    p_ls.set_defaults(func=cmd_ls)

    # cat
    p_cat = sub.add_parser("cat", help="Display remote file content")
    p_cat.add_argument("path", help="File path on device")
    p_cat.set_defaults(func=cmd_cat)

    # exec
    p_exec = sub.add_parser("exec", help="Execute arbitrary command on device")
    p_exec.add_argument("command", help="Command line string")
    p_exec.add_argument("-t", "--timeout", type=float, default=3.0, help="Command timeout in seconds")
    p_exec.set_defaults(func=cmd_exec)

    # info
    p_info = sub.add_parser("info", help="Show system telemetry (uname, cpuinfo, mem)")
    p_info.set_defaults(func=cmd_info)

    parsed = parser.parse_args()
    try:
        parsed.func(parsed)
    except (OSError, RuntimeError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        raise SystemExit(1) from error


if __name__ == "__main__":
    main()
