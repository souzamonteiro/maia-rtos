# Named parameter length — approved contract

Date: 2026-10-27. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the locally validated simple-braced checkpoint.
The previous SHELL_BRACED_PARAMETERS.md explicitly excluded length operators.
Owner authorization covers implementation and validation, without publication.

Follow-up (2026-10-28): [MAIACPP_LOCAL_STRUCT_TYPES.md](MAIACPP_LOCAL_STRUCT_TYPES.md)
is approved, implemented, published and consumed. It resolves the separate local
plain-struct finding below in the AST-strict path; the test guard now again uses
its original function-local Frame. The prior parameter-length evidence remains
historical, with the selected length behavior/bounds unchanged.

Follow-up (2026-10-30): [SHELL_PARAMETER_DEFAULTS.md](SHELL_PARAMETER_DEFAULTS.md)
is separately approved, implemented and validated locally for exactly
`${NAME-word}` / `${NAME:-word}`, including simple/status/length references in
the fallback. It supersedes only those default exclusions in this historical
checkpoint. Nested defaults and other operators remain excluded as specified.

Follow-up (2026-10-30): [SHELL_PARAMETER_ALTERNATES.md](SHELL_PARAMETER_ALTERNATES.md)
is separately approved and implemented/validated locally for exactly
`${NAME+word}` / `${NAME:+word}`. It supersedes only the alternate-operator
exclusion in this historical checkpoint. Mixed/default/alternate nesting and
other operators remain outside the selected scope.

## Selected semantics

Add exactly `${#NAME}` to ordinary words, double quotes, assignment RHS values
and existing redirect targets. NAME follows the existing generated ASCII rule
and31-byte bound. Render the bound value's Unicode scalar count in unsigned
ASCII decimal; unset/empty values produce `0`. Stored values remain limited to
95bytes, so the maximum supported count is95. Counting never changes bindings.

Examples with independently specified results:

```sh
NAME=ação; echo "${#NAME}"           # 4
NAME='ação 😄'; echo ${#NAME}        # 6 (11 UTF-8 bytes)
NAME=; echo "size=${#NAME}"         # size=0
unset NAME; LENGTH=${#NAME}; echo "$LENGTH" # 0
```

The count is fixed UTF-8 scalar semantics, independent of locale variables.
Precomposed ã counts1; a followed by combining tilde counts2; a supplementary
scalar counts1 even though its generated transport uses two UTF-16 units.
Grapheme clusters, terminal cells and byte counts are different measures and are
not implemented by this operation. No normalization, locale engine or font change.
An ordinary local LC_ALL/LC_CTYPE does not alter the count.

Single quotes and escaped dollars stay literal. Decimal result bytes inherit
existing quote protection and ordinary expansion provenance. Compose with literal
prefixes/suffixes, existing parameter expansion, fixed fields and MaiaGlob using
current stage/redirect ownership. Assignment RHS and redirects remain unsplit.
Static whole-line validation and dot caller reconstruction check syntax/name/
literal bounds without reading bindings or decoding values. Only selected units
read the table, against the existing incoming status/table snapshot.

## Generated syntax and scalar counting

- Extend main EBNF WORD with the complete `${#NAME}` fragment. Preserve ordinary
  comments beginning with #, standalone braces/group productions, NAME/
  ASSIGNMENT_WORD/IO_NUMBER priorities and operator adjacency.
- Add a generated length node in the word EBNF before opaque braced fallback,
  with an explicit hash token and VariableName child. Adjust RAW ownership to
  avoid hash ambiguity while preserving literal # in all other contexts.
- Consume only generated length/name nodes in semantic rendering. Never scan
  source for `${`, #, names or closing braces; never edit generated artifacts.
- Reuse the word frontend's generated prefixed maiaCCLexDecode helper on the
  already bounded value, with independent fixed96entry units/offset buffers.
  Count distinct source-byte offsets among decoded UTF-16 units: paired
  supplementary units share one offset and therefore count as one scalar.
  This is semantic processing of stored data, not a second shell parser. It does
  not reset/reuse the active word tree, token stream or lexer/collector state.
- Validate source value length before decode, reserve the decoder's final offset
  slot and perform no heap allocation or recursion. Malformed UTF-8 must fail
  deterministically before redirect opens/dispatch. Add an internal render result
  for invalid parameter encoding and handle it as ordinary unit status2 with
  `shell: invalid UTF-8 in parameter`, using existing completion/negation/AND-OR
  paths; do not turn it into a fatal session error. Direct native/WASM probes
  will inject malformed values; no new binary-variable input interface is added.

Other braced operators remain outside this increment, including `${#?}`,
`${#1}`, default/alternate/error/assignment/removal forms and nested expansion.
Unsupported quoted forms retain opaque literal ownership, including a length
form nested inside an unsupported outer expression. Unsupported unquoted forms
remain rejected before any whole-line effects. Existing complete `${NAME}` and
`${?}` behavior remains intact. No prefix/multiple/pipeline assignment, custom
IFS, environment/export, substitution or compound execution is implied.

## Scope and unchanged boundaries

Affected repository: MaiaOS only. Planned files: main/word EBNFs and their
reproducible generated artifacts, shell-word.hpp and selected-unit render error
handling in shell.cpp; native/WASM word probes and main/word/guest regression
suites; VFS recipe source/expected assets; shell.wasm; README/checklists,
architecture and handoff. Existing 255byte lines,95byte rendered words/values,
12 original/final word slots including redirects,32variables,256wordtokens/
2048nodes,32page/fourhandle shell manifest and all worker plans stay unchanged.
Kernel/I/O/compiler sources, host imports/syscalls and dependency pointers do not
change. Any toolchain defect requires a separate minimal owner-repository proposal.
Preserve all unrelated dirty work and three already staged dependency pointers.

## Acceptance gates — completed locally

- [x] JavaScript first: independent main token types/UTF-16 spans/AST ownership,
  complete/quoted/escaped/malformed length forms, # comments, brace groups,
  assignments and adjacent redirects; preserve existing contextual gaps.
- [x] Native raw C++98 lexer/parser parity and own-chain strict-C89/WASM token/
  span/tree parity. Regenerate through MaiaOS's own MaiaCC/tREx only.
- [x] Independent scalar expectations: ASCII/empty/unset, Portuguese, combining
  text, emoji, supplementary and Unicode boundaries, space/tab/LF, stored dollar/
  glob syntax counted as data, name31/32, value95/96 and decimal composition95/96.
  Compare valid UTF-8 counts with bash/zsh under an explicit supported UTF-8 locale;
  dash is an ASCII/byte oracle only, not the UTF-8 length reference on this host.
- [x] Native C++98 ASan/UBSan and actual WASM probes validate exact digits/masks,
  decoder buffer guards, truncated/overlong/invalid-continuation/surrogate/out-of-
  range UTF-8 rejection, no value mutation, active tree preservation and restored
  stack. Failures never read beyond the95byte bound. No JavaScript-only counter
  can substitute for the generated C++98/actual guest path.
- [x] Actual shell guest: quoted/unquoted concatenation, assignment snapshots,
  unsplit redirects, fields/glob,31/32name and95/96word/12slot bounds, skipped
  branches without value work, ordinary status/negation, dot persistence/return,
  atomic file handling and repeated commands with stable resources.
- [x] npm run build:shell -- --check-c89; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields, test:shell-variables,
  test:shell-status, test:shell-lists, test:shell-scripts, test:shell-redirection,
  test:shell-glob and test:shell-pipelines pass; generated artifacts reproduce.
- [x] Updated VFS recipes execute twice per line and by guest dot through actual
  keyboard/kernel/I/O/shell/cat/wc/LVGL; test:shell-lvgl, test:shell-boot and npm
  test pass. Record artifact size/hash and actual memory, with progress updates.
- [x] Scoped diff/index audit preserves prior work, clean dependency owners,
  versions and staged pointers. No commit/push/npm/version/history changes.

Browser interaction, MaiaWASMVM and device acceptance remain separate unrun gates
unless actually performed. Approval does not cover any excluded feature.

## Read-only investigation evidence

Current generated JS main parser rejects unquoted `${#NAME}` and accepts it
quoted structurally. Current word contract treats it as opaque quoted text.
Inspected generated word maiaCCLexDecode: validates UTF-8, emits UTF-16 units
plus source byte offsets, stores the final offset; no API addition required.
On this host, native bash and zsh return6 for `ação 😄` under en_US.UTF-8 and11
under C; dash returns11 under both. These are proposal probes, not tests of a
new MaiaOS implementation. Production shell.wasm is unchanged from the previous
validated checkpoint (1457603bytes).

## Separate toolchain finding — local struct declaration

The first test-only decoder-guard frame used a type declared inside its function.
Native clang++ C++98 passes; current own MaiaCpp AST-strict emits the variable
as int and strict C89 rejects its member access. Minimal read-only reproduction:

```cpp
int main() {
    struct Local { unsigned int value; } frame;
    frame.value = 7;
    return frame.value == 7 ? 0 : 1;
}
```

Evidence: /tmp/maia-local-struct-probe/probe.cpp, probe.c and native. Emitted
`int frame` makes `frame.value` invalid; clang reports two member-access errors.
This does not affect the production length counter, which uses ordinary local
arrays. Test guard instrumentation now uses a file-scope named frame type with
the same automatic storage, buffers, sentinels and checks; native sanitizer and
actual WASM guards still run. This is not a fix or conformance claim for local
C++ class declarations. The MaiaCpp principal is unchanged; any owner correction
requires separately approved scope, regressions and protocol propagation.

## Recorded implementation and validation

Main/word EBNFs regenerated only via own MaiaCC/tREx. ParameterLength owns HASH
and VariableName; HASH is removed from RAW and explicitly preserved in all
literal/escape/opaque productions. shellParameterLength bounds95bytes, uses the
prefixed generated decoder with independent96entry arrays, counts unique byte
offsets, and returns internal4 on invalid encoding. beginNextListUnit completes
that result normally with status2; no mutation/open/launch precedes validation.

All final commands exit0: npm run build:shell -- --check-c89 (1462003bytes,SHA256
3daa3b4152e4cbfb1f37b0b840a0b478ca102995bc55119b17cb7fec088a5802);
test:shell-grammar134nativefixtures/incl2known contextual gaps;node apps/shell/
rtest.js --cpp135trees;test:shell-parser154rawcases/27recursionprofiles;
test:shell-equivalence5960(2922accepted/2228syntax/810lexical),zero token/span/
tree differences and restored stack;test:shell-word382JS/native/WASMcases,
176regularbyte/maskexpectations plus seeded names/assignments,66native/dash/WASM
fieldcases,16independentUnicode counts against bash/zsh UTF-8 +native/WASM,
17invalid byte cases,95/96bound/decoder guards/value stability/active trees,
ASan/UBSan and6reproducedwordartifacts. Main header equals regenerated
maia-shell.cpp byte-for-byte.

test:shell-fields: positive UTF-8/literal/locale policy,31/32name,95/96word/value,
12/13slot bounds and pre-effect failure;100length scripts alongside200braced/
400field cycles; isolated test-only full-shell build via
`node scripts/build-shell-wasm.mjs --check-c89 --test-parameter-values <temporary-output.wasm>`.
That guest alone exports maia_test_* injection; production has none. Four bad
stored byte vectors exercise status2/negation/AND-OR, skipped branches, atomic
assignment/redirects, dot return, worker rejection, original bytes and usable
session recovery. Temporary guest/build outputs removed; production artifact
unchanged. Variables/status/lists/scripts/redirection/glob and31native sanitizer
list plans pass. Existing golden cases changed only for the approved `${#NAME}`
behavior, retaining unsupported forms/opaque ownership and whole-line atomicity.

Pipelines380flushes:fiveVFSrecipes66commands twice per-line and all five twice
by dot(recipe03 aborts at its deliberate line4syntax error). LVGL2264flushes/
1000parses/77911kernelsteps,stable shell20/I/O2pages;boot25sessions/41flushes,
zero surviving windows/endpoints. npm test66pass. Logs:/tmp/maiaos-length-*.log.
Progress/architecture/handoff and source/expected recipe assets updated. No root
or dependency publication, version/npm/history change. No interactive browser,
MaiaWASMVM/device or standalone I/O/font/kernel-policy validation. The separate
local-struct finding above remains unresolved, without owner edits.

Final baseline hash audit:36scopedfiles changed;index/threepins unchanged,all7
principal repositories clean,rootversion0.1.0 unchanged;recipe sources match
expected.json,production hash preserved after isolated test build,unrelated
prior work preserved. All final processes completed.

Suffix-removal follow-up (2026-10-30): named % / %% are separately approved and
validated; see [contract/evidence](SHELL_PARAMETER_SUFFIX_REMOVAL.md). Existing
semantics/regressions pass; operator nesting remains excluded. Root changes are
local, with published dependencies and no root commit/push/npm/version change.
