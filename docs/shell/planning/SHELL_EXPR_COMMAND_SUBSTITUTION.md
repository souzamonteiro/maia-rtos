# Expr command substitution — proposed execution contract

Date:2026-11-04. Status: explicitly approved by the owner on2026-11-04; implementation in progress.
Baseline:main b13f168 plus the validated local expr checkpoint described in
[SHELL_EXPR_VALIDATION.md](SHELL_EXPR_VALIDATION.md). This proposal supersedes
only the previous exclusion of the exact bounded command-substitution profile
below. It is not general subprocess/subshell execution or arithmetic expansion.

## Purpose and selected syntax

Enable standard command-substitution notation to use the existing guest expr:

```sh
N=1
N=$(expr "$N" + 1)
echo "$N"
printf '[%s]\n' "$(expr 'ação 😄' : '.*')"
N=0; while test "$N" -lt 3; do N=$(expr "$N" + 1); echo "$N"; done
```

Permit `$()` in assignment-only RHS and ordinary argument words, with or without
an enclosing double quote, including adjacent literal/parameter parts. Single
quotes and escaped dollars preserve literal text. At most two substitutions
per selected simple-command unit; one nesting level, one simple command in each.
After ordinary generated expansion/field splitting/glob, the inner command name
must be exactly `expr`. Quoted or variable-supplied `expr` names are allowed;
there is no dispatch to any other utility. The existing12 argv slots/95-byte
values apply independently to the child expression and the outer command.

Exclude substitution in outer command-name words, redirect operands, for-list
headers, parameter-default/alternate/removal operands, and all nested `$()` or
backticks. Exclude child assignments/redirections/lists/pipelines/compounds,
comments and physical newline syntax. Expr grouping is quoted/escaped argv,
not shell grouping. This is an explicitly restricted first substitution profile,
not a special variable-assignment API or a claim of full POSIX substitution.
Unsupported selected structures produce an ordinary expansion error before
outer command dispatch/assignment/redirection; skipped list/branch units do not
execute substitutions. Literal single-quoted occurrences are never rejected as
executable substitutions. Existing behavior outside this profile is preserved.

## Grammar and component ownership

Change the authored main-shell lexical fragments and the generated word-parts
EBNF together. Finite nonnested fragments must own complete `$()` word boundaries,
inner quotes and the closing parenthesis. A generated child-command tree owns
inner command/argument positions; no handwritten delimiter matching, quote
scanner, regex extraction, string splitting or synthetic argv from joined text.
Use MaiaCC/tREx exclusively and compare generated JS/C++98/own-WASM traces before
execution. Never hand-edit generated parsers. Expr still receives final argv
through its existing generated expression frontend and checked evaluator.

Use separate bounded guest scratch for outer word and child command/word trees.
Refactor word rendering only enough to select explicit outer/child storage;
never overwrite the active outer tree or copy dangling collector pointers.
Child rendering reuses existing parameter/quote/field/glob semantics, with
substitution disabled structurally. Storage and invocation are nonrecursive.
The outer unit plan, dot frame, loop/if control, selected status and pending
return/break/continue request remain in their owners and cannot be replaced by
child state. No reentry into the top-level shell scheduler/dispatcher.

The side-effect-free expr evaluator is reusable guest code. Factor only its
small builtin output/status wrapper so ordinary dispatch and capture use the
same result/error behavior. A private bounded guest sink receives its stdout;
child stderr uses existing pending stderr and normal backpressure. The host
transports bytes only, and never executes expressions or manages child syntax.
The expr profile neither writes variables/cwd nor reads stdin/opens output files.
Its input expansions may perform existing read-only filename enumeration, with
ordinary authorization and cleanup. It inherits parent variable/cwd/status data
read-only; this restricted execution requires no new kernel process or stream
handles. General commands with mutable/async/process semantics remain excluded.

## Captured bytes, field provenance and status

Capture the complete successful expr result plus its normal LF in96 bytes
(max95 result bytes plus LF). Strip all trailing LF bytes as command-substitution
semantics, retaining CR, embedded LF and exact UTF-8. Reject capacity exhaustion
normally; never truncate or print partial captured stdout on the terminal.
Append the result using existing95-byte outer-word bounds. Assignment RHS never
splits/globs captured output. Double quotes preserve one argument, including an
empty result. Unquoted capture bytes alone become field-splitting eligible and
then follow existing per-field MaiaGlob rules; they cannot become shell syntax.
An empty unquoted result can elide/promote ordinary argument fields as today.

Each selected outer unit retains its existing incoming-status snapshot. Inner
`$?` and outer `$?` use that snapshot; completing a substitution records a separate
last-capture status, without mutating it. This matches observed dash behavior;
macOS `/bin/sh` and `/bin/bash` instead expose a changed status in a later outer
word in one reference case. Preserve this documented selected MaiaOS policy.

An assignment-only command that expanded substitutions stores the resulting
value and returns the last substitution's completed expr status. Without a
substitution it keeps existing status0. A normal outer command returns its own
status even when a substitution returned nonzero. In particular:

| Input (following N=keep where applicable) | Result | Status |
| --- | --- | --- |
| `N=$(expr 2 + 3)` |N becomes5; no stdout|0|
| `N=$(expr 0)` |N becomes0; no stdout|1|
| `N=$(expr '')` |N becomes empty; no stdout|1|
| `N=$(expr 1 / 0)` |N becomes empty; existing expr stderr diagnostic|2|
| `N=$(expr 2147483647 + 1)` |N becomes empty; existing resource stderr|3|
| `N=$(expr 1)$(expr 0)` |N becomes10|1|
| `echo "$(expr 0)"` |prints0 and LF|0|
| `false; printf '[%s]/%s\n' "$(expr "$?")" "$?"` |prints[1]/1 and LF|0|

A syntactically supported child expr's statuses0/1/2/3 are command results, not
outer-expansion failures. A failure of capture transport/syntax/support/outer
capacity is an expansion failure: do not store the assignment or dispatch the
outer command. Existing variable-table failures remain atomic. Stderr already
produced by an earlier executed substitution is not transactional and must drain
normally if a later expansion fails. Ordinary later outer open/write/close errors
retain existing completion precedence; no rollback of completed assignments.
Negation/lists/if/loop/dot use the final outer completed status as before.

## Bounds and scheduling

Keep255-byte source unit,12 argv slots/95-byte values,32 variables, eight outer
leaf units/32 instructions, existing loop/dot bounds and16-step quantum.
At most two child evaluations per selected leaf, thus at most16 substitutions
per full eight-leaf execution pass (loops remain independently bounded).
Each child command uses at most32 tokens/1024 nodes; its word renderer reuses
existing256-token/2048-node bounds in separate scratch. One96-byte capture buffer
is reused serially. All storage counts are fixed and native/guest sizes must be
measured before accepting the complete build.

Each expr retains its4096 semantic-step limit and at most five serial BRE calls,
384KiB compile/64KiB match workspace,10000 matching steps per call. Thus two
captures add at most8192 semantic steps/100000 matching steps per selected leaf,
plus separately finite/measured generated parsing and existing glob work. No
universal CPU-time claim. Preserve32 shell pages,8 I/O pages and four open-file
handles; if the linked implementation does not fit, stop and report it rather
than changing manifests. All child parser/program/enum storage must release on
success, syntax/resource/encoding/allocator failure and parent termination.

## Acceptance and scope of approval

The owner explicitly approved this execution contract before production edits.
Implementation includes authored grammars, generated artifacts, private bounded
C++98 capture/rendering/status integration and tests. No compiler/library owner
edits, new syscall/capability/worker/process model, gitlinks, versions, npm,
commit/push or general-purpose shell worker are included.

Validate independent generated JS boundary/quote/child-role trees first, then
exact native C++98 ASan/UBSan and actual own-chain WASM tokens/UTF16 spans/trees,
results/statuses and canaries. Preserve all previous expression/word/grammar
fixtures. Native dash/Bash observations are references only; keep named expr
numeric/Unicode/BRE differences and the status-snapshot divergence visible.

Test counter loops, multiple captures/order, child/outer variable ownership,
empty/space/UTF-8/glob output, concatenation, LF stripping, status-only assignment
and ordinary outer-command status, eager expr errors, skipped branches, excluded
positions/forms, exact quotas, allocation failures, stable memory and no VFS or
stream leaks. Exercise delayed stderr/backpressure and close failures, suspended
dot/return/loop control without replay, and all existing shell regressions.

Add read-only VFS recipe19 and expected responses within current image/display
bounds. Run commands by KEY and guest dot through real kernel/I/O/shell/LVGL,
with kernel scheduling preserved. Reproduce generated artifacts and two strict
AST-C++98 -> C89 -> own-WASM builds; retain local production shell.wasm. Update
shell/kernel/I/O progress and handoff with exact outcomes. Browser manual and
VM/device conformance stay separate. A feasibility/reference probe is not
implementation acceptance.
