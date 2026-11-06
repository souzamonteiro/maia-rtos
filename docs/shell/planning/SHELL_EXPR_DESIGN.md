# Expr — generated frontend and execution design proposal

Date: 2026-11-01. Status: design/feasibility only; no production implementation.
Baseline: MaiaOS main b13f168 plus the validated local bracket-alias checkpoint.
The owner asked whether expr should use a MaiaCC-generated EBNF/parser/lexer.
The answer is yes. This note records the architecture and decisions needed for
an execution contract; it is not approval to change execution, owners or releases.

## Architecture

Use a dedicated maia-shell-expr.ebnf, generated exclusively through the pinned
in-repository MaiaCC/tREx into JavaScript and C++98. The main shell parser keeps
command syntax, expansion, quoting, fields, filename patterns and redirections.
Expr receives only final argv, as test does. Frame each complete UTF-8 argument
with NUL; never concatenate arguments with spaces or rescan shell source.
An empty string, a single string containing spaces and three arithmetic arguments
are distinct inputs. Operators and parentheses are separate argv frames.

The generated expression tree owns precedence, associativity, grouping, values
and operator identities. C++98 semantic evaluation walks that tree with explicit
fixed work/storage limits; it must not implement another expression parser or
move evaluation into the host. JavaScript is a comparison oracle, not runtime.
The resulting code must pass AST-strict MaiaCpp -> strict C89 -> MaiaC/MaiaWASM
and execute in the guest through normal streams/status/completion.

A suitable precedence skeleton, lowest to highest, is:

```ebnf
Expression ::= Or
Or         ::= And (OrOperator And)*
And        ::= Relation (AndOperator Relation)*
Relation   ::= Sum (RelationOperator Sum)*
Sum        ::= Product (AddOperator Product)*
Product    ::= Match (MultiplyOperator Match)*
Match      ::= Primary (MatchOperator Primary)*
Primary    ::= Open Expression Close | Value
```

This is a design sketch, not a second authoritative production grammar. Repeated
operator chains avoid left recursion; parentheses require a separately checked
input/tree bound. The official EBNF must own complete frame spellings and EOF.

The target utility includes arithmetic +, -, *, /, %, comparisons, logical
selection |/&, grouping and the : basic-regular-expression operation. These are
separate expression layers, not shell list/pipeline operators. GNU-specific
length/index/substr/match/+token forms are separate extensions, not implied.
The full target and any first limited execution slice must be named explicitly;
do not advertise an arithmetic-only evaluator as complete expr.

## Integration and prerequisites

Current kernel launch supports fixed cat/final-wc workers. A first expr builtin,
like current test/basename/dirname, would exercise the generated guest frontend
without adding a process profile. Keep evaluator code independent of builtin
transport so a later standalone utility can reuse it under its own launch
contract. Builtin integration is proposed, not implemented or approved here.
Do not silently add expr as a pipeline worker or change the kernel launch table.

Normal shell quoting/expansion happens before expr. For example, the eventual
command `expr 2 + 3 \* 4` should receive five expression frames; quoted operators
also work. Keep existing glob behavior; do not grant expr a glob bypass.
The shell currently lacks command substitution. Adding expr alone does not make
`N=$(expr "$N" + 1)` work. Output capture or arithmetic expansion needs a separate
shell contract; do not add a nonstandard assignment shortcut inside expr.

A bounded result is printed only after successful expression evaluation, followed
by LF, through existing stdout. Existing selected redirects may already have
opened/truncated a file before a usage/runtime error; retain cleanup semantics.
Private evaluation never looks up variables/files or launches workers. References
to shell variables are resolved by the existing shell before argv framing.

## Decisions required before an execution contract

- Numeric representation: start from checked signed32 arithmetic to match the
  existing bounded shell, with explicit overflow, division by zero and
  INT32_MIN/-1 handling. Exact status distinctions must be fixed and tested;
  neither C89 signed overflow nor host floating-point arithmetic is acceptable.
- String/number conversion, preservation versus normalization of decimal input,
  null/zero truth, decimal range boundaries and string comparison/collation must
  have fixed independent expectations. Do not import GNU arbitrary precision or
  native64bit behavior accidentally into the guest profile.
- Resolve evaluation of logical right operands, including errors in an unused
  operand. macOS /bin/expr observed division by zero in `1 | 1 / 0`; reference
  implementations may use different evaluation strategies. Pin the chosen
  POSIX/reference profile before implementing lazy or eager evaluation.
- Reserve concrete argv/frame/token/tree/value-stack and evaluation-step bounds,
  compatible with the current 12-slot/95-byte words and 255-byte input unit.
  Keep shell32/I/O8page manifests, four handles, loop and scheduler quotas.
- Define expression error/status/negation and delayed-close behavior before
  dispatch integration. Syntax, numeric errors and resource failures must not
  print a successful partial result.

The : operator needs its own compatibility work. MaiaRegex's legacy text API
supports a documented basic ASCII subset with captures; its generated-program
API is UTF-16 ordered-path execution, not a POSIX leftmost-longest matcher.
Neither description proves complete expr BRE semantics. Specify anchored match,
first capture versus match length, no-match results, UTF-8 character counts,
invalid patterns and aggregate work/capture limits. If a BRE parser is needed,
generate it through MaiaCC; never hand-write a replacement. Owner changes need
separate approval and publication/propagation under the existing protocol.
Do not reinterpret BRE as glob or ECMAScript syntax. An initial arithmetic-only
slice can be useful, but must explicitly reject/exclude regex and be documented
as partial until the selected BRE profile is validated.

## Validation plan

1. Generated JavaScript first: independent syntax/precedence/associativity/value
   trees and exact argv boundaries; all operator spellings, nested groups,
   missing/excess operands, empty/space/Unicode data and malformed input.
2. Compare full token/UTF16-span/tree traces with generated native C++98,
   ASan/UBSan, strict emitted C89 and actual own-chain WASM. Reproduce every
   generated artifact and retain parser recursion/work profiles.
3. Add a separately tested bounded semantic evaluator, fixed result/status/error
   expectations and native expr comparisons for the shared profile; keep
   platform numeric/locale/evaluation differences explicit.
4. Only under an approved execution contract: normal shell dispatch, selected/
   skipped units, quoted/expanded operators, glob, redirects, if/loops, dot/return,
   stderr backpressure, close failure, bounds and repeated memory/stack recovery.
5. Real KEY/kernel/I/O/LVGL recipe with exact expected bytes, preserving existing
   recipes. Condense guide/metadata as necessary; keep terminal4096bytes and
   image/script16KiB bounds. Run relevant regression/conformance gates and two
   identical strict builds before reporting execution completion.
6. Update shell/kernel/I/O checklists and evidence; audit protected bytes, owners,
   main HEAD and entire staged index. Manual browser/VM/device remains separate.

No compiler, MaiaRegex, kernel/I/O/worker, dependency, ABI, npm/version,
commit/push or history change is included in this design preparation.

## Feasibility evidence — 2026-11-01

Scratch-only grammar generation through own MaiaCC succeeded in JavaScript and
C++98. Generated JavaScript passes12 valid and5 invalid frame cases covering
precedence/grouping syntax, arithmetic/logic/regex expression structure, empty
and Unicode data, identity operands and missing/excess grouping/arguments.
Token values retain the exact NUL-delimited argv frames. Reproduction script:
`node /tmp/maia-expr-design-probe.cjs`; temporary grammar/parsers were removed.
This is generated syntax feasibility, not an implemented evaluator, native C++
execution/parity, production integration, WASM or regex-conformance result.

Twelve native macOS expr observations informed the questions above: precedence,
parentheses, leading-zero/-zero normalization, plus-prefixed numeric rejection,
logical selection, comparisons, division errors and remainder. They are not an
assertion of complete portability. Production shell.wasm remains1,703,122bytes,
SHA256 `980627cdb7af612e8d28d9e24bed032b20e986a91a09185ec85f7ed6629b8ca2`.
Preparation changes only this design, an unchecked shell row and handoff.

Reference sources:

- [OpenBSD expr manual](https://man.openbsd.org/expr.1): argv/operator structure,
  precedence, BRE operation and result/status behavior.
- [GNU expr manual](https://www.gnu.org/software/coreutils/manual/coreutils.html):
  token arguments, extensions and implementation differences.
- [GNU quoting FAQ](https://www.gnu.org/software/coreutils/faq/coreutils-faq.html):
  shell filename expansion precedes expr unless operators are protected.
- [Pinned MaiaRegex documentation](../../maiaregex/README.md): distinct legacy
  text and generated-program APIs; do not infer full BRE compatibility.

## MaiaRegex follow-up — 2026-11-01

The owner requested expanding MaiaRegex. Its concrete additive BRE API/bootstrap
proposal is now [MaiaRegex/BRE_DESIGN.md](../../../maiaregex/BRE_DESIGN.md).
It preserves ordered lexer semantics and defines UTF-8/captures/longest matching,
finite work bounds and separate native/own-WASM acceptance. Public API/bootstrap
approval is pending; no new matcher or expr integration exists yet.

## Follow-up — 2026-11-02

The selected additive MaiaRegex BRE profile is functionally validated and
published as8a1dcbf, with its historical development bootstrap protected from
incidental remote recursion. Tested compiler-consumer propagation is underway.
The concrete [execution contract](SHELL_EXPR_EXECUTION.md) was explicitly
approved after publication approval; implementation must follow completion of
that propagation. The preparation-only statements above are historical.
No production expr code or runtime integration has been edited at this point.

## Implementation checkpoint — 2026-11-04

The execution contract and the separate384KiB compilation/64KiB matching
quotas were explicitly approved. The generated argv frontend and iterative
C++98 evaluator now pass556 independent native sanitizer and own-chain WASM
cases, including five serial matches and the matching-step budget. Initial
`*` is literal in the published MaiaRegex BRE profile (macOS expr rejects this
spelling); raw numeric rendering and Unicode scalar match lengths also follow
the selected contract rather than platform-specific normalization/byte counts.
Previous "not implemented" statements above describe historical checkpoints.
Full production shell/LVGL functional acceptance passes; see SHELL_EXPR_VALIDATION.md for exact commands and artifact reproduction status.
