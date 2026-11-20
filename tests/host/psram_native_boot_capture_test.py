"""HOST model envelope only: no fabricated native-boot hardware capture."""
from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
warm=(r/'docs/audits/native-psram-profile-hardware.txt').read_text()
assert not m.evaluate_psram_native_boot(warm)
# Reuse an accepted downstream warm capture as a model of future output only.
# Never save this synthesized stream as a device log or hardware evidence.
base='\n'.join(l for l in warm.splitlines() if not l.startswith('PSRAM_PROFILE '))+'\n'
base=base.replace('before2=00000099','before2=00000008')
boot='PSRAM_BOOT result=00000000 stage=00000007 writes=00000008 supply_stage=00000004 supply_writes=00000003 adopted=00000000 dref=00000007 mul=00000005 beforectl=40200000 afterctl=40200180 beforeana=a0000000 afterana=76800000 clock_stage=00000006 attempts=0000003f beforeperi=0000c00c afterperi=0000c00c bus0=00010001 bus1=00010001 profile_stage=00000003 profile_attempts=00000007 profile_polls=0000000a before0=00008d28 after0=00008d30 before4=00000040 after4=00000020 before8=00000003 after8=0000004b'
model=base.replace('DPI_FB_CACHE ',boot+'\nDPI_FB_CACHE ',1)
assert m.evaluate_psram_native_boot(model)
assert not m.evaluate_mpll_initialize(model) # warm guard still demands old400M
assert not m.evaluate_psram_native_boot(model.replace(boot+'\nDPI_FB_CACHE ','DPI_FB_CACHE '+'\n'+boot,1))
for a,b in [('stage=00000007','stage=00000006'),('writes=00000008','writes=00000007'),
            ('adopted=00000000','adopted=00000001'),('beforectl=40200000','beforectl=40200100'),
            ('afterana=76800000','afterana=66800000'),('attempts=0000003f','attempts=0000001f'),
            ('bus0=00010001','bus0=00030103'),('after0=00008d30','after0=00008d28'),
            ('profile_attempts=00000007','profile_attempts=00000003')]:
    assert not m.evaluate_psram_native_boot(model.replace(boot,boot.replace(a,b))),a
assert not m.evaluate_psram_native_boot(model+boot+'\n')
assert not m.evaluate_psram_native_boot(model.replace(boot,''))
assert not m.evaluate_psram_native_boot(model.replace('released=00000001','released=00000000'))
print('HOST-only future native-boot envelope and negative boot/supply/modes/downstream checks PASS; target NOT RUN')
