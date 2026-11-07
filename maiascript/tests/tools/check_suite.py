#!/usr/bin/env python3
import json, pathlib, subprocess, shutil, sys
root=pathlib.Path(__file__).resolve().parents[1]
manifest=json.loads((root/'manifest.json').read_text())
for case in manifest:
 p=root/case['path']; assert p.is_file() and p.read_text().strip(), p
print('Syntax fixtures:',sum(c['kind']=='positive' for c in manifest),'positive,',sum(c['kind']=='negative' for c in manifest),'negative')
for p in sorted((root/'pairs').glob('*/main.S')):
 meta=json.loads((p.parent/'expected.json').read_text()); assert isinstance(meta['return_a0'],int)
 assert (p.parent/'main.ms').is_file()
print('RISC-V pairs:',len(list((root/'pairs').glob('*/main.S'))))
clang=shutil.which('clang')
if clang:
 subprocess.run(['sh',str(root/'tools/check_assembly.sh')],check=True,env=__import__('os').environ.copy())
else: print('clang not found; assembly compilation skipped')
print('NOTE: MaiaCC grammar parsing, semantic checks and RISC-V execution are NOT run by this harness')
