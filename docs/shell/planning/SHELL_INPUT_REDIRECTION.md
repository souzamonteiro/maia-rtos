# Simple-command file input redirection

Scope: guest-only integration of the existing generated Redirection tree with
existing maia.vfs.open/read/close. No grammar, public ABI, manifest, kernel,
worker, host transport, compiler or execution-lifecycle changes are required.
The owner requested continued implementation. Publication is outside this step.

Allow one `< file` and optionally one `> file` or `>> file` on a simple command,
in any source position. Decode argv first and retain redirect source order as
bounded AST metadata. Expand all words using MaiaGlob before opening files;
each redirect target must expand to exactly one name. Both targets count toward
the existing twelve-word limit. Ambiguity and unsupported syntax cause no opens.

Open validated redirections left to right. An earlier output may therefore be
created/truncated before a later missing input fails; an earlier missing input
prevents a later output from opening. Failures do not roll back filesystem writes.
All builtins validate/open input redirects even when they do not read stdin.
Operand-free cat consumes this file as exact bytes; cat with file operands uses
those operands, leaving the redirected input unread. Bare cat retains its usage
error. Terminal command input remains owned by I/O and is never rebound.

Use bounded cooperative reads and the existing stdout backpressure path. Close
owned handles on EOF, read/write errors, failed later opens, exit and teardown.
Keep diagnostics/prompt on terminal streams; ensure ignored input also closes.

Repeated redirects in the same direction, numbered descriptors, heredocs,
descriptor duplication, redirect-only commands and `<` in pipelines remain
unsupported and fail before opening files. Standalone wc remains unsupported.

Verification: strict C++98 -> C89 -> WASM shell build; actual guest regression
tests for binary/empty/quoted/glob/relative/private paths, source ordering,
ambiguity, word limits, errors, backpressure, cleanup, EOF and stable memory;
production boot/kernel/I/O/LVGL integration; generated frontend equivalence and
existing shell regression/unit suites. Interactive browser verification is
separate and must not be inferred from Node/LVGL integration.
