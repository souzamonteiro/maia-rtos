#!/usr/bin/env python3
"""Load the isolated scheduler probe into RAM, capture its result, then reset.

Requires esptool 4.12/pyserial. No flash-write commands are used. Display/desktop
are unavailable during this short diagnostic; reset returns to the flash image.
"""
import argparse
import contextlib
from pathlib import Path
import sys
import time
import re


def evaluate_capture(output, expected_fatal=None, expected_pc=None, expected_stack=None,
                     expected_guard=None, expected_sp=None):
    if expected_fatal is None:
        return ('MAIA_NATIVE_PREEMPT_PASS' in output and
                'MAIA_NATIVE_PREEMPT_FAIL' not in output and 'MAIA_NATIVE_FATAL' not in output and
                'MAIA_STACK_FAULT' not in output and expected_stack is None)
    armed = re.findall(r'FAULT_ARMED pc=([0-9a-f]{8})', output)
    fatal = re.findall(r'MAIA_NATIVE_FATAL cause=([0-9a-f]{8}) pc=([0-9a-f]{8}) value=([0-9a-f]{8}) sp=([0-9a-f]{8})', output)
    if expected_pc is None:
        if len(armed) != 1:
            return False
        expected_pc = int(armed[0], 16)
    stack = re.findall(r'MAIA_STACK_FAULT reason=([1-5]) emergency=([01]) guard=([01])', output)
    stack_ok = ('MAIA_STACK_FAULT' not in output if expected_stack is None else
                len(stack) == 1 and int(stack[0][0]) == expected_stack and stack[0][1] == '1' and
                (expected_guard is None or int(stack[0][2]) == expected_guard))
    return (stack_ok and len(armed) <= 1 and len(fatal) == 1 and
            (int(fatal[0][0], 16) & 0x80000fff) == expected_fatal and
            int(fatal[0][1], 16) == expected_pc and
            (expected_sp is None or int(fatal[0][3], 16) == expected_sp) and
            (not armed or int(armed[0], 16) == expected_pc) and 'FAULT_RESUMED' not in output and
            'MAIA_NATIVE_PREEMPT_PASS' not in output and 'MAIA_NATIVE_PREEMPT_FAIL' not in output)


def evaluate_touch_identity(output):
    states = re.findall(r'^TOUCH_STATUS result_code=0 raw=(0|4)$', output, re.M)
    ids = re.findall(r'^TOUCH_ID reads=16 firmware=([13]) final=([13]) errors=0$', output, re.M)
    return (len(states) == 1 and len(ids) == 1 and ids[0][0] == ids[0][1] and
            output.split('\n').count('TOUCH_ABSENT address=0x56 NACK') == 1 and
            output.split('\n').count('TOUCH_INJECT sampled_SCL_low TIMEOUT') == 1 and
            output.split('\n').count('MAIA_NATIVE_TOUCH_PASS') == 1 and
            all(word not in output for word in ('FAIL', 'MAIA_NATIVE_FATAL', 'MAIA_STACK_FAULT')))


def evaluate_expander(output):
    banks = re.findall(r'^EXPANDER bank=(67|68) rounds=16 mirror=1 unchanged=1 dir=(\d+) out=(\d+)$', output, re.M)
    return (evaluate_touch_identity(output) and len(banks) == 2 and
            sorted(bank[0] for bank in banks) == ['67', '68'] and
            all(int(v) <= 255 for bank in banks for v in bank[1:]) and
            output.split('\n').count('EXPANDER_ABSENT_WRITE NACK') == 1)


def evaluate_panel_reset(output):
    result = re.findall(r'^PANEL_RESET stage=9 changed=1 preserved=1 elapsed_us=(\d+) gpio23_input=1$', output, re.M)
    return evaluate_expander(output) and len(result) == 1 and int(result[0]) >= 200000


def evaluate_mipi(output, allow_stop=False):
    """A complete read-only observation, never a power-off/idle certificate."""
    if not allow_stop and 'VDMA_STOP' in output:
        return False
    lines = output.replace('\r', '').splitlines()
    schemas = {
        'MIPI_SAMPLE': 'i cpu sys phy cpu_after sys_after phy_after valid stable'.split(),
        'MIPI_DSI': 'i bridge dpi fifo power mode reset status'.split(),
        'MIPI_CSI': 'i host shutdown reset stop bridge'.split(),
        'MIPI_DMA': 'i cfg enable abort'.split(),
        'MIPI_CH': 'i ch cfg0 cfg1 transferred fifo sar dar llp'.split(),
    }
    records = {key: {} for key in schemas}
    if lines.count('MAIA_MIPI_STATE_PROBE schema=1 read-only') != 1 or lines.count('MAIA_NATIVE_MIPI_PASS') != 1:
        return False
    if 'FAIL' in output or 'FATAL' in output:
        return False
    for line in lines:
        if not line.startswith('MIPI_'):
            continue
        parts = line.split()
        kind = parts[0]
        if kind not in schemas:
            return False
        pattern = kind + ''.join(' ' + key + r'=([0-9a-f]{8})' for key in schemas[kind])
        match = re.fullmatch(pattern, line)
        if not match:
            return False
        row = dict(zip(schemas[kind], (int(value, 16) for value in match.groups())))
        index = (row['i'], row['ch']) if kind == 'MIPI_CH' else row['i']
        if row['i'] >= 8 or (kind == 'MIPI_CH' and row['ch'] >= 4) or index in records[kind]:
            return False
        records[kind][index] = row
    if any(len(rows) != (32 if kind == 'MIPI_CH' else 8) for kind, rows in records.items()):
        return False
    for i, row in records['MIPI_SAMPLE'].items():
        expected = ((row['sys'] >> 12) & 1) | (((row['sys'] >> 11) & 1) << 1) | (((row['sys'] >> 10) & 1) << 2)
        if row['cpu'] & (1 << 13) and row['sys'] & (1 << 5):
            expected |= 8
        if row['valid'] != expected or row['stable'] != 1 or any(row[key] != row[key + '_after'] for key in ('cpu', 'sys', 'phy')):
            return False
        skipped = []
        if not expected & 1:
            skipped.append(records['MIPI_DSI'][i])
        csi = records['MIPI_CSI'][i]
        if not expected & 2 and any(csi[key] for key in ('host', 'shutdown', 'reset', 'stop')):
            return False
        if not expected & 4 and csi['bridge']:
            return False
        if not expected & 8:
            skipped += [records['MIPI_DMA'][i]] + [records['MIPI_CH'][(i, c)] for c in range(4)]
        if any(value for entry in skipped for key, value in entry.items() if key not in ('i', 'ch')):
            return False
    return True


def evaluate_mipi_park(output, allow_gate=False):
    if not allow_gate and 'MIPI_GATE' in output:
        return False
    lines = output.replace('\r', '').splitlines()
    if lines.count('MAIA_NATIVE_MIPI_PARK_PASS') != 1 or 'FAIL' in output or 'FATAL' in output:
        return False
    if sum(line.startswith('MIPI_PARK_SETUP ') for line in lines) != 1 or sum(line.startswith('MIPI_PARK ') for line in lines) != 1:
        return False
    setup = re.findall(r'^MIPI_PARK_SETUP sys_before=([0-9a-f]{8}) sys_ready=([0-9a-f]{8}) dsi_phy=([0-9a-f]{8}) csi_phy=([0-9a-f]{8}) armed=0000000f live=00000000$', output.replace('\r', ''), re.M)
    result = re.findall(r'^MIPI_PARK result=00000000 stage=00000004 attempted=0000000f dsi_bridge=([0-9a-f]{8}) csi_bridge=([0-9a-f]{8}) dsi_phy=([0-9a-f]{8}) csi_phy=([0-9a-f]{8}) preserved=00000001 duplicate_busy=00000001$', output.replace('\r', ''), re.M)
    if len(setup) != 1 or len(result) != 1:
        return False
    before, ready, dsi, csi = map(lambda s: int(s, 16), setup[0])
    db, cb, dp, cp = map(lambda s: int(s, 16), result[0])
    return (ready == before | 0x800 and ready & 0x1c20 == 0x1c20 and dsi & 15 == 2 and csi & 1 == 1 and
            not db & 1 and not cb & 1 and dp == dsi & ~14 and cp == csi & ~1)


def evaluate_mipi_gate(output, allow_vo3=False):
    if not allow_vo3 and 'VO3_' in output:
        return False
    if not evaluate_mipi_park(output, allow_gate=True):
        return False
    lines = output.replace('\r', '').splitlines()
    if lines.count('MAIA_NATIVE_MIPI_GATE_PASS') != 1:
        return False
    for key in ('MIPI_ISP ', 'MIPI_ISP_STATE ', 'MIPI_GATE '):
        if sum(line.startswith(key) for line in lines) != 1:
            return False
    isp = re.findall(r'^MIPI_ISP clock=([0-9a-f]{8}) reset=([0-9a-f]{8}) norst=([0-9a-f]{8})$', output.replace('\r', ''), re.M)
    state = re.findall(r'^MIPI_ISP_STATE inspected=([01]{8}) control=([0-9a-f]{8}) camera=([0-9a-f]{8})$', output.replace('\r', ''), re.M)
    gate = re.findall(r'^MIPI_GATE result=00000000 stage=00000005 attempted=([0-9a-f]{8}) reset_before=([0-9a-f]{8}) reset_after=([0-9a-f]{8}) phy_before=([0-9a-f]{8}) phy_after=([0-9a-f]{8}) sys_before=([0-9a-f]{8}) sys_after=([0-9a-f]{8}) preserved=00000001 duplicate_busy=00000001$', output.replace('\r', ''), re.M)
    if len(isp) != 1 or len(gate) != 1 or len(state) != 1:
        return False
    clock, reset, norst = (int(s, 16) for s in isp[0])
    inspected, control, camera = (int(s, 16) for s in state[0])
    readable = bool(clock & (1 << 25)) and not bool(reset & (1 << 29))
    if inspected != int(readable) or (readable and (control & 0x18000001 or camera & 7 != 4)) or (not readable and (control or camera)):
        return False
    attempted, rb, ra, pb, pa, sb, sa = (int(s, 16) for s in gate[0])
    ready = re.search(r'^MIPI_PARK_SETUP .* sys_ready=([0-9a-f]{8}) ', output.replace('\r', ''), re.M)
    return (not norst & 0x7800 and rb == reset and ra == rb | 0x3c000000 and
            attempted == (6 if ra == rb else 7) and pb & 0x11 == 0x11 and pa == pb & ~0x93 and
            ready is not None and sb == int(ready[1], 16) and sb & 0x1c20 == 0x1c20 and sa == sb & ~0x1c00)


def evaluate_vo3_cycle(output, allow_dsi=False):
    if not allow_dsi and ('DSI_ACCESS' in output or 'DSI_CONFIG' in output):
        return False
    if not evaluate_mipi_gate(output, allow_vo3=True):
        return False
    lines = output.replace('\r', '').splitlines()
    schemas = {'VO3_CSI_PREP': 'sys_before sys_after phy_before phy_after reset_before reset_after'.split(), 'VO3_PLAN': 'dref mul defaults control analog'.split(),
               'VO3_OFF': 'control analog'.split(), 'VO3_ON': 'control analog'.split(),
               'VO3_ACQUIRE': 'stage adopted changed preserved duplicate_busy samples'.split()}
    rows = {}
    for line in lines:
        if not line.startswith('VO3_') or line in ('VO3_CYCLE_OFF_READY', 'VO3_CYCLE_ON_READY'):
            continue
        kind = line.split()[0]
        if kind not in schemas or kind in rows:
            return False
        match = re.fullmatch(kind + ''.join(' '+key+r'=([0-9a-f]{8})' for key in schemas[kind]),line)
        if not match:
            return False
        rows[kind] = dict(zip(schemas[kind], (int(v,16) for v in match.groups())))
    if rows.keys() != schemas.keys() or lines.count('MAIA_NATIVE_VO3_CYCLE_PASS') != 1:
        return False
    for marker in ('VO3_CYCLE_OFF_READY', 'VO3_CYCLE_ON_READY'):
        if lines.count(marker) != 1:
            return False
    if not (lines.index('VO3_CYCLE_OFF_READY') < lines.index('VO3_CYCLE_ON_READY') < lines.index('MAIA_NATIVE_VO3_CYCLE_PASS')):
        return False
    prep, plan, off, on, acquired = (rows[key] for key in schemas)
    park_start = re.search(r'^MIPI_PARK_SETUP sys_before=([0-9a-f]{8}) ',output.replace('\r',''),re.M)
    gate_phy = re.search(r'^MIPI_GATE .* phy_before=([0-9a-f]{8}) ',output.replace('\r',''),re.M)
    if not (prep['sys_before'] & 0x1020 == 0x1020 and prep['phy_before'] & 1 and
            prep['sys_after'] == prep['sys_before'] | 0xc00 and prep['phy_after'] == prep['phy_before'] | 0x10 and prep['reset_after'] == prep['reset_before'] & ~0x18000000 and
            park_start and int(park_start[1],16) == prep['sys_after'] and
            gate_phy and int(gate_phy[1],16) == prep['phy_after']):
        return False
    if not (0 <= plan['dref'] <= 15 and 0 <= plan['mul'] <= 7 and 0 <= plan['defaults'] <= 7 and
            plan['control'] & 0x4f80 == 0x180 and
            plan['analog'] & 0xf7800000 == (plan['dref'] << 28) | (plan['mul'] << 23) | (1 << 26)):
        return False
    return (off['control'] == plan['control'] & ~0x100 and off['analog'] == plan['analog'] and
            on['control'] == plan['control'] and on['analog'] == plan['analog'] and
            acquired == dict(stage=5, adopted=0, changed=1, preserved=1, duplicate_busy=1, samples=16))


def evaluate_dsi_access(output, allow_reference=False):
    if not allow_reference and 'DSI_REFERENCE' in output:
        return False
    if not evaluate_vo3_cycle(output, allow_dsi=True):
        return False
    clean = output.replace('\r','')
    schema = 'result stage attempted sys_before sys_after phy_before phy_after reset_before reset_after control analog duplicate_busy'.split()
    rows = re.findall(r'^DSI_ACCESS' + ''.join(' '+k+r'=([0-9a-f]{8})' for k in schema) + '$',clean,re.M)
    configs = re.findall(r'^DSI_CONFIG version=([0-9a-f]{8}) power=([0-9a-f]{8}) phy_reset=([0-9a-f]{8}) bridge=([0-9a-f]{8}) dpi=([0-9a-f]{8}) fifo=([0-9a-f]{8}) status=([0-9a-f]{8})$',clean,re.M)
    lines = clean.splitlines()
    if len(rows) != 1 or len(configs) != 1 or sum(l.startswith('DSI_ACCESS') for l in lines) != 1 or sum(l.startswith('DSI_CONFIG') for l in lines) != 1 or lines.count('MAIA_NATIVE_DSI_ACCESS_PASS') != 1:
        return False
    if not (lines.index('MAIA_NATIVE_VO3_CYCLE_PASS') < next(i for i,l in enumerate(lines) if l.startswith('DSI_ACCESS ')) < next(i for i,l in enumerate(lines) if l.startswith('DSI_CONFIG ')) < lines.index('MAIA_NATIVE_DSI_ACCESS_PASS')):
        return False
    a = dict(zip(schema,(int(v,16) for v in rows[0])))
    gate = re.search(r'^MIPI_GATE .*reset_after=([0-9a-f]{8}) .*phy_after=([0-9a-f]{8}) .*sys_after=([0-9a-f]{8}) ',clean,re.M)
    supply = re.search(r'^VO3_ON control=([0-9a-f]{8}) analog=([0-9a-f]{8})$',clean,re.M)
    _,power,phy,bridge,dpi,fifo,_ = (int(v,16) for v in configs[0])
    return (a['result'] == 0 and a['stage'] == 5 and a['attempted'] == 7 and a['duplicate_busy'] == 1 and
            gate and [a['reset_before'],a['phy_before'],a['sys_before']] == [int(v,16) for v in gate.groups()] and
            a['sys_after'] == a['sys_before'] | 0x1000 and a['phy_after'] == a['phy_before'] | 1 and
            a['reset_after'] == a['reset_before'] & ~0x4000000 and
            supply and [a['control'],a['analog']] == [int(v,16) for v in supply.groups()] and
            not power & 1 and not phy & 15 and not bridge & 1 and not dpi & 1 and not fifo & 0x3fff)


def evaluate_dsi_reference(output):
    if not evaluate_dsi_access(output, allow_reference=True):
        return False
    clean=output.replace('\r','');lines=clean.splitlines()
    keys='result stage attempted divider lp_gate provider_before provider_after source_before source_after phy_before phy_after nominal_hz preserved duplicate_busy samples'.split()
    rows=re.findall(r'^DSI_REFERENCE'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
    states=re.findall(r'^DSI_REFERENCE_STATE power=([0-9a-f]{8}) phy_reset=([0-9a-f]{8}) bridge=([0-9a-f]{8}) dpi=([0-9a-f]{8}) fifo=([0-9a-f]{8}) status=([0-9a-f]{8})$',clean,re.M)
    if len(rows)!=1 or len(states)!=1 or sum(l.startswith('DSI_REFERENCE ') for l in lines)!=1 or sum(l.startswith('DSI_REFERENCE_STATE') for l in lines)!=1 or lines.count('MAIA_NATIVE_DSI_REFERENCE_PASS')!=1:
        return False
    if not (lines.index('MAIA_NATIVE_DSI_ACCESS_PASS') < next(i for i,l in enumerate(lines) if l.startswith('DSI_REFERENCE ')) < next(i for i,l in enumerate(lines) if l.startswith('DSI_REFERENCE_STATE ')) < lines.index('MAIA_NATIVE_DSI_REFERENCE_PASS')):
        return False
    a=dict(zip(keys,(int(v,16) for v in rows[0])))
    access_phy=re.search(r'^DSI_ACCESS .*phy_after=([0-9a-f]{8}) ',clean,re.M)
    expected=16 | (1 if not a['provider_before']&0x100 else 0) | (14 if a['source_before']&0xc0000000 else 0)
    power,phy,bridge,dpi,fifo,_=(int(v,16) for v in states[0])
    return (a['result']==0 and a['stage']==7 and a['attempted']==expected and
            a['divider']>>16&255==23 and a['lp_gate']&0x0a000000==0x0a000000 and
            a['provider_after']==a['provider_before']|0x100 and a['source_after']==a['source_before']&~0xc0000000 and
            access_phy and int(access_phy[1],16)==a['phy_before'] and a['phy_after']==a['phy_before']|2 and
            a['nominal_hz']==20000000 and a['preserved']==1 and a['duplicate_busy']==1 and a['samples']==16 and
            not power&1 and not phy&15 and not bridge&1 and not dpi&1 and not fifo&0x3fff)


def evaluate_vdma_transfer(output):
    """Require real copy, an enabled channel inside stop, and a partial stable result."""
    lines = output.replace('\r', '').splitlines()
    if lines.count('MAIA_VDMA_TRANSFER_PROBE schema=1 owned-sram') != 1 or lines.count('MAIA_NATIVE_VDMA_TRANSFER_PASS') != 1 or 'FAIL' in output or 'FATAL' in output:
        return False
    schemas = {
        'DMA_MEMORY': 'start end alias bytes cache_line prepared'.split(),
        'DMA_COPY': 'matched status common'.split(),
        'DMA_STOP': 'result active initial_cfg initial_channels requested stage polls cfg channels abort fifo'.split(),
        'DMA_VERIFY': 'copied bad guards stable preserved duplicate_busy status common'.split(),
    }
    rows = {}
    for line in lines:
        if not line.startswith('DMA_'):
            continue
        kind = line.split()[0]
        if kind not in schemas or kind in rows:
            return False
        match = re.fullmatch(kind + ''.join(' ' + key + r'=([0-9a-f]{8})' for key in schemas[kind]), line)
        if not match:
            return False
        rows[kind] = dict(zip(schemas[kind], (int(value, 16) for value in match.groups())))
    if rows.keys() != schemas.keys():
        return False
    mem, copy, stop, verify = (rows[key] for key in schemas)
    if not (0x4ff00000 <= mem['start'] < mem['end'] <= 0x4ff28000 and mem['start'] % 128 == 0 and
            mem['end'] - mem['start'] == 66048 and mem['alias'] == mem['start'] + 0x40000000 and
            mem['bytes'] == 32768 and mem['cache_line'] in (16, 32, 64, 128) and mem['prepared'] == 1):
        return False
    if copy['matched'] != 32768 or copy['status'] & 0x023f7fe0 or copy['common']:
        return False
    expected = dict(result=0, active=1, initial_cfg=1, initial_channels=1, requested=1,
                    stage=3, cfg=0, channels=0, abort=0)
    if any(stop[key] != value for key, value in expected.items()) or not 1 <= stop['polls'] <= 4096 or stop['fifo'] > 15:
        return False
    return (0 < verify['copied'] < 32768 and verify['bad'] == 0 and
            all(verify[key] == 1 for key in ('guards', 'stable', 'preserved', 'duplicate_busy')) and
            not verify['status'] & 0x023f7fe0 and verify['common'] == 0)


def evaluate_vdma_stop(output, armed=False):
    if not evaluate_mipi(output, allow_stop=True):
        return False
    lines = output.replace('\r', '').splitlines()
    if lines.count('MAIA_VDMA_STOP_PROBE optional empty-controller writes; no transfers') != 1:
        return False
    expected = ('VDMA_STOP armed=' + ('00000001' if armed else '00000000') +
                ' stage=00000003 adopted=' + ('00000000' if armed else '00000001') +
                ' changed=' + ('00000001' if armed else '00000000') +
                ' requested=00000000 polls=00000001 cfg=00000000 channels=00000000 abort=00000000'
                ' fifo=00000000 preserved=00000001 duplicate_busy=00000001')
    if len([line for line in lines if line.startswith('VDMA_STOP')]) != 1 or expected not in lines:
        return False
    # The actual diagnostic requires the controller/channels initially empty.
    for line in lines:
        if line.startswith('MIPI_SAMPLE '):
            if int(re.search(r' valid=([0-9a-f]{8})', line)[1], 16) & 8 == 0:
                return False
        if line.startswith('MIPI_DMA '):
            if not line.endswith('cfg=00000000 enable=00000000 abort=00000000'):
                return False
        if line.startswith('MIPI_CH '):
            if any(int(re.search(' ' + key + r'=([0-9a-f]{8})', line)[1], 16)
                   for key in ('transferred', 'fifo', 'sar', 'dar', 'llp')):
                return False
    return True


def evaluate_ldo3(output):
    version = re.findall(r'^LDO3_VERSION chip_major=1 chip_minor=3 block_major=0 block_minor=([1-7]) disabled=0 rs_fail=0 corrected=([0-7])$', output, re.M)
    trim = re.findall(r'^LDO3_TRIM k=(\d+) vos=(\d+) c=(\d+)$', output, re.M)
    plan = re.findall(r'^LDO3_PLAN target_mv=2500 dref=(\d+) mul=(\d+) default_mask=(\d+) estimated_uv=(\d+) error_uv=(\d+)$', output, re.M)
    coeff = re.findall(r'^LDO3_COEFF k1000=(-?\d+) vos1000=(-?\d+) c1000=(-?\d+)$', output, re.M)
    current = re.findall(r'^LDO3_CURRENT enabled=[01] sw_owner=[01] bypass=[01] source=[0-7] dref=(\d+) mul=([0-7]) vdet=[01]$', output, re.M)
    if not all(len(x) == 1 for x in (version, trim, plan, coeff, current)):
        return False
    k, v, c = map(int, trim[0])
    if k>255 or v>63 or c>63 or int(current[0][0])>15:
        return False
    def signed(raw, sign):
        return -(raw-sign) if raw & sign else raw
    K, V, C = 975+signed(k,128) if k else 1000, -3+signed(v,32) if v else 0, 990+signed(c,32) if c else 1000
    candidates = []
    for d in range(16):
        ref = 10+d if d<9 else 2*d+2
        for m in range(8):
            predicted = (ref*K+20*V)*(4000+m*C)
            candidates.append((abs(200000000-predicted), d, m, predicted))
    error, dref, mul, predicted = min(candidates)
    expected = (dref, mul, (not k)*1+(not v)*2+(not c)*4, (predicted+40)//80, (error+40)//80)
    return (tuple(map(int, plan[0])) == expected and tuple(map(int, coeff[0])) == (K,V,C) and
            output.split('\n').count('LDO3_READONLY samples=16 unchanged=1 errors=0') == 1 and
            output.split('\n').count('MAIA_NATIVE_LDO3_PASS') == 1 and
            all(word not in output for word in ('FAIL','MAIA_NATIVE_FATAL','MAIA_STACK_FAULT')))


def evaluate_ldo3_acquire(output, cycle=False):
    records = re.findall(r'^LDO3_ACQUIRE stage=5 adopted=([01]) changed=([01]) preserved=1 duplicate_busy=1$', output, re.M)
    clocks = re.findall(r'^LDO3_CLOCKS sys_active=([01]) phy_active=([01])$', output, re.M)
    expected = ('0', '1') if cycle else ('1', '0')
    marker = output.split('\n').count('LDO3_CYCLE off_confirmed=1 clocks_gated=1')
    current = re.findall(r'^LDO3_CURRENT enabled=1 sw_owner=1 bypass=0 source=0 dref=(\d+) mul=(\d+) vdet=1$', output, re.M)
    planned = re.findall(r'^LDO3_PLAN target_mv=2500 dref=(\d+) mul=(\d+) ', output, re.M)
    return (evaluate_ldo3(output) and records == [expected] and len(clocks) == 1 and
            len(current) == 1 and current == planned and
            (not cycle or clocks == [('0', '0')]) and marker == int(cycle))


def serial_output_record(source, sequence):
    prefix = 'TXR:' + chr(ord('A') + source) + ':' + f'{sequence:02d}' + ':'
    return prefix + ''.join(chr(ord('a') + (source*7 + sequence + i) % 26)
                            for i in range(9, 47)) + '\n'


def evaluate_serial_output(output):
    if not evaluate_capture(output) or 'LEGACY_MUST_NOT_APPEAR' in output:
        return False
    records = [line for line in output.split('\n') if 'TXR:' in line]
    if len(records) != 80:
        return False
    counts = [0, 0]
    for line in records:
        source = 0 if line.startswith('TXR:A:') else 1 if line.startswith('TXR:B:') else -1
        if source < 0 or counts[source] >= 40 or line+'\n' != serial_output_record(source, counts[source]):
            return False
        counts[source] += 1
    totals = re.findall(r'^TX_SUM records=80 bytes=3840 errors=0$', output, re.M)
    hardware = re.findall(r'^TX_HW irq=(\d+) full=(\d+) busy=(\d+)$', output, re.M)
    return (counts == [40, 40] and len(totals) == 1 and len(hardware) == 1 and
            int(hardware[0][0]) >= 250 and int(hardware[0][1]) > 0 and int(hardware[0][2]) > 0)


def evaluate_serial_output_event(output):
    if not evaluate_serial_output(output):
        return False
    events = re.findall(r'^TX_EVENT head=(\d+) tail=(\d+) idle=(\d+)$', output, re.M)
    waits = re.findall(r'^TX_WAIT empty=(\d+) retry=(\d+) timeout=(\d+) irq_write=(\d+)$', output, re.M)
    return (output.split('\n').count('TX_EVENT_TRIGGER') == 1 and len(events) == len(waits) == 1 and
            int(events[0][0]) >= 20 and int(events[0][1]) >= 20 and int(events[0][2]) >= 39 and
            int(waits[0][0]) >= 2 and int(waits[0][1]) > 0 and int(waits[0][2]) > 0 and int(waits[0][3]) == 1)


class Vo3CycleStimulus:
    """Host observation windows only, not rail discharge/settling specifications."""
    def __init__(self, port, pause=time.sleep, now=time.monotonic):
        self.port, self.pause, self.now = port, pause, now
        self.phase, self.holds = 0, []

    def feed(self, output):
        clean = output.replace('\r', '')
        for index, marker, ack, name in ((0, 'VO3_CYCLE_OFF_READY', b'O', 'off'),
                                          (1, 'VO3_CYCLE_ON_READY', b'N', 'on')):
            if self.phase == index and marker+'\n' in clean:
                start = self.now()
                self.pause(0.020)
                elapsed = self.now()-start
                if elapsed < 0.020:
                    raise RuntimeError('short VO3 observation window')
                if self.port.write(ack) != 1:
                    raise RuntimeError('short VO3 acknowledgement write')
                self.port.flush()
                self.holds.append(elapsed)
                self.phase += 1
                print('[HOST] VO3 observation '+name+' ms='+format(elapsed*1000,'.3f'))


class SerialInputStimulus:
    """Two paced binary batches, armed only by the RAM serial probe markers."""
    def __init__(self, port, pause=time.sleep):
        self.port, self.pause, self.phase = port, pause, 0

    def feed(self, output):
        clean = output.replace('\r', '')
        for phase, marker, first, last in ((0, 'fill', 0, 64), (1, 'drain', 64, 256)):
            if self.phase == phase and 'MAIA_SERIAL_RX_READY ' + marker + '\n' in clean:
                for offset in range(first, last, 16):
                    data = bytes(range(offset, min(offset + 16, last)))
                    if self.port.write(data) != len(data):
                        raise RuntimeError('short serial fixture write')
                    self.pause(0.005)
                self.phase += 1
                print('[HOST] Serial RX ' + marker + ' sent=' + str(last-first))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', default='/dev/cu.usbmodem1301')
    parser.add_argument('--image', type=Path, required=True)
    parser.add_argument('--log', type=Path, required=True)
    parser.add_argument('--capture-seconds', type=float, default=8)
    parser.add_argument('--expect-stack-reason', type=int, choices=(1,2,3,4,5))
    parser.add_argument('--expect-stack-guard', type=int, choices=(0,1))
    parser.add_argument('--expect-stack-sp', type=lambda value: int(value,0))
    parser.add_argument('--expect-fatal-pc', type=lambda value: int(value, 0))
    parser.add_argument('--expect-fatal-cause', type=lambda value: int(value, 0), choices=(2, 3, 11))
    parser.add_argument('--validate-only', action='store_true')
    parser.add_argument('--dsi-reference', action='store_true', help='prepare guarded nominal20MHz DSI PHY reference, no PLL initialization')
    parser.add_argument('--dsi-access', action='store_true', help='verify guarded DSI bus/config access, no PLL or lane activation')
    parser.add_argument('--vo3-cycle', action='store_true', help='guarded VO3 disable/native acquire with host observation windows')
    parser.add_argument('--mipi-gate', action='store_true', help='verify held shared resets and boot MIPI clock gating')
    parser.add_argument('--mipi-park', action='store_true', help='verify reset-state bridge/PHY parking, no live streams')
    parser.add_argument('--vdma-transfer', action='store_true', help='verify owned SRAM DMA copy and active-channel stop')
    parser.add_argument('--vdma-stop', action='store_true', help='verify stopped VDMA adoption')
    parser.add_argument('--vdma-arm-empty', action='store_true', help='arm empty VDMA and verify native global stop; no transfers')
    parser.add_argument('--mipi-state', action='store_true', help='verify read-only inherited MIPI/DW-GDMA state')
    parser.add_argument('--ldo3-acquire', action='store_true', help='verify boot LDO3 active-state adoption')
    parser.add_argument('--ldo3-cycle', action='store_true', help='verify guarded VO3 disable/enable through native acquire')
    parser.add_argument('--ldo3', action='store_true', help='verify read-only VO3 trim/planning and unchanged hardware')
    parser.add_argument('--panel-reset', action='store_true', help='verify LCD/touch reset, preserved bits and touch recovery')
    parser.add_argument('--expander', action='store_true', help='verify both expanders and same-value writes, including touch regression')
    parser.add_argument('--touch-identify', action='store_true', help='verify boot-only touch identity and negative probes')
    parser.add_argument('--serial-output-event', action='store_true', help='verify TX empty waits and IRQ enqueue wakeup')
    parser.add_argument('--serial-output', action='store_true', help='verify 80 complete TX records from two producers')
    parser.add_argument('--serial-input', action='store_true', help='send binary RX fixture after RAM probe markers')
    args = parser.parse_args()
    if not 1 <= args.capture_seconds <= 30:
        parser.error('capture-seconds must be between 1 and 30')

    if (args.expect_fatal_cause is None) != (args.expect_fatal_pc is None):
        parser.error('fatal tests require both expected cause and PC from the test ELF symbol')

    if args.expect_stack_reason is not None and args.expect_fatal_cause is None:
        parser.error('stack checks require expected fatal cause and PC')
    if (args.expect_stack_guard is not None or args.expect_stack_sp is not None) and args.expect_stack_reason is None:
        parser.error('guard/SP expectations require a stack reason')
    if args.expect_stack_sp is not None and not 0 <= args.expect_stack_sp <= 0xffffffff:
        parser.error('expected source SP must fit RV32')

    if args.dsi_reference:
        args.dsi_access = True
    if args.dsi_access:
        args.vo3_cycle = True
    if args.vo3_cycle:
        args.mipi_gate = True
    if args.mipi_gate:
        args.mipi_park = True
    if args.mipi_park:
        if args.vdma_transfer or args.vdma_stop or args.vdma_arm_empty:
            parser.error('MIPI park excludes VDMA workloads')
        args.mipi_state = True
    if args.vdma_transfer:
        if args.vdma_arm_empty:
            parser.error('VDMA transfer excludes empty-arm profile')
        args.vdma_stop = True
    if args.vdma_arm_empty:
        args.vdma_stop = True
    if args.vdma_stop:
        args.mipi_state = True
    if args.mipi_state and (args.ldo3 or args.ldo3_acquire or args.ldo3_cycle or args.panel_reset or
                            args.expander or args.touch_identify or args.serial_output_event or
                            args.serial_output or args.serial_input or args.expect_fatal_cause is not None):
        parser.error('MIPI state requires a separate read-only profile')
    if args.ldo3_cycle:
        args.ldo3_acquire = True
    if args.ldo3_acquire:
        args.ldo3 = True
    if args.ldo3 and (args.panel_reset or args.expander or args.touch_identify or args.serial_output_event or
                      args.serial_output or args.serial_input or args.expect_fatal_cause is not None):
        parser.error("LDO3 requires a separate read-only profile")
    if args.panel_reset:
        args.expander = True
    if args.expander:
        args.touch_identify = True
    if args.touch_identify and (args.serial_output_event or args.serial_output or args.serial_input or
                                args.expect_fatal_cause is not None):
        parser.error("touch probe cannot be combined with scheduler/serial profiles")
    if args.serial_output_event:
        args.serial_output = True
    if args.serial_output and (args.serial_input or args.expect_fatal_cause is not None):
        parser.error('serial output workload cannot be combined with input/fatal profiles')
    if args.serial_input and args.expect_fatal_cause is not None:
        parser.error('serial input workload cannot be combined with a fatal profile')

    import esptool
    from esptool.bin_image import LoadFirmwareImage
    image = LoadFirmwareImage('esp32p4', str(args.image))
    # Existing linker.ld L2MEM_LOW window: validate before opening any port.
    if not image.segments or not all(
        0x4FF00000 <= segment.addr < segment.addr + len(segment.data) <= 0x4FF28000
        for segment in image.segments
    ) or not any(segment.addr <= image.entrypoint < segment.addr + len(segment.data)
                 for segment in image.segments):
        parser.error('image is outside the existing L2MEM_LOW RAM-load window')
    if args.expect_fatal_pc is not None and (args.expect_fatal_pc & 1 or not any(
        segment.addr <= args.expect_fatal_pc < segment.addr + len(segment.data)
        for segment in image.segments
    )):
        parser.error('expected fault PC must be halfword-aligned inside the loaded image')
    if args.validate_only:
        print('PASS: image and entry point confined to L2MEM_LOW; no device accessed')
        return 0

    class Tee:
        def __init__(self, logfile):
            self.logfile = logfile

        def write(self, text):
            self.logfile.write(text)
            self.logfile.flush()
            sys.__stdout__.write(text)
            sys.__stdout__.flush()

        def flush(self):
            self.logfile.flush()
            sys.__stdout__.flush()

    output = ''
    with args.log.open('w') as logfile, contextlib.redirect_stdout(Tee(logfile)):
        esp = esptool.get_default_connected_device(
            [args.port], port=args.port, connect_attempts=3,
            initial_baud=115200, chip='esp32p4')
        try:
            esptool.main(['--chip', 'esp32p4', '--no-stub', 'load_ram', str(args.image)], esp=esp)
            port = esp._port
            port.timeout = 0.1
            if args.mipi_park:
                if port.write(b'P') != 1:
                    raise RuntimeError('short MIPI park host-ready write')
                port.flush()
            stimulus = Vo3CycleStimulus(port) if args.vo3_cycle else SerialInputStimulus(port) if args.serial_input else None
            deadline = time.monotonic() + args.capture_seconds
            while time.monotonic() < deadline:
                data = port.read(8192)
                if data:
                    chunk = data.decode('utf-8', 'replace')
                    output += chunk
                    print(chunk, end='')
                    if stimulus is not None:
                        stimulus.feed(output)
        finally:
            print('\n[HOST] Resetting to the preserved flash application')
            try:
                esp.hard_reset()
            finally:
                esp._port.close()
        passed = evaluate_capture(output, args.expect_fatal_cause, args.expect_fatal_pc,
                                  args.expect_stack_reason, args.expect_stack_guard, args.expect_stack_sp)
        if args.touch_identify:
            passed = evaluate_touch_identity(output.replace("\r", ""))
        if args.expander:
            passed = evaluate_expander(output.replace("\r", ""))
        if args.panel_reset:
            passed = evaluate_panel_reset(output.replace("\r", ""))
        if args.dsi_reference:
            passed = evaluate_dsi_reference(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dsi_access:
            passed = evaluate_dsi_access(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.vo3_cycle:
            passed = evaluate_vo3_cycle(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.mipi_gate:
            passed = evaluate_mipi_gate(output)
        elif args.mipi_park:
            passed = evaluate_mipi_park(output)
        elif args.vdma_transfer:
            passed = evaluate_vdma_transfer(output)
        elif args.vdma_stop:
            passed = evaluate_vdma_stop(output, args.vdma_arm_empty)
        elif args.mipi_state:
            passed = evaluate_mipi(output)
        if args.ldo3:
            passed = evaluate_ldo3(output.replace("\r", ""))
        if args.ldo3_acquire:
            passed = evaluate_ldo3_acquire(output.replace("\r", ""), args.ldo3_cycle)
        if args.serial_input:
            passed = passed and stimulus.phase == 2
        if args.serial_output:
            passed = passed and evaluate_serial_output(output)
        if args.serial_output_event:
            passed = passed and evaluate_serial_output_event(output)
        print('[HOST] DSI reference:' if args.dsi_reference else '[HOST] DSI config access:' if args.dsi_access else '[HOST] Guarded VO3 cycle:' if args.vo3_cycle else '[HOST] MIPI clock gating:' if args.mipi_gate else '[HOST] MIPI reset parking:' if args.mipi_park else '[HOST] Active VDMA transfer:' if args.vdma_transfer else '[HOST] VDMA stop:' if args.vdma_stop else '[HOST] MIPI observation:' if args.mipi_state else '[HOST] LDO3 planning:' if args.ldo3 else '[HOST] Touch identity:' if args.touch_identify else '[HOST] Expected fatal trap:' if args.expect_fatal_cause is not None else '[HOST] Native preemption:', 'PASS' if passed else 'FAIL / NOT DEMONSTRATED')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
