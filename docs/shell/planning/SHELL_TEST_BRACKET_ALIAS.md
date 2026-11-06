# Bracket alias for bounded test — approved contract

Date: 2026-11-01. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the locally validated multiline-dot checkpoint.
SHELL_TEST_PREDICATES.md deliberately excluded this alias and any command-position
exception to strict filename patterns. This proposal requests precisely that
extension under AGENTS.md section 4.6; it does not authorize publication.

## Behavior and scope

Add the builtin `[ expression ]` as the existing bounded `test` predicate with a
required final argument whose complete value is `]`. These are ordinary argv
words, not a new shell compound or an alternative conditional grammar:

```sh
N=1
if [ "$N" -eq 1 ]; then echo selected; fi
[ -z "" ] && echo empty
[ a != b ]; echo $?
```

Reuse exactly the implemented string truth, -n/-z, =/!=, six signed-decimal32
comparisons and existing bounded predicate negation. Empty `[ ]` returns 1;
true returns 0, false 1, invalid expression/operand or missing terminator 2.
No stdout; usage errors go through existing bounded stderr/completion. A `]`
inside the expression is ordinary data: `[ "]" = "]" ]` is true. Only the final
argv frame terminates the invocation; whitespace remains necessary between words.
`[x]` remains an ordinary filename pattern/command and is not an alias spelling.

A quoted/escaped command name or one obtained through ordinary expansion may
resolve to `[`. Dispatch uses the final argv value, as for other builtins; do not
identify the alias by scanning source or special-casing an if condition. Unquoted
predicate `!` retains the main contextual-lexer limitation: quote it when it is
an operand; outer `! [ ... ]` works through existing pipeline negation.
No file predicates, -a/-o expression operators, recursive predicates, [[ syntax,
external process, worker, standalone wc or additional test integer width.

## Explicit command-position filename-pattern exception

Today MaiaGlob rejects the lone unprotected `[` before builtin dispatch. Keep
MaiaGlob itself unchanged, including its public API, malformed-pattern policy,
UTF-8 validation, budgets and generated pattern parser.

Only when a complete expansion field is exactly the single ASCII byte `[`, is
not a redirection target, is not a for-header value, and would be the first
non-redirection argv field of its current generated pipeline stage, compile that
field with a temporary one-byte protected mask. Still call the normal MaiaGlob
compiler and normal append path. Do not skip validation, dispatch early, change
the original source quote/provenance mask or protect the rest of the word/argv.
Command-position classification derives from existing generated stage ownership,
redirection descriptors and actual preceding expanded fields, not source spelling
or an assumption that original word index zero survives splitting.

Cover empty-elided first fields, promotion of a later field into command position,
multiple fields from one expansion, earlier patterns producing multiple results,
quoted/escaped/expanded names and each pipeline stage independently. A literal
`[` produced as a filename match follows normal final command dispatch; do not
rescan matched filenames as patterns. Unsupported bracket worker placements still
reject before launch under the fixed cat/final-wc profile. No new worker exists.

Every other lone `[` keeps its existing policy: `echo [`, `test [`, a bracket
predicate's unquoted `[` operand, `for N in [; do ...; done` and redirection target
`[` remain invalid patterns. Quote those data operands normally. Malformed
multi-byte patterns such as `[abc` remain errors even in command position. Valid
patterns retain enumeration/sorting/no-match behavior in all positions.
This narrow command-position exception is a deliberate approved policy change,
not a claim of full POSIX unmatched-bracket behavior.

## Generated predicate frontend and storage

Extend only maia-shell-test.ebnf and regenerate its JavaScript/XML/C++98 and
include adapters through the pinned in-repository MaiaCC/tREx. The generated
frontend must own the opening command frame, required final closing frame, arity,
operator/value distinctions and EOF for every supported alternative. Preserve
literal `[`/`]` values and existing `test` ambiguities. C++ must not strip argv
sentinels with a handwritten predicate recognizer, reparse joined shell source,
or synthesize an AST. Share the existing checked semantic evaluator.

The largest current predicate has four expression operands. With opening and
closing frames this requires six 96-byte slots: private transport up to 576
bytes plus its guard and seven tokens including EOF. Retain the current 128-node
predicate cap unless measured generation proves it insufficient; report that
before increasing it. The test spelling keeps its old semantic arity even though
the shared transport envelope grows. Prove unchanged `test` status/tree behavior,
exact closure ownership and bounded rejection of excess data/tokens/nodes.

No main or word EBNF/generated change is expected: the current main parser already
accepts bracket command words. No manual edits to generated code, host predicate
execution, recursive evaluator, new syscall/capability, process plan or ABI.

## Resource lifecycle and immutable recipes

Reuse normal lazy unit expansion, status, negation, if/loop selection, dot return,
redirection opening, stderr draining and handle close. The predicate performs no
VFS operations. An invoked malformed predicate may already have opened/truncated
its selected redirect: preserve that lifecycle, do not promise rollback.
Unselected branches must not invoke the predicate or open redirects. Raw close
failure and worker rejection retain existing policies, regardless of negation.

Keep 255-byte input/combined dot unit, eight physical lines per dot unit,
16KiB/256-line snapshot, 95-byte words/values, 12 original/final argv slots,
32 locals, eight leaves/32 flow instructions, loop limits, 32-op dispatch and
16-step quantum. Shell 32 pages/four handles, I/O eight pages and 4096-byte terminal
bound stay unchanged; measure actual memory/stack and repeated failure recovery.

Add one read-only VFS recipe with exact expected outputs, including a conditional
and bracket data. Preserve all prior sixteen recipes/expected bytes. The current
guide plus echo/prompt is 4073 bytes and expected.json is 15972 bytes: condense the
guide/metadata as needed and verify both display and 16KiB script-read bounds;
never enlarge terminal/snapshot storage to accommodate documentation. Host code
mounts bytes and compares expected output; all predicates execute in the guest.

Expected files: predicate EBNF/generated adapters, shell-test.hpp, the narrow
shell-glob.hpp command-mask adaptation, shell.cpp dispatch/help, focused existing
predicate/glob/field and integration tests, immutable VFS assets, reproducible
shell.wasm and progress/architecture/handoff evidence. Kernel/I/O/worker sources
and artifacts, compiler owners, dependencies/gitlinks, versions and npm stay
unchanged. No commit/push, history rewriting or staging. Preserve all prior dirty
work and the full index, including its three staged dependency pins. An owner
compiler defect requires a separate reproduction and authorization.

## Acceptance gates — passed locally

- [x] Generated JS first: fixed arity/terminator/operator/value expectations and
  exact token/UTF-16 span/complete-tree comparison against native C++98 and actual
  own-chain WASM; reproduce every generated artifact without manual changes.
- [x] Fixed native dash/Bash oracle vectors for the shared bounded profile;
  distinguish project numeric limits, strict argument-pattern policy and existing
  reserved-word exclusions from reference-shell behavior.
- [x] ASan/UBSan/native and own-chain WASM: closing bracket as data, empty values,
  Unicode/invalid bytes, all comparisons, signed32 extrema, 95/96-byte values,
  576-byte transport, token/node guards/exhaustion, corrupt framing and recovery.
- [x] Command-position mask exception: exact lone byte only, quote/provenance
  preservation, fields/elision/promotion, glob-result commands, redirects, for
  headers and all stage positions. Retain every old malformed argument pattern
  regression; add explicit positive command-only cases without weakening errors.
- [x] Actual guest if/loops/lists/status/negation, multiline dot/return without
  replay, missing/extra terminators, rejected workers before launch, selected/
  skipped redirects and no predicate VFS effects; blocked stderr and raw close
  failures preserve cleanup, normal status and next-unit selection.
- [x] Real KEY/kernel/I/O/shell/cat/wc/LVGL recipe twice per-line and via dot, all
  previous recipes and transfer/trap/cancel/reap checks, repeated success/error
  invocations and dot callers with stable memory/stack/resources.
- [x] Two `npm run build:shell -- --check-c89` runs reproduce shell.wasm.
  Run test:shell-predicates-native, test:shell-predicates, test:shell-glob,
  test:shell-fields, test:shell-word, test:shell-variables, test:shell-status,
  test:shell-lists-native, test:shell-lists, test:shell-loop-if-native,
  test:shell-loop-if, test:shell-for, test:shell-conditional,
  test:shell-loop-control, test:shell-scripts-native, test:shell-scripts,
  test:shell-redirection, test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-pipelines, test:shell-lvgl,
  test:shell-boot and npm test; focused return/multiline native/guest checks.
  Schedule expensive compiler jobs serially; preserve timeouts/expectations.
- [x] Update affected shell/kernel/I/O checklists and evidence. Audit all changed
  paths, protected bytes, principal owners, root HEAD and complete Git index.

Manual browser, MaiaWASMVM/device and full POSIX conformance remain separate;
no current preparation result marks any implementation gate complete.

## Read-only preparation evidence — 2026-11-01

Inspected current generated grammar, finite predicate evaluator, ordinary builtin
dispatch, stage/field/glob/redirection ownership, resource limits and prior
contracts. Eleven unchanged generated-main syntax cases parse, including raw
and expanded alias candidates, one-line/multiline if and the still-invalid
execution-profile argument/header brackets. Syntax acceptance alone is not glob
or predicate execution evidence.

Thirty-eight fixed native dash/Bash observations pass: sixteen bounded predicate
vectors per shell plus three missing/extra-terminator usage errors per shell.
They check statuses/stdout silence, empty/closing-bracket data and numeric/string/
negated predicates. Native source/loader lives only in temporary storage, removed
after the probe; /tmp/maia-bracket-probe.cjs records the exact reproduction.
No production MaiaOS alias exists yet; no generated/backend/build/full integration
gate was run in preparation. Preparation changes exactly this proposal, one
unchecked shell checklist row and handoff. Full baseline/index is recorded in
/tmp/maia-bracket-proposal-before.json. No publication or dependency action.

## Implementation and acceptance evidence — 2026-11-01

Bracket test alias (2026-11-01): [approved contract and exact acceptance](SHELL_TEST_BRACKET_ALIAS.md).
The generated private predicate frontend now owns `[ expression ]`, including
its final closing frame; existing bounded test semantics/status/cleanup are shared.
Only a lone ASCII `[` in actual stage command position receives a temporary
protected mask through the normal MaiaGlob compiler. Argument/redirect/header
patterns stay strict; main/word/return frontends and all kernel/I/O/worker
production remain unchanged. 457 JS/native/own-chain WASM tree/token/span/status
vectors,660 dash/Bash observations,12 invalid UTF8 frames,six slot vectors,
576/577-byte guards,ASan/UBSan and300 reuse calls pass. Production WASM passes445
fixed predicate cases,field promotion/glob commands,150 bracket dot callers with
no prefix replay,and12 true/false/usage close-failure/negation combinations.
Real KEY/kernel/I/O/LVGL runs17recipes/209inputs twice per-line and all17files
twice via dot,plus six bracket worker cases and20 multiline worker/dot returns
(1482flushes). All25 additional regression/conformance gates and66unit tests pass.
Host:1000parses/2177flushes/77911steps;boot:25sessions/28flushes. Shell21/I/O2pages
remain stable. Two strict builds reproduce shell.wasm1,703,122bytes/SHA256
`980627cdb7af612e8d28d9e24bed032b20e986a91a09185ec85f7ed6629b8ca2`.
Guide+echo/prompt4091bytes<4096;expected.json16042bytes<16384;prior16recipe bytes
remain unchanged. Root main b13f168,full staged index and seven principal owners
are preserved. Working tree only:no commit/push/npm/version/gitlink changes.
Manual browser,MaiaWASMVM/device and full POSIX conformance were not run.

Grammar additions are confined to maia-shell-test.ebnf, generated in JavaScript
and C++98 by scripts/generate-shell-test.mjs. Seven bracket alternatives own
CLOSE/EOF and share generated operator/value nodes with existing test semantics;
OPEN/CLOSE tokens remain legal data under Value. Maximum fixture tree is14/128
nodes. Six generated artifacts reproduce byte-for-byte (the include adapter is
unchanged). shell-test.hpp changes only its bounded transport and branch-node
semantics; shell.cpp adds alias dispatch/help/usage. shell-glob.hpp derives the
actual stage command field from generated stage IDs, previous expanded fields
and redirection descriptors. It passes a one-byte temporary mask through the
normal compiler and never edits the stored quote/provenance array or library.

All original225 test vectors remain, with225 bracket counterparts and seven
missing/extra-terminator cases. Initial preparation accidentally treated `[ ] ]`
as invalid; it is valid truth of `]` with a final delimiter, already covered by
its positive data fixture. The extra-data negative uses a nonclosing final word.
No preexisting negative or budget was removed. The guest wildcard fixture now
enters its explicit home directory; help expectations include the approved alias
spelling. These were test preparation/feature expectations, not compiler defects.

Exact commands (all final exit0):

- `npm run test:shell-predicates-native`:457 independent JS/native/own-WASM
  statuses and exact tokens/UTF16 spans/trees,660 fixed dash/Bash observations,
  ASan/UBSan,12 malformed UTF8 frames,576/577-byte envelope guards,token/tree
  exhaustion/recovery,95/96-byte slots,six additional native/WASM slot layouts,
  300 reused calls and six reproduced generated files.
- `npm run test:shell-predicates`:445 production status/silence cases,command
  promotion/quoted-empty fields/quoted-escaped-expanded aliases,glob-result
  commands and multiple sorted matches,strict bracket argument/header/redirect
  errors,stage rejection,if/loops/status,150 bracket dot callers with exactly150
  prefix appends,zero plain/skipped predicate VFS calls,12 close-failure results
  across test/bracket,true/false/usage and ordinary/negated completion. Stable21pages.
- `npm run test:shell-pipelines`:17immutable recipes/209per-line inputs twice,
  all17files twice through guest dot,six new bracket worker-selection/rejected
  stage cases,20multiline dot/return caller repetitions,1482realLVGLflushes.
  Existing128KiB delayed transfers,raw failures,traps,cancel/reap remain passing.
- `npm run build:shell -- --check-c89` twice:identical1,703,122bytes andSHA256
  above,AST-strict C++98,strict C89 and own MaiaC/MaiaWASM generation/validation.
- `npm run test:shell-lists-native`, `npm run test:shell-loop-if-native`,
  `npm run test:shell-scripts-native`, `npm run test:shell-grammar`,
  `npm run test:shell-parser`, `npm run test:shell-equivalence` retain native
  guards/plans and163syntaxfixtures(two known contextual gaps),185rawcases/
  27profiles and6426exactJS/native/own-WASM inputs respectively.
- `npm run test:shell-for`, `npm run test:shell-conditional`,
  `npm run test:shell-loop-control`, `npm run test:shell-loop-if`,
  `npm run test:shell-variables`, `npm run test:shell-status`,
  `npm run test:shell-lists`, `npm run test:shell-scripts`,
  `npm run test:shell-redirection`, `npm run test:shell-glob`,
  `npm run test:shell-lvgl`, `npm run test:shell-boot`, `npm test` (66tests).
- `node scripts/test-shell-return-native.mjs`,
  `node scripts/test-shell-return.mjs`,
  `node scripts/test-shell-multiline-native.mjs`,
  `node scripts/test-shell-multiline.mjs` retain existing status/control/return,
  guarded framing,one-failed-prefix-per-step and actual guest stability checks.
- `npm run test:shell-fields`, `npm run test:shell-word` run serially after
  compiler jobs,retaining existing independent word/field/native/WASM cases,
  invalid UTF8,selected/skipped malformed-value predicate probes,quote/provenance,
  aggregate pattern budgets,sanitizers,guards and reproducible generated outputs.
- `node --test tests/simulator-vfs.test.js`:two image/resource/display-bound tests.
  `git diff --check`, scoped baseline hash/index/owner audits and identical build
  artifact comparison pass. No missing prior file,index change or publication.

Final metadata16042bytes<16384;guide4050+echo35+prompt6=4091bytes<4096.
See the final audit for exact immutable asset sizes; previous
sixteen scripts and their expected text remain byte-identical. Display storage,
script/word/argv/loop quotas,shell32/I/O8page manifests and four handles stay fixed.
Manual browser,MaiaWASMVM/device,standalone native kernel/I/O/font and complete
POSIX conformance were not run for this shell-only increment. The browser harness
uses reference WebAssembly with actual LVGL and system calls,not MaiaWASMVM.
