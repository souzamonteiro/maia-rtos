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


def evaluate_sdmmc_state(output):
    """Validate a read-only observation, not an SD transfer or readiness."""
    lines = output.replace('\r', '').splitlines()
    if (lines.count('MAIA_SDMMC_STATE_PROBE schema=1 read-only') != 1 or
            lines.count('MAIA_NATIVE_SDMMC_STATE_PASS') != 1 or
            any(word in output for word in ('FAIL', 'MAIA_NATIVE_FATAL', 'MAIA_STACK_FAULT'))):
        return False
    schemas = {
        'SDMMC_SAMPLE': 'i sys clock div reset ldo analog sys_after clock_after reset_after ldo_after analog_after valid stable'.split(),
        'SDMMC_PINS': 'i d0 d1 d2 d3 clk cmd'.split(),
        'SDMMC_HOST': 'i ctrl clkdiv clksrc clkena tmout ctype cmd rint status cdetect fifoth bmod verid hcon'.split(),
    }
    rows = {kind: {} for kind in schemas}
    observed_order = []
    for line in lines:
        if not line.startswith('SDMMC_'):
            continue
        kind = line.split()[0]
        if kind not in schemas:
            return False
        match = re.fullmatch(kind + ''.join(' ' + key + r'=([0-9a-f]{8})' for key in schemas[kind]), line)
        if match is None:
            return False
        row = dict(zip(schemas[kind], (int(value, 16) for value in match.groups())))
        index = row['i']
        if index >= 8 or index in rows[kind]:
            return False
        rows[kind][index] = row
        observed_order.append((kind, index))
    expected_order = [(kind, index) for index in range(8) for kind in schemas]
    if observed_order != expected_order:
        return False
    for index in range(8):
        sample = rows['SDMMC_SAMPLE'][index]
        if sample['stable'] != 1 or any(sample[key] != sample[key + '_after']
                                       for key in ('sys', 'clock', 'reset', 'ldo', 'analog')):
            return False
        valid = bool(sample['sys'] & (1 << 14)) and not bool(sample['reset'] & (1 << 28))
        if sample['valid'] != int(valid):
            return False
        if not valid and any(value for key, value in rows['SDMMC_HOST'][index].items() if key != 'i'):
            return False
    return True


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


def evaluate_dsi_reference(output, allow_phy=False):
    if not allow_phy and ('DSI_PHY ' in output or 'DSI_PHY_PASS' in output):
        return False
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


def evaluate_dsi_phy(output, allow_command=False):
    if not allow_command and 'DSI_COMMAND' in output:
        return False
    if not evaluate_dsi_reference(output,allow_phy=True):
        return False
    clean=output.replace('\r','');lines=clean.splitlines()
    keys='result stage writes bytes polls status power phy_reset lanes_before lanes mbps preserved duplicate_busy samples'.split()
    rows=re.findall(r'^DSI_PHY'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
    if len(rows)!=1 or sum(l.startswith('DSI_PHY ') for l in lines)!=1 or lines.count('MAIA_NATIVE_DSI_PHY_PASS')!=1:
        return False
    if not (lines.index('MAIA_NATIVE_DSI_REFERENCE_PASS') < next(i for i,l in enumerate(lines) if l.startswith('DSI_PHY ')) < lines.index('MAIA_NATIVE_DSI_PHY_PASS')):
        return False
    a=dict(zip(keys,(int(v,16) for v in rows[0])))
    return (a['result']==0 and a['stage']==11 and a['writes']==40+(a['lanes_before']&3!=1) and a['bytes']==5 and
            1<=a['polls']<=100000 and a['status']&0x95==0x95 and a['power']&1==1 and a['phy_reset']&15==15 and
            a['lanes']==(a['lanes_before']&~3)|1 and a['mbps']==960 and a['preserved']==1 and a['duplicate_busy']==1 and a['samples']==16)


def evaluate_dsi_command(output, allow_short=False):
    if not allow_short and 'DSI_SHORT' in output:
        return False
    if not evaluate_dsi_phy(output,allow_command=True):
        return False
    clean=output.replace('\r','');lines=clean.splitlines()
    keys='result stage attempted status fifo escape_hz timeout_hz preserved duplicate_busy samples'.split()
    rows=re.findall(r'^DSI_COMMAND'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
    words=re.findall(r'^DSI_COMMAND_WORD index=([0-9a-f]{8}) before=([0-9a-f]{8}) after=([0-9a-f]{8})$',clean,re.M)
    if len(rows)!=1 or len(words)!=14 or sum(l.startswith('DSI_COMMAND ') for l in lines)!=1 or sum(l.startswith('DSI_COMMAND_WORD') for l in lines)!=14 or lines.count('MAIA_NATIVE_DSI_COMMAND_PASS')!=1:
        return False
    positions=[i for i,l in enumerate(lines) if l.startswith('DSI_COMMAND_WORD ')]
    if not (lines.index('MAIA_NATIVE_DSI_PHY_PASS') < next(i for i,l in enumerate(lines) if l.startswith('DSI_COMMAND ')) < positions[0] and positions[-1] < lines.index('MAIA_NATIVE_DSI_COMMAND_PASS')):
        return False
    masks=[1,0xffff,0x03ff03ff,0x03ff03ff,0x39,0xffffffff,0xffff,0xffff,0xffff,0xffff,0xffff,0x7fff,0xff00,3]
    values=[1,0x0c07,0x00320068,0x002e0080,0x19,0,0,0,0,0,0,6000,0x3f00,0]
    attempted=0
    for expected,(index,before,after),mask,value in zip(range(14),words,masks,values):
        index,before,after=(int(v,16) for v in (index,before,after))
        if index!=expected or after != (before&~mask)|value:
            return False
        if after!=before: attempted|=1<<index
    lanes=re.search(r'^DSI_PHY .*lanes=([0-9a-f]{8}) ',clean,re.M)
    if not lanes or int(words[12][1],16)!=int(lanes[1],16) or int(words[13][1],16)&3:
        return False
    a=dict(zip(keys,(int(v,16) for v in rows[0])))
    return (a['result']==0 and a['stage']==16 and a['attempted']==attempted and a['status']&0x95==0x95 and
            a['fifo']&0x7f==0x15 and a['escape_hz']==120000000//7 and a['timeout_hz']==10000000 and
            a['preserved']==1 and a['duplicate_busy']==1 and a['samples']==16)


def evaluate_dsi_short(output, allow_long=False):
    if not allow_long and 'DSI_LONG' in output:
        return False
    if not evaluate_dsi_command(output,allow_short=True):
        return False
    clean=output.replace('\r','');lines=clean.splitlines()
    keys='index result stage header attempted polls busy before after status fifo error0 error1'.split()
    rows=re.findall(r'^DSI_SHORT'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
    summary=re.findall(r'^DSI_SHORT_SUMMARY packets=([0-9a-f]{8}) busy=([0-9a-f]{8}) mode_changes=([0-9a-f]{8}) preserved=([0-9a-f]{8}) panel_ack=([0-9a-f]{8})$',clean,re.M)
    if len(rows)!=16 or len(summary)!=1 or sum(l.startswith('DSI_SHORT ') for l in lines)!=16 or sum(l.startswith('DSI_SHORT_SUMMARY') for l in lines)!=1 or lines.count('MAIA_NATIVE_DSI_SHORT_PASS')!=1:
        return False
    positions=[i for i,l in enumerate(lines) if l.startswith('DSI_SHORT ')]
    if not (lines.index('MAIA_NATIVE_DSI_COMMAND_PASS')<positions[0] and positions[-1]<next(i for i,l in enumerate(lines) if l.startswith('DSI_SHORT_SUMMARY '))<lines.index('MAIA_NATIVE_DSI_SHORT_PASS')):
        return False
    busy=changes=0;previous=None
    for index,row in enumerate(rows):
        a=dict(zip(keys,(int(v,16) for v in row)))
        if not (a['index']==index and a['result']==0 and a['stage']==5 and a['header']==5 and a['attempted']==1 and
                1<=a['polls']<=10000 and a['busy'] in (0,1) and a['before']&3==0 and a['after']==a['before']|0x10000 and
                a['status']&0x95==0x95 and a['fifo']&0x7f==0x15 and not a['error0']&0x1fffff and not a['error1']&0x81fff):
            return False
        if previous is not None and a['before']!=previous:
            return False
        previous=a['after'];busy+=a['busy'];changes+=a['before']!=a['after']
    return [int(v,16) for v in summary[0]]==[16,busy,changes,1,0]


def evaluate_dsi_long(output, allow_read=False):
    if not allow_read and 'DSI_READ' in output:
        return False
    if not evaluate_dsi_short(output,allow_long=True):
        return False
    clean=output.replace('\r','');lines=clean.splitlines()
    keys='index result stage header attempted bytes words polls space busy before after pre_fifo status fifo error0 error1'.split()
    follow_keys='result stage header attempted polls busy status fifo error0 error1'.split()
    rows=re.findall(r'^DSI_LONG'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
    follow=re.findall(r'^DSI_LONG_FOLLOWUP'+''.join(' '+k+r'=([0-9a-f]{8})' for k in follow_keys)+'$',clean,re.M)
    summary=re.findall(r'^DSI_LONG_SUMMARY packets=([0-9a-f]{8}) busy=([0-9a-f]{8}) mode_changes=([0-9a-f]{8}) preserved=([0-9a-f]{8}) panel_ack=([0-9a-f]{8})$',clean,re.M)
    if len(rows)!=4 or len(follow)!=1 or len(summary)!=1 or sum(l.startswith('DSI_LONG ') for l in lines)!=4 or sum(l.startswith('DSI_LONG_FOLLOWUP') for l in lines)!=1 or sum(l.startswith('DSI_LONG_SUMMARY') for l in lines)!=1 or lines.count('MAIA_NATIVE_DSI_LONG_PASS')!=1:
        return False
    positions=[i for i,l in enumerate(lines) if l.startswith('DSI_LONG ')]
    if not (lines.index('MAIA_NATIVE_DSI_SHORT_PASS')<positions[0] and positions[-1]<next(i for i,l in enumerate(lines) if l.startswith('DSI_LONG_FOLLOWUP '))<next(i for i,l in enumerate(lines) if l.startswith('DSI_LONG_SUMMARY '))<lines.index('MAIA_NATIVE_DSI_LONG_PASS')):
        return False
    previous=int(re.findall(r'^DSI_SHORT .* after=([0-9a-f]{8}) ',clean,re.M)[-1],16);busy=changes=0
    for index,row in enumerate(rows):
        a=dict(zip(keys,(int(v,16) for v in row)))
        if not (a['index']==index and a['result']==0 and a['stage']==6 and a['header']==0x139 and a['attempted']==1 and
                a['bytes']==a['words']==1 and 3<=a['polls']<=10000 and 2<=a['space']<a['polls'] and a['busy'] in (0,1) and
                a['before']==previous and a['after']==a['before']|0x80000 and a['before']&3==0 and a['pre_fifo']&0x30003==0x10001 and
                a['pre_fifo']&0x70==0x10 and a['status']&0x95==0x95 and a['fifo']&0xf007f==0x50015 and
                not a['error0']&0x1fffff and not a['error1']&0x81fff): return False
        busy+=a['busy'];changes+=a['before']!=a['after'];previous=a['after']
    a=dict(zip(follow_keys,(int(v,16) for v in follow[0])))
    return (a['result']==0 and a['stage']==5 and a['header']==5 and a['attempted']==1 and 1<=a['polls']<=10000 and a['busy'] in (0,1) and
            a['status']&0x95==0x95 and a['fifo']&0xf007f==0x50015 and not a['error0']&0x1fffff and not a['error1']&0x81fff and
            [int(v,16) for v in summary[0]]==[4,busy,changes,1,0])


def evaluate_dsi_read(output, allow_reset=False):
    if not allow_reset and 'DSI_RESET' in output: return False
    if not allow_reset and 'DSI_PANEL_RESET' in output: return False
    if not evaluate_dsi_long(output,allow_read=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    keys='index result stage headers words word id0 id1 id2 polls before after packet vc restored status fifo error0 error1'.split()
    rows=re.findall(r'^DSI_READ'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
    summary=re.findall(r'^DSI_READ_SUMMARY replies=([0-9a-f]{8}) id=([0-9a-f]{8}) stable=([0-9a-f]{8}) preserved=([0-9a-f]{8})$',clean,re.M)
    if len(rows)!=4 or len(summary)!=1 or sum(l.startswith('DSI_READ ') for l in lines)!=4 or sum(l.startswith('DSI_READ_SUMMARY') for l in lines)!=1 or lines.count('MAIA_NATIVE_DSI_READ_PASS')!=1: return False
    positions=[i for i,l in enumerate(lines) if l.startswith('DSI_READ ')]
    if not (lines.index('MAIA_NATIVE_DSI_LONG_PASS')<positions[0] and positions[-1]<next(i for i,l in enumerate(lines) if l.startswith('DSI_READ_SUMMARY '))<lines.index('MAIA_NATIVE_DSI_READ_PASS')): return False
    identity=None
    for index,row in enumerate(rows):
        a=dict(zip(keys,(int(v,16) for v in row)))
        expected_restore=1 | (2 if a['vc']&3 else 0) | (4 if a['before']|0x1040000 != a['before'] else 0)
        if not (a['index']==index and a['result']==0 and a['stage']==10 and a['headers']==3 and a['words']==1 and
                [a['id0'],a['id1'],a['id2']]==[(a['word']>>(8*i))&255 for i in range(3)] and 3<=a['polls']<=10000 and
                a['after']==a['before'] and a['before']&3==0 and a['packet']&4==0 and a['restored']==expected_restore and
                a['status']&0x97==0x95 and a['fifo']&0xf007f==0x50015 and not a['error0']&0x1fffff and not a['error1']&0x81fff): return False
        if identity is None: identity=a['word']&0xffffff
        elif identity!=a['word']&0xffffff: return False
    return [int(v,16) for v in summary[0]]==[4,identity,1,1]


def evaluate_dsi_reset(output, allow_st7121=False):
    if not allow_st7121 and 'ST7121_' in output: return False
    if not evaluate_dsi_read(output,allow_reset=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    schemas={
        'DSI_RESET_QUIET':'sys phy reset firmware dma_quiet supply_ready'.split(),
        'DSI_PANEL_RESET':'result stage changed elapsed_us'.split(),
        'DSI_RESET_RECOVERY':'firmware status banks_preserved clocks_preserved supply_preserved gpio23_input'.split(),
    }
    rows={};positions=[]
    for prefix,keys in schemas.items():
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=1 or sum(l.startswith(prefix) for l in lines)!=1: return False
        rows[prefix]=dict(zip(keys,(int(v,16) for v in found[0])))
        positions.append(next(i for i,l in enumerate(lines) if l.startswith(prefix)))
    q=rows['DSI_RESET_QUIET'];r=rows['DSI_PANEL_RESET'];a=rows['DSI_RESET_RECOVERY']
    if lines.count('MAIA_NATIVE_DSI_RESET_PASS')!=1: return False
    return (lines.index('MAIA_NATIVE_VO3_CYCLE_PASS')<positions[0]<positions[1]<positions[2]<
            lines.index('MAIA_NATIVE_DSI_RESET_PASS')<next(i for i,l in enumerate(lines) if l.startswith('DSI_ACCESS ')) and
            q['sys']&0x1c00==0 and q['phy']&0x93==0 and q['reset']&0x3c000000==0x3c000000 and
            q['firmware'] in (1,3) and q['dma_quiet']==q['supply_ready']==1 and
            r['result']==0 and r['stage']==9 and r['changed']==1 and r['elapsed_us']>=200000 and
            a['firmware']==q['firmware'] and a['status']==0 and
            all(a[k]==1 for k in ('banks_preserved','clocks_preserved','supply_preserved','gpio23_input')))


def evaluate_st7121(output, allow_power=False):
    if not allow_power and 'PANEL_POWER' in output: return False
    if not evaluate_dsi_reset(output,allow_st7121=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def rows(prefix,keys,count):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=count or sum(l.startswith(prefix+' ') for l in lines)!=count: return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    packets=rows('ST7121_PACKET','index command bytes result stage polls error0 error1'.split(),31)
    program=rows('ST7121_PROGRAM','result attempted completed waited_ms last callback elapsed_us'.split(),1)
    replies=rows('ST7121_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if packets is None or program is None or replies is None or lines.count('MAIA_NATIVE_ST7121_PASS')!=1: return False
    expected=[(1, 1), (96, 4), (96, 4), (96, 4), (120, 2), (121, 2), (164, 2), (183, 7), (176, 8), (191, 3), (165, 3), (215, 7), (144, 8), (163, 40), (166, 56), (167, 61), (172, 45), (173, 26), (174, 8), (178, 18), (232, 15), (117, 3), (231, 43), (225, 3), (234, 4), (200, 38), (201, 38), (96, 4), (17, 1), (41, 1), (53, 2)]
    for i,(p,(command,length)) in enumerate(zip(packets,expected)):
        if not (p['index']==i and p['command']==command and p['bytes']==length and p['result']==0 and
                p['stage']==(5 if length<=2 else 6) and 1<=p['polls']<=10000 and not p['error0']&0x1fffff and not p['error1']&0x81fff): return False
    p=program[0]
    if not (p['result']==0 and p['attempted']==p['completed']==31 and p['waited_ms']==1150 and p['last']==0x35 and p['callback']==0 and p['elapsed_us']>=1150000): return False
    identity=replies[0]['word']&0xffffff
    for i,a in enumerate(replies):
        if not (a['index']==i and a['result']==0 and a['stage']==10 and a['headers']==3 and 3<=a['polls']<=10000 and
                a['words']==1 and a['word']&0xffffff==identity and a['before']==a['after'] and not a['before']&3 and
                a['restored']==5 and a['status']&0x97==0x95 and a['fifo']&0xf007f==0x50015 and
                not a['error0']&0x1fffff and not a['error1']&0x81fff): return False
    positions=[i for i,l in enumerate(lines) if l.startswith('ST7121_PACKET ')]
    replies_at=[i for i,l in enumerate(lines) if l.startswith('ST7121_REPLY ')]
    summary_at=next(i for i,l in enumerate(lines) if l.startswith('ST7121_PROGRAM '))
    return (lines.index('MAIA_NATIVE_DSI_READ_PASS')<positions[0] and positions[-1]<summary_at<replies_at[0] and
            replies_at[-1]<lines.index('MAIA_NATIVE_ST7121_PASS'))


def evaluate_panel_power(output, allow_dpi=False):
    if not allow_dpi and 'DSI_DPI' in output: return False
    if not evaluate_st7121(output,allow_power=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    keys='phase index result stage request headers polls word value words before after restored status fifo error0 error1'.split()
    rows=re.findall('^PANEL_POWER'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
    summaries=re.findall(r'^PANEL_POWER_SUMMARY before=([0-9a-f]{8}) after=([0-9a-f]{8}) sleep_out=([0-9a-f]{8}) display_on=([0-9a-f]{8})$',clean,re.M)
    if len(rows)!=8 or sum(l.startswith('PANEL_POWER ') for l in lines)!=8 or len(summaries)!=1 or sum(l.startswith('PANEL_POWER_SUMMARY') for l in lines)!=1 or lines.count('MAIA_NATIVE_PANEL_POWER_PASS')!=1: return False
    values=[]
    for n,row in enumerate(rows):
        a=dict(zip(keys,(int(v,16) for v in row)))
        expected_restore=1 | (4 if a['before']|0x1040000 != a['before'] else 0)
        if not (a['phase']==n//4 and a['index']==n%4 and a['result']==0 and a['stage']==10 and a['request']==0xa06 and
                a['headers']==3 and 3<=a['polls']<=10000 and a['words']==1 and a['value']==a['word']&255 and
                a['before']==a['after'] and not a['before']&3 and a['restored']==expected_restore and a['status']&0x97==0x95 and
                a['fifo']&0xf007f==0x50015 and not a['error0']&0x1fffff and not a['error1']&0x81fff): return False
        values.append(a['value'])
    if len(set(values[:4]))!=1 or len(set(values[4:]))!=1 or values[0]&0x14 or values[4]&0x14!=0x14: return False
    if [int(v,16) for v in summaries[0]]!=[values[0],values[4],1,1]: return False
    positions=[i for i,l in enumerate(lines) if l.startswith('PANEL_POWER ')]
    start=next(i for i,l in enumerate(lines) if l.startswith('ST7121_PACKET '))
    summary=next(i for i,l in enumerate(lines) if l.startswith('PANEL_POWER_SUMMARY '))
    return (lines.index('MAIA_NATIVE_DSI_READ_PASS')<positions[0] and positions[3]<start and
            lines.index('MAIA_NATIVE_ST7121_PASS')<positions[4] and positions[-1]<summary<lines.index('MAIA_NATIVE_PANEL_POWER_PASS'))


def evaluate_dpi_stage(output, allow_clock=False):
    if not allow_clock and ('DPI_CLOCK' in output or 'DPI_COLOR' in output): return False
    if not evaluate_panel_power(output,allow_dpi=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def row(prefix,keys,count):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=count or sum(l.startswith(prefix+' ') for l in lines)!=count: return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    stages=row('DSI_DPI_STAGE','result stage attempted clock_before clock_after nominal_hz status fifo error0 error1'.split(),1)
    words=row('DSI_DPI_WORD','index before after'.split(),19)
    idle=row('DSI_DPI_IDLE','mode bridge dpi clock update dma_quiet preserved duplicate_busy'.split(),1)
    replies=row('DSI_DPI_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if stages is None or words is None or idle is None or replies is None or lines.count('MAIA_NATIVE_DPI_STAGE_PASS')!=1: return False
    a=stages[0];q=idle[0]
    if not (a['result']==0 and a['stage']==22 and a['nominal_hz']==80000000 and a['clock_before']&0x93==3 and
            a['clock_after']==a['clock_before']&~0xff60|0x220 and a['status']&0x97==0x95 and a['fifo']&0xf007f==0x50015 and
            not a['error0']&0x1fffff and not a['error1']&0x81fff): return False
    masks=[3,0x10f,0x1f,0xbf03,0x3fff,0x1fff,0x1fff,0xfff,0xfff,0x7fff,0x3ff,0x3ff,0x3ff,0x3fff,0x7f,0x0fff0fff,0x0fff0fff,0x0fff0fff,0x0fff0fff]
    values=[0,0,0,0xbf02,720,0,0,3,60,1203,20,24,200,1280,2,0x050005f4,0x00140018,0x02d00322,0x00020028]
    attempted=int(a['clock_before']!=a['clock_after'])
    for i,(w,mask,value) in enumerate(zip(words,masks,values)):
        if not (w['index']==i and w['after']==w['before']&~mask|value): return False
        if w['after']!=w['before']: attempted|=1<<(i+1)
    if a['attempted']!=attempted or words[3]['before']&0x14000: return False
    if not (q['mode']==1 and not q['bridge']&1 and not q['dpi']&1 and q['clock']==a['clock_after'] and
            q['update']==0 and q['dma_quiet']==q['preserved']==q['duplicate_busy']==1): return False
    first=replies[0]['word']&255
    for i,r in enumerate(replies):
        if not (r['index']==i and r['result']==0 and r['stage']==10 and r['headers']==3 and 3<=r['polls']<=10000 and
                r['words']==1 and r['word']&255==first and r['word']&0x14==0x14 and r['before']==r['after'] and
                not r['before']&3 and r['restored']==5 and r['status']&0x97==0x95 and r['fifo']&0xf007f==0x50015 and
                not r['error0']&0x1fffff and not r['error1']&0x81fff): return False
    positions=[i for i,l in enumerate(lines) if l.startswith('DSI_DPI_WORD ')]
    reply_at=[i for i,l in enumerate(lines) if l.startswith('DSI_DPI_REPLY ')]
    return (lines.index('MAIA_NATIVE_PANEL_POWER_PASS')<next(i for i,l in enumerate(lines) if l.startswith('DSI_DPI_STAGE '))<positions[0] and
            positions[-1]<next(i for i,l in enumerate(lines) if l.startswith('DSI_DPI_IDLE '))<reply_at[0] and reply_at[-1]<lines.index('MAIA_NATIVE_DPI_STAGE_PASS'))


def evaluate_dpi_clock(output,allow_pattern=False,allow_bridge=False):
    if not allow_bridge and "DPI_BRIDGE" in output: return False
    if not allow_pattern and ("DPI_PATTERN" in output): return False
    if not evaluate_dpi_stage(output,allow_clock=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def row(prefix,keys,count):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=count or sum(l.startswith(prefix+' ') for l in lines)!=count: return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    colors=row('DPI_COLOR','phase index result stage request headers polls word value words before after restored status fifo error0 error1'.split(),12)
    select=row('DPI_COLOR_SELECT','before after parameter attempted header'.split(),1)
    clocks=row('DPI_CLOCK','result stage attempted provider_before provider_after clock_before clock_after nominal_hz status fifo error0 error1'.split(),1)
    idle=row('DPI_CLOCK_IDLE','mode bridge dpi update dma_quiet preserved duplicate_busy'.split(),1)
    replies=row('DPI_CLOCK_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if any(a is None for a in (colors,select,clocks,idle,replies)) or lines.count('MAIA_NATIVE_DPI_CLOCK_PASS')!=1: return False
    vals=[]
    for i,r in enumerate(colors):
        if not (r['phase']==i//4 and r['index']==i%4 and r['result']==0 and r['stage']==10 and r['request']==0xc06 and
                r['headers']==3 and 3<=r['polls']<=10000 and r['words']==1 and r['value']==r['word']&255 and
                r['before']==r['after'] and not r['before']&3 and r['restored']==5 and r['status']&0x97==0x95 and
                r['fifo']&0xf007f==0x50015 and not r['error0']&0x1fffff and not r['error1']&0x81fff): return False
        vals.append(r['value'])
    if any(len(set(vals[i:i+4]))!=1 for i in (0,4,8)) or vals[0]&7 not in (0,5,6,7) or vals[4]&7!=5 or vals[8]!=vals[4]: return False
    a=select[0];attempted=int(vals[0]&7!=5)
    if not (a['before']==vals[0] and a['after']==vals[4] and a['parameter']==0x55 and a['attempted']==attempted and a['header']==(0x553a15 if attempted else 0)): return False
    a=clocks[0];q=idle[0]
    expected=2 | int(a['provider_before']!=a['provider_before']|0x40000000)
    if not (a['result']==0 and a['stage']==4 and a['attempted']==expected and a['provider_after']==a['provider_before']|0x40000000 and
            a['clock_before']&0xfff3==0x223 and a['clock_after']==a['clock_before']|0x80 and a['nominal_hz']==80000000 and
            a['status']&0x97==0x95 and a['fifo']&0xf007f==0x50015 and not a['error0']&0x1fffff and not a['error1']&0x81fff and
            q['mode']==1 and not q['bridge']&1 and not q['dpi']&1 and q['update']==0 and q['dma_quiet']==q['preserved']==q['duplicate_busy']==1): return False
    first=replies[0]['word']&255
    for i,r in enumerate(replies):
        if not (r['index']==i and r['result']==0 and r['stage']==10 and r['headers']==3 and 3<=r['polls']<=10000 and r['words']==1 and
                r['word']&255==first and r['word']&0x14==0x14 and r['before']==r['after'] and not r['before']&3 and r['restored']==5 and
                r['status']&0x97==0x95 and r['fifo']&0xf007f==0x50015 and not r['error0']&0x1fffff and not r['error1']&0x81fff): return False
    positions=[i for i,l in enumerate(lines) if l.startswith('DPI_COLOR ')]
    selected=next(i for i,l in enumerate(lines) if l.startswith('DPI_COLOR_SELECT '))
    clock=next(i for i,l in enumerate(lines) if l.startswith('DPI_CLOCK '))
    idle_at=next(i for i,l in enumerate(lines) if l.startswith('DPI_CLOCK_IDLE '))
    post=[i for i,l in enumerate(lines) if l.startswith('DPI_CLOCK_REPLY ')]
    return (lines.index('MAIA_NATIVE_DPI_STAGE_PASS')<positions[0] and positions[7]<selected<clock<idle_at<positions[8] and
            positions[-1]<post[0] and post[-1]<lines.index('MAIA_NATIVE_DPI_CLOCK_PASS'))


def evaluate_dpi_bridge(output,allow_tile=False):
    if not allow_tile and "DPI_DMA_TILE" in output: return False
    if not evaluate_dpi_clock(output,allow_bridge=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def rows(prefix,keys,n):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=n or sum(l.startswith(prefix+' ') for l in lines)!=n: return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    request=rows('DPI_BRIDGE_REQUEST','result stage attempted before after update_value irq_raw status fifo error0 error1'.split(),1)
    idle=rows('DPI_BRIDGE_IDLE','bridge dpi mode lp dma_quiet preserved duplicate_busy vpg_busy latch_verified'.split(),1)
    reply=rows('DPI_BRIDGE_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if any(a is None for a in (request,idle,reply)) or lines.count('MAIA_NATIVE_DPI_BRIDGE_PASS')!=1: return False
    if any(l.startswith('DPI_BRIDGE_') and l.split()[0] not in ('DPI_BRIDGE_REQUEST','DPI_BRIDGE_IDLE','DPI_BRIDGE_REPLY') for l in lines): return False
    a=request[0];q=idle[0]
    if not (a['result']==0 and a['stage']==4 and a['attempted']==3 and not a['before']&1 and a['after']==a['before']|1 and
            a['update_value']==1 and not a['irq_raw']&1 and a['status']&0x97==0x95 and a['fifo']&0xf007f==0x50015 and
            not a['error0']&0x1fffff and not a['error1']&0x81fff and q['bridge']==a['after'] and not q['dpi']&1 and
            q['mode']==1 and not q['lp']&3 and q['dma_quiet']==q['preserved']==q['duplicate_busy']==q['vpg_busy']==1 and q['latch_verified']==0): return False
    for i,r in enumerate(reply):
        if not (r['index']==i and r['result']==0 and r['stage']==10 and r['headers']==3 and 3<=r['polls']<=10000 and
                r['words']==1 and r['word']&255==reply[0]['word']&255 and r['word']&0x14==0x14 and r['before']==r['after'] and
                not r['before']&3 and r['restored']==5 and r['status']&0x97==0x95 and r['fifo']&0xf007f==0x50015 and
                not r['error0']&0x1fffff and not r['error1']&0x81fff): return False
    positions=[i for i,l in enumerate(lines) if l.startswith(('DPI_BRIDGE_REQUEST ','DPI_BRIDGE_IDLE ','DPI_BRIDGE_REPLY '))]
    return ([lines[i].split()[0] for i in positions]==['DPI_BRIDGE_REQUEST','DPI_BRIDGE_IDLE']+['DPI_BRIDGE_REPLY']*4 and
            lines.index('MAIA_NATIVE_DPI_CLOCK_PASS')<positions[0] and positions[-1]<lines.index('MAIA_NATIVE_DPI_BRIDGE_PASS'))


def dpi_dma_tile_hashes(source,base):
    import struct
    def fnv(data):
        h=2166136261
        for value in data: h=((h^value)*16777619)&0xffffffff
        return h
    pixels=b''.join(struct.pack('<H',color)*180 for color in (0xf800,0x07e0,0x001f,0xffe0))*8
    descriptors=[]
    for i in range(160):
        w=[0]*16;w[0]=source;w[2]=0x50105000;w[4]=1439
        w[6]=(base+(i+1)*64)|1 if i<159 else 0
        w[8]=0x1e1b41;w[9]=0x80108840|(0x40000000 if i==159 else 0)
        descriptors.append(struct.pack('<16I',*w))
    return fnv(pixels),fnv(b''.join(descriptors))


def evaluate_psram_park(output):
    if not evaluate_psram_supply(output):return False
    clean=output.replace('\r','')
    rows=re.findall(r'^PSRAM_PARK phase=([0-9a-f]{8}) result=([0-9a-f]{8}) stage=([0-9a-f]{8}) writes=([0-9a-f]{8}) restored=([0-9a-f]{8}) before0=([0-9a-f]{8}) after0=([0-9a-f]{8}) before1=([0-9a-f]{8}) after1=([0-9a-f]{8}) before2=([0-9a-f]{8}) after2=([0-9a-f]{8})$',clean,re.M)
    if len(rows)!=2 or sum(l.startswith('PSRAM_PARK ') for l in clean.splitlines())!=2:return False
    initial=None;parked=None
    for phase,row in enumerate(rows):
        ph,result,stage,writes,restored,c0,c1,p0,p1,o0,o1=map(lambda v:int(v,16),row)
        if (ph,result,stage,writes,restored)!=(phase,0,4,4,phase):return False
        if not phase:
            if c0&0xFFB000!=0x8000 or not c0&0x4000 or not p0&0x1000000 or not o0&0x10000000:return False
            if (c1,p1,o1)!=(c0&~0xC000,p0&~0x1000000,o0&~0x10000000):return False
            initial=(c0,p0,o0);parked=(c1,p1,o1)
        elif (c0,p0,o0)!=parked or (c1,p1,o1)!=initial:return False
    return clean.index('PSRAM_PARK phase=00000001')<clean.index('MPLL_BOOT ')

def evaluate_psram_supply(output):
    if not evaluate_psram_profile(output):return False
    rows=re.findall(r'^PSRAM_SUPPLY result=([0-9a-f]{8}) stage=([0-9a-f]{8}) adopted=([0-9a-f]{8}) writes=([0-9a-f]{8}) dref=([0-9a-f]{8}) mul=([0-9a-f]{8}) before0=([0-9a-f]{8}) after0=([0-9a-f]{8}) before1=([0-9a-f]{8}) after1=([0-9a-f]{8})$',output.replace('\r',''),re.M)
    if len(rows)!=1 or sum(l.startswith('PSRAM_SUPPLY ') for l in output.replace('\r','').splitlines())!=1:return False
    result,stage,adopted,writes,d,m,c0,c1,a0,a1=map(lambda v:int(v,16),rows[0])
    return (result,stage,adopted,writes)==(0,4,1,0) and 0<d<16 and 0<m<8 and c0==c1 and a0==a1 and c1&0x4F80==0x180 and a1>>28==d and (a1>>23)&7==m and bool(a1&0x4000000)

def evaluate_psram_profile(output):
    if not evaluate_mpll_initialize(output):return False
    rows=re.findall(r'^PSRAM_PROFILE phase=([0-9a-f]{8}) result=([0-9a-f]{8}) stage=([0-9a-f]{8}) attempts=([0-9a-f]{8}) polls=([0-9a-f]{8}) restored=([0-9a-f]{8}) before0=([0-9a-f]{8}) after0=([0-9a-f]{8}) before4=([0-9a-f]{8}) after4=([0-9a-f]{8}) before8=([0-9a-f]{8}) after8=([0-9a-f]{8})$',output.replace('\r',''),re.M)
    if len(rows)!=2 or sum(l.startswith('PSRAM_PROFILE ') for l in output.replace('\r','').splitlines())!=2:return False
    a,b=([int(x,16) for x in row] for row in rows)
    if a[:4]!=[0,0,3,7] or b[:4]!=[1,0,3,7] or not a[4] or not b[4] or a[5]!=0 or b[5]!=1:return False
    return (a[7]==(a[6]&~0x3f)|0x30 and a[9]==(a[8]&~0xe0)|0x20 and a[11]==0x4b and
            b[6]==a[7] and b[8]==a[9] and b[10]==a[11] and b[7]==a[6] and b[9]==a[8] and b[11]==a[10])


def evaluate_psram_native_boot(output):
    if not evaluate_mpll_initialize(output,boot=True):return False
    clean=output.replace('\r','')
    boot_pos=clean.find('PSRAM_BOOT ')
    cache_pos=clean.find('DPI_FB_CACHE ')
    if boot_pos<0 or cache_pos<0 or boot_pos>=cache_pos:return False
    keys='result stage writes supply_stage supply_writes adopted dref mul beforectl afterctl beforeana afterana clock_stage attempts beforeperi afterperi bus0 bus1 profile_stage profile_attempts profile_polls before0 after0 before4 after4 before8 after8'.split()
    lines=[l for l in clean.splitlines() if l.startswith('PSRAM_BOOT ')]
    if len(lines)!=1:return False
    row=re.fullmatch('PSRAM_BOOT'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys),lines[0])
    if row is None:return False
    a=dict(zip(keys,(int(v,16) for v in row.groups())))
    return bool(a['result']==0 and a['stage']==7 and a['writes']==8 and a['supply_stage']==4 and a['supply_writes']==3 and a['adopted']==0 and
        0<a['dref']<16 and 0<a['mul']<8 and not a['beforectl']&0x100 and a['afterctl']&0x4f80==0x180 and not(a['beforectl']^a['afterctl'])&~0x4f80 and
        a['afterana']>>28==a['dref'] and (a['afterana']>>23)&7==a['mul'] and a['afterana']&0x4000000 and not(a['beforeana']^a['afterana'])&~0xf7800000 and
        a['clock_stage']==6 and a['attempts']==63 and a['beforeperi']==a['afterperi'] and a['afterperi']&0xfff000==0xc000 and a['bus0']==a['bus1']==0x10001 and
        a['profile_stage']==3 and a['profile_attempts']==7 and a['profile_polls']>0 and a['after0']==(a['before0']&~0x3f)|0x30 and
        a['after4']==(a['before4']&~0xe0)|0x20 and not a['before8']&~0x4f and a['after8']==0x4b and
        clean.index('PSRAM_BOOT ')<clean.index('DPI_FB_FAST '))

def evaluate_mpll_initialize(output,boot=False):
    if not evaluate_lcd_stream(output,touch=True,input_lifecycle=True):return False
    rows=re.findall(r'^MPLL_BOOT result=([0-9a-f]{8}) stage=([0-9a-f]{8}) writes=([0-9a-f]{8}) polls=([0-9a-f]{8}) routing=([0-9a-f]{8}) saw_low=([0-9a-f]{8}) cal_before=([0-9a-f]{8}) cal_after=([0-9a-f]{8}) before1=([0-9a-f]{8}) after1=([0-9a-f]{8}) before2=([0-9a-f]{8}) after2=([0-9a-f]{8}) before3=([0-9a-f]{8}) after3=([0-9a-f]{8}) mhz=([0-9a-f]{8})$',output.replace('\r',''),re.M)
    if len(rows)!=1 or sum(l.startswith('MPLL_BOOT ') for l in output.replace('\r','').splitlines())!=1:return False
    v=[int(x,16) for x in rows[0]]
    return bool(v[0]==0 and v[1]==5 and v[2]==4 and v[3]>0 and v[4]==v[5]==1 and v[6]&0x200 and v[7]&0x300==0x300 and not((v[6]^v[7])&~0x100) and v[9]&0x20 and (boot or v[10]==0x99) and v[11]==0x99 and v[13]&0x30==0x30 and v[14]==400)


def evaluate_lcd_stream(output,visual=False,lifecycle=False,touch=False,input_lifecycle=False):
    if not evaluate_dpi_bridge(output):return False
    clean=output.replace('\r','');lines=clean.splitlines()
    marker='MAIA_NATIVE_TOUCH_LCD_PASS' if touch else 'MAIA_NATIVE_LCD_LIFECYCLE_PASS' if lifecycle else 'MAIA_NATIVE_LCD_STREAM_PASS'
    if lines.count(marker)!=1 or 'MAIA_NATIVE_DPI_FRAMEBUFFER_PASS' in lines:return False
    if input_lifecycle:
        if visual or not touch:return False
        rows=[l for l in lines if l.startswith('TOUCH_LIFECYCLE ')]
        if rows!=['TOUCH_LIFECYCLE cycles=00000003 stops=00000004 restarts=00000003 stopped=00000001 closed=00000001 refused=00000003 sequence=00000003 bus_refused=00000003']:return False

    if lifecycle:
        cycles=[l for l in lines if l.startswith('LCD_CYCLE ')]
        edits=[l for l in lines if l.startswith('LCD_EDIT ')]
        if len(cycles)!=3 or len(edits)!=6:return False
        total=0
        for i,l in enumerate(cycles):
            m=re.fullmatch(f'LCD_CYCLE index={i:08x} frames=([0-9a-f]{{8}}) updates=00000002 presented=00000002 irqs=([0-9a-f]{{8}}) published=00000002 stopped=00000001 released=00000001 errors=00000000 timeouts=00000000',l)
            if m is None or int(m[1],16)<20 or m[1]!=m[2]:return False
            total+=int(m[1],16)
        for i,l in enumerate(edits):
            expected=0xbcc4e003 if i%2==0 else 0xe3d71bef
            m=re.fullmatch(f'LCD_EDIT index={i:08x} hash={expected:08x} ticks=([0-9a-f]{{8}}) beats=([0-9a-f]{{8}})',l)
            if m is None or not 3<=int(m[1],16)<=200 or not int(m[2],16):return False
    def row(prefix,keys):
        selected=[l for l in lines if l.startswith(prefix+' ')]
        if len(selected)!=1:return None
        m=re.fullmatch(prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys),selected[0])
        return dict(zip(keys,(int(v,16) for v in m.groups()))) if m else None
    init=[l for l in lines if l.startswith('LCD_STREAM_INIT ')]
    if init!=[f'LCD_STREAM_INIT part={i:08x} result=00000000' for i in range(6)]:return False
    prep=row('LCD_STREAM_PREP','engine irq map ctrl'.split())
    if prep is None or prep['engine']!=5 or prep['irq']!=6:return False
    route=row('LCD_STREAM_ROUTE','map_before map_after ctrl_before ctrl_after'.split())
    task=row('LCD_STREAM_TASK','result frames waits timeouts errors stopped requested active_stop'.split())
    irq=row('LCD_STREAM_IRQ','line source irqs posts spurious drains released done recovered raw status common bridge error0 error1'.split())
    kernel=row('LCD_STREAM_KERNEL','ticks timer external ecalls beats work exited errors'.split())
    image=row('DPI_FB_IMAGE','source bytes hash native_width native_height rotation frames visual'.split())
    idle=row('DPI_FB_IDLE','cfg channels dpi mode hash guards duplicate'.split())
    memory=row('DPI_FB_MEMORY_RESTORE','bytes roots words mode axi'.split())
    if any(r is None for r in (route,task,irq,kernel,image,idle,memory)):return False
    fast=row('DPI_FB_FAST','result rate mpll div dll0 dll1'.split())
    cache=row('DPI_FB_CACHE','result stage size line'.split())
    copy_final=row('PSRAM_DMA_FINAL','cfg channels fifo restored preserved'.split())
    if fast!=dict(result=0,rate=200,mpll=400,div=0x99,dll0=0x21,dll1=0x21):return False
    if cache is None or cache['result']!=0 or cache['stage']!=3 or cache['size'] not in (131072,262144,524288) or cache['line'] not in (64,128):return False
    if copy_final!=dict(cfg=0,channels=0,fifo=0,restored=1,preserved=1):return False
    if touch:
        device=row('TOUCH_INFO','result firmware max_x max_y slots'.split())
        if device is None or device['result'] or device['firmware'] not in (1,3) or not 1<=device['slots']<=10 or not device['max_x'] or not device['max_y']:return False
        init_touch=[l for l in lines if l.startswith('TOUCH_INIT ')]
        if init_touch!=[f'TOUCH_INIT part={i:08x} result=00000000' for i in range(3)]:return False
        inp=row('TOUCH_INPUT','polls reports errors downs moves ups corners draws updates presented bus_calls bus_errors contentions'.split())
        if inp is None or inp['polls']<10 or inp['errors'] or inp['bus_errors'] or inp['contentions'] or inp['bus_calls']<inp['polls'] or inp['draws']!=inp['updates'] or inp['updates']!=inp['presented'] or inp['updates']<1:return False
        if visual and not(inp['corners']==15 and inp['downs']>=4 and inp['ups']>=4 and inp['moves']>=3 and inp['presented']>=6):return False
        events=[l for l in lines if l.startswith('TOUCH_EVENT ')]
        if len(events)>32:return False
        for i,l in enumerate(events):
            m=re.fullmatch(f'TOUCH_EVENT index={i:08x} tick=([0-9a-f]{{8}}) count=([0-9a-f]{{8}}) x=([0-9a-f]{{8}}) y=([0-9a-f]{{8}})',l)
            if m is None:return False
            count,x,y=(int(m[k],16) for k in (2,3,4))
            # Explicit invalidation while stopped/restarting is not a contact.
            if input_lifecycle and count==0xffffffff:
                if x or y:return False
            elif count>10 or x>=1280 or y>=720:return False

    duration=20000 if visual and touch else 12000 if visual else 1500
    if lifecycle and task['frames']!=total:return False
    if route['map_before']!=route['map_after'] or (route['ctrl_before']^route['ctrl_after'])&~1:return False
    if prep['map']!=route['map_before'] or prep['ctrl']!=route['ctrl_before'] or prep['ctrl']&0x101:return False
    if not(task['result']==0 and 40<=task['frames']<=2000 and task['waits']>=task['frames'] and not task['timeouts'] and not task['errors'] and task['stopped']==task['requested']==task['active_stop']==1):return False
    if not(17<=irq['line']<48 and irq['source']==24 and irq['irqs']==irq['posts']==task['frames'] and not irq['spurious'] and irq['released']==irq['done']==irq['recovered']==1 and irq['raw']==230399 and irq['status']&2 and not irq['status']&0x023f7fe0 and not irq['common'] and not irq['bridge']&1 and not irq['error0']&0x1fffff and not irq['error1']&0x81fff):return False
    if not(duration<=kernel['ticks']<=duration+(1000 if lifecycle else 205) and kernel['timer']>=duration and kernel['external']==irq['irqs'] and kernel['ecalls']>task['frames'] and kernel['beats']>=duration//2 and kernel['work']>=10000 and kernel['exited']==(4 if touch else 3) and not kernel['errors']):return False
    if image!=dict(source=0x4b800080,bytes=1843200,hash=0xe3d71bef,native_width=720,native_height=1280,rotation=90,frames=0,visual=int(visual)):return False
    inherited=re.search(r'^DPI_BRIDGE_IDLE bridge=[0-9a-f]{8} dpi=([0-9a-f]{8}) ',clean,re.M)
    if inherited is None:return False
    if idle!=dict(cfg=0,channels=0,dpi=int(inherited.group(1),16),mode=1,hash=0xe3d71bef,guards=1,duplicate=1):return False
    if not(memory['bytes']==4096 and memory['roots']==memory['words']==1 and memory['mode']==0x4b and memory['axi']&0x80000001==0x80000000):return False
    # Reuse the independently byte-checked memory/reply portion via exact rows.
    for prefix,count in [('DPI_FB_RAW',3),('DPI_FB_REPLY',4),('PSRAM_DMA_COPY',9)]:
        selected=[l for l in lines if l.startswith(prefix+' ')]
        if len(selected)!=count:return False
        if prefix=='DPI_FB_RAW':
            if selected!=[f'DPI_FB_RAW phase={i:08x} result=00000000 bytes=00001000 restored=00000001' for i in range(3)]:return False
        elif prefix=='DPI_FB_REPLY':
            for i,l in enumerate(selected):
                if not re.fullmatch(f'DPI_FB_REPLY index={i:08x} result=00000000 word=0000009c restored=00000005 error0=00000000 error1=00000000',l):return False
        else:
            for i,l in enumerate(selected):
                if not re.fullmatch(f'PSRAM_DMA_COPY index={i:08x} offset={(0,65408,0x1ff000)[i//3]:08x} bytes=00001000 burst={(1,4,16)[i%3]:08x} done=00000001 polls=[0-9a-f]{{8}} matched=00001000 guards=00000001 raw=000001ff status=00000000 common=00000000 us=[0-9a-f]{{8}}',l):return False
    order=[l.split()[0] for l in lines if l.startswith('LCD_STREAM_') and not l.startswith(('LCD_STREAM_INIT ','LCD_STREAM_PREP '))]
    return order==['LCD_STREAM_ROUTE','LCD_STREAM_TASK','LCD_STREAM_IRQ','LCD_STREAM_KERNEL']


def evaluate_dpi_framebuffer(output,visual=False):
    if not evaluate_dpi_bridge(output):return False
    clean=output.replace('\r','');lines=clean.splitlines()
    if lines.count('MAIA_NATIVE_DPI_FRAMEBUFFER_PASS')!=1:return False
    def rows(prefix,keys,n):
        selected=[l for l in lines if l.startswith(prefix+' ')]
        found=[re.fullmatch(prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys),l) for l in selected]
        if len(found)!=n or any(m is None for m in found):return None
        return [dict(zip(keys,(int(v,16) for v in m.groups()))) for m in found]
    cache=rows('DPI_FB_CACHE','result stage size line'.split(),1)
    raw=rows('DPI_FB_RAW','phase result bytes restored'.split(),3)
    fast=rows('DPI_FB_FAST','result rate mpll div dll0 dll1'.split(),1)
    image=rows('DPI_FB_IMAGE','source bytes hash native_width native_height rotation frames visual'.split(),1)
    transfer=rows('DPI_FB_TRANSFER','result stage frames items rearms polls raw done recovered released status common bridge error0 error1 us'.split(),1)
    idle=rows('DPI_FB_IDLE','cfg channels dpi mode hash guards duplicate'.split(),1)
    restored=rows('DPI_FB_MEMORY_RESTORE','bytes roots words mode axi'.split(),1)
    replies=rows('DPI_FB_REPLY','index result word restored error0 error1'.split(),4)
    dma=rows('PSRAM_DMA_COPY','index offset bytes burst done polls matched guards raw status common us'.split(),9)
    final=rows('PSRAM_DMA_FINAL','cfg channels fifo restored preserved'.split(),1)
    if any(v is None for v in (cache,raw,fast,image,transfer,idle,restored,replies,dma,final)):return False
    c=cache[0];frames=1000 if visual else 4
    if not(c['result']==0 and c['stage']==3 and c['size'] in (131072,262144,524288) and c['line'] in (64,128)):return False
    if any(v!=dict(phase=i,result=0,bytes=4096,restored=1) for i,v in enumerate(raw)):return False
    if fast!=[dict(result=0,rate=200,mpll=400,div=0x99,dll0=0x21,dll1=0x21)]:return False
    if image!=[dict(source=0x4b800080,bytes=1843200,hash=0xe3d71bef,native_width=720,native_height=1280,rotation=90,frames=frames,visual=int(visual))]:return False
    a=transfer[0]
    if not(a['result']==0 and a['stage']==18 and a['frames']==frames and a['items']==frames*230400 and a['rearms']==frames-1 and
           1<=a['polls']<=frames*100000 and a['raw']==230399 and a['done']==a['recovered']==a['released']==1 and
           a['status']&2 and not a['status']&0x023f7fe0 and not a['common'] and not a['bridge']&1 and
           not a['error0']&0x1fffff and not a['error1']&0x81fff and a['us']>=(10000000 if visual else 1)):return False
    inherited=re.search(r'^DPI_BRIDGE_IDLE bridge=[0-9a-f]{8} dpi=([0-9a-f]{8}) ',clean,re.M)
    if inherited is None:return False
    expected_dpi=int(inherited.group(1),16)
    if expected_dpi&1 or idle!=[dict(cfg=0,channels=0,dpi=expected_dpi,mode=1,hash=0xe3d71bef,guards=1,duplicate=1)]:return False
    q=restored[0]
    if not(q['bytes']==4096 and q['roots']==q['words']==1 and q['mode']==0x4b and q['axi']&0x80000001==0x80000000):return False
    for i,a in enumerate(dma):
        if not(a['index']==i and a['offset']==(0,65408,0x1ff000)[i//3] and a['bytes']==a['matched']==4096 and a['burst']==(1,4,16)[i%3] and
               a['done']==a['guards']==1 and 1<=a['polls']<=1000000 and a['raw']==511 and not a['status']&0x023f7fe0 and not a['common'] and a['us']):return False
    if final!=[dict(cfg=0,channels=0,fifo=0,restored=1,preserved=1)]:return False
    for i,p in enumerate(replies):
        if not(p['index']==i and p['result']==0 and p['word']&255==replies[0]['word']&255 and p['word']&0x14==0x14 and p['restored']==5 and
               not p['error0']&0x1fffff and not p['error1']&0x81fff):return False
    names=[l.split()[0] for l in lines if l.startswith('DPI_FB_')]
    if names!=['DPI_FB_READY','DPI_FB_CACHE','DPI_FB_RAW','DPI_FB_FAST','DPI_FB_IMAGE','DPI_FB_TRANSFER','DPI_FB_IDLE','DPI_FB_RAW','DPI_FB_RAW','DPI_FB_MEMORY_RESTORE']+['DPI_FB_REPLY']*4:return False
    if len([l for l in lines if l.startswith('PSRAM_DMA')])!=10:return False
    return (lines.index('MAIA_NATIVE_DPI_BRIDGE_PASS')<lines.index('DPI_FB_READY')<next(i for i,l in enumerate(lines) if l.startswith('DPI_FB_IMAGE '))<
            next(i for i,l in enumerate(lines) if l.startswith('PSRAM_DMA_COPY '))<next(i for i,l in enumerate(lines) if l.startswith('PSRAM_DMA_FINAL '))<
            next(i for i,l in enumerate(lines) if l.startswith('DPI_FB_TRANSFER '))<lines.index('MAIA_NATIVE_DPI_FRAMEBUFFER_PASS'))


def evaluate_dpi_dma_tile(output,allow_stage=False,allow_spatial=False):
    if not allow_spatial and "DPI_SPATIAL" in output:return False
    if not allow_stage and "DPI_DMA_STAGE" in output: return False
    if not evaluate_dpi_bridge(output,allow_tile=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def rows(prefix,keys,n):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=n or sum(l.startswith(prefix+' ') for l in lines)!=n: return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    memory=rows('DPI_DMA_TILE_MEMORY','start end alias source list source_bytes count list_bytes frame_bytes cache_line prepared'.split(),1)
    items=rows('DPI_DMA_TILE_ITEM','index source destination items next ctl0 ctl1 zero'.split(),3)
    verify=rows('DPI_DMA_TILE_VERIFY','source_hash list_hash guards preserved inert repeated_bytes'.split(),1)
    reply=rows('DPI_DMA_TILE_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if any(a is None for a in (memory,items,verify,reply)) or lines.count('MAIA_NATIVE_DPI_DMA_TILE_PASS')!=1: return False
    if any(l.startswith('DPI_DMA_TILE_') and l.split()[0] not in ('DPI_DMA_TILE_MEMORY','DPI_DMA_TILE_ITEM','DPI_DMA_TILE_VERIFY','DPI_DMA_TILE_REPLY') for l in lines): return False
    m=memory[0];v=verify[0]
    if not (0x4ff00000<=m['start']<m['end']<=0x4ff28000 and not m['start']&127 and m['end']-m['start']==22272 and
            m['alias']==m['start']+0x40000000 and m['source']==m['start']+128 and m['list']==m['start']+11904 and
            m['source_bytes']==11520 and m['count']==160 and m['list_bytes']==10240 and m['frame_bytes']==1843200 and
            m['cache_line'] in (16,32,64,128) and m['prepared']==1): return False
    for i,r in zip((0,80,159),items):
        if not (r['index']==i and r['source']==m['source'] and r['destination']==0x50105000 and r['items']==1440 and
                r['next']==((m['list']+(i+1)*64)|1 if i<159 else 0) and r['ctl0']==0x1e1b41 and
                r['ctl1']==(0x80108840|(0x40000000 if i==159 else 0)) and r['zero']==1): return False
    if not ((v['source_hash'],v['list_hash'])==dpi_dma_tile_hashes(m['source'],m['list']) and
            v['guards']==v['preserved']==v['inert']==1 and v['repeated_bytes']==m['frame_bytes']): return False
    for i,r in enumerate(reply):
        if not (r['index']==i and r['result']==0 and r['stage']==10 and r['headers']==3 and 3<=r['polls']<=10000 and
                r['words']==1 and r['word']&255==reply[0]['word']&255 and r['word']&0x14==0x14 and r['before']==r['after'] and
                not r['before']&3 and r['restored']==5 and r['status']&0x97==0x95 and r['fifo']&0xf007f==0x50015 and
                not r['error0']&0x1fffff and not r['error1']&0x81fff): return False
    positions=[i for i,l in enumerate(lines) if l.startswith(('DPI_DMA_TILE_MEMORY ','DPI_DMA_TILE_ITEM ','DPI_DMA_TILE_VERIFY ','DPI_DMA_TILE_REPLY '))]
    return ([lines[i].split()[0] for i in positions]==['DPI_DMA_TILE_MEMORY']+['DPI_DMA_TILE_ITEM']*3+['DPI_DMA_TILE_VERIFY']+['DPI_DMA_TILE_REPLY']*4 and
            lines.index('MAIA_NATIVE_DPI_BRIDGE_PASS')<positions[0] and positions[-1]<lines.index('MAIA_NATIVE_DPI_DMA_TILE_PASS'))


def evaluate_dpi_dma_stage(output,allow_frame=False):
    if not allow_frame and "DPI_DMA_FRAME" in output:return False
    if not evaluate_dpi_dma_tile(output,allow_stage=True):return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def rows(prefix,keys,n):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=n or sum(l.startswith(prefix+' ') for l in lines)!=n:return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    stage=rows('DPI_DMA_STAGE','result stage attempted source frame_bytes channel_status common_status status fifo error0 error1'.split(),1)
    words=rows('DPI_DMA_STAGE_WORD','index before after'.split(),16)
    idle=rows('DPI_DMA_STAGE_IDLE','mode lp dpi cfg channels abort source_hash list_hash preserved duplicate_busy'.split(),1)
    reply=rows('DPI_DMA_STAGE_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if any(a is None for a in (stage,words,idle,reply)) or lines.count('MAIA_NATIVE_DPI_DMA_STAGE_PASS')!=1:return False
    if any(l.startswith('DPI_DMA_STAGE_') and l.split()[0] not in ('DPI_DMA_STAGE_WORD','DPI_DMA_STAGE_IDLE','DPI_DMA_STAGE_REPLY') for l in lines):return False
    a=stage[0];q=idle[0]
    source=int(re.search(r'^DPI_DMA_TILE_MEMORY .* source=([0-9a-f]{8})',clean,re.M)[1],16)
    hashes=dpi_dma_tile_hashes(source,source-128+11904)
    if not(a['result']==0 and a['stage']==18 and a['source']==source and a['frame_bytes']==1843200 and not a['channel_status'] and not a['common_status'] and
           a['status']&0x97==0x95 and a['fifo']&0xf007f==0x50015 and not a['error0']&0x1fffff and not a['error1']&0x81fff and
           q['mode']==1 and not q['lp']&3 and not q['dpi']&1 and q['cfg']==q['channels']==q['abort']==0 and
           (q['source_hash'],q['list_hash'])==hashes and q['preserved']==q['duplicate_busy']==1):return False
    masks=[0xffffffff]*4+[0x3fffff,0x7fffff55,0xc7ffffff,15,0x7f8e319f,0xffffffc1]+[0xffffffff]+[0xf83f7ffb,0xf1,0x10000000,0x7ff,0xfff]
    values=[source,0,0x50105000,0,230399,0x1c1b51,0xf8000,0,0xa020001]+[0]*3+[0x10,0,768,256]
    attempted=0
    for i,r in enumerate(words):
        wanted=(r['before']&~masks[i])|values[i]
        if r['index']!=i or r['after']!=wanted:return False
        if r['before']!=wanted:attempted|=1<<i
    if attempted!=a['attempted']:return False
    for i,r in enumerate(reply):
        if not(r['index']==i and r['result']==0 and r['stage']==10 and r['headers']==3 and 3<=r['polls']<=10000 and r['words']==1 and
               r['word']&255==reply[0]['word']&255 and r['word']&0x14==0x14 and r['before']==r['after'] and not r['before']&3 and r['restored']==5 and
               r['status']&0x97==0x95 and r['fifo']&0xf007f==0x50015 and not r['error0']&0x1fffff and not r['error1']&0x81fff):return False
    positions=[i for i,l in enumerate(lines) if l.startswith(('DPI_DMA_STAGE ','DPI_DMA_STAGE_WORD ','DPI_DMA_STAGE_IDLE ','DPI_DMA_STAGE_REPLY '))]
    return([lines[i].split()[0] for i in positions]==['DPI_DMA_STAGE']+['DPI_DMA_STAGE_WORD']*16+['DPI_DMA_STAGE_IDLE']+['DPI_DMA_STAGE_REPLY']*4 and
           lines.index('MAIA_NATIVE_DPI_DMA_TILE_PASS')<positions[0] and positions[-1]<lines.index('MAIA_NATIVE_DPI_DMA_STAGE_PASS'))


def evaluate_dpi_spatial(output,center=False):
    if center:
        if 'DPI_SPATIAL' in output or 'DPI_DMA_TILE' in output or not evaluate_dpi_bridge(output):return False
    elif 'DPI_CENTER' in output or not evaluate_dpi_dma_tile(output,allow_spatial=True):return False
    tag='DPI_CENTER' if center else 'DPI_SPATIAL'
    marker='MAIA_NATIVE_DPI_CENTER_PASS' if center else 'MAIA_NATIVE_DPI_SPATIAL_PASS'
    clean=output.replace('\r','');lines=clean.splitlines()
    def rows(prefix,keys,n):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=n or sum(l.startswith(prefix+' ') for l in lines)!=n:return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    memory=rows(tag+'_MEMORY','source list nodes source_bytes list_bytes frame_bytes'.split(),1)
    report=rows(tag,'result stage frame_stage attempted frames polls items rearms active done count_raw status common underrun dma_fifo bridge_fifo dsi_status dsi_fifo error0 error1 transfer recovery recovered elapsed_us'.split(),1)
    words=rows(tag+'_WORD','index before after'.split(),80)
    restored=rows(tag+'_RESTORE','light mode lp dpi cfg channels abort source_hash source_kept tail_kept guards preserved duplicate_busy'.split(),1)
    reply=rows(tag+'_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if any(a is None for a in (memory,report,words,restored,reply)) or lines.count(marker)!=1:return False
    m,a,q=memory[0],report[0],restored[0]
    if center:
        buffers=rows('DPI_CENTER_BUFFER','start end alias source list source_bytes list_capacity span cache_line prepared'.split(),1)
        checks=rows('DPI_CENTER_BUFFER_VERIFY','source_hash guards preserved inert'.split(),1)
        if buffers is None or checks is None or lines.count('MAIA_NATIVE_DPI_CENTER_BUFFER_PASS')!=1:return False
        b=buffers[0];base=b['start'];source=b['source'];list_address=b['list']
        if not(base%128==0 and 0x4ff00000<=base and b['end']==base+46848<=0x4ff28000 and b['alias']==base+0x40000000 and
               source==base+128 and list_address==base+46336 and b['source_bytes']==46080 and b['list_capacity']==384 and
               b['span']==46848 and b['cache_line'] in (16,32,64,128) and b['prepared']==1):return False
        import struct
        h=2166136261
        for _ in range(32):
            for color in (0xf800,0x07e0,0x001f,0xffe0):
                for value in struct.pack('<H',color)*180:h=((h^value)*16777619)&0xffffffff
        expected_hash=h
        if checks[0]!=dict(source_hash=h,guards=1,preserved=1,inert=1):return False
        if not(lines.index('MAIA_NATIVE_DPI_BRIDGE_PASS')<next(i for i,l in enumerate(lines) if l.startswith('DPI_CENTER_BUFFER '))<next(i for i,l in enumerate(lines) if l.startswith('DPI_CENTER_BUFFER_VERIFY '))<lines.index('MAIA_NATIVE_DPI_CENTER_BUFFER_PASS')):return False
    else:
        prior=re.search(r'^DPI_DMA_TILE_MEMORY .* source=([0-9a-f]{8}) list=([0-9a-f]{8})',clean,re.M)
        source,list_address=(int(v,16) for v in prior.groups());expected_hash=0xe18c2e45
    if m!=dict(source=source,list=list_address,nodes=5,source_bytes=46080 if center else 11520,list_bytes=320,frame_bytes=1843200):return False
    if not(a['result']==0 and a['stage']==18 and a['frame_stage']==9 and a['attempted']==0xfff and a['frames']==400 and 400<=a['polls']<=40000000 and
           a['items']==92160000 and a['rearms']==399 and a['active']==a['done']==a['recovered']==1 and a['count_raw']==57599 and
           not a['status']&0x23f7fe0 and a['common']==a['underrun']==a['dma_fifo']==a['bridge_fifo']==a['error0']==a['error1']==a['transfer']==a['recovery']==0 and
           a['dsi_status']&0x97==0x95 and a['dsi_fifo']&0xf007f==0x50015 and a['elapsed_us']>0):return False
    counts=(57600,54720,5760,54720,57600) if center else (1440,56160,57600,57600,57600)
    offsets=(0,360,0,720,1080) if center else (0,0,360,720,1080)
    increment=2 if center else 0
    for n,items in enumerate(counts):
        expected=[0]*16;expected[0]=source+offsets[n];expected[2]=0x50105000;expected[4]=items-1
        expected[6]=(list_address+(n+1)*64)|1 if n<4 else 0;expected[8]=0x1c1b41 if n==increment else 0x1c1b51;expected[9]=0x800f8040|(0x40000000 if n==4 else 0)
        after=expected.copy();after[9]&=0x7fffffff;after[12]=items-1;after[13]=0xc0000000 if n==4 else 0x40000000
        if any(words[n*16+w]!=dict(index=n*16+w,before=expected[w],after=after[w]) for w in range(16)):return False
    if not(q['light']==q['source_kept']==q['tail_kept']==q['guards']==q['preserved']==q['duplicate_busy']==1 and q['mode']==1 and
           not q['lp']&3 and not q['dpi']&1 and q['cfg']==q['channels']==q['abort']==0 and q['source_hash']==expected_hash):return False
    for i,r in enumerate(reply):
        if not(r['index']==i and r['result']==0 and r['stage']==10 and r['headers']==3 and 3<=r['polls']<=10000 and r['words']==1 and
               r['word']&255==reply[0]['word']&255 and r['word']&0x14==0x14 and r['before']==r['after'] and not r['before']&3 and r['restored']==5 and
               r['status']&0x97==0x95 and r['fifo']&0xf007f==0x50015 and not r['error0']&0x1fffff and not r['error1']&0x81fff):return False
    allowed={tag,tag+'_MEMORY',tag+'_WORD',tag+'_RESTORE',tag+'_REPLY'}
    if any(l.startswith(tag) and l.split()[0] not in allowed|({'DPI_CENTER_BUFFER','DPI_CENTER_BUFFER_VERIFY'} if center else set()) for l in lines):return False
    pos=[i for i,l in enumerate(lines) if l.split() and l.split()[0] in allowed]
    return ([lines[i].split()[0] for i in pos]==[tag+'_MEMORY',tag]+[tag+'_WORD']*80+[tag+'_RESTORE']+[tag+'_REPLY']*4 and
            lines.index('MAIA_NATIVE_DPI_CENTER_BUFFER_PASS' if center else 'MAIA_NATIVE_DPI_DMA_TILE_PASS')<pos[0] and pos[-1]<lines.index(marker))


def evaluate_dpi_dma_frame(output,allow_refresh=False):
    if not allow_refresh and "DPI_DMA_REFRESH" in output:return False
    if not evaluate_dpi_dma_stage(output,allow_frame=True):return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def rows(prefix,keys,n):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=n or sum(l.startswith(prefix+' ') for l in lines)!=n:return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    light=rows('DPI_DMA_FRAME_LIGHT','out enable pad mux matrix'.split(),1)
    frame=rows('DPI_DMA_FRAME','result stage attempted polls recovery_polls active done count_raw items channel_status common_status bridge_status dma_fifo bridge_fifo recovered transfer_result recovery_result dsi_status dsi_fifo error0 error1 elapsed_us'.split(),1)
    restored=rows('DPI_DMA_FRAME_RESTORE','light mode lp dpi cfg channels abort source_hash list_hash preserved duplicate_busy'.split(),1)
    reply=rows('DPI_DMA_FRAME_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if any(a is None for a in (light,frame,restored,reply)) or lines.count('MAIA_NATIVE_DPI_DMA_FRAME_PASS')!=1:return False
    if any(l.startswith('DPI_DMA_FRAME_') and l.split()[0] not in ('DPI_DMA_FRAME_LIGHT','DPI_DMA_FRAME_RESTORE','DPI_DMA_FRAME_REPLY') for l in lines):return False
    a=frame[0];q=restored[0];z=light[0]
    source=int(re.search(r'^DPI_DMA_TILE_MEMORY .* source=([0-9a-f]{8})',clean,re.M)[1],16)
    if not(z['out']&0x400000 and z['enable']&0x400000 and not z['pad']&4 and z['mux']&0x7000==0x1000 and z['matrix']&0xfff==0x500):return False
    if not(a['result']==0 and a['stage']==9 and a['attempted']==0xfff and 1<=a['polls']<=100000 and 2<=a['recovery_polls']<=200000 and
           a['active']==a['done']==a['recovered']==1 and a['count_raw']==230399 and a['items']==230400 and a['channel_status']&2 and not a['channel_status']&0x23f7fe0 and
           a['common_status']==a['bridge_status']==a['dma_fifo']==a['bridge_fifo']==a['transfer_result']==a['recovery_result']==0 and
           a['dsi_status']&0x97==0x95 and a['dsi_fifo']&0xf007f==0x50015 and not a['error0']&0x1fffff and not a['error1']&0x81fff and a['elapsed_us']>0):return False
    if not(q['light']==q['preserved']==q['duplicate_busy']==1 and q['mode']==1 and not q['lp']&3 and not q['dpi']&1 and q['cfg']==q['channels']==q['abort']==0 and
           (q['source_hash'],q['list_hash'])==dpi_dma_tile_hashes(source,source-128+11904)):return False
    for i,r in enumerate(reply):
        if not(r['index']==i and r['result']==0 and r['stage']==10 and r['headers']==3 and 3<=r['polls']<=10000 and r['words']==1 and
               r['word']&255==reply[0]['word']&255 and r['word']&0x14==0x14 and r['before']==r['after'] and not r['before']&3 and r['restored']==5 and
               r['status']&0x97==0x95 and r['fifo']&0xf007f==0x50015 and not r['error0']&0x1fffff and not r['error1']&0x81fff):return False
    pos=[i for i,l in enumerate(lines) if l.startswith(('DPI_DMA_FRAME_LIGHT ','DPI_DMA_FRAME ','DPI_DMA_FRAME_RESTORE ','DPI_DMA_FRAME_REPLY '))]
    return([lines[i].split()[0] for i in pos]==['DPI_DMA_FRAME_LIGHT','DPI_DMA_FRAME','DPI_DMA_FRAME_RESTORE']+['DPI_DMA_FRAME_REPLY']*4 and
           lines.index('MAIA_NATIVE_DPI_DMA_STAGE_PASS')<pos[0] and pos[-1]<lines.index('MAIA_NATIVE_DPI_DMA_FRAME_PASS'))


def evaluate_dpi_dma_refresh(output):
    if not evaluate_dpi_dma_frame(output,allow_refresh=True):return False
    clean=output.replace('\r','');lines=clean.splitlines()
    rows=re.findall(r'^DPI_DMA_REFRESH frames=([0-9a-f]{8}) polls=([0-9a-f]{8}) items=([0-9a-f]{8}) rearms=([0-9a-f]{8})$',clean,re.M)
    if len(rows)!=1 or sum(l.startswith('DPI_DMA_REFRESH') for l in lines)!=1:return False
    frames,polls,items,rearms=(int(v,16) for v in rows[0])
    if not(frames==400 and 400<=polls<=40000000 and items==92160000 and rearms==399):return False
    at=next(i for i,l in enumerate(lines) if l.startswith('DPI_DMA_REFRESH '))
    return next(i for i,l in enumerate(lines) if l.startswith('DPI_DMA_FRAME '))<at<next(i for i,l in enumerate(lines) if l.startswith('DPI_DMA_FRAME_RESTORE '))


def evaluate_dpi_pattern(output):
    if not evaluate_dpi_clock(output,allow_pattern=True): return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def rows(prefix,keys,n):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=n or sum(l.startswith(prefix+' ') for l in lines)!=n: return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    light=rows('DPI_PATTERN_LIGHT','out_before enable_before pad_before mux_before matrix_before out enable pad mux matrix'.split(),1)
    if light is None or lines.count('DPI_PATTERN_LIGHT_RESTORE verified=00000001')!=1: return False
    z=light[0]
    if not (z['out']==z['out_before']|0x400000 and z['enable']==z['enable_before']|0x400000 and
            z['pad']==z['pad_before']&~4 and z['mux']==(z['mux_before']&~0x7000)|0x1000 and
            z['matrix']==(z['matrix_before']&~0xfff)|0x500): return False
    keys='result stage attempted pattern mode lp shadow polls status fifo error0 error1'.split()
    start=rows('DPI_PATTERN_START',keys,1);stop=rows('DPI_PATTERN_STOP',keys,1)
    hold=rows('DPI_PATTERN_HOLD','index mode pattern lp status fifo error0 error1 inactive exclusive'.split(),5)
    reply=rows('DPI_PATTERN_REPLY','index result stage headers polls word words before after restored status fifo error0 error1'.split(),4)
    if any(a is None for a in (start,stop,hold,reply)) or lines.count('MAIA_NATIVE_DPI_PATTERN_PASS')!=1: return False
    if any(l.startswith('DPI_PATTERN_') and l.split()[0] not in ('DPI_PATTERN_LIGHT','DPI_PATTERN_LIGHT_RESTORE','DPI_PATTERN_START','DPI_PATTERN_HOLD','DPI_PATTERN_STOP','DPI_PATTERN_REPLY') for l in lines): return False
    a=start[0];b=stop[0]
    def errors(r): return not (r['error0']&0x1fffff or r['error1']&0x81fff) and r['status']&1
    if not (a['result']==0 and a['stage']==4 and a['attempted']==7 and a['pattern']&0x111ffff==0x1bf02 and
            a['mode']==0 and a['lp']&3==3 and a['shadow']==0 and a['polls']==0 and errors(a)): return False
    if not (b['result']==0 and b['stage']==5 and b['attempted']==7 and b['pattern']==a['pattern']&~0x1110000 and
            b['mode']==1 and b['lp']==a['lp']&~3 and b['shadow']==0 and 1<=b['polls']<=10000 and errors(b) and
            b['status']&0x97==0x95 and b['fifo']&0xf007f==0x50015): return False
    for i,r in enumerate(hold):
        if not (r['index']==i and r['mode']==a['mode'] and r['pattern']==a['pattern'] and r['lp']==a['lp'] and
                r['inactive']==r['exclusive']==1 and errors(r)): return False
    for i,r in enumerate(reply):
        if not (r['index']==i and r['result']==0 and r['stage']==10 and r['headers']==3 and 3<=r['polls']<=10000 and
                r['words']==1 and r['word']&255==reply[0]['word']&255 and r['word']&0x14==0x14 and
                r['before']==r['after'] and not r['before']&3 and r['restored']==5 and r['status']&0x97==0x95 and
                r['fifo']&0xf007f==0x50015 and errors(r)): return False
    positions=[i for i,l in enumerate(lines) if l.startswith(('DPI_PATTERN_START ','DPI_PATTERN_HOLD ','DPI_PATTERN_STOP ','DPI_PATTERN_REPLY '))]
    names=[lines[i].split()[0] for i in positions]
    return (names==['DPI_PATTERN_START']+['DPI_PATTERN_HOLD']*5+['DPI_PATTERN_STOP']+['DPI_PATTERN_REPLY']*4 and
            lines.index('MAIA_NATIVE_DPI_CLOCK_PASS')<next(i for i,l in enumerate(lines) if l.startswith('DPI_PATTERN_LIGHT '))<positions[0] and
            positions[6]<lines.index('DPI_PATTERN_LIGHT_RESTORE verified=00000001')<positions[7] and positions[-1]<lines.index('MAIA_NATIVE_DPI_PATTERN_PASS'))


def evaluate_psram_state(output,allow_identity=False,allow_clock=False,allow_array=False,allow_window=False):
    clean=output.replace('\r','');lines=clean.splitlines()
    if 'FAIL' in clean or 'FATAL' in clean or lines.count('MAIA_NATIVE_PSRAM_STATE_OBSERVED')!=1:return False
    def rows(prefix,keys):
        selected=[l for l in lines if l.startswith(prefix+' ')]
        pattern='^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$'
        parsed=[re.fullmatch(pattern,l) for l in selected]
        if any(r is None for r in parsed):return None
        return [dict(zip(keys,(int(v,16) for v in r.groups()))) for r in parsed]
    meta=rows('PSRAM_STATE','result roots_valid samples'.split());roots=rows('PSRAM_ROOT','index address before after'.split());words=rows('PSRAM_WORD','index address before after'.split())
    if meta is None or roots is None or words is None or len(meta)!=1 or len(roots)!=9:return False
    addresses=(0x500e6014,0x500e6030,0x500e60c0,0x500e60cc,0x501151d0,0x501151d4,0x5012d04c,0x5012d050,0x5012d1c0)
    if any(r['index']!=i or r['address']!=address or r['before']!=r['after'] for i,(r,address) in enumerate(zip(roots,addresses))):return False
    a=meta[0];accessible=bool(roots[0]['before']&0x80000000 and roots[1]['before']&0xc000==0xc000 and not roots[2]['before']&0x02800000)
    if a['roots_valid']!=1 or a['result']!=(0 if accessible else 0xfffffffd) or a['samples']!=(54 if accessible else 0):return False
    offsets=(0,8,12,64,68,72,76,80,216,384,388,392,396,400,512,304,0,8,12,16,20,24,28,32,212,384,512)
    if len(words)!=(27 if accessible else 0):return False
    if any(w['index']!=i or w['address']!=(0x5008e000 if i<16 else 0x5008f000)+offsets[i] or w['before']!=w['after'] for i,w in enumerate(words)):return False
    allowed={'PSRAM_STATE','PSRAM_ROOT','PSRAM_WORD'}
    extra={'PSRAM_ID','PSRAM_ID_WORD','PSRAM_ID_RESTORE'} if allow_identity else set()
    if allow_clock:extra|={'PSRAM_CLOCK','PSRAM_CLOCK_WORD','PSRAM_CLOCK_FINAL','PSRAM_AXI','PSRAM_AXI_FINAL'}
    if allow_array:extra|={'PSRAM_ARRAY_TRANSFER','PSRAM_ARRAY_PATTERN','PSRAM_ARRAY_RESTORE','PSRAM_ARRAY_MODE'}
    if allow_window:extra|={'PSRAM_CACHE_BOOT_START','PSRAM_CACHE_BOOT','PSRAM_WINDOW_MODE','PSRAM_WINDOW_RAW','PSRAM_WINDOW','PSRAM_WINDOW_MMU','PSRAM_WINDOW_DATA','PSRAM_WINDOW_DIRECT','PSRAM_WINDOW_RESTORE'}
    if any(l.startswith('PSRAM_') and l.split()[0] not in allowed|extra for l in lines):return False
    positions=[i for i,l in enumerate(lines) if l.split() and l.split()[0] in allowed]
    return ([lines[i].split()[0] for i in positions]==['PSRAM_STATE']+['PSRAM_ROOT']*9+['PSRAM_WORD']*len(words) and positions[-1]<lines.index('MAIA_NATIVE_PSRAM_STATE_OBSERVED'))


def evaluate_psram_identity(output,allow_clock=False,allow_array=False,allow_window=False):
    if not evaluate_psram_state(output,allow_identity=True,allow_clock=allow_clock,allow_array=allow_array,allow_window=allow_window):return False
    if 'PSRAM_STATE result=00000000 roots_valid=00000001 samples=00000036' not in output:return False
    clean=output.replace('\r','');lines=clean.splitlines()
    def rows(prefix,keys,n):
        found=re.findall('^'+prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys)+'$',clean,re.M)
        if len(found)!=n or sum(l.startswith(prefix+' ') for l in lines)!=n:return None
        return [dict(zip(keys,(int(v,16) for v in a))) for a in found]
    read=rows('PSRAM_ID','index reg result stage bits setup restore triggered polls busy command value restored'.split(),8)
    words=rows('PSRAM_ID_WORD','read index before configured'.split(),80)
    restore=rows('PSRAM_ID_RESTORE','result roots words vendor density device kgd x16'.split(),1)
    if any(x is None for x in (read,words,restore)) or lines.count('MAIA_NATIVE_PSRAM_ID_PASS')!=1:return False
    for n,row in enumerate(read):
        reg=(0,2,4,8)[n%4];bits=16 if reg<4 else 8
        if not(row['index']==n and row['reg']==reg and row['result']==0 and row['stage']==5 and row['bits']==bits and row['setup']==row['restore']==0x3ff and
               row['triggered']==row['restored']==1 and 1<=row['polls']<=10000 and row['busy'] in (0,1) and row['command']==0 and row['value']<(1<<bits) and
               row['value']!=(0x5aa5 if bits==16 else 0xa5) and (n<4 or row['value']==read[n%4]['value'])):return False
        masks=(0x2270,0xff00f000,0xfc00003f,0xf000ffff,0xffffffff,0x3ff,0x403,2,0x11,0xffffffff)
        values=(0x2270,0xf0000000,0x7c00000b,0xf0004040,reg,bits-1,1,2,1,0xa55a5aa5)
        for i,(mask,value) in enumerate(zip(masks,values)):
            w=words[n*10+i]
            if w['read']!=n or w['index']!=i or w['configured']!=((w['before']&~mask)|value):return False
        if n:
            if any(words[n*10+i]['before']!=words[i]['before'] for i in range(10)):return False
    if not((read[0]['value']>>8)&31==13 and read[1]['value']&7==7 and (read[1]['value']>>3)&3==3 and (read[1]['value']>>5)&7==6 and read[3]['value']&64):return False
    if restore[0]!=dict(result=0,roots=1,words=1,vendor=13,density=7,device=3,kgd=6,x16=1):return False
    pos=[i for i,l in enumerate(lines) if l.startswith(('PSRAM_ID ', 'PSRAM_ID_WORD ', 'PSRAM_ID_RESTORE '))]
    return ([lines[i].split()[0] for i in pos]==(['PSRAM_ID']+['PSRAM_ID_WORD']*10)*8+['PSRAM_ID_RESTORE'] and
            lines.index('MAIA_NATIVE_PSRAM_STATE_OBSERVED')<pos[0] and pos[-1]<lines.index('MAIA_NATIVE_PSRAM_ID_PASS'))


def evaluate_psram_clock(output,allow_array=False,allow_window=False):
    if not evaluate_psram_identity(output,allow_clock=True,allow_array=allow_array,allow_window=allow_window):return False
    lines=output.replace('\r','').splitlines()
    if lines.count('MAIA_NATIVE_PSRAM_CLOCK_HOST_READY')!=1:return False
    def rows(prefix,keys,n):
        selected=[l for l in lines if l.startswith(prefix+' ')]
        parsed=[re.fullmatch(prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys),l) for l in selected]
        if len(parsed)!=n or any(r is None for r in parsed):return None
        return [dict(zip(keys,(int(v,16) for v in r.groups()))) for r in parsed]
    axi=rows('PSRAM_AXI',['control'],1);axi_final=rows('PSRAM_AXI_FINAL',['control'],1)
    if axi is None or axi_final is None or axi!=axi_final or axi[0]['control']&0x80000001!=0x80000000:return False
    phases=rows('PSRAM_CLOCK','phase result stage attempts restored'.split(),2)
    words=rows('PSRAM_CLOCK_WORD','phase index before after'.split(),6)
    final=rows('PSRAM_CLOCK_FINAL','result roots words'.split(),1)
    roots=rows('PSRAM_ROOT','index address before after'.split(),9)
    state=rows('PSRAM_WORD','index address before after'.split(),27)
    if any(r is None for r in (phases,words,final,roots,state)):return False
    if lines.count('MAIA_NATIVE_PSRAM_CLOCK_PASS')!=1:return False
    # Qualified supply fields, not a physical voltage/frequency measurement.
    control,analog,sys2,sys3,errors=[roots[i]['before'] for i in (4,5,6,7,8)]
    d,m=sys2>>28,(sys3>>3)&7
    if not(d and m and (sys2>>4)&3==1 and sys2&15==3 and not sys2&0xc0 and not (sys2>>11)&3 and
           (sys2>>8)&7 and not errors&8 and control&0x4f80==0x180 and (analog>>28)&15==d and (analog>>23)&7==m):return False
    peri=roots[1]['before'];before=(peri,0x30103,0x30103);active=(peri&~0xff3000,0x10001,0x10001)
    if peri&0xff3000!=0x1000 or state[0]['before'] or state[16]['before'] or state[2]['before']&1:return False
    if state[7]['before']!=0x30103 or state[20]['before']!=0x30103:return False
    for phase in range(2):
        if phases[phase]!=dict(phase=phase,result=0,stage=5,attempts=31,restored=phase):return False
        for i in range(3):
            if words[phase*3+i]!=dict(phase=phase,index=i,before=before[i],after=(active if phase==0 else before)[i]):return False
    if final[0]!=dict(result=0,roots=1,words=1):return False
    names=[l.split()[0] for l in lines if l.startswith(('PSRAM_CLOCK ', 'PSRAM_CLOCK_WORD ', 'PSRAM_CLOCK_FINAL '))]
    if names!=['PSRAM_CLOCK']+['PSRAM_CLOCK_WORD']*3+['PSRAM_CLOCK']+['PSRAM_CLOCK_WORD']*3+['PSRAM_CLOCK_FINAL']:return False
    ordered=[l.split()[0] for l in lines if l.startswith(('PSRAM_CLOCK ', 'PSRAM_CLOCK_WORD ', 'PSRAM_CLOCK_FINAL ', 'PSRAM_ID ', 'PSRAM_ID_WORD ', 'PSRAM_ID_RESTORE '))]
    if ordered!=['PSRAM_CLOCK']+['PSRAM_CLOCK_WORD']*3+(['PSRAM_ID']+['PSRAM_ID_WORD']*10)*8+['PSRAM_ID_RESTORE','PSRAM_CLOCK']+['PSRAM_CLOCK_WORD']*3+['PSRAM_CLOCK_FINAL']:return False
    positions={name:next(i for i,l in enumerate(lines) if l.startswith(name)) for name in ('PSRAM_CLOCK phase=00000000','PSRAM_CLOCK phase=00000001','PSRAM_ID index=00000000','PSRAM_CLOCK_FINAL')}
    return (lines.index('MAIA_NATIVE_PSRAM_CLOCK_HOST_READY')<next(i for i,l in enumerate(lines) if l.startswith('PSRAM_STATE ')) and
            lines.index('MAIA_NATIVE_PSRAM_STATE_OBSERVED')<positions['PSRAM_CLOCK phase=00000000']<positions['PSRAM_ID index=00000000'] and
            lines.index('MAIA_NATIVE_PSRAM_ID_PASS')<positions['PSRAM_CLOCK phase=00000001']<positions['PSRAM_CLOCK_FINAL']<lines.index('MAIA_NATIVE_PSRAM_CLOCK_PASS'))


def evaluate_psram_array(output,allow_window=False):
    if not evaluate_psram_clock(output,allow_array=True,allow_window=allow_window):return False
    lines=output.replace('\r','').splitlines()
    if lines.count('MAIA_NATIVE_PSRAM_ARRAY_PASS')!=1:return False
    def rows(prefix,keys,n):
        selected=[l for l in lines if l.startswith(prefix+' ')]
        parsed=[re.fullmatch(prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys),l) for l in selected]
        if len(parsed)!=n or any(r is None for r in parsed):return None
        return [dict(zip(keys,(int(v,16) for v in r.groups()))) for r in parsed]
    modes=rows('PSRAM_ARRAY_MODE','phase result stage before after'.split(),2)
    transfers=rows('PSRAM_ARRAY_TRANSFER','index write result completed chunks polls command restored attempted'.split(),11)
    patterns=rows('PSRAM_ARRAY_PATTERN','index matched expected actual'.split(),4)
    restore=rows('PSRAM_ARRAY_RESTORE','matched before after guards'.split(),1)
    if any(r is None for r in (modes,transfers,patterns,restore)):return False
    if modes!=[dict(phase=0,result=0,stage=3,before=0x4b,after=0x0b),dict(phase=1,result=0,stage=3,before=0x0b,after=0x4b)]:return False
    for i,t in enumerate(transfers):
        write=int(i%2==1)
        if not(t['index']==i and t['write']==write and t['result']==0 and t['completed']==4096 and t['chunks']==64 and
               64<=t['polls']<=640000 and t['command']==0 and t['restored']==1 and t['attempted']==write):return False
    for i,p in enumerate(patterns):
        h=2166136261;state=0x783a1b25
        for j in range(4096):
            state=(state*1664525+1013904223)&0xffffffff
            b=0 if i==0 else 255 if i==1 else (0x55 if j&1 else 0xaa) if i==2 else state>>24
            h=((h^b)*16777619)&0xffffffff
        if p!=dict(index=i,matched=4096,expected=h,actual=h):return False
    r=restore[0]
    if r['matched']!=4096 or r['before']!=r['after'] or r['guards']!=1:return False
    names=[l.split()[0] for l in lines if l.startswith(('PSRAM_ARRAY_TRANSFER ','PSRAM_ARRAY_PATTERN ','PSRAM_ARRAY_RESTORE ','PSRAM_ARRAY_MODE '))]
    if names!=['PSRAM_ARRAY_MODE','PSRAM_ARRAY_TRANSFER']+['PSRAM_ARRAY_TRANSFER','PSRAM_ARRAY_TRANSFER','PSRAM_ARRAY_PATTERN']*4+['PSRAM_ARRAY_TRANSFER','PSRAM_ARRAY_TRANSFER','PSRAM_ARRAY_RESTORE','PSRAM_ARRAY_MODE']:return False
    positions=[j for j,l in enumerate(lines) if l.startswith('PSRAM_ARRAY_')]
    return (lines.index('MAIA_NATIVE_PSRAM_ID_PASS')<positions[0] and positions[-1]<lines.index('MAIA_NATIVE_PSRAM_ARRAY_PASS')<
            next(j for j,l in enumerate(lines) if l.startswith('PSRAM_CLOCK phase=00000001')))


def evaluate_psram_window(output):
    if not evaluate_psram_array(output,allow_window=True):return False
    lines=output.replace('\r','').splitlines()
    if lines.count('MAIA_NATIVE_PSRAM_WINDOW_PASS')!=1 or lines.count('PSRAM_CACHE_BOOT_START')!=1:return False
    def rows(prefix,keys,n):
        selected=[l for l in lines if l.startswith(prefix+' ')]
        parsed=[re.fullmatch(prefix+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys),l) for l in selected]
        if len(parsed)!=n or any(r is None for r in parsed):return None
        return [dict(zip(keys,(int(v,16) for v in r.groups()))) for r in parsed]
    cache=rows('PSRAM_CACHE_BOOT','result stage size line ways'.split(),1)
    modes=rows('PSRAM_WINDOW_MODE','phase result stage before after'.split(),2)
    raw=rows('PSRAM_WINDOW_RAW','phase result write completed chunks restored'.split(),4)
    window=rows('PSRAM_WINDOW','phase result stage vaddr bytes physical restored polls'.split(),3)
    mmu=rows('PSRAM_WINDOW_MMU','phase index before after'.split(),192)
    data=rows('PSRAM_WINDOW_DATA','words matched expected actual'.split(),1)
    direct=rows('PSRAM_WINDOW_DIRECT',['matched'],1);restore=rows('PSRAM_WINDOW_RESTORE',['matched'],1)
    if any(r is None for r in (cache,modes,raw,window,mmu,data,direct,restore)):return False
    c=cache[0]
    if not(c['result']==0 and c['stage']==3 and c['size'] in (131072,262144,524288) and c['line'] in (64,128) and c['ways'] in (4,8)):return False
    if modes!=[dict(phase=0,result=0,stage=3,before=0x4b,after=0x0b),dict(phase=1,result=0,stage=3,before=0x0b,after=0x4b)]:return False
    if any(t!=dict(phase=i,result=0,write=int(i==2),completed=4096,chunks=64,restored=1) for i,t in enumerate(raw)):return False
    for i,w in enumerate(window):
        if not(w['phase']==i and w['result']==0 and w['stage']==(4,5,6)[i] and w['vaddr']==0x4b800000 and
               w['bytes']==0x400000 and w['physical']==0 and w['restored']==int(i==2) and 1<=w['polls']<=20000):return False
        for j in range(64):
            m=mmu[i*64+j]
            if m['phase']!=i or m['index']!=j or m['before']!=mmu[j]['before'] or m['after']!=(m['before'] if i==2 else 0xc00+j):return False
    if data[0]!=dict(words=0x100000,matched=0x100000,expected=0x3e80e5d7,actual=0x3e80e5d7):return False
    if direct!=[dict(matched=4096)] or restore!=[dict(matched=4096)]:return False
    names=[l.split()[0] for l in lines if l.startswith('PSRAM_WINDOW')]
    expected=['PSRAM_WINDOW_MODE','PSRAM_WINDOW_RAW','PSRAM_WINDOW']+['PSRAM_WINDOW_MMU']*64
    expected+=['PSRAM_WINDOW']+['PSRAM_WINDOW_MMU']*64+['PSRAM_WINDOW_DATA','PSRAM_WINDOW']+['PSRAM_WINDOW_MMU']*64
    expected+=['PSRAM_WINDOW_RAW','PSRAM_WINDOW_DIRECT','PSRAM_WINDOW_RAW','PSRAM_WINDOW_RAW','PSRAM_WINDOW_RESTORE','PSRAM_WINDOW_MODE']
    if names!=expected:return False
    positions=[i for i,l in enumerate(lines) if l.startswith('PSRAM_WINDOW')]
    return (lines.index('MAIA_NATIVE_PSRAM_CLOCK_HOST_READY')<lines.index('PSRAM_CACHE_BOOT_START')<
            next(i for i,l in enumerate(lines) if l.startswith('PSRAM_CACHE_BOOT '))<next(i for i,l in enumerate(lines) if l.startswith('PSRAM_STATE ')) and
            lines.index('MAIA_NATIVE_PSRAM_ARRAY_PASS')<positions[0] and positions[-1]<lines.index('MAIA_NATIVE_PSRAM_WINDOW_PASS')<
            next(i for i,l in enumerate(lines) if l.startswith('PSRAM_CLOCK phase=00000001')))


def evaluate_psram_dma(output):
    lines=output.replace('\r','').splitlines()
    dma=[l for l in lines if l.startswith('PSRAM_DMA')]
    clean='\n'.join(l for l in lines if not l.startswith('PSRAM_DMA'))+'\n'
    if not evaluate_psram_window(clean) or len(dma)!=10:return False
    keys='index offset bytes burst done polls matched guards raw status common us'.split()
    for i,line in enumerate(dma[:9]):
        m=re.fullmatch('PSRAM_DMA_COPY'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys),line)
        if m is None:return False
        a=dict(zip(keys,(int(v,16) for v in m.groups())))
        if not(a['index']==i and a['offset']==(0,65408,0x1ff000)[i//3] and a['bytes']==4096 and
               a['burst']==(1,4,16)[i%3] and a['done']==1 and 1<=a['polls']<=1000000 and
               a['matched']==4096 and a['guards']==1 and a['raw']==511 and not(a['status']&0x023f7fe0) and not a['common'] and a['us']):return False
    if dma[9]!='PSRAM_DMA_FINAL cfg=00000000 channels=00000000 fifo=00000000 restored=00000001 preserved=00000001':return False
    positions=[i for i,l in enumerate(lines) if l.startswith('PSRAM_DMA')]
    before=next(i for i,l in enumerate(lines) if l.startswith('PSRAM_WINDOW_DATA '))
    after=next(i for i,l in enumerate(lines) if l.startswith('PSRAM_WINDOW phase=00000002 '))
    return before<positions[0] and positions==list(range(positions[0],positions[0]+10)) and positions[-1]<after


def evaluate_psram_fast(output,rate):
    if rate not in (100,200):return False
    lines=output.replace('\r','').splitlines();fast=[l for l in lines if l.startswith('PSRAM_FAST')]
    if len(fast)!=2:return False
    tune=[l for l in lines if l.startswith('PSRAM_TUNE')]
    if len(tune)!=6 or tune[0]!='PSRAM_TUNE_REFERENCE result=00000000 matched=00080000 restored=00000001':return False
    mask=0
    for i,line in enumerate(tune[1:5]):
        p=re.fullmatch(r'PSRAM_TUNE_PHASE phase=([0-9a-f]{8}) result=00000000 copies=([0-9a-f]{8}) matched=([0-9a-f]{8}) restored=00000001',line)
        if p is None:return False
        phase,copies,matched=(int(v,16) for v in p.groups())
        if phase!=i or not(0<=copies<=9 and copies*4096<=matched<=36864):return False
        if copies==9:
            if matched!=36864:return False
            mask|=1<<i
        elif matched==36864:return False
    if not mask:return False
    length=best_length=best=0
    for i in range(4):
        length=length+1 if mask&(1<<i) else 0
        if length>best_length:best_length=length;best=i+1-length
    if tune[5]!=f'PSRAM_TUNE_SELECT mask={mask:08x} best={best:08x}':return False
    clean='\n'.join(l for l in lines if not l.startswith(('PSRAM_FAST','PSRAM_TUNE')))+'\n'
    if not evaluate_psram_dma(clean):return False
    keys='result rate stage attempts polls mpll div repeat restored power clock cal peri bus0 bus1 dll0 dll1'.split()
    m=re.fullmatch('PSRAM_FAST_START'+''.join(' '+k+r'=([0-9a-f]{8})' for k in keys),fast[0])
    if m is None:return False
    a=dict(zip(keys,(int(v,16) for v in m.groups())));bus=0x30103 if rate==100 else 0x10001
    if not(a['result']==0 and a['rate']==rate and a['stage']==5 and a['attempts']==31 and 4<=a['polls']<=40000 and
           a['mpll']==400 and a['div']==a['repeat'] and 40*((a['div']>>3)+1)//((a['div']&7)+1)==400 and
           a['restored']==1 and a['power']&0x1000000 and a['clock']&0x10000000 and a['cal']&0x300==0x300 and
           a['peri']==0xd00c and a['bus0']==a['bus1']==bus and a['dll0']==a['dll1']==0x21):return False
    if fast[1]!='PSRAM_FAST_STOP result=00000000 stage=00000005 attempts=0000001f restored=00000001 peri=0000c00c bus0=00010001 bus1=00010001 dll0=00000001 dll1=00000001':return False
    positions=[i for i,l in enumerate(lines) if l.startswith('PSRAM_FAST')]
    tp=[i for i,l in enumerate(lines) if l.startswith('PSRAM_TUNE')]
    return (next(i for i,l in enumerate(lines) if l.startswith('PSRAM_WINDOW_RAW phase=00000000 '))<tp[0]<positions[0]<tp[1] and
            tp[1:]==list(range(tp[1],tp[1]+5)) and tp[-1]<next(i for i,l in enumerate(lines) if l.startswith('PSRAM_WINDOW phase=00000000 ')) and
            next(i for i,l in enumerate(lines) if l.startswith('PSRAM_WINDOW_MMU phase=00000002 index=0000001f '))<positions[1]<
            next(i for i,l in enumerate(lines) if l.startswith('PSRAM_WINDOW_RAW phase=00000001 ')))


def evaluate_vdma_list(output):
    clean=output.replace('\r','');lines=clean.splitlines()
    if 'FAIL' in clean or 'FATAL' in clean or lines.count('MAIA_NATIVE_VDMA_LIST_PASS')!=1:return False
    def rows(prefix,keys,count):
        selected=[l for l in lines if l.startswith(prefix)]
        pattern='^'+prefix+' '+' '.join(k+'=([0-9a-f]{8})' for k in keys)+'$'
        parsed=[re.fullmatch(pattern,l) for l in selected]
        if len(selected)!=count or any(r is None for r in parsed):return None
        return [dict(zip(keys,(int(v,16) for v in r.groups()))) for r in parsed]
    memory=rows('VDMA_LIST_MEMORY','start end source destination list cache_line'.split(),1)
    words=rows('VDMA_LIST_WORD','index before after'.split(),32)
    copy=rows('VDMA_LIST_COPY','done polls matched expected source_kept guards list_valid raw fifo status common'.split(),1)
    idle=rows('VDMA_LIST_IDLE','cfg channels abort preserved'.split(),1)
    if any(r is None for r in (memory,words,copy,idle)):return False
    m,c,z=memory[0],copy[0],idle[0];base=m['start'];src=base+128;dst=base+33152
    if not(base%128==0 and 0x4ff00000<=base and m['end']==base+66048<=0x4ff28000 and
           m['source']==src and m['destination']==dst and m['list']==src+16384 and m['cache_line'] in (16,32,64,128)):return False
    expected=[0]*32
    expected[0]=src;expected[2]=dst;expected[4]=1439;expected[6]=(src+16448)|1;expected[8]=0x1b05;expected[9]=0x80000000
    expected[16]=src+12288;expected[18]=dst+11520;expected[20]=2655;expected[24]=0x1b15;expected[25]=0xc0000040
    after=expected.copy();after[9]&=0x7fffffff;after[25]&=0x7fffffff
    after[12]=1439;after[13]=0x40000000;after[28]=2655;after[29]=0xc0000000
    if any(r!={'index':i,'before':v,'after':after[i]} for i,(r,v) in enumerate(zip(words,expected))):return False
    samples=rows('VDMA_LIST_SAMPLE','offset word'.split(),16)
    if samples is None or any(row!=dict(offset=i*4,word=0x84108410) for i,row in enumerate(samples)):return False
    if not(c['done']==1 and 1<=c['polls']<=1000000 and c['matched']==c['expected']==32768 and
           c['source_kept']==c['guards']==c['list_valid']==1 and c['raw']<=0x3fffff and c['fifo']==0 and
           not c['status']&0x23f7fe0 and c['common']==0 and z==dict(cfg=0,channels=0,abort=0,preserved=1)):return False
    positions=[i for i,l in enumerate(lines) if l.startswith('VDMA_LIST_')]
    return ([lines[i].split()[0] for i in positions]==['VDMA_LIST_MEMORY']+['VDMA_LIST_SAMPLE']*16+['VDMA_LIST_WORD']*32+['VDMA_LIST_COPY','VDMA_LIST_IDLE'] and
            positions[-1]<lines.index('MAIA_NATIVE_VDMA_LIST_PASS'))


def evaluate_vdma_count(output):
    clean=output.replace('\r','');lines=clean.splitlines()
    found=re.findall(r'^VDMA_COUNT index=([0-9a-f]{8}) width=([0-9a-f]{8}) items=([0-9a-f]{8}) bytes=([0-9a-f]{8}) matched=([0-9a-f]{8}) raw=([0-9a-f]{8}) status=([0-9a-f]{8}) common=([0-9a-f]{8}) guards=([0-9a-f]{8}) tail=([0-9a-f]{8})$',clean,re.M)
    if len(found)!=4 or sum(l.startswith('VDMA_COUNT ') for l in lines)!=4 or lines.count('MAIA_NATIVE_VDMA_COUNT_PASS')!=1 or 'FAIL' in clean or 'FATAL' in clean:return False
    for n,(row,width,count) in enumerate(zip(found,(8,64,8,64),(8,8,256,4096))):
        i,w,c,b,matched,raw,status,common,guards,tail=map(lambda v:int(v,16),row)
        if not(i==n and w==width and c==count and b==matched==count*(width//8) and raw<=0x3fffff and not status&0x23f7fe0 and common==0 and guards==tail==1):return False
    return lines.count('VDMA_COUNT_IDLE cfg=00000000 channels=00000000 preserved=00000001')==1


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


class FramebufferStimulus(Vo3CycleStimulus):
    def __init__(self,port,visual=False,stream=False,**kwargs):
        super().__init__(port,**kwargs)
        self.visual,self.sent,self.stream=visual,False,stream
    def feed(self,output):
        super().feed(output)
        if self.phase==2 and not self.sent and 'DPI_FB_READY\n' in output.replace('\r',''):
            if self.port.write(b'V' if self.visual else b'E')!=1:raise RuntimeError('short framebuffer command')
            self.port.flush();self.sent=True
            print('[HOST] Framebuffer mode: '+(('native-service-visual' if self.visual else 'native-service-electronic') if self.stream else ('visual1000' if self.visual else 'electronic4')))


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


def connect_boot_state(esptool,port_name,wait_seconds):
    """Pinned host-tool protocol only; no automatic reset or active fallback."""
    import serial
    if esptool.__version__!='4.12.0':
        raise RuntimeError('boot-state connection requires validated esptool4.12.0')
    transport=None
    deadline=time.monotonic()+wait_seconds
    announced=False
    try:
        while True:
            try:
                if transport is None:
                    transport=serial.serial_for_url(port_name,exclusive=True,do_not_open=True)
                    # Configure idle control signals BEFORE opening on macOS too.
                    # Passing this object bypasses esptool's string-port opener.
                    transport.dtr=False;transport.rts=False
                    transport.open()
                    esp=esptool.CHIP_DEFS['esp32p4'](transport,115200)
                if not wait_seconds:
                    esp.connect('no_reset',3)
                    return esp
                if not announced:
                    print('[HOST] BOOT_OBSERVATION_ARMED port open with DTR=0 RTS=0; waiting for ROM, no reset strategy',flush=True)
                    announced=True
                # This pinned method returns a failed-sync error without closing
                # the port. Keep it open while the operator uses the button.
                def forbidden_reset():
                    raise RuntimeError('boot-state reset strategy must never run')
                error=esp._connect_attempt(forbidden_reset,'no_reset')
                if error is None:
                    esp.connect('no_reset',1) # normal chip/security checks after sync
                    return esp
            except (serial.SerialException,OSError):
                # Physical reset may re-enumerate USB; reopen only with idle
                # signals and the same named port, never a reset fallback.
                if transport is not None:
                    with contextlib.suppress(Exception):transport.close()
                transport=None
                if not wait_seconds:raise
            if time.monotonic()>=deadline:
                raise esptool.FatalError('Boot-state ROM wait expired; no image sent, no reset fallback')
            time.sleep(0.1)
    except BaseException:
        if transport is not None:
            with contextlib.suppress(Exception):transport.close()
        raise


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
    parser.add_argument('--psram-fast', type=int, choices=(100,200), help='qualify operational PSRAM clock and bytes')
    parser.add_argument('--psram-dma', action='store_true', help='qualify mapped PSRAM source DMA burst bytes')
    parser.add_argument('--psram-window', action='store_true', help='qualify native CPU2MiB PSRAM mapped memory')
    parser.add_argument('--psram-array', action='store_true', help='port HEX training array transfers and verify content restore')
    parser.add_argument('--psram-clock', action='store_true', help='qualify native XTAL20 PSRAM clock and restore')
    parser.add_argument('--psram-id', action='store_true', help='read/restore inherited HEX mode-register identity')
    parser.add_argument('--psram-state', action='store_true', help='observe PSRAM controllers without array/clock/MMU writes')
    parser.add_argument('--psram-boot-state', action='store_true', help='read-only PSRAM state from manually entered ROM download mode; connect without reset, no automatic cold-start claim')
    parser.add_argument('--psram-boot-wait', type=float, default=0, help='wait0..120 seconds with idle DTR/RTS for manual ROM entry; only with --psram-boot-state')
    parser.add_argument('--dpi-dma-center', action='store_true', help='verify central32-row native SRAM pixels and recovery')
    parser.add_argument('--dpi-dma-spatial', action='store_true', help='verify bounded five-block native spatial pixels and recovery')
    parser.add_argument('--dpi-dma-refresh', action='store_true', help='verify bounded400-frame color sequence and recovery')
    parser.add_argument('--dpi-dma-frame', action='store_true', help='transfer a single solid frame and verify recovery')
    parser.add_argument('--dpi-dma-stage', action='store_true', help='stage inactive solid-source channel0 and DSI flow, no transfer')
    parser.add_argument('--dpi-dma-tile', action='store_true', help='construct and verify SRAM source/tile LLIs without programming or enabling DMA')
    parser.add_argument('--psram-native-boot', action='store_true', help='untested off-supply boot initializer plus full PSRAM/LCD pipeline; no manual timing required or cold acceptance implied')
    parser.add_argument('--psram-park', action='store_true', help='verify native PSRAM/MPLL consumer stop/resume then recalibration and full pipeline; supply stays on')
    parser.add_argument('--psram-supply', action='store_true', help='verify native calibrated VO2 adoption (no supply power cycle) and full pipeline')
    parser.add_argument('--psram-profile', action='store_true', help='verify native PSRAM mode programming/restoration before full-frame qualification')
    parser.add_argument('--mpll-initialize', action='store_true', help='verify native powered400MHz MPLL programming and calibration')
    parser.add_argument('--input-lifecycle', action='store_true', help='three native touch/bus stop/restart cycles with LCD active')
    parser.add_argument('--touch-lcd', action='store_true', help='native touch contacts with scheduled LCD feedback')
    parser.add_argument('--lcd-lifecycle', action='store_true', help='three native LCD restart sessions and six full image publications')
    parser.add_argument('--lcd-stream', action='store_true', help='native task/IRQ LCD session with concurrent kernel load')
    parser.add_argument('--dpi-framebuffer', action='store_true', help='transfer full native PSRAM framebuffer')
    parser.add_argument('--framebuffer-visual', action='store_true', help='show1000frames with temporary backlight; owner readiness required')
    parser.add_argument('--dpi-bridge', action='store_true', help='prepare bridge and publish update without pixel output/DMA')
    parser.add_argument('--dpi-pattern', action='store_true', help='cycle native host color bars without DMA, restore commands')
    parser.add_argument('--dpi-clock', action='store_true', help='verify/select panel RGB565 then enable inactive DPI clock')
    parser.add_argument('--dpi-stage', action='store_true', help='stage inactive RGB565 DPI format/clock/timing and retain panel reads')
    parser.add_argument('--panel-power', action='store_true', help='bounded RDDPM before/after native ST7121 startup')
    parser.add_argument('--st7121', action='store_true', help='selected native ST7121 command program and subsequent responses')
    parser.add_argument('--dsi-reset', action='store_true', help='quiet native ST712x reset before DSI bring-up and responses')
    parser.add_argument('--dsi-read', action='store_true', help='bounded RDDID response word, no assumed panel model')
    parser.add_argument('--dsi-long', action='store_true', help='four bounded DCS long frames containing only NOP, then short NOP')
    parser.add_argument('--dsi-short', action='store_true', help='send sixteen bounded LP NOP packets; host drain only, no panel ACK')
    parser.add_argument('--dsi-command', action='store_true', help='configure command host/timings without sending packets')
    parser.add_argument('--dsi-phy', action='store_true', help='initialize two-lane960Mbps PHY, bounded lock/stop polling, no video')
    parser.add_argument('--dsi-reference', action='store_true', help='prepare guarded nominal20MHz DSI PHY reference, no PLL initialization')
    parser.add_argument('--dsi-access', action='store_true', help='verify guarded DSI bus/config access, no PLL or lane activation')
    parser.add_argument('--vo3-cycle', action='store_true', help='guarded VO3 disable/native acquire with host observation windows')
    parser.add_argument('--mipi-gate', action='store_true', help='verify held shared resets and boot MIPI clock gating')
    parser.add_argument('--mipi-park', action='store_true', help='verify reset-state bridge/PHY parking, no live streams')
    parser.add_argument('--vdma-list', action='store_true', help='qualify two-block SRAM list fetch and exact pixels')
    parser.add_argument('--vdma-count', action='store_true', help='qualify completion counter using SRAM copies')
    parser.add_argument('--vdma-transfer', action='store_true', help='verify owned SRAM DMA copy and active-channel stop')
    parser.add_argument('--vdma-stop', action='store_true', help='verify stopped VDMA adoption')
    parser.add_argument('--vdma-arm-empty', action='store_true', help='arm empty VDMA and verify native global stop; no transfers')
    parser.add_argument('--mipi-state', action='store_true', help='verify read-only inherited MIPI/DW-GDMA state')
    parser.add_argument('--sdmmc-state', action='store_true', help='observe SDMMC clocks, VO4, pins and gated host registers without SD commands')
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

    if args.psram_native_boot:
        if args.psram_park or args.psram_supply or args.psram_profile:parser.error('native boot excludes warm supply/profile/park entry')
        args.mpll_initialize=True
    if args.psram_park:args.psram_supply=True
    if args.psram_supply:args.psram_profile=True
    if args.psram_profile:args.mpll_initialize=True
    if args.mpll_initialize:args.input_lifecycle=True
    if args.input_lifecycle:
        if args.framebuffer_visual:parser.error('input lifecycle is electronic only')
        args.touch_lcd=True
    if args.touch_lcd:
        if args.lcd_lifecycle:parser.error('touch LCD excludes lifecycle')
        args.lcd_stream=True
    if args.lcd_lifecycle:args.lcd_stream=True
    if args.lcd_stream:args.dpi_framebuffer=True
    if args.framebuffer_visual and not args.dpi_framebuffer:parser.error('visual mode requires framebuffer')
    if args.dpi_framebuffer:
        if any((args.dpi_dma_center,args.dpi_dma_spatial,args.dpi_dma_tile,args.dpi_dma_stage,args.dpi_dma_frame,args.dpi_dma_refresh)):parser.error('framebuffer excludes SRAM diagnostics')
        args.dpi_bridge=True
    if args.dpi_dma_center:
        if args.dpi_dma_tile:parser.error('central profile excludes legacy tile')
        args.dpi_dma_spatial = True
    if args.dpi_dma_spatial:
        if args.dpi_dma_stage or args.dpi_dma_frame or args.dpi_dma_refresh:parser.error('DPI spatial excludes solid DMA stage/frame/refresh')
        if args.dpi_dma_center:args.dpi_bridge = True
        else:args.dpi_dma_tile = True
    if args.dpi_dma_refresh:
        args.dpi_dma_frame = True
    if args.dpi_dma_frame:
        args.dpi_dma_stage = True
    if args.dpi_dma_stage:
        args.dpi_dma_tile = True
    if args.dpi_dma_tile:
        args.dpi_bridge = True
    if args.dpi_bridge:
        if args.dpi_pattern: parser.error('bridge diagnostic requires clock profile without VPG')
        args.dpi_clock = True
    if args.dpi_pattern:
        args.dpi_clock = True
    if args.dpi_clock:
        args.dpi_stage = True
    if args.dpi_stage:
        args.panel_power = True
    if args.panel_power:
        args.st7121 = True
    if args.st7121:
        args.dsi_reset = True
    if args.dsi_reset:
        args.dsi_read = True
    if args.dsi_read:
        args.dsi_long = True
    if args.dsi_long:
        args.dsi_short = True
    if args.dsi_short:
        args.dsi_command = True
    if args.dsi_command:
        args.dsi_phy = True
    if args.dsi_phy:
        args.dsi_reference = True
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
    if args.vdma_list:
        if args.vdma_count:parser.error('VDMA list excludes count profile')
        args.vdma_transfer = True
    if args.vdma_count:
        args.vdma_transfer = True
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

    if args.psram_fast:args.psram_dma=True
    if args.psram_dma:args.psram_window=True
    if args.psram_window:args.psram_array=True
    if args.psram_array:args.psram_clock=True
    if args.psram_clock:args.psram_id=True
    if not 0<=args.psram_boot_wait<=120 or (args.psram_boot_wait and not args.psram_boot_state):
        parser.error('boot wait requires --psram-boot-state and duration0..120 seconds')
    if args.psram_boot_state:
        if args.psram_id or args.psram_clock or args.psram_array or args.psram_window or args.psram_dma or args.psram_fast:
            parser.error('boot-state observation excludes identity/memory/clock programming profiles')
        args.psram_state=True
    if args.psram_id:args.psram_state=True
    if args.psram_state:
        for name,value in vars(args).items():
            if name not in ('psram_state','psram_boot_state','psram_id','psram_clock','psram_array','psram_window','psram_dma') and isinstance(value,bool) and value and name!='validate_only':parser.error('PSRAM state requires isolated profile')
        if args.expect_fatal_cause is not None:parser.error('PSRAM state excludes fatal profile')
    if args.sdmmc_state:
        for name, value in vars(args).items():
            if name not in ('sdmmc_state', 'validate_only') and isinstance(value, bool) and value:
                parser.error('SDMMC state requires an isolated profile')
        if args.expect_fatal_cause is not None or args.psram_fast is not None:
            parser.error('SDMMC state requires an isolated profile')

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
        if args.psram_boot_state:
            print('[HOST] Boot-state observation: no connection reset; physical power history is supplied by the operator, not inferred from registers')
        esp = connect_boot_state(esptool,args.port,args.psram_boot_wait) if args.psram_boot_state else esptool.get_default_connected_device(
            [args.port], port=args.port, connect_attempts=3,
            initial_baud=115200, chip='esp32p4',before='default_reset')
        try:
            esptool.main(['--chip', 'esp32p4', '--no-stub', 'load_ram', str(args.image)], esp=esp)
            port = esp._port
            port.timeout = 0.1
            if args.psram_clock:
                if port.write(b'C') != 1:
                    raise RuntimeError('short PSRAM clock host-ready write')
                port.flush()
            if args.mipi_park:
                if port.write(b'P') != 1:
                    raise RuntimeError('short MIPI park host-ready write')
                port.flush()
            stimulus = FramebufferStimulus(port,args.framebuffer_visual,stream=args.lcd_stream) if args.dpi_framebuffer else Vo3CycleStimulus(port) if args.vo3_cycle else SerialInputStimulus(port) if args.serial_input else None
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
        if args.lcd_stream:
            passed = evaluate_lcd_stream(output,args.framebuffer_visual,args.lcd_lifecycle,args.touch_lcd,args.input_lifecycle) and stimulus.sent and stimulus.phase==2
            if args.input_lifecycle:
                rows=[l for l in output.replace('\r','').splitlines() if l.startswith('TOUCH_LIFECYCLE ')]
                passed=passed and rows==['TOUCH_LIFECYCLE cycles=00000003 stops=00000004 restarts=00000003 stopped=00000001 closed=00000001 refused=00000003 sequence=00000003 bus_refused=00000003']
            if args.psram_native_boot:passed=passed and evaluate_psram_native_boot(output)
            elif args.mpll_initialize:passed=passed and evaluate_mpll_initialize(output)
            if args.psram_profile:passed=passed and evaluate_psram_profile(output)
            if args.psram_supply:passed=passed and evaluate_psram_supply(output)
            if args.psram_park:passed=passed and evaluate_psram_park(output)

        elif args.dpi_framebuffer:
            passed = evaluate_dpi_framebuffer(output,args.framebuffer_visual) and stimulus.sent and stimulus.phase==2 and len(stimulus.holds)==2
        elif args.dpi_dma_spatial:
            passed = evaluate_dpi_spatial(output,center=args.dpi_dma_center) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dpi_dma_refresh:
            passed = evaluate_dpi_dma_refresh(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dpi_dma_frame:
            passed = evaluate_dpi_dma_frame(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dpi_dma_stage:
            passed = evaluate_dpi_dma_stage(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dpi_dma_tile:
            passed = evaluate_dpi_dma_tile(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dpi_bridge:
            passed = evaluate_dpi_bridge(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dpi_pattern:
            passed = evaluate_dpi_pattern(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dpi_clock:
            passed = evaluate_dpi_clock(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dpi_stage:
            passed = evaluate_dpi_stage(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.panel_power:
            passed = evaluate_panel_power(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.st7121:
            passed = evaluate_st7121(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dsi_reset:
            passed = evaluate_dsi_reset(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dsi_read:
            passed = evaluate_dsi_read(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dsi_long:
            passed = evaluate_dsi_long(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dsi_short:
            passed = evaluate_dsi_short(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dsi_command:
            passed = evaluate_dsi_command(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dsi_phy:
            passed = evaluate_dsi_phy(output) and stimulus.phase == 2 and len(stimulus.holds) == 2
        elif args.dsi_reference:
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
            passed = evaluate_vdma_list(output) if args.vdma_list else evaluate_vdma_count(output) if args.vdma_count else evaluate_vdma_transfer(output)
        elif args.vdma_stop:
            passed = evaluate_vdma_stop(output, args.vdma_arm_empty)
        elif args.mipi_state:
            passed = evaluate_mipi(output)
        if args.psram_state:
            passed = evaluate_psram_fast(output,args.psram_fast) if args.psram_fast else evaluate_psram_dma(output) if args.psram_dma else evaluate_psram_window(output) if args.psram_window else evaluate_psram_array(output) if args.psram_array else evaluate_psram_clock(output) if args.psram_clock else evaluate_psram_identity(output) if args.psram_id else evaluate_psram_state(output)
        if args.sdmmc_state:
            passed = evaluate_sdmmc_state(output)
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
        if args.sdmmc_state:
            print('[HOST] SDMMC state observation (not driver readiness):', 'PASS' if passed else 'FAIL / NOT DEMONSTRATED')
            return 0 if passed else 1
        print('[HOST] Native LCD task/IRQ service:' if args.lcd_stream else '[HOST] Full PSRAM framebuffer transfer/recovery:' if args.dpi_framebuffer else '[HOST] Operational PSRAM clock/window/DMA:' if args.psram_fast else '[HOST] PSRAM source DMA burst bytes:' if args.psram_dma else '[HOST] Native CPU2MiB PSRAM window/data/restore:' if args.psram_window else '[HOST] Ported HEX PSRAM array patterns/restore:' if args.psram_array else '[HOST] Native XTAL20 PSRAM clock/identity/restore:' if args.psram_clock else '[HOST] Inherited HEX PSRAM identity and restore:' if args.psram_id else '[HOST] PSRAM state observation (not memory readiness):' if args.psram_state else '[HOST] Central32-row native DMA pixels:' if args.dpi_dma_center else '[HOST] Finite five-block spatial DMA pixels:' if args.dpi_dma_spatial else '[HOST] Bounded solid DMA refresh and recovery:' if args.dpi_dma_refresh else '[HOST] Single solid DMA frame and recovery:' if args.dpi_dma_frame else '[HOST] Inactive DMA channel/DSI flow stage:' if args.dpi_dma_stage else '[HOST] Owned SRAM tile/descriptors, DMA inactive:' if args.dpi_dma_tile else '[HOST] Native inactive bridge and update publication:' if args.dpi_bridge else '[HOST] Native host color bars and command recovery:' if args.dpi_pattern else '[HOST] RGB565 and inactive native DPI clock:' if args.dpi_clock else '[HOST] Inactive native DPI configuration:' if args.dpi_stage else '[HOST] Native panel power transition:' if args.panel_power else '[HOST] Native ST7121 program:' if args.st7121 else '[HOST] Quiet panel reset and DSI response:' if args.dsi_reset else '[HOST] DSI RDDID response:' if args.dsi_read else '[HOST] DSI LP long framing:' if args.dsi_long else '[HOST] DSI LP short packets:' if args.dsi_short else '[HOST] DSI command config:' if args.dsi_command else '[HOST] DSI PHY:' if args.dsi_phy else '[HOST] DSI reference:' if args.dsi_reference else '[HOST] DSI config access:' if args.dsi_access else '[HOST] Guarded VO3 cycle:' if args.vo3_cycle else '[HOST] MIPI clock gating:' if args.mipi_gate else '[HOST] MIPI reset parking:' if args.mipi_park else '[HOST] Finite two-block SRAM list copy:' if args.vdma_list else '[HOST] SRAM completion counter qualification:' if args.vdma_count else '[HOST] Active VDMA transfer:' if args.vdma_transfer else '[HOST] VDMA stop:' if args.vdma_stop else '[HOST] MIPI observation:' if args.mipi_state else '[HOST] LDO3 planning:' if args.ldo3 else '[HOST] Touch identity:' if args.touch_identify else '[HOST] Expected fatal trap:' if args.expect_fatal_cause is not None else '[HOST] Native preemption:', 'PASS' if passed else 'FAIL / NOT DEMONSTRATED')
    return 0 if passed else 1


if __name__ == '__main__':
    raise SystemExit(main())
