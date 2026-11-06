# Simple braced parameters — approved contract

Date: 2026-10-27. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the validated local unquoted-fields checkpoint.
This extends SHELL_LOCAL_VARIABLES.md and SHELL_UNQUOTED_FIELDS.md, which
explicitly deferred braced parameter execution. Approval covers this implementation and validation, without publication.

Follow-up: [SHELL_PARAMETER_LENGTH.md](SHELL_PARAMETER_LENGTH.md) is separately
approved and implemented for exactly `${#NAME}`. Its explicit Unicode scalar
policy supersedes this original length exclusion; other operators remain
unsupported/opaque as documented. Evidence below records this original slice.

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

## Selected behavior

Add exactly `${NAME}` and `${?}`, in ordinary words, double quotes, assignment
RHS values and existing redirect targets. NAME uses the existing generated ASCII
variable-name rule and 31-byte bound. Braces delimit the name, so
`"${NAME}_backup"` uses NAME rather than NAME_backup. Unset names expand empty.
`${?}` uses the same incoming completed-status snapshot as bare `$?`.

Single-quoted and backslash-protected dollar forms remain literal. Expanded
bytes remain data: no syntax rescan, recursive expansion or quote interpretation.
Unquoted named values use existing fixed space/tab/LF splitting followed by
per-field MaiaGlob; quoted values retain quote masks/positional empty anchors.
Assignment RHS and redirect targets remain unsplit under their existing rules.
Empty-command handling, pipeline stage ownership, redirect ambiguity and all
current cleanup/status policies remain unchanged.

Only the two complete forms gain execution meaning. Preserve generated opaque
ownership of unsupported quoted braced text, including nested/unfinished forms:
`"${NAME:-$OTHER}"` stays literal with no inner expansion, as at the current
checkpoint. Unquoted unsupported/incomplete forms remain rejected by the main
frontend. Do not accidentally accept or partially expand a simple prefix inside
an unsupported expression. This compatibility policy is bounded project behavior,
not full POSIX parameter-expansion conformance.

Length `${#NAME}`, default/alternate/error/assignment operators, prefix/suffix
removal, indirect/positional parameters, custom IFS, environment/export,
substitution and compound execution remain outside this increment.

## Generated syntax and integration

- Extend maia-shell.ebnf WORD with generated complete simple-braced forms,
  including NAME and status, without making standalone braces ordinary word
  characters. Preserve brace-group syntax, NAME/ASSIGNMENT_WORD/IO_NUMBER
  priorities, adjacent operators, Unicode spans and existing rejection cases.
- Extend maia-shell-word.ebnf with explicit braced name/status nodes before the
  opaque alternative. Semantic rendering consumes these generated nodes using
  existing variable/status logic and descendant ownership; no handwritten
  dollar/name/brace scanner and no manual generated-source edits.
- Regenerate both frontends with MaiaOS's own MaiaCC/tREx dependency. Compare
  JavaScript first, then native C++98 raw lexer/parser and actual WASM.
- Preserve static whole-line preflight and dot caller reconstruction: validate
  syntax/name limits without reading bindings or performing expansion/VFS effects.
  Expand only selected units, using one status/table snapshot per unit.
- Preserve line255, word95 bytes before splitting, 12 original/final word slots
  including targets, 32 variables, 256 word tokens, 2048 word nodes and current
  manifest memory/handle budgets. Overflow follows existing bounded diagnostics.

Affected scope: MaiaOS EBNFs and generated frontend artifacts, shell-word.hpp,
word/main frontend tests and probes, focused guest integration tests, simulator
recipe/expected assets, shell.wasm, progress docs and handoff. Kernel/I/O production
code, syscalls, worker plans and compiler owner repositories are unchanged.
If a toolchain defect prevents this path, report it with a minimal regression
before proposing a separately approved owner-repository change.

## Acceptance gates — completed locally

- [x] Independently expected generated JS/native/WASM tokens, UTF-16 spans and
  trees for simple forms, adjacency, assignments, quotes, escapes, Unicode and
  malformed/complex forms; standalone brace groups retain their trees.
- [x] Native C++98 ASan/UBSan renderer and actual own-chain strict-C89/WASM probes
  agree on bytes, quote masks, split provenance and empty anchors. Compare
  supported expansion/argv cases against native dash with explicit expectations.
- [x] Exact name31/32, word95/96, source/final12/13, token/tree bounds and deep
  legacy opaque recursion retain bounded failure and restored guest stack.
- [x] Real guest commands cover name/suffix separation, unset/empty, literal
  inserted dollars, quoted/unquoted glob, assignment RHS, unsplit redirect paths,
  empty commands, selected/skipped AND-OR units, status negation, dot persistence
  and pipeline ownership. Invalid whole-line syntax fails before file effects.
- [x] npm run build:shell -- --check-c89; test:shell-word,
  test:shell-equivalence, test:shell-fields, test:shell-variables,
  test:shell-status, test:shell-lists, test:shell-scripts,
  test:shell-redirection, test:shell-glob and test:shell-pipelines pass.
- [x] Updated VFS recipes/expected bytes execute through real kernel, separate
  I/O/shell/worker guests and LVGL. test:shell-lvgl, test:shell-boot and npm test
  pass; record actual artifact hash/size and measured memory, not estimates.
- [x] Reproduction and scoped diff/index audits preserve unrelated dirty work and
  the three already staged dependency pointers; update relevant progress trackers.

No commit/push, npm publication, version changes or history rewrite is requested.
Browser acceptance and MaiaWASMVM/device validation remain distinct unrun gates
unless actually performed. Approval of this proposal authorizes implementation
and validation only, not any excluded language feature or publication.

## Read-only baseline evidence

Current generated JavaScript main parser rejects `echo ${NAME}`, `echo ${?}` and
`echo pre${NAME}post`; accepts quoted forms and `{ echo x; }` structurally.
Existing native generated word/renderer probes confirm quoted simple and complex
braced forms are currently literal. These are baseline probes, not implementation
or evidence of executing brace groups.

## Implementation and recorded results

Both EBNFs define complete generated braced name/status forms. Main WORD keeps
standalone brace tokens and contextual priorities. Word rendering consumes
BracedNamedParameter/BracedStatusParameter through existing bounded name/status
semantics; unsupported OpaqueBraced descendants are not partially evaluated.
No handwritten syntax scanner, host expansion or compiler-owner edit.

All commands exit0: npm run build:shell -- --check-c89 (1457603bytes,SHA256
31119c11d125b3767009e8649abddca516c0983521d8f7c2c17883d4466f28fa),
test:shell-grammar(125native syntax fixtures incl2known contextual gaps),
node apps/shell/rtest.js --cpp(126JS/C++trees),test:shell-parser(141rawcases,
27recursion profiles),test:shell-equivalence(5917cases:2904accepted/2203syntax/
810lexical;zero token/span/tree differences,stack restored),test:shell-word
(366JS/native/WASMrawcases,170regular byte/mask expectations plus seeded names/
assignments,66native/dash/WASMfield cases,ASan/UBSan,6reproducedwordartifacts).
Production main header equals separately regenerated maia-shell.cpp byte-for-byte.

test:shell-fields adds native dash bytes, suffix ownership, quoted/unquoted fields,
empty/opaque/malformed forms,31/32name and95/96byte bounds,12/13slot limits,
static whole-line/no-partial-file checks, lazy lists, dot return and200stable
braced cycles alongside400retained field cycles. Variables/status/lists/scripts/
redirection/glob/paths and31native sanitizer list plans pass unchanged.
Pipelines pass392LVGLflushes;fiveVFSrecipes61commands twice per-line and all
five twice by guest dot (recipe03 aborts at its deliberate line4syntax error).
Real shell/I/O/kernel/LVGL:1000parses,2173flushes,77911kernelsteps,stable
shell20/I/O2pages;boot25sessions/28flushes,zero surviving windows/endpoints.
npm test66pass. Logs:/tmp/maiaos-braced-*.log. All final processes completed.

Documentation/checklists/architecture and simulator expected bytes updated.
Rootmainb13f168 remains dirty/local;existing Cpp/Glob/Regex staged pointers
preserved. No commits/pushes/npm/versions/history change; no compiler/kernel/I/O
production edits or interactive browser/MaiaWASMVM/device validation.

Final audit:34scoped files changed from the task baseline; index unchanged,
all7principal repositories clean, rootversion0.1.0 unchanged, recipe sources
aligned with expected.json. Unrelated existing modifications preserved.

Suffix-removal follow-up (2026-10-30): named % / %% are separately approved and
validated; see [contract/evidence](SHELL_PARAMETER_SUFFIX_REMOVAL.md). Existing
semantics/regressions pass; operator nesting remains excluded. Root changes are
local, with published dependencies and no root commit/push/npm/version change.
