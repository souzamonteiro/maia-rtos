# Unquoted named parameters and bounded field splitting — approved contract

Date: 2026-10-27. Status: explicitly approved; implemented and validated locally.
Baseline: main b13f168 plus the validated local-variable/UTF-8 checkpoints.
This extends SHELL_LOCAL_VARIABLES.md, which explicitly excluded this behavior.
Approval covers the selected implementation, tests and production artifact; no publication.

Follow-up: [SHELL_BRACED_PARAMETERS.md](SHELL_BRACED_PARAMETERS.md) is explicitly
approved and implemented. It supersedes the original exclusion/literal behavior
for exactly `${NAME}` and `${?}`. Other quoted complex/unfinished braced forms
stay opaque; unsupported unquoted forms still reject. Historical evidence and
bounds in this contract remain scoped to their original checkpoint.

## Selected execution contract

Allow ordinary unquoted `$NAME` in command names/arguments, dot operands and
existing pipeline stages. Keep maximal generated ASCII name recognition. Perform
parameter expansion, semantic field splitting, then existing MaiaGlob filename
expansion; never parse expanded bytes as shell syntax or another parameter.

```sh
FILES='/system/tests/data/a.txt /system/tests/data/b.txt'; cat $FILES
WORDS='one two'; printf '<%s>\n' $WORDS
WORDS='one two'; printf '<%s>\n' "$WORDS"
CMD='printf <%s>\n'; $CMD one two
```

The FILES example uses existing simulator image assets; recipe updates will
exercise the same bounded behavior through the real guest.

- Split only unquoted expansion-origin ASCII space, tab and LF bytes. Source
  quoting/escapes, literal bytes and quoted expansions are not split. Collapse
  delimiter runs without creating implicit empty fields. NBSP, CR and other
  Unicode whitespace remain ordinary bytes; no Unicode normalization.
- Retain the existing policy that local IFS is an ordinary variable. Delimiters
  are fixed to space/tab/LF in this slice; custom/empty IFS, locale-dependent
  splitting and special-variable/environment semantics remain deferred.
- Unset/empty unquoted references can remove a word entirely. Quoted empty
  fragments retain an empty field at their position, including when adjacent to
  expanded whitespace. Do not implement this with one retain-empty flag for an
  entire word. Concatenated expansions and literal prefixes/suffixes compose
  before field boundaries are finalized.
- Concrete expected argv examples (dash verified; no glob metacharacters):
  A='x y', `pre$A'post'` -> [prex, ypost];
  A=' x y ', `pre$A'post'` -> [pre, x, y, post];
  A='x ', `$A""` -> [x, empty];
  A=' x', `""$A` -> [empty, x];
  A=' ', `""$A""` -> [empty, empty];
  A='a b', B='c d', `$A$B` -> [a, bc, d].
  Single/double quoted empties and empty quoted parameters follow the same rule.
- All unquoted fields enter the existing MaiaGlob stage independently, preserving
  their byte-protection masks. Wildcards introduced by unquoted values can match;
  quoted ones stay literal. Retain current sorting per field, unmatched-pattern
  fallback, hidden-name behavior and finite limits. This does not redefine the
  MaiaGlob pattern language: its backslash/class/invalid-pattern rules remain in
  force, including diagnostics for invalid unprotected patterns. Backslashes
  inserted from values do not become shell quoting or prevent field splitting.
  No claim of complete POSIX pathname-pattern equivalence is added.
- Inserted quotes, dollar text, semicolons, pipes and assignment-shaped text are
  data. A first surviving field becomes the command name without reclassification
  as an assignment, reserved word or new syntax. `unset [--] $NAMES` still validates
  every final operand before any mutation; invalid/oversized argv is atomic.
- Existing assignments still produce exactly one bounded RHS value, with no
  splitting or glob. Quoted named references, `$?`, opaque braced forms and
  unsupported positional/substitution forms retain their selected contracts.

## Redirects, empty commands and pipeline ownership

Redirect targets apply parameter expansion/quote removal and their existing
MaiaGlob path handling, with exactly one final path required. They do not undergo
field splitting: FILE='a b' followed by `printf data > $FILE` targets one pathname.
Retain current project-specific redirect glob/ambiguity behavior; this is not a
new claim of POSIX noninteractive redirect pathname-expansion conformance.
Empty targets retain existing invalid-path handling. Multiple glob results remain
ambiguous; no output is created/truncated before all expansion validation succeeds.

A syntactically present command word can disappear completely during expansion.
If a selected simple-command unit has no remaining argv, it succeeds with status0
after any valid existing redirections are opened/closed. `!` and AND-OR completion
apply normally. An explicitly quoted empty command name remains a real empty argv
entry and follows command-not-found handling. This does not add new literal
redirection-only syntax, prefix/multiple assignments or command substitutions.

Each emitted field keeps the stage of its original generated SimpleCommand.
An empty pipeline stage is a bounded unsupported-command failure, status2, before
redirect file opens or worker/group launch. Keep cat/final-wc profiles and all
existing pipeline plan formats, ownership, pumping, cancellation and cleanup.

## Bounds, preflight and integration design

Preserve physical255-byte lines,8 list units,4 pipeline stages,256 word tokens,
2048 word nodes,32 variables/31-byte names/95-byte values,12 input word slots
including redirect targets and12 final slots including targets. Preserve the
95-byte rendered-word bound *before* splitting, even if a longer value could
otherwise fit after splitting. Every emitted field also stays at most95bytes.
No manifest,32-page/four-handle budget, host ABI or worker package changes.

Static whole-line preflight/caller reconstruction must validate generated syntax,
name/static literal bounds and original word slots without reading bindings,
splitting speculative values, glob enumeration or mutations. Expand only selected
units against one status/table snapshot. Field/word overflow is status2 through
normal completion before redirects/dispatch; post-glob overflow retains existing
cancellation/cleanup. Skipped units perform no variable lookup or VFS activity.

Use the existing generated word tree to render annotated data: quote protection,
expansion provenance and positional empty-fragment anchors. A bounded semantic
field helper consumes those annotations, not dollar/name/quote scans over source.
Only ordinary Boolean quote masks reach MaiaGlob; provenance bits must never be
mistaken for quote protection. The word/argv/stage/redirect adapter records every
result atomically before the cooperative glob stage. Reuse fixed storage; no
unbounded allocation, host expansion or handwritten syntax parser.

No EBNF change is currently expected: NamedParameter and quote nodes already
exist. If a demonstrated grammar change is needed, edit only the owning shell
EBNF and regenerate via its own published MaiaCC; compare JS/native/WASM raw
syntax/tree behavior before semantic/guest tests. Never edit generated artifacts.
Any compiler/library owner defect requires a separate minimal case and approval.

Expected files: shell-word.hpp, a bounded semantic field helper if useful,
shell.cpp/shell-glob.hpp adapter, native/WASM word probes and focused scripts,
package test entry, simulator recipe/expected data, shell.wasm and progress docs.
No kernel/I/O/host/ABI/compiler/dependency code changes. No commit/push, npm,
versions or history operations.

## Acceptance gates — completed within the selected scope

- [x] Independent expected field arrays and byte/mask/anchor boundaries; generated
  JS/native/WASM token/span/tree equality and unchanged artifact reproduction.
- [x] Native C++98 ASan/UBSan field helper; dash oracle for selected field cases,
  /bin/sh for unaffected common cases, explicit bounds/deferred-pattern diagnostics.
- [x] Empty/unset values versus quoted empties, leading/trailing/repeated eligible
  whitespace, mixed quotes/escapes, multiple expansions/prefix/suffix, UTF-8/NBSP,
  insertion without reinjection, status snapshot and assignment no-split/no-glob.
- [x] Exact95/96 byte pre-split limits,12/13 original/final slots including targets,
  glob growth and atomic failures; skipped values/effects, invalid unset atomicity.
- [x] Command-name elision/promotion, quoted empty command, zero-argv success with
  close/negation/status, redirected paths with spaces and existing glob ambiguity.
- [x] Stage mapping after zero/many fields, empty-stage rejection before launches/
  redirects, cat/final-wc options/operands, named input/output and all old failures.
- [x] Fresh own C++98 -> strict C89 -> MaiaC/MaiaWASM shell; actual guest variables,
  lists/dot caller restoration, paths/printf/glob/redirection/limits and EOF/cleanup.
- [x] Updated VFS recipes twice per-line and by guest dot; real KEY/kernel/I/O/LVGL,
  stable stack/memory/endpoints/handles across repeated expansions and25 sessions;
  main equivalence, word, status, variables, scripts, pipeline, boot and unit gates.
- [x] Progress READMEs/architecture/handoff and generated production artifact reflect
  the tested scope; no interactive browser/VM/device or full POSIX claim inferred.

## Investigation evidence and external references

Read current EBNF, renderer, argv adapter, cooperative glob implementation,
pipeline planner, variable regressions and architecture. Current renderer rejects
unquoted NamedParameter before expansion; current argv adapter assumes one result
per source word and rejects runtime zero-argv with redirects. These assumptions
need explicit updates under this contract; main syntax is already generated.

Native probes confirmed space/tab/LF handling, no NBSP splitting, prefix/suffix
composition, empty command status0 and noninteractive redirect paths with spaces.
For positional quoted empties, /bin/sh on this macOS is Bash3.2.57 and disagrees
with dash: A=' x', `""$A` loses the leading empty there, while dash preserves it.
Use independently specified expectations and dash for these cases, not a blind
comparison to the macOS default. The GNU Autoconf manual documents this family
of old-Bash quoted-empty splitting bugs. These are investigation results, not
implemented MaiaOS behavior or a passing product acceptance gate.

Sources (paraphrased, no verbatim excerpts):
- [GNU Bash word splitting](https://www.gnu.org/s/bash/manual/html_node/Word-Splitting.html)
- [GNU Autoconf shell substitution portability](https://www.gnu.org/software/autoconf/manual/autoconf-2.69/html_node/Shell-Substitutions.html)
- [GNU Bash filename expansion](https://www.gnu.org/s/bash/manual/html_node/Filename-Expansion.html)

POSIX.1-2017 remains the project's reference. Its online chapter returned403 in
this investigation; no inaccessible text was quoted or asserted as newly checked.
The proposed bounded choices above are explicit project scope, not full POSIX.

## Completed local implementation and evidence

No EBNF/generated parser changes. shell-word.hpp retains Boolean quote masks
and adds separate fixed96byte split/anchor arrays derived from generated nodes;
empty quoted contexts anchor their output boundary. shell-fields.hpp returns at
most12 fixed slices of rendered data. shell.cpp builds original-slot preflight
and selected field argv separately, preserves stage/redirect ownership, and
permits runtime zero argv. shell-pipeline.hpp rejects empty stages before opens.
No host or other repository changes; original input/main/word/manifest bounds
remain. Backslash/class behavior still belongs to existing MaiaGlob, including
its selected invalid-pattern diagnostic (not full POSIX matching equivalence).

All final gates exit0:

- `npm run test:shell-word`:335 JS/native C++98/WASM token/span/tree cases;162
  regular byte/mask cases plus seeded parameters/assignments;34 independent
  native/dash/WASM field expectations, including metadata anchors at byte95,
  Unicode/NBSP/CR/tab/LF, mixed fragments and95/96 pre-split/12/13 field bounds.
  Native ASan/UBSan and all6 generated-source hashes reproduce exactly.
- `npm run build:shell -- --check-c89`:own strict C++98/C89/MaiaC/MaiaWASM,
  shell.wasm1454114bytes,SHA256
  8ed7f9e539355b22b78930bd777c259e04bfb3b0386e6d012f3e3d172c3f7309.
- `npm run test:shell-fields`:dash oracles, actual guest quote/empty/name
  promotion/no-rescan, assignment data ownership, fixed IFS choice, unsplit
  redirects, original/final/glob limits before file effects, atomic unset,
  empty stages before launch, dot/caller persistence, session isolation,400
  stable cycles, zero-argv input/output late-close failures with/without negation
  and a final unquoted command at EOF. Initial format fixtures accidentally
  submitted physical LF; normalized test command lines to escaped printf LF,
  keeping native and guest inputs identical. No product workaround/relaxation.
- `test:shell-lists-native`:31 ASan/UBSan planner cases; retained
  `test:shell-variables`, `test:shell-status`, `test:shell-lists`,
  `test:shell-scripts`, `test:shell-redirection`, `test:shell-paths`
  (13+704 native/WASM path cases), `test:shell-glob` all pass. Only the obsolete
  rejection of now-supported unquoted variables changed; unsupported assignment
  forms and their pre-effect checks remain.
- `test:shell-equivalence`:unchanged5839 raw main inputs,2861accepted,2168syntax/
  810lexical errors,zero JS/native/WASM differences and restored stack.
- `test:shell-pipelines`:actual KEY/I/O/kernel/cat/wc/LVGL,stage promotion/empty
  guards and expanded options; five recipes/56commands twice per-line/all five
  dotfiles twice;354flushes and stable cleanup. Source image remains16files.
- `test:shell-lvgl`:2164flushes,1000parses,77911kernelsteps,stable shell20/I/O2
  pages. `test:shell-boot`:25sessions/28flushes,zero surviving windows/endpoints.
  `npm test`:66pass. No interactive browser, standalone IO/font/native kernel
  policy rerun, MaiaWASMVM or device evidence inferred.

New focused script/test entry and test-only native/WASM probes exercise the
production renderer/field helper. Simulator05/expected bytes and subsystem
progress docs updated. All changes/artifact are local on main b13f168; existing
dirty work and staged dependency pins preserved. No commit/push, npm publication,
versions or history operations.
