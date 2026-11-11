#!/usr/bin/env python3
"""Load the isolated scheduler probe into RAM, capture its result, then reset.

Requires esptool 4.12/pyserial. No flash-write commands are used. Display/desktop
are unavailable during this short diagnostic; reset returns to the flash image.
"""
import argparse
import contextlib
from pathlib import Path
import sys
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/cu.usbmodem1301')
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--capture-seconds', type=float, default=8)
    parser.add_argument('--validate-only', action='store_true')
    args = parser.parse_args()
    if not 1 <= args.capture_seconds <= 30:
        parser.error('capture-seconds must be between 1 and 30')

    import esptool
    from esptool.bin_image import LoadFirmwareImage
    image = LoadFirmwareImage('esp32p4', str(args.image))
    # Existing linker.ld L2MEM_LOW window: validate before opening any port.
    if not image.segments or not all(
        0x4FF00000 <= segment.addr < segment.addr + len(segment.data) <= 0x4FF28000
        for segment in image.segments
    ) or not any(segment.addr <= image.entrypoint < segment.addr + len(segment.data)
                 for segment in image.segments):
        parser.error('image is outside the existing L2MEM_LOW RAM-load window')
    if args.validate_only:
        print('PASS: image and entry point confined to L2MEM_LOW; no device accessed')
        return 0

    class Tee:
        def __init__(self, logfile):
            self.logfile = logfile

        def write(self, text):
            self.logfile.write(text)
            self.logfile.flush()
            sys.__stdout__.write(text)
            sys.__stdout__.flush()

        def flush(self):
            self.logfile.flush()
            sys.__stdout__.flush()

    output = ''
    with args.log.open('w') as logfile, contextlib.redirect_stdout(Tee(logfile)):
        esp = esptool.get_default_connected_device(
            [args.port], port=args.port, connect_attempts=3,
            initial_baud=115200, chip='esp32p4')
        try:
            esptool.main(['--chip', 'esp32p4', '--no-stub', 'load_ram', str(args.image)], esp=esp)
            port = esp._port
            port.timeout = 0.1
            deadline = time.monotonic() + args.capture_seconds
            while time.monotonic() < deadline:
                data = port.read(8192)
                if data:
                    chunk = data.decode('utf-8', 'replace')
                    output += chunk
                    print(chunk, end='')
        finally:
            print('\n[HOST] Resetting to the preserved flash application')
            try:
                esp.hard_reset()
            finally:
                esp._port.close()
        passed = 'MAIA_NATIVE_PREEMPT_PASS' in output and 'MAIA_NATIVE_PREEMPT_FAIL' not in output
        print('[HOST] Native preemption:', 'PASS' if passed else 'FAIL / NOT DEMONSTRATED')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
