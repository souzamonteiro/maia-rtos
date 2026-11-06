# Bounded break/continue — approved execution contract

Date: 2026-10-31. Status: explicitly approved; implemented and validated locally (reference engine).
Baseline: main b13f168 plus the locally validated for/while/until checkpoint.
The owner explicitly approved this contract and implementation. Publication and
other-repository changes remain excluded; the previous exclusions are superseded
only within the scope below.

## Selected scope

Add shell-local `break` and `continue`, with no operand or exactly the operand
`1`, targeting the currently executing loop body in the current generated plan.
Examples:

```sh
for N in a b; do echo "$N"; break; echo NEVER; done
for N in a b; do echo "$N"; continue; echo NEVER; done
while true; do break; done; echo $?
N=aa; until test -z "$N"; do N=${N#a}; continue 1; done
```

The generated SimpleCommand/CmdName/Word/foreground-list ownership and existing
word/field/glob expansion determine final argv. Quoted or expanded command names
work like existing builtins. No new keyword, handwritten lexer/parser, synthetic
AST, source split or substring reparse. Keep main/word/test EBNFs and all generated
frontends unchanged. The exact final argument `1` is a finite builtin operand;
other numeric spellings/options, higher levels and additional operands return
ordinary status2 with a bounded diagnostic, without a control transfer.

Only a selected builtin in an active body's current plan may request a transfer.
For/while/until remain one loop of any kind per physical line; compound nesting
and multiline input remain unsupported. A selected control command outside a
body (including a while/until condition) returns ordinary status2 with a bounded
diagnostic and leaves its PC/frame unchanged. This is a deliberately bounded
MaiaOS profile, not a full POSIX break/continue claim. Ordinary condition truth
still applies: an until condition containing such a usage failure may select a
body, under its existing12-body/13-condition limit.

## Transfer and status

Successful `break` skips all remaining units of the body and exits that loop at
its existing aggregate join. Successful `continue` skips remaining body units
and reaches the existing certified next operation: for advances its frozen index
and binds the next value; while/until records the just-completed body result and
reevaluates its condition. No header replay, reset of the iteration counter,
extra body/condition, variable rollback or new backward edge is permitted.

Both builtins have raw success status0. Preserve the existing completed-unit `!`
rule: a negated successful control still transfers, with completed status1.
Break makes that completed status the loop result. Continue makes it the completed
body status, used by final for completion or by conditional last-body preservation.
Outer loop-owned `!` and following ;/&&/|| then apply once under existing rules.
A skipped control command neither requests a transfer nor changes status.

Dash and Bash differ here: `while true; do ! break; done` and
`N=a; while test -n "$N"; do N=; ! continue; done` return0 in this installed dash
and1 in Bash. The selected MaiaOS result is1, consistent with its current unit
negation contract. Keep these as explicitly differing reference cases; never
report them as common dash/Bash or POSIX-conformance agreement.

## Cleanup and bounded private state

Use a small fixed pending-control request owned by the selected current unit:
action, loop kind/descriptor identity and checked target. Reuse existing active
for/conditional frames and plan validation; no evaluator stack or recursive jump.
Validate that the unit belongs to the active body and that the target is exactly
that descriptor's next/join. Reject stale, dual-active, invalid-phase/count/PC or
mismatched-owner state before applying a transfer. All old malformed-target tests
remain; no arbitrary PC assignment or new uncertified backward edge is allowed.

A valid invocation arms a request only after existing expansion, operand/context
checks and redirection acquisition succeed. Wait for ordinary command completion:
output/diagnostics drain, input/output/directory handles close and owned worker
groups finish/reap. If command preparation/open/close/drain fails, cancel the
request; retain the normal error/status/list path, even when `!` later changes
that error status to0. A failed builtin cannot accidentally transfer because
negation made its completed status zero. Preserve already completed file effects;
no rollback promise. Fatal transport/exit/shutdown clears pending control.

After successful raw completion and normal negation publication, consume the
request exactly once and reach the certified join/next before dispatching any
skipped body remainder. Clear it on preparation, rejection, completion and
shutdown. A request cannot survive into a different input line, dot script or
caller reconstruction. Keep32 control operations per dispatch and16 guest steps.
No synchronous resource polling and no changes to WOULD_BLOCK/partial transfers.

## Dot boundary

A script line with its own active body may use these builtins normally. A dot
script does not target its suspended caller's loop: a command on a script line
with no active local body returns2, even when its caller is inside a loop.
Nested dot remains rejected. Do not search suspended frames, unwind dot, or add
a script-return/loop-control stack. This explicitly excludes control transfers
across the dot boundary. Existing caller phase/count/frozen-index/status restore
remains unchanged; a script-local transfer must leave its suspended caller intact.

## Boundaries and acceptance

Expected production edits: small private loop-control helper and command/
completion glue, only the necessary certified frame/plan hooks, help text and
shell.wasm. Add independent fixtures/native/WASM checks, real integration cases,
one immutable `/system/tests/12-loop-control.sh` recipe/expected data, and update
shell/kernel/I/O progress evidence. Do not enlarge8leaves/32instructions,
32tokens/1024nodes,12argv/95byte words,32variables,255byte lines,16KiB/256line
scripts,12for values,12conditional bodies/13conditions,32shell pages/four handles
or8I/O pages. Measure actual21/2pages again. Every display asset including command
echo and final prompt must fit4096bytes; shorten the guide if needed.

No compiler/other-repository edits, dependency pointers, glob matching policy,
public syscalls/ABI/process plans, kernel/I/O/worker production, fonts, manifests,
new packages, commit/push/npm/versions or history changes. Preserve all existing
root work and the entire staged index. A demonstrated owner defect requires a
minimal reproduction and separate scope approval before editing its owner.

Acceptance gates — completed locally:

- [x] Fixed generated-JS command/body ownership and independent expected output/
  status, common dash/Bash finite cases and explicitly separate negation variants.
- [x] C++98 ASan/UBSan exact for/while/until control traces, certified next/join,
  invalid request identity/phase/target/PC/count, one-op yields and capacity canaries.
- [x] Actual own AST-strict C++98 -> strict C89 -> MaiaC/MaiaWASM guest: first/
  middle/final break/continue, frozen values, current binding/status, zero loops,
  negation/AND-OR, expanded commands/operands, skipped invalid patterns with zero
  side effects, invalid argument/context and recovery. Verify the conditional
  limit cannot be bypassed by continue; no13th body/14th condition.
- [x] Redirect open/close/fatal/blocked-drain failure cancels pending transfer;
  successful transfer waits for cleanup. No skipped-file/worker effects and no
  stale request after failed/negated commands, prompts, EOF, exit or shutdown.
- [x] Dot local-loop transfers, caller-loop/script-no-loop ordinary errors,
  independent for/conditional caller-script frames, suspended budget preservation,
  read/close/stall/EOF/fatal cleanup and at least150dot/200loop stable repeats.
- [x] All12recipes twice per-line and via dot through real KEY/kernel/I/O/shell/
  cat/wc/LVGL, immutable VFS/display bound, worker reaping, current128KiB partial-
  transfer/cancel/trap/owner-exit tests and new body-control cleanup cases.
- [x] Existing focused for/conditional/predicate/list/script/word/fields/variables/
  status/redirection/glob/grammar/parser/equivalence gates, test:shell-lvgl,
  test:shell-boot and npm test. Record exact commands and outcomes; keep known
  contextual gaps distinguished from failures and wider POSIX conformance.
- [x] Two `npm run build:shell -- --check-c89` builds reproduce shell.wasm and
  unchanged generated frontends; update affected checklists/hash/memory/results,
  audit scoped paths/full index and seven clean principal owners. Manual browser,
  MaiaWASMVM and physical device validation remain separate.

## Historical preparation — superseded by acceptance below

Read current planning/for/conditional frames, command expansion/completion,
generated dot restore, architecture and approved loop contracts. Current builtin
completion drains/closes before publishing negated status and dispatching the
next unit; this is the intended transfer integration point, not a new host API.

`node /tmp/maia-loop-control-preparation.mjs` passes20 independent generated-JS
loop parses and40 fixed dash/Bash stdout/status comparisons, including selected/
skipped control, for binding, while/until selection, explicit1, dynamic command
names, outer negation and skipped remainder. Two additional negated-unit examples
record the differing per-shell outcomes above. All native runs are finite and
bounded by a2second timeout. Temporary parser loader removed; no guest behavior
is claimed. Existing production while/until acceptance remains the last passing
checkpoint; no production build or broad regression rerun in proposal preparation.

Production shell.wasm remains1,663,448bytes/SHA256
`57fa2c8c4a181b989f8c1af36a56c1a2fbfc77bbe258493a11bcec951d6e0a40`.
Main remains b13f168 with the previous dirty tree and three staged gitlinks intact.
Only this proposal, the shell pending checklist and handoff are changed. Next
step is explicit approval of this concrete contract, then independent generated
fixtures -> sanitized planner -> actual guest/kernel/I/O/LVGL acceptance.


## Completed implementation and acceptance — 2026-10-31

Loop control (2026-10-31): this approved contract.
`break`/`continue` accepts no operand or exact1 in the current generated body.
One fixed request certifies descriptor/PC/unit and its existing join/next; raw
close/drain failure cancels before negation, successful completion transfers once.
Pure for-frame/copy/validator extraction preserves existing semantics and allows
native checks of the actual production helper. No suspended-dot-caller transfer.
29generated-JS fixtures/54common dash-Bash byte/status comparisons and4separately
recorded negation differences,16nativeASan/UBSan exact traces and29actual guest
fixtures pass. First/middle/final control,0/12/13bounds,corrupt/dual-active frames,
raw close failure with negation,blocked diagnostic drain and dot/IO/EOF/exit/fatal
cleanup pass;150dot/200loops retain stack/memory at shell21pages.
KEY/kernel/I/O/shell/cat/wc/LVGL:12recipes/171commands twice per-line and all12dot
files twice;20control/dot/worker resumes,12explicit-control worker-failure cases
and previous128KiB partial-transfer/trap/cancel/reap tests pass (1193flushes).
All16remaining regression gates,five earlier native gates,isolated fields guest,
focused control suites and66unit tests pass. Shell21/I/O2pages remain within the
unchanged32/8page budgets. Guide+echo+prompt3764bytes<4096.
Two strict builds reproduce shell.wasm1,666,203bytes/SHA256
`70dff66b805db5a8f5e19e4c8f9ccb3832938515da4b05987c631a608b8f15f9`. Main/word/test grammars/generated artifacts, kernel/I/O/worker production,
ABI/plans,fonts/manifests/glob policy,seven clean principal owners and the entire
preexisting staged index/pins remain unchanged. Root main b13f168 stays local;
no commit/push/npm/versions. Manual browser,MaiaWASMVM/device and fullPOSIX
acceptance remain separate. Exact host/boot output is in the approved contract.

Exact acceptance commands (all exit0):

| Command | Result |
|---|---|
| `npm run build:shell -- --check-c89` (twice) | Own AST-strict C++98 -> strict C89 -> MaiaC/MaiaWASM; identical production bytes and unchanged generated frontends. |
| `npm run test:shell-loop-control-native` | 29generated-JS/54common dash-Bash comparisons plus4explicitly differing comparisons;16C++98ASan/UBSan exact traces,raw cancellation,one-op yields,invalid request/frame/dual-active state and slot canary. |
| `npm run test:shell-loop-control` | 29production fixtures plus context/operand/lazy/field/Unicode/full-table/bounds,raw close failures before negation,blocked drain,dot boundary/resume/IO/EOF/exit/fatal tests;150dot/200loops,stable21pages. |
| `npm run test:shell-for-native` | Existing native framing/planner/predicate/sanitizer regressions pass. |
| `npm run test:shell-conditional-native` | Existing native framing/planner/predicate/sanitizer regressions pass. |
| `npm run test:shell-lists-native` | Existing native framing/planner/predicate/sanitizer regressions pass. |
| `npm run test:shell-predicates-native` | Existing native framing/planner/predicate/sanitizer regressions pass. |
| `npm run test:shell-scripts-native` | Existing native framing/planner/predicate/sanitizer regressions pass. |
| `npm run test:shell-grammar` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-parser` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-equivalence` | 6426raw JS/native C++98/WASM inputs;3243accepted/2353syntax/830lexical,exact tokens/spans/trees and restored stack. |
| `npm run test:shell-word` | 710word trees,327byte/mask,252field,16Unicode count/17invalid byte vectors;sanitizers and own-chain probe/reproduction. |
| `npm run test:shell-for` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-conditional` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-predicates` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-variables` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-status` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-lists` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-scripts` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-redirection` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-glob` | Existing generated syntax/production guest regression suite passes. |
| `npm run test:shell-lvgl` | 1000parses/2240LVGLflushes/77911kernel steps,stable shell21/I/O2pages. |
| `npm run test:shell-boot` | 25actual LVGL launches/exits,28flushes,zero surviving windows/endpoints. |
| `npm run test` | 66unit tests pass. |
| `npm run test:shell-fields` | Existing field/parameter/UTF-8 suites and isolated malformed-value guest pass;no production test exports. |
| `npm run test:shell-pipelines` | 12recipes/171commands twice per-line and12dot files twice;1193flushes;20dot/control/worker resumes,12control/failure combinations,prior128KiB partial transfers and traps/cancel/reap pass. |
| `node --test tests/simulator-vfs.test.js` | Both tests pass;byte-exact immutable mounts,display/image quotas,preserved writable files and all-or-nothing loading. |

Measured host/boot output:

```text
Kernel/I/O/shell session passed with actual LVGL (2240 flushes): generated lexer/parser, commands, quoting, invalid/unsupported input, bounds, ls/cd/cwd/escaped names, 1024-entry ls, relative/absolute/UTF-8 cat, 1000 repeated parses, stable stack/memory (shell 21, I/O 2 pages), 77911 kernel steps, drained/closed streams.
Actual boot functions passed: failed I/O/shell startup cleanup, keyboard ownership, 25 real LVGL session launches/exits, directory/file-trap cleanup, manifest quotas, unchanged URL, zero surviving windows/endpoints (28 flushes).
```

New guide/recipe/expected display bytes including command echo and next prompt:
3764/657/863<4096.
The complete expected.json is14753bytes<16384;only indentation was compacted,
with all previous data retained. An initial mount correctly rejected the larger
format;no limit was raised. The close-backpressure test initially used reserved
unquoted `done` as echo data;corrected to `recovered`,without changing grammar or
relaxing its cleanup assertions. All authoritative final runs pass.

Git audit:23current-increment paths,all baseline files retained,entire original
index/pins preserved,seven clean principal owners and protected grammar/generated/
kernel/I/O/worker/ABI/font/manifest/dependency bytes unchanged. For-frame extraction
moves only the existing struct/copy/validator,without changing their bodies.
Main remains b13f168 with prior uncommitted work. No staging,commit,push,npm,
version or history change. Manual browser smoke check of
`. /system/tests/12-loop-control.sh`,MaiaWASMVM/device and wider POSIX remain pending.
