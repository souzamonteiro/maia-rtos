"""Real ROM-entry snapshot: observation validates, not a cold/readiness claim."""
import importlib.util
from pathlib import Path
r=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
c=(r/'docs/audits/native-psram-boot-armed-hardware.txt').read_text()
assert m.evaluate_psram_state(c)
assert c.count('BOOT_OBSERVATION_ARMED')==1
assert c.index('BOOT_OBSERVATION_ARMED')<c.index('RAM boot...')<c.index('MAIA_NATIVE_PSRAM_STATE_OBSERVED')<c.index('Hard resetting via RTS pin...')
assert 'address=501151d0 before=40200000 after=40200000' in c
assert 'address=501151d4 before=a0000000 after=a0000000' in c
for a,b in [('roots_valid=00000001','roots_valid=00000000'),('samples=00000036','samples=00000000'),
            ('before=40200000 after=40200000','before=40200000 after=40200100'),
            ('address=5008f200','address=5008f204')]:
    assert a in c
    assert not m.evaluate_psram_state(c.replace(a,b))
row=next(l for l in c.splitlines() if l.startswith('PSRAM_ROOT '))
assert not m.evaluate_psram_state(c+row+'\n')
assert not m.evaluate_psram_state(c.replace(row,''))
failed=(r/'docs/audits/native-psram-boot-no-sync.txt').read_text()
assert not m.evaluate_psram_state(failed)
# Matching powered roots are also a valid observation: never label either
# observation as cold startup or ready memory merely because validation passes.
assert m.evaluate_psram_state(c.replace('before=40200000 after=40200000','before=40200180 after=40200180'))
print('Real powered-off VO2 snapshot integrity/negative records/no-sync refusal; no cold or memory-ready inference PASS')
