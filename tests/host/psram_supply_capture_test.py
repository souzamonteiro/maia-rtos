"""Strict real VO2 adoption capture and downstream pipeline regressions."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('r',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
c=(r/'docs/audits/native-psram-supply-hardware.txt').read_text()
assert m.evaluate_psram_supply(c)
row=next(l for l in c.splitlines() if l.startswith('PSRAM_SUPPLY '))
for a,b in [('result=00000000','result=ffffffff'),('stage=00000004','stage=00000003'),
            ('adopted=00000001','adopted=00000000'),('writes=00000000','writes=00000001'),
            ('dref=00000007','dref=00000000'),('mul=00000005','mul=00000004'),
            ('after0=40200180','after0=40204180'),('after1=76800000','after1=72800000')]:
    assert a in row
    assert not m.evaluate_psram_supply(c.replace(row,row.replace(a,b)))
assert not m.evaluate_psram_supply(c+row+'\n')
assert not m.evaluate_psram_supply(c.replace(row,''))
assert not m.evaluate_psram_supply(c.replace(row,row+' extra=00000000'))
assert not m.evaluate_psram_supply(c.replace('released=00000001','released=00000000'))
assert not m.evaluate_psram_supply(c.replace('after4=00000020','after4=00000040'))
print('Actual VO2 adoption plus negative trim/owner/write/pipeline capture checks PASS')
