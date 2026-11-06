# Bounded printf integer conversions

> Historical milestone below. The [2026-10-26 byte-argument extension](SHELL_PRINTF_BYTE_ARGUMENTS.md)
> adds bounded %b/%c and argument-only backslash-c; its measurements and tests
> supersede those exclusions without changing the earlier integer/string behavior.


Status: implemented and validated on 2026-10-24. shell.wasm is 1281284 bytes;
initial memory remains 18 pages (I/O 2). No host ABI, capability, execution model,
grammar/generated frontend or rendering-boundary change. MaiaOS remains local
and unpublished at main b13f168; all compiler/dependency publications below were
explicitly authorized. No npm, version changes or history rewrite.

## Implemented profile

Standalone printf supports %d/%i (signed 32-bit decimal), %u (unsigned decimal),
%o (octal), %x/%X (hexadecimal), existing %s/%% and byte escapes. Format reuse and
missing arguments work with mixed conversions. Missing/empty numeric values are
zero; this explicit-empty policy is checked directly because macOS native printf
also emits a diagnostic for an empty numeric string. Differential comparisons use
the shared supported native range/forms without silently discarding diagnostics.

Numeric operands are complete ASCII literals: optional sign, decimal by default,
leading-zero octal or 0x/0X hexadecimal. Reject bare signs/prefixes, whitespace,
trailing junk, invalid digits and quoted character constants. Signed range is
-2147483648..2147483647; unsigned 0..4294967295, rejecting negative signs even -0.
Overflow/invalid input produces stderr and zero command stdout. Existing redirects
still open before builtin validation and can leave a target truncated.

Unsigned magnitude accumulation checks before multiply/add. INT32_MIN needs no
signed negation. At most 11 local reverse-digit bytes format a number. Validation
and measurement precede a second pass over immutable words into the existing
output queue, under the 2048-byte result bound. No dedicated result buffer or
host formatting/arithmetic. Floating point, width/precision, length modifiers,
locale, %b/%c and printf pipeline-worker execution remain unsupported.

## Owner corrections and protocol

| Principal | Published commit | Change |
|---|---|---|
| MaiaC | 47c357e | Typed unsigned /=, %=, >>=; promoted/mixed-width operands. |
| MaiaCpp | fe92e64 | Consume MaiaC; preserve %= instead of forcing int casts. |
| MaiaJS | 9ec3ae0 | Tested dependency pins and regenerated versioned examples. |
| MaiaGlob | 8f54df5 | Tested MaiaCpp pin and validation record. |

Only principal owner repositories were edited. Consumers pulled published
commits through their own submodules, recorded gitlinks, tested and published
in dependency order. MaiaOS consumed the published pins without committing or
pushing its accumulated work. A recursive audit verifies 34 gitlinks: matching
checkout/index, expected remote URLs, published origin/main ancestry and clean
tracked submodule contents. Package versions remain unchanged.

Original reproduction: UINT_MAX /=10 returned 0, %=10 returned UINT_MAX, >>=1
returned UINT_MAX. Native Clang C89 was correct. MaiaC used signed instructions
for compound operations; ordinary division already worked. After that fix,
a C++98 replay exposed MaiaCpp's separate forced-int %= rewrite. Both owners
now have focused regressions; no shell arithmetic workaround was introduced.
The standalone C89 fixture and scripts/test-unsigned-compound.mjs now PASS using
MaiaOS's own pinned compiler, with unsigned WAT instructions and exact results.

## Verification — integer milestone, before the IO_NUMBER follow-up

- MaiaC: 22 native C89/WASM cases plus exact i64/mixed-width checks; full bundle
  and 13 extra diagnostics; CLI compiler/examples/test.c builds/runs with return 0.
- MaiaCpp: 9 native C++98/strict C89/actual WASM cases, standard npm test and
  complete MaiaCC self-hosting after the final fix: three stable generations,
  both emitters, exact REx/XML/sample/WAT/shell parity and 27 memory growths.
- MaiaJS: 379 unit tests plus full example build/run/course/distribution suite.
- MaiaGlob: make verify with ASan/UBSan, 10638 native cases, 10621 frontend program
  comparisons, 10638 native/WASM cases, real walker/API checks and 500 repetitions
  stable at 18 pages. All 5 generated sources reproduce; 16 recursive pins match.
- MaiaOS: strict C++98 -> C89 -> WASM builds for shell/kernel/I/O/cat/wc, rebuilt
  Calculator/Clock; 64 unit tests; node scripts/test-printf-integers-native.mjs
  (C++98 ASan/UBSan); focused unsigned reproduction; test:shell-redirection native
  differential bounds/bases/mixed/reused/missing/empty/seeded cases, rejection
  atomicity, partial VFS writes/WOULD_BLOCK, truncate/append/EOF/errors; real
  test:shell-lvgl and test:shell-pipelines. Numeric file -> cat -> wc yields
  exactly 1 3 26 + LF. All stream/boot/directory/glob/file/kernel/worker/wc/LVGL app
  regressions pass. Generated shell parser/grammar unchanged; git diff --check.
- `npm run test:shell-equivalence`: 5356 identical raw inputs through generated
  JavaScript, native C++98 and actual WASM: 2400 accepted, 2136 syntax errors,
  820 lexical errors, exact tokens/UTF-16 offsets/complete trees and restored
  WASM stack.

## IO_NUMBER follow-up completed — 2026-10-24

The initially recorded IO_NUMBER adjacency limitation is now corrected through
the published MaiaCC generators and regenerated shell frontend. A spaced
decimal operand before > is an ordinary word; no quoting workaround is needed:

```sh
printf '%d %u %X\n' -2147483648 4294967295 255 > /tmp/numbers
cat < /tmp/numbers | cat | wc
```

Expected count: 1 3 26. This exact unquoted form passes actual guest/LVGL pipeline
integration. Adjacent explicit descriptors remain unsupported and must reject
without opening files. The newer shell.wasm is 1281412 bytes, still 18 pages.
See [the completed context correction](SHELL_IO_NUMBER_CONTEXT.md) for commits,
5704-case parity and execution evidence. Interactive browser/macOS keyboard,
MaiaWASMVM and physical-device conformance were not run.
