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
import re


def evaluate_capture(output, expected_fatal=None, expected_pc=None, expected_stack=None,
                     expected_guard=None, expected_sp=None):
    if expected_fatal is None:
        return ('MAIA_NATIVE_PREEMPT_PASS' in output and
                'MAIA_NATIVE_PREEMPT_FAIL' not in output and 'MAIA_NATIVE_FATAL' not in output and
                'MAIA_STACK_FAULT' not in output and expected_stack is None)
    armed = re.findall(r'FAULT_ARMED pc=([0-9a-f]{8})', output)
    fatal = re.findall(r'MAIA_NATIVE_FATAL cause=([0-9a-f]{8}) pc=([0-9a-f]{8}) value=([0-9a-f]{8}) sp=([0-9a-f]{8})', output)
    if expected_pc is None:
        if len(armed) != 1:
            return False
        expected_pc = int(armed[0], 16)
    stack = re.findall(r'MAIA_STACK_FAULT reason=([1-5]) emergency=([01]) guard=([01])', output)
    stack_ok = ('MAIA_STACK_FAULT' not in output if expected_stack is None else
                len(stack) == 1 and int(stack[0][0]) == expected_stack and stack[0][1] == '1' and
                (expected_guard is None or int(stack[0][2]) == expected_guard))
    return (stack_ok and len(armed) <= 1 and len(fatal) == 1 and
            (int(fatal[0][0], 16) & 0x80000fff) == expected_fatal and
            int(fatal[0][1], 16) == expected_pc and
            (expected_sp is None or int(fatal[0][3], 16) == expected_sp) and
            (not armed or int(armed[0], 16) == expected_pc) and 'FAULT_RESUMED' not in output and
            'MAIA_NATIVE_PREEMPT_PASS' not in output and 'MAIA_NATIVE_PREEMPT_FAIL' not in output)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/cu.usbmodem1301')
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--capture-seconds', type=float, default=8)
    parser.add_argument('--expect-stack-reason', type=int, choices=(1,2,3,4,5))
    parser.add_argument('--expect-stack-guard', type=int, choices=(0,1))
    parser.add_argument('--expect-stack-sp', type=lambda value: int(value,0))
    parser.add_argument('--expect-fatal-pc', type=lambda value: int(value, 0))
    parser.add_argument('--expect-fatal-cause', type=lambda value: int(value, 0), choices=(2, 3, 11))
    parser.add_argument('--validate-only', action='store_true')
    args = parser.parse_args()
    if not 1 <= args.capture_seconds <= 30:
        parser.error('capture-seconds must be between 1 and 30')

    if (args.expect_fatal_cause is None) != (args.expect_fatal_pc is None):
        parser.error('fatal tests require both expected cause and PC from the test ELF symbol')

    if args.expect_stack_reason is not None and args.expect_fatal_cause is None:
        parser.error('stack checks require expected fatal cause and PC')
    if (args.expect_stack_guard is not None or args.expect_stack_sp is not None) and args.expect_stack_reason is None:
        parser.error('guard/SP expectations require a stack reason')
    if args.expect_stack_sp is not None and not 0 <= args.expect_stack_sp <= 0xffffffff:
        parser.error('expected source SP must fit RV32')

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
    if args.expect_fatal_pc is not None and (args.expect_fatal_pc & 1 or not any(
        segment.addr <= args.expect_fatal_pc < segment.addr + len(segment.data)
        for segment in image.segments
    )):
        parser.error('expected fault PC must be halfword-aligned inside the loaded image')
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
        passed = evaluate_capture(output, args.expect_fatal_cause, args.expect_fatal_pc,
                                  args.expect_stack_reason, args.expect_stack_guard, args.expect_stack_sp)
        print('[HOST] Expected fatal trap:' if args.expect_fatal_cause is not None else '[HOST] Native preemption:', 'PASS' if passed else 'FAIL / NOT DEMONSTRATED')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
