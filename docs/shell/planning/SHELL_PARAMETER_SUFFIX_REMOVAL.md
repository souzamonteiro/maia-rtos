# Named parameter suffix removal — approved contract

Date: 2026-10-30. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the validated local named-alternate checkpoint.
Earlier parameter contracts exclude removal operators. This document proposes
exactly the bounded extension below. The owner explicitly approved its contract
and implementation; publication remains excluded.

## Selected semantics

Add `${NAME%pattern}` and `${NAME%%pattern}` in ordinary words, double quotes,
assignment RHS values and existing redirection targets. `%` removes the shortest
matching suffix; `%%` removes the longest. A valid pattern with no matching suffix
leaves the value unchanged. Unset/empty subjects yield empty. No assignment,
variable-slot allocation or status change occurs on success.

| Value | Expansion | Result |
|---|---|---|
| arquivo.tar.gz | `${N%.*}` | arquivo.tar |
| arquivo.tar.gz | `${N%%.*}` | arquivo |
| arquivo.tar.gz | `${N%.zip}` | arquivo.tar.gz |
| arquivo.tar.gz | `${N%}` | arquivo.tar.gz |
| /a/.b | `${N%%*}` | empty |
| ação😄 | `${N%?}` | ação |

Patterns use the existing MaiaGlob scalar profile: `*`, `?`, bracket sets/ranges
and quote protection, without normalization, locale collation or case folding.
Matching is string matching: slash and leading dot are ordinary subject scalars.
Compile with the published compiler, then set the public program flags to zero
for matching. Do not call directory walkers or VFS APIs.

Retain MaiaGlob's explicit invalid-pattern policy, including unclosed/reversed
sets, trailing unprotected backslash, literal slash as a set element or range endpoint, and
unsupported named POSIX/collation/equivalence classes. This is a bounded project
profile, not a claim of complete POSIX pattern support. Literal slash outside
sets is supported, and wildcards may consume it with flags zero. Do not change
MaiaGlob's owner sources or public API to broaden this increment.

The pattern word may be empty and may contain literal text, existing appropriate
quote/escape forms and simple named/status/length references. Inserted parameter
bytes are data, never reparsed as shell syntax. Their unquoted metacharacters may
participate in pattern matching; their shell quote provenance protects them when
quoted inside the pattern. No field splitting or filename enumeration occurs
while constructing the pattern. A pattern's temporary empty quotes never create
an output field by themselves.

Enclosing double quotes protect the resulting expansion, not pattern operators:
`"${N%.*}"` uses a wildcard. Quoting within the pattern protects its operators:
`"${N%".*"}"` uses a literal suffix. Derive these two distinct protection domains
from generated nodes; do not reuse the current ancestor-wide output protection
unchanged for pattern bytes. Preserve native comparisons for escapes and inner
quotes, including the existing finite double-quote escape profile.

The retained prefix is parameter-expansion data: unquoted output receives the
existing split provenance and then per-field MaiaGlob; quoted output stays one
protected field, including empty. Assignment RHS and redirection targets remain
unsplit under existing ambiguity rules. Use the incoming selected unit's status
and binding snapshot. Selected runtime errors follow ordinary status 2,
negation/list selection and resource cleanup before assignment, file opens or
worker launch; successful removal never mutates its subject.

This increment excludes `%`/`%%` on status/positional/special parameters, prefix
removal `#`/`##`, substitution, assignment/error operators and all operator nesting.
Adjacent supported expansions remain valid. Neither defaults/alternates/removal
inside the pattern nor removal inside a default/alternate word becomes supported.
Excluded complete quoted forms retain whole opaque ownership without inner
expansion; excluded unquoted forms reject before effects.

## Generated syntax and bounded evaluation

Extend main and word EBNFs and regenerate only through MaiaOS's own MaiaCC/tREx.
Generated nodes distinguish shortest/longest operators and pattern quote domains.
Preserve literal/escaped percent in ordinary, single-quoted, double-quoted,
fallback and opaque contexts if a dedicated percent token is introduced. Preserve
existing contextual dollar guards, Unicode offsets and main-word ownership.
No handwritten syntax scanner, substring parse, recursive renderer or generated
source patch. Traverse finite pattern descendants iteratively without resetting
the active word tree, its tokens, rendered output or empty anchors.

Construct at most 95 pattern bytes with a separate fixed quote mask. Validate
the subject's complete UTF-8 with the existing generated decoder, collecting at
most 96 scalar-boundary offsets in independent scratch. Never slice within a
multibyte scalar or UTF-16 surrogate pair. Invalid selected subject/pattern data
is an ordinary unit error, even when an empty suffix would otherwise suffice.
Skipped AND-OR units perform no binding lookup, compilation, decoding or matching.
Whole-line static validation still checks generated syntax, semantic name/literal
bounds and excluded forms without evaluating bindings or patterns.

Compile each selected pattern once using the published MaiaGlob compiler. Try
at most 96 whole-suffix candidates, including empty: from the end toward zero
for `%`, and from zero toward the end for `%%`, stopping at the first match.
No recursive star backtracking. Two independent fixed NFA state arrays and
instruction/range storage must not alias subject, pattern or word-tree scratch.
Loan the already initialized compiler workspace only while no filename walk is
active; preserve compiled program lifetimes and recompile filename patterns
normally afterward. Native/WASM word probes must supply equivalent private
storage. No per-removal heap storage; the generated compiler retains its existing
bounded lexer allocation behavior.

Use one aggregate ceiling of 1,048,576 matcher steps per selected removal, not
that ceiling anew for every candidate. The API resets workspace.steps each call;
subtract actual steps and pass only the remaining budget to each invocation.
Zero remaining budget is LIMIT, not NOMATCH. Keep compile/input limits separate
and explicit. Validate exact budgets, exhaustion and recovery independently.

## Scope and unchanged boundaries

MaiaOS only: both EBNFs and their reproducible artifacts; private word rendering
and pattern-matching helpers/storage; native/JavaScript/WASM probes and guest
regressions; bounded simulator VFS recipes/expected responses; shell.wasm;
affected progress READMEs, architecture, prior-contract follow-ups and handoff.

Keep line 255 bytes, names 31 bytes, rendered words/values/patterns 95 bytes,
32 variables, 12 original/final slots including redirects, current token/tree
limits, shell manifest 32 pages/four handles and I/O display 4096 bytes. Guides,
recipes and expected output including command echo must pass the existing size
gate. Reorganize/condense fixtures rather than enlarge terminal storage.
No syscall/import, process-plan, kernel/I/O production, compiler-owner, version
or dependency-pointer change. Preserve existing work and all three staged
Cpp/Glob/Regex pins. No commit/push, npm or history rewrite. An owner defect needs
a minimal reproduction and separately authorized owner correction/propagation.

## Acceptance gates — pending implementation

- [x] Independent expected results first: JS main/word syntax, complete operator
  and quote ownership, opaque exclusions; raw native C++98 and own-chain strict
  C89/WASM token/UTF-16 span/tree parity and artifact reproduction.
- [x] dash/Bash comparisons for shortest/longest, no match, empty pattern/value,
  unset names, slash/dot, classes, Unicode/supplementary/combining scalars, inner
  versus enclosing quotes, escapes, pattern references and adjacent expansions.
- [x] C++98 ASan/UBSan and actual WASM byte/mask/split/anchor expectations,
  invalid UTF-8 and invalid patterns, exact matcher budgets, scratch guards,
  restored stack and no compilation/decoding for skipped units.
- [x] 31/32 names, 95/96 pattern/result bounds, 12/13 slots, full variable table,
  atomic assignment/redirection errors, empty commands/stages, lists/negation,
  pipelines, dot/caller state and no VFS access during pattern matching.
- [x] Flat parser growth/recursion profiles and generation size/time against the
  exact alternate checkpoint, without raised limits/timeouts.
- [x] `npm run build:shell -- --check-c89`; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields,
  test:shell-variables, test:shell-status, test:shell-lists,
  test:shell-scripts, test:shell-redirection and test:shell-glob.
- [x] VFS recipes twice per-line and twice via guest dot through actual
  KEY/kernel/I/O/shell/cat/wc/LVGL; test:shell-pipelines, test:shell-lvgl,
  test:shell-boot, npm test, stable memory/resources and artifact hash/size.
- [x] Update affected progress trackers and audit scope, index and owner state.

Interactive browser, MaiaWASMVM and native-device acceptance are separate gates;
do not report them as performed from reference-engine or host tests.

## Read-only preparation evidence

A temporary strict C89/C++98 probe compiled the published MaiaGlob runtime,
Regex and generated pattern compiler directly, without writing in a repository.
Seven assertions passed: flags-zero `*` consumes `/a/.b`, `?` consumes one emoji,
dot/literal-class matching works, slash in a class remains INVALID, a protected
star is literal, and empty pattern matches empty input. This is native library
evidence only, not MaiaOS syntax/runtime implementation or new WASM validation.
`/bin/dash` and `/bin/bash` independently returned arquivo.tar / arquivo for the
two removal lengths, unchanged subject for quoted `.*` and escaped `*`, and
empty for longest-star on /a/.b and for an unset subject.

Production shell.wasm stays 1,534,819 bytes, SHA-256
20e9c8e2333f37e3749674e081e86bd44ecff2854dbca7937a24773b6c8c48f8.
Only this proposal, the unchecked shell checklist row and handoff are changed
by preparation. No source implementation, build/full-suite rerun or publication.

## Implementation and verification — 2026-10-30

Both generated frontends recognize finite named suffix operators and pattern
quote ownership. The private iterative renderer distinguishes inner pattern
protection from enclosing result protection, derives scalar byte boundaries
through the generated decoder and retains the subject without mutation. The
published MaiaGlob compiler runs once per selected expansion; flags zero enable
whole-string matching, with independent fixed program/NFA buffers and a loan of
the idle initialized compiler workspace. At most96 candidates share one1048576
step ceiling. No recursion, source scanner, substring reparse, filename walk,
new syscall or process plan is used for suffix matching.

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

- Syntax:154 native fixtures including2 prior contextual POSIX gaps;176 raw
  JS/native cases and27 recursion profiles without cycles.
- Main JS/native C++98/own-chain strict-C89/WASM:6297 inputs,3159 accepted /
  2313 syntax errors /825 lexical errors, exact tokens/UTF16 spans/complete
  trees and restored guest stack. Main JS/C++98/XML snapshots reproduce; cpp
  equals the production header. Main grammar did not change during quote repair.
- Word:651 JS/native/WASM trees,327 independent regular byte/mask expectations,
  185 independent field cases including48 suffix vectors;16 scalar lengths and
  17 malformed byte vectors. Native ASan/UBSan, decoder guards, exact limits,
  restored WASM stack and all6 generated artifacts reproduce/pass.
- Budget probe: abcdef against *z uses119 aggregate steps.119 succeeds;
  118/1/0 fail exactly at their quotas, followed by successful reuse. Native
  and WASM agree. Invalid selected UTF8 is rejected; static/skipped units avoid
  binding evaluation/compilation. Source and stored-value bytes are preserved.
- Independent extra suffix oracles:37 ASCII dash and42 UTF8 Bash comparisons.
  This dash build is byte-oriented for Unicode wildcards; fixed scalar expected
  results and UTF8 Bash are the Unicode oracle, without weakening expectations.
- Flat1/4/16/32 forms all peak at depth9, no cycles. Shortest calls
  51/138/486/950; longest50/134/470/918. Existing default55/154/550/1078,
  plus58/166/598/1174 and colon-plus57/162/582/1142 remain linear.
- Full shell: inner/enclosing quotes, escapes, fields/glob, no VFS access during
  matching, unsplit/ambiguous redirects, no subject mutation or implicit slots,
  31/32 names,95/96 words/patterns,12/13 slots, full32-slot table, static/runtime
  atomicity, empty commands/stages, lists/negation and caller/dot persistence.
  100 suffix scripts remain stable alongside400 field/200 braced/100 length/
  100 default/100 alternate cycles. All previous guest regression suites pass.
- A separate strict-C89/WASM guest injects malformed values only for tests:
  selected suffix subject/pattern failures complete normally with status2,
  skipped branches do not decode, and files/assignments/original bytes recover.
  Production exports have no injection API; temporary test outputs are removed
  and production hash is unchanged by that build.
- KEY/kernel/I/O/shell/cat/wc/LVGL:6 immutable recipes /94 commands twice per-line
  and all6 files twice via guest dot; recipe03 retains its deliberate line4
  syntax abort.566pipeline flushes, actual worker argv/paths, partial128KiB
  transfers, error/trap/cancel/reap and rollback pass.
- Host LVGL:2195flushes,1000parses/77911kernel steps, stable shell20/I/O2 pages.
  Boot:25sessions/28flushes, zero surviving windows/endpoints. Unit tests66pass.
- Sixth recipe06-suffix has14commands. Guide+echo4089bytes, script+echo586bytes
  and expected+echo791bytes fit the unchanged4096byte display; no bound raised.

### Integration findings and dependency correction

The first complete build hit MaiaC's eager diagnostic JSON string-size limit.
The separately approved [owner correction](MAIAC_OPTIONAL_PARSE_SERIALIZATION.md)
is published and propagated: Cbd19cac, Cpp8475812, JS49430f5, Glob703dda2.
The root build explicitly opts out of diagnostic JSON/XML through that API,
retaining/asserting the complete AST. No nested or sibling compiler patch.

Initial full-shell tests also caught an EBNF edit assigning percent to similarly
named rules rather than the exact DoubleLiteral/EscapedCharacter rules. Corrected
only the authoritative grammar and regenerated.127 double-quoted ASCII and7
explicit percent/format/escape/opaque/fallback expectations now prevent that
regression. Existing printf/variable expectations were preserved. A status-pattern
fixture explicitly supplies incoming127, matching its unchanged native oracle.

Exact pre-edit grammars were retained. One generation observation under concurrent
validation, not a throughput guarantee:

| Artifact | Alternate baseline bytes / seconds | Final suffix bytes / seconds |
|---|---|---|
| Main JS |71473 /2.275|73621 /2.288|
| Main C++98 |134124 /2.245|158729 /2.290|
| Word JS |124609 /3.724|133688 /4.325|
| Word C++98 |178489 /3.751|190358 /4.267|

### Final artifact, Git and remaining boundaries

shell.wasm:1566724bytes, SHA256
04d9e465e4209bdf11e7be93ab11036e90b05e6324d769681aebc9f0d0a74641.
Root main b13f168 remains local/unpublished with all prior dirty work preserved.
Cpp/Glob index pointers now consume their published commits; prior staged Regex
is unchanged.34recursive gitlinks across6 intended remotes verified published.
Compiler/library principals are clean; versions/npm are unchanged. Kernel/I/O
production, manifest budgets, ABI and process plans remain unchanged.

Prefix removal, nested operators, substitution and complete POSIX conformance
remain excluded. Interactive browser, MaiaWASMVM/device and standalone kernel
native-policy/I/O/font reruns were not performed; automated real LVGL/reference
execution is the evidence here. Reopen the simulator session and run
`. /system/tests/06-suffix.sh` for interactive review.

Final scope audit:42 justified root paths relative to the pre-suffix checkpoint;
only the authorized Cpp/Glob index entries advanced, prior Regex staging retained.
All seven compiler/library principals are clean at their intended commits.
AGENTS/package/gitmodules/kernel/I/O production/manifest and pre-existing work
are preserved. Final main/word JS/C++98/XML artifacts still reproduce byte-for-byte
after the isolated guest build; git diff --check passes. No root publication.

## Subsequent prefix increment — 2026-10-30

The separately approved [prefix contract](SHELL_PARAMETER_PREFIX_REMOVAL.md) now
implements #/## while preserving this suffix behavior and its119-step regression.
Its full acceptance passed, with unchanged owner commits/index and kernel/I/O
production. Current shell memory is21pages (within32), replacing the20-page
measurement above; that earlier result remains the suffix checkpoint evidence.
