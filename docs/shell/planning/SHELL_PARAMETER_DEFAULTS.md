# Named parameter defaults — approved contract

Date: 2026-10-29. Approved: 2026-10-30. Status: implemented and validated locally.
Baseline: main b13f168 plus the validated local parameter-length and published
MaiaCpp local-struct checkpoints. Previous parameter contracts excluded these
operators. Owner approved the proposed contract and implementation; publication is excluded.

Follow-up (2026-10-30): [SHELL_PARAMETER_ALTERNATES.md](SHELL_PARAMETER_ALTERNATES.md)
is separately approved and implemented/validated locally for exactly
`${NAME+word}` / `${NAME:+word}`. It supersedes only the alternate-operator
exclusion in this historical checkpoint. Mixed/default/alternate nesting and
other operators remain outside the selected scope.

## Selected behavior

Add exactly `${NAME-word}` and `${NAME:-word}` in ordinary words, double quotes,
assignment RHS values and existing redirection targets. NAME retains its ASCII
syntax and31-byte limit. The first form uses word only if NAME is unset; the
second uses word if NAME is unset or its value is empty. Neither form changes
NAME or allocates a variable slot. Distinguish absence using the existing table
index, rather than treating the current empty-string lookup as evidence of unset.

| NAME state | `${NAME-reserva}` | `${NAME:-reserva}` |
|---|---|---|
| Unset | reserva | reserva |
| Set empty | empty | reserva |
| Set to ação | ação | ação |

The fallback may be empty, contain literals, existing escapes and quote forms,
and use `$NAME`, `${NAME}`, `$?`, `${?}` and `${#NAME}`. Simple references within
the fallback use the selected unit's existing table/status snapshot. For example,
`${NAME:-${OTHER}}` and `${NAME:-${#OTHER}}` are in scope. Default operators nested
inside a fallback, such as `${NAME:-${OTHER:-x}}`, are explicitly excluded in this
first slice. No command/arithmetic substitution or additional braced operators.
Single-quoted and escaped dollars outside expansion remain literal.

Use POSIX quote ownership within this supported subset, verified independently
against native shells. An enclosing double quote protects the selected result;
single quote characters inside its fallback are data (for example,
`"${NAME:-'a b'}"` yields the literal apostrophes when NAME is unset). Outside
double quotes, fallback quote/escape nodes retain their own protection and empty
anchors. Only unprotected selected expansion bytes undergo the existing fixed
space/tab/LF splitting and per-field MaiaGlob. Inserted variable bytes are never
reparsed as source. Assignments and redirection paths remain unsplit.

Only the selected branch is semantically rendered: when NAME supplies the result,
do not look up fallback parameters, decode their UTF-8 lengths, generate their
fields/glob, or append their bytes. Unselected AND-OR units likewise do no value
work. Whole-line static validation still checks all syntax, name bounds and the
existing literal bounds without reading bindings. Alternative branches must not
be concatenated during static validation. Syntax/bound failures remain atomic
before assignments, file opens, worker launch or other command effects.

## Generated syntax and bounded evaluation

Extend both owner EBNFs and regenerate through MaiaOS's own MaiaCC/tREx only.
The main WORD grammar must own the complete default fragment, including fallback
spaces/operators protected by that fragment, without turning standalone braces
into ordinary word characters. Preserve comment, assignment, IO_NUMBER, reserved
word and adjacent-redirection ownership. Define explicit default/fallback nodes
in the word frontend before opaque fallback. Quote contexts must be represented
by generated structure; no handwritten dollar/name/brace/quote scanner.

Keep fallback productions finite with respect to the two new operators: they
may refer to existing simple/length parameter forms, but never recursively to a
default form. Semantic rendering traverses generated node ranges with fixed
state and descendant skipping, without reparsing substrings, resetting the
active tree or recursively calling the renderer. No new unbounded recursion or
heap allocation. Measure generation size/time and runtime stack/memory on the
corpus; compare with the baseline rather than raising limits to mask a failure.

Only complete supported forms gain execution meaning. Excluded nested defaults,
other operators and malformed forms retain the prior compatibility policy:
unsupported quoted forms remain wholly opaque, without partial inner expansion;
unsupported unquoted forms reject before effects. Preserve all existing simple
braced/length, assignment, list, script, pipeline and redirection behavior.

The new operators apply to named local variables only; status may appear in the
fallback, but `${?:-x}`, positional parameters, assignment/alternate/error/removal
operators, export/environment/IFS changes and compound execution are excluded.

## Affected files and unchanged boundaries

MaiaOS only: main/word EBNFs and reproducible generated sources; shell-word.hpp
and, only if needed, private shell field/provenance helpers; existing native/WASM
word probes and parser/guest suites; VFS recipe sources and expected outputs;
shell.wasm; shell progress README, architecture notes and handoff.

Keep line255 bytes, rendered word/value95 bytes, name31 bytes,32variables,
12 original/final slots including redirect targets,256 word tokens/2048nodes,
32-page/four-handle manifest and all existing process plans. Kernel, I/O,
compiler owner sources, syscalls/imports and dependency gitlinks do not change.
Preserve unrelated dirty work and the three staged Cpp/Glob/Regex pointers.
No commit/push, npm, version change or history rewrite. Any new toolchain defect
requires a separate minimal reproduction and owner-repository authorization.

## Acceptance gates — completed locally

- [x] JS first: independently expected complete/empty/malformed forms, token
  types, Unicode UTF-16 spans, generated branch/quote ownership, assignments,
  adjacent operators/comments/groups, and unchanged excluded opaque forms.
- [x] Raw native C++98 lexer/parser and actual own-chain strict-C89/WASM exact
  token/span/tree parity; deterministic generated sources and bounded profiles.
- [x] Independent native dash/bash oracles and C++98 ASan/UBSan plus actual WASM
  byte/mask/split/anchor expectations: unset versus empty versus nonempty,
  fallback simple/status/length references, UTF-8, inner/enclosing quotes,
  escaped closing braces, glob data, concatenation and empty fields.
- [x] Exact name31/32, value/word95/96, slots12/13 and token/tree limits;
  skipped fallback length of an injected malformed value does not decode/fail,
  selected invalid length retains ordinary status2 and no effects. Static
  preflight does no lookup; no fallback assignment or variable-slot consumption.
- [x] Actual shell guest: assignments, unsplit/ambiguous redirects, lazy lists,
  negation/status, dot caller restoration, pipelines and resource cleanup;
  repeated commands retain bounded stack and memory within current budgets.
- [x] npm run build:shell -- --check-c89; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields,
  test:shell-variables, test:shell-status, test:shell-lists,
  test:shell-scripts, test:shell-redirection and test:shell-glob pass.
- [x] Updated VFS recipes/expected bytes run through actual KEY/kernel/I/O/
  shell/workers/LVGL; test:shell-pipelines, test:shell-lvgl, test:shell-boot
  and npm test pass. Record artifact hash/size and measured memory.
- [x] Scoped diff/index audit and progress documentation preserve prior work,
  versions, dependencies and production architectural boundaries.

Interactive browser and MaiaWASMVM/device acceptance remain distinct gates.

## Read-only proposal evidence

Current generated main JS parser rejects `echo ${N-padrão}` and
`echo ${N:-padrão}`, while accepting the double-quoted form structurally.
Current word JS trees give OpaqueBraced ownership to `"${N:-$OTHER}"` and
`"${N:-${#OTHER}}"`, without inner NamedParameter or ParameterLength nodes.
Host /bin/sh independently returned the table above and literal apostrophes
for the enclosing-double-quote example. These are baseline/oracle probes, not
validation of a new MaiaOS implementation.

The browser skill was read and its supported runtime initialized; selecting iab
returned unavailable and browser discovery returned an empty list. No interactive
simulator acceptance was performed and no alternate rendering/browser was used.
Production source, generated artifacts and binaries are unchanged by preparation.


## Implementation notes and reference-shell differences

The generated word lexer distinguishes BRACED_DOLLAR before a brace from a bare
DOLLAR; fallback productions cannot consume a nested unsupported braced start as
plain data and then expand its interior. Complete opaque braced nodes preserve
inner quote ownership; the previous unfinished-form fallback remains available.
Semantic rendering selects a generated subtree, skips the unused branch, and
marks only selected unprotected fallback bytes for existing field splitting.
Names in either branch are checked against31bytes without value lookup.

Native dash on this host counts UTF-8 bytes for length; use Bash/Zsh with an
explicit UTF-8 locale for scalar-length expectations. Dash removes the backslash
in an escaped closing brace within an enclosing double-quoted default, whereas
/bin/sh (Bash) and /bin/bash preserve it. This selected bounded profile preserves
the backslash there, and tests use an explicit Bash oracle for that case rather
than pretending all native shells agree. Unquoted escaped closing braces remove
the backslash normally. These distinctions do not claim full POSIX conformance.


## Recorded frontend validation — 2026-10-30

Commands completed with exit0: npm run test:shell-grammar (140syntax cases,
including2explicit prior contextual gaps), test:shell-parser (162raw cases,
27recursion profiles without same-rule/same-position cycles),
test:shell-equivalence (6081raw inputs:3013accepted/2253syntax/815lexical),
and test:shell-word (432JS/native C++98/WASM trees,185explicit regular byte/mask
expectations,101independent native/WASM field cases,16Unicode counts,
17invalid encodings, ASan/UBSan, exact limits/stack/guards and6reproduced files).
Main JS/C++98/XML artifacts also reproduce byte-for-byte; the generated C++
snapshot and production header match. Default profiles for1/4/16/32fragments
have constant peak9, calls49/133/469/917 and zero cycles.

The initial real lists gate caught a95byte ordinary-word regression caused by
applying VariableName checks to the grammar's BareName literal alternative.
Exclude only BareName from that31byte semantic-name validation and keep the
original lists expectation. An independent95byte native/WASM word case now
passes; parameter/default/assignment names still fail above31bytes.
The unchanged lists expectation passes again in the final guest suite below.

Generation comparison used exact task-start EBNFs reconstructed only in /tmp;
both hashes were verified against /tmp/maiaos-defaults-before.json before use.
Own tREx generation on this host, one observation per backend under concurrent
build load (these timings are not runtime-throughput guarantees):

| Frontend/backend | Baseline bytes/time | Current bytes/time |
|---|---|---|
| Main JavaScript | 69,305 /1.315s | 71,445 /2.002s |
| Main C++98 | 108,713 /1.297s | 133,784 /2.005s |
| Word JavaScript | 64,207 /0.991s | 119,534 /3.300s |
| Word C++98 | 91,269 /1.001s | 170,742 /3.325s |

The word frontend grows to represent explicit fallback/quote/opaque ownership.
No grammar, token/tree, memory or timeout budget was raised. Measured constant
flat-call depth and actual guest stack/memory gates are separate from generation
cost; no native/WASM execution-speed improvement is claimed by these timings.
Logs: /tmp/maiaos-defaults-{grammar,parser,equivalence,word,generation}.log.


## Final guest verification and Git state — 2026-10-30

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

Production artifact:1,529,480bytes, SHA-256
`fbb18ff248c6c6fc694ee294fb9e84b694f07fe08b1b65779b74ea3a89fabed2`.
Generated artifacts are preserved with both EBNFs and existing reproduction
commands. Production exports contain no parameter-injection test API. An isolated
own-chain guest alone uses `--test-parameter-values <temporary-output.wasm>`;
it proves unused fallback lengths do not decode malformed stored bytes, selected
errors give ordinary status2, skipped lists do no work, assignment/file state is
atomic and original bytes/session recovery survive. Temporary outputs removed.

The actual guest suite covers unset/null/nonempty, inner/enclosing quotes, empty
fields, fallback values as data, glob,31/32names,95/96words,12/13slots, full32slot
table without implicit assignment/allocation, lazy lists/negation, dot return,
existing failures and100stable default scripts. Existing400field/200braced/
100length scripts and variable/status/list/script/redirection/glob suites pass.
New VFS data adds7rows to recipe05:five recipes/73commands run twice per-line
and all five files twice by guest dot, through actual KEY/kernel/I/O/shell/cat/
wc/LVGL. Recipe03 still aborts at its deliberate line4syntax error. Pipeline
cleanup/traps/cancel/backpressure pass with434flushes. Host1,000parses/
77,911kernelsteps stay shell20/I/O2pages (2,116flushes);25boot sessions have
zero surviving windows/endpoints (41flushes). npm test66pass. The expanded VFS
guide originally exceeded the unchanged4096byte I/O display; its text was
condensed to3985bytes and the image unit gate now checks guides/recipes/expected
text plus command echo fit. No terminal buffer or test expectation was enlarged.

MaiaOS mainb13f168 stays dirty/local with prior changes preserved. The index and
three Cpp6aa8eac/Glob48ad753/Regexe2ae3a8 pins are byte-identical to task start;
all seven compiler/library principals remain clean. Root package/version,
AGENTS, shell.cpp, kernel/I/O binaries and production interfaces are unchanged.
No commit/push, npm, release/version or history operation. Relevant progress
trackers and architecture updated. Interactive browser, MaiaWASMVM/device and
standalone kernel-policy/I/O/font gates were not run for this increment.

Suffix-removal follow-up (2026-10-30): named % / %% are separately approved and
validated; see [contract/evidence](SHELL_PARAMETER_SUFFIX_REMOVAL.md). Existing
semantics/regressions pass; operator nesting remains excluded. Root changes are
local, with published dependencies and no root commit/push/npm/version change.
