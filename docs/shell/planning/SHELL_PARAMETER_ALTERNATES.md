# Named parameter alternates — approved contract

Date: 2026-10-30. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the validated local named-default checkpoint.
SHELL_PARAMETER_DEFAULTS.md explicitly excludes alternate operators; this proposal
requests only the bounded extension below. Owner explicitly approved implementation and validation; publication remains
excluded.

## Selected semantics

Add exactly `${NAME+word}` and `${NAME:+word}` in ordinary words, double quotes,
assignment RHS values and existing redirection targets. NAME retains generated
ASCII syntax and its31byte bound. Neither form assigns NAME, allocates a variable
slot, emits NAME's bound value or changes status by itself.

| NAME state | `${NAME+alternativa}` | `${NAME:+alternativa}` |
|---|---|---|
| Unset | empty | empty |
| Set empty | alternativa | empty |
| Set nonempty (for example ação) | alternativa | alternativa |

The plus form tests table membership only. Colon-plus also tests whether the
stored value begins with NUL. Neither selection operation decodes or copies the subject
value: even a malformed stored nonempty byte sequence selects word normally.
An actual `${#OTHER}` within a selected word retains the existing UTF-8 validation
and ordinary status2 failure; an unselected word must not look up OTHER or decode
it. No value work for skipped AND-OR units or static preflight.

The alternative word reuses the supported default-word subset: literals, existing
escapes/quotes, `$NAME`, `${NAME}`, `$?`, `${?}` and `${#NAME}`. It may be empty.
All references use the selected unit's existing incoming table/status snapshot.
Default and alternate forms may be adjacent, but neither may appear inside the
other's selected word in this increment. Thus `${A:+${B}}` is supported, while
`${A:+${B:-x}}`, `${A:-${B:+x}}` and `${A:+${B:+x}}` remain excluded.
Command/arithmetic substitution and assignment/error/removal/positional operators
remain excluded. Literal occurrences protected by appropriate single quotes or
escaped dollars remain data under the existing quote profile.

Only a selected alternative word contributes bytes, field-split provenance,
quote protection, positional empty anchors or glob patterns. For example,
`${UNSET+""}` contributes no unquoted field; `${EMPTY+""}` contributes one explicit
empty field if EMPTY is set. Enclosing `"${UNSET+word}"` retains its own quoted
empty field. Preserve independent expectations for these three cases. Unquoted
selected expansion data uses existing fixed space/tab/LF splitting and MaiaGlob;
assignment RHS and redirect targets stay unsplit with existing ambiguity checks.
No rescan of inserted value bytes or accidental interpretation of operators.

Static whole-line validation checks syntax, semantic variable names and literal
bounds in every alternative without binding lookup, field/glob work or mutation.
Keep the existing conservative literal-bound policy even for unselected words;
never concatenate both runtime branches. BareName remains a literal-word grammar
alternative with a95byte bound, not a semantic variable name with a31byte bound.
Invalid whole-line syntax and static bounds fail before any command effects;
selected runtime overflow/encoding failures follow existing status/negation and
cleanup paths before assignment, file opens or worker launch.

## Generated syntax and bounded rendering

Extend both existing EBNFs and regenerate exclusively through MaiaOS's own
MaiaCC/tREx. Reuse finite default-word/quote productions and subtree traversal
where possible, adding generated operator identity for plus and colon-plus.
No handwritten source scanner, substring reparse, generated-file patch, recursive
renderer or broad refactor. No recursive operator production or heap allocation.
Preserve BRACED_DOLLAR/SUBSTITUTION_DOLLAR guards and complete opaque ownership.

Introduce an explicit plus token in the word EBNF if needed for operator identity;
remove only its byte from RAW and retain it in the appropriate literal/escaped/
opaque rules. In particular, preserve unquoted escaped plus and double-quoted
backslash-plus (the latter keeps its backslash). Do not accidentally broaden the
special double-quote escape set. Keep name/assignment/IO_NUMBER/reserved-word,
comment, Unicode span, group and adjacent-redirection ownership in the main lexer.

Previously opaque quoted alternate forms gain meaning only for complete forms in
this contract. Unsupported nested/malformed/other forms remain wholly opaque when
quoted, without inner expansion, and reject unquoted before effects. Preserve
existing defaults, simple/length/status references,95byte literal regression,
assignment semantics and all execution/resource contracts.

## Scope and fixed boundaries

MaiaOS only: main/word EBNFs and reproducible generated sources; shell-word.hpp
and only necessary private provenance helpers; existing parser/native/WASM word
probes and guest regression suites; VFS recipes/expected bytes; shell.wasm;
progress READMEs, architecture, prior-contract follow-ups and handoff.

Keep physical line255bytes, semantic name31bytes, rendered word/value95bytes,
32variables,12 original/final slots including redirect targets,256word tokens,
2048word nodes,32page/fourhandle shell manifest and the4096byte I/O display.
Guides/recipes/expected text plus command echo must retain their existing display
size gate; reorganize/condense data rather than enlarge terminal storage.
Kernel/I/O/compiler owner sources and binaries, syscalls/imports, process plans,
versions and dependency pointers stay unchanged. Preserve all prior dirty work
and the three staged Cpp/Glob/Regex gitlinks. No commit/push, npm, version change
or history rewrite. Any owner-toolchain defect needs its own minimal reproduction
and separately authorized correction/propagation.

## Acceptance gates — completed locally

- [x] JavaScript first: independent main token/span/AST expectations and word
  operator/quote/branch ownership; complete/empty/malformed, UTF-8, adjacent
  operators/assignments/comments/groups and opaque excluded combinations.
- [x] Raw native C++98 and actual own-chain strict-C89/WASM lexer/parser parity
  for exact tokens, UTF-16 spans and complete trees; artifact reproduction.
- [x] Independent native dash/Bash oracles plus C++98 ASan/UBSan and actual WASM
  byte/mask/split/anchor expectations for the three-state matrix, explicit empty
  quotes in selected versus skipped words, inner/enclosing quotes, escapes,
  fallback simple/status/length references, glob data and concatenation.
- [x] Skipped malformed-value length never decodes; selected length fails
  normally, malformed nonempty subject may select a valid alternative without
  decoding/copying it; no table mutation or implicit allocation in a full table.
- [x] Exact31/32names,95/96words/values (including literal BareName),12/13slots,
  current token/tree/line limits, static atomicity, lazy lists, assignments,
  unsplit/ambiguous redirects, empty commands/stages, dot/status and failure
  cleanup. Repeated actual guest runs keep memory and stack stable.
- [x] Record generation size/time relative to the exact default checkpoint;
  finite flat parser depth/no same-rule-position cycles in JS, restored native/
  WASM state and no raised bounds/timeouts to conceal a regression.
- [x] npm run build:shell -- --check-c89; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields,
  test:shell-variables, test:shell-status, test:shell-lists,
  test:shell-scripts, test:shell-redirection and test:shell-glob pass.
- [x] Updated VFS recipes/expected bytes run twice per-line and twice by guest
  dot through real KEY/kernel/I/O/shell/cat/wc/LVGL; test:shell-pipelines,
  test:shell-lvgl, test:shell-boot and npm test pass. Record artifact hash/size
  and measured memory; update affected progress trackers and Git-scope audit.

Interactive browser, MaiaWASMVM and native-device acceptance remain separate
unrun gates unless actually performed. No broader POSIX conformance is implied.

## Read-only preparation evidence

/bin/dash returned the table above. With N unset, `set -- ${N+""}` produced0
arguments; with N set empty, the same command produced1 argument. No MaiaOS
implementation is represented by these host oracle probes.

Current generated main JS rejects `echo ${N+x}` and `echo ${N:+x}`, while accepting
`echo "${N:+$OTHER}"` structurally. Current word JS gives OpaqueBraced ownership
to `"${N:+$OTHER}"` and `"${N:+${#OTHER}}"`, without inner NamedParameter or
ParameterLength nodes. Existing tests explicitly encode this prior exclusion;
replace expectations only for newly approved forms and retain negative coverage
for excluded nesting/other operators. No generator or semantic source was edited.

Production shell.wasm remains1,529,480bytes, SHA-256
fbb18ff248c6c6fc694ee294fb9e84b694f07fe08b1b65779b74ea3a89fabed2.
Only this proposal, the pending shell checklist row and handoff are changed by
preparation. No new build/full suite or interactive browser/VM/device run.


## Implementation and verification — 2026-10-30

Both EBNFs use the existing finite word/quote productions. Generated operator
nodes distinguish AlternateSetOperator and AlternateNonemptyOperator. PLUS is
removed from RAW and preserved in literal/escape/opaque productions, without
broadening DoubleEscapedCharacter. Existing BRACED_DOLLAR/SUBSTITUTION_DOLLAR
guards and complete opaque ownership prevent partial unsupported expansion.

The private renderer checks membership and, for colon-plus only, the first stored
byte. It skips the complete alternative subtree when unselected and traverses
it in the existing fixed loop when selected. Selection never implicitly emits
the subject value; explicit references within the selected word still expand
normally, including references to NAME itself.
Existing defaults keep their prior selection/emit path. Static validation still
checks all semantic names and literal bounds without lookup, while ordinary
BareName words retain their independent 95-byte limit. No recursive rendering,
source scanning, substring reparse, new syscall or execution plan.

All final commands exited 0:

```sh
npm run build:shell -- --check-c89
npm run test:shell-grammar
npm run test:shell-parser
npm run test:shell-equivalence
npm run test:shell-word
npm run test:shell-fields
npm run test:shell-variables
npm run test:shell-status
npm run test:shell-lists
npm run test:shell-scripts
npm run test:shell-redirection
npm run test:shell-glob
npm run test:shell-pipelines
npm run test:shell-lvgl
npm run test:shell-boot
npm test
```

- Syntax: 146 native fixtures, including two prior contextual POSIX gaps;
  168 raw JS/native cases and 27 recursion profiles without rule/position cycles.
- Main frontend: 6,190 raw JS/native C++98/WASM inputs; 3,087 accepted,
  2,283 syntax errors and 820 lexical errors; exact tokens, UTF-16 spans,
  complete trees and restored guest stack.
- Word frontend: 473 JS/native/WASM trees, 193 regular byte/mask expectations,
  seeded parameter/assignment probes, 137 independent native-shell/WASM field
  cases, 16 Unicode counts and 17 malformed UTF-8 vectors; ASan/UBSan,
  decoder guards, exact limits and six byte-reproduced generated artifacts.
  Main JS/C++98/XML also reproduce; .cpp equals the production generated header.
- Finite profiles at 1/4/16/32 repeated forms: all three operators stay at
  depth 9 with zero cycles. Existing colon-minus calls: 49/133/469/917;
  plus: 52/145/517/1013; colon-plus: 51/141/501/981.
- Actual shell guest: independent unset/empty/nonempty and native oracles;
  selected/skipped empty anchors, quoted/unquoted fields/glob, escaped plus,
  unsplit/ambiguous redirects, empty commands/stages, all fixed bounds, full
  32-slot table without mutation/allocation, lazy lists/status/negation and
  dot/caller persistence. 100 stable alternate scripts pass alongside prior
  100 default, 100 length, 200 braced and 400 field cycles.
- An isolated strict-C89/WASM guest proves malformed nonempty subject bytes can
  select a valid word without being decoded/emitted; unselected length never
  decodes, while selected invalid length completes normally with status 2 and
  preserves assignment/file state, original bytes and session recovery. This
  uses only the existing test-only --test-parameter-values build. Production
  exports contain no injection API; temporary test outputs were removed.
- Real KEY/kernel/I/O/shell/cat/wc/LVGL: five recipes now contain 80 commands,
  executed twice per-line and all five files twice by guest dot. Recipe03 still
  aborts at its deliberate line4 syntax error. Worker plans, trap/cancel/reap,
  backpressure, 128 KiB partial transfers and resource cleanup pass: 493 flushes.
- LVGL host: 1,000 parses, 77,911 kernel steps, stable shell 20 / I/O 2 pages,
  2,171 flushes. Boot: 25 sessions, zero surviving windows/endpoints, 28 flushes.
- npm test: 66 pass. VFS guide is 4,030 bytes (4,065 with command echo);
  recipe05 expected text is 3,749 bytes (3,796 with echo). Existing 4,096-byte
  display limit and its image unit check remain intact.

Production shell.wasm: 1,534,819 bytes; SHA-256:
`20e9c8e2333f37e3749674e081e86bd44ecff2854dbca7937a24773b6c8c48f8`.

### Generation comparison

Both exact task-start EBNFs were preserved in /tmp/maiaos-alternates-baseline
before edits; their hashes match /tmp/maiaos-alternates-before.json. Own tREx,
one observation per backend under concurrent build load:

| Frontend/backend | Default checkpoint bytes/time | Alternate bytes/time |
|---|---|---|
| Main JavaScript | 71,445 / 2.049 s | 71,473 / 2.217 s |
| Main C++98 | 133,784 / 2.052 s | 134,124 / 2.117 s |
| Word JavaScript | 119,534 / 3.329 s | 124,609 / 3.728 s |
| Word C++98 | 170,742 / 3.410 s | 178,489 / 3.823 s |

These measurements describe generation cost and artifact growth, not runtime
throughput guarantees. Flat rule depth, actual guest stack/memory and resource
checks remain independent evidence. No budget or timeout was raised.

### Git state and remaining limits

MaiaOS main b13f168 remains dirty/local with all previous work preserved. Index
and Cpp6aa8eac/Glob48ad753/Regexe2ae3a8 staged pins match task start; all seven
compiler/library principals remain clean. Kernel/I/O sources and binaries,
shell.cpp, AGENTS, package/version and public process/stream/VFS interfaces are
unchanged. Progress READMEs, architecture and prior-contract follow-ups updated.
No commit/push, npm, version or history operation. Logs:
/tmp/maiaos-alternates-*.log. Final jobs completed successfully.

Default/alternate nesting, other operators/substitution, interactive browser,
MaiaWASMVM/device and standalone kernel-policy/I/O/font gates remain excluded or
unrun as stated. Reload the simulator and run `. /system/tests/05-status.sh`
for the seven new examples; automated LVGL tests do not imply interactive review.

Suffix-removal follow-up (2026-10-30): named % / %% are separately approved and
validated; see [contract/evidence](SHELL_PARAMETER_SUFFIX_REMOVAL.md). Existing
semantics/regressions pass; operator nesting remains excluded. Root changes are
local, with published dependencies and no root commit/push/npm/version change.
