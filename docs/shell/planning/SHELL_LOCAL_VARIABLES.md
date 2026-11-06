# Bounded shell-local variables — approved execution contract

Date: 2026-10-27. Status: owner approved; implemented and validated locally.
Baseline: main b13f168 plus the locally validated shell increments.

Follow-up: [SHELL_UNQUOTED_FIELDS.md](SHELL_UNQUOTED_FIELDS.md) is separately
approved and implemented. It supersedes this original slice's unquoted-reference
rejection and adds bounded semantic fields while preserving assignment RHS,
quote/redirect, table and execution bounds. Evidence below records the original
quoted-variable increment, not the latest field test counts.

Follow-up: [SHELL_BRACED_PARAMETERS.md](SHELL_BRACED_PARAMETERS.md) is explicitly
approved and implemented. It supersedes the original exclusion/literal behavior
for exactly `${NAME}` and `${?}`. Other quoted complex/unfinished braced forms
stay opaque; unsupported unquoted forms still reject. Historical evidence and
bounds in this contract remain scoped to their original checkpoint.

## Objective and examples

Add persistent local string variables to the existing shell guest. The first
slice supports assignment-only units, safe named expansion in double quotes,
assignment-value expansion and bounded unset. This extends the foreground-list
and last-status contracts, which explicitly excluded assignments/variables.

```sh
NAME='ação e espaço'; printf '[%s]\n' "$NAME"
FILE=/system/welcome.txt; cat "$FILE"
NAME=first; COPY=$NAME; NAME=second; printf '%s/%s\n' "$NAME" "$COPY"
false; RESULT=$?; printf '%s/%s\n' "$RESULT" "$?"
unset NAME; printf '[%s]\n' "$NAME"
```

## Exact first-stage language and bounds

- Names are generated ASCII NAME syntax `[A-Za-z_][A-Za-z0-9_]*`, case-sensitive,
  at most31bytes. Values are UTF-8 byte strings, at most95bytes excluding NUL.
  A fixed table holds32variables with no eviction or dynamic allocation.
  Shell startup has no imported host environment or predefined local variables.
- Each assignment unit contains exactly one generated Assignment, no command
  name, no redirection and no pipeline stages. Lists can sequence assignments.
  Prefix assignments (`A=x echo ...`), multiple assignments in one unit,
  assignment redirects and assignments inside pipelines remain statically rejected
  for the whole line before any effect, even in skipped branches.
- Recognize the unquoted name and equals marker exclusively through generated
  syntax, before quote removal. A quoted/escaped equals or name must never create
  an assignment. Within ordinary command arguments, `A=x` remains ordinary data.
- Assignment RHS applies existing quote/escape removal, bare `$?` and simple
  `$NAME` expansion, including unquoted references, without field splitting or
  filename expansion. It can store spaces, wildcard bytes and additional equals.
  Braced references in this slice remain unsupported/literal as specified below;
  do not widen the main shell grammar's unquoted braces.
- Ordinary argv, command names, dot operands and redirect targets expand named
  references only inside double quotes. Single quotes and escaped dollars remain
  literal. Unset names expand to an empty string, preserving one quoted argument.
  Name recognition is maximal: `$NAMEsuffix` is one name. Use `"$NAME"suffix`
  for concatenation in this first slice.
- Unquoted named references in ordinary words are statically rejected before any
  line effect, rather than retained as apparent variable text or expanded with
  incorrect field-splitting semantics. Existing unquoted `$?` remains supported.
  Future unquoted named expansion needs a separate field-splitting/glob contract.
- Only bare `$NAME` is added here. Existing `${NAME}`, `${?}`, other special/
  positional forms, substitutions and arithmetic retain their declared current
  behavior; none becomes a new parameter language in this slice. In particular
  quoted braced forms remain literal and unquoted braced forms keep their current
  main-grammar rejection. Preserve generated opaque ownership for legacy braced
  forms so inner dollar text is not partially evaluated as a new parameter.
  Exact recognition and fallback behavior must be covered
  by generated-tree and byte expectations, not ad hoc source rewriting.
- `unset [--] [name ...]` removes local bindings; zero names succeeds, absent
  names are harmless. Validate every expanded name and bound before removing any
  binding. Invalid names diagnose and return2 without partial deletions. Empty
  assignment stores an empty value; it does not free a slot. Unset frees slots.
- Names such as HOME/PWD/PATH/IFS have no new special effects. They are ordinary
  local strings in this stage; existing cd home/cwd behavior and absence of
  environment, PATH search and field splitting remain unchanged and documented.
  No export/readonly/set, worker inheritance or host environment access is added.

## Generated frontend and semantic ownership

The main EBNF already recognizes Assignment/ASSIGNMENT_WORD; retain its grammar
and generated token boundaries. Extend the focused word-parts EBNF to identify
assignment name/value ownership and named parameters. A shared generated Input
can distinguish assignment-form words and ordinary words; only a main-tree
Assignment grants assignment semantics. The same bytes as a Word argument must
be rendered as data. Do not split an ASSIGNMENT_WORD by searching for equals.

All new name validation, including unset operands, must consume generated name
syntax. No handwritten NAME lexer, parameter scanner or regex adapter. Preserve
MaiaCC-generated JS/C++98/XML and deterministic prefix/unprefix generation.
Use the existing collector scratch where possible; verify unchanged256-token/
2048-node word and1024-node main bounds rather than silently growing buffers.

The renderer emits variable bytes only as protected data in quoted contexts.
Never reparse a stored value as syntax, redirects, source code or a second round
of expansion. Literal variable bytes `$?`, quotes, semicolons, backticks and glob
operators remain data after insertion. Stored values do not retain assignment
quote masks: protection comes from the later expansion context.

## Selection, commit and completion

- Validate the whole line's structures, names and static bounds without reading
  speculative values or changing bindings. Caller reconstruction after dot also
  performs syntax/validation only. No skipped unit evaluates/mutates variables,
  enumerates globs, opens files or starts workers.
- Evaluate a selected assignment RHS against the old table and one incoming
  status snapshot. Self-reference sees its previous value. Fully render and check
  bounds/capacity before committing the single binding atomically. Overflow or
  table exhaustion returns2 and leaves every existing binding unchanged.
  Updating an existing name works even with all32slots occupied.
- Successful assignment returns0 through normal completion; `$?` in its RHS
  sees the incoming status. `! A=x` commits the value then negates completed0
  into1. Ordinary runtime rendering failure still follows existing AND-OR and
  negation rules. Static whole-line rejection is not inverted.
- Quoted argument and redirect expansions happen only for selected units and
  observe preceding completed assignments. Overflow must occur before redirect
  opens. All words/stages in a unit see stable local bindings and the same
  incoming status; no command substitution or mutation occurs during expansion.
- Dot bodies share the current shell table; updates persist after return, including
  updates before a later script-line failure. Caller reparse must not reexecute
  prefix assignments or overwrite values. Empty scripts and existing snapshot/
  abort/exit semantics remain unchanged. A new shell instance starts empty.
- Existing delayed output/close/reap status publication, fixed step budget,
  fatal transport/exit cleanup and kernel last-worker policy remain unchanged.

## Files, boundaries and acceptance

Expected files: shell.cpp, shell-list.hpp, shell-word.hpp, a bounded variable
helper, maia-shell-word.ebnf and generated word artifacts; generation/test
adapters as required; shell.wasm, focused tests, simulator recipe/expected bytes,
progress READMEs, architecture notes and handoff.

No host ABI, syscall, capability, kernel/I/O source, process-plan format, worker
package, manifest budget, compiler/dependency or repository-boundary change.
No commit/push, npm, versions or history operations. If existing generators or
compilers cannot implement the specified generated frontend, report a minimal
reproducer and request owner-component approval before changing that repository.

- [x] JS word AST/token/span fixtures and native C++98 equality first, including
  assignment versus data ownership, quote/escape boundaries, maximal names,
  legacy literal/braced forms and unsupported unquoted named references.
- [x] Native C++98 ASan/UBSan and exact reproduction of generated word artifacts;
  main JS/native/WASM parity retained and focused guest frontend probes added.
- [x] Native /bin/sh oracle for the selected assignment/quoted-expansion subset;
  explicit expectations for intentional bounds and deferred language behavior.
- [x] Own C++98 -> strict C89 -> MaiaC/MaiaWASM production shell build and real
  guest assignment/lookup/unset, Unicode and quote/glob protection, self-reference,
  31/32-byte names,95/96-byte values,32/33slots, atomic failures and slot reuse.
- [x] No source injection or second expansion from stored bytes; runtime overflow
  before file effects, skipped lookup/mutation, stable snapshots, redirects,
  AND-OR/negation, dot-body persistence and caller restoration without prefix replay.
- [x] Independent shell-instance isolation; repeated assignment/unset cycles with
  stable memory/stack/resources, within unchanged32-page/four-handle limits.
- [x] Real KEY/kernel/I/O/LVGL scenarios for named file paths and worker pipelines,
  updated recipes twice per-line/by dot, retained status/list/script/printf/path/
  redirection/parser/boot/unit regressions and accurate progress/checkpoint docs.

Interactive browser, MaiaWASMVM/device, full environment semantics and broader
POSIX conformance are separate gates and must remain visibly pending.

## Investigation completed before approval

Read the actual EBNF, planner, word renderer, generator transport and tests.
Generated main JS AST probes accept NAME=value, quoted Unicode/space RHS,
NAME=$OTHER, empty RHS and quoted named/braced command words. They distinguish
Assignment nodes from Word arguments; main unquoted braced references reject.
Current planner/validator explicitly reject Assignment nodes. Current word tests
expect literal `$name`, including inside quotes; those expectations change only
for the newly approved exact bare-name contexts, with independent native oracles.
Four native /bin/sh probes confirmed quoted space/Unicode preservation, unset
empty expansion, self/value copy, skipped assignment and incoming status versus
successful assignment status. These are investigation checks, not guest variable
implementation evidence. No product code or generated artifacts edited here.


## Completed local evidence

The focused EBNF now generates AssignmentForm/AssignmentValue, BareName,
VariableName and NamedParameter; legacy braced forms use opaque generated
ownership. LETTER/DIGIT tokens remain one character so escaped identifiers keep
existing byte protection (e.g. backslash-abc protects only a). Main EBNF and
main generated parser remain unchanged. The renderer consumes nodes, not raw
name/equals/dollar scans. Single main Assignment-only units are structurally
certified before effects; literal assignment-shaped argv still render as data.
The fixed shell-variables.hpp table commits only fully validated values; unset
validates every rendered name with the generated frontend before deletion.

Passing commands:

- `npm run test:shell-word`:318 JS/native C++98/actual WASM token/span/tree probes with
  native/WASM renderer parity;162 regular byte/mask expectations plus seeded named/assignment
  cases,31/32-byte names,95/96-byte values, legacy opaque/nested text, escaped
  names and protection boundaries, uint32 status, ASan/UBSan, strict emitted C89
  and byte-for-byte reproduction of all6generated word artifacts. Test-only guest
  probe receives seed bytes through copied input and calls production helpers;
  production execution is independently tested below.
- `npm run test:shell-lists-native`:31 generated main-tree plans, native C++98
  ASan/UBSan, single assignment ownership and unsupported profiles, existing
  eight/nine-unit/negation/selection/range cases.
- `npm run build:shell -- --check-c89`: own strict C++98/C89/MaiaC/MaiaWASM;
  shell.wasm1,451,733bytes, SHA256
  da911addef11b8609d853001f36404465175b967016bcaa8062aaff13f02b1a5.
- `npm run test:shell-variables`: native /bin/sh oracles and actual guest checks
  for assignment/quote/suffix/status, empty/unset, no reinjection or RHS glob,
  skipped evaluation/effects, exact names/values/table capacity, full-table
  update/slot reuse, atomic failed mutations, runtime overflow before file opens,
  negated failed assignment, script-body persistence/caller prefix non-replay,
  session isolation, close/EOF and400 stable cycles. Final rerun includes negated
  overflow and unchanged previous binding. Output masks are also tested above.
- `npm run test:shell-status`, `test:shell-lists`, `test:shell-scripts`,
  `test:shell-redirection`, `test:shell-paths`: retained failures/limits/lifecycle/
  printf fields/path utilities plus native sanitizers pass. The prior literal
  double-quoted $name expectation now expects empty unset expansion; ordinary
  unquoted names receive explicit structural rejection tests instead.
- `npm run test:shell-equivalence`:unchanged main5,839 raw inputs;2,861accepted,
  2,168syntax/810lexical failures, exact JS/native/WASM tokens/spans/trees and
  restored stack. This is main parsing, separate from word/guest execution.
- `npm run test:shell-pipelines`:325actual LVGL flushes, real KEY/kernel/I/O/cat/wc;
  five recipes/51commands twice per-line and all five dot files twice, named input/
  output paths and variables in scripts. Added fixture uses absolute shared path
  because these direct cases precede the existing cd-to-home case.
- `npm run test:shell-lvgl`:2,227flushes/1,000parses/77,911kernelsteps,stable
  shell20/I/O2pages within unchanged32/8budgets. `npm run test:shell-boot`:
  25sessions/41flushes,zero surviving windows/endpoints. `npm test`:66pass.

No compiler/dependency or kernel/I/O implementation changes. Test adapter
construction exposed an independent raw-UTF-8 narrow source string limitation
in both MaiaCpp and MaiaC; native Clang C++98 passes the minimal case, both owner
frontends reject it lexically. See UTF8_SOURCE_STRING_LITERALS.md for reproducers
and the separately required approval. Test-only seeds now use the same runtime
copied-byte input as the real shell, preserving every Unicode/quote expectation;
no raw-source-literal conformance is claimed. No owner files edited or fix
silently assumed. Missing graphics callback fixtures in the probe host were
corrected before executing the probe; import checks still permit env only.

No interactive browser, MaiaWASMVM/device or broad POSIX conformance claim.
No commit/push/npm/version/history change; root main b13f168 plus preserved dirty
work/staged Cpp/Glob/Regex pins. All feature/source/artifacts remain local.
