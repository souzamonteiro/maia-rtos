#!/usr/bin/env python3
import contextlib
import importlib.util
import io
from pathlib import Path
import shlex
import sys
import tempfile
import unittest
from types import SimpleNamespace
from unittest.mock import patch

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "maia_cli", ROOT / "tools/maia-cli/maia.py"
)
MAIA_CLI = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MAIA_CLI)


class FakeSerial:
    def __init__(self):
        self.files = {}
        self.input = bytearray()
        self.is_open = True
        self.suppress_prompt = False
        self.corrupt_next_read = False
        self.malformed_next_read = False
        self.commands = []

    def close(self):
        self.is_open = False

    def reset_input_buffer(self):
        self.input.clear()

    def read(self, size):
        data = bytes(self.input[:size])
        del self.input[:size]
        return data

    def write(self, data):
        command = data.decode("ascii").strip()
        self.commands.append(command)
        args = shlex.split(command)
        output = ""

        if args[0] == "hexwrite":
            append = len(args) > 1 and args[1] == "-a"
            index = 2 if append else 1
            remote_path = args[index]
            payload = args[index + 1]
            if payload == "--empty":
                self.files[remote_path] = b""
            else:
                written = bytes.fromhex(payload)
                if append:
                    self.files[remote_path] += written
                else:
                    self.files[remote_path] = written
        elif args[0] == "hexread":
            remote_path = args[1]
            if remote_path not in self.files:
                output = f"hexread: cannot open {remote_path}\r\n"
            else:
                offset = int(args[2])
                count = int(args[3])
                chunk = bytearray(self.files[remote_path][offset:offset + count])
                if self.corrupt_next_read and chunk:
                    chunk[0] ^= 0x01
                    self.corrupt_next_read = False
                if self.malformed_next_read:
                    output += "not-hex\r\n"
                    self.malformed_next_read = False
                else:
                    output += bytes(chunk).hex() + "\r\n"
        else:
            output = f"unknown command: {args[0]}\r\n"

        response = command + "\r\n" + output
        if not self.suppress_prompt:
            response += "msh> "
        self.input.extend(response.encode("ascii"))
        return len(data)


class OpenTrackingSerial:
    def __init__(self):
        self.dtr = True
        self.rts = True
        self.is_open = False
        self.line_states_at_open = None

    def open(self):
        self.line_states_at_open = (self.dtr, self.rts)
        self.is_open = True

    def reset_input_buffer(self):
        pass


def make_client(fake_serial):
    client = MAIA_CLI.MaiaSerialClient.__new__(MAIA_CLI.MaiaSerialClient)
    client.ser = fake_serial
    return client


class MaiaCliTransferTests(unittest.TestCase):
    def test_client_opens_port_without_asserting_reset_lines(self):
        fake = OpenTrackingSerial()
        serial_module = SimpleNamespace(Serial=lambda: fake)
        with patch.object(MAIA_CLI, "serial", serial_module):
            MAIA_CLI.MaiaSerialClient("/dev/tty-test", 115200, 0.1)

        self.assertEqual(fake.line_states_at_open, (False, False))

    def test_push_and_pull_binary_file_with_readback(self):
        payload = bytes(range(256)) + b"\x00MaiaKV\xff"
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "source.bin"
            target = Path(temp_dir) / "download.bin"
            source.write_bytes(payload)
            fake = FakeSerial()
            client = make_client(fake)

            with contextlib.redirect_stdout(io.StringIO()):
                client.push_file(str(source), "/sd/data.bin", chunk_size=31)
                client.pull_file("/sd/data.bin", str(target), chunk_size=47)

            self.assertEqual(fake.files["/sd/data.bin"], payload)
            self.assertEqual(target.read_bytes(), payload)

    def test_empty_file_is_created_and_pulled(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "empty.bin"
            target = Path(temp_dir) / "download.bin"
            source.write_bytes(b"")
            fake = FakeSerial()
            client = make_client(fake)

            with contextlib.redirect_stdout(io.StringIO()):
                client.push_file(str(source), "/sd/empty.bin")
                client.pull_file("/sd/empty.bin", str(target))

            self.assertEqual(fake.files["/sd/empty.bin"], b"")
            self.assertEqual(target.read_bytes(), b"")
            self.assertIn("hexwrite /sd/empty.bin --empty", fake.commands)

    def test_push_fails_on_readback_mismatch_without_retrying_append(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "source.bin"
            source.write_bytes(b"content")
            fake = FakeSerial()
            fake.corrupt_next_read = True
            client = make_client(fake)

            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaisesRegex(RuntimeError, "Readback verification"):
                    client.push_file(str(source), "/sd/data.bin", chunk_size=4)

            self.assertEqual(fake.commands.count("hexwrite -a /sd/data.bin 656e74"), 0)
            self.assertEqual(fake.files["/sd/data.bin"], b"cont")

    def test_push_verifies_write_after_prompt_timeout_without_replaying(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "source.bin"
            source.write_bytes(b"content")
            fake = FakeSerial()
            client = make_client(fake)
            execute = client.execute_command
            timed_out = False

            def timeout_after_first_write(command, timeout=3.0):
                nonlocal timed_out
                output = execute(command, timeout)
                if command.startswith("hexwrite ") and not timed_out:
                    timed_out = True
                    raise TimeoutError("simulated delayed prompt")
                return output

            with contextlib.redirect_stdout(io.StringIO()):
                with patch.object(client, "execute_command",
                                  side_effect=timeout_after_first_write):
                    client.push_file(str(source), "/sd/data.bin", chunk_size=4)

            self.assertTrue(timed_out)
            self.assertEqual(fake.files["/sd/data.bin"], b"content")
            self.assertEqual(
                fake.commands.count("hexwrite /sd/data.bin 636f6e74"), 1
            )

    def test_push_chunk_limit_matches_shell_segment_length(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "source.bin"
            fake = FakeSerial()
            client = make_client(fake)
            remote_path = "/sd/data.bin"
            append_prefix = f"hexwrite -a {remote_path} "
            max_chunk = (
                MAIA_CLI.MAX_SHELL_SEGMENT_CHARS - len(append_prefix)
            ) // 2
            source.write_bytes(bytes(range(max_chunk * 2)))

            with contextlib.redirect_stdout(io.StringIO()):
                client.push_file(str(source), remote_path, chunk_size=max_chunk)

            self.assertEqual(fake.files[remote_path], source.read_bytes())
            self.assertLessEqual(
                max(len(command) for command in fake.commands
                    if command.startswith("hexwrite ")),
                MAIA_CLI.MAX_SHELL_SEGMENT_CHARS,
            )
            with self.assertRaisesRegex(ValueError, "command-line limit"):
                client.push_file(
                    str(source), remote_path, chunk_size=max_chunk + 1
                )

    def test_max_push_chunk_size_accounts_for_remote_path_length(self):
        remote_path = "/sd/scicalc-stream.mobj"
        append_prefix = f"hexwrite -a {remote_path} "
        expected = (
            MAIA_CLI.MAX_SHELL_SEGMENT_CHARS - len(append_prefix)
        ) // 2

        self.assertEqual(MAIA_CLI._max_push_chunk_size(remote_path), expected)
        self.assertEqual(expected, 45)

    def test_bulk_push_verifies_larger_readback_blocks(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "source.bin"
            payload = bytes(range(256)) * 2
            source.write_bytes(payload)
            fake = FakeSerial()
            client = make_client(fake)

            with contextlib.redirect_stdout(io.StringIO()):
                client.push_file(
                    str(source), "/sd/data.bin", chunk_size=31,
                    verify_each_chunk=False,
                )

            read_commands = [cmd for cmd in fake.commands
                             if cmd.startswith("hexread ")]
            self.assertEqual(fake.files["/sd/data.bin"], payload)
            self.assertEqual(len(read_commands), 2)

    def test_readback_retries_a_delayed_prompt_once(self):
        fake = FakeSerial()
        fake.files["/sd/data.bin"] = b"payload"
        client = make_client(fake)
        execute = client.execute_command
        timed_out = False

        def timeout_after_first_read(command, timeout=3.0):
            nonlocal timed_out
            output = execute(command, timeout)
            if command.startswith("hexread ") and not timed_out:
                timed_out = True
                raise TimeoutError("simulated delayed prompt")
            return output

        with patch.object(client, "execute_command",
                          side_effect=timeout_after_first_read):
            self.assertEqual(client._read_remote_bytes("/sd/data.bin", 0, 7),
                             b"payload")

        self.assertEqual(
            fake.commands.count("hexread /sd/data.bin 0 7"), 2
        )

    def test_pull_errors_do_not_replace_existing_local_file(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            target = Path(temp_dir) / "existing.bin"
            target.write_bytes(b"keep")
            fake = FakeSerial()
            client = make_client(fake)

            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaises(FileNotFoundError):
                    client.pull_file("/sd/missing.bin", str(target))

            self.assertEqual(target.read_bytes(), b"keep")

    def test_malformed_read_response_is_not_treated_as_eof(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            target = Path(temp_dir) / "download.bin"
            fake = FakeSerial()
            fake.files["/sd/data.bin"] = b"data"
            fake.malformed_next_read = True
            client = make_client(fake)

            with contextlib.redirect_stdout(io.StringIO()):
                with self.assertRaisesRegex(RuntimeError, "Invalid hexread response"):
                    client.pull_file("/sd/data.bin", str(target))

            self.assertFalse(target.exists())

    def test_remote_paths_and_chunk_limits_are_validated(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "source.bin"
            source.write_bytes(b"x")
            fake = FakeSerial()
            client = make_client(fake)

            with self.assertRaisesRegex(ValueError, "unsupported characters"):
                client.push_file(str(source), "/sd/a;echo x")
            with self.assertRaisesRegex(ValueError, "absolute"):
                client.push_file(str(source), "relative.bin")
            with self.assertRaisesRegex(ValueError, "Chunk size"):
                client.push_file(str(source), "/sd/data.bin", chunk_size=257)
            self.assertEqual(fake.commands, [])

    def test_cli_resolves_relative_paths_under_sd(self):
        with tempfile.TemporaryDirectory() as temp_dir:
            source = Path(temp_dir) / "upload.bin"
            target = Path(temp_dir) / "download.bin"
            source.write_bytes(b"upload")
            push_fake = FakeSerial()
            push_args = SimpleNamespace(
                port="unused",
                baud=115200,
                remote="nested/upload.bin",
                local=str(source),
                chunk_size=32,
            )
            pull_args = SimpleNamespace(
                port="unused",
                baud=115200,
                remote="nested/upload.bin",
                local=str(target),
                chunk_size=64,
            )

            with patch.object(
                MAIA_CLI, "MaiaSerialClient", return_value=make_client(push_fake)
            ):
                with contextlib.redirect_stdout(io.StringIO()):
                    MAIA_CLI.cmd_push(push_args)

            pull_fake = FakeSerial()
            pull_fake.files["/sd/nested/upload.bin"] = b"upload"
            with patch.object(
                MAIA_CLI, "MaiaSerialClient", return_value=make_client(pull_fake)
            ):
                with contextlib.redirect_stdout(io.StringIO()):
                    MAIA_CLI.cmd_pull(pull_args)

            self.assertEqual(push_fake.files["/sd/nested/upload.bin"], b"upload")
            self.assertEqual(target.read_bytes(), b"upload")

    def test_execute_command_requires_prompt(self):
        fake = FakeSerial()
        fake.suppress_prompt = True
        client = make_client(fake)

        with self.assertRaises(TimeoutError):
            client.execute_command("echo test", timeout=0.01)


if __name__ == "__main__":
    unittest.main()
