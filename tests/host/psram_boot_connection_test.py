"""Exercise actual runner CLI before any serial access: no-reset is read-only."""
import contextlib
import importlib.util
import io
from pathlib import Path
import sys
import tempfile
import types
from unittest.mock import patch

r=Path(__file__).resolve().parents[2]
sp=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(sp);sp.loader.exec_module(m)
class BeforeSerial(Exception):pass
calls=[]
def connect(*args,**kwargs):
    calls.append(kwargs)
    raise BeforeSerial
esp=types.ModuleType('esptool');esp.get_default_connected_device=connect
def boot_connect(tool,port,wait):
    assert tool is esp
    calls.append({'before':'no_reset','wait':wait})
    raise BeforeSerial
m.connect_boot_state=boot_connect
bin_image=types.ModuleType('esptool.bin_image')
image=types.SimpleNamespace(segments=[types.SimpleNamespace(addr=0x4FF00000,data=bytes(1024))],entrypoint=0x4FF00100)
bin_image.LoadFirmwareImage=lambda *a:image
with tempfile.TemporaryDirectory() as d,patch.dict(sys.modules,{'esptool':esp,'esptool.bin_image':bin_image}):
    def run(flags):
        calls.clear()
        argv=['probe','--image',str(Path(d)/'not-opened.bin'),'--log',str(Path(d)/'log')]+flags
        with patch.object(sys,'argv',argv),contextlib.redirect_stderr(io.StringIO()),contextlib.redirect_stdout(io.StringIO()):
            try:return m.main()
            except BeforeSerial:return 'before-serial'
            except SystemExit as e:return e.code
    assert run(['--psram-state'])=='before-serial' and calls[0]['before']=='default_reset'
    assert run(['--psram-boot-state'])=='before-serial' and calls[0]['before']=='no_reset'
    assert run(['--psram-boot-state','--psram-state'])=='before-serial' and calls[0]['before']=='no_reset'
    assert run(['--psram-boot-state','--psram-boot-wait','120'])=='before-serial' and calls[0]['wait']==120
    assert run(['--psram-boot-wait','1'])==2 and not calls
    for bad in ['-1','121','nan','inf']:
        assert run(['--psram-boot-state','--psram-boot-wait',bad])==2 and not calls
    assert run(['--psram-boot-state','--validate-only'])==0 and not calls
    for flag in ['--psram-id','--psram-clock','--psram-array','--psram-window','--psram-dma',
                 '--psram-supply','--psram-profile','--psram-park','--mpll-initialize',
                 '--lcd-stream','--touch-lcd','--input-lifecycle','--ldo3-acquire']:
        assert run(['--psram-boot-state',flag])==2 and not calls,flag
    assert run(['--psram-boot-state','--psram-fast','200'])==2 and not calls
    assert run(['--psram-boot-state','--expect-fatal-cause','2'])==2 and not calls
    image.segments[0].addr=0x4B800000
    assert run(['--psram-boot-state'])==2 and not calls
print('CLI no-reset/read-only isolation and image validation before serial access PASS')
