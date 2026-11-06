# Bounded pipeline negation — approved execution contract

Date: 2026-10-27. Status: owner approved; implemented and validated locally.
Baseline: main b13f168 plus the existing locally validated shell increments.

## Purpose and scope

Execute the existing generated grammar's single optional `!` prefix on a
supported foreground unit: `! false`, `! cat /tmp/a | wc -l`, and
`! . /system/tests/05-status.sh`. This extends the approved foreground-list
contract, which explicitly excluded negation. It does not add a syntax rule.

Use only the MaiaCC-generated Pipeline subtree and its directly owned `!`
terminal. Store a bounded negation flag per list unit; ignore that terminal
when collecting command words. Quoted or escaped exclamation marks remain
ordinary word content. Do not scan/split source or alter generated artifacts.
The existing grammar rejects repeated prefixes such as `! ! true`.

## Completion and status

- For a selected unit, logical negation maps completed zero to 1 and every
  completed nonzero status to 0. Publish this result to `$?`, AND-OR selection
  and script status only once, after output/diagnostics drain, owned handles
  close and any worker group completes and is reaped.
- Capture the flag before runtime word rendering, including rendering overflow.
  Runtime expansion, command-not-found, usage and operational failures retain
  their diagnostics and effects; their completed statuses are inverted too.
  For example, `! missing_command; echo $?` prints a diagnostic followed by 0.
- A close failure participates in the original unit result before inversion.
  Never invert early and then overwrite the logical result during cleanup.
- The kernel still owns the last worker's raw pipeline result. Negate that
  completed result in the shell; add no pipefail mode or kernel status policy.
  Thus a missing input operand in a first cat does not override a successful
  final wc unless the existing kernel/transport contract reports group failure.
- Skipped units preserve status and cause no expansion, opens or worker starts.
  Each selected unit still snapshots the incoming `$?` once for all words.
- Whole-line syntax, unsupported structure and static-bound rejection retain
  status 2 and occur before any unit effects. They are not executable units
  whose errors a `!` prefix can turn into successful execution.
- Bare exit still stops the shell and remaining units. Fatal process/transport
  errors still terminate it. Negation does not cancel exit or recover a fatal
  guest failure; numeric process-exit semantics remain unchanged.

## Dot scripts and lifecycle

Preserve the caller dot unit's flag separately while its body overwrites the
current list/tree. Body units apply their own flags independently. After a
generated reparse restores the caller, restore its pending flag and invert the
dot return exactly once. Cover empty scripts, load/framing/line rejection,
ordinary body failure, negated final body units and negated caller suffixes.
An exit inside a script still exits; no caller suffix runs.

Preserve the current cooperative step budget, bounded pumping, WOULD_BLOCK,
handle ownership, cancellation/reaping and terminal EOF behavior. No recursion,
unbounded allocation or synchronous polling is introduced.

## Boundaries and expected files

Expected changes: shell-list.hpp, shell.cpp and shell-script.hpp; focused native
planner, status/list/script and real pipeline tests; the existing simulator
status recipe/expected bytes; shell.wasm; progress READMEs and handoff.
Keep at most eight units, 255-byte input, 32 tokens, 1024 main-tree nodes,
95-byte words, twelve expanded words and two-to-four fixed worker stages.

No grammar/generator/compiler/dependency edit, new worker package, host ABI,
syscall, capability, binary process-plan change, kernel/I/O implementation,
rendering replacement, npm publication, version change, commit or push.
Unsupported compound forms, assignments, background execution, richer pipeline
programs and POSIX special-builtin fatal-error semantics remain deferred.
Report any necessary owner-component change separately before editing it.

## Investigation completed before approval

Read production planner, static word validation, asynchronous completion and dot
caller restoration. The current validator rejects the owned non-word `!` token;
the generated parser already recognizes it directly under Pipeline.
Read-only generated JavaScript AST checks confirmed ownership for `! false`
and `! cat /tmp/a | wc -l`, ordinary words for quoted/escaped `!`, and rejection
of `! ! true`. Eight `/bin/sh` probes confirmed builtin truth values,
unknown-command and redirect failure inversion, AND-OR selection, last-stage
pipeline semantics and unconditional exit. These are investigation/oracle
checks, not evidence of implemented guest negation.

## Required implementation acceptance

- [x] Generated JS/native C++98/WASM token/tree parity for representative
  prefixes, quotes/escapes, pipelines, separators and invalid repeated prefixes.
- [x] Native C++98 planner ASan/UBSan: owned flag/ranges, eight/nine-unit bounds,
  truth table and unsupported whole-line atomicity.
- [x] Actual own C++98 -> strict C89 -> MaiaC/MaiaWASM shell build.
- [x] Guest truth/AND-OR/$? cases compared with explicit expectations and native
  /bin/sh; runtime failures, overflow and missing-command diagnostics retained.
- [x] Skipped effects, delayed input/output close errors, empty/delayed/failing
  dot return and caller reconstruction do not lose or double-apply negation.
- [x] Real kernel cat/wc completion, redirects, blocked I/O, cancellation/traps,
  exit/EOF and repeated runs preserve cleanup, stack and memory bounds.
- [x] Existing status/list/script/word/parser, redirection/pipeline, boot/LVGL
  and unit regressions; readable simulator recipe and progress evidence updated.

Interactive browser, MaiaWASMVM/device and broad POSIX conformance are separate
acceptance gates and must not be inferred from reference WASM/LVGL tests.


## Completed local evidence

The executor reads only the generated Pipeline-owned prefix. shell-list.hpp
stores a flag per unit; shell.cpp skips that owned syntax terminal during word
collection, captures the selected flag before runtime rendering and inverts once
at completion after close diagnostics drain. shell-script.hpp saves/restores the
pending caller flag independently of body execution and generated reparse.
No grammar, compiler, dependency, host, kernel, I/O or worker implementation
changed. Existing list rejection coverage now uses a negated unsupported compound
instead of the newly supported simple-command prefix; all other cases remain.

Passing commands:

- `npm run test:shell-lists-native`:25 generated-tree plans, C++98 ASan/UBSan,
  exact prefix ownership, quoting, ranges, eight/nine-unit bounds and selection.
- `npm run build:shell -- --check-c89`: own C++98/strict-C89/WASM chain;
  shell.wasm1,418,431bytes, SHA256
  e3a5bd5b7dbdbe07cbaf7ba61b4be7d37a3be4c6fece9ceb83da1afff3e19e6a.
- `npm run test:shell-status`: native shell truth-table comparisons, unchanged
  diagnostics, skipped opens/enumeration, static rejection, runtime overflow,
  negated dot load/frame/line failures, delayed/empty/body/caller return,
  close/read failures,150 repeated negated scripts and exit/EOF cleanup;
  previous150 status and200 builtin repetitions retained.
- `npm run test:shell-lists`, `test:shell-scripts`, `test:shell-redirection`:
  existing real guest bounds, selection, failure/partial/backpressure/resource
  and script snapshot/return regressions pass.
- `npm run test:shell-equivalence`:5,839 JS/native C++98/WASM raw inputs;
  2,861 accepted,2,168 syntax and810 lexical failures, zero token/span/tree
  divergences and restored WASM stack. Six new unique negation probes.
- `npm run test:shell-word`:306 generated JS/native token/span/tree cases,
  158 byte/mask cases, ASan/UBSan and artifact reproduction pass.
- `npm run test:shell-pipelines`:256 actual LVGL flushes; real kernel/I/O/cat/wc,
  last-worker status, six injected failure modes both normally and negated,
  late output close failure, cancellation/trap/reap and prior128KiB slow I/O.
  Five simulator recipes/40 commands twice per-line and all five dot files twice.
- `npm run test:shell-lvgl`:2,158 flushes,1,000 parses,77,911 kernel steps,
  stable shell20/I/O2 pages. `npm run test:shell-boot`:25 sessions,28 flushes,
  zero surviving windows/endpoints. `npm test`:66 passing tests.

No interactive browser, MaiaWASMVM/device, separate kernel/I/O native-policy
rebuild or full POSIX-conformance run. No commit/push/npm/version change.
