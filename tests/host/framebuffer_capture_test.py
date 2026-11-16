from pathlib import Path
import importlib.util
r=Path(__file__).resolve().parents[2];spec=importlib.util.spec_from_file_location('runner',r/'tests/hardware/run_native_scheduler_probe.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
base=(r/'docs/audits/native-dpi-bridge-hardware.txt').read_text()
for visual in (False,True):
 n=1000 if visual else 4
 rows='DPI_FB_READY\nDPI_FB_CACHE result=00000000 stage=00000003 size=00040000 line=00000040\nDPI_FB_RAW phase=00000000 result=00000000 bytes=00001000 restored=00000001\nDPI_FB_FAST result=00000000 rate=000000c8 mpll=00000190 div=00000099 dll0=00000021 dll1=00000021\n'
 rows+=f'DPI_FB_IMAGE source=4b800080 bytes=001c2000 hash=e3d71bef native_width=000002d0 native_height=00000500 rotation=0000005a frames={n:08x} visual={int(visual):08x}\n'
 for i in range(9):rows+=f'PSRAM_DMA_COPY index={i:08x} offset={(0,65408,0x1ff000)[i//3]:08x} bytes=00001000 burst={(1,4,16)[i%3]:08x} done=00000001 polls=00000001 matched=00001000 guards=00000001 raw=000001ff status=00000000 common=00000000 us=00000017\n'
 rows+='PSRAM_DMA_FINAL cfg=00000000 channels=00000000 fifo=00000000 restored=00000001 preserved=00000001\n'
 rows+=f'DPI_FB_TRANSFER result=00000000 stage=00000012 frames={n:08x} items={n*230400:08x} rearms={n-1:08x} polls=00001000 raw=000383ff done=00000001 recovered=00000001 released=00000001 status=00000012 common=00000000 bridge=00000000 error0=00000000 error1=00000000 us={16000000 if visual else 65000:08x}\n'
 rows+='DPI_FB_IDLE cfg=00000000 channels=00000000 dpi=000019d0 mode=00000001 hash=e3d71bef guards=00000001 duplicate=00000001\nDPI_FB_RAW phase=00000001 result=00000000 bytes=00001000 restored=00000001\nDPI_FB_RAW phase=00000002 result=00000000 bytes=00001000 restored=00000001\nDPI_FB_MEMORY_RESTORE bytes=00001000 roots=00000001 words=00000001 mode=0000004b axi=c0000000\n'
 for i in range(4):rows+=f'DPI_FB_REPLY index={i:08x} result=00000000 word=0000009c restored=00000005 error0=00000000 error1=00000000\n'
 rows+='MAIA_NATIVE_DPI_FRAMEBUFFER_PASS\n'
 capture=base+'\n'+rows
 assert m.evaluate_dpi_framebuffer(capture,visual)
 assert not m.evaluate_dpi_framebuffer(capture,not visual)
 for a,b in [('released=00000001','released=00000000'),('hash=e3d71bef','hash=e3d71bee'),('raw=000383ff','raw=00038400'),('rotation=0000005a','rotation=00000000'),('dll1=00000021','dll1=00000001'),('burst=00000010','burst=0000000f'),('words=00000001 mode=','words=00000000 mode=')]:assert not m.evaluate_dpi_framebuffer(capture.replace(a,b),visual),(a,b)
 assert not m.evaluate_dpi_framebuffer(capture+rows,visual)
print('Electronic/visual complete capture positives and negative ownership/data/rotation/error checks PASS')
class Port:
 def __init__(self):self.sent=[]
 def write(self,b):self.sent.append(b);return len(b)
 def flush(self):pass
for v in (False,True):
 p=Port();t=[0.0]
 def pause(x):t[0]+=x
 s=m.FramebufferStimulus(p,v,pause=pause,now=lambda:t[0]);s.feed('VO3_CYCLE_OFF_READY\nVO3_CYCLE_ON_READY\nDPI_FB_REA');assert p.sent==[b'O',b'N'];s.feed('VO3_CYCLE_OFF_READY\nVO3_CYCLE_ON_READY\nDPI_FB_READY\n');s.feed('DPI_FB_READY\n');assert p.sent==[b'O',b'N',b'V' if v else b'E'] and s.sent
print('Complete readiness markers/single E orV stimulus PASS')
