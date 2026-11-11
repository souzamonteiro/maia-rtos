"""Reject foreign runtime symbols/archives in the isolated native diagnostic.

This is a regression gate, not a proof of provenance, hardware correctness,
or absence of renamed/copied code. The probe links project objects + libgcc.
"""
import argparse
from pathlib import Path
import re
import subprocess

FORBIDDEN = re.compile(
    r'^(?:[xv]Task\w*|[xv]Queue\w*|[xv]Semaphore\w*|[xv]Port\w*|'
    r'ux(?:Task|Queue|Semaphore|Timer)\w*|[xv](?:Timer|EventGroup|StreamBuffer)\w*|'
    r'pv(?:Port|Task)\w*|port[A-Z_]\w*|esp_\w*|heap_caps_\w*|pthread_\w*|'
    r'nvs_\w*|bsp_\w*|app_main)$')


def symbol_errors(artifact, nm):
    output = subprocess.check_output([nm, '-a', str(artifact)], text=True)
    errors = []
    for line in output.splitlines():
        fields = line.split()
        if not fields:
            continue
        name = fields[-1].lstrip('_')
        if FORBIDDEN.fullmatch(name):
            errors.append('forbidden runtime symbol: ' + fields[-1])
        if len(fields) >= 2 and fields[-2] == 'U':
            errors.append('unresolved symbol: ' + fields[-1])
    return errors


def archive_errors(map_text):
    return ['unexpected linked archive: ' + archive
            for archive in sorted(set(re.findall(r'^LOAD\s+(.+?\.a)\s*$', map_text, re.M)))
            if Path(archive).name != 'libgcc.a']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--elf', type=Path, required=True)
    parser.add_argument('--map', type=Path, required=True)
    parser.add_argument('--nm', required=True)
    args = parser.parse_args()
    errors = symbol_errors(args.elf, args.nm) + archive_errors(args.map.read_text())
    for error in errors:
        print(error)
    if not errors:
        print('Native probe runtime gate: PASS (project objects + libgcc; no unresolved/foreign symbols)')
    return bool(errors)


if __name__ == '__main__':
    raise SystemExit(main())
