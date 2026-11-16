"""Actual target profile programming/readback/restoration and full pipeline."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2];sp=importlib.util.spec_from_file_location('r',r/'tests/hardware/run_native_scheduler_probe.py');m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
c=(r/'docs/audits/native-psram-profile-hardware.txt').read_text()
assert m.evaluate_psram_profile(c)
for a,b in [('attempts=00000007','attempts=00000003'),('after0=00008d30','after0=00008d20'),('after4=00000020','after4=00000040'),('after8=0000004b','after8=00000043'),('restored=00000001 before0=','restored=00000000 before0='),('released=00000001','released=00000000')]:
 assert a in c,a
 assert not m.evaluate_psram_profile(c.replace(a,b)),a
row=next(l for l in c.splitlines() if l.startswith('PSRAM_PROFILE '))
assert not m.evaluate_psram_profile(c+row+'\n')
assert not m.evaluate_psram_profile('\n'.join(l for l in c.splitlines() if l!=row))
print('Actual PSRAM profile and negative mode/write/restoration/source-lifetime capture checks PASS')
