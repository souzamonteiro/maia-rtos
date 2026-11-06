# Bounded multiline units in dot scripts — approved contract

Date: 2026-11-01. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the locally validated script-return checkpoint.
This proposes a script execution change under AGENTS.md section4.6. The owner's
response explicitly approves this contract and implementation.
Publication and changes in other repositories are excluded.

## Selected scope

Allow the currently implemented compound/list/pipeline forms to span physical
lines inside `. path` scripts. Examples:

```sh
if true
then
    echo selected
fi

for N in a b
do
    echo "$N"
done
```

This extends input framing only. Keep the existing single loop,one optionally
negated loop-body if,flat if/elif/else,break/continue,return,variables/expansions,
redirections and fixed cat/final-wc execution contracts. Nested if/loops beyond
the already approved shape,groups,subshells,functions,case,heredocs,background
jobs,nested dot and generic execution remain rejected,even in skipped branches.

Support generated newline placement in if/for/while/until,comments/blank lines
within a compound,and permitted continuation after `|`, `&&` or `||`. Every
selected logical unit still receives whole-unit structural/quota preflight before
effects. Ordinary complete commands on successive lines remain separate units,
with existing status and non-errexit behavior. Do not batch the entire script
into a new AST or reset execution budgets at each physical line of a compound.

Interactive terminal input stays one physical line: no secondary prompt,new
keyboard editing/multiline paste behavior,I/O protocol or host interpreter.
Quoted multiline source words,escaped-newline continuation and multiline
parameter-source words remain excluded. Expanded values keep existing quote,
field/glob and byte semantics;do not reject expansion data merely for containing
LF. The restriction concerns source terminals owned by the main generated lexer.

## Generated completion and physical framing

Keep all existing EBNFs/generated frontends unchanged. The main Input grammar
already owns linebreaks,compound termination and pipeline/list continuation.
Use that generated lexer/parser to test a bounded physical prefix,never source
keyword matching,brace/quote counting,semicolon insertion or handwritten syntax.
Do not use errorPosition/expectedToken as an invented incomplete-input API:
current errors include speculative failures and are not an EOF-completeness
contract.

For each new unit:

1. Start from the next physical line in the existing immutable snapshot. Preserve
   its bytes without the final physical LF,as the current one-line reader does.
2. Quietly try the generated lexer and Input parser on that entire candidate.
   An accepted candidate is the earliest complete unit. Apply the existing
   whole-unit AST validation/plan/expansion preflight and then execute normally.
3. On lexical/syntax failure only,append the actual separating LF and the next
   physical line,within the bounds below,and retry cooperatively. Preserve CR,
   indentation and comment bytes. Never rewrite source into a different syntax.
4. At snapshot EOF with no complete candidate,abort with ordinary syntax status2
   and the first physical line of that unit. At a byte/line/token/node bound,
   abort with ordinary limit status2. Unsupported structure or static limits
   after successful parsing abort immediately;do not search for a later prefix.

The last trial omits its final LF just as today;separating LF is retained only
when extending a failed candidate. This preserves token counts and behavior of
existing complete one-line units. CR already present before LF is preserved;
when extended,the generated CRLF rule owns that pair. No normalization or newly
claimed dash/Bash CRLF agreement is introduced.

A failed prefix is not executed and its partial tree is never planned or used
for effects. Selection/expansion/VFS/process/binding effects wait for a complete,
validated unit. This algorithm intentionally does not distinguish an invalid
prefix from an incomplete one: either may be extended up to the fixed bound,
then rejected if still incomplete. It is not syntax-error recovery or permission
to skip a bad line. No command after a rejected unit executes. Previous completed
units retain their effects;this is not whole-script transactional validation.

Explicitly validate the existing source-word restriction using generated
WORD/ASSIGNMENT_WORD terminal ownership and byte spans before effects. A
successfully parsed source word spanning CR/LF remains unsupported. Do not
introduce a second word lexer or apply that source restriction to expansion data.

## Bounds and cooperative progress

One logical candidate may span at most8physical lines and255bytes total,including
preserved interior LF/CR/indentation/comments. The final LF omitted for a trial
does not count. Blank/comment lines inside an unfinished unit count toward8.
Keep32main tokens/1024nodes,8static leaves/32flow instructions,95byte words,
12argv,32variables,for12values,conditional12bodies/13conditions and all pipeline
limits per complete logical unit. Reject before array writes or body effects.
The containing snapshot keeps16KiB/256physical lines,255bytes per physical line,
no NUL and its existing load/read/close validation before any execution.

Process at most one failed-prefix lexer/parser attempt per shell scheduler step,
then yield to the existing kernel scheduling path before extending it. Keep the
existing16-step execution quantum and32-op dispatcher. Completion work is bounded
by8physical attempts per unit,plus at most one duplicate acceptance parse if the
existing preparation entry is retained. Measure/prove that bound;do not silently
create a synchronous scan of the entire snapshot or a non-progress busy loop.
A small explicit collection flag/counters may extend the private script state;
this is not a new public process state or scheduler policy.

Use the existing256byte command buffer for the candidate and retain at most
fixed counters for its length,physical count and starting line. No second source
snapshot,AST,recursive evaluator or dynamic compound stack. A narrow private
factor of preparation may reuse an accepted tree instead of reparsing,but must
preserve original terminal/caller preparation and validation semantics.

Retain shell32/I/O8page and four-handle budgets;measure actual memory. The script
file is already closed before collection,so lookahead opens no handle. Failed
probes do not modify last completed status,caller frames or saved source.
Blank/comment-only complete units preserve prior status as before.

## Execution, diagnostics and return

Execute the accepted unit's existing forward/backward plan across any yields,
workers,redirections or script return. Do not change branch selection,unit versus
aggregate negation,loop last-body status,control transfer or raw failure policy.
Unit boundaries come from the generated full-Input acceptance,not from a token
spelling or newline count used as a semantic command separator.

Track physical position separately from unit start. Rejected/incomplete units
report their first physical line in existing script-level abort diagnostics;
one-line failures retain their current number. Explicit return certifies the
actual active script position and EXEC origin. It abandons the current logical
unit/script and reconstructs the saved caller once after cleanup. Its requested
status and the dot caller's negation remain independent. Clear collection state
on load,completion,abort,return,exit and fatal teardown;no pending source or
partially built plan may run on return to the terminal.

A caller remains the existing one-line terminal source because nested dot stays
unsupported. Preserve saved for values/index and conditional phase/count/body
status,inner-if prefix certification and generated caller PC without replay.
A multiline loop must not reset its iteration budget at physical boundaries or
return,consume terminal bytes while collecting,or let script-local control target
a suspended caller. EOF/exit/fatal cleanup and ordinary command-error continuation
remain unchanged.

## Files and ownership

Expected changes: small script collection/framing state and completion glue in
shell-script.hpp/shell.cpp,a narrow preparation/profile check only if needed,
reproducible shell.wasm,independent generated/native fixtures,sanitized framing/
state tests,actual guest/kernel/I/O/LVGL tests and immutable recipe16 with distinct
terminal per-line versus dot expectations. Update shell/kernel/I/O trackers,
architecture and handoff only with verified results.

Host mounts bytes only. Shell C++98 owns completion/parsing/planning/execution;
kernel retains process scheduling/reaping;I/O retains editing/events/real LVGL.
No new syscall,ABI,capability,manifest budget,process plan,kernel/I/O/worker
production,glob policy,compiler/other-repository edit,gitlink,staging,commit/push,
npm,version or history change. Preserve all prior work/full index. A grammar or
generator defect requires separate owner reproduction/approval before edits.

## Acceptance gates — passed for the approved implementation

- [x] Independent generated-JS completion/owned-node fixtures and exact target
  C++98 tokens/spans/trees on identical raw LF/CRLF sources. Test earliest-prefix
  boundaries,comments/blank lines,complete one-line compatibility,operator tails,
  malformed prefixes and excluded source-word continuation. Retain all existing
  grammar/parser/equivalence/word/return artifacts byte-for-byte.
- [x] Fixed native shell output/status/effect comparisons within shared scope.
  Keep existing interrupted-negation differences and project CRLF behavior
  explicit;do not change previously approved semantics to match a different shell.
- [x] Native C++98 ASan/UBSan of any framing/state helper:0/1/8/9line bounds,
  255/256candidate bytes,physical framing/NUL/snapshot limits,no overflow,failed
  probes without effects,cleared stale/corrupt state and at-most-one-failed-trial
  per scheduled step. Keep original32/33flow and request/frame certification.
- [x] Actual own-chain WASM:multiline if/elif/else,for/while/until,permitted inner
  if/negation,break/continue/return,status/default parameters/Unicode/fields/glob,
  pipeline/list continuations,lazy effects,8leaves/32tokens/1024nodes and0/12/13
  iteration cases. Parsing/collection cannot grant wider compound nesting.
- [x] Earlier completed unit effects survive a rejected later unit;incomplete/
  unsupported/over-limit whole units have no body effects. EOF,physical start
  diagnostics,ordinary failure continuation,blank status and unchanged recipe03
  abort behavior are tested. No error scanner or host execution fallback.
- [x] Multiline worker operations and raw read/write/open/close/drain faults,
  backpressure/cancel/reap,return cleanup/caller PC and independent negation,
  at least150script/200loop repetitions with stable stack/memory/handles and no
  replay,count reset,terminal-byte consumption or suspended-caller control.
- [x] New recipe plus all15existing immutable recipes twice per-line and via dot
  through actual KEY/kernel/I/O/shell/cat/wc/LVGL. Keep the expected terminal
  rejection of incomplete lines separate from dot completion. Existing recipe
  bytes remain intact;all display/guide/assets fit4096byte I/O/16KiB image limits.
- [x] Existing return,loop-if/control,conditional/for,list/script,status/word/
  fields/variables,predicate/redirection/glob,grammar/parser/equivalence,real
  LVGL/boot and unit gates pass without weakened expectations or timeout changes.
- [x] Two `npm run build:shell -- --check-c89` builds reproduce final shell.wasm;
  update checklists/memory/hash/size/results,audit scope/all protected hashes,
  full index/pins and seven clean owners. Manual browser,MaiaWASMVM/device,
  interactive multiline and general POSIX conformance stay separate.

## Preparation evidence — not guest implementation

`node /tmp/maia-multiline-preparation.mjs` passes24unchanged generated-JS earliest-
complete-prefix fixtures,91bounded physical-prefix parse attempts and46fixed
per-shell dash/Bash output/status comparisons. Fixture data:
/tmp/maia-multiline-preparation-cases.json. Tests cover LF compounds/operator
continuations,comments/blanks,8physical lines,one-line boundaries,return,Unicode,
loop control and negation. Temporary parser/body files are removed;subprocesses
have2second timeouts. No production source/build/full regression rerun is claimed.

Among23native-reference fixtures,one records the already approved interrupted-
negation difference (dash1/Bash0). The24th is generated-parser-only CRLF framing:
installed dash/Bash do not share the main grammar's CRLF interpretation. Initial
reference assertions exposed those two distinctions;the final fixed observations
separate them rather than treating either as a new toolchain defect.

Production shell.wasm remains1,689,597bytes/SHA256
`d54d3d1f619d3f4b5e980f8b2e75ec9ca5d2fa9dcdc7b1e449c2f7b6e5189cd2`.
Main remains b13f168 with the previous dirty tree and staged gitlinks. Only this
proposal,the shell pending checklist and handoff changed at preparation. The
owner subsequently approved this contract;final implementation evidence follows.

## Final local acceptance — 2026-11-01

Multiline dot units (2026-11-01): the scoped approved contract above.
Dot scripts now collect the earliest physical prefix accepted by the unchanged
MaiaCC-generated Input parser. One fixed framing state reuses the256-byte command
buffer,up to8physical lines/255bytes;incomplete candidates yield after one failed
trial per shell step. Only a complete,validated unit may expand or execute.
Main/word/test/return grammars and generated frontends remain byte-identical.
24 generated-JS fixtures/91prefix attempts,46fixed native-shell observations and
24C++98ASan/UBSan traces compare exact tokens/UTF-16 spans/trees. Guarded framing
checks0/1/8/9lines,255/256bytes,9corrupt-state variants,no partial append and reset.
Actual WASM passes all24fixtures,one-failed-trial-per-step timing,whole-unit
atomicity/source-word exclusions,earlier effects/start-line diagnostics,existing
loop/status/return quotas,load/close/short/stall and EOF/exit/fatal cleanup.
150dot/200loop repetitions retain stack and memory at shell21pages without replay
or budget reset. Interactive input remains one physical line;multiline source
words,escaped-newline continuation and broader nesting remain excluded.
Real KEY/kernel/I/O/shell/cat/wc/LVGL runs16immutable recipes/203per-line inputs
twice and all16files twice via dot. Recipe16 has distinct fixed terminal rejection
versus guest dot output.20additional multiline/worker/return resumptions and6new
worker-failure cases pass with existing128KiB transfer/trap/cancel/reap regressions
(1459flushes). Host1000parses/2375flushes/77911steps and25sessions/41flushes retain
shell21/I/O2pages.24baseline regression gates,two focused return gates,isolated
fields/word conformance and66unit tests pass. Two strict builds reproduce
shell.wasm1,691,894bytes/SHA256
`8d87228dcb99dc01d6407c03fa30acf2bc2938e60e47752956677309259b6483`.
All existing generated frontends,kernel/I/O/worker production,ABI,fonts/manifests,
glob policy,seven clean principal owners and the entire prior staged index/pins
are preserved. Guide+echo+prompt4073bytes<4096;expected.json15972bytes<16384.
Working tree only:no commit/push/npm/version changes. Manual browser,
MaiaWASMVM/device and broader POSIX conformance remain separate.

Exact commands (all exit0):

- `node scripts/test-shell-multiline-native.mjs`:24generated-JS completion
  fixtures/91candidate attempts,46fixed native reference outputs/statuses,
  identical24C++98ASan/UBSan traces with complete token/UTF-16 span/tree comparison,
  guarded0/1/8/9line/255/256byte append bounds,9corruptions and no partial writes.
- `node scripts/test-shell-multiline.mjs`:24actual guest fixtures,8/9lines,
  255/256bytes,one-failed-trial-per-step file-effect test,unsupported nesting/
  source-word/escaped continuation,whole-unit atomicity,earlier effects/start-line
  diagnostics,lazy Unicode/glob,0/12/13loop bounds,150dot/200loop stability,
  raw return-close cancellation,read/close/short/stall and EOF/exit/fatal cleanup.
- `npm run test:shell-pipelines`:16recipes/203per-line inputs twice and16files
  twice via guest dot,20new multiline/worker/return resumptions,4additional
  pipeline/condition/list cases and6worker-failure/return combinations;
  previous128KiB transfers/trap/cancel/reap,1459realLVGLflushes.
- `npm run build:shell -- --check-c89` twice:identical final artifact hash/size
  above,strict AST-own-chain build and unchanged generated frontends.
- `npm run test:shell-for-native`, `npm run test:shell-conditional-native`,
  `npm run test:shell-loop-control-native`, `npm run test:shell-lists-native`,
  `npm run test:shell-predicates-native`, `npm run test:shell-scripts-native`,
  `npm run test:shell-loop-if-native` retain frame/control/canary regressions.
- `npm run test:shell-grammar`, `npm run test:shell-parser`,
  `npm run test:shell-equivalence`:163syntaxfixtures(two known contextual gaps),
  185rawcases/27profiles,6426exactJS/native/WASMinputs respectively.
- `npm run test:shell-for`, `npm run test:shell-conditional`,
  `npm run test:shell-loop-control`, `npm run test:shell-loop-if`,
  `npm run test:shell-predicates`, `npm run test:shell-variables`,
  `npm run test:shell-status`, `npm run test:shell-lists`,
  `npm run test:shell-scripts`, `npm run test:shell-redirection`,
  `npm run test:shell-glob`, `npm run test:shell-lvgl`,
  `npm run test:shell-boot`, `npm test` (66unit tests).
- `node scripts/test-shell-return-native.mjs`,
  `node scripts/test-shell-return.mjs`:existing generated status frontend,
  certified request,all256statuses,raw cancellation/caller restoration retained.
- `npm run test:shell-fields`:actual expansion/status/pattern regressions and
  separate own-chain malformed-value guest,normal error/negation/cleanup.
- `npm run test:shell-word` in isolation:710wordtrees,327byte/mask vectors,
  252field cases,16Unicode counts,17invalid-byte cases,guards/sanitizers,
  own-chainWASM and reproduced generated artifacts;no timeout/expectation change.
- `git diff --check`;baseline hash/scope audit/full staged-index comparison;
  seven principal owners remain clean,with protected production/grammar/generated
  hashes preserved. No missing baseline file,staging/commit/push/npm/versions/pins.

One bounded duplicate parse is retained only after acceptance,through the original
preparation entry;failed probes never plan or execute partial trees. The actual
8-line effect test observes no VFS body write through the first7scheduled trials,
then normal completion. Ordinary one-line token counts remain unchanged because
a final physical LF is included only when extending a failed prefix. Recipe03
retains its line4abort/output. Recipe16's isolated terminal lines fail normally,
while dot prints `multiline` and succeeds;host expected-output selection does
not execute or simulate the body. Existing15recipe bytes are unchanged.
Guide4032+echo35+prompt6=4073bytes<4096;recipe31bytes/expected76bytes;
expected.json15972bytes<16384. Shell21/I/O2pages stay within32/8budgets.
Standalone native kernel/I/O/font,manual browser,MaiaWASMVM/device and general
POSIX conformance were not run for this shell-only increment.
