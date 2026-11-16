"""Evidence evaluator regression against actual Tab5 UART, not hardware simulation."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
c=(r/'docs/audits/native-lcd-stream-hardware.txt').read_text()
assert m.evaluate_lcd_stream(c)
assert not m.evaluate_lcd_stream(c,True)
for a,b in [
 ('irqs=00000063','irqs=00000062'),
 ('external=00000063','external=00000062'),
 ('beats=00000586','beats=00000000'),
 ('released=00000001','released=00000000'),
 ('map_after=00000000','map_after=00000012'),
 ('requested=00000001','requested=00000000'),
 ('active_stop=00000001','active_stop=00000000'),
 ('line=00000012 source=','line=00000010 source='),
 ('engine=00000005','engine=00000004'),
 ('irq=00000006','irq=00000003'),
 ('part=00000005 result=00000000','part=00000005 result=fffffffc'),
 ('raw=000383ff','raw=00038400'),
 ('hash=e3d71bef','hash=e3d71bee'),
 ('errors=00000000 stopped=','errors=00000001 stopped='),
]:
 assert a in c,a
 assert not m.evaluate_lcd_stream(c.replace(a,b)),a
for prefix in ('LCD_STREAM_INIT ','LCD_STREAM_PREP ','LCD_STREAM_ROUTE ','LCD_STREAM_IRQ '):
 row=next(l for l in c.splitlines() if l.startswith(prefix))
 assert not m.evaluate_lcd_stream(c+row+'\n'),prefix
 assert not m.evaluate_lcd_stream('\n'.join(l for l in c.splitlines() if l!=row)),prefix
print('Actual UART positive; negative IRQ/task/ownership/init/stop/corruption/duplicate checks PASS')
