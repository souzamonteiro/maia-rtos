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
test_bin = root.parent / 'bin' / 'test_compiler'
if test_bin.is_file():
    print('Running end-to-end MaiaScript -> MaiaASM -> RISC-V execution suite:')
    subprocess.run([str(test_bin)], check=True)
else:
    print('test_compiler not yet built; run make test in maiascript/')
