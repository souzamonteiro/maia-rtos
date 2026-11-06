# Bounded test predicates — approved contract

Date: 2026-10-31. Status: approved by the project owner; implemented; acceptance gates passed locally (reference engine).
Baseline: main b13f168 plus the validated local bounded-if checkpoint.
Add the standalone shell builtin test, making existing if conditions useful for
string/integer comparisons. This is neither general expression evaluation nor
complete POSIX test. Publication and other-repository changes are excluded.

## Selected command semantics

Run after the existing generated shell/word frontends, field splitting and filename
expansion. Predicates consume the resulting argv as data; quotes, whitespace,
backslashes, dollar signs, semicolons and pipeline characters inside a value
are never reparsed as shell source. Return0 for true,1 for false,2 for invalid
expression/integer/bounded frontend failure. True/false are silent. Errors write
one bounded diagnostic through existing stderr; do not introduce fatal special-
builtin semantics. Preserve outer lists/if/status/negation/dot behavior.

Support exactly this finite profile, with at most4 expression operands:

| Operand count | Selected interpretation |
|---|---|
|0|false|
|1|true iff that value is nonempty, even if it spells !, -n, --, [ or ]|
|2|! value, -n value or -z value; other forms are usage errors|
|3|value binary-operator value, or ! followed by -n/-z value|
|4|! followed by value binary-operator value|

For3 operands, a recognized middle binary operator takes precedence over a
leading literal !: test '!' = '!' is true. For2 operands, test ! -n negates a
nonempty string; it is not a missing unary operand. No recursive negation:
three operands ! ! x are outside this first profile (status2); two operands ! !
still negate the one literal string !. No special -- option delimiter.

String operators are = and != only, with exact UTF-8 byte equality, without
pattern matching, normalization, locale collation or length-based ordering.
-n/-z test nonempty/empty. Numeric operators are -eq/-ne/-lt/-le/-gt/-ge on
strict signed decimal32-bit values, from -2147483648 through2147483647.
Require the entire value to match [+-]?[0-9]+; accept leading zeroes as decimal
and normalize negative zero for comparisons. Reject whitespace, empty/sign-only,
explicit base prefixes, floats and overflow in numeric positions. A long all-zero
value may remain valid within the95-byte argument bound. A nonnumeric one-operand
value stays an ordinary nonempty string. This differs from printf's current
base-detection rules: do not reuse that conversion unchanged or alter printf.

Excluded: file predicates, -a/-o expression composition, parentheses, ==, broader
integer/locale semantics, recursive negation, [[ ]], the [ alias and generic
pipeline workers. Operators excluded as syntax may still appear as ordinary
single string values. Prefer shell &&/|| between separate test calls.

Examples within the existing one-line if profile:

```sh
N=ready; if test "$N" = ready; then echo yes; else echo no; fi
N=; if test -z "$N"; then echo empty; fi
if test -2 -lt 10; then echo smaller; fi
test '!' ação = acao; echo $?
```

## Generated predicate syntax and private transport

Add an authoritative finite predicate EBNF and reproducible JavaScript/C++98/XML
artifacts generated exclusively through the in-repository MaiaCC/tREx. Both
existing shell EBNFs and their ownership stay unchanged. Never hand-write an
expression parser, infer syntax with ad hoc argv comparisons or patch generated
sources. The adapter may perform envelope/count/size checks and arithmetic on
already classified generated nodes, as distinct from syntax recognition.

Frame each final argv value, including command name test, as exact UTF-8 bytes
followed by one NUL delimiter. Shell argv cannot contain an embedded NUL. Retain
empty strings as delimiter-only frames. No escaping, joining with spaces, quoting
reconstruction or source substitution. Treat all bytes before a delimiter as
value data, including newline and punctuation. The grammar recognizes complete
operator/decimal argument frames and generic value frames; operator-like tokens
must also be accepted in value positions. Anchor each finite arity alternative
to the invocation end so local alternative success cannot consume a wrong prefix.

Use independent private storage: at most5 frames,480 payload bytes plus a guard
terminator,6 tokens including EOF and128 collector nodes. Reject excess arguments
as ordinary test usage errors; the existing12-slot shell limit still applies
before dispatch. Lexer recognition, UTF-16 spans, operation/value ownership and
arity precedence belong to generated nodes. Token transport copies bounded bytes
only. Preserve the command/list and word trees, their tokens and rendered argv.

All selected argument frames must be valid UTF-8 for this generated frontend;
invalid test data is ordinary status2. Unselected commands do not frame, parse,
decode or compare values. The integer grammar owns complete decimal spelling;
semantic conversion performs checked unsigned magnitude accumulation and signed
ordering, without signed overflow or implementation-dependent out-of-range casts.
Check both operands even if an equal/unequal outcome appears obvious. Bound all
work by fixed argv/byte/token/tree limits; no recursion or per-expression heap
storage beyond the generated frontend's existing bounded temporary behavior.
Prove allocation/resource recovery on success, parse error and numeric overflow.

The existing main grammar reserves unquoted `!` outside the pipeline prefix.
Pass `'!'` as a predicate value/operator; `! test ...` remains outer negation.
This increment does not change contextual classification in the main frontend.

## Integration and boundaries

Use normal builtin dispatch, stdout/stderr, completed-unit status, if selection,
outer ! and delayed close handling. Test itself performs no VFS lookup or process
launch and emits no stdout. Existing selected redirects still open before builtin
execution: > can create/truncate a target even when test returns1/2. Preserve that
lifecycle; do not claim predicate errors roll back earlier redirect effects.
Unselected branches neither open redirects nor invoke the predicate frontend.
Keep test outside the fixed cat/final-wc worker profile; unsupported pipeline
placement rejects under existing launch validation, without adding a worker.

The [ alias is deliberately separate. The published MaiaGlob profile rejects an
unquoted isolated [ as an invalid filename pattern before builtin dispatch; the
existing echo [ regression documents that policy. Do not bypass glob for a
command name, automatically quote it, weaken invalid-pattern checks or edit
MaiaGlob to accommodate this increment. A future alias proposal must resolve
that contract explicitly; native [ oracles below are reference evidence only.

Expected changes: predicate EBNF/generation adapter and artifacts, small private
C++98 predicate helper and builtin/help dispatch, focused JS/native/WASM tests,
if/list/status/dot/redirect regressions, a bounded immutable simulator recipe and
exact expected bytes, reproducible shell.wasm and affected progress/evidence docs.
No host predicate implementation, DOM/LVGL replacement or bytecode interpreter.

Preserve line255bytes,95-byte words/values,12original/final slots including
redirects,32variables, eight leaf units/32flow instructions,32main tokens/1024main
nodes,2–4 existing workers and16KiB/256line scripts. Manifest shell32pages/four
handles and I/O8pages stay unchanged; measure actual usage (baseline21/2pages).
Guide/recipes/expected text plus command echo and final prompt must fit4096bytes;
current guide4078 including echo/prompt needs condensation before adding content.
No new syscall/capability/public ABI/process plan, kernel/I/O/worker production,
glob behavior, owner/dependency pointer, npm/version, commit/push or history change.
Preserve all previous local work and the three staged Cpp/Glob/Regex pins. A
separately found owner defect needs a minimal reproducer and explicit approval.

## Acceptance gates — passed locally

- [x] Independent expected JS syntax/arity/operator/value trees first, then raw
  native C++98 /own-chain strict C89/WASM token, UTF16 and complete-tree parity;
  reproduce every generated artifact without manual edits or source reparse.
- [x] Fixed expected statuses/silence plus native dash/Bash oracles for the shared
  profile: empty/value/arity ambiguity, literal operators, ! precedence, unary
  predicates, byte equality, spaces/quotes/backslashes/newlines/Unicode and data
  that looks like shell syntax. Keep excluded-profile expectations separate.
- [x] C++98 ASan/UBSan and actual WASM integer min/max, positive/negative/zero,
  leading zeroes/signs, all six comparisons,95-byte values,96-byte/static bounds,
  overflow/malformed/invalid UTF8, token/tree guards and reusable failure paths.
- [x] Guest if/elif/else, &&/||/$?/outer !, quotes/fields/glob, assignments and dot
  persistence; no matching/enumeration inside test. Skipped parse/decode and
  redirects have no effects. Plain test makes no VFS call; selected redirect
  creation/truncation, blocked stderr and close failure follow existing policy.
- [x] Reject unsupported worker placements before launch; preserve the negative
  invalid-glob bracket regression and all current strict glob rules.
- [x] Actual KEY/kernel/I/O/shell/cat/wc/LVGL recipe twice per-line and via dot;
  all existing eight recipes and conditional failure/transfer/reap tests remain.
  Repeated predicates/conditions/scripts keep memory/stack/resources stable.
- [x] npm run build:shell -- --check-c89; test:shell-grammar, test:shell-parser,
  test:shell-equivalence, test:shell-word, test:shell-fields, test:shell-variables,
  test:shell-status, test:shell-lists-native, test:shell-lists,
  test:shell-scripts-native, test:shell-scripts, test:shell-redirection,
  test:shell-glob, test:shell-pipelines, test:shell-lvgl, test:shell-boot; npm test.
  Add an explicit focused predicate gate, recording exact commands/results.
- [x] Update affected shell/kernel/I/O progress, actual artifact hash/size and
  memory observations; audit protected files, owner state, scope and full index.

Interactive browser, MaiaWASMVM/device, standalone kernel native/I/O/font and
full POSIX conformance remain separate; do not infer them from reference tests.

## Read-only preparation evidence — 2026-10-31

Read instructions, architecture, dispatch/word/glob paths, current integer
formatter and generated decoder. The existing formatter accepts base prefixes,
so decimal-only test must retain its own generated integer classification.
136 fixed native status/stdout-silence comparisons passed:34 vectors across
dash/Bash and test/[. These do not implement or authorize the [ alias in MaiaOS.

A temporary minimal argument-framing EBNF was generated through own MaiaCC in
JavaScript/C++98, compiled natively with ASan/UBSan and checked on10 independent
frames. Exact values survive empty arguments, spaces/quotes/backslashes/newline,
operators as data, UTF8/supplementary/combining scalars and95-byte arguments.
Both models preserve NUL frame boundaries and EOF. Scratch outputs were removed;
this proves transport feasibility only, not predicate syntax or new guest runtime.

Preparation changes only this proposal, one unchecked shell checklist row and
handoff. Production shell.wasm remains1595285bytes/SHA256
6570e1f2b07cb8bcc9d0a2af421f3112db18dbe559ed275c61d5df0adb7e9e16.
No production source/grammar/build, full-suite rerun, owner or publication action.

Preparation audit:exactly3 documentation paths changed; every production file
and entire Git index, including prior staged pins, remain unchanged.
git diff --check passes. No commit/push or release action.

## Implementation and acceptance evidence — 2026-10-31

Authoritative maia-shell-test.ebnf is generated by scripts/generate-shell-test.mjs
through the pinned MaiaCC/tREx. Six production JS/XML/C++98/prefix artifacts
reproduce byte-for-byte. shell-test.hpp frames final argv into private fixed
storage, collects generated nodes and evaluates checked decimal magnitudes or
byte/string predicates. shell.cpp adds only generated-frontend/helper includes,
help and ordinary builtin dispatch. scripts/build-shell-wasm.mjs generates the
new frontend; main/word EBNFs and their generated files remain byte-identical.

The independent fixture file has225 expectations for arity, operation/negation,
values, UTF16 offsets and semantic status. All pass in generated JavaScript,
native C++98 ASan/UBSan and own strict C89/WASM, including exact full-tree parity.
330 fixed dash/Bash comparisons confirm shared-profile statuses/stdout silence.
Overflow/project exclusions remain separate from native64bit/recursive behavior.
Native/WASM raw frames preserve empty strings, LF, quotes, backslashes, whitespace,
metacharacters, combining/supplementary Unicode and95-byte values. Six malformed
UTF8 vectors, excess tokens/envelope,95/96byte slot bounds, guarded temporary tree
exhaustion/recovery, input sentinels and300 reused success/error/overflow calls
pass with stable memory/stack. The probe is separate from production guest ABI.

Production shell.wasm:219 fixed statuses and silence checks (the6 LF-containing
vectors use the raw native/WASM transport, since CLI input is physical-line based).
If/elif/else, lists/outer negation/status, fields/glob, selected redirect timing,
zero plain/skipped VFS effects, rejected worker placements and150 dot/predicate
repetitions pass. One-byte stderr backpressure and delayed close failures on
true/false/usage preserve existing completion semantics. Selected malformed
stored values are rejected by existing glob UTF8 validation before dispatch;
the isolated own-chain seed guest proves skipped branches neither decode nor
open redirects. Direct raw frontend tests independently prove predicate UTF8
rejection. No glob bypass, main contextual-word fix or hidden test export enters
production. Quote argument '!' and reserved literal 'done' in recipe09.

| Exact command | Result |
|---|---|
| npm run build:shell -- --check-c89 | PASS: AST-strict C++98, strict emitted C89, complete compiler AST and validated own WASM;1645575bytes. |
| npm run test:shell-predicates-native | PASS:225 JS/native/own-WASM token/span/tree/status expectations,330 native oracles, sanitizers, guards/exhaustion/bounds/error recovery,300 repeated calls and six reproduced generated artifacts. |
| npm run test:shell-predicates | PASS:219 production cases, if/lists/status/fields/glob/dot150times, redirects, skipped/no-VFS effects, unsupported workers, stderr/close failure and stable21pages. |
| npm run test:shell-grammar | PASS:163 native fixtures including2 known contextual gaps. |
| npm run test:shell-parser | PASS:185 raw parser cases and27 recursion profiles, no cycles. |
| npm run test:shell-equivalence | PASS:6426 raw JS/native C++98/own-WASM inputs;3243accepted/2353syntax/830lexical errors; exact tokens/UTF16/trees and restored stack. |
| npm run test:shell-word | PASS:710 exact trees,327 byte/mask,252 field cases,16 Unicode counts/17 invalid vectors, native/WASM/sanitizers/guards/budgets and six reproduced word artifacts. |
| npm run test:shell-fields | PASS:existing field/parameter/removal/length/native oracles, limits/atomicity/dot/status/lazy branches,400 field cycles and other100/200cycle suites; isolated invalid test-value selected/skipped regressions. |
| npm run test:shell-variables | PASS:existing native byte oracles, generated assignment/name ownership, bounds/mutations/isolation, dot/close/EOF and400cycles. |
| npm run test:shell-status | PASS:persistent/snapshotted status,quotes/glob/static/runtime bounds,dot, delayed errors,negation/EOF and150/200repetitions. |
| npm run test:shell-lists-native | PASS:C++98 sanitizers,31flat plans/25conditional traces/50fixed native outputs and statuses. |
| npm run test:shell-lists | PASS:flat/conditional selection, native oracles, bounds/skipped effects/dot PC/no replay, delayed cleanup/reaping and150repetitions. |
| npm run test:shell-scripts-native | PASS:19 physical framing cases with ASan/UBSan. |
| npm run test:shell-scripts | PASS:bounded snapshot/framing/generated execution/caller restore,blocked/partial/error reads,limits/atomicity,150repetitions,EOF/exit/fatal cleanup. |
| npm run test:shell-redirection | PASS:binary/printf, truncate/append/stream paths, native comparisons, failure timing and cleanup. |
| npm run test:shell-glob | PASS:strict quoted/unquoted patterns, Unicode/fields/VFS, bounds/errors/lazy expansions and invalid isolated bracket policy unchanged. |
| npm run test:shell-pipelines | PASS:9 immutable VFS recipes/134commands twice per-line and all9files twice by dot, actual KEY/kernel/I/O/shell/cat/wc/LVGL;744flushes, bounded128KiB delayed transfers,conditional failure/trap/cancel/reap. |
| npm run test:shell-lvgl | PASS:real LVGL2423flushes/1000parses/77911kernel steps, stable shell21/I/O2pages and drained/closed streams. |
| npm run test:shell-boot | PASS:25real LVGL session launches/exits, startup/trap cleanup,41flushes and zero surviving windows/endpoints. |
| npm test | PASS:66tests, including immutable VFS and display assets with echo and final prompt below4096bytes. |

Recipe09 has12commands; guide+command echo+final prompt3507bytes<4096. No
terminal bound, manifest32/8page budgets/four shell handles, font, syscall,
capability, ABI, process plan, kernel/I/O production or dependency change.
Production artifact SHA256:
27aee6b260d971e3fcfb3ad0d8f881916ffc62bdc5da102bd6398044e5d70e37.

Root main b13f168 and all prior local work remain; the entire index including
three staged Cpp/Glob/Regex pins is preserved. All seven principal dependency
repos remain clean at their prior SHAs. No commit/push/npm/version/history action.
Manual browser, MaiaWASMVM/device, standalone kernel/I/O/font and complete POSIX
gates were not run. Next safe action:reload simulator and run
. /system/tests/09-test.sh; broader test/file/alias or control-flow work needs
its own bounded contract and approval.

Final audit (2026-10-31): second production build reproduced the same1645575byte
WASM and SHA256 above. Final native/WASM collector canary/exhaustion/recovery
gate passes. Exactly33 scoped paths changed; all other baseline bytes and the
entire index are preserved. All build/test sessions completed; no publication.

Owner acceptance update (2026-10-31): the owner reports the test increment worked
in the simulator. This records user-observed smoke acceptance, not an additional
agent browser inspection, a full manual matrix or MaiaWASMVM/device validation.
