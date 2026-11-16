"""Strict lifecycle acceptance against real Tab5 UART; corrupted captures reject."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
c=(r/'docs/audits/native-lcd-lifecycle-hardware.txt').read_text()
assert m.evaluate_lcd_stream(c,lifecycle=True)
assert not m.evaluate_lcd_stream(c)
assert not m.evaluate_lcd_stream(c,visual=True,lifecycle=True)
for a,b in [
 ('updates=00000002','updates=00000001'),
 ('presented=00000002','presented=00000001'),
 ('published=00000002','published=00000001'),
 ('hash=bcc4e003','hash=e3d71bef'),
 ('ticks=00000044 beats=00000003','ticks=00000044 beats=00000000'),
 ('released=00000001','released=00000000'),
 ('irqs=00000018','irqs=00000017'),
 ('external=00000048','external=00000047'),
]:
 assert a in c,a
 assert not m.evaluate_lcd_stream(c.replace(a,b),lifecycle=True),a
for prefix in ('LCD_CYCLE ','LCD_EDIT '):
 row=next(l for l in c.splitlines() if l.startswith(prefix))
 assert not m.evaluate_lcd_stream(c+row+'\n',lifecycle=True)
 assert not m.evaluate_lcd_stream('\n'.join(l for l in c.splitlines() if l!=row),lifecycle=True)
print('Actual three-session/six-publication UART and negative cycle/hash/presentation/scheduling/IRQ/ownership checks PASS')

v=(r/'docs/audits/native-lcd-lifecycle-visual-hardware.txt').read_text()
assert m.evaluate_lcd_stream(v,visual=True,lifecycle=True)
assert not m.evaluate_lcd_stream(v,lifecycle=True)
assert not m.evaluate_lcd_stream(v.replace('hash=bcc4e003','hash=e3d71bef'),visual=True,lifecycle=True)
print('Actual visual capture and wrong mode/hash negatives PASS')

f=(r/'docs/audits/native-lcd-lifecycle-final-hardware.txt').read_text()
assert m.evaluate_lcd_stream(f,lifecycle=True)
print('Final harness electronic capture PASS')
