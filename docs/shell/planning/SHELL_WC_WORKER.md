# WC worker and bounded pipeline selection

> The subsequent [count selection extension](SHELL_WC_OPTIONS.md) implements
> -l/-w/-c options. The no-options restrictions below describe the initial slice.

Date: 2026-10-23. Status: owner approved compiler correction/publication/propagation and the exact
worker-selection contract on 2026-10-23. The worker and integration are implemented
and validated locally; MaiaOS publication remains excluded. Baseline is MaiaOS b13f168 (owner committed the previous slice).

## Worker using existing APIs

Add apps/wc/wc.cpp, manifest.json and reproducible wc.wasm, built through the
repository's published MaiaCpp -> MaiaC -> MaiaWASM submodules, with strict C89
checking. Existing imports only: stream.read/write/close, process.args_size/
args_copy/exit. No VFS, graphics, allocator, host command processing or new syscall.
Use fixed buffers, at most 16 I/O/counting operations per step, partial writes,
WOULD_BLOCK yielding and immutable copied argv. Keep input word state across
chunks. stdout receives one ASCII record: `lines words bytes\n` with single
spaces, no field padding. Lines count LF bytes; words are maximal byte runs
separated by ASCII space/HT/LF/VT/FF/CR; byte count includes NUL and UTF-8 bytes.
This is a defined byte-oriented profile, not locale-aware POSIX completeness.

This first scope is stdin only, no options or file operands. Empty input outputs
`0 0 0\n`. All counters are unsigned 32-bit, checked before increment: overflow
reports `wc: count overflow\n` on stderr and exits 1, with no result record.
Read/argv/transport failures also exit 1; normal completion exits 0. Preserve
backpressure, EOF and binary input semantics. Resource request: <=32 memory
pages (verify actual initial pages), zero VFS handles. No data-dependent allocation.

## Exact integration extension for owner review

Keep existing plan v1/v2 and worker argv/completion formats, import signatures,
stream actions, scheduler, cancellation, namespaces, quotas and output-redirection
ownership. Extend the trusted START action's fixed package index whitelist:
1 remains org.maiaplatform.cat; 2 selects org.maiaplatform.wc at /apps/wc/.
No arbitrary package names, paths or guest-chosen PID/capabilities.

Permit `wc` only as the last stage of an existing 2..4 stage pipeline, with argv
exactly ["wc"]. All preceding stages remain cat; first cat still requires files.
Thus `cat a | wc`, `cat a | cat | wc`, and their > / >> final-output forms work.
`wc` alone, wc options/operands, wc as an earlier stage and generic exec remain
outside this slice and must be rejected before target mutation or launch.
The shell validates the generated SimpleCommand tree/expanded argv; the kernel
independently checks stage position/name/argc before reserving any resources.
No shell grammar edits or handwritten parsing. Existing cat behavior is retained.

Kernel-selected wc child profile contains stream.read/write/close and process.exit
plus immutable args bootstrap; no VFS bits. Reserve existing <=32 worker pages,
zero open files, and existing total worker quota. Host package resolution follows
only the kernel index, validates the package identity/path/imports/manifest against
that fixed profile, intersects grants with the owner, and defers entry until all
startup acknowledgments. Kernel remains the lifecycle and scheduling authority.
I/O continues to own the real LVGL terminal. No host computes counts or interprets
shell text. Keep separate caches/failed-fetch retry for the two fixed packages.

## Compiler defect and owner correction

The initial strict build found that valid C++98 static_cast in a do-while body
can remain verbatim in emitted C89. Minimal valid source:

```cpp
char digits[10]; unsigned int n=0,value=7;
do {digits[n++]=static_cast<char>('0'+value%10);value/=10;}while(value);
```

A complete native C++98 executable using this fragment exits 0. The published
MaiaCpp AST-strict compiler emits `static_cast<char>` in C output; Clang C89
rejects it. The same assignment outside do-while lowers successfully, isolating
the control-flow lowering path. The reviewable source is apps/wc/tests/cpp98-do-while-cast.cpp; emitted C/native
binaries are outside Git at /tmp/maia-wc-cast-repro/. At initial diagnosis no compiler repository had been edited; the published fix
and consumer commits are recorded in the checkpoint below.

Required fix: lower do-while body expressions through the compiler's existing
C++ expression/cast conversion in the principal maiacpp repository, preserving
loop iterations, side effects, nested statements and C89 validity. Add regression
coverage for cast-bearing body/condition, nested loops, continue/break and the
original minimal case; compare native C++98 semantics with Maia-generated WASM.
Keep existing tests and fail strict checking if C++ syntax remains in C output.
No source rewrite in wc to conceal the generator defect, no generated-parser edits.

Per the synchronization protocol, validate and publish the principal MaiaCpp fix
before updating MaiaJS, MaiaGlob and MaiaOS consumers via their own submodules,
with tests at each step. This requires explicit cross-repository/publication
approval. No npm, versions or history rewrite. MaiaOS feature commit/push remains
separately unauthorized.

## Acceptance

- Native C++98 ASan/UBSan counting/formatting oracle, chunk boundaries, all byte
  classes and near-overflow counters; actual WASM stdin/EOF/backpressure/error/
  malformed argv tests with manifest/import checks and stable stack/memory.
- Kernel tests for exact whitelist, immutable argv, no VFS delegation to wc,
  unsupported position/options/operands rejected before reservations, ownership,
  failed launch/trap/cancel/owner exit, resource cleanup and v1/v2 compatibility.
- Production boot/scheduler integration with actual kernel/shell/cat/wc/I/O/LVGL:
  known text, empty/binary/UTF-8 input, large input through tiny rings, quoted/glob
  inputs, relative/private paths, two/four stages, redirected and appended counts,
  negative forms before mutation, diagnostics on stderr and repeated cleanup.
- Strict builds for changed guests, unit and existing worker/pipeline/redirection
  regressions; frontend equivalence if AST extraction changes. Browser interaction
  is a separate check; no device or MaiaWASMVM execution claim.

The worker implementation uses existing public APIs. The owner approved the
fixed kernel/host whitelist and shell execution changes above, excluding MaiaOS
commit/push, npm and version changes.


## Implementation checkpoint

The principal MaiaCpp fix is published as f5f0208: distinguish do-while AST nodes
before the while branch and lower both body and condition through the existing
AST generator. Five native C++98/strict C89/actual WASM regressions and the full
MaiaCpp npm test passed. No parser/grammar or wc source workaround was used.
MaiaJS consumed it and passed test:full (e8fbb26 plus regenerated-manifest commit
f8f5184); MaiaGlob consumed it as d4a0fb9 and passed make test-wasm, including
10638 native/WASM cases. MaiaOS consumes f5f0208/d4a0fb9 via its own submodules.
No version/npm/history changes; MaiaOS feature and gitlinks are uncommitted.

wc.wasm is 4704 bytes and starts with two pages. Strict C89 build, native
ASan/UBSan and actual WASM worker tests pass. Kernel, mixed worker and production
boot/shell/I/O/LVGL integration cover both plan versions, rejected forms without
launch/mutation, zero wc VFS authority, binary/empty/private/quoted/glob inputs,
redirection/append, failed package fetch/retry, profile denial, cancellation,
traps, parent exit, repeated cleanup and stable memory. See AGENT_HANDOFF.md for
exact commands/results and remaining browser/device limitations.
