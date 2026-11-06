# Bounded printf field formatting

Date: 2026-10-27. Status: implemented and validated locally under the owner's request
to continue bounded shell development.

## Increment and boundaries

Extend only the existing standalone printf formatter after generated shell
parsing/expansion: decimal field width, left alignment (`-`), string byte precision,
integer minimum digit precision and integer zero padding (`0`). Examples:
`%8s`, `%-8.3s`, `%04d`, `%8.4x`, `%.0u`. Repeated `-`/`0` flags are accepted
within the profile. Zero padding is ignored for left alignment or explicit integer
precision; it follows a negative sign. Numeric zero with precision zero emits no
digits. Width/precision count bytes, not Unicode display cells; string precision
may split UTF-8. Existing numeric input rules, bounds and empty argument behavior
are retained. Missing precision digits after `.` mean zero.

Width and precision parameters are each limited to 2048; exceeding either is an
output-limit error without unchecked decimal arithmetic or large loops. The
aggregate output still has its existing 2048-byte limit. Formatting is measured
and validated before emission; errors produce no stdout, while redirects retain
the existing open/truncate ordering. No extra output buffer/allocation is added.

Only `s`, `d`, `i`, `u`, `o`, `x`, `X` accept these modifiers. `0` on strings is
unsupported; `%b`, `%c` and `%%` keep their unmodified profile, including existing
byte escape/stop semantics. Dynamic `*`, positional arguments, `+`/space/`#`,
floating point, length modifiers and locale remain deferred.

This is a routine extension within the existing command formatter and completion
model. No shell EBNF/generated frontend, execution architecture, public host ABI,
syscall, capability, worker package, kernel/I/O implementation or dependency
change. No compiler edits, commits/pushes, npm or version changes are included.

## Files and acceptance

Expected source: apps/shell/shell-printf.hpp only. Update its native sanitizer
and actual WASM redirection tests, real KEY/kernel/I/O/LVGL cases and simulator
recipe bytes; regenerate shell.wasm and record progress in shell/kernel/I/O
READMEs and handoff. The main lexer/parser remains exclusively MaiaCC generated.

- [x] C++98 ASan/UBSan comparisons against native Bash printf in LC_ALL=C, with
  independent expected bytes for sign/zero/precision precedence and numeric bounds.
- [x] Literal/zero/missing width/precision, format reuse, missing arguments,
  empty/truncated UTF-8 strings and unchanged %b/%c behavior.
- [x] Exact 2048/2049 aggregate limits, oversized/overflow parameter rejection,
  unsupported combinations and late invalid conversions produce no stdout.
- [x] Own C++98 -> strict C89 -> MaiaC/MaiaWASM shell artifact; real WASM byte
  comparisons, binary/truncated UTF-8 output, redirects/partial writes/failures,
  status/list/negation/script behavior and repeated stable resources.
- [x] Real KEY/kernel/I/O/LVGL pipelines count formatted bytes, simulator recipes
  run twice per-line and by guest dot, and existing boot/unit regressions pass.

Interactive browser, MaiaWASMVM/device and full POSIX conformance remain separate
validation gates. No rendered host approximation replaces guest formatting.


## Completed local evidence

Only shell-printf.hpp has new production source logic: bounded decimal field
values, a padding helper, precision-aware integer output and string byte limits.
The preflight/emission queue and execution lifecycle remain unchanged. Existing
invalid-format coverage replaces the newly supported %.1s case with unsupported
%.*s; it retains the other unsupported formats and adds explicit field failures.
Native and WASM tests include independent sign/zero/precision bytes and native
LC_ALL=C references. The first new WASM test run exposed literal LF characters
in the test command builder; those fixture formats were corrected to send escaped
backslash-n. No executor change or expected-output relaxation was needed.

Passing commands:

- `node scripts/test-printf-integers-native.mjs`: C++98 ASan/UBSan;108 field
  comparisons against native Bash, independent bytes, repeated flags, sign/zero/
  precision precedence, missing/reused arguments, split UTF-8 and exact2048-byte
  output plus oversized parameter/aggregate rejection. Existing numeric and
  %b/%c/escape/stop/atomicity checks retained, including stop before oversized
  unvisited field syntax.
- `npm run build:shell -- --check-c89`: own strict C++98/C89/MaiaC/MaiaWASM;
  production shell.wasm1,420,648bytes, SHA256
  a654a02f936c5ad61a76c69f82fe0113b35d78d0a4a0f7f6d8d845f257972e84.
- `npm run test:shell-redirection`: real guest byte-oracle checks, exact bounds,
  unsigned/signed extremes, precision bases, binary/truncated UTF-8, redirects,
  partial/WOULD_BLOCK and failed writes preserving accepted prefixes, status and
  negation;100 stable formatted-file repetitions. Final rerun includes added
  precision/reuse boundary and partial/error write cases; all earlier cases pass.
- `npm run test:shell-status`, `test:shell-lists`, `test:shell-scripts`: existing
  lazy selection, expansion, script return, negation, delayed failures and
  memory/stack/resource regressions retained and pass.
- `npm run test:shell-pipelines`: actual KEY/kernel/I/O/LVGL/cat/wc,256flushes;
  five recipes/43commands twice per-line and all five dot files twice. Recipe02
  adds3 field rows, including a formatted file counted by a real wc worker.
- `npm run test:shell-lvgl`:2,090flushes/1,000parses/77,911kernelsteps,stable
  shell20/I/O2pages. `npm run test:shell-boot`:25sessions/28flushes,zero surviving
  windows/endpoints. `npm test`:66pass.

No compiler/dependency/grammar or generated frontend changes. Full5839-input
main parity, word306/158 and unchanged native list/framing/kernel helpers were
not rerun; earlier checkpoints remain valid evidence, not new runs. Interactive
browser, MaiaWASMVM/device and broader POSIX conformance remain unrun.
Root main b13f168 plus preserved prior dirty work/staged Cpp/Glob/Regex pointers;
no commit/push/npm/version/history operation.
