import importlib.util
from pathlib import Path
import sys
sys.dont_write_bytecode = True
spec = importlib.util.spec_from_file_location('runner', Path(__file__).resolve().parents[1] / 'hardware/run_native_scheduler_probe.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)
evaluate = runner.evaluate_capture
good = 'FAULT_ARMED pc=4ff00836\nMAIA_NATIVE_FATAL cause=30000002 pc=4ff00836 value=00000000 sp=4ff02000\n'
assert evaluate(good, 2)
assert not evaluate(good, 3)
assert not evaluate(good.replace('pc=4ff00836 value', 'pc=4ff00838 value'), 2)
assert not evaluate(good + 'FAULT_RESUMED', 2)
assert not evaluate(good + 'MAIA_NATIVE_PREEMPT_PASS', 2)
assert not evaluate(good + good, 2)
assert not evaluate(good.split('\n', 1)[1], 2)
assert not evaluate('FAULT_RESUMED', 2)
assert evaluate('MAIA_NATIVE_PREEMPT_PASS')
assert not evaluate('MAIA_NATIVE_PREEMPT_PASS MAIA_NATIVE_PREEMPT_FAIL')
assert not evaluate('MAIA_NATIVE_PREEMPT_PASS ' + good)
print('PASS: fatal evidence requires exact expected PC/cause and rejects resumed/missing/duplicate output')

# Expected PC from ELF remains authoritative if early serial output is lost.
assert evaluate(good.split('\n', 1)[1], 2, 0x4ff00836)
assert not evaluate(good, 2, 0x4ff00838)
assert not evaluate(good.replace('pc=4ff00836', 'pc=4ff00838', 1), 2, 0x4ff00836)

assert not evaluate(good + 'MAIA_NATIVE_PREEMPT_FAIL', 2)
assert not evaluate(good.replace('cause=30000002', 'cause=b0000002'), 2)

stack = 'MAIA_STACK_FAULT reason=1 emergency=1 guard=1\n' + good.replace('cause=30000002', 'cause=3800000b')
assert evaluate(stack, 11, 0x4ff00836, 1, 1, 0x4ff02000)
assert not evaluate(stack, 11, 0x4ff00836)
assert not evaluate(stack, 11, 0x4ff00836, 2, 1)
assert not evaluate(stack.replace('emergency=1', 'emergency=0'), 11, 0x4ff00836, 1, 1)
assert not evaluate(stack, 11, 0x4ff00836, 1, 0)
assert not evaluate(stack, 11, 0x4ff00836, 1, 1, 0)
assert not evaluate(stack + stack, 11, 0x4ff00836, 1)
assert not evaluate('MAIA_NATIVE_PREEMPT_PASS MAIA_STACK_FAULT')
assert not evaluate(good.replace(' sp=4ff02000',''), 2)

def sdmmc_capture(sys_clock=0x4000, reset=0):
    valid = int(bool(sys_clock & 0x4000) and not bool(reset & 0x10000000))
    capture = 'MAIA_SDMMC_STATE_PROBE schema=1 read-only\n'
    for i in range(8):
        capture += (f'SDMMC_SAMPLE i={i:08x} sys={sys_clock:08x} clock=01000000 div=00000001 '
                    f'reset={reset:08x} ldo=00004180 analog=00000000 sys_after={sys_clock:08x} '
                    f'clock_after=01000000 reset_after={reset:08x} ldo_after=00004180 '
                    f'analog_after=00000000 valid={valid:08x} stable=00000001\n')
        capture += f'SDMMC_PINS i={i:08x} d0=00000000 d1=00000000 d2=00000000 d3=00000000 clk=00000000 cmd=00000000\n'
        capture += (f'SDMMC_HOST i={i:08x} ctrl={valid:08x} clkdiv=00000000 clksrc=00000000 '
                    'clkena=00000000 tmout=00000000 ctype=00000000 cmd=00000000 rint=00000000 '
                    'status=00000000 cdetect=00000000 fifoth=00000000 bmod=00000000 '
                    'verid=00000000 hcon=00000000\n')
    return capture + 'MAIA_NATIVE_SDMMC_STATE_PASS\n'


sd_observed = sdmmc_capture()
sd_skipped = sdmmc_capture(sys_clock=0)
sd_reset = sdmmc_capture(reset=0x10000000)
assert runner.evaluate_sdmmc_state(sd_observed)
assert runner.evaluate_sdmmc_state(sd_skipped)
assert runner.evaluate_sdmmc_state(sd_reset)
for old, new in [('stable=00000001', 'stable=00000000'),
                 ('sys_after=00004000', 'sys_after=00000000'),
                 ('valid=00000001', 'valid=00000000'),
                 ('i=00000007', 'i=00000008'),
                 ('MAIA_NATIVE_SDMMC_STATE_PASS', 'MAIA_NATIVE_SDMMC_STATE_FAIL'),
                 ('SDMMC_HOST i=00000000', 'SDMMC_OTHER i=00000000')]:
    assert not runner.evaluate_sdmmc_state(sd_observed.replace(old, new, 1))
assert not runner.evaluate_sdmmc_state(sd_observed + sd_observed)
assert not runner.evaluate_sdmmc_state(sd_observed + 'MAIA_NATIVE_FATAL\n')
assert not runner.evaluate_sdmmc_state(sd_skipped.replace('ctrl=00000000', 'ctrl=00000001', 1))
assert not runner.evaluate_sdmmc_state(sd_reset.replace('valid=00000000', 'valid=00000001', 1))
print('PASS: SDMMC observations require eight ordered stable samples and guarded host access')


class FakePort:
    def __init__(self):
        self.data = bytearray()
    def write(self, data):
        self.data.extend(data)
        return len(data)

port = FakePort()
stimulus = runner.SerialInputStimulus(port, lambda seconds: None)
stimulus.feed('unrelated output')
assert not port.data
stimulus.feed('MAIA_SERIAL_RX_READY fi')
assert not port.data
stimulus.feed('MAIA_SERIAL_RX_READY fill\r\n')
assert port.data == bytes(range(64)) and stimulus.phase == 1
stimulus.feed('MAIA_SERIAL_RX_READY fill\r\n')
assert len(port.data) == 64
stimulus.feed('MAIA_SERIAL_RX_READY drain\n')
assert port.data == bytes(range(256)) and stimulus.phase == 2
stimulus.feed('MAIA_SERIAL_RX_READY drain\n')
assert len(port.data) == 256
class ShortPort:
    def write(self, data):
        return len(data) - 1
try:
    runner.SerialInputStimulus(ShortPort(), lambda seconds: None).feed('MAIA_SERIAL_RX_READY fill\n')
except RuntimeError:
    pass
else:
    raise AssertionError('short writes must fail the stimulus')
print('PASS: binary fixture waits for complete markers, sends each phase once and rejects short writes')


tx_records = [runner.serial_output_record(source, seq) for seq in range(40) for source in (0, 1)]
tx = ''.join(tx_records) + 'TX_SUM records=80 bytes=3840 errors=0\nTX_HW irq=270 full=100 busy=50\nMAIA_NATIVE_PREEMPT_PASS\n'
assert runner.evaluate_serial_output(tx)
assert not runner.evaluate_serial_output(tx.replace(tx_records[0], '', 1))
assert not runner.evaluate_serial_output(tx + tx_records[0])
assert not runner.evaluate_serial_output(tx.replace(tx_records[0], tx_records[0][:-2]+'!\n', 1))
assert not runner.evaluate_serial_output(tx.replace(tx_records[0], tx_records[2], 1))
assert not runner.evaluate_serial_output(tx.replace('full=100', 'full=0'))
assert not runner.evaluate_serial_output(tx.replace('busy=50', 'busy=0'))
assert not runner.evaluate_serial_output(tx.replace('irq=270', 'irq=0'))
assert not runner.evaluate_serial_output(tx.replace('bytes=3840', 'bytes=1'))
assert not runner.evaluate_serial_output(tx + 'LEGACY_MUST_NOT_APPEAR')
assert not runner.evaluate_serial_output(tx.replace('\n', '\r\n'))
print('PASS: TX capture requires all 80 intact ordered raw records, real IRQs, saturation and no legacy leak')


event_tx = tx + 'TX_EVENT_TRIGGER\nTX_EVENT head=20 tail=50 idle=69\nTX_WAIT empty=4 retry=200 timeout=150 irq_write=1\n'
assert runner.evaluate_serial_output_event(event_tx)
assert not runner.evaluate_serial_output_event(tx)
assert not runner.evaluate_serial_output_event(event_tx.replace('head=20', 'head=0'))
assert not runner.evaluate_serial_output_event(event_tx.replace('tail=50', 'tail=0'))
assert not runner.evaluate_serial_output_event(event_tx.replace('idle=69', 'idle=0'))
assert not runner.evaluate_serial_output_event(event_tx.replace('timeout=150', 'timeout=0'))
assert not runner.evaluate_serial_output_event(event_tx.replace('irq_write=1', 'irq_write=0'))
assert not runner.evaluate_serial_output_event(event_tx + 'TX_EVENT_TRIGGER\n')
print('PASS: TX event capture requires quiet head/tail, finite retry and exactly one IRQ-triggered record')

identity = ('TOUCH_STATUS result_code=0 raw=4\nTOUCH_ID reads=16 firmware=1 final=1 errors=0\n'
            'TOUCH_ABSENT address=0x56 NACK\n'
            'TOUCH_INJECT sampled_SCL_low TIMEOUT\nMAIA_NATIVE_TOUCH_PASS\n')
assert runner.evaluate_touch_identity(identity)
assert runner.evaluate_touch_identity(identity.replace('firmware=1 final=1', 'firmware=3 final=3'))
for old, new in [('raw=4', 'raw=1'), ('result_code=0', 'result_code=2'), ('reads=16', 'reads=1'), ('final=1', 'final=3'), ('errors=0', 'errors=1'),
                 ('firmware=1', 'firmware=0'), ('TOUCH_ABSENT address=0x56 NACK\n', ''),
                 ('TOUCH_INJECT sampled_SCL_low TIMEOUT\n', ''), ('MAIA_NATIVE_TOUCH_PASS', '')]:
    assert not runner.evaluate_touch_identity(identity.replace(old, new))
assert not runner.evaluate_touch_identity(identity + identity)
assert not runner.evaluate_touch_identity(identity + 'MAIA_NATIVE_TOUCH_FAIL')
assert not runner.evaluate_touch_identity(identity + 'MAIA_NATIVE_FATAL')
assert not runner.evaluate_touch_identity('MAIA_NATIVE_PREEMPT_PASS')
print('PASS: touch capture requires stable known firmware, all reads, negative probes and no failure')

expander = (identity + 'EXPANDER bank=67 rounds=16 mirror=1 unchanged=1 dir=111 out=102\n'
            'EXPANDER bank=68 rounds=16 mirror=1 unchanged=1 dir=255 out=0\n'
            'EXPANDER_ABSENT_WRITE NACK\n')
assert runner.evaluate_expander(expander)
for old,new in [('bank=68','bank=67'),('rounds=16','rounds=0'),('mirror=1','mirror=0'),
                ('unchanged=1','unchanged=0'),('dir=255','dir=256'),('EXPANDER_ABSENT_WRITE NACK\n','')]:
    assert not runner.evaluate_expander(expander.replace(old,new))
assert not runner.evaluate_expander(identity)
assert not runner.evaluate_expander(expander + 'MAIA_NATIVE_TOUCH_FAIL')
print('PASS: expander evidence requires both banks, unchanged snapshots, writes and touch regression')

panel = expander + 'PANEL_RESET stage=9 changed=1 preserved=1 elapsed_us=250000 gpio23_input=1\n'
assert runner.evaluate_panel_reset(panel)
for old,new in [('stage=9','stage=8'),('changed=1','changed=0'),('preserved=1','preserved=0'),
                ('elapsed_us=250000','elapsed_us=199999'),('gpio23_input=1','gpio23_input=0')]:
    assert not runner.evaluate_panel_reset(panel.replace(old,new))
assert not runner.evaluate_panel_reset(expander)
assert not runner.evaluate_panel_reset(panel + 'PANEL_RESET stage=9 changed=1 preserved=1 elapsed_us=250000 gpio23_input=1\n')
assert not runner.evaluate_panel_reset(panel + 'MAIA_NATIVE_TOUCH_FAIL')
print('PASS: panel reset evidence requires verified stage, minimum delay, bit preservation and touch recovery')

ldo = ('LDO3_VERSION chip_major=1 chip_minor=3 block_major=0 block_minor=1 disabled=0 rs_fail=0 corrected=0\n'
       'LDO3_TRIM k=0 vos=0 c=0\n'
       'LDO3_PLAN target_mv=2500 dref=9 mul=6 default_mask=7 estimated_uv=2500000 error_uv=0\n'
       'LDO3_COEFF k1000=1000 vos1000=0 c1000=1000\n'
       'LDO3_CURRENT enabled=0 sw_owner=0 bypass=0 source=0 dref=10 mul=0 vdet=0\n'
       'LDO3_READONLY samples=16 unchanged=1 errors=0\nMAIA_NATIVE_LDO3_PASS\n')
assert runner.evaluate_ldo3(ldo)
for old,new in [('chip_minor=3','chip_minor=2'),('block_major=0','block_major=1'),('rs_fail=0','rs_fail=1'),
                ('dref=9 mul=6','dref=13 mul=3'),('default_mask=7','default_mask=0'),
                ('estimated_uv=2500000','estimated_uv=2500001'),('k1000=1000','k1000=999'),
                ('samples=16','samples=1'),('unchanged=1','unchanged=0'),('k=0 vos=0','k=256 vos=0')]:
    assert not runner.evaluate_ldo3(ldo.replace(old,new))
assert not runner.evaluate_ldo3(ldo+ldo)
assert not runner.evaluate_ldo3(ldo+'MAIA_NATIVE_LDO3_FAIL')
assert not runner.evaluate_ldo3('MAIA_NATIVE_LDO3_PASS')
print('PASS: LDO3 evidence validates raw trims, model/grid selection, stable registers and known silicon/schema')

powered_ldo = ldo.replace('enabled=0 sw_owner=0 bypass=0 source=0 dref=10 mul=0 vdet=0',
                          'enabled=1 sw_owner=1 bypass=0 source=0 dref=9 mul=6 vdet=1')
acquire = powered_ldo + 'LDO3_CLOCKS sys_active=1 phy_active=1\nLDO3_ACQUIRE stage=5 adopted=1 changed=0 preserved=1 duplicate_busy=1\n'
cycle = (powered_ldo + 'LDO3_CLOCKS sys_active=0 phy_active=0\nLDO3_CYCLE off_confirmed=1 clocks_gated=1\n'
         'LDO3_ACQUIRE stage=5 adopted=0 changed=1 preserved=1 duplicate_busy=1\n')
assert runner.evaluate_ldo3_acquire(acquire)
assert runner.evaluate_ldo3_acquire(cycle, True)
assert not runner.evaluate_ldo3_acquire(acquire, True)
assert not runner.evaluate_ldo3_acquire(cycle)
for old,new in [('stage=5','stage=4'),('preserved=1','preserved=0'),('duplicate_busy=1','duplicate_busy=0'),
                ('adopted=1','adopted=0'),('changed=0','changed=1')]:
    assert not runner.evaluate_ldo3_acquire(acquire.replace(old,new))
assert not runner.evaluate_ldo3_acquire(cycle.replace('off_confirmed=1','off_confirmed=0'), True)
assert not runner.evaluate_ldo3_acquire(acquire + acquire)
print('PASS: LDO3 acquisition evidence requires exact adoption or guarded disable/enable, preservation and duplicate rejection')

assert not runner.evaluate_ldo3_acquire(cycle.replace('sys_active=0', 'sys_active=1'), True)
assert not runner.evaluate_ldo3_acquire(cycle.replace('phy_active=0', 'phy_active=1'), True)

for old,new in [('enabled=1','enabled=0'),('sw_owner=1','sw_owner=0'),('bypass=0','bypass=1'),
                ('source=0','source=1'),('dref=9 mul=6 vdet=1','dref=10 mul=6 vdet=1'),('vdet=1','vdet=0')]:
    assert not runner.evaluate_ldo3_acquire(acquire.replace(old,new))

# Complete read-only MIPI observations; PASS is not an idle certificate.
def mipi_fixture(cpu=0, system=0, valid=0):
    text = 'MAIA_MIPI_STATE_PROBE schema=1 read-only\n'
    for i in range(8):
        text += f'MIPI_SAMPLE i={i:08x} cpu={cpu:08x} sys={system:08x} phy=000002b3 cpu_after={cpu:08x} sys_after={system:08x} phy_after=000002b3 valid={valid:08x} stable=00000001\n'
        text += f'MIPI_DSI i={i:08x} bridge=00000000 dpi=00000000 fifo=00000000 power=00000000 mode=00000000 reset=00000000 status=00000000\n'
        text += f'MIPI_CSI i={i:08x} host=00000000 shutdown=00000000 reset=00000000 stop=00000000 bridge=00000000\n'
        text += f'MIPI_DMA i={i:08x} cfg=00000000 enable=00000000 abort=00000000\n'
        for c in range(4):
            text += f'MIPI_CH i={i:08x} ch={c:08x} cfg0=00000000 cfg1=00000000 transferred=00000000 fifo=00000000 sar=00000000 dar=00000000 llp=00000000\n'
    return text + 'MAIA_NATIVE_MIPI_PASS\n'
for sample in (mipi_fixture(), mipi_fixture(0x2000, 0x1c20, 15)):
    assert runner.evaluate_mipi(sample)
    assert not runner.evaluate_mipi(sample + sample)
    assert not runner.evaluate_mipi(sample.replace('MAIA_NATIVE_MIPI_PASS', 'MAIA_NATIVE_MIPI_FAIL'))
    assert not runner.evaluate_mipi(sample.replace('MIPI_CH i=00000000 ch=00000000', 'MIPI_CH i=00000000 ch=00000001'))
    assert not runner.evaluate_mipi(sample.replace('stable=00000001', 'stable=00000000', 1))
    assert not runner.evaluate_mipi(sample.replace('cpu_after=', 'missing=', 1))
    assert not runner.evaluate_mipi(sample.replace('MIPI_DMA i=00000000', 'MIPI_UNKNOWN i=00000000', 1))
assert not runner.evaluate_mipi(mipi_fixture(0x2000, 0x1c20, 0))
assert not runner.evaluate_mipi(mipi_fixture().replace('sar=00000000', 'sar=00000001', 1))
assert not runner.evaluate_mipi(mipi_fixture().replace('cpu_after=00000000', 'cpu_after=00000001', 1))
assert not runner.evaluate_mipi(mipi_fixture().replace('MIPI_DSI i=00000007', 'MIPI_DSI i=00000008', 1))
print('PASS: complete MIPI records, clock validity, skipped domains, duplicate/missing/malformed/unstable negatives')

# Hardware scope is explicit: empty controller only, no active transfer claim.
for armed in (False, True):
    row = ('VDMA_STOP armed=' + ('00000001' if armed else '00000000') +
           ' stage=00000003 adopted=' + ('00000000' if armed else '00000001') +
           ' changed=' + ('00000001' if armed else '00000000') +
           ' requested=00000000 polls=00000001 cfg=00000000 channels=00000000 abort=00000000'
           ' fifo=00000000 preserved=00000001 duplicate_busy=00000001\n')
    capture = 'MAIA_VDMA_STOP_PROBE optional empty-controller writes; no transfers\n' + mipi_fixture(0x2000, 0x1c20, 15) + row
    assert runner.evaluate_vdma_stop(capture, armed)
    assert not runner.evaluate_mipi(capture)  # read-only mode must reject stop profile
    assert not runner.evaluate_vdma_stop(capture, not armed)
    assert not runner.evaluate_vdma_stop(capture + row, armed)
    for key in ('stage=00000003', 'preserved=00000001', 'duplicate_busy=00000001'):
        assert not runner.evaluate_vdma_stop(capture.replace(key, key[:-1]+'0'), armed)
    assert not runner.evaluate_vdma_stop(capture.replace('cfg=00000000 enable', 'cfg=00000001 enable', 1), armed)
    assert not runner.evaluate_vdma_stop(capture.replace('sar=00000000', 'sar=00000001', 1), armed)
    assert not runner.evaluate_vdma_stop(capture.replace('requested=00000000', 'requested=00000001'), armed)
    assert not runner.evaluate_vdma_stop(capture.replace('cfg=00000000 channels', 'cfg=00000001 channels'), armed)
print('PASS: VDMA empty-controller scope, adoption/armed distinction and invalid/failure captures')

transfer = """MAIA_VDMA_TRANSFER_PROBE schema=1 owned-sram
DMA_MEMORY start=4ff03280 end=4ff13480 alias=8ff03280 bytes=00008000 cache_line=00000040 prepared=00000001
DMA_COPY matched=00008000 status=00000003 common=00000000
DMA_STOP result=00000000 active=00000001 initial_cfg=00000001 initial_channels=00000001 requested=00000001 stage=00000003 polls=00000002 cfg=00000000 channels=00000000 abort=00000000 fifo=00000000
DMA_VERIFY copied=00000400 bad=00000000 guards=00000001 stable=00000001 preserved=00000001 duplicate_busy=00000001 status=4000001b common=00000000
MAIA_NATIVE_VDMA_TRANSFER_PASS
"""
assert runner.evaluate_vdma_transfer(transfer)
assert not runner.evaluate_vdma_stop(transfer)
for old, new in [('copied=00000400','copied=00008000'), ('copied=00000400','copied=00000000'),
                 ('initial_channels=00000001','initial_channels=00000000'), ('requested=00000001','requested=00000000'),
                 ('active=00000001','active=00000000'), ('matched=00008000','matched=00007fff'),
                 ('alias=8ff03280','alias=4ff03280'), ('end=4ff13480','end=4ff33480'),
                 ('prepared=00000001','prepared=00000000'), ('guards=00000001','guards=00000000'),
                 ('stable=00000001','stable=00000000'), ('polls=00000002','polls=00001001'),
                 ('status=4000001b','status=4000003b'), ('common=00000000','common=00000001')]:
    assert not runner.evaluate_vdma_transfer(transfer.replace(old,new,1))
assert not runner.evaluate_vdma_transfer(transfer+transfer)
assert not runner.evaluate_vdma_transfer(transfer.replace('DMA_COPY matched','DMA_UNKNOWN matched'))
print('PASS: DMA transfer evidence requires bounded memory, copy, active stop and partial stable result')

park = """MIPI_PARK_SETUP sys_before=0c0d5678 sys_ready=0c0d5e78 dsi_phy=00000002 csi_phy=00000001 armed=0000000f live=00000000
MIPI_PARK result=00000000 stage=00000004 attempted=0000000f dsi_bridge=00000000 csi_bridge=00000000 dsi_phy=00000000 csi_phy=00000000 preserved=00000001 duplicate_busy=00000001
MAIA_NATIVE_MIPI_PARK_PASS
"""
assert runner.evaluate_mipi_park(park)
assert not runner.evaluate_mipi(park)
for old,new in [('live=00000000','live=00000001'), ('attempted=0000000f','attempted=00000000'),
                ('dsi_phy=00000002','dsi_phy=00000003'), ('stage=00000004','stage=00000003'),
                ('preserved=00000001','preserved=00000000'), ('sys_ready=0c0d5e78','sys_ready=0c0d5678'),
                ('dsi_bridge=00000000','dsi_bridge=00000001')]:
    assert not runner.evaluate_mipi_park(park.replace(old,new,1))
assert not runner.evaluate_mipi_park(park+park)
assert not runner.evaluate_mipi_park(park.replace('MIPI_PARK_SETUP','MISSING'))
print('PASS: MIPI parked reset scope, host clock preparation, all modified fields and capture negatives')

assert not runner.evaluate_mipi_park(park + 'MIPI_PARK malformed\n')
assert not runner.evaluate_mipi_park(park + 'MIPI_PARK_SETUP malformed\n')
assert not runner.evaluate_mipi_park(park.replace('dsi_phy=00000002','dsi_phy=80000002'))

gate = park + """MIPI_ISP clock=007fffff reset=80000001 norst=80000001
MIPI_ISP_STATE inspected=00000000 control=00000000 camera=00000000
MIPI_GATE result=00000000 stage=00000005 attempted=00000007 reset_before=80000001 reset_after=bc000001 phy_before=000002b3 phy_after=00000220 sys_before=0c0d5e78 sys_after=0c0d4278 preserved=00000001 duplicate_busy=00000001
MAIA_NATIVE_MIPI_GATE_PASS
"""
assert runner.evaluate_mipi_gate(gate)
assert not runner.evaluate_mipi_park(gate)
for old,new in [('clock=007fffff','clock=027fffff'),('reset_after=bc000001','reset_after=9c000001'),
                ('norst=80000001','norst=80004001'),('phy_after=00000220','phy_after=00000221'),
                ('sys_after=0c0d4278','sys_after=0c0d4258'),('attempted=00000007','attempted=00000006'),
                ('stage=00000005','stage=00000004')]:
    assert not runner.evaluate_mipi_gate(gate.replace(old,new,1))
assert not runner.evaluate_mipi_gate(gate+gate)
assert not runner.evaluate_mipi_gate(gate+'MIPI_GATE malformed\n')
print('PASS: MIPI gate capture requires ISP off, all held resets, exact preserved masks and exclusive mode')

clocked_gate = gate.replace('clock=007fffff','clock=02e82000').replace('inspected=00000000 control=00000000 camera=00000000', 'inspected=00000001 control=40002442 camera=00000004')
assert runner.evaluate_mipi_gate(clocked_gate)
for old,new in [('control=40002442','control=40002443'), ('control=40002442','control=48002442'),
                ('camera=00000004','camera=00000005'), ('camera=00000004','camera=00000000'),
                ('inspected=00000001','inspected=00000000')]:
    assert not runner.evaluate_mipi_gate(clocked_gate.replace(old,new))

cycle = 'VO3_CSI_PREP sys_before=0c0d5278 sys_after=0c0d5e78 phy_before=000002a3 phy_after=000002b3 reset_before=08000040 reset_after=00000040\n' + clocked_gate.replace('sys_before=0c0d5678','sys_before=0c0d5e78') + """VO3_PLAN dref=0000000d mul=00000003 defaults=00000000 control=00000180 analog=d5800000
VO3_OFF control=00000080 analog=d5800000
VO3_CYCLE_OFF_READY
VO3_ON control=00000180 analog=d5800000
VO3_CYCLE_ON_READY
VO3_ACQUIRE stage=00000005 adopted=00000000 changed=00000001 preserved=00000001 duplicate_busy=00000001 samples=00000010
MAIA_NATIVE_VO3_CYCLE_PASS
"""
assert runner.evaluate_vo3_cycle(cycle)
assert not runner.evaluate_mipi_gate(cycle)
for old,new in [('VO3_OFF control=00000080','VO3_OFF control=00000180'),('VO3_ON control=00000180','VO3_ON control=00000080'),
                ('analog=d5800000','analog=d5000000'),('dref=0000000d','dref=0000000c'),
                ('adopted=00000000','adopted=00000001'),('samples=00000010','samples=0000000f')]:
    assert not runner.evaluate_vo3_cycle(cycle.replace(old,new,1))
assert not runner.evaluate_vo3_cycle(cycle+cycle)
assert not runner.evaluate_vo3_cycle(cycle.replace('VO3_CYCLE_OFF_READY','MISSING'))
assert not runner.evaluate_vo3_cycle(cycle.replace('VO3_CYCLE_OFF_READY','VO3_CYCLE_ON_READY',1))
class CyclePort:
    def __init__(self): self.data=b''
    def write(self,data): self.data+=data;return len(data)
    def flush(self): pass
class CycleClock:
    def __init__(self): self.value=0.0
    def now(self): return self.value
    def pause(self,duration): self.value+=duration
port=CyclePort();clock=CycleClock();stimulus=runner.Vo3CycleStimulus(port,clock.pause,clock.now)
stimulus.feed('VO3_CYCLE_OFF_READY\n');assert port.data == b'O' and stimulus.phase == 1
stimulus.feed(cycle);assert port.data == b'ON' and stimulus.phase == 2 and stimulus.holds == [.02,.02]
stimulus.feed(cycle);assert port.data == b'ON'
try:
    runner.Vo3CycleStimulus(CyclePort(),lambda _:None,lambda:0).feed(cycle)
except RuntimeError: pass
else: raise AssertionError('short host observation window accepted')
print('PASS: VO3 off/on preservation, non-adoption, complete captures and phased host observation ACKs')

assert not runner.evaluate_vo3_cycle(cycle.replace('VO3_CSI_PREP','UNKNOWN'))
assert not runner.evaluate_vo3_cycle(cycle.replace('phy_after=000002b3','phy_after=000002a3',1))
assert not runner.evaluate_vo3_cycle(cycle.replace('sys_after=0c0d5e78','sys_after=0c0d5e58',1))

dsi_access = cycle + """DSI_ACCESS result=00000000 stage=00000005 attempted=00000007 sys_before=0c0d4278 sys_after=0c0d5278 phy_before=00000220 phy_after=00000221 reset_before=bc000001 reset_after=b8000001 control=00000180 analog=d5800000 duplicate_busy=00000001
DSI_CONFIG version=3134312a power=00000000 phy_reset=00000000 bridge=00000000 dpi=00000000 fifo=00000000 status=00000000
MAIA_NATIVE_DSI_ACCESS_PASS
"""
assert runner.evaluate_dsi_access(dsi_access)
assert not runner.evaluate_vo3_cycle(dsi_access)
for old,new in [('sys_after=0c0d5278','sys_after=0c0d5e78'),('phy_after=00000221','phy_after=00000223'),
                ('reset_after=b8000001','reset_after=98000001'),('power=00000000','power=00000001'),
                ('phy_reset=00000000','phy_reset=00000008'),('bridge=00000000 dpi=','bridge=00000001 dpi='),
                ('dpi=00000000','dpi=00000001'),('fifo=00000000 status=','fifo=00000001 status='),
                ('control=00000180 analog=d5800000 duplicate_busy=','control=00000080 analog=d5800000 duplicate_busy=')]:
    assert not runner.evaluate_dsi_access(dsi_access.replace(old,new))
assert not runner.evaluate_dsi_access(dsi_access+dsi_access)
assert not runner.evaluate_dsi_access(dsi_access+'DSI_ACCESS malformed\n')
assert not runner.evaluate_dsi_access(dsi_access.replace('MAIA_NATIVE_DSI_ACCESS_PASS',''))
assert not runner.evaluate_dsi_access('MAIA_NATIVE_DSI_ACCESS_PASS\n'+dsi_access.replace('MAIA_NATIVE_DSI_ACCESS_PASS\n',''))
print('PASS: DSI config capture requires preserved VO3, exact clocks/reset release and transmit controls off')

reference = dsi_access + """DSI_REFERENCE result=00000000 stage=00000007 attempted=0000001f divider=a51705f1 lp_gate=0a12abcd provider_before=a0ff00ff provider_after=a0ff01ff source_before=a1234567 source_after=21234567 phy_before=00000221 phy_after=00000223 nominal_hz=01312d00 preserved=00000001 duplicate_busy=00000001 samples=00000010
DSI_REFERENCE_STATE power=00000000 phy_reset=00000000 bridge=00000000 dpi=000019d0 fifo=00000000 status=00001528
MAIA_NATIVE_DSI_REFERENCE_PASS
"""
assert runner.evaluate_dsi_reference(reference)
assert not runner.evaluate_dsi_access(reference)
for old,new in [('stage=00000007','stage=00000006'),('attempted=0000001f','attempted=00000017'),
                ('divider=a51705f1','divider=a51805f1'),('lp_gate=0a12abcd','lp_gate=0212abcd'),
                ('provider_after=a0ff01ff','provider_after=a0ff01fe'),('source_after=21234567','source_after=61234567'),
                ('phy_after=00000223','phy_after=000002a3'),('nominal_hz=01312d00','nominal_hz=01312d01'),
                ('DSI_REFERENCE_STATE power=00000000','DSI_REFERENCE_STATE power=00000001')]:
    assert not runner.evaluate_dsi_reference(reference.replace(old,new))
assert not runner.evaluate_dsi_reference(reference+reference)
assert not runner.evaluate_dsi_reference(reference+'DSI_REFERENCE malformed\n')
assert not runner.evaluate_dsi_reference(reference.replace('MAIA_NATIVE_DSI_REFERENCE_PASS',''))
matching_reference=reference.replace('attempted=0000001f','attempted=00000010').replace('provider_before=a0ff00ff','provider_before=a0ff01ff').replace('source_before=a1234567','source_before=21234567')
assert runner.evaluate_dsi_reference(matching_reference)
print('PASS: DSI reference capture requires exact divider/provider/source/gate preservation and nominal frequency')

phy = reference + """DSI_PHY result=00000000 stage=0000000b writes=00000029 bytes=00000005 polls=00000004 status=00001595 power=00000001 phy_reset=0000000f lanes_before=abcd1200 lanes=abcd1201 mbps=000003c0 preserved=00000001 duplicate_busy=00000001 samples=00000010
MAIA_NATIVE_DSI_PHY_PASS
"""
assert runner.evaluate_dsi_phy(phy)
assert not runner.evaluate_dsi_reference(phy)
for old,new in [('stage=0000000b','stage=0000000a'),('writes=00000029','writes=00000028'),('bytes=00000005','bytes=00000004'),
                ('polls=00000004','polls=00000000'),('status=00001595','status=00001594'),('status=00001595','status=00001515'),
                ('power=00000001','power=00000000'),('phy_reset=0000000f','phy_reset=00000007'),('mbps=000003c0','mbps=000003c1'),
                ('lanes=abcd1201','lanes=abcd1202')]:
    assert not runner.evaluate_dsi_phy(phy.replace(old,new))
assert not runner.evaluate_dsi_phy(phy+phy)
assert not runner.evaluate_dsi_phy(phy+'DSI_PHY malformed\n')
assert not runner.evaluate_dsi_phy(phy.replace('MAIA_NATIVE_DSI_PHY_PASS',''))
print('PASS: PHY capture requires completed program, bounded lock/all-stop, exact lanes and nominal960Mbps')

command_masks=[1,0xffff,0x03ff03ff,0x03ff03ff,0x39,0xffffffff,0xffff,0xffff,0xffff,0xffff,0xffff,0x7fff,0xff00,3]
command_values=[1,0x0c07,0x00320068,0x002e0080,0x19,0,0,0,0,0,0,6000,0x3f00,0]
command=phy+'DSI_COMMAND result=00000000 stage=00000010 attempted=00001fff status=00001595 fifo=00000015 escape_hz=01059449 timeout_hz=00989680 preserved=00000001 duplicate_busy=00000001 samples=00000010\n'
for i,(mask,value) in enumerate(zip(command_masks,command_values)):
    before=(0xa5000000&~mask)|(~value&mask) if i<12 else 0xabcd1201 if i==12 else 0x40000000
    command+=f'DSI_COMMAND_WORD index={i:08x} before={before:08x} after={(before&~mask)|value:08x}\n'
command+='MAIA_NATIVE_DSI_COMMAND_PASS\n'
assert runner.evaluate_dsi_command(command)
assert not runner.evaluate_dsi_phy(command)
for old,new in [('stage=00000010 attempted=','stage=0000000f attempted='),('attempted=00001fff','attempted=00000fff'),
                ('fifo=00000015','fifo=00000055'),('escape_hz=01059449','escape_hz=01059448'),
                ('after=a5000c07','after=a5000c05'),('after=a5000c07','after=a5000c06'),
                ('escape_hz=01059449','escape_hz=01312d00'),('after=abcd3f01','after=abcd3f02'),
                ('DSI_COMMAND_WORD index=00000001','DSI_COMMAND_WORD index=00000000')]:
    assert old in command
    assert not runner.evaluate_dsi_command(command.replace(old,new))
assert not runner.evaluate_dsi_command(command+command)
assert not runner.evaluate_dsi_command(command+'DSI_COMMAND_WORD malformed\n')
assert not runner.evaluate_dsi_command(command.replace('MAIA_NATIVE_DSI_COMMAND_PASS',''))
print('PASS: command config capture requires exact masked table, complete indices, rates and empty/not-busy FIFO')

short=command
for i in range(16):
    short+=f'DSI_SHORT index={i:08x} result=00000000 stage=00000005 header=00000005 attempted=00000001 polls=00000004 busy=00000001 before={0 if i==0 else 0x10000:08x} after=00010000 status=00001595 fifo=00000015 error0=00000000 error1=00000000\n'
short+='DSI_SHORT_SUMMARY packets=00000010 busy=00000010 mode_changes=00000001 preserved=00000001 panel_ack=00000000\nMAIA_NATIVE_DSI_SHORT_PASS\n'
assert runner.evaluate_dsi_short(short)
assert not runner.evaluate_dsi_command(short)
for old,new in [('header=00000005','header=00000015'),('attempted=00000001 polls=','attempted=00000000 polls='),
                ('after=00010000 status=','after=00010002 status='),('error0=00000000','error0=00010000'),
                ('error1=00000000','error1=00000100'),('panel_ack=00000000','panel_ack=00000001'),
                ('DSI_SHORT index=00000001','DSI_SHORT index=00000000')]:
    assert not runner.evaluate_dsi_short(short.replace(old,new,1))
assert not runner.evaluate_dsi_short(short+short)
assert not runner.evaluate_dsi_short(short+'DSI_SHORT malformed\n')
assert not runner.evaluate_dsi_short(short.replace('MAIA_NATIVE_DSI_SHORT_PASS',''))
assert runner.evaluate_dsi_short(short.replace('busy=00000001 before=','busy=00000000 before=').replace('busy=00000010 mode_changes=','busy=00000000 mode_changes='))
print('PASS: short capture requires16 ordered NOP publications, LP/idle/error preservation and no panel ACK claim')

long=short
for i in range(4):
    long+=f'DSI_LONG index={i:08x} result=00000000 stage=00000006 header=00000139 attempted=00000001 bytes=00000001 words=00000001 polls=00000004 space=00000002 busy=00000001 before={0x10000 if i==0 else 0x90000:08x} after=00090000 pre_fifo=00010011 status=00001595 fifo=00050015 error0=00000000 error1=00000000\n'
long+='DSI_LONG_FOLLOWUP result=00000000 stage=00000005 header=00000005 attempted=00000001 polls=00000002 busy=00000001 status=00001595 fifo=00050015 error0=00000000 error1=00000000\nDSI_LONG_SUMMARY packets=00000004 busy=00000004 mode_changes=00000001 preserved=00000001 panel_ack=00000000\nMAIA_NATIVE_DSI_LONG_PASS\n'
assert runner.evaluate_dsi_long(long)
assert not runner.evaluate_dsi_short(long)
for old,new in [('header=00000139','header=00000439'),('bytes=00000001 words=','bytes=00000004 words='),
                ('space=00000002','space=00000004'),('after=00090000','after=000d0000'),
                ('pre_fifo=00010011','pre_fifo=00010051'),('DSI_LONG index=00000001','DSI_LONG index=00000000')]:
    assert not runner.evaluate_dsi_long(long.replace(old,new,1))
assert not runner.evaluate_dsi_long(long+long)
assert not runner.evaluate_dsi_long(long+'DSI_LONG malformed\n')
assert not runner.evaluate_dsi_long(long.replace('MAIA_NATIVE_DSI_LONG_PASS',''))
assert not runner.evaluate_dsi_long(long.replace('DSI_LONG_FOLLOWUP','MISSING'))
print('PASS: long capture requires4 exact-WC NOP frames, payload counts/LP state, bounded checks and short followup')

assert not runner.evaluate_dsi_long(long.replace("fifo=00050015", "fifo=00010015",1))

read=long
for i in range(4):
    read+=f'DSI_READ index={i:08x} result=00000000 stage=0000000a headers=00000003 words=00000001 word=aabbccdd id0=000000dd id1=000000cc id2=000000bb polls=00000007 before=00090000 after=00090000 packet=00000019 vc=00000000 restored=00000005 status=00001595 fifo=00050015 error0=00000000 error1=00000000\n'
read+='DSI_READ_SUMMARY replies=00000004 id=00bbccdd stable=00000001 preserved=00000001\nMAIA_NATIVE_DSI_READ_PASS\n'
assert runner.evaluate_dsi_read(read)
assert not runner.evaluate_dsi_long(read)
for old,new in [('headers=00000003','headers=00000001'),('words=00000001 word=','words=00000000 word='),
                ('id0=000000dd','id0=000000dc'),('after=00090000 packet=','after=010d0000 packet='),
                ('restored=00000005','restored=00000001'),('status=00001595 fifo=00050015 error0=00000000 error1=00000000\nDSI_READ', 'status=00001597 fifo=00050015 error0=00000000 error1=00000000\nDSI_READ')]:
    assert old in read;assert not runner.evaluate_dsi_read(read.replace(old,new,1))
assert not runner.evaluate_dsi_read(read+read)
assert not runner.evaluate_dsi_read(read+'DSI_READ malformed\n')
assert not runner.evaluate_dsi_read(read.replace('MAIA_NATIVE_DSI_READ_PASS',''))
refusal=('DSI_READ index=00000000 result=fffffff9 stage=00000006 headers=00000003 '
         'words=00000000 word=00000000 id0=00000000 id1=00000000 id2=00000000 '
         'polls=00000006 before=00090000 after=010d0000 packet=00000019 vc=00000000 '
         'restored=00000000 status=000015bd fifo=00050005 error0=00100000 error1=00000000\n'
         'MAIA_NATIVE_MIPI_PARK_FAIL read-id\n')
assert not runner.evaluate_dsi_read(long+refusal)
# A success marker cannot turn a PHY-error response into a valid read.
assert not runner.evaluate_dsi_read(read.replace(
    'restored=00000005 status=00001595 fifo=00050015 error0=00000000',
    'restored=00000005 status=00001595 fifo=00050015 error0=00100000',1))
print('PASS: read capture requires four response words/stable byte views, bounded polls and restored host state')

quiet_reset=('DSI_RESET_QUIET sys=0c0d4278 phy=00000220 reset=3c000040 firmware=00000001 dma_quiet=00000001 supply_ready=00000001\n'
             'DSI_PANEL_RESET result=00000000 stage=00000009 changed=00000001 elapsed_us=0003bba3\n'
             'DSI_RESET_RECOVERY firmware=00000001 status=00000000 banks_preserved=00000001 clocks_preserved=00000001 supply_preserved=00000001 gpio23_input=00000001\n'
             'MAIA_NATIVE_DSI_RESET_PASS\n')
assert 'MAIA_NATIVE_VO3_CYCLE_PASS\n' in read
reset=read.replace('MAIA_NATIVE_VO3_CYCLE_PASS\n','MAIA_NATIVE_VO3_CYCLE_PASS\n'+quiet_reset)
assert runner.evaluate_dsi_reset(reset)
assert not runner.evaluate_dsi_read(reset)
for old,new in [('sys=0c0d4278','sys=0c0d5278'),('phy=00000220','phy=00000221'),
                ('reset=3c000040','reset=38000040'),('firmware=00000001 dma_quiet','firmware=00000002 dma_quiet'),
                ('dma_quiet=00000001','dma_quiet=00000000'),('supply_ready=00000001','supply_ready=00000000'),
                ('stage=00000009 changed=','stage=00000008 changed='),('elapsed_us=0003bba3','elapsed_us=00000001'),
                ('DSI_RESET_RECOVERY firmware=00000001','DSI_RESET_RECOVERY firmware=00000003'),
                ('banks_preserved=00000001','banks_preserved=00000000'),('clocks_preserved=00000001','clocks_preserved=00000000'),
                ('gpio23_input=00000001','gpio23_input=00000000')]:
    assert old in reset and not runner.evaluate_dsi_reset(reset.replace(old,new,1))
assert not runner.evaluate_dsi_reset(read)
assert not runner.evaluate_dsi_reset(reset+quiet_reset)
assert not runner.evaluate_dsi_reset(read+quiet_reset)
assert not runner.evaluate_dsi_reset(reset.replace('MAIA_NATIVE_DSI_RESET_PASS\n',''))
print('PASS: quiet reset requires parked clocks/DMA, supported firmware, completed reset/recovery before DSI response')

program=reset
for i,(command,length) in enumerate([(1, 1), (96, 4), (96, 4), (96, 4), (120, 2), (121, 2), (164, 2), (183, 7), (176, 8), (191, 3), (165, 3), (215, 7), (144, 8), (163, 40), (166, 56), (167, 61), (172, 45), (173, 26), (174, 8), (178, 18), (232, 15), (117, 3), (231, 43), (225, 3), (234, 4), (200, 38), (201, 38), (96, 4), (17, 1), (41, 1), (53, 2)]):
    program+=f'ST7121_PACKET index={i:08x} command={command:08x} bytes={length:08x} result=00000000 stage={5 if length<=2 else 6:08x} polls=00000007 error0=00000000 error1=00000000\n'
program+='ST7121_PROGRAM result=00000000 attempted=0000001f completed=0000001f waited_ms=0000047e last=00000035 callback=00000000 elapsed_us=00130000\n'
for i in range(4):
    program+=f'ST7121_REPLY index={i:08x} result=00000000 stage=0000000a headers=00000003 polls=00000007 word=00fba080 words=00000001 before=000b0000 after=000b0000 restored=00000005 status=000015bd fifo=00050015 error0=00000000 error1=00000000\n'
program+='MAIA_NATIVE_ST7121_PASS\n'
assert runner.evaluate_st7121(program)
assert not runner.evaluate_dsi_reset(program)
for old,new in [('command=000000a7 bytes=0000003d','command=000000a7 bytes=0000003c'),
                ('completed=0000001f waited_ms=','completed=0000001e waited_ms='),('waited_ms=0000047e','waited_ms=0000047d'),
                ('elapsed_us=00130000','elapsed_us=00010000'),('last=00000035 callback=','last=00000029 callback='),
                ('before=000b0000 after=000b0000','before=000b0000 after=010f0000'),
                ('words=00000001 before=000b0000','words=00000000 before=000b0000')]:
    assert old in program and not runner.evaluate_st7121(program.replace(old,new,1))
assert not runner.evaluate_st7121(reset)
assert not runner.evaluate_st7121(program.replace('MAIA_NATIVE_ST7121_PASS',''))
assert not runner.evaluate_st7121(program+program)
assert not runner.evaluate_st7121(program.replace('error0=00000000 error1=00000000\nST7121_REPLY','error0=00100000 error1=00000000\nST7121_REPLY',1))
print('PASS: pinned ST7121 packet sequence, full waits, post-program stable responses and host restoration required')

def power_row(phase,index,value):
    mode=0x90000 if phase==0 else 0xb0000
    return (f'PANEL_POWER phase={phase:08x} index={index:08x} result=00000000 stage=0000000a request=00000a06 headers=00000003 polls=00000007 '
            f'word={value:08x} value={value:08x} words=00000001 before={mode:08x} after={mode:08x} restored=00000005 status=000015bd fifo=00050015 error0=00000000 error1=00000000\n')
power_before=''.join(power_row(0,i,8) for i in range(4))
power_after=''.join(power_row(1,i,0x9c) for i in range(4))
power=program.replace('MAIA_NATIVE_DSI_READ_PASS\n','MAIA_NATIVE_DSI_READ_PASS\n'+power_before)+power_after
power+='PANEL_POWER_SUMMARY before=00000008 after=0000009c sleep_out=00000001 display_on=00000001\nMAIA_NATIVE_PANEL_POWER_PASS\n'
assert runner.evaluate_panel_power(power)
assert not runner.evaluate_st7121(power)
for old,new in [('request=00000a06','request=00000406'),('value=00000008','value=00000009'),
                ('stage=0000000a request=','stage=00000007 request='),('PANEL_POWER phase=00000000 index=00000001','PANEL_POWER phase=00000000 index=00000000'),
                ('before=00090000 after=00090000 restored=00000005','before=00090000 after=010d0000 restored=00000005'),
                ('before=000b0000 after=000b0000 restored=00000005','before=000b0000 after=000b0000 restored=00000001'),
                ('sleep_out=00000001 display_on=00000001','sleep_out=00000000 display_on=00000001')]:
    assert old in power and not runner.evaluate_panel_power(power.replace(old,new,1))
assert not runner.evaluate_panel_power(power.replace(power_after,''))
assert not runner.evaluate_panel_power(power.replace('MAIA_NATIVE_PANEL_POWER_PASS',''))
assert not runner.evaluate_panel_power(power+power_after)
# Consistent byte/word/summary without SleepOut or DisplayOn still rejects.
for bad in (0x8c,0x98):
    assert not runner.evaluate_panel_power(power.replace('0000009c',f'{bad:08x}'))
assert not runner.evaluate_panel_power(power.replace(power_before,'')+power_before)
print('PASS: eight bounded RDDPM responses, low-byte views, restored state and real SleepOut/DisplayOn transition required')
# Already-on before programming must not count as a demonstrated transition.
already_on=power.replace(power_before,power_before.replace('00000008','0000009c')).replace(
    'PANEL_POWER_SUMMARY before=00000008','PANEL_POWER_SUMMARY before=0000009c')
assert not runner.evaluate_panel_power(already_on)
# Unrelated status bits are observations, not a hardcoded08/9C acceptance test.
other_bits=power.replace(power_before,power_before.replace('00000008','00000000')).replace(
    power_after,power_after.replace('0000009c','00000094')).replace(
    'PANEL_POWER_SUMMARY before=00000008 after=0000009c','PANEL_POWER_SUMMARY before=00000000 after=00000094')
assert runner.evaluate_panel_power(other_bits)
dpi_values=[0,0,0,0xbf02,720,0,0,3,60,1203,20,24,200,1280,2,0x050005f4,0x00140018,0x02d00322,0x00020028]
attempted=sum(1<<(i+1) for i,v in enumerate(dpi_values) if v)
dpi_stage=power+f'DSI_DPI_STAGE result=00000000 stage=00000016 attempted={attempted:08x} clock_before=00000223 clock_after=00000223 nominal_hz=04c4b400 status=000015bd fifo=00050015 error0=00000000 error1=00000000\n'
for i,value in enumerate(dpi_values):
    dpi_stage+=f'DSI_DPI_WORD index={i:08x} before=00000000 after={value:08x}\n'
dpi_stage+='DSI_DPI_IDLE mode=00000001 bridge=00000000 dpi=000019d0 clock=00000223 update=00000000 dma_quiet=00000001 preserved=00000001 duplicate_busy=00000001\n'
for i in range(4):
    dpi_stage+=f'DSI_DPI_REPLY index={i:08x} result=00000000 stage=0000000a headers=00000003 polls=00000006 word=0000009c words=00000001 before=000b0000 after=000b0000 restored=00000005 status=000015bd fifo=00050015 error0=00000000 error1=00000000\n'
dpi_stage+='MAIA_NATIVE_DPI_STAGE_PASS\n'
assert runner.evaluate_dpi_stage(dpi_stage)
assert not runner.evaluate_panel_power(dpi_stage)
for old,new in [('clock_after=00000223','clock_after=000002a3'),('nominal_hz=04c4b400','nominal_hz=042c1d80'),
                ('mode=00000001 bridge=00000000 dpi=','mode=00000000 bridge=00000000 dpi='),
                ('dma_quiet=00000001 preserved=00000001','dma_quiet=00000000 preserved=00000001'),
                ('dpi=000019d0 clock=','dpi=000019d1 clock='),('update=00000000 dma_quiet=','update=00000001 dma_quiet='),
                ('index=00000009 before=00000000 after=000004b3','index=00000009 before=00000000 after=000004b2'),
                ('index=0000000e before=00000000 after=00000002','index=0000000e before=00000000 after=00000000'),
                ('duplicate_busy=00000001\nDSI_DPI_REPLY','duplicate_busy=00000000\nDSI_DPI_REPLY')]:
    assert old in dpi_stage and not runner.evaluate_dpi_stage(dpi_stage.replace(old,new,1))
assert not runner.evaluate_dpi_stage(power)
assert not runner.evaluate_dpi_stage(dpi_stage.replace('MAIA_NATIVE_DPI_STAGE_PASS',''))
assert not runner.evaluate_dpi_stage(dpi_stage+dpi_stage)
print('PASS: staged RGB565/80MHz route/timing requires exact fields, idle output/DMA and retained power responses')

def dpi_color_row(phase,index,value):
    return (f'DPI_COLOR phase={phase:08x} index={index:08x} result=00000000 stage=0000000a request=00000c06 headers=00000003 polls=00000006 '
            f'word={value:08x} value={value:08x} words=00000001 before=000b0000 after=000b0000 restored=00000005 status=000015bd fifo=00050015 error0=00000000 error1=00000000\n')
clocked=dpi_stage+''.join(dpi_color_row(p,i,7 if p==0 else 5) for p in (0,1) for i in range(4))
clocked+='DPI_COLOR_SELECT before=00000007 after=00000005 parameter=00000055 attempted=00000001 header=00553a15\n'
clocked+='DPI_CLOCK result=00000000 stage=00000004 attempted=00000002 provider_before=58170503 provider_after=58170503 clock_before=00000223 clock_after=000002a3 nominal_hz=04c4b400 status=000015bd fifo=00050015 error0=00000000 error1=00000000\n'
clocked+='DPI_CLOCK_IDLE mode=00000001 bridge=00000000 dpi=000019d0 update=00000000 dma_quiet=00000001 preserved=00000001 duplicate_busy=00000001\n'
clocked+=''.join(dpi_color_row(2,i,5) for i in range(4))
for i in range(4):
    clocked+=f'DPI_CLOCK_REPLY index={i:08x} result=00000000 stage=0000000a headers=00000003 polls=00000006 word=0000009c words=00000001 before=000b0000 after=000b0000 restored=00000005 status=000015bd fifo=00050015 error0=00000000 error1=00000000\n'
clocked+='MAIA_NATIVE_DPI_CLOCK_PASS\n'
assert runner.evaluate_dpi_clock(clocked)
assert not runner.evaluate_dpi_stage(clocked)
for old,new in [('provider_after=58170503 clock_before','provider_after=18170503 clock_before'),
                ('clock_after=000002a3 nominal_hz','clock_after=00000223 nominal_hz'),
                ('DPI_COLOR_SELECT before=00000007 after=00000005','DPI_COLOR_SELECT before=00000007 after=00000007'),
                ('header=00553a15','header=00773a15'),('stage=00000004 attempted=00000002 provider_before','stage=00000003 attempted=00000002 provider_before'),
                ('DPI_CLOCK_IDLE mode=00000001','DPI_CLOCK_IDLE mode=00000000')]:
    assert old in clocked and not runner.evaluate_dpi_clock(clocked.replace(old,new,1))
assert not runner.evaluate_dpi_clock(clocked.replace('MAIA_NATIVE_DPI_CLOCK_PASS',''))
assert not runner.evaluate_dpi_clock(dpi_stage)
assert not runner.evaluate_dpi_clock(clocked+clocked)
# Already-selected5 skips COLMOD, but still verifies readback and owned clock.
selected=clocked
for i in range(4): selected=selected.replace(dpi_color_row(0,i,7),dpi_color_row(0,i,5))
selected=selected.replace('DPI_COLOR_SELECT before=00000007','DPI_COLOR_SELECT before=00000005').replace('attempted=00000001 header=00553a15','attempted=00000000 header=00000000')
assert runner.evaluate_dpi_clock(selected)
print('PASS: RGB565 readback, exact provider/gate enable, inactive output/DMA and retained color/power responses required')

initial_zero=clocked
for i in range(4): initial_zero=initial_zero.replace(dpi_color_row(0,i,7),dpi_color_row(0,i,0))
initial_zero=initial_zero.replace('DPI_COLOR_SELECT before=00000007','DPI_COLOR_SELECT before=00000000')
assert runner.evaluate_dpi_clock(initial_zero)

# Synthetic schema fixtures only; never hardware evidence.
patterned=clocked+'\nDPI_PATTERN_LIGHT out_before=00000000 enable_before=00000000 pad_before=00000004 mux_before=00007200 matrix_before=0000007f out=00400000 enable=00400000 pad=00000000 mux=00001200 matrix=00000500\n'+('DPI_PATTERN_START result=00000000 stage=00000004 attempted=00000007 pattern=0001bf02 mode=00000000 lp=00000003 shadow=00000000 polls=00000000 status=00001591 fifo=00050015 error0=00000000 error1=00000000\n')
for i in range(5):
    patterned+=f'DPI_PATTERN_HOLD index={i:08x} mode=00000000 pattern=0001bf02 lp=00000003 status=00001591 fifo=00050015 error0=00000000 error1=00000000 inactive=00000001 exclusive=00000001\n'
patterned+='DPI_PATTERN_STOP result=00000000 stage=00000005 attempted=00000007 pattern=0000bf02 mode=00000001 lp=00000000 shadow=00000000 polls=00000002 status=00001595 fifo=00050015 error0=00000000 error1=00000000\n'
patterned+='DPI_PATTERN_LIGHT_RESTORE verified=00000001\n'
patterned+='\n'.join(l.replace('DPI_CLOCK_REPLY','DPI_PATTERN_REPLY') for l in clocked.splitlines() if l.startswith('DPI_CLOCK_REPLY '))+'\nMAIA_NATIVE_DPI_PATTERN_PASS\n'
assert runner.evaluate_dpi_pattern(patterned)
assert not runner.evaluate_dpi_clock(patterned)
for a,b in [('pattern=0001bf02','pattern=0011bf02'),('lp=00000003','lp=00000001'),
            ('inactive=00000001','inactive=00000000'),('exclusive=00000001','exclusive=00000000'),
            ('shadow=00000000','shadow=00000001'),('polls=00000002 status=00001595','polls=00000000 status=00001595'),
            ('DPI_PATTERN_STOP result=00000000','DPI_PATTERN_STOP result=00000001'),
            ('DPI_PATTERN_HOLD index=00000003','DPI_PATTERN_HOLD index=00000004'),
            ('MAIA_NATIVE_DPI_PATTERN_PASS',''),('DPI_PATTERN_START result=00000000','DPI_PATTERN_START result=00000000 error0=00000000')]:
    assert not runner.evaluate_dpi_pattern(patterned.replace(a,b)),(a,b)
assert not runner.evaluate_dpi_pattern(patterned.replace('DPI_PATTERN_HOLD index=00000003 mode=00000000','DPI_PATTERN_HOLD index=00000003 mode=00000001'))
assert not runner.evaluate_dpi_pattern(patterned+'DPI_PATTERN_BOGUS result=00000000\n')
print('PASS: native VPG start/hold/stop/reply order, exclusive transport, inactive DMA and restored LP required')

for a,b in [('out=00400000','out=00400001'),('matrix=00000500','matrix=00000100'),('mux=00001200','mux=00007200'),('DPI_PATTERN_LIGHT_RESTORE verified=00000001','')]:
    assert not runner.evaluate_dpi_pattern(patterned.replace(a,b))

bridge=clocked+'\nDPI_BRIDGE_REQUEST result=00000000 stage=00000004 attempted=00000003 before=a4000000 after=a4000001 update_value=00000001 irq_raw=00000000 status=000015bd fifo=00050015 error0=00000000 error1=00000000\nDPI_BRIDGE_IDLE bridge=a4000001 dpi=000019d0 mode=00000001 lp=00000000 dma_quiet=00000001 preserved=00000001 duplicate_busy=00000001 vpg_busy=00000001 latch_verified=00000000\n'
bridge+='\n'.join(l.replace('DPI_CLOCK_REPLY','DPI_BRIDGE_REPLY') for l in clocked.splitlines() if l.startswith('DPI_CLOCK_REPLY '))+'\nMAIA_NATIVE_DPI_BRIDGE_PASS\n'
assert runner.evaluate_dpi_bridge(bridge)
assert not runner.evaluate_dpi_clock(bridge)
for a,b in [('after=a4000001','after=00000001'),('before=a4000000','before=a4000001'),('update_value=00000001','update_value=00000000'),
            ('irq_raw=00000000','irq_raw=00000001'),('dpi=000019d0','dpi=000019d1'),('mode=00000001 lp=00000000','mode=00000000 lp=00000003'),
            ('duplicate_busy=00000001 vpg_busy=00000001','duplicate_busy=00000000 vpg_busy=00000001'),('latch_verified=00000000','latch_verified=00000001'),
            ('DPI_BRIDGE_REPLY index=00000002','DPI_BRIDGE_REPLY index=00000003'),('MAIA_NATIVE_DPI_BRIDGE_PASS','')]:
    assert not runner.evaluate_dpi_bridge(bridge.replace(a,b)),(a,b)
assert not runner.evaluate_dpi_bridge(bridge+'DPI_BRIDGE_BOGUS result=00000000\n')
print('PASS: bridge ownership, WT publication without ACK claim, idle output/DMA and retained power replies required')

tile=bridge+'\nDPI_DMA_TILE_MEMORY start=4ff10000 end=4ff15700 alias=8ff10000 source=4ff10080 list=4ff12e80 source_bytes=00002d00 count=000000a0 list_bytes=00002800 frame_bytes=001c2000 cache_line=00000040 prepared=00000001\n'
for i in (0,80,159):
    link=(0x4ff12e80+(i+1)*64)|1 if i<159 else 0
    ctl1=0x80108840|(0x40000000 if i==159 else 0)
    tile+=f'DPI_DMA_TILE_ITEM index={i:08x} source=4ff10080 destination=50105000 items=000005a0 next={link:08x} ctl0=001e1b41 ctl1={ctl1:08x} zero=00000001\n'
a,b=runner.dpi_dma_tile_hashes(0x4ff10080,0x4ff12e80)
tile+=f'DPI_DMA_TILE_VERIFY source_hash={a:08x} list_hash={b:08x} guards=00000001 preserved=00000001 inert=00000001 repeated_bytes=001c2000\n'
tile+='\n'.join(l.replace('DPI_BRIDGE_REPLY','DPI_DMA_TILE_REPLY') for l in bridge.splitlines() if l.startswith('DPI_BRIDGE_REPLY '))+'\nMAIA_NATIVE_DPI_DMA_TILE_PASS\n'
assert runner.evaluate_dpi_dma_tile(tile)
assert not runner.evaluate_dpi_bridge(tile)
for a,b in [('source=4ff10080','source=8ff10080'),('end=4ff15700','end=4ff28080'),('count=000000a0','count=0000009f'),
            ('destination=50105000','destination=50100000'),('ctl0=001e1b41','ctl0=001e1b45'),('ctl1=c0108840','ctl1=80108840'),
            ('next=00000000','next=4ff12e81'),('guards=00000001','guards=00000000'),('inert=00000001','inert=00000000'),
            ('cache_line=00000040','cache_line=00000008'),('MAIA_NATIVE_DPI_DMA_TILE_PASS','')]:
    assert not runner.evaluate_dpi_dma_tile(tile.replace(a,b)),(a,b)
assert not runner.evaluate_dpi_dma_tile(tile+'DPI_DMA_TILE_BOGUS result=00000000\n')
print('PASS: finite tile/list NC layout, independent source/list hashes and inert peripherals required')

stage=tile+'\nDPI_DMA_STAGE result=00000000 stage=00000012 attempted=0000ffff source=4ff10080 frame_bytes=001c2000 channel_status=00000000 common_status=00000000 status=000015bd fifo=00050015 error0=00000000 error1=00000000\n'
stage_masks=[0xffffffff]*4+[0x3fffff,0x7fffff55,0xc7ffffff,15,0x7f8e319f,0xffffffc1]+[0xffffffff]+[0xf83f7ffb,0xf1,0x10000000,0x7ff,0xfff]
stage_values=[0x4ff10080,0,0x50105000,0,230399,0x1c1b51,0xf8000,0,0xa020001]+[0]*3+[0x10,0,768,256]
for i,(mask,value) in enumerate(zip(stage_masks,stage_values)):
    before=0xa4000000|(~value&mask);after=(before&~mask)|value
    stage+=f'DPI_DMA_STAGE_WORD index={i:08x} before={before:08x} after={after:08x}\n'
a,b=runner.dpi_dma_tile_hashes(0x4ff10080,0x4ff12e80)
stage+=f'DPI_DMA_STAGE_IDLE mode=00000001 lp=00000000 dpi=000019d0 cfg=00000000 channels=00000000 abort=00000000 source_hash={a:08x} list_hash={b:08x} preserved=00000001 duplicate_busy=00000001\n'
stage+='\n'.join(l.replace('DPI_DMA_TILE_REPLY','DPI_DMA_STAGE_REPLY') for l in tile.splitlines() if l.startswith('DPI_DMA_TILE_REPLY '))+'\nMAIA_NATIVE_DPI_DMA_STAGE_PASS\n'
assert runner.evaluate_dpi_dma_stage(stage)
assert not runner.evaluate_dpi_dma_tile(stage)
for a,b in [('attempted=0000ffff','attempted=0000fffe'),('channel_status=00000000','channel_status=00000080'),('after=801c1b51','after=801e1b41'),
            ('after=8a020001','after=8a020011'),('cfg=00000000 channels=00000000','cfg=00000001 channels=00000000'),
            ('DPI_DMA_STAGE_WORD index=0000000f','DPI_DMA_STAGE_WORD index=0000000e'),('MAIA_NATIVE_DPI_DMA_STAGE_PASS','')]:
    changed=stage.replace(a,b);assert changed!=stage,(a,b)
    assert not runner.evaluate_dpi_dma_stage(changed),(a,b)
print('PASS: inactive fixed-source single-block channel/bridge fields, masks and memory preservation required')

frame=stage+'\nDPI_DMA_FRAME_LIGHT out=00400000 enable=00400d40 pad=00000000 mux=00001800 matrix=00000500\n'
frame+='DPI_DMA_FRAME result=00000000 stage=00000009 attempted=00000fff polls=00000005 recovery_polls=00000002 active=00000001 done=00000001 count_raw=000383ff items=00038400 channel_status=0000001b common_status=00000000 bridge_status=00000000 dma_fifo=00000000 bridge_fifo=00000000 recovered=00000001 transfer_result=00000000 recovery_result=00000000 dsi_status=000015bd dsi_fifo=00050015 error0=00000000 error1=00000000 elapsed_us=00004000\n'
a,b=runner.dpi_dma_tile_hashes(0x4ff10080,0x4ff12e80)
frame+=f'DPI_DMA_FRAME_RESTORE light=00000001 mode=00000001 lp=00000000 dpi=000019d0 cfg=00000000 channels=00000000 abort=00000000 source_hash={a:08x} list_hash={b:08x} preserved=00000001 duplicate_busy=00000001\n'
frame+='\n'.join(l.replace('DPI_DMA_STAGE_REPLY','DPI_DMA_FRAME_REPLY') for l in stage.splitlines() if l.startswith('DPI_DMA_STAGE_REPLY '))+'\nMAIA_NATIVE_DPI_DMA_FRAME_PASS\n'
assert runner.evaluate_dpi_dma_frame(frame)
assert not runner.evaluate_dpi_dma_stage(frame)
for a,b in [('items=00038400','items=000383ff'),('channel_status=0000001b','channel_status=00000019'),('bridge_status=00000000','bridge_status=00000001'),
            ('active=00000001 done=00000001','active=00000000 done=00000001'),('recovered=00000001','recovered=00000000'),('attempted=00000fff','attempted=00000ffe'),
            ('cfg=00000000 channels=00000000','cfg=00000001 channels=00000000'),('DPI_DMA_FRAME_REPLY index=00000002','DPI_DMA_FRAME_REPLY index=00000003'),
            ('MAIA_NATIVE_DPI_DMA_FRAME_PASS','')]:
    changed=frame.replace(a,b);assert changed!=frame,(a,b)
    assert not runner.evaluate_dpi_dma_frame(changed),(a,b)
print('PASS: actual full item count/done/active observations, no errors, idle recovery and retained replies required')

calibration=''
for i,(width,count) in enumerate(zip((8,64,8,64),(8,8,256,4096))):
    b=count*(width//8)
    calibration+=f'VDMA_COUNT index={i:08x} width={width:08x} items={count:08x} bytes={b:08x} matched={b:08x} raw={count-1:08x} status=00000000 common=00000000 guards=00000001 tail=00000001\n'
calibration+='VDMA_COUNT_IDLE cfg=00000000 channels=00000000 preserved=00000001\nMAIA_NATIVE_VDMA_COUNT_PASS\n'
assert runner.evaluate_vdma_count(calibration)
assert not runner.evaluate_vdma_count(calibration.replace('matched=00008000','matched=00007fff'))
assert not runner.evaluate_vdma_count(calibration.replace('tail=00000001','tail=00000000'))
print('PASS: counter qualification requires actual byte-copy proof; raw count remains an observation')

refreshed=frame.replace('DPI_DMA_FRAME_RESTORE light=', 'DPI_DMA_REFRESH frames=00000190 polls=000186a0 items=057e4000 rearms=0000018f\nDPI_DMA_FRAME_RESTORE light=')
assert runner.evaluate_dpi_dma_refresh(refreshed)
assert not runner.evaluate_dpi_dma_frame(refreshed)
for a,b in [('frames=00000190','frames=0000018f'),('rearms=0000018f','rearms=00000190'),('items=057e4000','items=057e3fff')]:
    assert not runner.evaluate_dpi_dma_refresh(refreshed.replace(a,b))
print('PASS: bounded400-frame total, checked rearm count and final recovery required')

# Independently constructed SRAM list capture. First node increments RGB565 data;
# second node repeats a solid source. Missing node/data/idle failures must reject.
base=0x4ff02000
linked=f'VDMA_LIST_MEMORY start={base:08x} end={base+66048:08x} source={base+128:08x} destination={base+33152:08x} list={base+16512:08x} cache_line=00000040\n'
lli=[base+128,0,base+33152,0,1439,0,(base+16576)|1,0,0x1b05,0x80000000,0,0,0,0,0,0,
     base+12416,0,base+44672,0,2655,0,0,0,0x1b15,0xc0000040,0,0,0,0,0,0]
for i in range(16):linked+=f'VDMA_LIST_SAMPLE offset={i*4:08x} word=84108410\n'
observed=lli.copy();observed[9]=0;observed[25]=0x40000040
observed[12]=1439;observed[13]=0x40000000;observed[28]=2655;observed[29]=0xc0000000
for i,v in enumerate(lli):linked+=f'VDMA_LIST_WORD index={i:08x} before={v:08x} after={observed[i]:08x}\n'
linked+='VDMA_LIST_COPY done=00000001 polls=00000010 matched=00008000 expected=00008000 source_kept=00000001 guards=00000001 list_valid=00000001 raw=00000a5f fifo=00000000 status=00000000 common=00000000\nVDMA_LIST_IDLE cfg=00000000 channels=00000000 abort=00000000 preserved=00000001\nMAIA_NATIVE_VDMA_LIST_PASS\n'
assert runner.evaluate_vdma_list(linked)
for old,new in [('matched=00008000','matched=00007fff'),('guards=00000001','guards=00000000'),('done=00000001','done=00000000'),('source_kept=00000001','source_kept=00000000'),('status=00000000','status=00002000'),('preserved=00000001','preserved=00000000'),('after=40000040','after=c0000040'),('word=84108410','word=a5a5a5a5')]:
    assert not runner.evaluate_vdma_list(linked.replace(old,new)),(old,new)
assert not runner.evaluate_vdma_list(linked+linked)
assert not runner.evaluate_vdma_count(linked)

spatial_capture=tile+'DPI_SPATIAL_MEMORY source=4ff10080 list=4ff12e80 nodes=00000005 source_bytes=00002d00 list_bytes=00000140 frame_bytes=001c2000\n'
spatial_capture+='DPI_SPATIAL result=00000000 stage=00000012 frame_stage=00000009 attempted=00000fff frames=00000190 polls=00061a80 items=057e4000 rearms=0000018f active=00000001 done=00000001 count_raw=0000e0ff status=00000012 common=00000000 underrun=00000000 dma_fifo=00000000 bridge_fifo=00000000 dsi_status=000015bd dsi_fifo=00050015 error0=00000000 error1=00000000 transfer=00000000 recovery=00000000 recovered=00000001 elapsed_us=00600000\n'
for n,count in enumerate((1440,56160,57600,57600,57600)):
    before=[0]*16;before[0]=0x4ff10080+(360*(n-1) if n else 0);before[2]=0x50105000;before[4]=count-1
    before[6]=(0x4ff12e80+64*(n+1))|1 if n<4 else 0;before[8]=0x1c1b41 if n==0 else 0x1c1b51;before[9]=0xc00f8040 if n==4 else 0x800f8040
    after=before.copy();after[9]&=0x7fffffff;after[12]=count-1;after[13]=0xc0000000 if n==4 else 0x40000000
    for w in range(16):spatial_capture+=f'DPI_SPATIAL_WORD index={n*16+w:08x} before={before[w]:08x} after={after[w]:08x}\n'
spatial_capture+='DPI_SPATIAL_RESTORE light=00000001 mode=00000001 lp=00000000 dpi=000019d0 cfg=00000000 channels=00000000 abort=00000000 source_hash=e18c2e45 source_kept=00000001 tail_kept=00000001 guards=00000001 preserved=00000001 duplicate_busy=00000001\n'
spatial_capture+='\n'.join(l.replace('DPI_DMA_TILE_REPLY','DPI_SPATIAL_REPLY') for l in tile.splitlines() if l.startswith('DPI_DMA_TILE_REPLY '))+'\nMAIA_NATIVE_DPI_SPATIAL_PASS\n'
assert runner.evaluate_dpi_spatial(spatial_capture)
assert not runner.evaluate_dpi_dma_tile(spatial_capture)
for old,new in [('frames=00000190','frames=0000018f'),('after=400f8040','after=c00f8040'),('count_raw=0000e0ff','count_raw=0000e100'),('tail_kept=00000001','tail_kept=00000000'),('underrun=00000000','underrun=00000001'),('guards=00000001','guards=00000000'),('items=057e4000','items=057abc00')]:
    assert old in spatial_capture
    assert not runner.evaluate_dpi_spatial(spatial_capture.replace(old,new)),(old,new)
assert not runner.evaluate_dpi_spatial(spatial_capture+spatial_capture)

# Independent central32-row profile; no legacy tile construction/160-node list.
import struct
center_hash=2166136261
for _ in range(32):
    for color in (0xf800,0x07e0,0x001f,0xffe0):
        for value in struct.pack('<H',color)*180:center_hash=((center_hash^value)*16777619)&0xffffffff
center=bridge+'DPI_CENTER_BUFFER start=4ff08000 end=4ff13700 alias=8ff08000 source=4ff08080 list=4ff13500 source_bytes=0000b400 list_capacity=00000180 span=0000b700 cache_line=00000040 prepared=00000001\n'
center+=f'DPI_CENTER_BUFFER_VERIFY source_hash={center_hash:08x} guards=00000001 preserved=00000001 inert=00000001\nMAIA_NATIVE_DPI_CENTER_BUFFER_PASS\n'
center+='DPI_CENTER_MEMORY source=4ff08080 list=4ff13500 nodes=00000005 source_bytes=0000b400 list_bytes=00000140 frame_bytes=001c2000\n'
center+=next(l for l in spatial_capture.splitlines() if l.startswith('DPI_SPATIAL ')).replace('DPI_SPATIAL','DPI_CENTER')+'\n'
for n,count in enumerate((57600,54720,5760,54720,57600)):
    before=[0]*16;before[0]=0x4ff08080+(0,360,0,720,1080)[n];before[2]=0x50105000;before[4]=count-1
    before[6]=(0x4ff13500+(n+1)*64)|1 if n<4 else 0;before[8]=0x1c1b41 if n==2 else 0x1c1b51;before[9]=0xc00f8040 if n==4 else 0x800f8040
    after=before.copy();after[9]&=0x7fffffff;after[12]=count-1;after[13]=0xc0000000 if n==4 else 0x40000000
    for w in range(16):center+=f'DPI_CENTER_WORD index={n*16+w:08x} before={before[w]:08x} after={after[w]:08x}\n'
center+=next(l for l in spatial_capture.splitlines() if l.startswith('DPI_SPATIAL_RESTORE ')).replace('DPI_SPATIAL','DPI_CENTER').replace('e18c2e45',f'{center_hash:08x}')+'\n'
center+='\n'.join(l.replace('DPI_SPATIAL_REPLY','DPI_CENTER_REPLY') for l in spatial_capture.splitlines() if l.startswith('DPI_SPATIAL_REPLY '))+'\nMAIA_NATIVE_DPI_CENTER_PASS\n'
assert runner.evaluate_dpi_spatial(center,center=True)
assert not runner.evaluate_dpi_spatial(center)
assert not runner.evaluate_dpi_spatial(spatial_capture,center=True)
for old,new in [('source_bytes=0000b400','source_bytes=00002d00'),('span=0000b700','span=00005700'),('source=4ff08080','source=4ff13500'),('before=0000167f','before=0000059f'),('before=001c1b41','before=001c1b51'),('after=400f8040','after=c00f8040'),('source_kept=00000001','source_kept=00000000')]:
    assert old in center,(old,new)
    assert not runner.evaluate_dpi_spatial(center.replace(old,new),center=True),(old,new)
assert not runner.evaluate_dpi_spatial(center+center,center=True)

# Read-only PSRAM snapshot success means observation, not available memory.
root_addresses=(0x500e6014,0x500e6030,0x500e60c0,0x500e60cc,0x501151d0,0x501151d4,0x5012d04c,0x5012d050,0x5012d1c0)
def psram_capture(accessible):
    out=f'PSRAM_STATE result={0 if accessible else 0xfffffffd:08x} roots_valid=00000001 samples={54 if accessible else 0:08x}\n'
    for i,address in enumerate(root_addresses):
        v=(0x80002000 if i==0 else 0xc100 if i==1 else 0) if accessible else 0
        out+=f'PSRAM_ROOT index={i:08x} address={address:08x} before={v:08x} after={v:08x}\n'
    if accessible:
        offsets=(0,8,12,64,68,72,76,80,216,384,388,392,396,400,512,304,0,8,12,16,20,24,28,32,212,384,512)
        for i,offset in enumerate(offsets):out+=f'PSRAM_WORD index={i:08x} address={(0x5008e000 if i<16 else 0x5008f000)+offset:08x} before={i:08x} after={i:08x}\n'
    return out+'MAIA_NATIVE_PSRAM_STATE_OBSERVED\n'
assert runner.evaluate_psram_state(psram_capture(True))
assert runner.evaluate_psram_state(psram_capture(False))
for old,new in [('after=80002000','after=00002000'),('samples=00000036','samples=00000035'),('address=5008e000','address=5008c000'),('roots_valid=00000001','roots_valid=00000000')]:
    assert not runner.evaluate_psram_state(psram_capture(True).replace(old,new))
assert not runner.evaluate_psram_state(psram_capture(False).replace('result=fffffffd','result=00000000'))
assert not runner.evaluate_psram_state(psram_capture(True)+psram_capture(True))

identity=psram_capture(True)
for n in range(8):
    reg=(0,2,4,8)[n%4];bits=16 if reg<4 else 8;value=(0x0d30,0x20df,0x20,0x4b)[n%4]
    identity+=f'PSRAM_ID index={n:08x} reg={reg:08x} result=00000000 stage=00000005 bits={bits:08x} setup=000003ff restore=000003ff triggered=00000001 polls=00000003 busy=00000001 command=00000000 value={value:08x} restored=00000001\n'
    original=(0x2ca00c,0x80000000,0x5c000007,0x70000000,0,0,2,0,0x20,0x0d30)
    configured=(0x2ca27c,0xf0000000,0x7c00000b,0xf0004040,reg,bits-1,1,2,0x21,0xa55a5aa5)
    for i,(b,c) in enumerate(zip(original,configured)):identity+=f'PSRAM_ID_WORD read={n:08x} index={i:08x} before={b:08x} configured={c:08x}\n'
identity+='PSRAM_ID_RESTORE result=00000000 roots=00000001 words=00000001 vendor=0000000d density=00000007 device=00000003 kgd=00000006 x16=00000001\nMAIA_NATIVE_PSRAM_ID_PASS\n'
assert runner.evaluate_psram_identity(identity)
assert not runner.evaluate_psram_state(identity)
for a,b in [('value=00000d30','value=00005aa5'),('restored=00000001','restored=00000000'),('configured=f0004040','configured=f000c0c0'),('command=00000000','command=00040000'),('value=000020df','value=000020dd'),('x16=00000001','x16=00000000')]:
    assert not runner.evaluate_psram_identity(identity.replace(a,b)),(a,b)
assert not runner.evaluate_psram_identity(identity+identity)

assert not runner.evaluate_psram_identity(psram_capture(False)+identity[len(psram_capture(True)):])

# Clock qualification requires real warm profile, calibrated powered VO2 and
# complete before/after clock words, not just identity or a PASS marker.
clock_roots=[0xE6DFB7FF,0xD00C,0x38000040,0,0x40200180,0x76800000,0x7B054313,0x3A184DEA,0]
clock_state=[]
for line in identity[:identity.index('PSRAM_ID index=')].splitlines():
    if line.startswith('PSRAM_ROOT '):
        row=dict(item.split('=') for item in line.split()[1:]);v=clock_roots[int(row['index'],16)]
        line=line[:line.index(' before=')]+f' before={v:08x} after={v:08x}'
    elif line.startswith('PSRAM_WORD '):
        row=dict(item.split('=') for item in line.split()[1:]);i=int(row['index'],16)
        v=0 if i in (0,2,16) else 0x30103 if i in (7,20) else i
        line=line[:line.index(' before=')]+f' before={v:08x} after={v:08x}'
    clock_state.append(line)
def clock_phase(phase):
    text=f'PSRAM_CLOCK phase={phase:08x} result=00000000 stage=00000005 attempts=0000001f restored={phase:08x}\n'
    for i,b in enumerate((0xd00c,0x30103,0x30103)):
        a=b if phase else (0xc00c,0x10001,0x10001)[i]
        text+=f'PSRAM_CLOCK_WORD phase={phase:08x} index={i:08x} before={b:08x} after={a:08x}\n'
    return text
clock_capture='MAIA_NATIVE_PSRAM_CLOCK_HOST_READY\n'+'\n'.join(clock_state)+'\n'+'PSRAM_AXI control=80000000\n'+clock_phase(0)+identity[identity.index('PSRAM_ID index='):]+clock_phase(1)
clock_capture+='PSRAM_CLOCK_FINAL result=00000000 roots=00000001 words=00000001\nPSRAM_AXI_FINAL control=80000000\nMAIA_NATIVE_PSRAM_CLOCK_PASS\n'
assert runner.evaluate_psram_clock(clock_capture)
assert not runner.evaluate_psram_clock(identity)
assert not runner.evaluate_psram_identity(clock_capture)
assert not runner.evaluate_psram_state(clock_capture)
for a,b in [('after=0000c00c','after=0000d00c'),('attempts=0000001f','attempts=0000000f'),
            ('before=76800000 after=76800000','before=66800000 after=66800000'),
            ('before=40200180 after=40200180','before=40200080 after=40200080'),
            ('after=00010001','after=00030103'),('PSRAM_CLOCK_FINAL result=00000000 roots=00000001 words=00000001','PSRAM_CLOCK_FINAL result=00000000 roots=00000001 words=00000000')]:
    assert a in clock_capture
    assert not runner.evaluate_psram_clock(clock_capture.replace(a,b)),(a,b)
assert not runner.evaluate_psram_clock(clock_capture+clock_capture)
assert not runner.evaluate_psram_clock(clock_capture.replace('MAIA_NATIVE_PSRAM_CLOCK_HOST_READY\n',''))

array_records='PSRAM_ARRAY_MODE phase=00000000 result=00000000 stage=00000003 before=0000004b after=0000000b\n'
for i in range(11):
    w=i%2
    array_records+=f'PSRAM_ARRAY_TRANSFER index={i:08x} write={w:08x} result=00000000 completed=00001000 chunks=00000040 polls=00000080 command=00000000 restored=00000001 attempted={w:08x}\n'
    if i in (2,4,6,8):
        p=i//2-1;state=0x783a1b25;h=2166136261
        for j in range(4096):
            state=(state*1664525+1013904223)&0xffffffff
            b=0 if p==0 else 255 if p==1 else (0x55 if j&1 else 0xaa) if p==2 else state>>24
            h=((h^b)*16777619)&0xffffffff
        array_records+=f'PSRAM_ARRAY_PATTERN index={p:08x} matched=00001000 expected={h:08x} actual={h:08x}\n'
array_records+='PSRAM_ARRAY_RESTORE matched=00001000 before=12345678 after=12345678 guards=00000001\nPSRAM_ARRAY_MODE phase=00000001 result=00000000 stage=00000003 before=0000000b after=0000004b\nMAIA_NATIVE_PSRAM_ARRAY_PASS\n'
array_capture=clock_capture.replace('PSRAM_CLOCK phase=00000001',array_records+'PSRAM_CLOCK phase=00000001')
assert runner.evaluate_psram_array(array_capture)
assert not runner.evaluate_psram_clock(array_capture)
assert not runner.evaluate_psram_array(clock_capture)
for a,b in [('completed=00001000','completed=00000040'),('chunks=00000040','chunks=0000003f'),('matched=00001000','matched=00000fff'),('guards=00000001','guards=00000000'),('before=12345678 after=12345678','before=12345678 after=12345679'),('PSRAM_ARRAY_TRANSFER index=00000001 write=00000001','PSRAM_ARRAY_TRANSFER index=00000001 write=00000000')]:
    assert a in array_capture
    assert not runner.evaluate_psram_array(array_capture.replace(a,b)),(a,b)
assert not runner.evaluate_psram_array(array_capture+array_capture)

assert not runner.evaluate_psram_array(array_capture.replace("before=0000004b after=0000000b","before=0000004b after=0000004b"))

assert not runner.evaluate_psram_array(array_capture.replace("control=80000000","control=80000001"))

window_records='PSRAM_WINDOW_MODE phase=00000000 result=00000000 stage=00000003 before=0000004b after=0000000b\n'
window_records+='PSRAM_WINDOW_RAW phase=00000000 result=00000000 write=00000000 completed=00001000 chunks=00000040 restored=00000001\n'
for phase in range(3):
    stage=(4,5,6)[phase]
    window_records+=f'PSRAM_WINDOW phase={phase:08x} result=00000000 stage={stage:08x} vaddr=4b800000 bytes=00400000 physical=00000000 restored={int(phase==2):08x} polls=00000002\n'
    for i in range(64):window_records+=f'PSRAM_WINDOW_MMU phase={phase:08x} index={i:08x} before=00000000 after={0 if phase==2 else 0xc00+i:08x}\n'
    if phase==1:window_records+='PSRAM_WINDOW_DATA words=00100000 matched=00100000 expected=3e80e5d7 actual=3e80e5d7\n'
window_records+='PSRAM_WINDOW_RAW phase=00000001 result=00000000 write=00000000 completed=00001000 chunks=00000040 restored=00000001\nPSRAM_WINDOW_DIRECT matched=00001000\n'
window_records+='PSRAM_WINDOW_RAW phase=00000002 result=00000000 write=00000001 completed=00001000 chunks=00000040 restored=00000001\n'
window_records+='PSRAM_WINDOW_RAW phase=00000003 result=00000000 write=00000000 completed=00001000 chunks=00000040 restored=00000001\n'
window_records+='PSRAM_WINDOW_RESTORE matched=00001000\nPSRAM_WINDOW_MODE phase=00000001 result=00000000 stage=00000003 before=0000000b after=0000004b\nMAIA_NATIVE_PSRAM_WINDOW_PASS\n'
window_capture=array_capture.replace('PSRAM_STATE result=', 'PSRAM_CACHE_BOOT_START\nPSRAM_CACHE_BOOT result=00000000 stage=00000003 size=00020000 line=00000080 ways=00000008\nPSRAM_STATE result=',1)
window_capture=window_capture.replace('PSRAM_CLOCK phase=00000001',window_records+'PSRAM_CLOCK phase=00000001')
assert runner.evaluate_psram_window(window_capture)
assert not runner.evaluate_psram_array(window_capture)
assert not runner.evaluate_psram_window(array_capture)
for a,b in [('vaddr=4b800000','vaddr=48100000'),('bytes=00400000','bytes=00001000'),('actual=3e80e5d7','actual=3e80e5d8'),('PSRAM_WINDOW_DIRECT matched=00001000','PSRAM_WINDOW_DIRECT matched=00000fff'),('after=00000c01','after=00000c00'),('stage=00000006','stage=00000005')]:
    assert a in window_capture
    assert not runner.evaluate_psram_window(window_capture.replace(a,b)),(a,b)
assert not runner.evaluate_psram_window(window_capture+window_capture)

# PSRAM DMA is a stricter profile; never accept CPU-only proof as DMA evidence.
dma_records=''
for i in range(9):
    dma_records+=f'PSRAM_DMA_COPY index={i:08x} offset={(0,65408,0x1ff000)[i//3]:08x} bytes=00001000 burst={(1,4,16)[i%3]:08x} done=00000001 polls=00000001 matched=00001000 guards=00000001 raw=000001ff status=00000000 common=00000000 us=00000100\n'
dma_records+='PSRAM_DMA_FINAL cfg=00000000 channels=00000000 fifo=00000000 restored=00000001 preserved=00000001\n'
dma_capture=window_capture.replace('PSRAM_WINDOW phase=00000002',dma_records+'PSRAM_WINDOW phase=00000002')
assert runner.evaluate_psram_dma(dma_capture)
assert not runner.evaluate_psram_dma(window_capture)
assert not runner.evaluate_psram_window(dma_capture)
for a,b in [('burst=00000010','burst=0000000f'),('matched=00001000','matched=00000fff'),('guards=00000001','guards=00000000'),('status=00000000','status=00000020'),('raw=000001ff','raw=00000200'),('polls=00000001','polls=00000000'),('restored=00000001 preserved=','restored=00000000 preserved='),('offset=0000ff80','offset=00010000')]:
    assert not runner.evaluate_psram_dma(dma_capture.replace(a,b)),(a,b)
assert not runner.evaluate_psram_dma(dma_capture+dma_records)

# Operational clock proof includes read-only four-phase DMA qualification.
for rate in (100,200):
    tune_reference='PSRAM_TUNE_REFERENCE result=00000000 matched=00080000 restored=00000001\n'
    fast_start=f'PSRAM_FAST_START result=00000000 rate={rate:08x} stage=00000005 attempts=0000001f polls=00000004 mpll=00000190 div=00000099 repeat=00000099 restored=00000001 power=01000000 clock=10000000 cal=00000300 peri=0000d00c bus0={0x30103 if rate==100 else 0x10001:08x} bus1={0x30103 if rate==100 else 0x10001:08x} dll0=00000021 dll1=00000021\n'
    phases=''.join(f'PSRAM_TUNE_PHASE phase={i:08x} result=00000000 copies={9 if i else 7:08x} matched={36864 if i else 36000:08x} restored=00000001\n' for i in range(4))+'PSRAM_TUNE_SELECT mask=0000000e best=00000001\n'
    fast_stop='PSRAM_FAST_STOP result=00000000 stage=00000005 attempts=0000001f restored=00000001 peri=0000c00c bus0=00010001 bus1=00010001 dll0=00000001 dll1=00000001\n'
    fast_capture=dma_capture.replace('PSRAM_WINDOW phase=00000000',tune_reference+fast_start+phases+'PSRAM_WINDOW phase=00000000').replace('PSRAM_WINDOW_RAW phase=00000001',fast_stop+'PSRAM_WINDOW_RAW phase=00000001')
    assert runner.evaluate_psram_fast(fast_capture,rate)
    assert not runner.evaluate_psram_fast(dma_capture,rate)
    assert not runner.evaluate_psram_dma(fast_capture)
    for a,b in [('mpll=00000190','mpll=00000168'),('repeat=00000099','repeat=00000091'),('cal=00000300','cal=00000200'),('peri=0000d00c','peri=0000c00c'),('attempts=0000001f','attempts=0000000f'),('restored=00000001','restored=00000000'),('matched=00001000','matched=00000fff'),('mask=0000000e best=00000001','mask=0000000e best=00000002')]:
        assert not runner.evaluate_psram_fast(fast_capture.replace(a,b),rate),(a,b)
    assert not runner.evaluate_psram_fast(fast_capture+fast_start,rate)
