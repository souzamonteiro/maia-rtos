# Bounded VFS script input — approved implementation contract

Date: 2026-10-26. Status: owner approved; implemented and validated locally.
Baseline: main b13f168 plus locally validated foreground lists and prior work.
The following implementation and reference tests are complete in the working tree; publication is excluded.

## User-visible scope

Add the dot builtin `. path`, executing a bounded VFS file in the current shell:

```sh
. /system/tests/01-files.sh
. /system/tests/02-streams.sh
. /system/tests/03-errors.sh
. /system/tests/04-lists.sh && echo completed
```

Accept exactly one path operand after the existing quote removal/filename
expansion. Resolve it directly against the current cwd, including names without
a slash; no PATH search. Cwd changes remain visible to subsequent script and
caller commands. No positional arguments, source alias, sh process launch or
separate subshell is added. Invocation redirections are rejected before any
redirect or script open; commands inside the file retain their existing redirects.
Dot as a worker pipeline stage remains outside the fixed cat/final-wc profile.

Allow dot as a selected foreground list unit, including `&&`/`||`. A skipped
invocation must not open/read the file. Allow one active script only: a nested
dot invocation fails with internal status 2 before opening another script or
overwriting caller state. This is a declared bounded subset, not POSIX dot
conformance or complete script support.

## File loading and physical input limits

- Use existing guest `maia.vfs.open/read/close` with read-only flags and existing
  namespace/capability checks. No host filesystem access or command interpreter.
- Cooperatively load at most 16,384 bytes into one bounded guest snapshot.
  Honor partial reads and WOULD_BLOCK. Close the script handle before executing
  any body command. Size/read/close failure must not execute a partial snapshot.
  A size check at exactly the byte limit must distinguish EOF from excess input.
- Validate physical framing before body effects: at most 256 physical lines,
  at most 255 bytes excluding LF per line, and no NUL bytes. An unterminated
  nonempty final line counts as a line; empty file has zero lines. Preserve CR
  and other bytes for the generated lexer rather than silently normalizing them.
- Frame only physical LF-delimited input, just as the current terminal reader
  does. This does not parse shell operators or constructs. Every line goes
  through the existing generated MaiaCC lexer/parser, AST validator and executor.
  No string splitting on shell operators or handwritten multiline recognition.
- Keep the existing per-line 32-token/1024-node/eight-unit limits, 95-byte words,
  twelve expanded words per unit and fixed pipeline/stream/manifest limits.
  The script snapshot and caller bookkeeping must fit the unchanged 32-page
  shell memory budget and four-handle limit; actual consumption must be measured.
- Quoted multiline words, escaped-newline continuation, compound bodies across
  lines and heredocs remain unsupported. Do not concatenate incomplete lines,
  accept an invalid prefix or imply full POSIX script parsing.

The snapshot gives deterministic source bytes for this invocation. Commands may
write the original file under normal VFS authorization; those writes do not
change the already loaded script. A later invocation loads the current file.

## Execution, status and caller continuation

Feed one complete physical line at a time. Wait for the current line's full
foreground list to complete, output to drain, owned handles to close and worker
groups to reap before selecting the next line. Keep cooperative step budgets;
do not recurse through script lines or synchronously wait on workers.

Blank/comment-only lines have no command effect and do not replace the script's
last executed status. An initial empty script succeeds. Ordinary command errors
retain their existing diagnostics/status and continue to the next line; no
implicit errexit behavior is introduced. The script result is its last executed
unit's status, subject to the following abort conditions:

| Condition | Behavior / internal status |
| --- | --- |
| Missing operand, extra operand, nested dot, invocation redirect | Reject invocation / 2 |
| File open/read/close or allocation failure | Abort load, execute no body / 1 |
| Snapshot bytes/line bounds or NUL violation | Abort load, execute no body / 2 |
| Generated lexical/syntax error, unsupported AST or static bounds in a body line | Reject that whole line, abort remaining script / 2 |
| Ordinary selected command or expansion failure | Finish the line's AND-OR semantics, continue next line |
| Bare exit in body | Stop script and caller remainder; exit current shell |
| Fatal terminal/process transport failure | Existing fatal teardown; never execute subsequent lines |

A body syntax/structure error may follow successful earlier lines: preserve
those earlier effects; do not claim whole-script transactional syntax validation.
Report the failing physical line number with script-level abort diagnostics.

Preserve the caller's physical line and next-unit index in bounded guest storage.
On normal completion/error, regenerate its AST using the same generated lexer/
parser and restore the saved next-unit index, then finish dot with the script
status. Never rerun already executed caller units or eagerly expand remaining
ones. A bounded reparse avoids copying an entire second tree; if restoration of
the immutable, previously accepted caller unexpectedly fails, stop as an internal
fatal error rather than reinterpret source with a fallback parser.

Script input is separate from terminal stream 0 and command `<` input. Do not
consume queued terminal bytes while loading/executing a script. Bare cat retains
its current usage policy; it does not read subsequent script lines. EOF in the
script returns to its caller, not process EOF. Existing terminal EOF and bare-exit
process status remain unchanged. Emit no per-line prompt or command echo; emit
one prompt when the entire caller foreground list completes. I/O retains keyboard
editing/history and real LVGL rendering.

## Affected files and boundaries

Expected implementation: shell.cpp and a focused C++98 script-input helper,
completion/status integration with existing list/glob helpers where needed,
shell.wasm, focused native/reference WASM tests, actual kernel/I/O/LVGL script
tests, VFS recipe instructions, shell/kernel/I/O progress and handoff documents.
The host continues to mount recipe bytes only; it never executes script commands.

No EBNF/generated-parser change is expected. No kernel, I/O, worker, compiler,
dependency, manifest limit, capability, syscall or packed process-plan change is
included. Report any required additional contract/owner change before proceeding.
No commit/push, npm release, version change or history operation is authorized.

## Required validation

- [x] Native C++98 ASan/UBSan for any independently testable framing helper;
  byte/line/last-line bounds, embedded NUL and rejected partial snapshots.
- [x] Own MaiaCpp -> C89 -> MaiaC/MaiaWASM build with C89 syntax verification;
  generated frontend comparison and existing lexer/parser equivalence retained.
- [x] Actual guest dot: empty/comments, final line without LF, quoting/UTF-8 paths,
  relative paths/cwd retention and selected/skipped caller conditional units.
- [x] Native-shell differential output/side effects within this declared profile;
  caller prefixes run once, suffixes see current files/cwd and script status.
- [x] Partial/blocked reads, close/read/open errors, exact/over limits, source
  mutation after loading, unsupported multiline/static syntax, nested invocation,
  invocation redirects, no leaked handles and no use of script input as stdin.
- [x] Syntax failure aborts remaining body while ordinary command failure
  continues; blank/comment lines preserve status; exit/fatal stop caller suffixes.
- [x] Run all four existing VFS recipes by dot through actual KEY events and
  separate shell/I/O/kernel/cat/wc WASM with real LVGL; exact expected output,
  isolated read-only image and worker/resource reaping.
- [x] Repeated scripts and pipelines, terminal bytes queued behind invocation,
  output backpressure, stable stack/memory and unchanged manifest budgets.
- [x] Existing list/files/glob/directory/redirection/pipeline/LVGL/boot and unit
  regressions pass; trackers and handoff describe evidence and remaining limits.

Interactive browser acceptance, MaiaWASMVM/device execution and broader POSIX
conformance remain separate gates; do not infer them from reference WASM tests.

## Completed evidence — 2026-10-26

Implementation: shell-script-frame.hpp checks physical byte/line bounds only;
shell-script.hpp owns the fixed16KiB snapshot, cooperative VFS load/close,
per-line input framing and generated caller restoration. shell.cpp selects this
input source only while idle and records per-line completion status; the existing
glob-to-dispatch boundary recognizes dot before opening invocation redirects.
No EBNF/parser/kernel/I/O/worker/compiler/ABI/dependency/manifest change.
Generated production and comparison C++ headers remain byte-identical.

Artifact: shell.wasm1,316,463bytes; SHA256
32063f4be84f78a038b38a860ac76b6e20d31bcc9ea991324f98214627dab12a.
Measured shell19/I/O2 pages remain stable within unchanged manifest budgets.
The snapshot uses static guest storage; it creates no extra heap allocation or
retained source handle during body execution.

Passing final commands:

- `npm run test:shell-scripts-native`:19 native C++98 ASan/UBSan physical-framing
  edge cases (including exact16KiB/256lines/255bytes, excess, NUL, UTF8/CR/EOF).
- `npm run test:shell-scripts`: actual guest, empty/comments/shebang, no-final-LF,
  UTF8/relative paths, cwd and generated caller continuation, conditional status,
  nested/redirect/usage rejection, no skipped opens, quotas, invalid UTF8/syntax/
  static lines, snapshot bounds and mutation, native output/file/cwd comparisons,
  partial/blocked/read/close failures,150 repetitions, queued terminal lines,
  loader/body peer closure, EOF and exit with zero leaked handles.
- `npm run build:shell -- --check-c89`: own MaiaCpp/C89/MaiaC/MaiaWASM chain.
- `npm run test:shell-equivalence`:5833 JS/native/WASM cases;2856accepted,
  2167syntax/810lexical failures; exact UTF16 spans/full trees/restored stack.
- `npm run test:shell-lists`, `test:shell-files`, `test:shell-glob`,
  `test:shell-directories`, `test:shell-redirection`: retained passing regressions.
- `npm run test:shell-pipelines`:4 recipes/28 commands twice in per-line mode,
  then all4 scripts twice through dot/real KEY/kernel/I/O/cat/wc/LVGL; caller
  recovery,30 repeated script pipelines, worker/output failure and exit cleanup.
  Recipe03 intentionally aborts at its syntax error on physical line4; subsequent
  lines remain covered by the unchanged per-line tests. Final173LVGLflushes.
- `npm run test:shell-lvgl`:2004flushes,1000parses,77911kernelsteps,stable19/2pages.
- `npm run test:shell-boot`:25sessions/28flushes. `npm test`:66passed.
- `git diff --check` and byte comparison of generated C++ parsers: pass.

Logs: /tmp/maiaos-script-*.log. The VFS README now distinguishes per-line expected
files from dot's no-command-echo output and recipe03's intentional abort.
Interactive browser, MaiaWASMVM/device, new kernel/I/O builds and native kernel
policy/sanitizer suites were not rerun for this shell-only increment. Prior
separate native syntax/recursion checks remain recorded in the foreground-list
checkpoint; no new POSIX conformance claim is made.

main remains b13f168 with all prior dirty work and staged dependency pins
preserved. No commit/push, npm/version or history operation. Next development
requires a bounded contract for variables/status expansion or extended script
syntax; direct multiline editing/paste and generic sh process execution remain
pending.

## Conditional execution follow-up — 2026-10-31

The separately approved [if contract](SHELL_IF_EXECUTION.md) adds bounded one-line
nonnested conditionals while preserving8 total leaf units and current interfaces.
A private32-slot forward plan now saves the caller's next instruction PC across
dot reconstruction, without replay or another tree/branch stack. Conditional
status/negation, actual WASM/kernel/I/O/LVGL and existing regression gates pass.
The original exclusions/results above describe their earlier approved slices;
loops, nested/multiline compounds and broader POSIX conformance remain pending.
