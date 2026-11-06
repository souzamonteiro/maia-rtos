# Bounded basename and dirname utilities

Date: 2026-10-27. Status: implemented and validated locally under the owner's request
to continue shell development.

## Scope and boundaries

Add `basename [--] path [suffix]` and `dirname [--] path` to the existing
simple-command dispatcher. This is a bounded utility increment, with no new
execution model or host API. Operands still use generated shell parsing, existing
quote/status/glob expansion, redirects and delayed completion/status handling.

Treat expanded operands as byte strings without VFS access, path resolution,
normalization or cwd changes. Trim trailing slashes. Basename emits the final
component; strip a literal nonempty suffix only when shorter than that component
and exactly matching its tail. An empty path produces an empty basename.
Dirname removes the final component plus its immediately preceding slash run;
no parent component produces `.` for relative paths and `/` for rooted paths.
An empty path has dirname `.`. All-slash paths produce `/` for either utility.
Preserve interior repeated slashes, dots and UTF-8 bytes. For example dirname
`//a/b` is `//a`, while dirname `//a` and basename `//` are `/`.

One leading `--` ends operand-option recognition; otherwise dash-prefixed words
are literal operands. No GNU/BSD `-a`, `-s`, zero termination or multi-path mode.
Append one LF to a successful result. Missing/excess operands diagnose on stderr
and return existing usage status2 without stdout. Existing 95-byte word/twelve-
word/255-byte line limits remain; no heap allocation, recursion or output-sized
scratch is introduced. Pipeline uses remain unsupported fixed-worker profiles.

No parser/lexer replacement, EBNF/compiler/dependency edit, new syscall,
capability, kernel/I/O implementation, worker package or ABI change. No commit,
push, npm, versions or history operation.

## Changes and validation

Production: a small shell-path.hpp helper plus shell.cpp dispatch/help integration;
regenerated shell.wasm. Add an actual C++98 ASan/UBSan native utility harness and
real WASM cases, advertise the test command, extend existing real pipeline/LVGL
checks and the fixed simulator recipe/expected bytes. Update progress trackers.

- [x] Independent expected outputs and native basename/dirname comparisons for
  empty/root/relative/trailing/repeated slash paths, dots, Unicode, suffix exact/
  equal/absent/empty and dash-prefixed operands; native ASan/UBSan.
- [x] Build through own C++98 -> strict C89 -> MaiaC/MaiaWASM; actual guest byte
  parity, quotes, glob and exact word bounds, no standalone VFS/cwd effects.
- [x] Usage/expansion/redirect/close errors preserve status, negation and AND-OR
  selection; lazy skipped effects, scripts, EOF and stable repeated resources.
- [x] Actual KEY/kernel/I/O/LVGL rendering and worker byte counts of redirected
  results; recipes twice per-line and by guest dot, existing status/list/script/
  redirection regressions,25 boot sessions,1,000 parses and unit tests.

Interactive browser, MaiaWASMVM/device and full POSIX utility conformance remain
separate validation gates. Literal pathname transformations never check whether
a file exists, nor grant access to the named path.


## Completed local evidence

Production source: shell-path.hpp literal byte transformations and shell.cpp
include/dispatch/help. No extra buffer/allocation, generated syntax or file probe.
Usage is checked before stdout; normal redirect setup/closure is unchanged.
Test command test:shell-paths is registered in package.json without version edits.
Existing help expectation names both commands exactly. Recipe01 adds3 commands
plus exact expected JSON/text bytes; fixed image stays16files/five recipes.

Passing commands:

- `npm run test:shell-paths`:13 independent +704 native utility byte comparisons
  through C++98 ASan/UBSan and the same actual WASM cases. Quoting/glob/UTF-8,
  empty/root/dot/repeated slashes, exact/equal/empty suffixes, literal dash operands,
  95/96-byte limits, usage atomicity, no standalone VFS access/cwd changes,
  skipped effects, expansion/redirect/close errors, status/negation/AND-OR,
  script return, EOF and150 repeated utility/file groups pass.
- `npm run build:shell -- --check-c89`: own C++98/strict-C89/MaiaC/MaiaWASM;
  shell.wasm1,422,245bytes, SHA256
  74501984f742317aa31e288aaaa98849db893330989cc70f212b7c47435862d2.
- `npm run test:shell-status`, `test:shell-lists`, `test:shell-scripts`,
  `test:shell-redirection`: retained real guest lifecycle, status/negation,
  snapshot, bounds and printf fields/partial/failing I/O regressions pass.
- `npm run test:shell-pipelines`:257flushes, real KEY/kernel/I/O/LVGL/cat/wc;
  five recipes/46commands twice per-line and all five dot files twice, including
  counting a redirected basename result. Unsupported utility stages stay rejected.
- `npm run test:shell-lvgl`:2,125flushes/1,000parses/77,911kernelsteps,stable
  shell20/I/O2pages. `npm run test:shell-boot`:25sessions/28flushes,zero remaining
  windows/endpoints. `npm test`:66pass.

The first new guest test incorrectly assumed startup cwd was home; production
initializes it to `/`. Corrected that fixture and added a second non-root cwd
invariance check; no production workaround or expected utility-byte relaxation.
Standalone native-only preliminary run covered688 comparisons; final combined
run includes two extra dash operand paths (704).
No full5839-input main/306+158word parity or unchanged native planner/framing/
formatter/kernel helpers rerun; earlier evidence retained. No interactive
browser, MaiaWASMVM/device or full POSIX conformance claim. Principals untouched;
root main b13f168 preserves prior dirty work and staged Cpp/Glob/Regex pointers.
No commit/push/npm/version/history operation.
