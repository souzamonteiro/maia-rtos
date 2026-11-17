"""Idle-before-open, held-port ROM wait, USB re-enumeration and fail cleanup."""
import importlib.util
from pathlib import Path
import sys
import types
from unittest.mock import patch

r=Path(__file__).resolve().parents[2]
s=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py')
m=importlib.util.module_from_spec(s);s.loader.exec_module(m)
class SerialError(Exception):pass
class Fatal(Exception):pass
events=[];ports=[];attempts=[];ticks=0
class Port:
    def __init__(self):self.dtr=self.rts=True;self.opened=False
    def open(self):
        assert self.dtr is False and self.rts is False
        self.opened=True;events.append('open-idle')
    def close(self):self.opened=False;events.append('close')
def serial_for_url(name,**kw):
    assert name=='fixture' and kw=={'exclusive':True,'do_not_open':True}
    p=Port();ports.append(p);return p
class Loader:
    def __init__(self,p,baud):
        assert isinstance(p,Port) and p.opened and baud==115200
        self._port=p;events.append('construct-open-object')
    def _connect_attempt(self,reset,mode):
        assert mode=='no_reset' and self._port.opened
        events.append('sync-held-port')
        result=attempts.pop(0)
        if isinstance(result,OSError):raise result
        return result
    def connect(self,mode,n):
        assert mode=='no_reset' and n in (1,3)
        events.append(('check',n))
        if fail_check:raise Fatal('injected chip check failure')
def mono():
    global ticks
    ticks+=1;return ticks
serial=types.SimpleNamespace(serial_for_url=serial_for_url,SerialException=SerialError)
esp=types.SimpleNamespace(__version__='4.12.0',CHIP_DEFS={'esp32p4':Loader},FatalError=Fatal)
def setup(script):
    global ticks,fail_check
    events.clear();ports.clear();attempts[:]=script;ticks=0;fail_check=False
with patch.dict(sys.modules,{'serial':serial}),patch.object(m,'time',types.SimpleNamespace(monotonic=mono,sleep=lambda n:None)):
    setup([]);obj=m.connect_boot_state(esp,'fixture',0)
    assert events==['open-idle','construct-open-object',('check',3)] and obj._port.opened
    setup([Fatal('not ROM yet'),None]);obj=m.connect_boot_state(esp,'fixture',10)
    assert events==['open-idle','construct-open-object','sync-held-port','sync-held-port',('check',1)]
    assert len(ports)==1 and obj._port.opened
    setup([OSError('USB reset/re-enumeration'),None]);obj=m.connect_boot_state(esp,'fixture',10)
    assert len(ports)==2 and not ports[0].opened and ports[1].opened
    assert events==['open-idle','construct-open-object','sync-held-port','close','open-idle','construct-open-object','sync-held-port',('check',1)]
    setup([Fatal('no ROM')])
    try:m.connect_boot_state(esp,'fixture',0.5);assert False
    except Fatal:pass
    assert not ports[0].opened and events[-1]=='close' and not any(isinstance(e,tuple) for e in events)
    setup([None]);fail_check=True
    try:m.connect_boot_state(esp,'fixture',10);assert False
    except Fatal:pass
    assert not ports[0].opened and events[-1]=='close'
    setup([]);esp.__version__='5.0'
    try:m.connect_boot_state(esp,'fixture',10);assert False
    except RuntimeError:pass
    assert not ports and not events
print('Idle signals before open/held ROM wait/no reset fallback/re-enumeration/deadline/version/cleanup PASS')
