# Named parameter prefix removal — approved contract

Date: 2026-10-30. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the locally validated suffix checkpoint and published
Cpp8475812 / Glob703dda2 dependencies. Previous removal approval excludes prefix
operators. This proposal grants neither publication nor changes in other owners.

## Selected semantics

Add only `${NAME#pattern}` and `${NAME##pattern}` in ordinary words, enclosing
double quotes, assignment RHS and existing redirection targets. Remove the
shortest/longest whole matching prefix respectively; retain the remaining suffix.
A valid pattern with no match leaves the subject unchanged. Empty patterns leave
it unchanged; unset/empty subjects produce empty. Success does not mutate a
binding, allocate a variable slot or independently change completed-unit status.

| Value | Expansion | Result |
|---|---|---|
| arquivo.tar.gz | `${N#*.}` | tar.gz |
| arquivo.tar.gz | `${N##*.}` | gz |
| arquivo.tar.gz | `${N#.zip}` | arquivo.tar.gz |
| abc | `${N#}` | abc |
| /a/.b | `${N##*}` | empty |
| 😄ação | `${N#?}` | ação |

Reuse the published MaiaGlob compiler/matcher, string flags zero, and existing
scalar `*`, `?`, bracket/range profile. Slash and leading dot are ordinary scalars;
matching does not enumerate the VFS. Preserve the current invalid-pattern policy,
including invalid/reversed sets, slash in sets, trailing unprotected backslash and
unsupported POSIX named/collation/equivalence classes. No locale normalization or
case folding. Invalid selected patterns or subject UTF-8 fail normally with status
2 before effects, including when an empty candidate would otherwise match.

Reuse finite PatternValue: literal text, existing pattern quotes/escapes and
simple named/status/length references. Expansion bytes are data, never reparsed.
No splitting or filename walking while constructing a pattern. Inner pattern
quotes protect metacharacters; enclosing double quotes protect the final result
only. Thus `"${N#*a}"` uses a wildcard, while `"${N#"*a"}"` uses literal *a.
Pattern-local empty quotes do not anchor an output field. Retained unquoted data
uses existing split provenance then per-field glob; outer quotes preserve one
protected field, including empty. Assignments and redirects remain unsplit.

Use one incoming selected-unit binding/status snapshot. Static preflight validates
syntax and name/literal limits without lookup, compilation, decoding or matching;
skipped AND-OR units perform none of those runtime operations. Errors participate
in existing lists, negation and cleanup without assignment, target-file mutation
or worker launch. Preserve dot/caller state and pipeline stage ownership.

Exclude operators on special/status/positional parameters, all operator nesting,
substitution and assignment/error operators. Supported adjacent expansions remain
valid. Unsupported complete quoted forms retain whole opaque ownership, without
inner expansion; unsupported unquoted forms reject before effects. Existing
`${#NAME}` length, `%`/`%%`, defaults and alternates retain their semantics.

## Generated syntax and bounded implementation

Extend both authoritative EBNFs and regenerate only through in-repository
MaiaCC/tREx. Distinguish HASH-before-name length from HASH-after-name removal by
generated nodes, including longest-operator ownership. Preserve literal/escaped
hash, comments, quotes, fallback and opaque contexts; do not introduce a handwritten
scanner, substring parser, recursive renderer or patch generated sources. Edit
exact named productions to avoid confusing similarly named quote/escape rules.

Reuse iterative pattern rendering and independent generated UTF-8 decoder scratch.
Validate the complete subject/pattern and collect at most96 scalar byte boundaries,
including zero/end, without slicing a scalar or surrogate pair. Compile once per
selected removal, with independent bounded program/range/NFA storage and a loan
of the idle glob compiler workspace. Keep filename-walk program lifetimes intact.
No per-removal heap buffers; existing generated compiler allocations remain bounded.

For shortest removal try whole prefixes at ascending scalar boundaries; longest
tries descending. On a match emit subject[boundary..end]; no-match emits the full
subject. At most96 candidates share one1048576 matcher-step allowance. Subtract
actual work after each call; zero remaining budget is LIMIT, not NOMATCH. A small
private direction parameter/helper is permitted to share existing suffix storage
and matching logic; preserve suffix ordering and exact-budget behavior. No broad
refactor or owner API change.

Keep line255bytes, names31bytes, words/values/patterns95bytes,32variables,
12original/final slots including redirects, current tokens/trees, shell32pages /
four handles and I/O4096byte display. The current image guide plus command echo
uses4089bytes: condense/reorganize recipes and guide rather than enlarge storage.
Add bounded prefix examples and exact expected bytes to the immutable VFS image.

## Scope and acceptance gates

MaiaOS only: both EBNFs/artifacts, private word/pattern helpers, JS/native/WASM
probes, guest regressions, VFS recipes, reproducible shell.wasm and affected
progress/evidence documents. Preserve all pre-existing edits and staged gitlinks.
No kernel/I/O production, ABI/syscall, manifest, process-plan, compiler/library
owner, dependency-pointer, version, npm, commit/push or history change.
A separately discovered owner defect needs a minimal reproduction and approval.

- [x] Independent JS expected syntax/trees first, especially length-versus-removal,
  shortest/longest, hash literals/comments/escapes, inner/outer quotes and opaque
  nesting; raw native C++98 then own-chain strict C89/WASM token/span/tree parity.
- [x] Fixed bytes plus dash/Bash comparisons: empty/unset/no match, slash/dot,
  bracket sets, escapes, simple/status/length pattern references, adjacent forms,
  supplementary and combining Unicode. Use scalar-aware Bash/fixed expectations
  for Unicode rather than this dash build's byte-oriented wildcard behavior.
- [x] Native ASan/UBSan and actual WASM masks/split/anchors, complete malformed
  UTF-8 validation, invalid patterns, scratch guards, exact aggregate budgets,
  exhaustion/recovery and restored stack. Retain suffix exact-budget regression.
- [x] Full guest name31/32, byte95/96, slot12/13, full32-variable table, no implicit
  mutation/slots, static/runtime atomicity, skipped work, empty commands/stages,
  unsplit redirects, no matching VFS access, lists/negation/pipelines/dot/caller.
- [x] Flat1/4/16/32 recursion/work profiles and generation sizes/times against exact
  pre-prefix grammars; reproduce artifacts without raising limits/timeouts.
- [x] `npm run build:shell -- --check-c89`; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields, test:shell-variables,
  test:shell-status, test:shell-lists, test:shell-scripts, test:shell-redirection,
  test:shell-glob, test:shell-pipelines, test:shell-lvgl, test:shell-boot; npm test.
- [x] Real KEY/kernel/I/O/shell/cat/wc/LVGL recipes twice per-line and via guest dot,
  bounded display assets including echo, partial transfers and stable resources;
  isolate malformed-value guest hooks and preserve production exports/hash.
- [x] Update affected checklists/evidence/handoff and audit scope/index/artifacts.

Interactive browser, MaiaWASMVM and device gates are separate, not implied by
reference-engine automation. Kernel/I/O checklists change only if affected.

## Preparation evidence — not implementation

Read current architecture, EBNFs, matcher, probes and completed suffix contract.
32 independent fixed-result comparisons pass:14 dash /18 UTF-8 Bash, covering
shortest/longest, no-match, empty, literal versus wildcard, escapes, ranges,
references, slash/dot, supplementary and combining scalars. Current generated JS
still owns both quoted prefix examples as opaque, with no inner named expansion;
`${#NAME}` retains its generated ParameterLength node (3 ownership checks).
These are read-only baseline checks, not prefix runtime or new WASM validation.

Preparation changes only this proposal, one unchecked shell checklist row and
handoff. Production shell.wasm remains1566724bytes, SHA256
04d9e465e4209bdf11e7be93ab11036e90b05e6324d769681aebc9f0d0a74641.
No source/build/full-suite/publication action was performed during preparation.

## Implementation and verification — 2026-10-30

The approved extension is implemented locally. Both authoritative EBNFs generate
prefix nodes with distinct shortest/longest operators; HASH before the name still
owns length. Pattern rendering stays iterative and reuses inner-quote masks and
generated UTF-8 boundaries. The private direction-aware MaiaGlob matcher compiles
once, uses string flags zero, and tries whole prefixes in the required order.
The emitted suffix is parameter data for existing field/glob/redirect paths.
No VFS enumeration during matching, manual scanner, substring reparse, recursive
renderer, owner change, new syscall or host command interpretation is introduced.
Real guest I/O and kernel scheduling retain LVGL ownership and resource cleanup.

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
npm run test:shell-lists
npm run test:shell-scripts
npm run test:shell-redirection
npm run test:shell-glob
npm run test:shell-pipelines
npm run test:shell-lvgl
npm run test:shell-boot
npm test
```

- Main syntax:163 native fixtures, including2 pre-existing contextual POSIX gaps;
  185 raw JS/native cases and27 generated recursion profiles pass without cycles.
- Main JS/native C++98/own-chain WASM:6426 raw inputs,3243 accepted /2353 syntax
  errors /830 lexical errors; exact tokens/UTF16 spans/complete trees/restored
  guest stack. Main JS/C++98/XML/header artifacts reproduce byte-for-byte.
- Word:710 JS/native/WASM trees;327 independent regular byte/mask cases,
  252 field expectations including67 new prefix vectors,16 scalar counts and
  17 malformed byte vectors. Native ASan/UBSan, strict emitted C89, decoder guards,
  restored WASM stack and all6 generated word artifacts reproduce/pass.
- Prefix budget:abcdef against z* completes with119 aggregate matcher steps;
  exact quota succeeds, one-less/1/0 exhaust exactly at their quotas, and full
  quota reuse recovers in native and actual WASM. Suffix119-step regression
  remains unchanged. Complete invalid selected subject/pattern UTF-8 is rejected;
  static/skipped forms avoid evaluation, preserving source/stored bytes.
- Fixed expectations additionally agree with native shells: preparation32 cases
  (14dash/18Bash) plus supported prefix field cases with UTF8 Bash. This dash
  build has byte-oriented Unicode wildcard behavior; scalar expectations are
  preserved. New cases cover empty ##, ### with a literal hash pattern, escaped
  hash, inner/outer pattern quotes, braces/length ownership and cross-operator
  nesting remaining wholly opaque when quoted.
- Flat1/4/16/32 prefix fragments keep depth9, no cycles. Shortest calls
  52/139/487/951; longest51/135/471/919. Other choice/removal profiles stay linear;
  no limit, timeout or threshold increased.
- Full shell:quotes/fields/glob, 31/32names,95/96bytes,12/13slots, full32-slot
  table, no implicit mutation/slot, static/runtime errors before effects,
  unsplit/ambiguous redirects, empty commands/stages, no matching VFS access,
  lists/lazy selection/negation and dot/caller state.100prefix scripts remain
  stable alongside100suffix/100alternate/100default/100length/200braced/400field
  cycles. Previous variables/status/lists/scripts/redirection/glob tests pass.
- An isolated own-chain full guest injects four malformed UTF8 values for tests
  only. Selected prefix subjects/patterns fail normally with2, skipped work does
  not decode, assignments/files/pipeline launch remain atomic and session/source
  bytes recover. Production exports lack injection APIs; temporary outputs are
  removed and production artifact hash remains unchanged.
- Real KEY/kernel/I/O/shell/cat/wc/LVGL:7 immutable recipes /108commands twice
  per-line and all7files twice via guest dot. Recipe03 retains intentional syntax
  abort at line4.590pipeline flushes;128KiB partial transfers, input/output errors,
  traps/cancel/rollback/reaping pass. Host1000parses/77911kernel steps/2219flushes;
  boot25sessions/28flushes, no surviving windows/endpoints. Unit66pass.
- Stable memory is now shell21pages (previous20), I/O2. Existing shell32-page /
  four-handle and I/O8-page limits are unchanged. Binary:1587368bytes, SHA256
  60c27e3cd2d4f6447cbaba16a536f65272acca316223b224332eec969f881376. No memory/performance claim for VM/device is implied.
- Seventh recipe07-prefix has14commands. Guide+echo4083bytes, script+echo598 and
  expected+echo791 all fit unchanged4096display. Existing guide was condensed;
  no terminal or syntax bound was raised. Reopen the simulator and run
  `. /system/tests/07-prefix.sh` for interactive review.

### Large-probe diagnostic serialization

The first expanded budget-probe build hit the known MaiaC diagnostic JSON string
size limit (RangeError: Invalid string length). It still requested default debug
serialization. The final probe explicitly uses the already published
serializeParseTrees:false API, as the production/full isolated builds do, and
asserts complete AST/collector identity and null diagnostic representations.
No owner source, grammar expectation, timeout or functional test was weakened.
The initial smaller prefix word run passed703trees/247fields; those are an early
checkpoint, not the final acceptance counts above. The failed expanded run is
kept in temporary logs; final complete command passes with all assertions.

### Generation comparison and Git state

Exact pre-prefix EBNFs were retained. Times below are one host observation under
concurrent compilation, not a throughput guarantee:

| Artifact | Suffix baseline bytes /seconds | Prefix bytes /seconds |
|---|---|---|
|Main JS|73621 /2.379|75769 /2.577|
|Main C++98|158729 /2.447|183310 /2.702|
|Word JS|133688 /4.625|139011 /4.794|
|Word C++98|190358 /4.923|196136 /4.849|

Root remains main b13f168 with existing local work preserved. Index/gitlinks are
unchanged, including prior staged Cpp8475812 /Glob703dda2 /Regexe2ae3a8. All7
compiler/library principal repositories remain clean at their published baseline
commits. No commit/push, versions, npm or history action. Kernel/I/O production,
ABI/plans/manifests/fonts and unrelated work remain unchanged. Interactive
browser, MaiaWASMVM/device and standalone kernel native/I/O/font gates were not
run. Nested operators and broader POSIX execution remain excluded.

Final scope audit:35 justified root paths versus the pre-prefix checkpoint;
complete index unchanged, including all three prior staged gitlinks. Protected
production files and all pre-existing work preserved; all7owner principals clean.
Final main/word generated artifacts reproduce after isolated guest validation;
git diff --check passes. No root publication.
