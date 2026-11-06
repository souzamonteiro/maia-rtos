# Bounded foreground command lists — approved implementation contract

Date: 2026-10-26. Status: owner approved this contract; implemented and validated locally.
Baseline: main b13f168 plus the existing locally validated shell increments.
No publication is included. The production shell guest and generated artifacts remain local changes.

## Objective

Execute foreground lists with `;`, `&&` and `||` using the existing generated
MaiaCC syntax tree. This is a prerequisite for script-file execution, not a
script reader or a complete POSIX shell implementation.

Examples within the existing command profile:

```sh
printf 'hello\n' > /tmp/a; cat /tmp/a
cat /missing && echo skipped; echo continued
cat /missing || echo recovered
cd /tmp && printf 'data\n' > a; cat /tmp/a
cat /system/welcome.txt | wc -l && echo counted
```

## Syntax and bounded planning

- Keep the current EBNF, generated lexer/parser, token transport and tree
  collector. Build an iterative execution plan from AST ancestry and terminal
  ownership; never split source strings on operators or add a handwritten parser.
- Support at most eight pipeline units per physical input line. A unit is one
  currently supported simple command or one existing two-to-four-worker pipeline.
  Preserve the 255-byte input, 32-token, 1024-node, 95-byte word and twelve-word
  per-unit expansion limits. Reject excess rather than silently truncating.
- Follow AST precedence: pipelines bind before AND-OR; `&&` and `||` have equal
  precedence and evaluate left to right; `;` starts the next unconditional unit.
  A trailing semicolon remains valid. Physical newlines keep the current input
  reader behavior; multiline continuation and buffered script parsing are deferred.
- Validate the entire line's supported AST structure and static bounds before
  any command effect. Reject background `&`, negation `!`, assignments, compound
  commands and unsupported redirection structures anywhere in the line, including
  a branch that would be skipped. Do not silently execute a supported prefix.
- Preserve quoted/escaped operators as word content according to the generated
  tokens/tree. Keep the remaining contextual reserved-word limitations explicit.

## Selection and command status

Keep an internal status for the last executed unit. A skipped unit leaves it
unchanged. Execute the right side of `&&` only for status zero and the right side
of `||` only for nonzero status. Thus `a && b || c` follows left-to-right selection.
An unconditional separator always selects the next unit after ordinary failure.

| Outcome | Internal status |
| --- | --- |
| Successful builtin, file operation or expansion/execution | 0 |
| VFS, stream, glob enumeration or other operational failure | 1 |
| Usage, unsupported command profile or runtime argument/expansion limit | 2 |
| Command name not found | 127 |
| Supported worker pipeline | Existing kernel completion status at packet offset 12 |

The existing kernel packet already reports the final worker's exit status, or
failure when rollback/trapping failed the group. Preserve that authority; no
`pipefail` mode or host-side status policy is introduced. A shell-owned input,
output or close failure overrides an otherwise successful pipeline with status 1.
For multi-operand cat/ls, any operand failure keeps the unit nonzero even when
later operands succeed. Distinguish ordinary command failure from fatal process
transport errors, which continue to stop the shell.

These statuses are internal to list selection in this slice. `$?`, variables,
numeric `exit` operands and process-exit status changes are deferred. Keep the
current bare `exit`/EOF lifecycle contract; `exit` stops remaining units.

## Ordering and resource ownership

- Select a unit before glob expansion or redirection opens. Skipped units must
  not enumerate, create/truncate files, change cwd or start workers.
- Expand each selected unit when it runs, so a preceding `cd` or file creation
  affects its relative paths/glob results. Do not pre-expand the whole list.
- Preserve current per-unit expansion checks and source-order redirect opens.
  Runtime failure can occur after earlier units have succeeded; do not roll back
  those effects or misrepresent whole-line structural validation as transactional
  execution. A failed redirection retains existing open/truncation semantics.
- Keep the input/tree storage alive for the complete plan. Do not consume the
  next physical input line while a list remains active.
- Advance only after stdout/stderr drain, owned redirect/file/directory handles
  close and worker groups complete/reap. Preserve WOULD_BLOCK yielding and bounded
  step budgets; no recursion or synchronous polling loops.
- Emit one prompt after the full list, with no intermediate prompts. Preserve
  no-final-newline output, keyboard editing/history and real LVGL rendering.

## Files and architectural boundaries

Expected changes: shell.cpp plus a bounded shell list-planner helper as needed,
shell-glob.hpp/shell-pipeline.hpp completion and status plumbing, shell.wasm,
focused tests, simulator recipes/expected data, README/checklist and handoff.
No parser generation change is expected. Kernel, I/O, cat/wc workers, capabilities,
syscall signatures and binary process plans remain unchanged. All command effects
stay in the C++98 shell guest through existing VFS/stream/process imports; the
host transports bytes and real I/O/LVGL renders the terminal.

If a compiler/generator defect or additional public contract is required, report
the reproducer and obtain the applicable approval before changing its owner.
No compiler/dependency edits, commits, pushes, npm release, version changes or
history operations are included in this proposal.

## Required validation and acceptance

- [x] AST planner cases: precedence, trailing separator, quoted/escaped operators,
  exact bounds, rejection before effects for unsupported/oversized whole lines.
- [x] Native C++98 ASan/UBSan for any independently testable planner/status helper.
- [x] Build actual shell through MaiaCpp -> C89 -> MaiaC/MaiaWASM, with C89 checks.
- [x] Real WASM list output/side effects compared with explicit expectations and
  a native POSIX shell for the selected supported profile.
- [x] Short-circuit chains, unknown command, builtin usage, printf errors, cat/ls
  multi-operand failures and redirection/glob failures select correct branches.
- [x] Skipped branches cause no glob enumeration, file changes or worker launch;
  selected later units observe prior cwd/file changes.
- [x] Real kernel cat/wc pipelines inside lists: status selection, partial writes,
  blocked input/output, cancellation, close failures, reaping and no handle leaks.
- [x] Existing parser/lexer equivalence and shell files/glob/directories,
  redirection/pipeline, LVGL/boot and unit regressions remain passing.
- [x] Repeat lists with stable memory/stack and resources; EOF and exit during
  list execution do not execute another input line or leave live children.
- [x] Simulator VFS recipes and shell/kernel/I/O evidence reflect actual tested
  behavior, while script-file execution and other pending features stay unchecked.

Interactive browser acceptance, MaiaWASMVM/device execution and broader POSIX
conformance must be reported separately from automated reference WASM/LVGL tests.

## Completed local evidence — 2026-10-26

Implementation: shell-list.hpp consumes generated preorder Pipeline subtrees and
operator ownership; shell.cpp retains the tree and advances a bounded plan only
after asynchronous completion. Existing per-unit word decoding performs a static
whole-line preflight, then selected units decode/expand in the current cwd.
Status plumbing covers builtin/printf/operand/glob/VFS failures, delayed close
errors and the kernel's existing completion packet. The host, kernel, I/O and
workers were not changed for this increment. `shell.wasm`: 1,295,720 bytes.

Passing commands:

- `npm run test:shell-lists-native`: 16 actual generated-tree cases, C++98
  ASan/UBSan, owned ranges and complete zero/nonzero selection table.
- `npm run test:shell-lists`: actual guest, 72 differential AND-OR/sequence
  selection cases against /bin/sh, bounds/whole-line rejection, zero skipped
  enumeration/opens, cwd/file visibility, ambiguous/overflow glob targets,
  sticky multi-operand/read/write/close errors, 150 repeated lists, EOF/exit/fatal
  and blocked-output ordering; stable memory/stack and no leaked handles.
- `node scripts/test-printf-integers-native.mjs`: existing formatter sanitizer
  and byte/integer differential regression after returning success/failure.
- `npm run build:shell -- --check-c89`: own C++98/C89/WASM chain; artifact above.
- `npm run test:shell-grammar`, `test:shell-parser`, `test:shell-equivalence`:
  existing syntax/context-gap checks, generated raw-input parity/recursion and
  5,833 JS/native/WASM cases (2,856 accepted, 2,167 syntax, 810 lexical failures).
- `npm run test:shell-redirection`, `test:shell-files`, `test:shell-glob`,
  `test:shell-directories`, `test:shell-pipelines`: existing regressions plus
  successive groups, final-worker status, failure short-circuit/reaping and the
  four VFS recipes (28 commands, each run twice through real KEY/I/O/LVGL).
- `npm run test:shell-lvgl`: 1,000 repeated parses, stable shell19/I/O2 pages,
  77,911 kernel steps. `npm run test:shell-boot`: 25 sessions, clean teardown.
- `npm test`: 66 passing tests, including image isolation/atomic mounting.

The four simulator recipes are readable test data, not executable script files.
Interactive browser and MaiaWASMVM/device acceptance are not claimed. No commit,
push, dependency changes, npm publication, version change or history operation.

## Conditional execution follow-up — 2026-10-31

The separately approved [if contract](SHELL_IF_EXECUTION.md) adds bounded one-line
nonnested conditionals while preserving8 total leaf units and current interfaces.
A private32-slot forward plan now saves the caller's next instruction PC across
dot reconstruction, without replay or another tree/branch stack. Conditional
status/negation, actual WASM/kernel/I/O/LVGL and existing regression gates pass.
The original exclusions/results above describe their earlier approved slices;
loops, nested/multiline compounds and broader POSIX conformance remain pending.
