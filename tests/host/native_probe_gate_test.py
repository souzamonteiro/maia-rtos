"""Positive/negative checks for the native probe's artifact regression gate."""
import importlib.util
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

sys.dont_write_bytecode = True
ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('probe_gate', ROOT / 'scripts/check_native_probe.py')
gate = importlib.util.module_from_spec(spec)
spec.loader.exec_module(gate)

assert not gate.archive_errors('LOAD /compiler/libgcc.a\n')
for name in ('libfreertos.a', 'libc.a', 'libpthread.a'):
    assert gate.archive_errors('LOAD /vendor path/' + name + '\n'), name

cc, nm = shutil.which('cc'), shutil.which('nm')
assert cc and nm
with tempfile.TemporaryDirectory(prefix='maia-native-gate-') as directory:
    source = Path(directory) / 'fixture.c'
    artifact = Path(directory) / 'fixture.o'
    source.write_text('void maia_allowed(void) {}\n')
    subprocess.run([cc, '-c', str(source), '-o', str(artifact)], check=True)
    assert not gate.symbol_errors(artifact, nm)
    for symbol in ('vTaskStartScheduler', 'xTaskCreatePinnedToCore', 'pthread_mutex_lock',
                   'heap_caps_malloc', 'esp_timer_get_time'):
        # Symbol-only negative fixture, not an imported runtime implementation.
        source.write_text('void ' + symbol + '(void) {}\n')
        subprocess.run([cc, '-c', str(source), '-o', str(artifact)], check=True)
        assert gate.symbol_errors(artifact, nm), symbol
    source.write_text('extern void missing(void); void probe(void) { missing(); }\n')
    subprocess.run([cc, '-c', str(source), '-o', str(artifact)], check=True)
    assert gate.symbol_errors(artifact, nm), 'unresolved symbol accepted'
print('PASS: native probe gate accepts project support and rejects foreign archives/symbols/unresolved references')
