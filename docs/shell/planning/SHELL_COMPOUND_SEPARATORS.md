# Generated compound-command separator correction

Status: implemented and validated locally, 2026-10-24. The owner requested
continued implementation. This increment fixes the documented separator gaps
using the existing MaiaCC pipeline and MaiaOS interfaces only. Valid compound
execution and contextual reserved-word classification remain pending.

## Scope

Require a terminating separator after final simple commands and redirected
compounds before closing keywords/braces. Unredirected compounds may delimit
themselves: native syntax accepts `if (true) then (echo hi) fi` and nested `fi fi`.
Preserve optional trailing separators inside `(subshell)`.
Require a sequential separator after an explicit `for ... in ...` word list,
while preserving native-valid `for x do ...; done`, `for x; do ...; done`,
newlines before `in`, and empty explicit word lists.

Keep the existing CompoundList for subshells. Add TerminatedCompoundList with
factored command/tail rules for brace/keyword bodies: final simple/redirected
commands require a connector or separator, unredirected compounds may close.
Factor before descending into a compound so successful nested compounds are
not reparsed for alternative endings. Body tails have linear depth in body
length; measure calls/depth at 8/32/128 commands/nesting, including actual WASM,
and retain production input/token/tree bounds. Existing top-level flat loops
must remain constant-depth. Factor for-clause alternatives so each branch
includes DoGroup; do not rely on retrying a successful inner choice after failure
in its caller.

Edit only the shell EBNF, regression fixtures/checks, generated artifacts and
documentation. No handwritten scanner/parser or edits to generated code. No
MaiaCC/compiler/dependency change, syscall, ABI, capability, kernel/I/O execution
model or rendering change. No commit/push, versions or npm publication.

Parsing a valid compound command still does not execute it: the bounded shell
must continue reporting unsupported syntax before file opens. Invalid forms
must report syntax errors before command execution or file mutation. Contextual
reserved words, here-documents, functions, case and general POSIX conformance
remain separate pending work.

Independent native syntax checks also caught two incorrect draft expectations:
`if true; then echo yes else echo no; fi` is valid because else is an argument,
and `if true; then if false; then echo hi; fi fi` is a valid compound ending.
The former is retained as an explicit contextual-lexer gap; the latter is a
mandatory valid regression. Neither is relabeled invalid to simplify the fix.

## Acceptance

- Independently specified valid/invalid fixtures checked with `/bin/sh -n`
  (syntax validation only; never execute the reference input).
- JavaScript grammar/collector validation first, then native raw C++98 lexer/
  parser and actual C++98 -> C89 -> WASM comparison of tokens, offsets and trees.
- Missing separators in all implemented compound forms reject; semicolon,
  newline and permitted asynchronous separators remain accepted. Empty bodies
  reject, nested compounds retain correct boundaries and subshells remain valid.
- Real shell WASM distinguishes invalid syntax from valid-but-unsupported
  compounds, preserving target files and handles. Real kernel/I/O/LVGL path.
- Regenerate versioned JS/C++/XML and production header/shell.wasm from the EBNF.
  Strict C89 build, unit/boot/file/glob/directory/redirection/pipeline regressions,
  recursion profiles, stable stacks/memory and updated progress checklist.

Initial evidence: the native `/bin/sh -n` rejects `if true then echo yes; fi`,
`{ echo hi }`, `while false do echo x; done` and `for x in a do echo hi; done`.
It accepts `(echo hi)` and `for x do echo hi; done`; requiring every for-clause
header or every subshell body to end with a semicolon would be incorrect.

## Completed verification

- Before-fix evidence in `/tmp/maiaos-compound-reproduction.log`: the previous
  generated parser accepted all four malformed examples above while native
  syntax rejected them. Draft expectation errors were caught by the independent
  native oracle and corrected before the grammar implementation.
- `npm run test:shell-grammar`: 102 `/bin/sh -n` expectations pass (100 selected
  grammar cases and two native-valid contextual gaps). Parsing/collector checks
  pass; missing separators reject and native-valid endings remain accepted.
- `node apps/shell/rtest.js --cpp`: all 103 JS/native cases and full trees match;
  versioned comparison JS/C++/XML regenerated through pinned MaiaCC.
- `npm run test:shell-parser`: 111 raw-input JS/native cases and 27 recursion
  profiles pass. Existing flat words/lists/pipelines remain at depth 11 and keep
  their original rule-call budget. New compound profiles have <15 calls/token,
  non-growing calls/level across 8/32/128 sizes, no zero-progress cycles, and
  peak depth <=7n+12. Body tails are right-recursive: linear depth, not constant.
  At 128, body-list peak400/calls2899; body-pipeline peak273/calls2391;
  nested peak658/calls8755; incomplete nested peak658/calls8099. The new nested
  profile contains multiple commands per level; its token-based budget replaces
  a draft n-only budget, with an independent growth check added.
- `npm run test:shell-equivalence`: 5785 actual JS/native C++98/WASM raw inputs,
  2798 accepted, 2167 syntax errors and 820 lexical errors. Exact tokens, UTF-16
  offsets and complete trees; restored WASM stack, including deep/flat bodies.
- `npm run build:shell -- --check-c89`: own C++98 -> strict C89 -> MaiaC/MaiaWASM;
  shell.wasm 1287425 bytes. Production bounds unchanged: 255 input bytes,
  32 tokens and 1024 tree nodes; increasing them requires revisiting stack/work
  budgets. No generated source was manually edited.
- `npm test`: 64 passed. `npm run test:shell-redirection`: actual guest syntax
  errors and valid-but-unsupported compounds both preserve target bytes and
  leave no open handles, with existing binary/backpressure/error cases retained.
- `npm run test:shell-lvgl`: real kernel/I/O/LVGL, new syntax/unsupported output
  distinctions, 1000 repeated parses, stable shell18/I/O2 pages and stacks.
  `npm run test:shell-pipelines`, `test:shell-boot`, `test:shell-files`,
  `test:shell-glob`, `test:shell-directories`: all pass on the rebuilt guest.

Logs: `/tmp/maiaos-compound-{grammar,parser,artifacts,equivalence,build,unit,
redirection,lvgl,pipelines,boot,files,glob,directories}.log`.
Reference execution uses native WebAssembly and real LVGL; interactive browser,
MaiaWASMVM and physical-device validation were not run. No dependency changes,
commit/push, release/version/npm or execution-model changes. All prior local
work and staged published dependency pins are preserved on main b13f168.
