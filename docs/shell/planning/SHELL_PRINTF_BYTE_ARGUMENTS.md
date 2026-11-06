# Bounded printf byte arguments

2026-10-26. Extend the existing standalone guest printf with %b and %c, through
the generated argv and existing stream/VFS queue. No new syscall, capability,
parser/grammar, kernel worker, execution model, compiler or publication changes.

%b interprets argument escapes: \\, \a, \b, \f, \n, \r, \t, \v and \0 followed
by zero to three octal digits. Values above 255, unknown escapes and trailing
backslash fail with the existing format diagnostic and zero command stdout.
Plain octal escapes without the leading zero, hex/Unicode escapes and \e remain
outside this bounded argument profile. Format-string escapes retain the existing
one-to-three-digit octal syntax. %s data remain literal.

Argument \c stops the entire printf command: remaining argument bytes, format
tail and format reuse are ignored. Validation and emission follow the same
stopping point. Invalid input before it still fails atomically; ignored tails
are not validated. There is no shell exit or change to the next command.

%c copies the first argument byte, without locale/grapheme decoding. Missing or
empty arguments emit no bytes as an explicit local policy; these cases are
checked directly rather than compared with Bash, which emits NUL for them.
This is not a Unicode character conversion; %s/%b preserve whole UTF-8 strings.
Both conversions consume one argument per occurrence and support format reuse.
Existing two-pass validation, 2048-byte bound and pending output queue remain.
Redirects still open before builtin validation and can truncate on failure.

Validation gates:
- [x] Native C++98 ASan/UBSan, exact bytes/bounds/stop/reuse/atomicity.
- [x] Differential native Bash printf under LC_ALL=C on the shared supported subset;
  explicit local expectations for empty %c and deliberately rejected extensions.
- [x] Strict C++98 -> C89 -> MaiaC/MaiaWASM production build and actual guest tests.
- [x] Binary VFS output, append, partial writes/WOULD_BLOCK, EOF/closed-peer cleanup.
- [x] Real kernel/I/O/shell/cat/wc/LVGL integration and simulator recipe.
- [x] Update progress/handoff; no commit/push, npm or versions.

Reference: https://www.gnu.org/software/coreutils/manual/html_node/printf-invocation.html
This is the project's declared bounded byte profile, not full POSIX printf.
Darwin's external /usr/bin/printf truncates %b at embedded NUL. Its pre-existing
text/integer differential tests remain; new binary comparisons use the Bash
builtin with constant command text and argv forwarding, plus independent byte
expectations and an exhaustive 0..255 octal sweep. No expected bytes are dropped.

## Completed validation — 2026-10-26

- `node scripts/test-printf-integers-native.mjs`: C++98 ASan/UBSan passes,
  existing integer cases retained, exact byte expectations and native Bash
  LC_ALL=C comparison; 2048/over-bound/stop-at-bound measurement and malformed
  argument atomicity pass. Missing/empty %c policy is explicitly checked.
- `npm run build:shell -- --check-c89`: own published toolchain; shell.wasm
  1,292,086 bytes. No grammar/generated parser changes; production generated
  header equals the versioned comparison C++ source.
- `npm run test:shell-redirection`: actual guest queue/VFS; exhaustive0..255
  octal output, mixed/repeated/missing arguments, stop/ignored tails, escaped
  literal backslash-c, binary append, 3-byte/WOULD_BLOCK VFS transfers, invalid
  argument truncation ordering, EOF and closed-peer cleanup; stable stack/memory.
- `npm run test:shell-pipelines`: actual kernel/I/O/generated shell/cat/wc/LVGL,
  binary six-byte file counted through a pipe, no-implicit-LF terminal output,
  all22 recipe commands twice, resources reaped;110 LVGL flushes.
- `npm run test:shell-lvgl`: help advertises conversions;1000 repeated parses,
  stable shell19/I/O2 pages,2121 flushes,77911 kernel steps.
- `npm run test:shell-boot`:25 sessions/28flushes pass. `npm test`:66 pass.
  `npm run test:shell-files`: existing file/Unicode/binary/backpressure suite passes.
- `git diff --check`: pass. Logs /tmp/maiaos-printf-bytes-*.log.

The first new terminal assertion assumed all output was in the history label.
With no final LF, correct existing I/O behavior keeps `|Z` beside `maia> ` in
its live-line label. The fixture now explicitly asserts both labels and an
exact expected live line; all old exact-prompt assertions remain unchanged.
No newline was added to the builtin and no rendering policy was changed.

The 2026-10-24 frontend equivalence/grammar gates are retained historical
results, not rerun here: EBNF/parser sources unchanged. Kernel/I/O/native
sanitizer policy implementations and compilers are unchanged. No browser
interaction, MaiaWASMVM or physical-device acceptance claimed.
MaiaOS remains main/b13f168 with prior dirty work and staged dependency pins
preserved. No commit/push, package versions, npm or history rewriting.

HTTP smoke passed on the existing localhost:8080 server: current shell.wasm
and updated 02-streams.sh are byte-exact. Shell SHA-256:
6701ac5ebed247eec3aeabaf67747a1cb8978a035052be6c34339eaead60681f.
The server was not restarted or replaced; interactive browser input was not run.
