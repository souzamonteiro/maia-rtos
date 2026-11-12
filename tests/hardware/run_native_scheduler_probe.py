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


def evaluate_mipi(output, allow_stop=False):
    """A complete read-only observation, never a power-off/idle certificate."""
    if not allow_stop and 'VDMA_STOP' in output:
        return False
    lines = output.replace('\r', '').splitlines()
    schemas = {
        'MIPI_SAMPLE': 'i cpu sys phy cpu_after sys_after phy_after valid stable'.split(),
        'MIPI_DSI': 'i bridge dpi fifo power mode reset status'.split(),
        'MIPI_CSI': 'i host shutdown reset stop bridge'.split(),
        'MIPI_DMA': 'i cfg enable abort'.split(),
        'MIPI_CH': 'i ch cfg0 cfg1 transferred fifo sar dar llp'.split(),
    }
    records = {key: {} for key in schemas}
    if lines.count('MAIA_MIPI_STATE_PROBE schema=1 read-only') != 1 or lines.count('MAIA_NATIVE_MIPI_PASS') != 1:
        return False
    if 'FAIL' in output or 'FATAL' in output:
        return False
    for line in lines:
        if not line.startswith('MIPI_'):
            continue
        parts = line.split()
        kind = parts[0]
        if kind not in schemas:
            return False
        pattern = kind + ''.join(' ' + key + r'=([0-9a-f]{8})' for key in schemas[kind])
        match = re.fullmatch(pattern, line)
        if not match:
            return False
        row = dict(zip(schemas[kind], (int(value, 16) for value in match.groups())))
        index = (row['i'], row['ch']) if kind == 'MIPI_CH' else row['i']
        if row['i'] >= 8 or (kind == 'MIPI_CH' and row['ch'] >= 4) or index in records[kind]:
            return False
        records[kind][index] = row
    if any(len(rows) != (32 if kind == 'MIPI_CH' else 8) for kind, rows in records.items()):
        return False
    for i, row in records['MIPI_SAMPLE'].items():
        expected = ((row['sys'] >> 12) & 1) | (((row['sys'] >> 11) & 1) << 1) | (((row['sys'] >> 10) & 1) << 2)
        if row['cpu'] & (1 << 13) and row['sys'] & (1 << 5):
            expected |= 8
        if row['valid'] != expected or row['stable'] != 1 or any(row[key] != row[key + '_after'] for key in ('cpu', 'sys', 'phy')):
            return False
        skipped = []
        if not expected & 1:
            skipped.append(records['MIPI_DSI'][i])
        csi = records['MIPI_CSI'][i]
        if not expected & 2 and any(csi[key] for key in ('host', 'shutdown', 'reset', 'stop')):
            return False
        if not expected & 4 and csi['bridge']:
            return False
        if not expected & 8:
            skipped += [records['MIPI_DMA'][i]] + [records['MIPI_CH'][(i, c)] for c in range(4)]
        if any(value for entry in skipped for key, value in entry.items() if key not in ('i', 'ch')):
            return False
    return True


def evaluate_vdma_stop(output, armed=False):
    if not evaluate_mipi(output, allow_stop=True):
        return False
    lines = output.replace('\r', '').splitlines()
    if lines.count('MAIA_VDMA_STOP_PROBE optional empty-controller writes; no transfers') != 1:
        return False
    expected = ('VDMA_STOP armed=' + ('00000001' if armed else '00000000') +
                ' stage=00000003 adopted=' + ('00000000' if armed else '00000001') +
                ' changed=' + ('00000001' if armed else '00000000') +
                ' requested=00000000 polls=00000001 cfg=00000000 channels=00000000 abort=00000000'
                ' fifo=00000000 preserved=00000001 duplicate_busy=00000001')
    if len([line for line in lines if line.startswith('VDMA_STOP')]) != 1 or expected not in lines:
        return False
    # The actual diagnostic requires the controller/channels initially empty.
    for line in lines:
        if line.startswith('MIPI_SAMPLE '):
            if int(re.search(r' valid=([0-9a-f]{8})', line)[1], 16) & 8 == 0:
                return False
        if line.startswith('MIPI_DMA '):
            if not line.endswith('cfg=00000000 enable=00000000 abort=00000000'):
                return False
        if line.startswith('MIPI_CH '):
            if any(int(re.search(' ' + key + r'=([0-9a-f]{8})', line)[1], 16)
                   for key in ('transferred', 'fifo', 'sar', 'dar', 'llp')):
                return False
    return True


def evaluate_ldo3(output):
    version = re.findall(r'^LDO3_VERSION chip_major=1 chip_minor=3 block_major=0 block_minor=([1-7]) disabled=0 rs_fail=0 corrected=([0-7])$', output, re.M)
    trim = re.findall(r'^LDO3_TRIM k=(\d+) vos=(\d+) c=(\d+)$', output, re.M)
    plan = re.findall(r'^LDO3_PLAN target_mv=2500 dref=(\d+) mul=(\d+) default_mask=(\d+) estimated_uv=(\d+) error_uv=(\d+)$', output, re.M)
    coeff = re.findall(r'^LDO3_COEFF k1000=(-?\d+) vos1000=(-?\d+) c1000=(-?\d+)$', output, re.M)
    current = re.findall(r'^LDO3_CURRENT enabled=[01] sw_owner=[01] bypass=[01] source=[0-7] dref=(\d+) mul=([0-7]) vdet=[01]$', output, re.M)
    if not all(len(x) == 1 for x in (version, trim, plan, coeff, current)):
        return False
    k, v, c = map(int, trim[0])
    if k>255 or v>63 or c>63 or int(current[0][0])>15:
        return False
    def signed(raw, sign):
        return -(raw-sign) if raw & sign else raw
    K, V, C = 975+signed(k,128) if k else 1000, -3+signed(v,32) if v else 0, 990+signed(c,32) if c else 1000
    candidates = []
    for d in range(16):
        ref = 10+d if d<9 else 2*d+2
        for m in range(8):
            predicted = (ref*K+20*V)*(4000+m*C)
            candidates.append((abs(200000000-predicted), d, m, predicted))
    error, dref, mul, predicted = min(candidates)
    expected = (dref, mul, (not k)*1+(not v)*2+(not c)*4, (predicted+40)//80, (error+40)//80)
    return (tuple(map(int, plan[0])) == expected and tuple(map(int, coeff[0])) == (K,V,C) and
            output.split('\n').count('LDO3_READONLY samples=16 unchanged=1 errors=0') == 1 and
            output.split('\n').count('MAIA_NATIVE_LDO3_PASS') == 1 and
            all(word not in output for word in ('FAIL','MAIA_NATIVE_FATAL','MAIA_STACK_FAULT')))


def evaluate_ldo3_acquire(output, cycle=False):
    records = re.findall(r'^LDO3_ACQUIRE stage=5 adopted=([01]) changed=([01]) preserved=1 duplicate_busy=1$', output, re.M)
    clocks = re.findall(r'^LDO3_CLOCKS sys_active=([01]) phy_active=([01])$', output, re.M)
    expected = ('0', '1') if cycle else ('1', '0')
    marker = output.split('\n').count('LDO3_CYCLE off_confirmed=1 clocks_gated=1')
    current = re.findall(r'^LDO3_CURRENT enabled=1 sw_owner=1 bypass=0 source=0 dref=(\d+) mul=(\d+) vdet=1$', output, re.M)
    planned = re.findall(r'^LDO3_PLAN target_mv=2500 dref=(\d+) mul=(\d+) ', output, re.M)
    return (evaluate_ldo3(output) and records == [expected] and len(clocks) == 1 and
            len(current) == 1 and current == planned and
            (not cycle or clocks == [('0', '0')]) and marker == int(cycle))


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
    parser.add_argument('--vdma-stop', action='store_true', help='verify stopped VDMA adoption')
    parser.add_argument('--vdma-arm-empty', action='store_true', help='arm empty VDMA and verify native global stop; no transfers')
    parser.add_argument('--mipi-state', action='store_true', help='verify read-only inherited MIPI/DW-GDMA state')
    parser.add_argument('--ldo3-acquire', action='store_true', help='verify boot LDO3 active-state adoption')
    parser.add_argument('--ldo3-cycle', action='store_true', help='verify guarded VO3 disable/enable through native acquire')
    parser.add_argument('--ldo3', action='store_true', help='verify read-only VO3 trim/planning and unchanged hardware')
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

    if args.vdma_arm_empty:
        args.vdma_stop = True
    if args.vdma_stop:
        args.mipi_state = True
    if args.mipi_state and (args.ldo3 or args.ldo3_acquire or args.ldo3_cycle or args.panel_reset or
                            args.expander or args.touch_identify or args.serial_output_event or
                            args.serial_output or args.serial_input or args.expect_fatal_cause is not None):
        parser.error('MIPI state requires a separate read-only profile')
    if args.ldo3_cycle:
        args.ldo3_acquire = True
    if args.ldo3_acquire:
        args.ldo3 = True
    if args.ldo3 and (args.panel_reset or args.expander or args.touch_identify or args.serial_output_event or
                      args.serial_output or args.serial_input or args.expect_fatal_cause is not None):
        parser.error("LDO3 requires a separate read-only profile")
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
        if args.vdma_stop:
            passed = evaluate_vdma_stop(output, args.vdma_arm_empty)
        elif args.mipi_state:
            passed = evaluate_mipi(output)
        if args.ldo3:
            passed = evaluate_ldo3(output.replace("\r", ""))
        if args.ldo3_acquire:
            passed = evaluate_ldo3_acquire(output.replace("\r", ""), args.ldo3_cycle)
        if args.serial_input:
            passed = passed and stimulus.phase == 2
        if args.serial_output:
            passed = passed and evaluate_serial_output(output)
        if args.serial_output_event:
            passed = passed and evaluate_serial_output_event(output)
        print('[HOST] VDMA stop:' if args.vdma_stop else '[HOST] MIPI observation:' if args.mipi_state else '[HOST] LDO3 planning:' if args.ldo3 else '[HOST] Touch identity:' if args.touch_identify else '[HOST] Expected fatal trap:' if args.expect_fatal_cause is not None else '[HOST] Native preemption:', 'PASS' if passed else 'FAIL / NOT DEMONSTRATED')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
