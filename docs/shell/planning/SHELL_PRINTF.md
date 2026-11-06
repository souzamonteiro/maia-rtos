# Bounded string printf builtin

> Historical milestone below. The [2026-10-26 byte-argument extension](SHELL_PRINTF_BYTE_ARGUMENTS.md)
> adds bounded %b/%c and argument-only backslash-c; its measurements and tests
> supersede those exclusions without changing the earlier integer/string behavior.


> This records the initial string-only slice. The implemented and WASM-validated
> [integer extension](SHELL_PRINTF_INTEGERS.md) supersedes the numeric exclusions
> and artifact measurement below. Its compiler corrections are published and
> consumed through tested gitlinks.

Status: implemented and validated locally. shell.wasm is 1278822 bytes with
unchanged 18-page initial memory. Strict guest build, native printf comparison,
actual kernel/I/O/shell/LVGL integration, binary file -> cat -> wc pipeline and
existing regressions pass. Interactive browser, MaiaWASMVM and device execution
remain separate checks.

2026-10-23. Add a ninth builtin to the existing C++98 shell dispatcher, using
generated command words and existing stream/VFS output. This is string printf,
not full POSIX printf. No new host ABI, capabilities, process execution model,
grammar, parser, compiler/dependency or rendering changes.

Syntax: printf [--] FORMAT [ARG ...]. Support %s, %% and format escapes \\, \a,
\b, \f, \n, \r, \t, \v and one-to-three octal digits after a backslash (byte
0..255). %s copies its argument bytes literally, never interpreting escapes or
format directives inside data. Missing string arguments are empty. Reuse FORMAT
until all string arguments are consumed; a format without %s runs once and
ignores surplus arguments. No implicit newline. Optional leading -- disambiguates
the format '--'; other dash-prefixed formats are literal, with no other options.

Reject dangling %, unknown conversions/escapes, dangling backslash and octal
values above 255 with stderr diagnostic and zero command stdout. Numeric/%b/%c,
width, precision, locale, \c, hexadecimal and Unicode escapes remain unsupported.
Validate/measure the entire bounded result before enqueueing any output. Use two
passes over immutable words, with no dedicated result buffer: after validation
and capacity checks, emit to the existing pending output queue. A checked 2048-byte
result limit exceeds the 1900-byte upper bound under current decoded word/argv
limits (95-byte format, at most 10 arguments).
Binary NUL bytes use explicit output lengths, not strlen.

Existing expansions/quoting and redirect opening order apply: invalid formatting
can leave a redirect target truncated, while diagnostics stay on stderr. Stream
and VFS partial writes/WOULD_BLOCK resume through the existing pending queue;
errors preserve accepted bytes and release handles. printf is standalone builtin
only: it does not become an approved kernel pipeline worker.

Reference semantics for the supported subset:
https://www.gnu.org/software/coreutils/manual/html_node/printf-invocation.html
Validate actual shell WASM output against /usr/bin/printf for supported cases;
explicitly test rejected features, quoted UTF-8/literal data, repeated formats,
missing arguments, octal binary, maximum words/output, redirection/truncation/
append, tiny-ring backpressure, partial writes/errors, EOF and closed peers.
Keep real I/O/kernel/LVGL and existing shell regressions. Rebuild shell.wasm
through MaiaCpp AST-strict -> checked C89 -> MaiaC/MaiaWASM. Preserve all local
work and artifact policy; no commit/push, versions or npm actions.
