# Bounded while/until execution — approved contract

Date:2026-10-31. Status:explicitly approved; implemented and validated locally.
Baseline:main b13f168 plus the validated local explicit-list for checkpoint.
The generated grammar already recognizes WhileClause/UntilClause. At the baseline,
execution rejected them; the acceptance below demonstrates actual loop execution.
Implementation within this contract is approved; publication and other-repository work remain excluded.

## Selected scope

Execute these existing generated forms on one physical input line:

```sh
N=aaa; while test -n "$N"; do echo "$N"; N=${N#a}; done
N=aaa; until test -z "$N"; do echo "$N"; N=${N#a}; done
while false; do echo NEVER; done; echo $?
```

While executes the body when the completed condition list returns0. Until
executes it when that result is nonzero. After each completed body, reevaluate
the condition. Conditions and bodies are nonempty lists from the generated
TerminatedCompoundList/DoGroup ownership, using existing supported simple units,
assignments, ;/&&/||, negation, test, fixed cat/final-wc pipelines, redirects and
dot under their current limits. No extra expression parser or test syntax.

At most one loop of any kind (for/while/until) per physical line. Existing for-
only behavior stays unchanged; mixed/sequential loop constructs on the same line
remain excluded. Existing nonnested if may be an outer sibling. Reject all
compound nesting, including if in a loop or a loop in if, before any effects.
Reject loop pipeline stages and compound-level redirections; retain supported
unit redirects/pipelines inside the condition/body. Multiline input, break/
continue builtins, jobs, general launch and implicit/arithmetic for stay excluded.

Preserve eight static leaf descriptors for the whole line, including conditions,
bodies, skipped work and outer siblings;32 private instructions/main tokens and
1024main nodes. Do not duplicate syntax/leaf descriptors for later iterations.
Main/word/test EBNFs and generated artifacts are expected to remain unchanged.
A demonstrated grammar/owner defect needs a separate minimal reproduction and
explicit scope decision before edits.

## Status and reevaluation

Each selected condition/body unit uses ordinary lazy word expansion, field/glob,
VFS/streams and completed status. Unlike a for header, condition/body words expand
anew on each selected visit and see current bindings/cwd/files. A skipped outer
loop performs no condition/body expansion or VFS/process/binding effects. Full
structural/literal/quota validation still examines the entire generated line.

The loop result is0 if no body ran, otherwise the last completed body list's
status. Save that status separately: the final condition status must not replace
it. Condition side effects still persist, including those from the terminating
condition. First condition sees incoming $?; first body sees the selecting
condition's status (0 for while, nonzero for until). Later conditions see the
previous body's completed status. Ordinary per-unit snapshots within lists stay.

Normal nonzero/usage/expansion/I/O results remain condition statuses: a failed
while condition exits, and a failed until condition selects its body. There is
no errexit or implicit break. Keep diagnostics and completed effects; do not
turn a runtime condition failure into a structural rejection or promise rollback.
Bare exit, fatal transport failure and guest shutdown stop the shell regardless
of loop selection. Apply a loop-owned outer ! once after aggregate completion/
cleanup; unit/dot-owned ! is independent. Outer AND-OR skipping preserves prior
status and must not run aggregate zero/inversion logic. Emit one outer prompt.

## Explicit resource limit

Permit at most12 body entries per conditional loop. Permit a thirteenth complete
condition evaluation so a loop that terminates naturally after its twelfth body
can still finish normally. If that condition would select a thirteenth body,
do not enter the body: report exactly one bounded additional diagnostic
`shell: loop iteration limit exceeded\n`
and finish the loop with ordinary status2. Outer ! and ;/&&/|| then follow their
existing status rules; the shell stays usable. This is a documented MaiaOS
resource bound, not a claim of unbounded POSIX loop semantics.

Keep all effects of the12 bodies and13th condition; no transactional rollback.
Do not evaluate a fourteenth condition, replay part of a condition or silently
truncate a loop with success. Conditions that terminate earlier do not consume
additional visits. No-body while/until returns0 even if its terminating condition
was a usage/I/O failure, with that condition's diagnostic still visible.

The bounded maximum is104static-leaf dispatches per physical line including
outer units (a conservative13x8bound). Dot execution remains independently
bounded by its16KiB/256physical-line/no-nested-dot contract. Test exactly12bodies
with natural termination and attempted13th entries separately; native unbounded
oracles do not define the project's budget-exhaustion result.

Keep32control operations per private dispatch and the current16-step guest
quantum. Pending work/yield must remain distinguishable from end-of-list.
No recursive evaluator, synchronous worker/VFS/stream polling or unbounded edge.

## Private plan and continuation state

Build condition/body descriptors and entry/check/next/join instructions over
actual generated ownership. Reuse the existing unit collector/renderer/dispatch,
not synthetic Command/Pipeline nodes, source splitting or substring reparsing.
Only a certified conditional-next edge may return to its own condition entry.
All unrelated jumps remain forward; preserve the existing certified for-next
edge and every old malformed-target/capacity rejection.

Maintain a fixed private conditional-loop frame containing descriptor identity,
while/until kind, condition/body phase, checked body-entry count, whether a body
completed, last body status and continuation ownership. Initialize it only on
selected entry. Phase/count checks must prevent double budget consumption on
resume or entry into a body without a completed selecting condition. Verify
bounds/targets/state before using them. No need to rename/rewrite the working
for snapshot machinery: a small conditional frame and its saved caller copy
can coexist with the current fixed for frames. One loop per line means only
one active loop kind in that execution context; no runtime frame stack.

Advance/check only after pending output drains, owned input/output/directory
handles close and worker groups complete/reap. Delayed failures must be reflected
in condition/body status before choosing or repeating work. Keep WOULD_BLOCK,
partial transfer, cancellation and quotas. A budget error must drain stderr and
complete cleanup before outer list selection; it cannot leave a worker or handle
owned by the finished loop.

## Dot caller preservation

Allow dot in conditions or bodies under the existing single active script rule.
Save/restore the conditional frame together with the current saved caller source,
PC and independent negation. Preserve phase, number of body entries and last
completed body status. Return reconstructs the caller through the same generated
frontend, validates the saved frame/PC against that plan, then continues without
rerunning earlier condition/body units or restarting/resetting the counter.

A dot script physical line may execute its own nonnested for/while/until while
its caller loop is suspended. Keep one fixed conditional caller frame, current
for caller snapshot, one script snapshot and one saved caller line; no additional
AST, recursion or dynamic context stack. Restore the appropriate active kind,
not both; preserve the other kind's existing bounds and frozen values. Script
variables/cwd/file effects remain shared; never restore the old variable table.
Nested dot remains rejected. Read/close/framing failure, early script exit,
EOF/fatal transport and loop budget failure must neither replay a call nor
lose the caller's saved body status or count.

## Boundaries and acceptance

Expected edits are private shell planning/continuation, small bounded frame
storage, focused fixtures/native/WASM tests, existing integration adapters, one
immutable VFS recipe with exact expected responses, reproduced shell.wasm and
shell/kernel/I/O progress/evidence docs. No compiler-owner edits, changes to
pattern semantics/glob library, host ABI/syscalls/capabilities/process plans,
kernel/I/O/worker production, fonts, manifests or dependency pointers.

Retain line255/word95/argv12/variables32 and all current script/worker bounds.
Shell32page/four-handle and I/O8page budgets stay; actual baseline21/2pages is
measured again, not assumed. Every guide/recipe/expected text plus command echo
and final prompt must fit4096bytes (baseline guide3597). No raised display limit.
No commit/push, npm, versions or history rewrite. Preserve all prior root work
and the entire staged index; owner defects require separate explicit approval.

Acceptance gates (completed locally on2026-10-31):

- [x] Independent generated-JS condition/body ownership and exact expected
  statuses/bytes before native planning; dash/Bash oracles for the shared finite
  profile, zero/one/multiple iterations, while/until selection, saved body versus
  terminating condition status, $? snapshots, outer/unit ! and lazy lists.
- [x] Native C++98 ASan/UBSan exact phase/count/edge traces; one-op yielding,
  malformed phase/targets/counts, no arbitrary backward edge,8/9leaves and
  32/33flow slots/canaries. Keep every current for/if/list planner regression.
- [x] Actual own AST-strict C++98 -> strict C89 -> MaiaC/MaiaWASM:0/1/12bodies,
  naturally terminating13thcondition, would-enter13th-body error2, exactly one
  budget diagnostic, no14thcondition, persistent effects and subsequent recovery.
  Unbounded native loops are not run as budget oracles.
- [x] Actual condition/body field/glob/Unicode/status/variable/full-table/limit
  cases; skipped malformed UTF8/invalid patterns and zero skipped VFS effects.
  Check failed conditions according to while/until truth, without hidden fatal
  semantics, and final condition side effects without losing last body status.
- [x] Dot in conditions/bodies across read/close/short/stalled/error paths,
  for-caller/conditional-script and conditional-caller/for-script combinations,
  preserved budget/phase/body status, no replay/extra opens and nested-dot rejection.
  At least200repeat loops/150dot resumptions keep stack/memory/resources stable.
- [x] Existing actual worker pipelines/redirects in conditions/bodies:delayed
  failure/trap/close/partial transfer, per-visit cleanup, owner shutdown/exit/
  EOF/fatal output and budget-error stderr backpressure leave no resources.
- [x] New and all10existing immutable recipes twice per-line and via dot through
  actual KEY/kernel/I/O/shell/cat/wc/LVGL; retain prior128KiB/conditional/for/
  cancel/reap and VFS immutability/display-bound tests.
- [x] npm run build:shell -- --check-c89; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields, test:shell-variables,
  test:shell-status, test:shell-predicates-native, test:shell-predicates,
  test:shell-for-native, test:shell-for, test:shell-lists-native, test:shell-lists,
  test:shell-scripts-native, test:shell-scripts, test:shell-redirection,
  test:shell-glob, test:shell-pipelines, test:shell-lvgl, test:shell-boot; npm test.
  Add focused conditional-loop gates and record exact commands/results.
- [x] Reproduce production shell.wasm and unchanged generated artifacts, update
  affected trackers/hash/size/memory evidence, audit scoped paths, full index
  and clean principal owners. Browser manual/VM/device/full POSIX stay separate.

## Historical preparation evidence — superseded by acceptance below

Read architecture, current contracts, WhileClause/UntilClause grammar, private
for/list planner, fixed frames and dot restore path. Both clauses already have
one direct condition list and one DoGroup-owned body list. Current flow validation
only permits kinds1..8 and the certified for-next; execution still rejects
conditional loops. The new phase/status/budget state is a real execution-model
change requiring specific owner approval under AGENTS4.6.

20independent generated-JS ownership cases and40fixed dash/Bash stdout/status
comparisons pass, including zero iterations, body versus final condition status,
condition side effects, incoming/selected status, lazy outer selection and exact
12body termination. Temporary parser loader removed; no new guest build.
Log:/tmp/maiaos-conditional-loop-preparation.log.

Current approved for-focused native and production WASM gates passed again:
`npm run test:shell-for-native` (25JS ownership/50native oracles,9sanitized
traces/edge/bound/canary checks) and `npm run test:shell-for` (25WASM fixtures,
150dot/200loop repeats,96actual dispatches, table/IO/dot/exit/EOF/fatal tests,
stable21pages). Logs:/tmp/maiaos-conditional-prep-native.log and
/tmp/maiaos-conditional-prep-for.log. This does not implement while/until. The production
shell.wasm remains1656737bytes/SHA256
1a7c95591845cf76c13727258a971554f951c4a612022b4b627975c1b1064961.


## Completed implementation and acceptance — 2026-10-31

Private generated-tree planning lives in shell-list.hpp; the fixed conditional
phase/count/body-status frame lives in shell-conditional-loop.hpp. Small shell.cpp
control dispatch and shell-script.hpp caller snapshots use existing command
completion, streams/VFS and kernel worker APIs. No hand parser, host evaluator,
synthetic syntax tree, new syscall or compiler-owner workaround was introduced.
The main scheduling quantum remains16 and private control budget32.

Bounded while/until (2026-10-31): this approved contract.
20 independent generated-JS ownership cases/40 dash-Bash byte/status oracles,
11 C++98 ASan/UBSan exact phase/count/status traces and20 actual WASM fixtures
pass. Tests cover0/1/12 bodies, natural13th condition versus attempted13th body,
103 actual maximum leaf dispatches, saved last-body status, lazy effects,
full-table/Unicode/field limits and isolated malformed UTF-8 condition values.
150 dot callers/200 loops retain stack/memory at shell21pages; conditions/bodies
preserve their phase/count/status across stalled/short/failed script reads/closes.
KEY/kernel/I/O/cat/wc/LVGL runs11recipes/158commands twice per-line and all11files
twice via dot. Worker resumptions,13-condition budget exhaustion, until failure/
trap recovery and existing128KiB partial-transfer/cancel/reap tests pass
(990pipeline flushes). The until-trap fixture holds its first input at WOULD_BLOCK
until the trap is armed, restores reads, then checks real failure/recovery.
All21 prior/focused acceptance gates, separate field/pipeline gates and66unit
tests pass. Guide+echo+final prompt3676bytes<4096; stable shell21/I/O2pages.
Two strict builds reproduce shell.wasm1,663,448bytes/SHA256
`57fa2c8c4a181b989f8c1af36a56c1a2fbfc77bbe258493a11bcec951d6e0a40`.
Main/word/test grammars/generated files, kernel/I/O/worker production, ABI/plans,
fonts/manifests/glob policy, the entire preexisting index and seven clean principal
owners are preserved. Root main b13f168 remains local; no commit/push/npm/version.
Manual browser acceptance of this increment, MaiaWASMVM/device and full POSIX
remain pending. The exact host/boot counters are recorded in the contract.

Exact acceptance commands (all exit0):

| Command | Result |
|---|---|
| `npm run build:shell -- --check-c89` (twice) | Own AST-strict C++98 -> strict C89 -> MaiaC/MaiaWASM, reproduced1,663,448byte production guest and unchanged generated frontends. |
| `npm run test:shell-conditional-native` | 20generated JS ownership/40dash-Bash oracles;11ASan/UBSan traces,0/12/13 bounds,103dispatches,one-op yields,invalid metadata and slot canaries. |
| `npm run test:shell-conditional` | 20actual fixtures;phase/status/budget/dot/IO/bounds/fatal cleanup,150dot/200loop repeats,stable21pages. |
| `npm run test:shell-for-native` / `npm run test:shell-for` | Existing25JS/50native/25WASM fixtures,9sanitized traces,96dispatches,150dot/200loop regressions pass. |
| `npm run test:shell-predicates-native` / `npm run test:shell-predicates` | Generated predicate and actual builtin/native/WASM/sanitized regressions pass. |
| `npm run test:shell-grammar` / `npm run test:shell-parser` | 163syntax fixtures(two recorded contextual gaps),185raw cases and27recursion profiles pass;no grammar changes. |
| `npm run test:shell-equivalence` | 6426raw JS/C++98/WASM inputs:3243accepted,2353syntax,830lexical;exact tokens/spans/trees and restored stack. |
| `npm run test:shell-word` | 710JS/native/WASM cases,327byte/mask,252fields,16length/17invalid UTF-8;native sanitizers,own-chain probe and artifact reproduction pass. |
| `npm run test:shell-fields` | Existing field/operator cycles and isolated malformed UTF-8 guest pass;selected while failure terminates normally,skipped until never decodes,and persistent until failure evaluates13conditions then returns2. |
| `npm run test:shell-variables` / `npm run test:shell-status` | Actual production variable/status/builtin/negation/cleanup regressions pass. |
| `npm run test:shell-lists-native` / `npm run test:shell-lists` | Existing flat-list/if sanitized plans,native oracles and real WASM selection/dot/cleanup pass. |
| `npm run test:shell-scripts-native` / `npm run test:shell-scripts` | Existing physical framing and own-WASM snapshot/caller/status/IO/EOF/fatal regressions pass. |
| `npm run test:shell-redirection` / `npm run test:shell-glob` | Existing actual production file/expansion/bounds/atomicity and cleanup regressions pass. |
| `npm run test:shell-pipelines` | 11recipes/158commands twice and all11dot files twice;990LVGLflushes;conditional worker failure/trap/budget/owner-exit and prior128KiB partial-transfer/cancel/reap pass. |
| `npm run test:shell-lvgl` / `npm run test:shell-boot` | See exact output below;real imported kernel/I/O/LVGL,stable memories and complete teardown. |
| `node --test tests/simulator-vfs.test.js` / `npm test` | Immutable image/4096byte display limits and all66unit tests pass. |

Measured host and boot output:

```text
Kernel/I/O/shell session passed with actual LVGL (2186 flushes): generated lexer/parser, commands, quoting, invalid/unsupported input, bounds, ls/cd/cwd/escaped names, 1024-entry ls, relative/absolute/UTF-8 cat, 1000 repeated parses, stable stack/memory (shell 21, I/O 2 pages), 77911 kernel steps, drained/closed streams.
Actual boot functions passed: failed I/O/shell startup cleanup, keyboard ownership, 25 real LVGL session launches/exits, directory/file-trap cleanup, manifest quotas, unchanged URL, zero surviving windows/endpoints (28 flushes).
```

Production shell.wasm SHA256:
`57fa2c8c4a181b989f8c1af36a56c1a2fbfc77bbe258493a11bcec951d6e0a40`.
New guide/recipe/expected text including echo and final prompt measures3676/722/
880bytes, within the unchanged4096byte display bound. The VM/device and wider
POSIX matrix are not covered by these browser-native reference-engine tests.

The first until-worker-trap setup completed its tiny successful condition before
the trap could be injected. Its fixture now holds the first input at WOULD_BLOCK
until armed,restores normal reads,then traps the actual worker. The complete
trap/failure matrix passes;no production behavior or expected failure semantics
were weakened. Earlier failed setup logs are not acceptance evidence.

Git audit:24current-increment paths,all baseline files retained,full staged index
unchanged,seven principal owners clean and all protected grammar/generated/
kernel/I/O/worker/ABI/font/manifest/dependency bytes unchanged. Root main remains
b13f168;prior uncommitted work and three staged gitlinks preserved. No staging,
commit,push,npm,version or history change. Next safe action:owner browser smoke
check of `. /system/tests/11-conditional.sh`;any broader execution contract needs
its own explicit approval.
