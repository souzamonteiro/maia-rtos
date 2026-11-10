#!/usr/bin/env python3
"""Real Tab5 keyboard/serial smoke test; requires the recovered apps firmware.

Run with the same pyserial Python environment as tools/maia-cli/maia.py.
The test opens/closes desktop windows and sounds a ten-second alarm.
It verifies serial reports, not pixel rendering or physical audio output.
"""
import argparse
import importlib.util
from pathlib import Path
import sys
import re
import time

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/cu.usbmodem1301')
    parser.add_argument('--log', type=Path, required=True)
    args = parser.parse_args()
    spec = importlib.util.spec_from_file_location('maia_cli', ROOT / 'tools/maia-cli/maia.py')
    cli = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(cli)

    with args.log.open('w') as logfile:
        def record(text):
            print(text, flush=True)
            logfile.write(text + '\n')
            logfile.flush()

        client = cli.MaiaSerialClient(args.port)
        try:
            def receive(seconds=2):
                output = client.wait_for_prompt(timeout=seconds)
                record(output)
                return output

            def terminal():
                record('[HOST] Escape active window; open integrated terminal')
                client.ser.write(b'\x1b\x1b')
                receive()
                client.ser.write(b't')
                time.sleep(0.5)

            def command(text, timeout=3):
                record('[HOST COMMAND] ' + text)
                output = client.execute_command(text, timeout=timeout)
                record(output)
                return output

            def keys(text, seconds=2):
                record('[HOST KEYS] ' + repr(text))
                client.ser.write(text.encode('ascii'))
                return receive(seconds)

            terminal()
            listing = command('ls /sd/apps')
            assert 'scicalc.mobj' in listing and 'alarm.mobj' in listing, listing
            command('exit', 1)
            output = keys('g', 3) # desktop manifest hotkey
            assert 'Executing maia_module_main' in output, output
            assert 'Unresolved external' not in output, output
            for expression, expected in [('2+3*4=', '14'),
                                         ('c2.5^2=', '6.25'),
                                         ('c9^0.5=', '3'),
                                         ('c0s', '0'),
                                         ('c1/0=', 'ERRO DOM')]:
                output = keys(expression)
                results = re.findall(r'\[SciCalc\][^\r\n]* = ([^\r\n]+)', output)
                assert results and results[-1].strip() == expected, output
            terminal()
            command('exit', 1)
            output = keys('l', 3) # desktop manifest hotkey
            assert 'Executing maia_module_main' in output, output
            assert 'Unresolved external' not in output, output
            output = keys('1', 12)
            assert '[Alarm] armed for 10' in output, output
            assert '[Alarm] fired' in output, output
            output = keys('c')
            assert '[Alarm] cancelled' in output, output
            output = keys('1', 1)
            assert '[Alarm] armed for 10' in output, output
            output = keys('c', 12)
            assert '[Alarm] cancelled' in output and '[Alarm] fired' not in output, output
            terminal()
            assert 'APPS_RECOVERY_OK' in command('echo APPS_RECOVERY_OK')
            command('exit', 1)
            record('PASS: scientific calculations, domain error, timer fire/cancel and return to desktop')
        finally:
            client.close()


if __name__ == '__main__':
    main()
