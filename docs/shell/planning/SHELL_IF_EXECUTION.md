# Bounded if execution — approved contract

Date: 2026-10-31. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the locally validated prefix-removal checkpoint.
The generated grammar already parses if; the current executor rejects all
CompoundCommand nodes. Existing list contracts exclude compound execution.
This proposal authorizes neither publication nor changes in other repositories.

## Selected language scope

Execute `if condition; then body; elif condition; then body; else body; fi`
using the existing generated IfClause and TerminatedCompoundList nodes. Else and
elif are optional. Permit at most two elif clauses per if, four if constructs per
physical line and no nested compound commands. Each whole if must fit one current
physical input line, including when read by dot; multiline input is deferred.

An if may stand alone or occupy an existing outer foreground list position with
`;`, `&&`, `||` and a directly owned optional `!`. It cannot be a stage of a pipe
and cannot carry compound-level redirections. Existing simple-command/pipeline
redirects inside conditions and bodies remain supported. Ordinary string words
such as quoted 'if' remain words. Preserve known globally reserved-word limits.

Condition/body lists may use existing simple commands, single assignment-only
units, lists, negation, supported cat/final-wc pipelines, redirects and dot calls
within their current contracts. The total of all leaf pipeline units across the
entire physical line, including skipped branches and ordinary outer units, stays
at most eight. Each condition/body must be nonempty according to the grammar.
Loops, groups, subshells, nested if, background jobs, generic launch and richer
assignments remain excluded, even in a branch that would be skipped.

Examples within scope:

```sh
if true; then echo yes; fi
if false; then echo skipped; else echo selected; fi
if false; then :; elif true; then echo selected; fi
if cat /system/welcome.txt | wc -l; then echo counted; fi
if false; then :; fi; echo $?
! if true; then false; fi && echo recovered
```

## Status, selection and effects

Evaluate condition lists in source order. A completed zero status selects that
body and skips all later conditions/bodies; nonzero selects the next elif
condition or the else body. A selected body completes with its last executed
unit's status. If no condition succeeds and no else exists, the if result is0.
Apply an outer if-owned ! exactly once to that aggregate result, after all unit
cleanup. Skipping an entire if through outer AND-OR preserves previous status;
it must not execute the zero-result or inversion instructions.

Existing &&/|| left-to-right behavior applies within each condition/body.
Selected simple units retain their own incoming $? snapshot. The first body
unit sees the completed condition's status; else sees the last failed condition.
A selected runtime expansion/usage/operational error is an ordinary nonzero
condition/body status, with existing diagnostics and cleanup; do not turn it into
a structural parse error or add special-builtin fatal-error semantics.

Validate the complete generated line's supported structure, names, literal bounds
and quotas before effects, including unselected branches. At runtime, only selected
units may read bindings, render parameters, split/glob, open/mutate files, change
cwd or launch workers. Preserve partial effects from already completed units if
a later selected unit fails; do not claim whole-line transactional execution.

Advance or select a branch only after pending output drains, owned handles close
and kernel worker groups complete/reap. Preserve WOULD_BLOCK and delayed-close
status overrides. Bare exit and fatal guest/transport failure stop the shell;
negation and branch selection must not resume it. Emit one prompt after the full
outer list, without branch/condition or dot-body prompts.

## Bounded private execution plan

Keep all syntax/lexing/parsing in existing MaiaCC-generated frontends. Build an
iterative private control-flow plan over generated ancestry and direct terminal
ownership, without splitting source, substring reparsing, a handwritten parser,
recursive execution or synthetic/rewritten AST nodes. No EBNF change is expected.
If a genuine grammar defect appears, isolate it before changing this scope.

Retain at most8 leaf-unit descriptors for existing static word validation. Add
at most32 fixed private instructions for execute, conditional gate, forward jump,
no-selected-body zero result and aggregate if completion/negation. All jump
targets are validated instruction boundaries and strictly forward. No loops or
runtime branch stack are required. Dispatch at most32 control instructions per
scheduler step, returning to existing asynchronous command execution for work;
never poll streams/workers synchronously. Check capacities before writing arrays.

The current CompoundSequence/CompoundBodyCommand grammar does not wrap each inner
pipeline in a Pipeline node. Certify exact SimpleCommand subtree IDs, pipeline
stage ownership, owned pipe/negation/separator terminals and selected ranges from
that generated structure. Adapt private word collection to these certified
ranges; do not forge Pipeline nodes, skip syntax by token spelling alone or
collect words from a following condition/body. Preserve ordinary Pipeline paths,
assignment-only restrictions, stage IDs and redirect source order. This is AST
execution planning, not another grammar or tokenization layer.

Outer AND-OR gating skips the complete if region. Condition tests jump to a
selected body or next condition; body completion jumps to the join. The no-match
path sets0 before that join. If negation is represented at the join, never also
apply it to the last inner command. Existing individual ! flags stay independent.
No saved runtime branch stack or mutable alternate parse tree is introduced.

Dot may occur in a condition/body subject to existing no-nesting/no-redirect
rules. Save the caller's next instruction PC and individual dot-unit negation.
When the generated frontend reconstructs the immutable caller, rebuild exactly
the same plan, validate the saved PC, restore it and complete the dot unit once.
Do not rerun the condition, earlier assignments, previous branch units or opens.
Aggregate if negation remains in immutable join metadata. A dot-body script may
execute a complete one-line if; nested dot remains unsupported. Source/framing
errors return through current dot status/cleanup policy. Exit in a script still
exits the shell without caller continuation.

## Boundaries and affected files

MaiaOS only: shell-list.hpp or a small private flow helper, shell.cpp word/planner/
completion integration, shell-script.hpp continuation, focused JS/native/WASM
planner/list/status/script/word/pipeline regressions, fixed VFS recipes and exact
expected bytes, reproducible shell.wasm and affected progress/evidence docs.

Keep input255bytes,32tokens,1024main-tree nodes,95-byte words/values/patterns,
12original/final argument slots including redirects,32variables, eight leaf units,
2–4 fixed workers, script16KiB/256physical lines and existing word-frontend limits.
The32-instruction array is new private bounded storage, not a new syscall/ABI or
permission to enlarge other budgets. Measure actual pages against the unchanged
shell32-page/four-handle and I/O8-page budgets; current prefix baseline is21/2.
Guide/recipe/expected output including command echo must fit unchanged4096display;
current guide+echo4083bytes needs condensation before adding the next recipe.

No compiler/library owner edit, dependency-pointer update, kernel/I/O/worker
production change, host command interpretation, public syscall/capability/process
plan change, version, npm, commit/push or history rewrite. Preserve all existing
local work and staged Cpp/Glob/Regex pins. Any owner defect requires a minimal
reproducer and separate authorization before editing its principal repository.

## Required acceptance — pending approval and implementation

- [x] Independent expected generated JS trees first: if/elif/else ownership,
  condition/body groups, inner pipeline chains, assignment/negation/redirect
  provenance and quoted keywords. Raw native C++98 /own-chain WASM parity.
- [x] C++98 ASan/UBSan planner: all existing flat-list plans unchanged; exact
  eight/nine leaf-unit, if/elif and instruction caps; forward target/range checks;
  no cross-branch words, out-of-bounds writes, recursion or partial supported
  prefix on static rejection. Preserve original unrelated negative expectations.
- [x] Explicit guest outputs/status/side effects plus dash/Bash oracles: successful,
  failed/no-else, elif/else, aggregate and inner !, outer/inner AND-OR, $? snapshots,
  assignments, variable/quote/Unicode/default/pattern expansions and empty argv.
- [x] Skipped expansion/decode/glob/open/launch spies; static invalid branches
  reject before effects; ordinary condition errors choose later branches after
  cleanup. Keep invalid UTF8, limits and exact pattern-budget regressions.
- [x] Dot in condition/body and one-line if in a VFS script; empty/failing/delayed
  dot, parse/framing error return, no condition/prefix replay and correct next
  branch/aggregate negation/status. Exit/EOF/fatal errors preserve lifecycle.
- [x] Actual kernel/I/O/cat/wc/LVGL: pipeline conditions/bodies and redirects,
  delayed partial transfers/closes, kernel rollback/trap/cancel/reap, one prompt,
  repeated conditionals/scripts and stable stack/memory/resources; new immutable
  VFS recipe twice per-line and via dot, existing seven recipes preserved.
- [x] npm run build:shell -- --check-c89; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields, test:shell-variables,
  test:shell-status, test:shell-lists-native, test:shell-lists,
  test:shell-scripts-native, test:shell-scripts, test:shell-redirection,
  test:shell-glob, test:shell-pipelines, test:shell-lvgl, test:shell-boot; npm test.
- [x] Reproduce unchanged generated sources and rebuilt shell.wasm; update affected
  shell/kernel/I/O checklists with actual results and remaining limits, then audit
  scope, protected files, owner state, complete index and binary hash/size.

Interactive browser, MaiaWASMVM and device acceptance remain separate from
reference-engine/real-LVGL automation. This contract does not claim full POSIX
compound execution, multiline scripts or complete reserved-word classification.

## Read-only preparation evidence — 2026-10-31

Read project instructions, architecture, generated grammar/collectors, flat planner,
word collection, asynchronous completion and dot reconstruction. Ten generated JS
ownership checks pass, including seven directly owned condition/body lists in a
two-elif/else example (23tokens), nested/pipe-if examples retained as syntax only,
and quoted keywords remaining ordinary words. Inner pipeline chains use the
factored body grammar, confirming the private range adapter requirement above.
32 fixed native outcomes pass (16dash/16Bash), including no-selected-branch zero,
body/condition statuses, AND-OR/negation, pipeline and dot return without replay.
These are source/oracle checks, not implemented MaiaOS conditional execution.

Preparation changes only this proposal, one unchecked shell checklist row and
handoff. Production shell.wasm remains1587368bytes/SHA256
60c27e3cd2d4f6447cbaba16a536f65272acca316223b224332eec969f881376.
No source implementation, new guest build, full-suite rerun or publication.

Native baseline `npm run test:shell-lists-native` exited0:31 generated-tree
plans pass C++98 ASan/UBSan, including current rejection of if execution and
existing subtree/selection/eight-unit bounds. Source grammar/production binary
were not rebuilt. Final preparation audit: exactly3 documentation paths change;
all production files and entire index, including prior staged pins, are unchanged.
`git diff --check` passes. No commit/push or release action.

## Implemented and verified — 2026-10-31

The approved profile executes locally in shell.wasm. shell-list.hpp retains8
certified leaf descriptors and adds32 fixed private forward instructions. Planning
uses generated IfClause/TerminatedCompoundList/CompoundBodyCommand ancestry,
SimpleCommand stage IDs and directly owned operator terminals. It never scans
source, fabricates Pipeline nodes or recursively executes a branch. All jump
boundaries/capacities and excluded structures are checked before rendering/effects.

shell.cpp collects words/redirects only from each certified range, executes a
selected unit through existing expansion/VFS/stream/kernel adapters, and advances
control after normal completion and cleanup. Failed conditions select elif/else;
no selected body yields0. Aggregate if negation is applied once at its join,
independently of inner pipeline/dot negation. gLastStatus reflects that result.
shell-script.hpp saves/restores the next instruction PC across generated caller
reconstruction, preserving branch position without replay or a second tree/stack.

All final commands exited0:

```sh
npm run build:shell -- --check-c89
npm run test:shell-grammar
npm run test:shell-parser
npm run test:shell-equivalence
npm run test:shell-word
npm run test:shell-fields
npm run test:shell-variables
npm run test:shell-status
npm run test:shell-lists-native
npm run test:shell-lists
npm run test:shell-scripts-native
npm run test:shell-scripts
npm run test:shell-redirection
npm run test:shell-glob
npm run test:shell-pipelines
npm run test:shell-lvgl
npm run test:shell-boot
npm test
```

- Native planner:31 existing list plans retain prior expectations except the
  explicitly newly supported simple if case, now positive.25 independent JS
  condition/body ownership fixtures and50 fixed dash/Bash output/status oracles
  precede extended C++98 ASan/UBSan traces.25 exact traces/statuses, body pipeline
  stages/quoted operators, eight/nine units, four ifs/two elifs, target validation,
  instruction exhaustion and unsupported nested/stage/redirect cases pass.
- Actual guest:25 fixed outputs/statuses plus48 native AND-OR combinations;
  then/elif/else/no-else, inner/aggregate !, outer lazy gates, condition errors,
  status snapshots, assignments/Unicode/fields and quoted syntax pass. Static
  invalid branches reject before earlier effects; skipped open/walk and malformed
  UTF8 paths do not evaluate. Original flat-list/lifecycle regressions pass.
- Dot:condition/body use, empty/missing/parse-error/blocked/short-read/close-failed
  scripts, immutable PC reconstruction, no condition/assignment replay, separate
  dot/aggregate negation and one-line if in scripts pass.150 new stable conditional
  script repetitions join existing150 list/script repetitions. Bare exit and
  fatal streams stop later branches;8192-byte backpressure prevents early effects.
  Input-close failure selects else only after cleanup, with correct aggregate !.
- Main unchanged generated frontend:163 native syntax fixtures (including2 prior
  contextual POSIX gaps),185 raw JS/native cases/27 recursion profiles,6426 exact
  JS/native/WASM inputs (3243accepted/2353syntax/830lexical), tokens/UTF16/tree
  parity and restored stack. No grammar or generated production file changed.
- Word unchanged behavior:710 JS/native/WASM trees,327 regular byte/mask cases,
  252 independent field expectations,16 scalar counts/17 invalid vectors;
  ASan/UBSan, guards, full AST strict C89 and6 reproduced artifacts pass. Prefix
  and suffix119-step exact quota/exhaustion/recovery regressions remain intact.
  The isolated full own-chain malformed-value guest additionally proves skipped
  invalid expansion inside if and selected invalid-condition normal status2/else.
  Production exports lack injection hooks; temporary guests are removed and
  production binary hash remains unchanged by those tests.
- Real KEY/kernel/I/O/shell/cat/wc/LVGL:8 immutable recipes /122commands twice
  per-line and all8files twice via guest dot. Recipe03 still aborts at intentional
  syntax error line4. Its old unsupported-if check now uses an unsupported group
  body; the newly supported if has positive coverage. All other prior recipe
  behavior stays intact. New08-if.sh contains14bounded commands.
- Pipeline integration:634 final flushes;20 conditional dot/worker resumes with no
  replay; original AND-OR failures plus12 new conditional cases covering input
  read/close, stream write, output read/write and launch failure, normally and
  aggregate-negated. Branch selection follows reaping with no leaked endpoints,
  handles or workers. Conditional128KiB delayed/partial feed/drain and an actual
  worker-stop/trap path selecting else recover; original owner/blocked-worker
  trap, cancellation/rollback, reaping and shutdown tests pass.
- Host LVGL:2249flushes,1000parses/77911kernel steps, stable shell21/I/O2pages.
  Boot25sessions/28flushes leaves no windows/endpoints. Unit66pass. Standalone
  script framing19 C++98 ASan/UBSan cases and all earlier guest suites pass.
- Production shell.wasm1595285bytes, SHA256
6570e1f2b07cb8bcc9d0a2af421f3112db18dbe559ed275c61d5df0adb7e9e16.
  Shell32-page/four-handle and I/O8-page budgets,32tokens/1024nodes, eight leaf
  units,95-byte words/12slots,32variables and16KiB/256line scripts are unchanged.

### Integration findings resolved within scope

A new oracle fixture initially used globally reserved done as an unquoted argument;
quoted it, preserving its expected output and known grammar scope. Tests asserting
rejection of the newly supported if were updated only for that intentional change;
unsupported-group/nested cases preserve negative coverage. No old functional
expectation was changed to accommodate incorrect execution.

The slow-transfer fixture originally injected WOULD_BLOCK into zero-byte endpoint
availability probes as well as data writes. A second pipeline could therefore
spuriously report input busy. Delay/short-write injection now applies only to
nonempty feeding, retaining the actual probe result and extensive data stalls.
No pipeline source/kernel/ABI workaround was needed; both transfers and failure
modes passed afterward.

The expanded guide then exposed the terminal's final prompt reserve:4092bytes of
guide+echo fit the old unit check but the subsequent6-byte prompt scrolled away
its first characters in real LVGL. Condensed text; final guide+echo4072bytes,
plus prompt4078, within unchanged4096. The image test now explicitly reserves
that prompt for every readable asset.08script+echo771 /expected+echo968 also fit
with the prompt. No I/O storage, font, manifest, tick or timeout was increased.
The accepted final integration log is /tmp/maiaos-if-pipelines-verified.log;
earlier logs retain those fixture failures and are superseded for acceptance.

### Git, architecture and remaining boundaries

Root main b13f168 remains local/unpublished. Complete index and prior staged
Cpp8475812/Glob703dda2/Regexe2ae3a8 are unchanged. All7compiler/library principal
repos remain clean at their existing commits. No dependency, npm/version,
commit/push or history action. Existing work is preserved; kernel/I/O/worker
production, host semantics, ABI/process plans/manifests/fonts are unchanged.

Both generated frontends and MaiaGlob remain in the C++98 guest; the host only
transports bytes/instances, the kernel owns scheduling/process cleanup and real
I/O/LVGL owns terminal rendering/editing. Tests use the native WebAssembly
reference engine. Interactive browser, MaiaWASMVM/device and standalone kernel
native-policy/I/O/font checks were not run. Nested compounds, more than2elif,
multiline if, compound redirections/pipeline stages, loops/groups/subshells,
complete contextual keywords and broad POSIX conformance remain pending.
Reopen the simulator and run `. /system/tests/08-if.sh` for manual acceptance.

Final scope audit:30 justified root paths versus the pre-if checkpoint. Entire
index (including all3staged pins) and every unrelated tracked/untracked file
remain unchanged. Both grammars/generated sources and protected kernel/I/O/ABI/
manifest files retain their baseline bytes. All7owner principals remain clean.
Production SHA is preserved after isolated guest tests; git diff --check passes.
No root publication.
