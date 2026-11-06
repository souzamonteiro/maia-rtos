# Expr builtin — proposed bounded execution contract

2026-11-02. Explicitly approved by the owner; implementation follows completion
of the authorized BRE publication/propagation. Implementation in progress. Follows
SHELL_EXPR_DESIGN.md and the separately approved/published MaiaRegex BRE API.
Publication/propagation of BRE is a prerequisite, not approval of this contract.

## Integration and ownership

Add a reusable C++98 evaluator and a shell builtin `expr`, using a dedicated
MaiaCC-generated EBNF frontend in JavaScript/C++98. It consumes the final expanded
argv, copied as complete NUL-terminated frames; no rescanning shell source or
joining arguments with spaces. The generated tree owns all expression syntax,
precedence, grouping and operand/operator roles. No handwritten expression
parser, host evaluation or host regex. Compile AST-strict MaiaCpp -> strict C89
-> own MaiaC/MaiaWASM and execute inside shell.wasm. The host carries bytes only.

Use published root MaiaRegex sources through MaiaOS's `maiaregex` submodule;
include its C++98 BRE compiler with generated namespace adapters and its C89
executor. Use one existing ordered lexer runtime, whose API/source is unchanged.
Consume the newly tested published MaiaCpp/MaiaGlob pointers as needed, preserving
all unrelated OS changes. This approves these MaiaOS dependency pointers only;
no compiler implementation change or incidental bootstrap upgrade.

Keep existing stream stdout/stderr, selected redirects, delayed completion/close,
status, lists/negation, if/loops and dot/return behavior. No new syscall,
capability, worker ID, kernel scheduler change or pipeline-program support.
No command substitution/assignment shortcut, GNU functions, shell arithmetic
expansion, variable lookup, VFS access or subprocess inside the evaluator.

## Syntax and value rules

All operators are complete argv frames. From lower to higher precedence:
`|`, `&`, `= != < <= > >=`, `+ -`, `* / %`, `:`, grouping. Chains at each level
associate left. All operator spellings and `(` / `)` are reserved in ordinary
value position; the generated rule immediately after `:` accepts exactly one
raw pattern frame, including a spelling otherwise reserved as an operator.
The pattern is not a grouped expression. Empty and whitespace-bearing values
remain single frames. Quoting/glob behavior belongs to the existing shell:
write `expr 2 + 3 '*' 4` so filename expansion does not consume the operator.

Raw string operands preserve their exact UTF-8 bytes, including leading zeros.
Computed integers use canonical decimal. Conversion for arithmetic accepts only
ASCII `-?[0-9]+`, within signed32; leading zeros are decimal, `+1` is string data
and fails numeric arithmetic. Conversion is checked semantic processing, not a
second expression parser. Truth is false for empty strings and decimal zero
(including `00`/`-0`), true otherwise; truth checks never mutate a string value.

Both logical operands are evaluated, left to right; an error in either operand
is an error even if its value would not be selected. `A | B` selects A if true,
otherwise B. `A & B` selects A only when both are true, otherwise integer0.
This eager policy matches the observed BSD/macOS behavior on division errors;
no short-circuit promise is made.

Comparisons use signed32 numeric order if both values are decimal integers;
a decimal operand outside that range is an operand error in that numeric case.
Otherwise compare exact UTF-8 bytes as unsigned bytes (C-locale-like order),
without normalization or locale collation. Both numeric conversion and result
formatting handle INT32_MIN without C89 undefined behavior. Division truncates
toward zero; remainder has the dividend's sign. Division by zero is invalid;
INT32_MIN/-1 division overflows, but INT32_MIN%-1 is0 without executing a signed
division instruction. Check addition/subtraction/multiplication before overflow.

`:`, anchored at start0 through MAIA_BRE_ANCHORED, uses the published longest
BRE profile. With any capture in the pattern, return the first capture's original
UTF-8 substring; if absent/unmatched/no match, return empty. Without captures,
return whole-match Unicode scalar length, or0 for no match. Empty participation
and no match both yield false zero/empty results. Do not add a textual `^` or
fall back to the legacy ordered matcher. Reject invalid UTF-8 and unsupported
BRE syntax normally; preserve the documented three libc-difference families.

## Fixed bounds, status and cleanup

Keep the current12 command-word slots (thus at most11 expression frames),95-byte
words and255-byte input unit. The authored EBNF unrolls at most4 parenthesis
levels, preventing unbounded generated-parser descent. Private buffers:1,056
framed bytes,12 token slots including EOF,512 collector nodes,12 semantic values
of at most95 bytes,4,096 evaluator steps. Reject capacity exhaustion normally.
No recursive semantic evaluator; traverse the generated tree iteratively.

For each of at most5 `:` operations, compile/free one BRE program serially:
pattern/input95 bytes,384 instructions/ranges,9 captures/depth,repeat255,
128 frames,10,000 matching steps,384KiB compilation workspace and64KiB execution workspace. Thus total matching
work is at most50,000 abstract steps; generated frontend work is separate, finite
and measured. No universal CPU or total allocator bound is claimed. Check NOMEM,
release every parser/program/work buffer and restore frames on every path.
Keep the shell32-page/I/O8-page manifests and four-handle bound. Verify actual
linked memory/stack and reproducible builds; do not increase limits silently.

Print exactly one complete result plus LF only after successful evaluation:
status0 for a true result;1 for false. Status2 means invalid syntax, nonnumeric/
out-of-range arithmetic operand, zero divisor, encoding or bad/unsupported BRE.
Status3 means checked arithmetic overflow, explicit evaluation/parser/BRE budget
exhaustion or allocation failure. Errors print one bounded stderr diagnostic,
no successful stdout result. Existing transport/close failures retain shell
completion precedence; selected redirections may already truncate a target.

## Independent expected examples

| Shell input | stdout | status |
| --- | --- | --- |
| `expr 2 + 3 '*' 4` | `14` + LF |0|
| `expr '(' 2 + 3 ')' '*' 4` | `20` + LF |0|
| `expr 001` | `001` + LF |0|
| `expr -0` | `-0` + LF |1|
| `expr ''` | LF |1|
| `expr +1 + 2` | none |2|
| `expr 1 '|' 1 / 0` | none |2|
| `expr 2147483647 + 1` | none |3|
| `expr -2147483648 % -1` | `0` + LF |1|
| `expr 'ação 😄' : '.*'` | `6` + LF |0|
| `expr 'ação' : '\(a.*\)'` | `ação` + LF |0|
| `expr abc : z` | `0` + LF |1|

These are selected-profile expectations. Raw numeric rendering differs from
observed macOS normalization;32-bit/locale/eager/BRE bounds are explicit. Native
expr comparisons cover only a shared profile, with differences recorded openly.

## Acceptance and approval boundary

Generated JS syntax/trees first; exact native C++98/ASan/UBSan and actual own-WASM
lexer/token/span/tree parity; independently expected arithmetic/status/UTF-8/BRE
vectors; malformed/overflow/budget/allocation cases; repeated stack/memory recovery.
Compare the portable subset with native expr, preserving named differences.

Then real kernel/I/O/LVGL KEY execution: lists/skips, quoting/fields/glob,
redirections, if/loops, dot/return, stdout/stderr backpressure and close errors.
Add a bounded VFS recipe and expected bytes without exceeding terminal4096bytes
or image/script16KiB limits. Run shell conformance/regressions and two identical
strict builds; update shell/kernel/I/O progress with exact evidence. Browser
manual/VM/device conformance remains separate. Include reproducible shell.wasm
and generated sources locally; no commit/push, npm or version changes.

Reference: [OpenBSD expr manual](https://man.openbsd.org/expr.1) for utility
structure/precedence/result convention; [OpenBSD implementation](https://github.com/openbsd/src/blob/master/bin/expr/expr.c)
for observed eager logical evaluation. No source is copied into production.
The earlier feasibility observations and portability distinctions are retained
in SHELL_EXPR_DESIGN.md. The owner explicitly granted AGENTS4.6 execution approval on 2026-11-02.

## Approved quota clarification — 2026-11-02

The first strict native ASan/UBSan run reproduced a resource rejection for
`expr abc : 'a.*'`. The published compiler explicitly checks
`sizeof(BreCompile) <= max_workspace_bytes` before parsing. Its native Clang
C++98 context is361,592 bytes, already larger than the proposed64KiB. This is
fixed frontend storage, not expression recursion or a failed match.

Owner-approved adjustment: supply384KiB (393,216 bytes) as the BRE compilation workspace
limit, then execute with the original64KiB workspace limit. Programs are compiled,
executed and freed serially; retain95-byte input/patterns,384instructions/ranges,
128frames/10,000 matching steps, five calls,32shell pages/four handles. Generated
lexer/parser allocations remain separately finite/measured as documented by the
library;384KiB is not a promise covering all allocator overhead. No compiler/
library edit, ABI/syscall/worker or manifest enlargement. Native and actual-WASM
resource/cleanup tests must demonstrate the complete retained memory envelope.
The owner explicitly approved this adjustment and its tests on 2026-11-02. Production expr supplies the two distinct quotas; full native/WASM and shell integration validation remains required.

## Functional acceptance — 2026-11-04

The approved bounded builtin now executes in shell.wasm. Native, own-WASM,
allocation-failure, production stream/VFS and real KEY/kernel/I/O/LVGL tests pass.
See [exact commands, results, artifact and retained limitations](SHELL_EXPR_VALIDATION.md).
Working-tree implementation is separate from publication; no OS commit/push.
