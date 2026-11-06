# Negation of the single loop-body if — approved contract

Date: 2026-11-01. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the locally validated loop-body-if checkpoint.
SHELL_LOOP_BODY_IF.md explicitly excludes negated inner compounds. This document
authorizes that one extension only,with explicit owner approval under
AGENTS.md section4.6. Publication and other-repository work are
excluded.

## Selected scope and generated ownership

Permit `! if ...; then ...; fi` for the one IfClause already allowed in the
DoGroup-owned body of for/while/until. Retain existing optional else/two elif,
current loop shape and all smaller quotas. Examples:

```sh
for N in a b; do ! if true; then echo "$N"; fi; done; echo $?
for N in a; do ! if true; then break; fi; done; echo $?
N=a; while test -n "$N"; do ! if true; then N=; continue; fi; done
```

No extra compound nesting: if-inside-if, loops-inside-if/loops, if in loop
conditions, multiple inner if constructs, groups/subshells, compound pipeline
stages/redirections and multiline input remain excluded. Only the exact formerly
excluded negated-inner-if shape is promoted into positive cases; retain all
other rejection tests and supported plain/root-level behavior.

Read negation only from the generated CompoundSequence-owned prefix associated
with that CompoundBodyCommand/IfClause. Reuse the existing cursor prefix/node ID;
never scan source or infer negation from a neighboring command, string spelling,
unit flags or an unrelated AST root. Keep main/word/test EBNFs and all generated
frontends unchanged. A grammar/compiler owner defect needs a minimal reproduction
and separate scope decision before owner edits.

## Ordinary completion and skipping

Without a control transfer, negate the completed aggregate if status once at its
existing join, after condition/body output drains and resources close/reap.
No-selected-body if returns0 before negation, therefore1 after negation. Selected
nonzero body status becomes0; selected zero becomes1. Keep current if selection,
AND-OR/status snapshots, error diagnostics and branch laziness. The loop sees the
completed body status under its unchanged rules.

Skipping the entire inner if by &&/|| must skip its join/inversion and preserve
the previous status. An unselected branch performs no expansion/VFS/process/
binding effects. No-body loops do not execute or invert their inner if. Ordinary
failure preserves previous effects; no implicit errexit or rollback.

## Successful break/continue across the inner region

Preserve the approved raw-success and completed-unit-negation rules. When a
successful selected break/continue originates inside the certified inner if,
its transfer bypasses the remaining if instructions and loop-body remainder.
The consumed inner-if aggregate must still be negated exactly once before
publishing the final transfer status to the loop. Do not visit its no-match zero,
repeat its last command, execute skipped branches or later run its join again.

For raw success0, the transferred status is:

| Control-unit ! | Inner-if ! | Status delivered to the loop |
|---|---|---|
| absent | absent | 0 |
| present | absent | 1 |
| absent | present | 1 |
| present | present | 0 |

An outer loop-owned ! is independent and applies once at its existing completion
join. A later ordinary body unit can replace the status only when actually
selected; a transferred control skips it. Break exits with the transferred
status. Continue records it as the completed body status and advances the
existing frozen for index or conditional next operation; final while/until
termination still returns the saved last-body result. Keep12conditional body
entries/13condition evaluations; negation cannot reset or bypass that bound.

Apply this rule to controls in the inner if's conditions and selected branches.
Controls before/after that region remain unaffected by its !. Preserve no
control across suspended dot callers, bare exit/fatal shutdown, the exact operand
profile (none or1) and all current invalid-context/argument behavior.

Dash and Bash differ on interrupted negation: among the36prepared fixtures,
12have distinct per-shell status. The selected profile is the measured Bash
parity rule above, consistent with current MaiaOS unit negation. The dash results
remain explicit differing reference observations; do not claim common POSIX
agreement or select a result accidentally by bypassing a join.

## Cleanup, certification and continuation

A raw preparation/open/close/drain failure cancels the pending control request
before negation, even when unit ! makes its completed error status0. It must not
trigger a special aggregate-unwind path. Resume the normal failed-command/list/
if path, applying the ordinary join once if reached. Preserve bounded diagnostics,
partial effects, WOULD_BLOCK/backpressure and cleanup before any next work.
Fatal/exit/shutdown clears the request and stops execution.

Extend only the existing static ShellLoopIfPlan with certified negation/prefix
identity and its validated join argument. Verify0/1flags, exact generated prefix
ownership, loop/body ancestry, region gate/join/end, EXEC-to-subtree membership
and boundary targets. Never weaken the previous root/unit-aliasing regressions
or permit new arbitrary entry/exit/backward edges. Plain regions have the exact
absent-prefix sentinel, not a stale or borrowed ! terminal.

For a successful pending request, use its saved origin PC/unit/loop identity to
determine whether it crosses this one region. Validate before mutating the PC;
never infer origin from the destination after jumping. The existing validated
request plus static region metadata is sufficient: do not add a runtime branch
stack, recursive unwinder, general evaluator or second AST. Any extra fixed
request identity field must be justified by certification, not by new nesting.
Consume/invert once after raw cleanup and unit negation, then synchronize the
published command/list/last status before normal loop continuation/outer selection.
Ordinary non-transferring completion still uses the existing join.

Dot reconstruction must recreate the same prefix/negation metadata from the
same generated caller tree. Preserve the saved unit negation separately from
aggregate if/loop negation. A script's statuses/effects do not overwrite the
caller plan identity; restore exact PC/frame without replay, lost negation or
extra inversion. Script-local loops use their own certified region; never
transfer into the suspended caller. Keep single-dot/no-nested-dot.

## Boundaries and acceptance

Expected production edits: small private cursor/region certification changes,
loop-control completion and the necessary status synchronization in shell.cpp,
plus reproducible shell.wasm. Add independent fixtures/native sanitized traces,
actual guest/kernel/I/O/LVGL cases and a versioned recipe/expected pair; update
shell/kernel/I/O trackers, architecture and handoff after acceptance.

Keep one loop/one inner if, four total if clauses, two elif per if,8static leaves/
32instructions,32main tokens/1024nodes,32control ops/16guest steps,255byte lines,
95byte words/12argv/32variables,16KiB/256line dot snapshots,12frozen for values,
12conditional bodies/13conditions,32shell pages/four handles and8I/O pages.
Measure actual21/2pages again. Guide/recipe/expected text plus echo/prompt stays
below4096bytes; each image asset stays within16KiB. Shorten data formatting if
needed; do not raise quotas, test timeouts or dispatch bounds to accommodate work.

No other repositories, gitlinks, glob policy, new syscalls/ABI/capabilities/process
plans, kernel/I/O/worker production, fonts/manifests, npm, versions, staging,
commit/push or history changes. Preserve all previous local work and full index.
Standalone native subsystem tests, browser manual acceptance, MaiaWASMVM/device
and general POSIX conformance remain separate from the scoped reference tests.

Acceptance gates — passed for the approved implementation:

- [x] Independent generated-JS owned-prefix/loop/DoGroup/IfClause cases and fixed
  per-shell output/status expectations. Cover all three negation bits, both
  control origins, for/while/until, elif/else/no-match and lazy outer/inner gates.
  Report common and differing dash/Bash observations separately.
- [x] Native C++98 ASan/UBSan exact forward/transfer traces, successful parity,
  raw-failure cancellation, origins inside/before/after the region, one-op yields,
  malformed flags/prefix/root/unit/PC/targets,32/33slot canaries and previous
  plain/control/region-aliasing exclusions. Construction stays acyclic.
- [x] Actual AST-strict C++98 -> strict C89 -> MaiaC/MaiaWASM guest: aggregate and
  unit/loop negation without double inversion, transferred status versus normal
  joins,0/12/13bounds,last-body preservation, lazy UTF-8/fields/glob/VFS effects,
  raw operand/open/close/drain failures, recovery and unchanged structural quotas.
- [x] Dot in negated inner conditions/branches, caller/script negation separation,
  read/close/short/stall failures, no replay/count reset or suspended-caller
  transfer, EOF/exit/fatal cleanup and at least150dot/200loop stable repetitions.
- [x] New and all13existing immutable recipes twice per-line and via dot through
  actual KEY/kernel/I/O/shell/cat/wc/LVGL. New worker/redirect/failure/control
  cases retain128KiB partial transfers, trap/cancel/reap, image/display bounds
  and VFS isolation. Every transfer waits for resource cleanup.
- [x] All current native/actual loop-body-if,control,conditional,for,list,script,
  predicate,grammar/parser/equivalence,word/fields/variables/status,redirection/
  glob suites,test:shell-lvgl,test:shell-boot and npm test pass with exact commands.
  If a subprocess timeout recurs,diagnose/rerun without silently raising limits.
- [x] Two final `npm run build:shell -- --check-c89` builds reproduce production
  bytes and unchanged generated frontends; update affected checklists/hash/size/
  memory/results,audit scoped paths/full index and seven clean principal owners.
  Do not infer manual/VM/device conformance or publication.

## Preparation evidence — not guest implementation

Read current architecture,preceding loop/if/control contracts,owned body cursor,
static region validation,pending request consumption and cleanup/status publication.
The current cursor rejects a negated inner compound and the region join argument
is required to be0. Simply removing either guard would lose the inversion when
control bypasses the join. The proposal defines that interrupted completion.

`node /tmp/maia-loop-if-negation-preparation.mjs` passes36generated-JS ancestry
fixtures and72fixed per-shell byte/status comparisons;12fixtures explicitly differ
between installed dash/Bash. Twelve ordinary/lazy/no-body cases have shared
expectations;24successful-control truth-table cases distinguish inner,unit and
outer loop negation,with no infinite native loops. Every subprocess has a2second
timeout;temporary parser loader removed. No production build/full regression
rerun is claimed for documentation-only preparation.

Production shell.wasm remains1,670,152bytes/SHA256
`447bdf4b392e15e4e79404e596f3087dfa540eb6606be4fd2a027770e731d945`.
Main remains b13f168 with the previous dirty tree and three staged gitlinks.
Only this proposal,the shell pending checklist/review date and handoff change.
The owner explicitly approved this contract and implementation on2026-11-01.
Implementation validation is complete below; the preparation evidence above
records the unchanged previous checkpoint, not the final artifact.

## Final local acceptance — 2026-11-01

Inner-if negation (2026-11-01): the scoped contract above.
The one loop-body if now accepts its own generated `!` prefix. Ordinary completion
inverts at its join; successful break/continue inverts the skipped aggregate once
from its certified origin after cleanup and unit negation. Raw failure cancels
transfer before negation. Static prefix/region validation keeps other nesting
excluded; no runtime if stack, grammar, syscall or kernel/I/O policy changes.
139 generated-JS fixtures / 278 per-shell comparisons (52 explicit dash/Bash
differences),46 C++98 ASan/UBSan exact traces and139 actual WASM fixtures pass.
Negated cases retain150 dot/200 loop repetitions,raw operand/open/close/drain
failure handling,0/12/13 bounds,PC/no replay,UTF-8/lazy effects and EOF/exit/fatal
cleanup. Real KEY/kernel/I/O/shell/cat/wc/LVGL executes14 immutable recipes /
195 commands twice per-line and all14 files twice via dot.20 additional dot/worker
resumptions,12 new worker-failure/control combinations and existing128KiB partial
transfer/trap/cancel/reap cases pass (1336 flushes). Host1000 parses/2249 flushes/
77911 kernel steps and25 sessions/28 flushes retain shell21/I/O2 pages.
All23 additional native/frontend/guest gates,isolated word conformance and66 unit
tests pass. Two strict builds reproduce shell.wasm1,671,325 bytes/SHA256
`b435617fc213a34afec71cc794cae68b088c67fd81bf1a451c6492a078c9a554`.
Generated frontends,kernel/I/O/worker production,ABI,fonts/manifests,glob policy,
seven clean principal owners and the entire preexisting index/pins are preserved.
Working tree only: no commit/push/npm/version changes. Browser manual acceptance,
MaiaWASMVM/device and broader POSIX/nesting/multiline remain separate.

Exact commands (all exit0):

- `npm run build:shell -- --check-c89` twice,identical hash/size above.
- `npm run test:shell-loop-if-native`:139generated fixtures/278per-shell checks,
  52explicit differences;46sanitized exact traces,including prefix/join flag and
  borrowed-terminal/root/unit/PC/target rejection,raw-failure cancellation and
  one-op yields. The116negation fixtures are independent checked-in expectations;
  Bash parity is selected only for documented interrupted-control differences.
- `npm run test:shell-loop-if`:23previous plus116new actual fixtures;negated
  150dot/200loops,failed raw operand/open/close and blocked drain,no caller-control
  leakage,script read/close/short/stall,loop budget,no replay,EOF/exit/fatal.
- `npm run test:shell-pipelines`:14recipes/195commands twice per-line plus14dot
  files twice,20newdot/worker resumptions,12newnegated-region/failure combinations,
  current128KiB transfer/trap/cancel/reap regression;1336realLVGLflushes.
- `npm run test:shell-for-native`, `npm run test:shell-conditional-native`,
  `npm run test:shell-loop-control-native`, `npm run test:shell-lists-native`,
  `npm run test:shell-predicates-native`, `npm run test:shell-scripts-native`.
- `npm run test:shell-grammar`, `npm run test:shell-parser`,
  `npm run test:shell-equivalence`:163syntaxfixtures(two known contextual gaps),
  185rawcases/27profiles,6426exact JS/native/WASMinputs respectively.
- `npm run test:shell-for`, `npm run test:shell-conditional`,
  `npm run test:shell-loop-control`, `npm run test:shell-predicates`,
  `npm run test:shell-variables`, `npm run test:shell-status`,
  `npm run test:shell-lists`, `npm run test:shell-scripts`,
  `npm run test:shell-redirection`, `npm run test:shell-glob`,
  `npm run test:shell-fields`, `npm run test:shell-lvgl`,
  `npm run test:shell-boot`, `npm test` (66unit tests).
- `npm run test:shell-word` in isolation:710wordtrees,327byte/mask expectations,
  252field vectors,16Unicode counts,17invalid-byte cases,sanitizers/guards/limits,
  actual own-toolchain WASM and six reproduced generated artifacts.
- `git diff --check`; hash inventory versus task baseline; full-index comparison;
  read-only status/HEAD audit of maiacc,maiac,maiacpp,maiawasm,maiaregex,maiajs,
  maiaglob (all clean,unchanged). No staging/commit/push/npm/version/pin changes.

The initial new failure fixture exceeded32tokens by appending a status query;
production correctly rejected before injection/effects. The corrected positive
queries on the next line and the oversized form remains an explicit negative
actual-guest test. No budget or existing expectation was relaxed.
Guide3901bytes+command echo35bytes+prompt6bytes=3942bytes<4096;recipe14=707bytes,
expected transcript766bytes;expected.json15594bytes<16384. Native standalone
kernel/I/O/font suites,manual browser,MaiaWASMVM/device and general POSIX
conformance were not run for this shell increment.
