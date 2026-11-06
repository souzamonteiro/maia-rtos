# WC count selection

2026-10-23. The owner reported rejected wc -l, then explicitly requested proceeding
after the diagnosis. This authorizes the bounded option extension below; no
additional publication, dependency or execution-lifecycle changes are included.
Status: implemented and validated locally through native C++98 ASan/UBSan,
actual WASM workers, kernel and production boot/I/O/LVGL integration.

Accept -l (LF lines), -w (ASCII-whitespace words), -c (bytes) on the fixed final
wc worker. Support grouped, separate and repeated options; output selected fields
once in canonical lines/words/bytes order regardless of option order. Default wc
retains all three counts. Separate fields with one space and finish with LF.
Reject empty arguments, bare -, --, long/unknown options and file operands.
Standalone wc and wc in earlier pipeline positions remain outside this slice.

Preserve checked u32 counting/overflow, existing word/packet/buffer limits, stdin
stream-only profile, manifests, v1/v2/v3 wire formats, START index, namespaces,
scheduler, cancellation, pipes and real I/O/LVGL responsibilities. No syscalls,
capabilities, grammar/generated parser edits, host execution or compiler changes.

Use a bounded C++98 option-mask helper shared by worker, shell and kernel to
avoid divergent acceptance. Worker independently validates tightly packed copied
argv before reading. Shell validates all options before path normalization,
target opens or launch; report `wc: expected -l, -w or -c options only` for invalid
wc arguments. Kernel independently rejects invalid final-wc argv before resource
reservation. Cat operands retain existing path semantics.

Modify apps/wc, the shell pipeline adapter and kernel validation; update native,
WASM, kernel and actual production boot/I/O/LVGL regressions and documentation.
Rebuild wc/kernel/shell using own published pins with --check-c89, retaining their
artifacts. Verify all seven option masks, argument order/grouping/repetition,
packet corruption/truncation, overflow, UTF-8/binary inputs, backpressure,
v1/v2/v3 selection, rejection before effects, cleanup and existing behavior.
No commit/push, npm or version changes. Browser/device/MaiaWASMVM checks remain
separate from Node native-reference integration.
