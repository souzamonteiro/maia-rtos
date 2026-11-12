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
