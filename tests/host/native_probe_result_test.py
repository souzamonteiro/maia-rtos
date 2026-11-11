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
