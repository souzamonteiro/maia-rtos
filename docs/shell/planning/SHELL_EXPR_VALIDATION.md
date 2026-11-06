# Bounded expr acceptance — 2026-11-04

Implementation of [the approved execution contract](SHELL_EXPR_EXECUTION.md),
including the separately approved384KiB compilation /64KiB matching quotas.
This records local working-tree acceptance; no MaiaOS commit/push/npm/version.

## Implementation and architectural path

`apps/shell/maia-shell-expr.ebnf` owns final-NUL-framed argv syntax. MaiaCC emits
JavaScript/C++98 lexer/parser/tree events. `shell-expr.hpp` evaluates that tree
iteratively, with unsigned magnitudes for checked signed32 arithmetic, eager
logic, byte comparisons and the published anchored longest BRE API. Existing
shell streams carry the result/status and stderr diagnostics. No host expression
parser/evaluator/regex, new syscall/worker, compiler/library implementation edit,
ABI/capability or manifest enlargement. Compilation is actual AST-strict
MaiaCpp C++98 -> strict C89 -> MaiaC/MaiaWASM -> guest execution.

The actual configured MaiaCpp preprocessor supplies the published BRE include
path; the ordinary C++ compiler then consumes its preprocessed source. The old
shell's `stdint.h` diagnostic is retained explicitly: MaiaCpp already maps these
fixed-width names intrinsically. All other unresolved includes fail the build.
Generated parser guards/identifiers are transported by generated macro adapters;
the expr frontend is included before the main shell parser. Generated files are
never edited manually. BRE compilation and matching/free run serially.

Published pins consumed: MaiaCpp16cca71, MaiaGlob85db91b, MaiaRegex8a1dcbf.
Regex's development-only historical bootstrap remains fixed/skipped. The earlier
publication was separately authorized and completed; these three OS gitlinks
are staged, while the feature and artifact remain uncommitted on main.

## Exact commands and observed results

Run in the MaiaOS root. Every listed completed command exited0.

| Command | Evidence |
| --- | --- |
| `node scripts/generate-shell-expr.mjs` | Six frontend files regenerated independently and compared byte-for-byte. |
| `node scripts/test-shell-expr-syntax.mjs` |556 independently expected generated-JS syntax/frame/span cases. |
| `node scripts/test-shell-expr-native.mjs` |556 C++98 ASan/UBSan status/result/token/span/full-tree cases, collector/work guards;281 shared `/bin/expr` oracles. |
| `node scripts/test-shell-expr-allocation.mjs` |110 arithmetic +441 BRE allocation failures; status3, atomic result, no leaks/invalid frees, recovery after every failure. |
| `node --max-old-space-size=16384 scripts/test-shell-expr-wasm.mjs` |Actual own-chain artifact;556 cases, exact JS parity, depth42, five invalid UTF-8 frames, transport/capacity/work guards,300 stable calls at18pages. See artifact-reuse note below. |
| `node --max-old-space-size=16384 scripts/test-shell-expr-fault-wasm.mjs` |110 arithmetic +444 BRE failure sites, peak285292bytes, zero outstanding allocations, canaries/stack/recovery;20pages within32. |
| `node scripts/test-shell-expr.mjs` |553 production cases with separate stdout/stderr; three private transport cases covered above; no overlong quoted cases omitted. Lists/fields/if/loops/dot/return, redirections/cleanup, zero plain/skipped VFS calls, unsupported worker placements,150 BRE/script cycles;21pages within32. One-byte stream and close-failure tests cover all four statuses plus negation. |
| `node scripts/test-shell-host.mjs --lvgl --shell-wasm /tmp/maia-shell-expr-preview.wasm` |Actual kernel/I/O/shell/LVGL:2192 flushes,77911 kernel steps,1000 parses, shell21/I/O2pages, stable stack/memory and closed streams. |
| `node scripts/test-shell-pipelines.mjs` |Actual boot functions/kernel/I/O/cat/wc/LVGL:18 immutable VFS recipes/222 commands twice by KEY and all18 twice by guest dot.1530flushes; pipeline ownership/reap/cancel/faults and128KiB partial I/O remain passing. |
| `node scripts/test-shell-boot.mjs` |25 actual LVGL boot/exit sessions; launch failures, quotas and zero windows/endpoints after cleanup. |
| `node --test tests/*.test.js` |66 unit tests pass, including read-only/atomic VFS mounting and unchanged4096-byte display/16KiB asset bounds. |
| `bash scripts/test-shell-grammar.sh` |Existing positive/negative grammar cases pass; two known contextual-reserved-word POSIX gaps remain explicitly reported. |
| `node scripts/test-shell-equivalence.mjs` |6426 inputs,3243 accepted/2353 syntax errors/830 lexical errors; zero JS/native/WASM token/span/tree differences and restored guest stack. |
| `git diff --check` |Pass. |

The initial WASM harness loaded two authoring expectations before corrections.
The already built identical-code artifact was rerun with
`node scripts/test-shell-expr-wasm.mjs --wasm /private/var/folders/6p/871hys095mq5mm7078sqplx40000gn/T/maia-expr-parity-xYK3Ch/expr.wasm`.
All556 cases pass. This is actual execution, not a compilation-only claim.
The selected published BRE profile treats an initial `*` as literal (macOS expr
rejects it); valid interval256 exceeds the approved max_repeat255 and returns3,
whereas a reversed interval is malformed and returns2. These independently
justified expectations are annotated in the corpus. Existing invalid patterns
were retained; a positive literal-star case and reversed interval were added.
Also tested: maximum five serial BRE operations and matching-step exhaustion.

Additional unchanged production regressions, all passed:
`node scripts/test-shell-directories.mjs`, `test-shell-files.mjs`,
`test-shell-glob.mjs`, `test-shell-lists.mjs`, `test-shell-variables.mjs`,
`test-shell-status.mjs`, `test-shell-redirection.mjs`, `test-shell-paths.mjs`,
`test-shell-scripts.mjs`, `test-shell-predicates.mjs`, `test-shell-for.mjs`,
`test-shell-conditional.mjs`, `test-shell-loop-control.mjs`,
`test-shell-loop-if.mjs`, `test-shell-return.mjs`, `test-shell-multiline.mjs`
(each filename after the first is also under `scripts/`).

## Reproducible artifact

Two independent strict builds passed and produced identical bytes:
`node --max-old-space-size=16384 scripts/build-shell-wasm.mjs --check-c89 --output /tmp/maia-shell-expr-preview.wasm`.
Second independent strict build:
`node --max-old-space-size=16384 scripts/build-shell-wasm.mjs --check-c89 --output /tmp/maia-shell-expr-reproduction.wasm`.
Both were compared byte-for-byte against the tested production artifact installed locally as `apps/shell/shell.wasm`:
1994402bytes; SHA-256
`231b0f50388751a63802777e4a59ad5ec08c57b6d7a032d70c893539a9fda2f0`.
Production has no `expr_*`, `bre_fault_*` or parameter-injection exports.

## Manual recipe and retained limits

In the simulator: `. /system/tests/18-expr.sh`. The read-only VFS also carries
`18-expr.expected.txt`; metadata was compacted without removing old recipe data
so `expected.json` stays15540bytes and the guide3998bytes, within existing bounds.
For example `expr 2 + 3 '*' 4` prints14; `expr 'ação 😄' : '.*'` prints6.
Protect operator/glob spellings with shell quotes. False results still print a
complete result/LF and return1; syntax/operand errors return2, checked overflow/
resource failures3. No partial successful output on errors.

This is a bounded builtin: signed32,11 expression frames,95-byte words,
255-byte shell unit, four grouping levels,512 collector nodes/4096 evaluation
steps, five BRE calls/10000 matching steps each. No command substitution/result
assignment shortcut, GNU extensions or expr pipeline worker. Raw numeric strings
and Unicode scalar lengths follow the selected contract; no universal libc/POSIX
claim. Browser manual interaction, MaiaWASMVM and physical-device acceptance were
not run. LVGL evidence above is the real reference host/WASM stack, not DOM UI.
