# Bounded explicit-list for execution — approved contract

Date:2026-10-31. Status:explicitly approved by the project owner; implemented and validated locally (reference engine).
Baseline:main b13f168 plus the locally validated test/if checkpoint. The owner
reports that the test increment worked in the simulator. At baseline generated
ForClause syntax was accepted but the executor rejected it; this increment adds
execution under the bounded contract below.
Implementation within this contract is approved; publication and other-repository changes remain excluded.

## Selected language scope

Execute only an explicit-list for on one physical input line:

```sh
for N in a b; do echo "$N"; done
for F in /system/tests/data/*.txt; do cat "$F"; done
for N in; do echo NEVER; done; echo $?
! for N in a b; do test "$N" = a; done; echo $?
```

Use existing generated ForClause, Name, WordList, DoGroup and terminated-body
ownership. Require the explicit in keyword, including for an empty list. Omitted
in/positional-argument iteration, arithmetic for, while/until, break/continue
builtins, compound nesting and multiline input remain excluded. Preserve known
globally reserved words: quote a reserved word when it is data.

At most one for construct per physical line. It may occupy an outer foreground
list position with ;, &&, || and directly owned optional !. Existing nonnested
if constructs may remain siblings on that line under their existing limits;
if inside for or for inside if remains unsupported. Reject structural exclusions
in every branch before effects. A for cannot be a pipeline stage or carry
compound-level redirections. Supported simple-unit redirects/pipelines inside
its body remain available; fixed cat/final-wc worker policy is unchanged.

The body supports existing simple commands, single assignment-only units,
foreground lists, unit negation, fixed pipelines and dot under existing limits.
All static leaf descriptors across the entire line, including body and skipped
branches, still total at most8; repeated execution does not duplicate the AST or
consume more descriptors. Main tokens32/nodes1024 and flow instructions32 remain.

## Snapshot, binding and expansion semantics

Validate the entire generated line and literal/name bounds before effects.
The loop Name must be an unquoted generated identifier of at most31 ASCII bytes;
reuse the generated name checks, never a handwritten name scanner. This is a
shell-local binding with the existing32-slot table and95-byte value limit.

On selected loop entry, expand the header list exactly once using existing word
rendering, quote/empty anchors, fixed space/tab/LF splitting and MaiaGlob. All
header words capture the same incoming $? status before any iteration/binding.
Preserve source order and existing per-word glob sorting/no-match/error policy.
Permit at most12 original header words and12 final values, each at most95bytes.
An explicit empty in list or expansion producing zero fields is valid. A quoted
empty value is one iteration. There is no synthetic command name or fake AST.

Collect all final values into independent fixed12x96-byte storage before the
first binding/body effect. A header failure/overflow has ordinary status2 (or
existing operational status), leaves the loop variable unchanged and executes
no body. Already completed earlier outer units retain their effects. A skipped
loop performs no header expansion, matching/enumeration, binding or body effects.
Header expansion may perform the same bounded directory reads as ordinary glob;
these reads are not file writes and are not claimed to be transactional.

For each frozen value, assign the binding immediately before its body. Values
are bytes, never re-expanded/reparsed on assignment. The header snapshot does not
change if the body changes bindings, cwd or matching files. Body units expand
again on each selected iteration, after that iteration's binding; ordinary
lazy &&/|| and per-unit incoming status snapshots still apply.

Zero iterations do not create/change a binding and return0, even with a full
variable table. A nonempty list needing a new slot fails normally with2 before
its first body if all32slots are occupied. Updating an existing binding works
with a full table. The body may modify or unset the loop binding; do not restore
an older value on exit. Next iteration assigns its next frozen value. If body
mutations fill the table after unsetting the loop variable, a subsequent binding
failure stops the loop with2, preserving earlier completed effects.

No environment inheritance/export, automatic lexical scope or implicit binding
rollback is introduced. Header literals/expansions and body words retain their
current95-byte/12-slot limits independently.

## Status, completion and scheduling

Zero iterations return0. Otherwise return the status of the last executed body
list in the final iteration. Ordinary nonzero/usage statuses do not break the
loop; no errexit, break or continue semantics are added. First body sees the
incoming completed status; subsequent iterations see the previous completed
body status. Loop binding itself does not overwrite $?. Verify these facts with
fixed native oracles before implementing them.

Apply for-owned ! exactly once after aggregate completion/cleanup. Outer AND-OR
skipping preserves previous status and must not run zero-result/negation logic.
Body-unit ! and dot-caller ! remain independent. Publish final result to existing
$?/lists/if/dot policy. Bare exit, fatal transport error or guest shutdown must
stop execution and clear pending loop/header state without another iteration.

Advance only after pending stdout/stderr drains, owned handles close and kernel
worker groups complete/reap. Delayed failures become the unit/body result before
iteration advances. Keep existing partial transfers/WOULD_BLOCK behavior and a
single prompt after the outer line. Do not poll VFS/streams/workers synchronously.

Retain at most32 control operations per private dispatch; distinguish yield or
pending header expansion from end-of-list. Leave the current maiaos_step quantum
unchanged. At most12 iterations of at most8 static leaves means at most96 body/
outer leaf dispatches per physical line (dot body execution remains separately
bounded by its existing16KiB/256-line/no-nested-dot contract). No unbounded loop,
recursive evaluator or host scheduler/predicate semantics is introduced.

## Private plan and asynchronous expansion

Extend the current generated-AST planner with a fixed for descriptor identifying
the Name, header Word nodes and body instruction range. No substring splitting,
new handwritten parser, generated-source edits, synthetic Pipeline/Command nodes
or grammar change is expected. A demonstrated grammar/owner defect is a separate
scope decision before any related edit.

Existing gate/join instructions still govern outer selection and aggregate !.
Add private entry/next operations for the selected header snapshot and iteration.
Only the certified next operation may jump back to the same loop body entry,
after advancing/checking a monotonically bounded index. All existing non-loop
jumps stay forward. Validate targets, descriptor ownership, initialized state,
counts and capacities; arbitrary backward edges must remain rejected. Update
existing native planner tests to prove the new permitted edge and retain every
old malformed-target/capacity exclusion. Do not weaken tests to accept any jump.

Reuse bounded word/glob expansion with an explicit private header purpose and
completion path: it captures final fields instead of dispatching them as argv.
No fake argv command, VFS-policy bypass, host glob or change to pattern semantics.
Header state is independent of command/list/word trees and asynchronous worker
state. Check quotas before writes; release directory handles on every error,
shutdown and completion path. Array/index/target overflow is a normal bounded
failure, never a silently empty list or partial body.

## Dot caller restoration

A body may invoke . path under its current contract. Dot saves the caller source
and next instruction PC; it must additionally save the caller's frozen header
values, count/index, descriptor identity and loop phase. Rebuild the generated
caller tree/plan exactly as today, validate the saved frame against that plan
and restore its continuation without repeating header expansion, an earlier
iteration, earlier body commands or the dot invocation itself.

One loop frame is active for the current line and one fixed caller frame can be
suspended by the existing single dot invocation. Script physical lines may use
their own nonnested for while the caller is suspended. Nested dot stays rejected.
Do not save/rollback the shared variable table or cwd; script effects persist.
Only the frozen snapshot/iteration continuation is restored. No runtime frame
stack, second retained caller AST, second script snapshot or loop-source replay.

Error reads, malformed framing, close failures, EOF/exit and fatal output during
dot must preserve existing status/negation/cleanup and avoid duplicate body work.
Intentional next-iteration dot calls are new invocations; resumption within the
same iteration must not cause an additional script open or header glob walk.

## Boundaries and acceptance

Expected implementation scope:private shell planner/continuation and header
expansion integration, bounded loop/caller storage, focused fixtures/native and
WASM tests, existing regression adapters, one immutable simulator recipe with
exact expected output, reproduced shell.wasm and affected progress/evidence docs.
Main/word/test EBNFs and their generated artifacts, shell test semantics, all
glob matching/sorting policies, host ABI/capabilities, kernel/I/O/worker production,
fonts, manifests and all dependency pins stay unchanged.

Retain shell32page/four-handle and I/O8page budgets; baseline actual21/2pages.
Measure actual memory rather than promise an unchanged21page observation. Guide,
recipes and expected text with command echo plus final prompt must remain below
4096bytes; current guide3507bytes leaves room without changing the display bound.
No owner changes, syscalls/process plans, versions, npm, commit/push or history
rewriting. Preserve all prior root work and the entire staged index. A separate
owner issue needs a minimal reproduction and explicit approval before editing.

Acceptance gates (passed locally):

- [x] Independent JavaScript generated Name/header/body ownership expectations
  before native planning; fixed dash/Bash stdout/status oracles for empty/quoted
  empty/one/multiple values, frozen header versus changing variables/cwd/files,
  first/subsequent $? status, byte/Unicode and selected/skipped behavior.
- [x] Native C++98 ASan/UBSan exact plan/edge/iteration traces, bound/canary tests,
  one-for/no-nesting/profile exclusions,8/9 leaves,12/13 original/final items,
  31/32-byte names,95/96-byte values,32/33flow slots, no arbitrary backward edge.
- [x] Full own AST-strict C++98 -> strict C89 -> MaiaC/MaiaWASM execution; compare
  exact statuses/bytes to independent fixtures, stable stack and memory/resources
  through200 repeated loops and150 dot continuations. Table-full zero/nonzero,
  body unset/mutation/failure, header atomicity, ordinary nonzero continuation,
  aggregate/inner negation and lazy lists must have explicit expectations.
- [x] Partial/stalled header glob; selected/skipped malformed UTF8 and invalid
  pattern cases; no header re-enumeration after dot, no body work before complete
  snapshot; directory/stream/read/write/close failure and trap/shutdown recovery.
- [x] Fixed pipelines/redirects in repeated bodies; worker launch/reap, delayed
  completion/status and dot's saved caller iteration all exercise actual guests.
  Unsupported compound pipeline/redirection/nesting must reject before effects.
- [x] New and all9previous immutable recipes twice per-line and via dot through
  actual KEY/kernel/I/O/shell/cat/wc/LVGL; image/terminal budgets, handles/endpoints,
  pipeline failure/partial-transfer/cancel/reap and memory stability preserved.
- [x] npm run build:shell -- --check-c89; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields, test:shell-variables,
  test:shell-status, test:shell-predicates-native, test:shell-predicates,
  test:shell-lists-native, test:shell-lists, test:shell-scripts-native,
  test:shell-scripts, test:shell-redirection, test:shell-glob,
  test:shell-pipelines, test:shell-lvgl, test:shell-boot; npm test.
  Add explicit focused loop gates and record exact commands/results.
- [x] Reproduce generated artifacts and production shell.wasm, update affected
  shell/kernel/I/O trackers, audit scoped paths/full index/principal owner state.

Manual browser, MaiaWASMVM/device and complete POSIX remain separate gates.

## Preparation evidence — execution not implemented

Read current architecture, ForClause grammar, iterative planner, word/variable,
expansion/completion and dot paths. The existing planner only permits if and
strictly forward control flow; dot currently saves only its next PC/negation.
Therefore loop edges and snapshot restoration need the explicit contract above.

12 independent generated-JavaScript Name/WordList/DoGroup ownership expectations
and24 fixed dash/Bash stdout/status oracles pass. Examples prove explicit empty
lists, quoted empty values, byte-preserving Unicode, frozen values despite body
mutation, incoming status capture, outer skip/negation and final-body status.
Four additional dash/Bash observations confirm first/subsequent body status
and empty-list0. Log:/tmp/maiaos-for-preparation.log; scratch loader removed.
No C++98/WASM loop implementation, new guest build or full-suite rerun yet.
The validated production shell.wasm remains1645575bytes/SHA256
27aee6b260d971e3fcfb3ad0d8f881916ffc62bdc5da102bd6398044e5d70e37.

## Implemented acceptance evidence — 2026-10-31

Files:shell-list.hpp adds one generated descriptor, entry/next and certified
backward-edge validation; shell-for.hpp owns the fixed frozen snapshot/binding
adapter; shell-glob.hpp adds only a private header-purpose completion branch,
using identical MaiaGlob syntax/walk policy. shell.cpp preserves its16-step
quantum and caps control dispatch at32operations, treating header/yield separately
from done. shell-script.hpp copies one fixed caller frame with its PC/negation;
return reparses the unchanged caller and validates descriptor/count/index/PC
before restoring the snapshot, without rolling back bindings/cwd or replaying
header/body. Existing generated syntax remains authoritative. No new grammar,
AST fabrication, source scanner, host evaluator, worker or syscall is introduced.

The checked-in25fixture corpus has independent expected header ownership and
stdout/status. 50dash/Bash comparisons pass before native planning. Sanitized
native planner tests cover9 exact traces with simulated post-expansion counts,
zero/skipped/negated loops and if siblings, single-operation yields,96maximum
leaf dispatches,8/9leaf and12/13header quotas, malformed backward targets/entry
metadata and32-slot exhaustion with a canary. Native word gates retain31/32name
and95/96value checks; actual for header tests separately exercise those bounds.

Actual shell WASM passes25 exact byte/status fixtures,12literal/expanded values,
13header rejection,96real body dispatches, quoted empty/zero lists, frozen
parameters/glob, mutable/unset loop binding, table-full first/later binding
failures, ordinary nonzero continuation, outer/inner negation and skipped effects.
150caller loops invoke300dot scripts containing their own loops;200further
loops retain stable memory/stack. A separate glob/dot test counts only the target
directory open (literal prefix components use their own existing walker opens):
it occurs once across caller restoration, and append output proves no replay.

Additional focused cases cover stalled/short/error dot reads, script close error,
nested-dot rejection, header open/read resource and I/O errors, directory close,
WOULD_BLOCK, delayed body close with a later successful iteration, bare exit,
EOF without LF and a fatal peer closure mid-loop without a second body write.
Existing MaiaGlob maps -4 open/read resource failures to normal status2/limit
errors and other I/O failures to1; close failure remains1. These policies are
preserved rather than reclassified for loops. Selected malformed UTF8 headers
fail through existing glob validation; the isolated own-chain seed guest proves
skipped invalid headers/body expansions do not decode or open redirect targets.

Actual KEY/kernel/I/O/LVGL:10immutable recipes/146commands twice per-line and
all10files twice via dot (recipe03 retains its intentional physical-line4 abort).
20loop/dot/worker caller resumes launch real cat/wc workers;6loop pipeline failure
modes verify cleanup before the next iteration. Added loop worker-trap and owner-
exit cases retain the old128KiB/conditional trap/partial-transfer/cancel/reap gates.
769pipelineflushes; host1000parses/77911steps/2165flushes; boot25sessions/28flushes,
zero surviving windows/endpoints. Shell21/I/O2pages stay within32/8page budgets.

| Exact command | Result |
|---|---|
| npm run build:shell -- --check-c89 | PASS twice:AST-strict C++98 -> strict C89 -> own MaiaC/MaiaWASM; complete compiler AST/validation, identical1656737byte production artifacts. |
| npm run test:shell-for-native | PASS:25generated JS ownership/50native stdout-status oracles,9C++98ASan/UBSan finite traces, one-op yielding,96dispatches, bounds/targets/exclusions and32slot canary. |
| npm run test:shell-for | PASS:25actual fixtures, frozen fields/glob,150dot/200loop repeats,96actual dispatches, bounds/full table/atomicity/status/lazy branches, header and dot I/O failures, exit/EOF/fatal cleanup and21pages. |
| npm run test:shell-predicates-native | PASS:225JS/native/own strictC89/WASM token/span/tree/status cases,330native oracles, invalidUTF8/bounds/tree guard/recovery,300calls and six reproduced artifacts. |
| npm run test:shell-predicates | PASS:219production cases plus if/lists/status/fields/glob/dot/redirect timing, no-VFS/skipped effects, delayed close/stderr and150repeats. |
| npm run test:shell-grammar | PASS:163native fixtures including2known contextual gaps. |
| npm run test:shell-parser | PASS:185raw cases/27recursion profiles, no cycles. |
| npm run test:shell-equivalence | PASS:6426raw JS/native/own-WASM inputs;3243accepted/2353syntax/830lexical errors, exact tokens/UTF16/complete trees and stack restoration. |
| npm run test:shell-word | PASS:710trees/327byte-mask/252field cases,16Unicode counts/17invalid vectors, aggregate matcher budgets, sanitizers/guards, own strictC89/WASM and six reproduced artifacts. |
| npm run test:shell-fields | PASS:all current field/parameter/removal/length/quote/native-oracle/bound/lazy/dot suites;400field and100/200other cycles, isolated malformed stored values including selected/skipped for headers. |
| npm run test:shell-variables | PASS:generated ownership/native byte comparisons, limits/atomicity, dot/isolation, close/EOF and400cycles. |
| npm run test:shell-status | PASS:existing snapshots/quoting/glob/names/paths, static/runtime bounds, dot/status/negation/cleanup and150/200repeats. |
| npm run test:shell-lists-native | PASS:31flat plans/25conditional traces/50native fixed outputs, ASan/UBSan, target/capacity and exclusion checks. |
| npm run test:shell-lists | PASS:all existing flat/conditional selection, native comparisons, bounds/skipped effects, dot PC/no replay, errors/cleanup and150repeats. |
| npm run test:shell-scripts-native | PASS:19physical framing cases with ASan/UBSan. |
| npm run test:shell-scripts | PASS:bounded snapshots/generated execution/caller return, partial/stalled/error reads, quotas/atomicity,150repeats and EOF/exit/fatal cleanup. |
| npm run test:shell-redirection | PASS:current binary/printf/native-oracle, stream/truncate/append/limits/failure timing and cleanup suite. |
| npm run test:shell-glob | PASS:existing sorting/quoting/Unicode/limits/atomicity/cooperative traversal and strict invalid-bracket policy. |
| npm run test:shell-pipelines | PASS:10recipes/146commands twice per-line and via dot,20loop worker resumes,6loop failure modes,loop traps/owner cleanup,all prior128KiB/conditional/cancel/reap tests;769flushes. |
| npm run test:shell-lvgl | PASS:real LVGL2165flushes/1000parses/77911kernel steps, shell21/I/O2pages and drained/closed streams. |
| npm run test:shell-boot | PASS:25real LVGL launches/exits,startup/trap/quota cleanup,28flushes and zero surviving windows/endpoints. |
| npm test | PASS:66unit tests, immutable VFS/read-only paths and full display bounds including final prompt. |

Recipe10 has12commands. Guide/script/expected with echo+prompt are respectively
3597/644/733bytes, all below4096. Production shell.wasm SHA256:
1a7c95591845cf76c13727258a971554f951c4a612022b4b627975c1b1064961.

Main b13f168 remains local. Preserve prior staged Cpp8475812/Glob703dda2/
Regexe2ae3a8 and the entire index. Seven principal dependencies remain clean at
prior SHAs; main/word/test grammars/artifacts, all kernel/I/O/worker production,
ABI/plans/manifests/fonts and glob library/policy are unchanged. No publication,
versions, npm or history action. Manual10-for browser acceptance, VM/device,
standalone native kernel/I/O/font and full POSIX were not run.
Next safe action:reload simulator and run . /system/tests/10-for.sh.

Final audit:25scoped root paths changed from /tmp/maiaos-for-before.json; all
other baseline bytes and the full index are preserved. Protected generated
frontends/grammars/kernel/I/O/workers/ABI/manifests and seven clean principal
repos are unchanged. git diff --check passes; no build/test process remains.
