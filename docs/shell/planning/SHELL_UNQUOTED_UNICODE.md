# Unquoted Unicode words in the generated shell frontend

Completed and reviewed: **2026-10-24**. MaiaCC fix/publication and tested consumer
propagation explicitly authorized by the owner. MaiaOS changes remain local;
no npm, version changes or history rewriting.

## Result and contract

`cat /system/tests/data/ação.txt` and unquoted/escaped non-ASCII arguments now
work through the generated shell frontend. EBNF `UnicodeChar` includes scalars
U+0080–U+D7FF and U+E000–U+10FFFF in UnquotedChar and EscapedChar. ASCII shell
operators, separators, IO_NUMBER contexts and resource bounds are unchanged.
No normalization, case folding or Unicode whitespace splitting: VFS names and
argv preserve exact UTF-8 bytes. Font coverage and grapheme editing are separate
I/O limits; script-file execution is still pending.

Owner correction in MaiaCC: the JS and C++98 models formerly dropped numeric
literals/ranges above U+FFFF. Both now lower supplementary class intervals to
at most three surrogate rectangles per interval under the existing UTF-16
regex contract. Mixed/negated classes consume full scalars; BMP-only classes
retain their established representation. Invalid/reversed numeric ranges fail
explicitly. The REx Char EBNF groups raw supplementary pairs before single
units, preventing split halves in XML. Generated default REx parser/XML and
all affected consumer artifacts were regenerated; no handwritten scanner,
manual generated edits, host matching or command execution was introduced.

## Validation

- [x] `/bin/sh -n`: 113 independent syntax expectations, including two existing
  contextual reserved-word gaps kept explicit. Eleven new Unicode word/span
  cases promoted from the proposal into shell-grammar-scenarios.json.
- [x] `node apps/shell/rtest.js --cpp`: 114 JS/native accepted-tree comparisons.
- [x] `npm run test:shell-parser`: 122 raw JS/native cases and 27 recursion
  profiles. Previous depth/call/no-zero-progress-cycle gates unchanged.
- [x] `npm run test:shell-equivalence`: 5,833 exact JS/native C++98/WASM inputs;
  2,856 accepted, 2,167 syntax errors, 810 lexical errors; identical tokens,
  UTF-16 offsets/full trees and restored stack. Includes BMP/astral boundaries,
  escapes, adjacent redirections and supplementary characters at EOF.
- [x] `npm run build:shell -- --check-c89`: own published submodules, strict
  C++98 -> C89 -> WASM. Artifact 1,290,757 bytes; generated production header
  equals versioned maia-shell.cpp byte-for-byte.
- [x] `npm test`: 66/66. `test:shell-files`, `test:shell-glob`,
  `test:shell-directories`, `test:shell-redirection` all pass. Exact unquoted
  UTF-8 bytes, escaped characters, combining-vs-precomposed filenames, NBSP as
  a literal word character, supplementary filenames and glob all validated.
- [x] `test:shell-pipelines`: actual kernel/cat/wc/I/O/LVGL, unquoted native
  composition-event fixtures, file/input/output redirection and all 22 recipe
  commands twice; image immutable, resources reaped, 103 flushes.
- [x] `test:shell-lvgl`: 1,000 repeated parses, stable stack, shell 19 / I/O 2
  pages, 2,091 flushes and 77,911 kernel steps. Existing shell budget remains
  32 pages. `test:shell-boot`: 25 sessions and 28 flushes, clean lifecycle.
- [x] 34 recursive gitlinks match clean nested checkouts, intended remote URLs
  and actual published main ancestry. Logs: /tmp/maia-unicode-publication-audit.log.
- [x] HTTP smoke: existing localhost:8080 server serves the rebuilt shell.wasm
  and both updated recipe files byte-for-byte. Binary SHA-256:
  `4ea8fde8bc75c631da491ececdf469feb617e1732e00e2021df4004ef340fda1`.
  All seven affected principal checkouts are clean and HEAD equals live remote
  main; the MaiaOS working tree remains intentionally uncommitted.

Other logs: /tmp/maiaos-unicode-*.log. Before-fix evidence:
/tmp/maiaos-unquoted-unicode-before.log. The withdrawn proposal files were
promoted into active fixtures; the principal MaiaCC owns the reusable
unicode-scalars.ebnf regression. Browser interaction, MaiaWASMVM and device
conformance were not run. These gates exercise reference WASM and actual LVGL.

## Published propagation

| Principal | Published main | Validation |
|---|---|---|
| MaiaCC | df4c3b5 | Regenerated REx; 117 tests, native bootstrap/source parity and Unicode acceptance/offset/invalid-range regressions. |
| MaiaWASM | bfd2bc2 | WAT regenerated; 10 fixtures, 41 assembler modules, 6 proposal modules, static linker and exact Unicode data bytes. |
| MaiaC | 93d339f | Parser regenerated; full bundle +13 extras; freshly compiled test.c WAT/WASM validated and versioned. |
| MaiaCpp | 04fc13a | Standard suite; complete MaiaCC WASM bootstrap generations 0/1/2, both emitters on seven grammars including Unicode, invalid numeric inputs; 27 real memory growths. |
| MaiaJS | 03f465e | Parser regenerated using own native REx and local tREx XML emitter; 379 tests and 33 fixtures. |
| MaiaGlob | 3498282 | Sanitizers, 10,638 native/WASM comparisons and 500 stable guest repetitions. |
| MaiaSQL | 316fe24 | Native MaiaCC parser regeneration, identical JS emission from same XML, 39 runtime/parser tests pass; one optional test skipped by default. |

MaiaSQL's explicitly attempted `test:parser:slow` exceeded its 600000 ms
JavaScript-frontend timeout. This known regeneration-performance limitation is
not marked passing. The existing parallel C++98 frontend/emitter completed the
same EBNF regeneration; JS emission from its XML is byte-identical, and the
runtime suite passes on that regenerated parser. Details/reproduction are in
MaiaSQL README. No timeout was raised or test weakened to conceal it.

MaiaOS main remains b13f168 with prior local work intact; its staged cpp/glob
pins consume the published commits above. No MaiaOS commit/push is authorized
for this increment. Next: reload the simulator, launch the shell and try
`cat /system/tests/data/ação.txt` or the updated recipe commands individually.
