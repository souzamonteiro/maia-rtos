#!/usr/bin/env python3
"""Check configuration isolation/guards; does not emulate ESP-IDF or hardware."""
import os
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
with tempfile.TemporaryDirectory(prefix='maia-singlecore-config-') as directory:
    fixture = Path(directory)
    platform = fixture / 'tab5'
    platform.mkdir()
    for name in ('CMakeLists.txt', 'sdkconfig.singlecore.defaults'):
        shutil.copy2(ROOT / 'platforms/tab5' / name, platform / name)
    seed = '# CONFIG_FREERTOS_UNICORE is not set\nCONFIG_BOARD_SENTINEL=37\n'
    (platform / 'sdkconfig').write_text(seed)
    (platform / 'sdkconfig.defaults').write_text('')
    sdk = fixture / 'idf/tools/cmake'
    sdk.mkdir(parents=True)
    # Only record the CMake inputs; this stub supplies no runtime or drivers.
    (sdk / 'project.cmake').write_text(
        'file(WRITE "${CMAKE_BINARY_DIR}/selected-config.txt" "${SDKCONFIG}")\n')
    env = dict(os.environ, IDF_PATH=str(fixture / 'idf'))

    def configure(build, enabled):
        return subprocess.run(
            ['cmake', '-S', str(platform), '-B', str(build),
             '-DMAIA_TAB5_SINGLECORE_DRIVERS=' + ('ON' if enabled else 'OFF')],
            env=env, text=True, capture_output=True)

    baseline = fixture / 'baseline'
    result = configure(baseline, False)
    assert result.returncode == 0, result.stdout + result.stderr
    assert not (baseline / 'sdkconfig.singlecore').exists()

    experiment = fixture / 'experiment'
    result = configure(experiment, True)
    assert result.returncode == 0, result.stdout + result.stderr
    config = experiment / 'sdkconfig.singlecore'
    assert (experiment / 'selected-config.txt').read_text() == str(config)
    assert 'CONFIG_BOARD_SENTINEL=37' in config.read_text()
    assert config.read_text().count('CONFIG_FREERTOS_UNICORE') == 1
    assert 'CONFIG_FREERTOS_UNICORE=y' in config.read_text()
    assert (platform / 'sdkconfig').read_text() == seed

    # Refuse to repurpose the canonical working build for the experiment.
    result = configure(platform / 'build', True)
    assert result.returncode != 0 and 'Use a separate build directory' in result.stderr
    assert (platform / 'sdkconfig').read_text() == seed
    assert not (platform / 'build/sdkconfig.singlecore').exists()

    # Catch Kconfig/cache overrides that would silently produce two cores.
    result = subprocess.run(
        ['cmake', '-DMAIA_TAB5_SINGLECORE_DRIVERS=ON', '-DCONFIG_FREERTOS_UNICORE=OFF',
         '-P', str(ROOT / 'platforms/tab5/main/CMakeLists.txt')],
        text=True, capture_output=True)
    assert result.returncode != 0 and 'requires CONFIG_FREERTOS_UNICORE=y' in result.stderr

print('PASS: default unchanged, isolated config preserves board settings, unsafe build/config rejected')
