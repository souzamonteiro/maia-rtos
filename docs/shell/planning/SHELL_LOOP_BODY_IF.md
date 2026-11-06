# One bounded if inside a loop body — approved contract

Date: 2026-10-31. Status: explicitly approved; implemented and validated locally (reference engine).
Baseline: main b13f168 plus the locally validated loop-control checkpoint.
The owner explicitly approved this bounded execution-model increment. Earlier
compound-nesting exclusions are superseded only in the selected scope below.
Publication and other-repository work remain excluded.

## Selected nesting scope

Permit at most one nonnegated IfClause inside the DoGroup-owned body of the one
for/while/until on a physical line. The inner if supports existing optional else
and at most two elif clauses, with ordinary condition/body lists and current
supported simple units. Examples:

```sh
for N in a b c; do if test "$N" = b; then break; fi; echo "$N"; done
for N in a b c; do if test "$N" = b; then continue; fi; echo "$N"; done
N=aa; while test -n "$N"; do if true; then echo "$N"; N=${N#a}; fi; done
```

Use actual generated ancestry: the selected IfClause must be under that loop's
DoGroup body, with no intervening IfClause/loop. The maximum permitted compound
ancestry is precisely loop -> if. Keep one loop of any kind per physical line,
at most one if under its body and the current four-if total across the line
(including any existing nonnested outer siblings), subject to all smaller quotas.

Continue rejecting if inside if, loops inside if, loops inside loops, if inside
a while/until condition, multiple inner if constructs, groups/subshells, background
jobs and multiline compounds. Reject compound-level redirects or pipeline stages
at either nesting level. A negated nested compound (`! if ...`) is excluded in
this first step, including in skipped branches; existing negated outer compounds
remain supported. Ordinary unit `!` inside the inner lists remains supported.
No other previously unsupported compound ancestry becomes valid.

Every generated clause is structurally validated before any line effects,
including skipped branches. Reclassify exactly the newly permitted old rejection
fixtures into independently specified positive cases; retain their original
syntax coverage and all other exclusions, with explicit deeper/multiple/negated
inner-compound negative cases. Do not remove tests or accept general nesting.

## Status, selection and loop control

Retain existing if semantics: evaluate conditions in order, select one body,
return its last completed status, or0 if no body is selected and there is no
else. Existing ;/&&/|| gates apply both around the inner if and within each list.
Unselected branches perform no expansion, VFS, binding or process effects; the
whole-tree syntax/literal/quota preflight still applies. Reevaluate the selected
if condition and words on each loop iteration using current bindings and cwd.

The loop's ordinary body status includes the inner if's completed result and any
later selected body units. Keep for's frozen header/index and while/until's
separate last-body status,12-body/13-condition bound and terminating-condition
effects. No rollback, implicit errexit/break or new resource counter semantics.
A valid break/continue in a selected inner-if condition or branch remains in the
current loop body. Its successful certified transfer skips the remaining inner
if, all later branches and the loop-body remainder; it must not execute a
no-match zero operation or unfinished inner join after transferring. Break exits
the loop; continue reaches its existing next operation without resetting counts.
Failed raw preparation/open/close/drain cancels transfer under the current
loop-control contract, before ordinary unit negation. Outer loop-owned negation
still runs once; unit-owned negation keeps its approved profile.

Enclosing inner-if negation is excluded because it needs an explicit aggregate
unwind rule. Preparation observes that `for N in a b; do ! if true; then break;
fi; done` and an equivalent continue example differ between installed dash
(status0) and Bash(status1). Do not silently choose behavior, accidentally skip
negation, or add a new runtime stack in this step. A later contract can address it.

## Planning and continuation architecture

Keep all lexing/parsing in unchanged MaiaCC-generated main/word/test frontends.
Build private instructions only over actual generated subtree ranges, direct
owned terminals, condition/body ancestry and certified stage identities. Do not
split/reparse source, create substitute nodes, recursively evaluate commands or
introduce a second lexical/grammar interpretation in a collector.

Use bounded nonrecursive plan construction: shared flat-range emission for
existing simple spans, the existing flat if emission for its nonnested blocks,
and a loop-body walk that delegates its single IfClause to those flat helpers.
Keep the construction call graph acyclic; do not make the existing body and if
builders recursively call each other. This may factor the private collector,
but must not change the meaning of the existing flat plans. No general-purpose
compound compiler or broad parser/executor rewrite is authorized.

The completed inner if is a fixed region of forward branch/join instructions
inside the existing loop body. Only the existing certified loop-next edge may
repeat it. Reuse eight total static leaf descriptors and32 private instructions
for the entire line, counting both selected and skipped branches; test overflow
before writing any array and reject before any effects. Keep the current
conservative104 leaf-dispatch bound for conditional loops (96 for for loops),32control
operations per private dispatch and16 guest steps. No recursive runtime evaluator,
new branch/loop stack, synchronous resource polling or expanded quantum.

Dot callers already save their next private PC and one active loop frame. A dot
in the inner condition/branch must reconstruct the same generated plan, validate
that saved PC and frame, then resume without rerunning conditions, bindings,
header expansion or earlier branch units. No runtime if frame or second AST is
needed. The script may execute its independently permitted plan while its caller
is suspended. Keep single-dot/no-nested-dot and no control transfer across the
suspended caller boundary; do not widen scope via script reconstruction.

Wait for output draining, handle closure and worker completion/reaping before
branch selection, loop continuation or break/continue transfer. Preserve ordinary
error status, partial transfers, WOULD_BLOCK, cancellation, cleanup and fatal/
exit/EOF behavior. Emit one prompt only after the outer line completes.

## Scope boundaries and acceptance

Expected production edits: bounded private list-plan construction/validation and
only necessary continuation or loop-control certification hooks, plus reproduced
shell.wasm. Add focused independent fixtures, native sanitized plans/traces,
actual own-chain WASM tests, real integration and a versioned VFS recipe/expected
pair; update affected shell/kernel/I/O checklists and architecture/handoff docs.

Preserve32main tokens/1024nodes,255byte physical lines,95byte words,12argv,
32variables,16KiB/256line dot snapshots,12frozen for values,12conditional bodies/
13condition evaluations,32shell pages/four handles and8I/O pages. Measure current
21/2actual pages again. All guide/recipe/expected text plus echo and prompt must
fit4096bytes; every image asset remains within16KiB. Shorten formatting/text as
needed; never raise production or test budgets to fit new examples.

No EBNF/generator changes are expected. A demonstrated grammar/compiler owner
defect requires a separate minimal reproduction and explicit approval. No other
repositories, gitlinks, glob matching changes, new syscalls/ABI/capabilities/
process plans, kernel/I/O/worker production, fonts/manifests, versions, npm,
commit/push, staging or history changes. Preserve all previous work and the
entire staged index.

Acceptance gates — completed locally:

- [x] Independent generated-JS loop/DoGroup/IfClause ownership and fixed native
  stdout/status expectations before native planning; finite dash/Bash common
  cases, skipped/selected branches, elif/else, per-iteration reevaluation,
  last-body status, unit/outer negation and body control. Record divergent
  excluded forms separately, never as common POSIX agreement.
- [x] Native C++98 ASan/UBSan exact flat/nested-region instruction/status traces,
  bounded yields, branch targets within the right region, current loop-next
  certification, malformed descriptors/PC/edges,8/9leaves and32/33slots/canaries.
  All prior flat if/list/for/conditional/control framing invariants remain covered.
- [x] Actual AST-strict C++98 -> strict C89 -> MaiaC/MaiaWASM: for/while/until with
  selected/unselected/no-body inner if, both elif slots, AND-OR/status, successful
  and failed break/continue from its conditions/branches,12/13budget boundaries,
  lazy UTF-8/fields/glob/VFS effects, literal/runtime limits and recovery.
- [x] Correct whole-line rejection of every excluded nesting, inner negation,
  compound redirect/stage, branch limit and array/token/leaf quota before effects,
  even in skipped regions. Existing root-level forms retain their exact behavior.
- [x] Dot inside inner conditions and branches, caller/script frame independence,
  no replay or budget reset, stalled/short/read/close/framing failures, EOF/exit/
  fatal cleanup, at least150dot/200loop repetitions with stable stack/resources.
- [x] New and all12existing immutable recipes twice per-line and via dot through
  actual KEY/kernel/I/O/shell/cat/wc/LVGL. New conditional/control worker failures,
  redirects/backpressure/cleanup plus existing128KiB transfers/trap/cancel/reap;
  preserved image/display bounds and private VFS/process policy.
- [x] Existing grammar/parser/equivalence, word/fields/variables/status,
  predicate/list/script/for/conditional/control native and actual guest suites,
  redirection/glob, test:shell-lvgl, test:shell-boot and npm test pass. Record
  exact commands/results; broader interactive browser/VM/device remain separate.
- [x] Two `npm run build:shell -- --check-c89` runs reproduce the production
  artifact and unchanged generated frontends; update affected checklists,
  artifact hash/size/memory evidence and audit scoped paths/full index/seven
  clean principal owners. No publication or broad POSIX-conformance claim.

## Historical preparation — superseded by acceptance below

Read generated CompoundSequence/CompoundBodyCommand/IfClause/DoGroup ownership,
current flat-body/if/loop planner, structural rejection, loop-control completion,
dot reconstruction, architecture and preceding approved contracts. Existing
preflight rejects every IfClause with a loop ancestor; the current flat-body
walker cannot simply be allowed to consume nested branches. This requires
bounded plan construction, not just removing the guard.

`node /tmp/maia-loop-if-preparation.mjs` passes20 generated-JS ancestry cases and
40fixed dash/Bash stdout/status comparisons. These cover plain and elif/else
selection, zero branch, conditional break/continue, control inside inner if
conditions, status, skipped outer/inner gates and while/until reevaluation.
Every run is finite and has a2second timeout; temporary parser loader removed.
The negated-inner-compound divergences above are exploratory exclusions, not
common test agreement or current guest execution. No production build or broad
regression rerun was needed for this documentation-only preparation.

Production shell.wasm remains1,666,203bytes/SHA256
`70dff66b805db5a8f5e19e4c8f9ccb3832938515da4b05987c631a608b8f15f9`.
Main remains b13f168 with prior dirty files and three staged dependency pointers.
Only this proposal, the shell pending checklist and handoff change. Next action:
explicit approval of this concrete scope before implementation.


## Completed implementation and acceptance — 2026-10-31

Loop-body if (2026-10-31): this approved contract.
One nonnegated if inside a for/while/until body executes through the generated
DoGroup/IfClause tree,shared flat-span cursor and existing forward if instructions.
A static descriptor validates loop ancestry,region entry/join and exact EXEC/AST
ownership. No recursive construction or runtime branch frame is introduced.
23generated-JS ownership/46dash-Bash byte-status comparisons,20C++98ASan/UBSan
traces and23actual WASM fixtures pass. Tests cover96/102dispatches,0/12/13bounds,
region/root/unit corruption,8leaves/32/33slots,conditional control,raw-close
cancellation,blocked drain and dot PC/no replay/budget preservation.150dot callers/
200loops keep stack/memory stable at shell21pages. Existing loop frames/control
and generated frontends remain unchanged;only one normal-completion reset is
added in shell.cpp. Two newly supported old negative cases become positives;
deeper nesting remains explicitly rejected.
Real KEY/kernel/I/O/shell/cat/wc/LVGL:13recipes/184commands twice per-line and all13
files twice via dot;20new branch/dot/worker resumptions,12new inner-if worker-
failure combinations and existing128KiB transfer/trap/cancel/reap tests pass
(1172flushes).19native/guest regression gates,three frontend gates,the isolated
word rerun,fields guest and66unit tests pass. The first word run exceeded its
unchanged10second native subprocess timeout;the identical test passes in isolation,
without changed expectations or timeout. Guide+echo+prompt3860bytes<4096;
shell21/I/O2pages retain32/8page budgets. Two final strict builds reproduce
shell.wasm1,670,152bytes/SHA256`447bdf4b392e15e4e79404e596f3087dfa540eb6606be4fd2a027770e731d945`.
Main/word/test grammars/generated files,kernel/I/O/worker production,ABI/plans,
fonts/manifests/glob policy,the entire staged index/pins and seven clean principal
owners are preserved. Root main b13f168 remains local;no commit/push/npm/versions.
Negated inner if,other compound nesting,multiline and MaiaWASMVM/device/manual
browser acceptance remain separate. See the contract for exact measured output.

Exact commands (final acceptance exits0):

| Command | Result |
|---|---|
| `npm run build:shell -- --check-c89` (twice on final source) | Own AST-strict C++98 -> strict C89 -> MaiaC/MaiaWASM; identical production bytes. |
| `npm run test:shell-loop-if-native` / `npm run test:shell-loop-if` | 23generated/actual fixtures,46dash-Bash comparisons,20sanitized plan traces,96/102dispatches,region/AST identity and boundary/IO/dot/cleanup tests pass. |
| `npm run test:shell-grammar` | 163syntax fixtures,including2recorded contextual gaps;no broader POSIX claim. |
| `npm run test:shell-parser` | 185raw cases and27recursion profiles pass. |
| `npm run test:shell-equivalence` | 6426raw JS/native C++98/WASM inputs:3243accepted,2353syntax,830lexical;exact tokens/spans/trees and restored stack. |
| `npm run test:shell-word` (isolated rerun) | Unmodified710word trees,327byte/mask,252fields,16Unicode/17invalid vectors,sanitizers,own-chain probe and reproduced artifacts pass;initial native subprocess timeout recorded separately. |
| `npm run test:shell-for-native` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-conditional-native` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-loop-control-native` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-lists-native` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-predicates-native` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-scripts-native` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-for` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-conditional` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-loop-control` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-predicates` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-variables` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-status` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-lists` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-scripts` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-redirection` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-glob` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-lvgl` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-boot` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test` | Pass;existing scoped native/actual guest regression coverage retained. |
| `npm run test:shell-fields` | Existing field/parameter/Unicode and isolated malformed-value guest tests pass;no test exports in production. |
| `npm run test:shell-pipelines` | 13recipes/184commands twice and all13dot files twice;1172flushes,new branch/control failures and prior128KiB/trap/cancel/reap coverage. |
| `node --test tests/simulator-vfs.test.js` | Both tests pass;byte-exact immutable image,display/image quotas and preserved user files. |

Measured host/boot output:

```text
Kernel/I/O/shell session passed with actual LVGL (2786 flushes): generated lexer/parser, commands, quoting, invalid/unsupported input, bounds, ls/cd/cwd/escaped names, 1024-entry ls, relative/absolute/UTF-8 cat, 1000 repeated parses, stable stack/memory (shell 21, I/O 2 pages), 77911 kernel steps, drained/closed streams.
Actual boot functions passed: failed I/O/shell startup cleanup, keyboard ownership, 25 real LVGL session launches/exits, directory/file-trap cleanup, manifest quotas, unchanged URL, zero surviving windows/endpoints (41 flushes).
```

New guide/recipe/expected text including echo and next prompt:3860/929/1095bytes,
all below4096. expected.json is14655bytes<16384;pair-per-line formatting preserves
all old expectations and makes each command/output pair reviewable on GitHub.
The new recipe is `/system/tests/13-loop-if.sh`.

A negative native fixture exposed insufficient numeric-only region validation:
changing its AST root to a sibling if was accepted. The added ancestry and exact
EXEC/subtree membership checks reject unrelated roots and units;native negative
log exits30 before the fix and final20traces pass. The1669470byte functional build
predating this certification is not a final reproduction artifact. Both final
builds measure1670152bytes with SHA256
`447bdf4b392e15e4e79404e596f3087dfa540eb6606be4fd2a027770e731d945`.
A three-elif loop fixture reaches the unchanged32token quota before semantic
branch validation;its production expectation checks that input limit. Existing
native flat-if tests retain the explicit third-elif rejection. No limits raised.
The first word run returned ETIMEDOUT at the existing10000ms subprocess bound;
the identical test passed in isolation with no timeout or expectation edits.
No cause is inferred solely from that one timeout;the initial log is retained.

Git audit:25increment paths,all prior files/index/pins preserved,seven principal
owners clean and protected grammar/generated/kernel/I/O/worker/ABI/font/manifest/
dependency bytes unchanged. Main b13f168 plus local work;no staging,commit,push,
npm,version or history change. Manual browser smoke check of the new recipe,
MaiaWASMVM/device and broader POSIX conformance remain pending.
