"""Verify SDMMC diagnostic isolation before opening any serial port."""
import contextlib
import importlib.util
import io
from pathlib import Path
import sys
import tempfile
import types
from unittest.mock import patch

root = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('runner', root / 'tests/hardware/run_native_scheduler_probe.py')
runner = importlib.util.module_from_spec(spec)
spec.loader.exec_module(runner)


class BeforeSerial(Exception):
    pass


calls = []


def connect(*args, **kwargs):
    assert len(args) == 1
    calls.append(kwargs)
    raise BeforeSerial


esp = types.ModuleType('esptool')
esp.get_default_connected_device = connect
bin_image = types.ModuleType('esptool.bin_image')
image = types.SimpleNamespace(
    segments=[types.SimpleNamespace(addr=0x4FF00000, data=bytes(1024))],
    entrypoint=0x4FF00100)
def load_image(chip, path):
    assert chip == 'esp32p4' and path.endswith('not-opened.bin')
    return image


bin_image.LoadFirmwareImage = load_image

with tempfile.TemporaryDirectory() as directory, patch.dict(
        sys.modules, {'esptool': esp, 'esptool.bin_image': bin_image}):
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

    assert run(['--sdmmc-cmd8', '--validate-only']) == 0 and not calls
    assert run(['--sdmmc-cmd8']) == 'before-serial' and calls[0]['before'] == 'default_reset'
    for flag in ['--sdmmc-state', '--psram-state', '--mipi-state', '--touch-identify', '--serial-input']:
        assert run(['--sdmmc-cmd8', flag]) == 2 and not calls, flag
    assert run(['--sdmmc-cmd8', '--psram-fast', '200']) == 2 and not calls
    assert run(['--sdmmc-cmd8', '--expect-fatal-cause', '2', '--expect-fatal-pc', '0x4ff00100']) == 2 and not calls

    class Port:
        def __init__(self, short=False):
            self.writes = []
            self.flushed = False
            self.closed = False
            self.short = short
            self.timeout = None

        def write(self, data):
            self.writes.append(data)
            return 0 if self.short else len(data)

        def flush(self):
            self.flushed = True

        def read(self, size):
            assert size == 8192
            return b'MAIA_NATIVE_SDMMC_CMD8_FAIL simulated\n'

        def close(self):
            self.closed = True

    class Device:
        def __init__(self, short=False):
            self._port = Port(short)
            self.reset = False

        def hard_reset(self):
            self.reset = True

    def fake_connect(ports, port, connect_attempts, initial_baud, chip, before):
        assert ports == [port] and connect_attempts == 3 and initial_baud == 115200
        assert chip == 'esp32p4' and before == 'default_reset'
        return device

    def fake_load(arguments, esp):
        assert arguments[:3] == ['--chip', 'esp32p4', '--no-stub']
        assert not esp._port.writes
        calls.append((arguments, esp))

    device = Device()
    esp.get_default_connected_device = fake_connect
    esp.main = fake_load
    with patch.object(runner.time, 'monotonic', side_effect=[0, 0, 2]):
        assert run(['--sdmmc-cmd8', '--capture-seconds', '1']) == 1
    assert device._port.writes == [b'S'] and device._port.flushed
    assert device.reset and device._port.closed and len(calls) == 1
    device = Device(short=True)
    try:
        run(['--sdmmc-cmd8'])
    except RuntimeError as error:
        assert 'short SDMMC CMD8 host-ready write' in str(error)
    else:
        raise AssertionError('short host-ready write must fail explicitly')
    assert device._port.writes == [b'S'] and not device._port.flushed
    assert device.reset and device._port.closed

    image.segments[0].addr = 0x4B800000
    assert run(['--sdmmc-cmd8']) == 2 and not calls
print('PASS: SDMMC CMD8 profile, conflicting options and RAM image are checked before serial access')
