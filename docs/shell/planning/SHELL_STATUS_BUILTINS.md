# Status builtins in the existing simple-command dispatcher

Date: 2026-10-27. Status: implemented and validated locally under the owner's instruction
to continue shell development. This is a bounded dispatch addition within the
existing approved simple-command model, not a new process or execution contract.

## Behavior

Implement `true` and `:` as silent success (status0), and `false` as silent
failure (status1). Ignore expanded arguments, including option-looking operands;
normal word/glob limits and expansion errors still apply before dispatch.
Preserve quoting and generated command-name rendering. All existing source-order
redirections run normally, including empty create/truncate and append. Open and
close failures retain the existing status1/error behavior. Commands complete
only after their normal close/drain path, so `$?`, AND-OR lists and dot return
observe completed status. Skipped units have no expansion or file effects.

Keep the current pipeline profile: these builtins are not new worker packages;
using them as pipeline stages remains unsupported and atomic. No special-builtin
noninteractive fatal-error rules, assignments, variables, conditionals, loops,
redirection-only commands or wider POSIX conformance are added.

## Changes and validation

Edit only MaiaOS: dispatcher/help, actual-WASM status regressions, real
kernel/I/O/LVGL cases and the existing read-only status recipe/expected data.
Keep the fixed VFS image file set and normal mount path. Extend its existing
recipe rather than introducing another fixture directory or test interpreter.
Update shell/kernel/I/O progress and handoff with exact results.

Compare silent output/status and AND-OR results against native `/bin/sh`;
assert exact guest VFS bytes, skipped opens/enumerations, ordinary expansion/
open/close failures, script continuation/return, memory/stack/handle stability,
and pipeline rejection without launching workers. Build through own published
C++98 -> strict C89 -> MaiaC/MaiaWASM. Run word/status/list/script, redirection,
pipeline/LVGL/boot and unit gates appropriate to the changed dispatch and fixture.
Regenerate production shell.wasm; never edit generated parsers. Grammar and
generated frontend sources remain unchanged.

No compiler/dependency, ABI, syscall, capability, manifest, stream, VFS format or
kernel/I/O implementation changes. No commit/push, npm, versions or history edits.

## Completed checkpoint — 2026-10-27

Only dispatcher/help, reference regressions, existing status recipe/expected
bytes and progress docs changed; shell.wasm regenerated to1,417,694 bytes,
SHA256 b66d4c180637e8deadd6f77474416a9dad5d9a0194b433ff0eb980a409fdcc7d.
Own build --check-c89 passes. Status suite compares native shell output/status,
ignored/quoted arguments,empty truncation/append,skipped opens/enumerations,
pre-dispatch glob failure/overflow,input-open and input/output-close failures,
script return/continuation and200 stable builtin repetitions. Original150 status
repetitions and all previous tests retained. Pipeline rejection has both an
unexpected-launch negative guard and actual kernel/I/O/LVGL cases.

npm run test:shell-status, test:shell-lists, test:shell-scripts,
test:shell-redirection, test:shell-word, test:shell-pipelines, test:shell-lvgl,
test:shell-boot and npm test pass. Word306 JS/native trees +158 byte/mask cases,
ASan/UBSan and byte reproduction pass. Unit66pass. Five recipes/37 commands twice
and all five files twice by guest dot; stable shell20/I/O2 pages,1,000 parses,
77,911 kernel steps,225 pipeline flushes,2,131 LVGL flushes and25 boot sessions.
Generated main C++ comparison and diff whitespace checks pass.

Main full5,833-input grammar parity,unchanged native list/script/printf/kernel
helpers,new I/O/kernel builds,interactive browser,MaiaWASMVM/device were not rerun
for this dispatch-only increment. Their earlier evidence remains separate.
MaiaOS main b13f168 retains prior local edits and staged dependency pins; no new
commits/pushes/npm/versions. Logs /tmp/maiaos-status-builtins-*.log.
