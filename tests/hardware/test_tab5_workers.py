#!/usr/bin/env python3
"""Exercise shell workers after changing core policy; real serial device only.

Opens the desktop terminal, runs a background job, compiles a temporary program,
validates and executes its module, removes its temporary files and exits.
Does not flash firmware or certify physical display/audio/peripheral behavior.
"""
import argparse
import importlib.util
from pathlib import Path
import re
import sys
import time
import uuid

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/cu.usbmodem1301')
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--boot-wait', type=float, default=0,
                        help='Capture startup after flashing before opening the terminal')
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('maia_cli', ROOT / 'tools/maia-cli/maia.py')
    cli = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cli)
    prefix = '/sd/.w' + uuid.uuid4().hex[:12]
    temporary_source = prefix + '.ms'
    temporary_module = prefix + '.mobj'

    with args.log.open('w') as logfile:
        def record(text):
            print(text, flush=True)
            logfile.write(text + '\n')
            logfile.flush()

        client = cli.MaiaSerialClient(args.port)
        try:
            if args.boot_wait > 0:
                record(client.wait_for_prompt(timeout=args.boot_wait))
            client.ser.write(b'\x1b\x1b')
            record(client.wait_for_prompt(timeout=2))
            client.ser.write(b't')
            time.sleep(0.5)

            def command(text, timeout=3):
                assert len(text.encode('ascii')) <= 62, 'desktop terminal input limit exceeded'
                record('[HOST COMMAND] ' + text)
                # The desktop keyboard ring holds 63 pending characters.
                # Pace serial bursts so longer commands are not dropped there.
                client.flush_input()
                data = (text + '\n').encode('ascii')
                for offset in range(0, len(data), 32):
                    client.ser.write(data[offset:offset + 32])
                    time.sleep(0.05)
                output = client.wait_for_prompt(timeout=timeout)
                record(output)
                return output

            output = command('echo WORKER_BACKGROUND_OK &')
            match = re.search(r'\[(\d+)\] echo WORKER_BACKGROUND_OK', output)
            assert match, output
            job_id = match[1]
            output = command('jobs')
            assert re.search(r'\[' + job_id + r'\] Done\s+.*WORKER_BACKGROUND_OK.*exit 0', output), output
            # Exercise repeated lexer/parser work without depending on external
            # libraries or overwriting installed apps. Record actual duration.
            source = 'i32 main() {\n i32 result = 0;\n' + ' result = result + 1;\n' * 32 + ' return result;\n}\n'
            record('[HOST TEMPORARY SOURCE] ' + temporary_source)
            data = source.encode('ascii')
            # Integrated terminal has no serial msh> prompt. Use the same
            # CLI transport, verifying each chunk rather than waiting for it.
            # Desktop input reserves two bytes of its 64-byte buffer.
            for offset in range(0, len(data), 14):
                chunk = data[offset:offset + 14]
                command('hexwrite ' + ('-a ' if offset else '') + temporary_source + ' ' + chunk.hex(), timeout=0.2)
                output = command('hexread ' + temporary_source + ' ' + str(offset) + ' ' + str(len(chunk)), timeout=0.5)
                assert chunk.hex() in output, output
            started = time.monotonic()
            output = command('maiasc ' + temporary_source + ' -o ' + temporary_module, timeout=1)
            success = '[SUCCESS] Compiled ' + temporary_source + ' -> ' + temporary_module
            while success not in output and time.monotonic() - started < 120:
                if any(marker in output for marker in ('task_wdt', 'Guru Meditation', 'assert failed:')):
                    break
                chunk = client.wait_for_prompt(timeout=1)
                if chunk:
                    record(chunk)
                    output += chunk
            record('[HOST COMPILATION SECONDS] ' + str(round(time.monotonic() - started, 2)))
            assert success in output, output
            assert not any(marker in output for marker in ('task_wdt', 'Guru Meditation', 'assert failed:')), output
            output = command('modinfo ' + temporary_module)
            assert '[VALID MNM-1 BINARY]' in output, output
            output = command('runelf ' + temporary_module)
            assert 'Exit code: 32' in output, output
            command('rm ' + temporary_module)
            command('rm ' + temporary_source)
            output = command('echo WORKERS_TEST_OK')
            assert 'WORKERS_TEST_OK' in output, output
            command('exit', timeout=1)
            record('PASS: background job, on-device compilation, valid module returned 32, files removed, desktop restored')
        finally:
            client.close()


if __name__ == '__main__':
    main()
