"""Actual target MPLL initialization and downstream memory/LCD qualification."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('r',r/'tests/hardware/run_native_scheduler_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
c=(r/'docs/audits/native-mpll-initialize-hardware.txt').read_text()
assert m.evaluate_mpll_initialize(c)
for a,b in [('saw_low=00000001','saw_low=00000000'),('writes=00000004 polls=','writes=00000003 polls='),('routing=00000001','routing=00000000'),('after2=00000099','after2=00000091'),('after3=00000070','after3=00000050'),('mhz=00000190','mhz=0000012c'),('cal_after=0000034c','cal_after=0000014c'),('released=00000001','released=00000000')]:
 assert a in c,a
 assert not m.evaluate_mpll_initialize(c.replace(a,b)),a
row=next(l for l in c.splitlines() if l.startswith('MPLL_BOOT '))
assert not m.evaluate_mpll_initialize(c+row+'\n')
print('Actual MPLL power/program/calibration plus memory/DMA/LCD capture and negative guards PASS')
