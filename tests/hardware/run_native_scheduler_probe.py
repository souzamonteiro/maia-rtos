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


def evaluate_touch_identity(output):
    states = re.findall(r'^TOUCH_STATUS result_code=0 raw=(0|4)$', output, re.M)
    ids = re.findall(r'^TOUCH_ID reads=16 firmware=([13]) final=([13]) errors=0$', output, re.M)
    return (len(states) == 1 and len(ids) == 1 and ids[0][0] == ids[0][1] and
            output.split('\n').count('TOUCH_ABSENT address=0x56 NACK') == 1 and
            output.split('\n').count('TOUCH_INJECT sampled_SCL_low TIMEOUT') == 1 and
            output.split('\n').count('MAIA_NATIVE_TOUCH_PASS') == 1 and
            all(word not in output for word in ('FAIL', 'MAIA_NATIVE_FATAL', 'MAIA_STACK_FAULT')))


def evaluate_expander(output):
    banks = re.findall(r'^EXPANDER bank=(67|68) rounds=16 mirror=1 unchanged=1 dir=(\d+) out=(\d+)$', output, re.M)
    return (evaluate_touch_identity(output) and len(banks) == 2 and
            sorted(bank[0] for bank in banks) == ['67', '68'] and
            all(int(v) <= 255 for bank in banks for v in bank[1:]) and
            output.split('\n').count('EXPANDER_ABSENT_WRITE NACK') == 1)


def evaluate_panel_reset(output):
    result = re.findall(r'^PANEL_RESET stage=9 changed=1 preserved=1 elapsed_us=(\d+) gpio23_input=1$', output, re.M)
    return evaluate_expander(output) and len(result) == 1 and int(result[0]) >= 200000


def serial_output_record(source, sequence):
    prefix = 'TXR:' + chr(ord('A') + source) + ':' + f'{sequence:02d}' + ':'
    return prefix + ''.join(chr(ord('a') + (source*7 + sequence + i) % 26)
                            for i in range(9, 47)) + '\n'


def evaluate_serial_output(output):
    if not evaluate_capture(output) or 'LEGACY_MUST_NOT_APPEAR' in output:
        return False
    records = [line for line in output.split('\n') if 'TXR:' in line]
    if len(records) != 80:
        return False
    counts = [0, 0]
    for line in records:
        source = 0 if line.startswith('TXR:A:') else 1 if line.startswith('TXR:B:') else -1
        if source < 0 or counts[source] >= 40 or line+'\n' != serial_output_record(source, counts[source]):
            return False
        counts[source] += 1
    totals = re.findall(r'^TX_SUM records=80 bytes=3840 errors=0$', output, re.M)
    hardware = re.findall(r'^TX_HW irq=(\d+) full=(\d+) busy=(\d+)$', output, re.M)
    return (counts == [40, 40] and len(totals) == 1 and len(hardware) == 1 and
            int(hardware[0][0]) >= 250 and int(hardware[0][1]) > 0 and int(hardware[0][2]) > 0)


def evaluate_serial_output_event(output):
    if not evaluate_serial_output(output):
        return False
    events = re.findall(r'^TX_EVENT head=(\d+) tail=(\d+) idle=(\d+)$', output, re.M)
    waits = re.findall(r'^TX_WAIT empty=(\d+) retry=(\d+) timeout=(\d+) irq_write=(\d+)$', output, re.M)
    return (output.split('\n').count('TX_EVENT_TRIGGER') == 1 and len(events) == len(waits) == 1 and
            int(events[0][0]) >= 20 and int(events[0][1]) >= 20 and int(events[0][2]) >= 39 and
            int(waits[0][0]) >= 2 and int(waits[0][1]) > 0 and int(waits[0][2]) > 0 and int(waits[0][3]) == 1)


class SerialInputStimulus:
    """Two paced binary batches, armed only by the RAM serial probe markers."""
    def __init__(self, port, pause=time.sleep):
        self.port, self.pause, self.phase = port, pause, 0

    def feed(self, output):
        clean = output.replace('\r', '')
        for phase, marker, first, last in ((0, 'fill', 0, 64), (1, 'drain', 64, 256)):
            if self.phase == phase and 'MAIA_SERIAL_RX_READY ' + marker + '\n' in clean:
                for offset in range(first, last, 16):
                    data = bytes(range(offset, min(offset + 16, last)))
                    if self.port.write(data) != len(data):
                        raise RuntimeError('short serial fixture write')
                    self.pause(0.005)
                self.phase += 1
                print('[HOST] Serial RX ' + marker + ' sent=' + str(last-first))


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
    parser.add_argument('--panel-reset', action='store_true', help='verify LCD/touch reset, preserved bits and touch recovery')
    parser.add_argument('--expander', action='store_true', help='verify both expanders and same-value writes, including touch regression')
    parser.add_argument('--touch-identify', action='store_true', help='verify boot-only touch identity and negative probes')
    parser.add_argument('--serial-output-event', action='store_true', help='verify TX empty waits and IRQ enqueue wakeup')
    parser.add_argument('--serial-output', action='store_true', help='verify 80 complete TX records from two producers')
    parser.add_argument('--serial-input', action='store_true', help='send binary RX fixture after RAM probe markers')
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

    if args.panel_reset:
        args.expander = True
    if args.expander:
        args.touch_identify = True
    if args.touch_identify and (args.serial_output_event or args.serial_output or args.serial_input or
                                args.expect_fatal_cause is not None):
        parser.error("touch probe cannot be combined with scheduler/serial profiles")
    if args.serial_output_event:
        args.serial_output = True
    if args.serial_output and (args.serial_input or args.expect_fatal_cause is not None):
        parser.error('serial output workload cannot be combined with input/fatal profiles')
    if args.serial_input and args.expect_fatal_cause is not None:
        parser.error('serial input workload cannot be combined with a fatal profile')

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
            stimulus = SerialInputStimulus(port) if args.serial_input else None
            deadline = time.monotonic() + args.capture_seconds
            while time.monotonic() < deadline:
                data = port.read(8192)
                if data:
                    chunk = data.decode('utf-8', 'replace')
                    output += chunk
                    print(chunk, end='')
                    if stimulus is not None:
                        stimulus.feed(output)
        finally:
            print('\n[HOST] Resetting to the preserved flash application')
            try:
                esp.hard_reset()
            finally:
                esp._port.close()
        passed = evaluate_capture(output, args.expect_fatal_cause, args.expect_fatal_pc,
                                  args.expect_stack_reason, args.expect_stack_guard, args.expect_stack_sp)
        if args.touch_identify:
            passed = evaluate_touch_identity(output.replace("\r", ""))
        if args.expander:
            passed = evaluate_expander(output.replace("\r", ""))
        if args.panel_reset:
            passed = evaluate_panel_reset(output.replace("\r", ""))
        if args.serial_input:
            passed = passed and stimulus.phase == 2
        if args.serial_output:
            passed = passed and evaluate_serial_output(output)
        if args.serial_output_event:
            passed = passed and evaluate_serial_output_event(output)
        print('[HOST] Touch identity:' if args.touch_identify else '[HOST] Expected fatal trap:' if args.expect_fatal_cause is not None else '[HOST] Native preemption:', 'PASS' if passed else 'FAIL / NOT DEMONSTRATED')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
