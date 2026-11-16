"""Actual native UART acceptance, not a simulated hardware run."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
c=(r/'docs/audits/native-touch-lcd-electronic-hardware.txt').read_text()
assert m.evaluate_lcd_stream(c,touch=True)
assert not m.evaluate_lcd_stream(c,visual=True,touch=True)
assert not m.evaluate_lcd_stream(c)
for a,b in [('slots=0000000a','slots=0000000b'),('bus_errors=00000000','bus_errors=00000001'),('contentions=00000000','contentions=00000001'),('presented=00000001','presented=00000000'),('exited=00000004','exited=00000003'),('external=00000065','external=00000064')]:
 assert a in c,a
 assert not m.evaluate_lcd_stream(c.replace(a,b),touch=True),a
for prefix in ('TOUCH_INFO ','TOUCH_INPUT ','TOUCH_INIT '):
 row=next(l for l in c.splitlines() if l.startswith(prefix))
 assert not m.evaluate_lcd_stream(c+row+'\n',touch=True)
 assert not m.evaluate_lcd_stream('\n'.join(l for l in c.splitlines() if l!=row),touch=True)
print('Actual electronic touch/LCD capture and negative capacity/bus/presentation/IRQ/init checks PASS')

f=(r/'docs/audits/native-touch-lcd-final-electronic-hardware.txt').read_text()
assert m.evaluate_lcd_stream(f,touch=True)
print('Final status-first electronic capture PASS')
