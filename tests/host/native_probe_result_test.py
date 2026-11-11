import importlib.util
from pathlib import Path
import sys
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('runner', Path(__file__).resolve().parents[1] / 'hardware/run_native_scheduler_probe.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
evaluate = runner.evaluate_capture
good = 'FAULT_ARMED pc=4ff00836\nMAIA_NATIVE_FATAL cause=30000002 pc=4ff00836 value=00000000 sp=4ff02000\n'
assert evaluate(good, 2)
assert not evaluate(good, 3)
assert not evaluate(good.replace('pc=4ff00836 value', 'pc=4ff00838 value'), 2)
assert not evaluate(good + 'FAULT_RESUMED', 2)
assert not evaluate(good + 'MAIA_NATIVE_PREEMPT_PASS', 2)
assert not evaluate(good + good, 2)
assert not evaluate(good.split('\n', 1)[1], 2)
assert not evaluate('FAULT_RESUMED', 2)
assert evaluate('MAIA_NATIVE_PREEMPT_PASS')
assert not evaluate('MAIA_NATIVE_PREEMPT_PASS MAIA_NATIVE_PREEMPT_FAIL')
assert not evaluate('MAIA_NATIVE_PREEMPT_PASS ' + good)
print('PASS: fatal evidence requires exact expected PC/cause and rejects resumed/missing/duplicate output')

# Expected PC from ELF remains authoritative if early serial output is lost.
assert evaluate(good.split('\n', 1)[1], 2, 0x4ff00836)
assert not evaluate(good, 2, 0x4ff00838)
assert not evaluate(good.replace('pc=4ff00836', 'pc=4ff00838', 1), 2, 0x4ff00836)

assert not evaluate(good + 'MAIA_NATIVE_PREEMPT_FAIL', 2)
assert not evaluate(good.replace('cause=30000002', 'cause=b0000002'), 2)

stack = 'MAIA_STACK_FAULT reason=1 emergency=1 guard=1\n' + good.replace('cause=30000002', 'cause=3800000b')
assert evaluate(stack, 11, 0x4ff00836, 1, 1, 0x4ff02000)
assert not evaluate(stack, 11, 0x4ff00836)
assert not evaluate(stack, 11, 0x4ff00836, 2, 1)
assert not evaluate(stack.replace('emergency=1', 'emergency=0'), 11, 0x4ff00836, 1, 1)
assert not evaluate(stack, 11, 0x4ff00836, 1, 0)
assert not evaluate(stack, 11, 0x4ff00836, 1, 1, 0)
assert not evaluate(stack + stack, 11, 0x4ff00836, 1)
assert not evaluate('MAIA_NATIVE_PREEMPT_PASS MAIA_STACK_FAULT')
assert not evaluate(good.replace(' sp=4ff02000',''), 2)


class FakePort:
    def __init__(self):
        self.data = bytearray()
    def write(self, data):
        self.data.extend(data)
        return len(data)

port = FakePort()
stimulus = runner.SerialInputStimulus(port, lambda seconds: None)
stimulus.feed('unrelated output')
assert not port.data
stimulus.feed('MAIA_SERIAL_RX_READY fi')
assert not port.data
stimulus.feed('MAIA_SERIAL_RX_READY fill\r\n')
assert port.data == bytes(range(64)) and stimulus.phase == 1
stimulus.feed('MAIA_SERIAL_RX_READY fill\r\n')
assert len(port.data) == 64
stimulus.feed('MAIA_SERIAL_RX_READY drain\n')
assert port.data == bytes(range(256)) and stimulus.phase == 2
stimulus.feed('MAIA_SERIAL_RX_READY drain\n')
assert len(port.data) == 256
class ShortPort:
    def write(self, data):
        return len(data) - 1
try:
    runner.SerialInputStimulus(ShortPort(), lambda seconds: None).feed('MAIA_SERIAL_RX_READY fill\n')
except RuntimeError:
    pass
else:
    raise AssertionError('short writes must fail the stimulus')
print('PASS: binary fixture waits for complete markers, sends each phase once and rejects short writes')


tx_records = [runner.serial_output_record(source, seq) for seq in range(40) for source in (0, 1)]
tx = ''.join(tx_records) + 'TX_SUM records=80 bytes=3840 errors=0\nTX_HW irq=270 full=100 busy=50\nMAIA_NATIVE_PREEMPT_PASS\n'
assert runner.evaluate_serial_output(tx)
assert not runner.evaluate_serial_output(tx.replace(tx_records[0], '', 1))
assert not runner.evaluate_serial_output(tx + tx_records[0])
assert not runner.evaluate_serial_output(tx.replace(tx_records[0], tx_records[0][:-2]+'!\n', 1))
assert not runner.evaluate_serial_output(tx.replace(tx_records[0], tx_records[2], 1))
assert not runner.evaluate_serial_output(tx.replace('full=100', 'full=0'))
assert not runner.evaluate_serial_output(tx.replace('busy=50', 'busy=0'))
assert not runner.evaluate_serial_output(tx.replace('irq=270', 'irq=0'))
assert not runner.evaluate_serial_output(tx.replace('bytes=3840', 'bytes=1'))
assert not runner.evaluate_serial_output(tx + 'LEGACY_MUST_NOT_APPEAR')
assert not runner.evaluate_serial_output(tx.replace('\n', '\r\n'))
print('PASS: TX capture requires all 80 intact ordered raw records, real IRQs, saturation and no legacy leak')
