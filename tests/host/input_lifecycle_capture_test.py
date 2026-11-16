"""Actual UART: lifecycle invalidation is accepted only in the strict profile."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
# Preserve original host FAIL; evaluate only UART captured before host reset.
c=(r/'docs/audits/native-input-lifecycle-hardware.txt').read_text().split('[HOST] Resetting to the preserved flash application',1)[0]
assert m.evaluate_lcd_stream(c,touch=True,input_lifecycle=True)
assert not m.evaluate_lcd_stream(c,touch=True)
assert not m.evaluate_lcd_stream(c,visual=True,touch=True,input_lifecycle=True)
for a,b in [('restarts=00000003','restarts=00000002'),('closed=00000001','closed=00000000'),('sequence=00000003','sequence=00000000'),('bus_refused=00000003','bus_refused=00000000'),('count=ffffffff x=00000000','count=ffffffff x=00000001'),('bus_errors=00000000','bus_errors=00000001')]:
 assert a in c,a
 assert not m.evaluate_lcd_stream(c.replace(a,b),touch=True,input_lifecycle=True),a
print('Actual lifecycle capture and negative restart/close/sequence/invalid-state/bus checks PASS')
