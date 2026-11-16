"""Real consumer park/resume, exact restoration and subsequent calibration."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('r',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
c=(r/'docs/audits/native-psram-park-hardware.txt').read_text()
assert m.evaluate_psram_park(c)
rows=[l for l in c.splitlines() if l.startswith('PSRAM_PARK ')]
for a,b in [('stage=00000004','stage=00000003'),('writes=00000004','writes=00000003'),
            ('after0=0000000c','after0=0000400c'),('after1=08000000','after1=00000000'),
            ('after2=0ffcfffd','after2=1ffcfffd'),('restored=00000001','restored=00000000'),
            ('after0=0000c00c','after0=0000d00c')]:
    assert a in c,a
    assert not m.evaluate_psram_park(c.replace(a,b)),a
for row in rows:
    assert not m.evaluate_psram_park(c+row+'\n')
    assert not m.evaluate_psram_park(c.replace(row,''))
    assert not m.evaluate_psram_park(c.replace(row,row+' extra=00000000'))
assert not m.evaluate_psram_park(c.replace('saw_low=00000001','saw_low=00000000'))
assert not m.evaluate_psram_park(c.replace('released=00000001','released=00000000'))
assert not m.evaluate_psram_park(c.replace(rows[1]+'\n','')+rows[1]+'\n')
print('Actual consumer park/resume plus negative gates/preserved fields/restore/order/calibration capture PASS')
