"""Verify SDHC capture validation and the isolated RAM-runner profile."""
import contextlib
import importlib.util
import io
from pathlib import Path
import sys
import tempfile
import types
from unittest.mock import patch

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location(
    'runner', root / 'tests/hardware/run_native_scheduler_probe.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


def capture(lba=0, repeat=False, second_lba=0):
    c_size = 121811
    csd = (0, (c_size & 0xFFFF) << 16,
           (1 << 22) | (9 << 16) | ((c_size >> 16) & 0x3F),
           1 << 30)
    sector = bytearray((i * 13 + 7) & 0xFF for i in range(512))
    if lba == 0:
        sector[510:512] = b'\x55\xaa'
    second_sector = bytearray(sector)
    if repeat and second_lba == 0:
        second_sector[510:512] = b'\x55\xaa'
    lines = [
        ('MAIA_SDMMC_READ_PROBE schema=6 slot=0 read-only dma repeat'
         if repeat else 'MAIA_SDMMC_READ_PROBE schema=5 slot=0 read-only dma'),
        (f'SDMMC_BLOCK result=00000000 completed=0000000d command=00000011 '
         f'ocr=c0ff8000 rca=00001234 blocks_hi=00000000 blocks_lo={124735488:08x} '
         + (f'reads=00000002 lba={lba:08x} second_lba={second_lba:08x} '
            if repeat else f'lba={lba:08x} ')
         + 'cmd17_raw=0000000c cmd17_status=00008804 response=00000900 '
         + ('fifoth_before=00000000 ' if repeat else 'fifoth_before=01ff0000 ')
         + 'fifoth=00000000 intmask=00000000 '
         'cardthrctl_before=00010003 cardthrctl=00000000 '
         'cache_line=00000040 cache_status=00000000 dma_status=00000102 '
         'dma_ctrl=02000020 dma_bmod=00000082 dma_inten=00000102 '
         'dma_dbaddr=4ff10000 dma_desc0=0000001c dma_desc1=00000200 '
         'dma_desc2=4ff10080 dma_desc3=00000000'),
        'SDMMC_CID w0=11223344 w1=55667788 w2=99aabbcc w3=ddeeff00',
        'SDMMC_CSD ' + ' '.join(f'w{i}={word:08x}' for i, word in enumerate(csd)),
        ('SDMMC_CONFIG sys=00004000 source=01000000 divider=38929200 ldo=00004180 '
         'clkdiv=00000014 clksrc=00000000 clkena=00000001 tmout=009c40ff '
         'ctype=00000000 cdetect=00000000 d0=00000e00 d3=00001e00 '
         'clk=00000e00 cmd=00000e00 out=00000400 enable=00000400 input=00000400 '
         'ctrl=00000000 bmod=00000000 idsts=00000000 idinten=00000000 '
         'tcbc=00000200 tbbc=00000200'),
    ]
    for offset in range(0, 512, 16):
        data = sector[offset:offset + 16].hex()
        lines.append(f'SDMMC_BLOCK_DATA offset={offset:08x} data={data}')
    if repeat:
        for offset in range(0, 512, 16):
            data = second_sector[offset:offset + 16].hex()
            lines.append(f'SDMMC_SECOND_BLOCK_DATA offset={offset:08x} data={data}')
    lines.append('MAIA_NATIVE_SDMMC_READ_PASS')
    return '\n'.join(lines) + '\n'


good = capture()
good_lba1 = capture(1)
good_repeat = capture(32768, True, 32774)
assert runner.evaluate_sdmmc_read(good)
assert runner.evaluate_sdmmc_read(good, expected_lba=0)
assert runner.evaluate_sdmmc_read(good_lba1, expected_lba=1)
assert not runner.evaluate_sdmmc_read(good_lba1, expected_lba=0)
assert not runner.evaluate_sdmmc_read(good, expected_lba=1)
assert not runner.evaluate_sdmmc_read(
    capture(124735488), expected_lba=124735488)
assert runner.evaluate_sdmmc_read(
    good_repeat, 124735488, 32768, True, 32774, True)
assert not runner.evaluate_sdmmc_read(
    good_repeat, 124735488, 32768, True, 32773, True)
assert not runner.evaluate_sdmmc_read(
    good_repeat.replace('reads=00000002', 'reads=00000001', 1),
    124735488, 32768, True, 32774, True)
assert not runner.evaluate_sdmmc_read(
    good_repeat.replace('SDMMC_SECOND_BLOCK_DATA offset=00000000 data=',
                        'SDMMC_OTHER_DATA offset=00000000 data=', 1),
    124735488, 32768, True, 32774, True)
assert runner.evaluate_sdmmc_block0(good)
assert runner.evaluate_sdmmc_block0(good, 124735488)
assert not runner.evaluate_sdmmc_block0(good, 124735487)
for old, new in [
        ('completed=0000000d', 'completed=0000000c'),
        ('command=00000011', 'command=00000008'),
        ('ocr=c0ff8000', 'ocr=40ff8000'),
        ('rca=00001234', 'rca=00000000'),
        ('cmd17_raw=0000000c', 'cmd17_raw=0000010c'),
        ('cmd17_status=00008804', 'cmd17_status=00008004'),
        ('response=00000900', 'response=00000000'),
        ('fifoth=00000000', 'fifoth=01ff0000'),
        ('intmask=00000000', 'intmask=00000020'),
        ('cardthrctl=00000000', 'cardthrctl=00000001'),
        ('cache_line=00000040', 'cache_line=00000008'),
        ('cache_status=00000000', 'cache_status=00000001'),
        ('dma_status=00000102', 'dma_status=00000100'),
        ('dma_status=00000102', 'dma_status=00000106'),
        ('dma_ctrl=02000020', 'dma_ctrl=00000020'),
        ('dma_bmod=00000082', 'dma_bmod=00000080'),
        ('dma_inten=00000102', 'dma_inten=00000002'),
        ('dma_desc0=0000001c', 'dma_desc0=8000001c'),
        ('dma_desc1=00000200', 'dma_desc1=000001ff'),
        ('dma_dbaddr=4ff10000', 'dma_dbaddr=4ff10004'),
        ('dma_desc2=4ff10080', 'dma_desc2=4ff10100'),
        ('idsts=00000000', 'idsts=00000106'),
        ('tcbc=00000200', 'tcbc=000001ff'),
        ('tbbc=00000200', 'tbbc=00000000'),
        ('w3=40000000', 'w3=00000000'),
        (f'blocks_lo={124735488:08x}', f'blocks_lo={124735489:08x}'),
        ('divider=38929200', 'divider=00000000'),
        ('offset=00000000', 'offset=00000010'),
        ('MAIA_NATIVE_SDMMC_READ_PASS', 'MAIA_NATIVE_SDMMC_READ_FAIL'),
]:
    assert not runner.evaluate_sdmmc_block0(good.replace(old, new, 1)), (old, new)
before_signature, after_signature = good.rsplit('55aa\n', 1)
assert not runner.evaluate_sdmmc_block0(before_signature + '55ab\n' + after_signature)
assert not runner.evaluate_sdmmc_block0(good.replace('SDMMC_BLOCK_DATA ', 'SDMMC_OTHER ', 1))
assert not runner.evaluate_sdmmc_block0(good + good)
assert not runner.evaluate_sdmmc_block0(good + 'MAIA_STACK_FAULT\n')


class BeforeSerial(Exception):
    pass


calls = []
esp = types.ModuleType('esptool')


def connect(*args, **kwargs):
    calls.append(kwargs)
    raise BeforeSerial


esp.get_default_connected_device = connect
image_module = types.ModuleType('esptool.bin_image')
image = types.SimpleNamespace(
    segments=[types.SimpleNamespace(addr=0x4FF00000, data=bytes(1024))],
    entrypoint=0x4FF00100)
image_module.LoadFirmwareImage = lambda chip, path: image

with tempfile.TemporaryDirectory() as directory, patch.dict(
        sys.modules, {'esptool': esp, 'esptool.bin_image': image_module}):
    def run(flags):
        calls.clear()
        argv = ['probe', '--image', str(Path(directory) / 'not-opened.bin'),
                '--log', str(Path(directory) / 'log'), *flags]
        with patch.object(sys, 'argv', argv), contextlib.redirect_stderr(io.StringIO()), \
                contextlib.redirect_stdout(io.StringIO()):
            try:
                return runner.main()
            except BeforeSerial:
                return 'before-serial'
            except SystemExit as error:
                return error.code

    assert run(['--sdmmc-block0', '--validate-only']) == 0 and not calls
    assert run(['--sdmmc-read', '--sdmmc-lba', '1', '--validate-only']) == 0 and not calls
    assert run(['--sdmmc-lba', '1', '--validate-only']) == 2 and not calls
    assert run(['--sdmmc-read', '--sdmmc-repeat-read', '--validate-only']) == 2 and not calls
    assert run(['--sdmmc-read', '--sdmmc-repeat-read', '--sdmmc-second-lba', '32774',
                '--sdmmc-lba', '32768', '--sdmmc-identical-blocks',
                '--expected-sd-block-count', '124735488', '--validate-only']) == 0 and not calls
    assert run(['--sdmmc-read', '--sdmmc-lba', str(124735488),
                '--expected-sd-block-count', '124735488', '--validate-only']) == 2 and not calls
    assert run(['--sdmmc-block0', '--sdmmc-cmd8']) == 2 and not calls
    assert run(['--expected-sd-block-count', '124735488']) == 2 and not calls

    class Port:
        timeout = None

        def __init__(self, output):
            self.output = output
            self.writes = []
            self.flushed = False
            self.closed = False

        def write(self, data):
            self.writes.append(data)
            return len(data)

        def flush(self):
            self.flushed = True

        def read(self, size):
            return self.output.encode() if self.output else b''

        def close(self):
            self.closed = True

    class Device:
        def __init__(self, output):
            self._port = Port(output)
            self.reset = False

        def hard_reset(self):
            self.reset = True

    device = Device(good)

    def fake_connect(ports, port, connect_attempts, initial_baud, chip, before):
        assert ports == [port] and connect_attempts == 3 and initial_baud == 115200
        assert chip == 'esp32p4' and before == 'default_reset'
        return device

    def fake_load(arguments, esp):
        assert arguments[:3] == ['--chip', 'esp32p4', '--no-stub']
        assert not esp._port.writes
        calls.append((arguments, esp))

    esp.get_default_connected_device = fake_connect
    esp.main = fake_load
    with patch.object(runner.time, 'monotonic', side_effect=[0, 0, 2]):
        assert run(['--sdmmc-block0', '--expected-sd-block-count', '124735488',
                    '--capture-seconds', '1']) == 0
    assert device._port.writes == [b'B'] and device._port.flushed
    assert device.reset and device._port.closed and len(calls) == 1
    device = Device(good_repeat)
    with patch.object(runner.time, 'monotonic', side_effect=[0, 0, 2]):
        assert run(['--sdmmc-read', '--sdmmc-lba', '32768',
                    '--sdmmc-repeat-read', '--sdmmc-second-lba', '32774',
                    '--sdmmc-identical-blocks',
                    '--expected-sd-block-count', '124735488',
                    '--capture-seconds', '1']) == 0
    assert device.reset and device._port.closed and len(calls) == 1
    device = Device(good_lba1)
    with patch.object(runner.time, 'monotonic', side_effect=[0, 0, 2]):
        assert run(['--sdmmc-read', '--sdmmc-lba', '1',
                    '--expected-sd-block-count', '124735488',
                    '--capture-seconds', '1']) == 0
    assert device.reset and device._port.closed and len(calls) == 1
print('PASS: SDHC LBA validation, geometry, capacity, RAM isolation and host handshake')
