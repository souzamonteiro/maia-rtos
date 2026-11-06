# Generated shell IO_NUMBER adjacency

Later follow-up: the [compound separator correction](SHELL_COMPOUND_SEPARATORS.md)
regenerates the same frontend again; use that checkpoint and the shell README
for the latest artifact size and expanded regression counts.

Status: implemented and validated on 2026-10-24, after explicit owner approval.
Both MaiaCC generators preserve positive lexical context, including strict EOF.
All affected consumers have passed their gates and been published. MaiaOS has
consumed those published pins; its implementation, generated sources and
shell.wasm remain local, without commit/push. No versions or npm publication.

| Principal | Published final commit |
|---|---|
| MaiaCC | 3e4432b (fd324af ancestor) |
| MaiaWASM | cf84121 |
| MaiaC | 8354034 |
| MaiaCpp | 7886420 |
| MaiaJS | 7c43a22 |
| MaiaGlob | a736320 |

## Problem and verified evidence

The shell currently classifies all unquoted digit-only words as IO_NUMBER.
Consequently `printf '%d' 255 > /tmp/numbers` treats 255 as an explicit file
descriptor and is rejected by the bounded executor. Numeric redirection targets
such as `cat < 123` are also classified incorrectly. Quoting avoids the defect.

A temporary copy of the shell grammar was generated using MaiaOS's own published
MaiaCC submodule, with only this candidate lexical change:

```ebnf
IO_NUMBER ::= [0-9]+ & [<>]
```

The XML retains the `&` context item, but both generated JavaScript and native
C++98 scanners still emit IO_NUMBER for `255` in `echo 255 > out`, `echo 255`,
and `echo 255\t>out`, and for the target in `cat < 123`. The adjacent descriptor
in `echo 255>out` remains IO_NUMBER, as it should. Raw token spans were compared;
this is evidence about generated lexing, not command execution. Probe results
are in `/tmp/maia-io-number-context-probe.json` (temporary diagnostic evidence).

The JavaScript owner `parser-generator/grammar-parser.js`,
`parseContextExpression`, currently returns only LexicalSequence and ignores
the following context LexicalItem. The self-hosted C++98 owner
`parser-generator/cpp98/grammar-model.hpp`, `maiaCCModelChoices`, likewise builds
only that sequence. Fixing EBNF alone cannot correct either generator.

## Implemented change

1. In principal MaiaCC, retain the existing REx positive lexical context `&`
   as a typed assertion after the consuming sequence. Emit it as positive
   lookahead in both JavaScript and C++98 generator implementations, for both
   target languages. Reuse existing MaiaRegex assertion support; do not add
   shell-specific scanner code. Verify alternation, references, quantifiers,
   zero-width consumption, failure fallback and token preference semantics.
2. Keep the public grammar syntax and CLI unchanged. Regenerate MaiaCC-owned
   artifacts when required and verify JavaScript/self-hosted generation parity.
3. Validate, commit and publish MaiaCC before downstream consumption. Propagate
   through MaiaWASM, MaiaC, MaiaCpp, MaiaJS, MaiaGlob and MaiaOS using each
   repository's own published submodules, in protocol dependency order. Run the
   repository-local complete suites and the complete MaiaCC WASM bootstrap.
4. In MaiaOS, apply the candidate IO_NUMBER grammar change and regenerate every
   versioned shell frontend through the now-published in-repository MaiaCC.
   Verify AST construction and execution independently. Numeric words become
   WORD unless immediately followed by `<` or `>`; the assertion consumes no
   part of the operator. Retain rejection of explicit descriptor execution,
   which is outside the current executor profile.
5. Update docs and retain reproducible generated artifacts and shell.wasm under
   the owner's artifact policy. Preserve all existing local MaiaOS work.

No syscall, capability, kernel packet, execution profile, stream, VFS or LVGL
change. Reserved-word/assignment context, general POSIX compliance, fd duplication,
and explicit descriptor execution remain outside this increment. No versions,
npm, history rewrite or MaiaOS commit/push is included in the proposal.

The same review exposed empty EOF references: a new regression classified
`end!` as a guarded end-only token before correction. Both emitters now preserve
direct `$` and indirect EOF references as strict input-end assertions. They use
`(?![\s\S])` rather than JavaScript `$`, which also matches before a final
newline. Tests require fallback before `!` and before a final newline. The
automatic final EOF token remains unchanged.

## Acceptance cases

| Source | Required generated classification / behavior |
|---|---|
| `echo 255` | 255 is WORD. |
| `echo 255 > out` | 255 is WORD; ordinary stdout redirection. |
| `echo 255\t>out` | 255 is WORD; whitespace is not part of adjacency. |
| `echo 255>out` | 255 is IO_NUMBER; descriptor syntax parses but execution stays unsupported. |
| `echo '255'>out` | Quoted 255 is WORD. |
| `echo 2>&1` | 2 is IO_NUMBER, 1 is WORD; execution stays unsupported. |
| `cat < 123` | Numeric path is WORD and can be opened by the existing executor. |
| `echo 12x>out` | Complete 12x is WORD; no numeric-prefix split. |
| `printf '%d' 255 > /tmp/numbers` | Actual guest writes exactly the three bytes 255. |

Also cover escaped digits, adjacent/spaced `>>`, numeric output filenames,
EOF, newline, Unicode offsets, numeric operands in pipelines and unchanged
no-side-effect rejection of unsupported descriptor operations. Expected token
spans and AST ownership must be asserted, rather than relying on agreement
between two equally incorrect generators. Compare supported command/file bytes
with the native shell independently.

Required final gates: MaiaCC unit/integration tests, native C++98 and self-hosted
WASM generator parity; all affected downstream complete suites; MaiaOS
`test:shell-grammar`, `test:shell-parser`, expanded `test:shell-equivalence`,
strict `build:shell -- --check-c89`, `test:shell-redirection`,
`test:shell-pipelines`, `test:shell-lvgl`, `npm test`, and relevant boot/file/glob
regressions. Guest formatting/execution stays in C++98/WASM; I/O owns the terminal
and actual LVGL renders it. Interactive browser/device checks must be reported
separately from reference-WASM integration.

## Completed verification

- MaiaCC `npm run build`, then `npm test`: 116 tests pass. Native complete
  generations and both emitted languages match; context tests assert expected
  types, values and spans independently of cross-language equality.
- MaiaWASM `npm test`: 10 cases pass. MaiaC `npm run test:full`: full bundle
  and 13 extra diagnostics pass; `node tools/webc.js compiler/examples/test.c
  -o /tmp/maiac-context-final-cli --run` returns 0. Reviewed 36 versioned example
  manifests: timestamp changes only.
- MaiaCpp `npm test` and `npm run check:maiaCC-selfhost-wasm`: PASS. All three
  complete guest generations have identical frontend and generator binaries;
  both emitted languages match JS references on REx, XML, sample, WAT, shell
  and the added lexical-context fixture (including EOF). 27 real memory growths.
- MaiaJS `npm run test:full`: PASS, after all final gitlinks were committed.
  An earlier execution started during synchronization and was discarded as
  acceptance evidence; the complete suite was repeated. Reviewed 46 versioned
  manifests: timestamps and temporary input paths only.
- MaiaGlob `make verify`: ASan/UBSan, 10638 native cases, 10621 frontend program
  comparisons, 10638 native/WASM cases, walker/API checks and 500 repetitions
  stable at 18 pages. `node scripts/check-generated.mjs`: 5 artifacts reproduce
  exactly and 16 recursive pins match.
- MaiaOS `npm run test:shell-grammar`: independently specified classifications,
  exact spans and descriptor AST ownership pass. Only the separate reserved-word
  contextual gap remains in that suite. `node apps/shell/rtest.js --cpp`:
  56 JS/native parser-collector cases pass, regenerating versioned JS/C++/XML.
- `npm run test:shell-parser`: 64 raw-input JS/native cases pass; flat rule depth
  stays 11 as command length grows; no zero-progress recursion cycles.
  `npm run test:shell-equivalence`: 5704 identical JS/native C++98/WASM inputs,
  2748 accepted, 2136 syntax errors, 820 lexical errors. Exact tokens, UTF-16
  offsets and full trees; WASM stack restored.
- `npm run build:shell -- --check-c89`: own published in-repository compiler
  chain succeeds; shell.wasm is 1281412 bytes. The production C++98 header equals
  regenerated maia-shell.cpp byte for byte.
- `npm test`: 64 passed. `npm run test:shell-redirection`,
  `test:shell-pipelines`, `test:shell-lvgl`, `test:shell-boot`, `test:shell-files`,
  `test:shell-glob`, `test:shell-directories`: all pass. Actual generated guest
  execution agrees with native shell for spaced/quoted/escaped numeric operands;
  numeric names, truncate/append/input and cat/wc pipelines work. Unsupported
  descriptors preserve existing target bytes and open no handles. Real I/O/LVGL
  runs 1000 parses at shell 18 / I/O 2 memory pages with stable stacks and cleanup.
- Recursive publication audit: 34 index pins match clean checkouts, intended
  origins and remotely confirmed published main ancestry. Principal owners are
  clean and match origin/main. MaiaOS remains main b13f168 with its accumulated
  edits; staged cpp/glob pins updated, prior Regex pin preserved. No root commit.

Logs: `/tmp/maiacc-context-eof-final.log`,
`/tmp/maiacpp-context-final-{full,selfhost,binaries}.log`,
`/tmp/maiajs-context-final-full.log`, `/tmp/maiaglob-context-*.log`,
`/tmp/maiaos-context-*.log`, `/tmp/maia-context-publication-audit.log`.
Interactive browser/macOS keyboard, MaiaWASMVM and device execution were not run.
