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

# Auto-reexec with ESP-IDF python if pyserial is not installed in current python
try:
    import serial
    import serial.tools.list_ports
except ImportError:
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
        try:
            self.ser = serial.Serial(port, baudrate, timeout=timeout)
        except Exception as e:
            print(f"Error opening serial port '{port}': {e}", file=sys.stderr)
            sys.exit(1)
        time.sleep(0.1)
        self.flush_input()

    def close(self):
        if self.ser and self.ser.is_open:
            self.ser.close()

    def flush_input(self):
        try:
            self.ser.read(4096)
        except Exception:
            pass

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
        self.flush_input()
        self.ser.write((cmd.strip() + "\n").encode("utf-8"))
        buf = ""
        t0 = time.time()
        while (time.time() - t0) < timeout:
            chunk = self.ser.read(256).decode("utf-8", errors="ignore")
            if chunk:
                buf += chunk
                if "msh> " in buf and len(buf.strip()) > len(cmd):
                    break
            else:
                time.sleep(0.01)
        # Strip echo command and prompt from output
        lines = buf.splitlines()
        clean = []
        for line in lines:
            line_str = line.strip()
            if line_str == cmd.strip():
                continue
            if line_str.startswith("msh> "):
                line_str = line_str[5:].strip()
            if line_str:
                clean.append(line_str)
        return "\n".join(clean)

    def push_file(self, local_path, remote_path, chunk_size=32):
        if not os.path.isfile(local_path):
            raise FileNotFoundError(f"Local file not found: {local_path}")

        file_size = os.path.getsize(local_path)
        with open(local_path, "rb") as f:
            data = f.read()

        print(f"[*] Uploading: {local_path} ({file_size} bytes)")
        print(f"[*] Target   : {remote_path}")

        total_chunks = (file_size + chunk_size - 1) // chunk_size if file_size > 0 else 1
        t_start = time.time()

        # Handle empty file
        if file_size == 0:
            self.ser.write(f"hexwrite {remote_path} \n".encode("utf-8"))
            self.wait_for_prompt(timeout=1.0)
            print("[+] Upload complete (empty file created)")
            return

        sent = 0
        chunk_idx = 0
        first = True

        while sent < file_size:
            chunk = data[sent:sent + chunk_size]
            hex_str = chunk.hex()
            if first:
                cmd = f"hexwrite {remote_path} {hex_str}\n"
                first = False
            else:
                cmd = f"hexwrite -a {remote_path} {hex_str}\n"

            self.ser.write(cmd.encode("utf-8"))
            self.wait_for_prompt(timeout=2.0)

            sent += len(chunk)
            chunk_idx += 1

            pct = (sent / file_size) * 100.0
            elapsed = time.time() - t_start
            speed = (sent / 1024.0) / elapsed if elapsed > 0 else 0
            bar_len = 28
            filled = int(bar_len * (sent / file_size))
            bar = "=" * filled + ">" if filled < bar_len else "=" * bar_len
            bar = bar.ljust(bar_len)

            sys.stdout.write(f"\r[{bar}] {pct:5.1f}% ({sent}/{file_size} B) @ {speed:5.1f} KB/s")
            sys.stdout.flush()

        elapsed = time.time() - t_start
        print(f"\n[+] Successfully transferred {file_size} bytes in {elapsed:.2f}s ({file_size / elapsed / 1024:.1f} KB/s)")

    def pull_file(self, remote_path, local_path, chunk_size=64):
        print(f"[*] Downloading: {remote_path}")
        print(f"[*] Local target: {local_path}")
        t_start = time.time()
        offset = 0
        received_data = bytearray()

        while True:
            cmd = f"hexread {remote_path} {offset} {chunk_size}"
            out = self.execute_command(cmd, timeout=2.0)
            hex_line = ""
            for l in out.splitlines():
                l_str = l.strip()
                if l_str and all(c in "0123456789abcdefABCDEF" for c in l_str):
                    hex_line = l_str
                    break

            if not hex_line:
                break

            try:
                chunk = bytes.fromhex(hex_line)
            except ValueError:
                break

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
        elif remote.endswith("/") or remote == "/sd":
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
        local = args.local
        if not local:
            local = os.path.basename(args.remote)
        client.pull_file(args.remote, local, chunk_size=args.chunk_size)
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
    remote_mobj = f"{remote_dir}/{os.path.basename(local_mobj)}"
    remote_manifest = f"{remote_dir}/{os.path.basename(manifest_local)}"

    client = MaiaSerialClient(args.port, args.baud)
    try:
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
    p_push.add_argument("remote", nargs="?", default="", help="Path on device (e.g. /sd/myfile.ext)")
    p_push.add_argument("--chunk-size", type=int, default=32, help="Binary chunk size in bytes (default: 32)")
    p_push.set_defaults(func=cmd_push)

    # pull
    p_pull = sub.add_parser("pull", help="Download arbitrary file from device to host")
    p_pull.add_argument("remote", help="Path on device (e.g. /sd/myfile.ext)")
    p_pull.add_argument("local", nargs="?", default="", help="Path on local machine")
    p_pull.add_argument("--chunk-size", type=int, default=64, help="Binary chunk size in bytes (default: 64)")
    p_pull.set_defaults(func=cmd_pull)

    # push-module
    p_mod = sub.add_parser("push-module", help="Package, upload .mobj + .mnm.json and verify with modinfo")
    p_mod.add_argument("module", help="Path to local .mobj file")
    p_mod.add_argument("remote_dir", nargs="?", default="/sd", help="Remote directory (default: /sd)")
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
    parsed.func(parsed)


if __name__ == "__main__":
    main()

